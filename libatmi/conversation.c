/* 
** Implements main functions for conversational server
** Some thoughts:
** 1. Control is on that side which sends the message.
** 2. ?
**
** @file conversation.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, ATR Baltic, SIA. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or ATR Baltic's license for commercial use.
** -----------------------------------------------------------------------------
** GPL license:
** 
** This program is free software; you can redistribute it and/or modify it under
** the terms of the GNU General Public License as published by the Free Software
** Foundation; either version 2 of the License, or (at your option) any later
** version.
**
** This program is distributed in the hope that it will be useful, but WITHOUT ANY
** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
** PARTICULAR PURPOSE. See the GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License along with
** this program; if not, write to the Free Software Foundation, Inc., 59 Temple
** Place, Suite 330, Boston, MA 02111-1307 USA
**
** -----------------------------------------------------------------------------
** A commercial use license is available from ATR Baltic, SIA
** contact@atrbaltic.com
** -----------------------------------------------------------------------------
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <sys/stat.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <atmi_int.h>
#include <typed_buf.h>

#include "../libatmisrv/srv_int.h"
#include <thlock.h>
#include <xa_cmn.h>
#include <tperror.h>
#include <atmi_shm.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define CONV_TARGET_FLAGS(X)     \
    do\
    {\
        /* Fix up send/receive flags */\
        if (X->flags & TPSENDONLY)\
        {\
            X->flags&=~TPSENDONLY;\
            X->flags|=TPRECVONLY;\
        }\
        else if (X->flags & TPRECVONLY)\
        {\
            X->flags&=~TPRECVONLY;\
            X->flags|=TPSENDONLY;\
        }\
    } \
    while (0)
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
int M_had_open_con = FALSE;
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
private mqd_t open_conv_q(char *q,  struct mq_attr *q_attr);
private mqd_t open_reply_q(char *q, struct mq_attr *q_attr);

/**
 * Fix queue attributes to match the requested mode.
 * @param conv
 * @param flags
 * @return SUCCEED/FAIL
 */
private int setup_queue_attrs(struct mq_attr *p_q_attr,
                                mqd_t listen_q,
                                char *listen_q_str, 
                                long flags)
{
    int ret=SUCCEED;
    int change_flags = FALSE;
    struct mq_attr new;
    char fn[] = "setup_queue_attrs";

    NDRX_LOG(log_debug, "ATTRS BEFORE: %d", p_q_attr->mq_flags);

    if (flags & TPNOBLOCK && !(p_q_attr->mq_flags & O_NONBLOCK))
    {
        /* change attributes non block mode*/
        new = *p_q_attr;
        new.mq_flags |= O_NONBLOCK;
        change_flags = TRUE;
        NDRX_LOG(log_debug, "Changing queue [%s] to non blocked",
                                            listen_q_str);
    }
    else if (!(flags & TPNOBLOCK) && (p_q_attr->mq_flags & O_NONBLOCK))
    {
        /* change attributes to block mode */
        new = *p_q_attr;
        new.mq_flags &= ~O_NONBLOCK; /* remove non block flag */
        change_flags = TRUE;
        NDRX_LOG(log_debug, "Changing queue [%s] to blocked",
                                            listen_q_str);
    }

    if (change_flags)
    {
        if (FAIL==mq_setattr(listen_q, &new,
                            p_q_attr))
        {
            _TPset_error_fmt(TPEOS, "%s: Failed to change attributes for queue [%s] fd %d: %s",
                                fn, listen_q_str, listen_q, strerror(errno));
            ret=FAIL;
            goto out;
        }
        else
        {
            /* Save new attrs */
            *p_q_attr = new;
        }
    }

    NDRX_LOG(log_debug, "ATTRS AFTER: %d", p_q_attr->mq_flags);
    
out:
    return ret;
}

/**
 * Closes any connection made as client.
 * This will send disconnect event to them, because normally server should do
 * tpreturn!
 * Try to close all queues, if have some error, report it
 */
public int close_open_client_connections(void)
{
    int i;
    int ret=SUCCEED;

    /* nothing to do, we do not have opened any client connections! */
    if (!M_had_open_con)
    {
        return SUCCEED;
    }

    for (i=0; i<MAX_CONNECTIONS; i++)
    {
        if (CONV_IN_CONVERSATION==G_tp_conversation_status[i].status)
        {
            if (FAIL==_tpdiscon(i))
            {
                NDRX_LOG(log_warn, "Failed to close connection [%d]", i);
                ret=FAIL;
            }
        }
    }
    M_had_open_con = FALSE;
    
    return ret;
}


/**
 * Return TRUE if we have any open connection
 * @return TRUE/FALSE
 */
public int have_open_connection(void)
{
    int i;
    int ret=FALSE;
    /* nothing to do, we do not have opened any client connections! */
    if (!M_had_open_con)
    {
        return FALSE;
    }

    for (i=0; i<MAX_CONNECTIONS; i++)
    {
        if (CONV_IN_CONVERSATION==G_tp_conversation_status[i].status)
        {
            ret=TRUE;
            break;
        }
    }

    NDRX_LOG(log_debug, "We % open connections!",
                            ret?"have":"do not have");

    return ret;
    
}


/**
 * This assumes that we have already set last call data.
 * TODO: Error if already in conversation?
 */
public int accept_connection(void)
{
    int ret=SUCCEED;
    tp_conversation_control_t *conv = &G_accepted_connection;
    char fn[] = "accept_connection";
    long revent;
    int q_opened=FALSE;
    char their_qstr[NDRX_MAX_Q_SIZE+1];
/*
    char send_q[NDRX_MAX_Q_SIZE+1]; */

    conv->flags = G_last_call.flags; /* Save call flags */
    
    /* Fix up cd for future use for replies.  */
    conv->cd = G_last_call.cd - MAX_CONNECTIONS;
    /* Change the status, that we have connection open */
    conv->status = CONV_IN_CONVERSATION;
    /* 1. Open listening queue */
    sprintf(conv->my_listen_q_str, NDRX_CONV_SRV_Q,
                    G_atmi_conf.q_prefix, G_last_call.my_id, G_last_call.cd-MAX_CONNECTIONS,
                    G_atmi_conf.my_id); /* In accepted connection we put their id */
    
    /* TODO: Firstly we should open the queue on which to listen right? */
    if ((mqd_t)FAIL==(conv->my_listen_q =
                    open_conv_q(conv->my_listen_q_str, &conv->my_q_attr)))
    {
        NDRX_LOG(log_error, "%s: Failed to open listen queue", fn);
        ret=FAIL;
        goto out;
    }
    q_opened=TRUE;

    /* 2. Connect to their reply queue */
    strcpy(conv->reply_q_str, G_last_call.reply_to);
    
    /* Check is this coming from bridge. If so the we connect to bridge */
    
    if (0!=G_last_call.callstack[0])
    {
        br_dump_nodestack(G_last_call.callstack, "Incoming conversation from bridge,"
                                           "using first node from node stack");
        sprintf(their_qstr, NDRX_SVC_QBRDIGE, G_atmi_conf.q_prefix, 
                (int)G_last_call.callstack[0]);
    }
    else
    {
        /* Local conversation  */
        strcpy(their_qstr, conv->reply_q_str);
    }
    
    NDRX_LOG(log_debug, "Connecting to: [%s]", their_qstr);
    
    if ((mqd_t)FAIL==(conv->reply_q=open_reply_q(their_qstr, &conv->reply_q_attr)))
    {
        NDRX_LOG(log_error, "%s: Cannot connect to reply queue [%s] - "
                                        "cannot accept connection!", 
                                        fn, their_qstr);
        ret=FAIL;
        goto out;
    }

    /* 3. Send back reply to their queue */
    NDRX_LOG(log_debug, "About to send handshake back to client...");
    if (SUCCEED!=_tpsend(G_last_call.cd, NULL, 0, 0, &revent,
                            ATMI_COMMAND_CONNRPLY))
    {
        NDRX_LOG(log_error, "%s: Failed to reply for acceptance!");
        ret=FAIL;

    }
    
out:

    /* Close down the queue if we fail but queue was opened! */
    if (SUCCEED!=ret && q_opened)
    {
        if (FAIL==mq_close(conv->my_listen_q))
        {
            NDRX_LOG(log_warn, "Failed to close %s:%s",
                        conv->my_listen_q_str, strerror(errno));
        }
    }

    if (SUCCEED==ret)
    {
        conv->handshaked=TRUE;
    }

    NDRX_LOG(log_debug, "%s: returns %d", fn, ret);
    return ret;
}

/**
 * Return current connection from given cd
 * @param cd
 * @return NULL/connection control
 */
public tp_conversation_control_t*  get_current_connection(int cd)
{
    tp_conversation_control_t *ret=NULL;
    char fn[] = "get_current_connection";

    if (cd>=0 && cd<MAX_CONNECTIONS)
    {
        ret=&G_tp_conversation_status[cd];
    }
    else if (cd>=MAX_CONNECTIONS)
    {
        ret=&G_accepted_connection;
    }
    else
    {
        _TPset_error_fmt(TPEINVAL, "%s: Invalid connection descriptor %d", fn, cd);
    }

    if (NULL!=ret && CONV_IN_CONVERSATION!=ret->status)
    {
        _TPset_error_fmt(TPEINVAL, "%s: Invalid connection descriptor %d - "
                                        "connection closed", fn, cd);
        ret=NULL;
    }

    return ret;
}

/**
 * Close connection (Normally!)
 * @return SUCCEED/FAIL
 */
public int normal_connection_shutdown(tp_conversation_control_t *conv, int killq)
{
    int ret=SUCCEED;
    char fn[] = "normal_connection_shutdown";

    NDRX_LOG(log_debug, "%s: Closing [%s]", fn, conv->my_listen_q_str);

    /* close down the queue */
    if (SUCCEED!=mq_close(conv->my_listen_q))
    {
        NDRX_LOG(log_warn, "%s: Failed to mq_close [%s]: %s",
                                        fn, conv->my_listen_q_str, strerror(errno));
        _TPset_error_fmt(TPEOS, "%s: Failed to mq_close [%s]: %s",
                                        fn, conv->my_listen_q_str, strerror(errno));
       /* ret=FAIL;
        goto out; */
    }
    
    /* Remove the queue */
    if (killq && SUCCEED!=mq_unlink(conv->my_listen_q_str))
    {
        NDRX_LOG(log_warn, "%s: Failed to mq_unlink [%s]: %s",
                                        fn, conv->my_listen_q_str, strerror(errno));
        _TPset_error_fmt(TPEOS, "%s: Failed to mq_unlink [%s]: %s",
                                        fn, conv->my_listen_q_str, strerror(errno));
        /* ret=FAIL;
        goto out; */
    }
    
    /* Kill the reply queue too? */
    
    NDRX_LOG(log_debug, "%s: Closing [%s]", fn, conv->reply_q_str);

    /* close down the queue */
    if (SUCCEED!=mq_close(conv->reply_q))
    {
        NDRX_LOG(log_warn, "%s: Failed to mq_close [%s]: %s",
                                        fn, conv->reply_q_str, strerror(errno));
        _TPset_error_fmt(TPEOS, "%s: Failed to mq_close [%s]: %s",
                                        fn, conv->reply_q_str, strerror(errno));
       /* ret=FAIL;
        goto out; */
    }
    
    /* Remove the queue */
    NDRX_LOG(log_warn, "UNLINKING: %s %d", conv->reply_q_str, killq);
    if (killq && SUCCEED!=mq_unlink(conv->reply_q_str))
    {
        NDRX_LOG(log_warn, "%s: Failed to mq_unlink [%s]: %s",
                                        fn, conv->reply_q_str, strerror(errno));
        _TPset_error_fmt(TPEOS, "%s: Failed to mq_unlink [%s]: %s",
                                        fn, conv->reply_q_str, strerror(errno));
        /* ret=FAIL;
        goto out; */
    }
    
    /* At this point we reset the CD - free the CD!!  */
    /* Unregister CD from global tx */
    if (G_atmi_xa_curtx.txinfo)
    {
        /* try to unregister... anyway (even was called with TPNOTRAN)
         * will not find in hash, that's it... 
         */
        atmi_xa_cd_unreg(&(G_atmi_xa_curtx.txinfo->conv_cds), conv->cd);
    }
    
    memset(conv, 0, sizeof(*conv));
    
out:
    return ret;
}

/**
 * Get free connection descriptor
 * This function should be synced, so that
 * each thread have it's own cd.
 * @return
 */
private int conv_get_cd(long flags)
{

    static __thread int cd=1; /* first available */
    int start_cd = cd; /* mark where we began */

    while (CONV_NO_INITATED!=G_tp_conversation_status[cd].status)
    {
        cd++;

        if (cd > MAX_CONNECTIONS-1)
        {
            cd=1; /* TODO: Maybe start with 0? */
        }

        if (start_cd==cd)
        {
            NDRX_LOG(log_debug, "Connection descritors overflow restart!");
            break;
        }
    }

    if (CONV_NO_INITATED!=G_tp_conversation_status[cd].status)
    {
        NDRX_LOG(log_debug, "All connection descriptors have been taken - FAIL!");
        cd=FAIL;
    }
    else
    {
        NDRX_LOG(log_debug, "Got free connection descriptor %d", cd);
    }
    
    if (FAIL!=cd && !(flags & TPNOTRAN) && G_atmi_xa_curtx.txinfo)
    {
        NDRX_LOG(log_debug, "Registering conv cd=%d under global "
                "transaction!", cd);
        if (SUCCEED!=atmi_xa_cd_reg(&(G_atmi_xa_curtx.txinfo->conv_cds), cd))
        {
            cd=FAIL;
        }
    }

    return cd;
}

/**
 * Open the queue on which we are going to listen for the stuff...
 * @param q
 * @return SUCCEED/FAIL
 */
private mqd_t open_conv_q(char *q,  struct mq_attr *q_attr)
{
    mqd_t ret=(mqd_t)FAIL;
    char fn[] = "open_conv_q";

    ret = ndrx_mq_open_at (q, O_RDWR | O_CREAT, S_IWUSR | S_IRUSR, NULL);

    if ((mqd_t)FAIL==ret)
    {
        _TPset_error_fmt(TPEOS, "%s:Failed to open queue [%s]: %s", fn, q, strerror(errno));
        goto out;
    }

        /* read queue attributes */
    if (FAIL==mq_getattr(ret, q_attr))
    {
        _TPset_error_fmt(TPEOS, "%s: Failed to read attributes for queue [%s] fd %d: %s",
                                fn, q, ret, strerror(errno));
        /* close queue */
        mq_close(ret);
        /* unlink the queue */
        mq_unlink(q);

        ret=(mqd_t)FAIL;
        goto out;
    }

out:
    
    return ret;
}

/**
 * Open conversation's reply queue
 * @param q
 * @param q_attr
 * @return
 */
private mqd_t open_reply_q(char *q, struct mq_attr *q_attr)
{
    mqd_t ret=(mqd_t)FAIL;
    char fn[] = "open_reply_q";

    ret = ndrx_mq_open_at (q, O_RDWR, S_IWUSR | S_IRUSR, NULL);

    if ((mqd_t)FAIL==ret)
    {
        _TPset_error_fmt(TPEOS, "%s:Failed to open queue [%s]: %s", fn, q, strerror(errno));
        goto out;
    }

    /* read queue attributes */
    if (FAIL==mq_getattr(ret, q_attr))
    {
        _TPset_error_fmt(TPEOS, "%s: Failed to read attributes for queue [%s] fd %d: %s",
                                fn, q, ret, strerror(errno));
#if 0
        /* We cannot remove their Q!!! */
        /* close queue */
        mq_close(ret);
        /* unlink the queue */
        mq_unlink(q);
#endif
        ret=(mqd_t)FAIL;
        goto out;
    }
    
out:

    return ret;
}

/* #define CONV_USE_ACK */
/**
 * Send ACK message
 * @param conv - conversation descriptor
 * @return
 */
private int send_ack(tp_conversation_control_t *conv, long flags)
{
#if CONV_USE_ACK
    int ret=SUCCEED;
    tp_command_generic_t ack;
    char fn[]="get_ack";
    
    memset(&ack, 0, sizeof(ack));
    ack.command_id = ATMI_COMMAND_CONVACK;
    ack.cd=conv->cd;
    
    if (SUCCEED!=(ret=generic_qfd_send(conv->reply_q, (char *)&ack, sizeof(ack), flags)))
    {
        int err;
        
        CONV_ERROR_CODE(ret, err);

        _TPset_error_fmt(err, "%s: Failed to send ACK, os err: %s", fn, strerror(ret));

        ret=FAIL;
        goto out;
    }
    NDRX_LOG(log_debug, "ACK sent for cd %d", conv->cd);
out:
    return ret;
#else
    return SUCCEED;
#endif
}

/**
 * Get ACK message
 * @param cd
 * @return
 */
public int get_ack(tp_conversation_control_t *conv, long flags)
{
#if CONV_USE_ACK
    int ret=SUCCEED;
    char buf[ATMI_MSG_MAX_SIZE];
    tp_command_generic_t *ack = (tp_command_generic_t *)buf;
    long rply_len;
    unsigned prio;

    if (SUCCEED!=setup_queue_attrs(&conv->my_q_attr, conv->my_listen_q,
                                    conv->my_listen_q_str, 0L))
    {
        ret=FAIL;
        goto out;
    }
    NDRX_LOG(log_debug, "Waiting for ACK");
    rply_len = generic_q_receive(conv->my_listen_q, buf, sizeof(buf), &prio, flags);
    
    if (rply_len<sizeof(tp_command_generic_t))
    {
        ret=FAIL;
        _TPset_error_fmt(TPESYSTEM, "Invalid ACK reply, len: %d expected %d",
                    rply_len, sizeof(tp_command_generic_t));
        goto out;
    }

    if (ack->cd!=conv->cd)
    {
        ret=FAIL;
        _TPset_error_fmt(TPESYSTEM, "Invalid ACK reply, waiting for cd %d got %d",
                    conv->cd, ack->cd);
    }
    else if (ATMI_COMMAND_CONVACK!=ack->command_id)
    {
        ret=FAIL;
        _TPset_error_fmt(TPESYSTEM, "Invalid ACK command %hd",
                    ack->command_id);
    }
    else

    {
        NDRX_LOG(log_debug, "Got ACK for cd %d", conv->cd);
    }

out:
    return ret;
#else
    return SUCCEED;
#endif
}

/**
 * Internal implementation of tpconnect
 * @param svc
 * @param data
 * @param len
 * @param flags
 * @return
 */
public int _tpconnect (char *svc, char *data, long len, long flags)
{
    int ret=SUCCEED;
    int cd;
    char fn[] = "_tpconnect";
    typed_buffer_descr_t *descr;
    buffer_obj_t *buffer_info;
    char buf[ATMI_MSG_MAX_SIZE];
    long data_len = MAX_CALL_DATA_SIZE;
    tp_command_call_t *call = (tp_command_call_t *)buf;
    time_t timestamp;
    char send_qstr[NDRX_MAX_Q_SIZE+1];
    char reply_qstr[NDRX_MAX_Q_SIZE+1]; /* special one for conversation */
    char their_qstr[NDRX_MAX_Q_SIZE+1]; /* Their Q (could be bridge service) */
    long revent = 0;
    short command_id=ATMI_COMMAND_CONNECT;
    tp_conversation_control_t *conv;
    /* Will have each thread own call sequence */
    static __thread unsigned callseq = 0;
    int is_bridge;
    NDRX_LOG(log_debug, "%s: called", fn);

    /* Check service availability */
    if (FALSE==ndrx_shm_get_svc(svc, send_qstr, &is_bridge))
    {
        NDRX_LOG(log_error, "Service is not available %s by shm", 
                svc);
        ret=FAIL;
        _TPset_error_fmt(TPENOENT, "%s: Service is not available %s by shm", 
                fn, svc);
        goto out;
    }
    
    /* Get free connection */
    if (FAIL==(cd=conv_get_cd(flags)))
    {
        _TPset_error_msg(TPELIMIT, "Could not get free connection descriptor");
        ret=FAIL;
        goto out;
    }

    conv = &G_tp_conversation_status[cd];
    /* Hmm setup cd? */
    conv->cd = cd;
    
    memset(call, 0, sizeof(*call));

    /*
     * Prepare some data if have something to prepare
     */
    if (NULL!=data)
    {
        /* fill up the details */
        if (NULL==(buffer_info = find_buffer(data)))
        {
            _TPset_error_fmt(TPEINVAL, "Buffer %p not known to system!", fn);
            ret=FAIL;
            goto out;
        }

        descr = &G_buf_descr[buffer_info->type_id];

        /* prepare buffer for call */
        if (SUCCEED!=descr->pf_prepare_outgoing(descr, data, len, call->data, &data_len, flags))
        {
            /* not good - error should be already set */
            ret=FAIL;
            goto out;
        }
        call->buffer_type_id = buffer_info->type_id;
    }
    else
    {
        data_len=0; /* no data */
    }
    /* OK, now fill up the details */
    call->data_len = data_len;
    data_len+=sizeof(tp_command_call_t);
    

    /* Format the conversational reply queue */
    sprintf(reply_qstr, NDRX_CONV_INITATOR_Q, G_atmi_conf.q_prefix, 
            G_atmi_conf.my_id, cd);
    NDRX_LOG(log_debug, "%s/%s/%d reply_qstr: [%s]",
		G_atmi_conf.q_prefix,  G_atmi_conf.my_id, cd, reply_qstr);
    strcpy(call->reply_to, reply_qstr);

    /* TODO: Firstly we should open the queue on which to listen right? */
    if ((mqd_t)FAIL==(conv->my_listen_q =
                    open_conv_q(reply_qstr, &conv->my_q_attr)))
    {
        NDRX_LOG(log_error, "%s: Failed to open listen queue", fn);
        ret=FAIL;
        goto out;
    }

    strcpy(conv->my_listen_q_str, reply_qstr);

    call->command_id = ATMI_COMMAND_CONNECT;

    strncpy(call->name, svc, XATMI_SERVICE_NAME_LENGTH);
    call->name[XATMI_SERVICE_NAME_LENGTH] = EOS;
    call->flags = flags;
    /* Prepare role flags */
    
    strcpy(call->my_id, G_atmi_conf.my_id);
    
    /* TODO: lock call descriptor - possibly add to connection descriptor structs */
    timestamp = time(NULL);
    call->cd = cd;
    /* Generate flags for target now. */
    CONV_TARGET_FLAGS(call);
    call->timestamp = timestamp;
    callseq++;
    call->callseq = callseq;
    
    /* Add global transaction info to call (if needed & tx available) */
    if (!(call->flags & TPNOTRAN) && G_atmi_xa_curtx.txinfo)
    {
        NDRX_LOG(log_info, "Current process in global transaction (%s) - "
                "prepare call", G_atmi_xa_curtx.txinfo->tmxid);
        
        atmi_xa_cpy_xai_to_call(call, G_atmi_xa_curtx.txinfo);   
    }
    /* Reset call timer...! */
    n_timer_reset(&call->timer);

    NDRX_LOG(log_debug, "Sending request to: %s", send_qstr);

    /* And then we call out the service. */
    if (SUCCEED!=(ret=generic_q_send(send_qstr, (char *)call, data_len, flags)))
    {
        int err;
        
        if (ENOENT==ret)
        {
            err=TPENOENT;
        }
        else
        {
            CONV_ERROR_CODE(ret, err);
        }

        _TPset_error_fmt(err, "%s: Failed to send, os err: %s", fn, strerror(ret));
        ret=FAIL;

        goto out;
    }
    else
    {
        conv->status = CONV_IN_CONVERSATION;
        conv->timestamp = timestamp;
        conv->callseq = callseq;

        /* Save the flags, alright? */
        conv->flags |= (flags & TPSENDONLY);
        conv->flags |= (flags & TPRECVONLY);
        
    }

    /* So now, we shall receive back handshake, by receving private queue 
     * id on which we can reach the server!
     */
    memset(buf, 0, sizeof(buf));

    if (SUCCEED!=_tprecv(cd, (char **)&buf, &data_len, 0L, &revent, &command_id))
    {
        /* We should have */
        if (ATMI_COMMAND_CONNRPLY!=command_id)
        {
            _TPset_error_fmt(TPESYSTEM, "%s: Invalid connect handshake reply %d", 
                                fn, command_id);
            ret=FAIL;
            goto out;
        }
    }
    
    strcpy(conv->reply_q_str, buf);
    if (is_bridge)
    {
        NDRX_LOG(log_warn, "Service is bridge");
        strcpy(their_qstr, send_qstr);
    }
    else
    {
        strcpy(their_qstr, conv->reply_q_str);
    }
    
    NDRX_LOG(log_debug, "Got reply queue for conversation: [%s] - trying to open [%s]",
                                    conv->reply_q_str, their_qstr);
    
    if ((mqd_t)FAIL==(conv->reply_q=open_reply_q(their_qstr, &conv->reply_q_attr)))
    {
        NDRX_LOG(log_error, "Cannot establish connection - failed to "
                                    "open reply queue [%s]", conv->reply_q_str);
        ret=FAIL;
        goto out;
    }

    /* Put down the flag that we have open connection */
    if (SUCCEED==ret)
    {
        M_had_open_con = TRUE;
    }

    conv->handshaked=TRUE;
    
out:
    /* TODO: Kill conversation if FAILED!!!! */
    NDRX_LOG(log_debug, "%s: ret= %d cd=%d", fn, ret);

    if (FAIL!=ret)
    	return cd;
    else
        return ret;
}

/**
 * TODO: Validate invalid queue.
 * TODO: Check cd - is it open?
 * @param cd
 * @param data
 * @param len
 * @param flags
 * @param revent
 * @return
 */
public int _tprecv (int cd, char * *data, 
                        long *len, long flags, long *revent,
                        short *command_id)
{
    int ret=SUCCEED;
    char fn[] = "_tprecv";
    long rply_len;
    unsigned prio;
    char rply_buf[ATMI_MSG_MAX_SIZE];
    tp_command_call_t *rply=(tp_command_call_t *)rply_buf;
    typed_buffer_descr_t *call_type;
    int answ_ok = FALSE;
    tp_conversation_control_t *conv;

    NDRX_LOG(log_debug, "%s enter", fn);

    *revent = 0;
    memset(rply_buf, 0, sizeof(*rply_buf));
    
    /* choose the connection */
    if (NULL==(conv=get_current_connection(cd)))
    {
        ret=FAIL;
        goto out;
    }

    /* Check are we allowed to receive? */
    if (ATMI_COMMAND_CONVDATA==*command_id && conv->flags & TPSENDONLY)
    {
        ret=FAIL;
        _TPset_error_fmt(TPEPROTO, "%s: Not allowed to receive "
                                    "because flags say: TPSENDONLY!", fn);
        goto out;
    }

    /* Change the attributes of the queue to match required */
    if (SUCCEED!=setup_queue_attrs(&conv->my_q_attr, conv->my_listen_q,
                                    conv->my_listen_q_str, flags))
    {
        ret=FAIL;
        goto out;
    }

    /* TODO: If we keep linked list with call descriptors and if there is
     * none, then we should return something back - FAIL/proto, not? */
    /**
     * We will drop any answers not registered for this call
     */
    while (!answ_ok)
    {
        /* receive the reply back */
        rply_len = generic_q_receive(conv->my_listen_q, rply_buf, sizeof(rply_buf), 
                                        &prio, flags);

        
        if (GEN_QUEUE_ERR_NO_DATA==rply_len)
        {
            /* there is no data in reply, nothing to do & nothing to return */
            ret=FAIL;
            goto out;
        }
        else if (FAIL==rply_len)
        {
            /* we have failed */
            NDRX_LOG(log_debug, "%s failed to receive answer", fn);
            ret=FAIL;
            goto out;
        }
        else
        {
            /* if answer is not expected, then we receive again! */
            if (conv->cd!=rply->cd)
            {

                NDRX_LOG(log_warn, "Dropping incoming message (not expected): "
                        "expected cd: %d, cd: %d, timestamp :%d, callseq: %u, reply from %s",
                        conv->cd, rply->cd, rply->timestamp, rply->callseq, rply->reply_to);
                /* clear the attributes we got... */
                memset(rply_buf, 0, sizeof(*rply_buf));
                continue; /* Wait for next message! */
            }
            else
            {
                answ_ok=TRUE;
            }

            /* Send ACK? */
            if (conv->handshaked && FAIL==send_ack(conv, flags))
            {
                ret=FAIL;
                goto out;
            }

            *command_id=rply->command_id;

            /* Save the last return codes */
            conv->rcode=rply->rcode;
            conv->rval=rply->rval;


            /* TODO: check incoming type! */
            /* we have an answer - prepare buffer */

            if (rply->sysflags & SYS_FLAG_REPLY_ERROR)
            {
                _TPset_error_msg(rply->rcode, "Server failed to generate reply");
                conv->revent = *revent = TPEV_SVCERR; /* Server failed. */
                ret=FAIL;
                goto out;
            } /* Forced close received. */
            else if (ATMI_COMMAND_DISCONN==rply->command_id)
            {
                conv->revent = *revent=TPEV_DISCONIMM;
                if (FAIL==normal_connection_shutdown(conv, FALSE))
                {
                    NDRX_LOG(log_error, "Failed to close conversation");
                    ret=FAIL;
                    goto out;
                }
                ret=FAIL; /* anyway! */
                _TPset_error(TPEEVENT); /* We have event! */
            }
            else if (ATMI_COMMAND_CONNRPLY==rply->command_id)
            {
                /* We have reply queue already in
                 * This is really used only for handshaking internally
                 * so we do not use buffer managment here!
                 */
                strcpy((char *)data, rply->data); /* return reply queue */
            }
            else
            {
                call_type = &G_buf_descr[rply->buffer_type_id];

                ret=call_type->pf_prepare_incoming(call_type,
                                rply->data,
                                rply->data_len,
                                data,
                                len,
                                flags);
                /* TODO: Check buffer acceptance or do it inside of prepare_incoming? */
                if (ret==FAIL)
                {
                    goto out;
                }
#if 0
                /* if all OK, then finally check the reply answer */
                if (TPSUCCESS!=rply->rval)
                {
                    _TPset_error_fmt(TPESVCFAIL, "Service returned %d", rply->rval);
                    ret=FAIL;
                    goto out;
                }
#endif
                /* Normal connection shutdown */
                if (ATMI_COMMAND_TPREPLY==rply->command_id)
                {
                    /* Basically we close the connection (normally)
                     * This means we also needs to close & unlink queues
                     */
                    NDRX_LOG(log_debug, "Server did tpreturn - closing conversation!");

                    /* Save the rcode returned. */
                    M_svc_return_code = rply->rcode;

                    if (TPSUCCESS!=rply->rval)
                    {
                        conv->revent = *revent = TPEV_SVCFAIL;
                    }
                    else
                    {
                        conv->revent = *revent = TPEV_SVCSUCC;
                    }

                    /*
                     * Gracefully shutdown the connection
                     */
                    if (SUCCEED!=normal_connection_shutdown(conv, TRUE))
                    {
                        ret=FAIL;
                        goto out;
                    }
                    ret=FAIL; /* anyway! */
                    _TPset_error(TPEEVENT); /* We have event! */
                    goto out;
                }

                if (rply->flags & TPSENDONLY)
                {
                    NDRX_LOG(log_debug, "Sender program issued TPRECVONLY "
                                    "- so we become TPSENDONLY!");
                    ret=FAIL;
                    conv->revent = *revent = TPEV_SENDONLY;

                    _TPset_error_fmt(TPEEVENT, "Got event TPEV_SENDONLY");
                    
                    /* Set our conversation as senders */
                    conv->flags|=TPSENDONLY;
                    conv->flags&=~TPRECVONLY;
                }
            } /* else - normal reply/data from send! */
        } /* reply recieved ok */
    }
out:
    NDRX_LOG(log_debug, "%s return %d", fn, ret);

    if (G_atmi_xa_curtx.txinfo)
    {
        if (TPEV_DISCONIMM == *revent ||  TPEV_SVCERR == *revent ||  
                TPEV_SVCFAIL == *revent)
        {
             NDRX_LOG(log_warn, "tprcv error - mark "
                     "transaction as abort only!");    
            /* later should be handled by transaction initiator! */
            G_atmi_xa_curtx.txinfo->tmtxflags |= TMTXFLAGS_IS_ABORT_ONLY;
        }
        /* If we got any reply, and it contains abort only flag
         * (for current transaction, then we abort it...)
         */
        if (0==strcmp(G_atmi_xa_curtx.txinfo->tmxid, rply->tmxid))
                
        {
            if (rply->tmtxflags & TMTXFLAGS_IS_ABORT_ONLY)
            {
                G_atmi_xa_curtx.txinfo->tmtxflags |= TMTXFLAGS_IS_ABORT_ONLY;
            }
            
            /* Update known RMs */
            if ( !(rply->tmtxflags & TMTXFLAGS_IS_ABORT_ONLY) &&
                    EOS!=rply->tmknownrms[0] &&
                    SUCCEED!=atmi_xa_update_known_rms(G_atmi_xa_curtx.txinfo->tmknownrms, 
                        rply->tmknownrms))
            {
                G_atmi_xa_curtx.txinfo->tmtxflags |= TMTXFLAGS_IS_ABORT_ONLY;
                FAIL_OUT(ret);
            }
        }   
    }

    return ret;
}

/**
 * Receive any un-expected messages.
 * With this we catch the cases when server have been illegally made tpreturn!
 * @return
 */
private void process_unsolicited_messages(int cd)
{
    short command_id=ATMI_COMMAND_CONNUNSOL;
    char *data=NULL;
    long len;
    long revent;

    /* Flush down all messages */
    while (SUCCEED==_tprecv (cd, &data, &len, TPNOBLOCK, &revent, &command_id))
    {
        NDRX_LOG(log_debug, "Ignoring unsolicited message!");
        /* Free up data */
        tpfree(data);
        data=NULL;
    }

}


/*public int	_tpconnect (char *svc, char *data, long len, long flags) */
public int	_tpsend (int cd, char *data, long len, long flags, long *revent,
                            short command_id)
{
    int ret=SUCCEED;
    char fn[] = "_tpsend";
    typed_buffer_descr_t *descr;
    buffer_obj_t *buffer_info=NULL;
    char buf[ATMI_MSG_MAX_SIZE];
    long data_len = MAX_CALL_DATA_SIZE;
    tp_command_call_t *call = (tp_command_call_t *)buf;
    time_t timestamp;
    char send_q[NDRX_MAX_Q_SIZE+1];
    char reply_q[NDRX_MAX_Q_SIZE+1]; /* special one for conversation */
    tp_conversation_control_t *conv;

    NDRX_LOG(log_debug, "%s: called", fn);
    *revent = 0;

    /* Check the current connection descriptor */
    if (NULL==(conv=get_current_connection(cd)))
    {
        ret=FAIL;
        goto out;
    }

    /* Check are we allowed to receive? */
    if (ATMI_COMMAND_CONVDATA==command_id && conv->flags & TPRECVONLY)
    {
        ret=FAIL;
        _TPset_error_fmt(TPEPROTO, "%s: Not allowed to receive "
                                    "because flags say: TPRECVONLY!", fn);
        goto out;
    }
    
    /* Manage the flags for active connection */
    if (flags & TPRECVONLY)
    {
        NDRX_LOG(log_debug, "%s: Program issued TPRECVONLY", fn);
        /* Set our conversation as senders */
        conv->flags|=TPRECVONLY;
        conv->flags&=~TPSENDONLY;
    }

    memset(call, 0, sizeof(*call));

    /* Change the mode in which we run.\
     * We may receive message in async mode.
     */
    if (SUCCEED!=setup_queue_attrs(&conv->reply_q_attr, conv->reply_q,
                                    conv->reply_q_str, flags))
    {
        ret=FAIL;
        goto out;
    }

    /* Check for any messages & process them */
    process_unsolicited_messages(cd);
    
    /*
     * Check are we still in connection?
     */
    if (CONV_IN_CONVERSATION!=conv->status)
    {
        ret=FAIL;

        /* If it exited with SUCCEED, then it is ERR. */
        if (conv->revent == TPEV_SVCSUCC)
        {
            *revent=TPEV_SVCERR;
        }
        else /* else we publish the event back (disconnect or SVCFAIL) */
        {
            *revent=conv->revent;
        }

        NDRX_LOG(log_error, "Un-solicited disconnect from server of "
                                "cd %d. Returning event %ld", cd, *revent);

        /* close our listening queue */
        normal_connection_shutdown(conv, FALSE);
        ret=FAIL;
        _TPset_error(TPEEVENT); /* Set that we have event for caller! */
        goto out;
    }
    
    /*
     * Prepare some data if have something to prepare
     */
    if (NULL!=data)
    {
        /* fill up the details */
        if (NULL==(buffer_info = find_buffer(data)))
        {
            _TPset_error_fmt(TPEINVAL, "Buffer %p not known to system!", fn);
            ret=FAIL;
            goto out;
        }

        descr = &G_buf_descr[buffer_info->type_id];

        /* prepare buffer for call */
        if (SUCCEED!=descr->pf_prepare_outgoing(descr, data, len, call->data, &data_len, flags))
        {
            /* not good - error should be already set */
            ret=FAIL;
            goto out;
        }

        call->buffer_type_id = buffer_info->type_id;
    }
    else if (ATMI_COMMAND_CONNRPLY==command_id)
    {
        /* We send them conversion related Q 
        strcpy(call->conv_related_q_str, conv->my_listen_q_str);
        */
        /* Indicate that this is string buffer! */
        call->buffer_type_id = BUF_TYPE_STRING;
        strcpy(call->data, conv->my_listen_q_str);
        data_len = strlen(call->data) + 1; /* Include EOS... */
    }
    else
    {
        data_len=0; /* no data */
    }
    /* OK, now fill up the details */
    call->data_len = data_len;

    data_len+=sizeof(tp_command_call_t);

    /* So here is little trick for bridge.
     * As it needs to know where reply should go, we should leave here original caller
     * reply address, but pack in different my_listen_q_str
     */
    NDRX_LOG(log_debug, "Reply address should be: [%s] but our address is: [%s]",
            conv->reply_q_str, conv->my_listen_q_str);
    
    strcpy(call->reply_to, conv->reply_q_str);
     
    /*
    strcpy(call->reply_to, conv->my_listen_q_str);
    */

    call->command_id = command_id;
    call->flags = flags;
    
    /* Fix up send/receive flags */
    CONV_TARGET_FLAGS(call);

    /* TODO: lock call descriptor - possibly add to connection descriptor structs */
    call->cd = conv->cd;

    /* Fix connection thing */
    if (call->cd>MAX_CONNECTIONS)
    {
        call->cd-=MAX_CONNECTIONS;
    }

    call->timestamp = conv->timestamp;
    call->callseq = conv->callseq;
    
    /* Add global transaction info for sending... */
    if (G_atmi_xa_curtx.txinfo)
    {
        NDRX_LOG(log_info, "Current process in global transaction (%s) - "
                "prepare call", G_atmi_xa_curtx.txinfo->tmxid);
        
        atmi_xa_cpy_xai_to_call(call, G_atmi_xa_curtx.txinfo);
    }
    
    /* And then we call out the service. */
    if (SUCCEED!=(ret=generic_qfd_send(conv->reply_q, (char *)call, data_len, flags)))
    {
        int err;

        if (ENOENT==ret)
        {
            err=TPENOENT;
        }
        else
        {
            CONV_ERROR_CODE(ret, err);
        }

        _TPset_error_fmt(err, "%s: Failed to send, os err: %s", fn, strerror(ret));
        ret=FAIL;

        goto out;
    }

    if (conv->handshaked && SUCCEED!=get_ack(conv, flags))
    {
        ret=FAIL;
        goto out;
    }

out:
    /* TODO: Kill conversation if FAILED!!!! */
    NDRX_LOG(log_debug, "%s: return %d", fn, ret);
    return ret;
}


/**
 * Close connection imideatelly
 * @param cd
 * @return SUCCEED/FAIL
 */
public int _tpdiscon (int cd)
{
    int ret=SUCCEED;
    long revent;
    tp_conversation_control_t *conv;
    char fn[]="_tpdiscon";
    
    /* Check our role */
    if (NULL==(conv=get_current_connection(cd)))
    {
        ret=FAIL;
        goto out;
    }

    /* Send disconnect command to server */
    if (FAIL==_tpsend (cd, NULL, 0L, 0L, &revent, ATMI_COMMAND_DISCONN))
    {
        NDRX_LOG(log_debug, "Failed to send disconnect to server - IGNORE!");
    }


    /* Close down then connection (We close down this only if we are server!)*/
    if (FAIL==normal_connection_shutdown(conv, TRUE))
    {
        ret=FAIL;
        goto out;
    }
    
out:
    NDRX_LOG(log_warn, "%s: return %d", fn, ret);
    return ret;
}

