/**
 * @brief Implements main functions for conversational server
 *   Some thoughts:
 *   1. Control is on that side which sends the message.
 *   2. ?
 *
 * @file conversation.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * AGPL or Mavimax's license for commercial use.
 * -----------------------------------------------------------------------------
 * AGPL license:
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License, version 3 as published
 * by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU Affero General Public License, version 3
 * for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along 
 * with this program; if not, write to the Free Software Foundation, Inc., 
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * -----------------------------------------------------------------------------
 * A commercial use license is available from Mavimax, Ltd
 * contact@mavimax.com
 * -----------------------------------------------------------------------------
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <atmi_int.h>
#include <typed_buf.h>

#include "../libatmisrv/srv_int.h"
#include "userlog.h"
#include <thlock.h>
#include <xa_cmn.h>
#include <tperror.h>
#include <atmi_shm.h>
#include <atmi_tls.h>
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


/* #define CONV_USE_ACK 1 */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
int M_had_open_con = EXFALSE;
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
exprivate mqd_t open_conv_q(char *q,  struct mq_attr *q_attr);
exprivate mqd_t open_reply_q(char *q, struct mq_attr *q_attr);
exprivate void rcv_hash_delall(tp_conversation_control_t *conv);
exprivate char * rcv_hash_ck(tp_conversation_control_t *conv, unsigned short msgseq);

/**
 * Closes any connection made as client.
 * This will send disconnect event to them, because normally server should do
 * tpreturn!
 * Try to close all queues, if have some error, report it
 */
expublic int close_open_client_connections(void)
{
    int i;
    int ret=EXSUCCEED;

    ATMI_TLS_ENTRY;
    
    /* nothing to do, we do not have opened any client connections! */
    if (!M_had_open_con)
    {
        return EXSUCCEED;
    }

    for (i=0; i<MAX_CONNECTIONS; i++)
    {
        if (CONV_IN_CONVERSATION==G_atmi_tls->G_tp_conversation_status[i].status)
        {
            if (EXFAIL==ndrx_tpdiscon(i))
            {
                NDRX_LOG(log_warn, "Failed to close connection [%d]", i);
                ret=EXFAIL;
            }
        }
    }
    M_had_open_con = EXFALSE;
    
    return ret;
}


/**
 * Return TRUE if we have any open connection
 * @return TRUE/FALSE
 */
expublic int have_open_connection(void)
{
    int i;
    int ret=EXFALSE;
    ATMI_TLS_ENTRY;
    
    /* nothing to do, we do not have opened any client connections! */
    if (!M_had_open_con)
    {
        return EXFALSE;
    }

    for (i=0; i<MAX_CONNECTIONS; i++)
    {
        if (CONV_IN_CONVERSATION==G_atmi_tls->G_tp_conversation_status[i].status)
        {
            ret=EXTRUE;
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
expublic int accept_connection(void)
{
    int ret=EXSUCCEED;
    tp_conversation_control_t *conv;
    long revent;
    int q_opened=EXFALSE;
    char their_qstr[NDRX_MAX_Q_SIZE+1];
    ATMI_TLS_ENTRY;
    
    conv= &G_atmi_tls->G_accepted_connection;
/*
    char send_q[NDRX_MAX_Q_SIZE+1]; */

    conv->flags = G_atmi_tls->G_last_call.flags; /* Save call flags */
    
    /* Fix up cd for future use for replies.  */
    conv->cd = G_atmi_tls->G_last_call.cd - MAX_CONNECTIONS;
    /* Change the status, that we have connection open */
    conv->status = CONV_IN_CONVERSATION;
    conv->msgseqout = NDRX_CONF_MSGSEQ_START;
    conv->msgseqin = NDRX_CONF_MSGSEQ_START;
    conv->callseq = G_atmi_tls->G_last_call.callseq;
    /* 1. Open listening queue */
    snprintf(conv->my_listen_q_str, sizeof(conv->my_listen_q_str), 
                    NDRX_CONV_SRV_Q,
                    G_atmi_tls->G_atmi_conf.q_prefix, 
                    G_atmi_tls->G_last_call.my_id, 
                    G_atmi_tls->G_last_call.cd-MAX_CONNECTIONS,
                    /* In accepted connection we put their id */
                    G_atmi_tls->G_atmi_conf.my_id
                    );
    
    /* TODO: Firstly we should open the queue on which to listen right? */
    if ((mqd_t)EXFAIL==(conv->my_listen_q =
                    open_conv_q(conv->my_listen_q_str, &conv->my_q_attr)))
    {
        NDRX_LOG(log_error, "%s: Failed to open listen queue", __func__);
        ret=EXFAIL;
        goto out;
    }
    q_opened=EXTRUE;

    /* 2. Connect to their reply queue */
    NDRX_STRCPY_SAFE(conv->reply_q_str, G_atmi_tls->G_last_call.reply_to);
    
    /* Check is this coming from bridge. If so the we connect to bridge */
    
    if (0!=G_atmi_tls->G_last_call.callstack[0])
    {
        br_dump_nodestack(G_atmi_tls->G_last_call.callstack, 
                "Incoming conversation from bridge,"
                "using first node from node stack");
#if defined(EX_USE_POLL) || defined(EX_USE_SYSVQ)
        /* poll() mode: */
        {
            int is_bridge;
            char tmpsvc[MAXTIDENT+1];

            snprintf(tmpsvc, sizeof(tmpsvc), NDRX_SVC_BRIDGE, 
                    (int)G_atmi_tls->G_last_call.callstack[0]);

            if (EXSUCCEED!=ndrx_shm_get_svc(tmpsvc, their_qstr, &is_bridge,
                    NULL))
            {
                NDRX_LOG(log_error, "Failed to get bridge svc: [%s]", 
                        tmpsvc);
                EXFAIL_OUT(ret);
            }
        }
#else
        snprintf(their_qstr, sizeof(their_qstr),NDRX_SVC_QBRDIGE, 
                G_atmi_tls->G_atmi_conf.q_prefix, 
                (int)G_atmi_tls->G_last_call.callstack[0]);
#endif
        
    }
    else
    {
        /* Local conversation  */
        NDRX_STRCPY_SAFE(their_qstr, conv->reply_q_str);
    }
    
    NDRX_LOG(log_debug, "Connecting to: [%s]", their_qstr);
    
    if ((mqd_t)EXFAIL==(conv->reply_q=open_reply_q(their_qstr, &conv->reply_q_attr)))
    {
        NDRX_LOG(log_error, "Cannot connect to reply queue [%s] - "
                                        "cannot accept connection!", 
                                        their_qstr);
        ret=EXFAIL;
        goto out;
    }

    /* 3. Send back reply to their queue */
    NDRX_LOG(log_debug, "About to send handshake back to client...");
    if (EXSUCCEED!=ndrx_tpsend(G_atmi_tls->G_last_call.cd, NULL, 0, 0, &revent,
                            ATMI_COMMAND_CONNRPLY))
    {
        NDRX_LOG(log_error, "Failed to reply for acceptance!");
        ret=EXFAIL;

    }
    
out:

    /* Close down the queue if we fail but queue was opened! */
    if (EXSUCCEED!=ret && q_opened)
    {
        if (EXFAIL==ndrx_mq_close(conv->my_listen_q))
        {
            NDRX_LOG(log_warn, "Failed to close %s:%s",
                        conv->my_listen_q_str, strerror(errno));
        }
    }

    if (EXSUCCEED==ret)
    {
        conv->handshaked=EXTRUE;
    }

    NDRX_LOG(log_debug, "returns %d",  ret);
    return ret;
}

/**
 * Return current connection from given cd
 * @param cd
 * @return NULL/connection control
 */
expublic tp_conversation_control_t*  get_current_connection(int cd)
{
    tp_conversation_control_t *ret=NULL;
    ATMI_TLS_ENTRY;
    
    if (cd>=0 && cd<MAX_CONNECTIONS)
    {
        ret=&G_atmi_tls->G_tp_conversation_status[cd];
    }
    else if (cd>=MAX_CONNECTIONS)
    {
        ret=&G_atmi_tls->G_accepted_connection;
    }
    else
    {
        ndrx_TPset_error_fmt(TPEINVAL, "Invalid connection descriptor %d", cd);
    }

    if (NULL!=ret && CONV_IN_CONVERSATION!=ret->status)
    {
        ndrx_TPset_error_fmt(TPEINVAL, "Invalid connection descriptor %d - "
                                        "connection closed",  cd);
        ret=NULL;
    }

    return ret;
}

/**
 * Close connection (Normally!)
 * @return SUCCEED/FAIL
 */
expublic int normal_connection_shutdown(tp_conversation_control_t *conv, int killq)
{
    int ret=EXSUCCEED;
    char fn[] = "normal_connection_shutdown";
    ATMI_TLS_ENTRY;
    
    NDRX_LOG(log_debug, "%s: Closing [%s]",  __func__, conv->my_listen_q_str);

    /* close down the queue */
    if (EXSUCCEED!=ndrx_mq_close(conv->my_listen_q))
    {
        NDRX_LOG(log_warn, "Failed to ndrx_mq_close [%s]: %s",
                                         conv->my_listen_q_str, strerror(errno));
        ndrx_TPset_error_fmt(TPEOS, "%s: Failed to ndrx_mq_close [%s]: %s",
                            __func__, conv->my_listen_q_str, strerror(errno));
       /* ret=FAIL;
        goto out; */
    }
    
    /* Remove the queue */
    if (killq && EXSUCCEED!=ndrx_mq_unlink(conv->my_listen_q_str))
    {
        NDRX_LOG(log_warn, "Failed to ndrx_mq_unlink [%s]: %s",
                                         conv->my_listen_q_str, strerror(errno));
        ndrx_TPset_error_fmt(TPEOS, "%s: Failed to ndrx_mq_unlink [%s]: %s",
                            __func__, conv->my_listen_q_str, strerror(errno));
        /* ret=FAIL;
        goto out; */
    }
    
    /* Kill the reply queue too? */
    
    NDRX_LOG(log_debug, "Closing [%s]",  conv->reply_q_str);

    /* close down the queue */
    if (EXSUCCEED!=ndrx_mq_close(conv->reply_q))
    {
        NDRX_LOG(log_warn, "Failed to ndrx_mq_close [%s]: %s",
                                        conv->reply_q_str, strerror(errno));
        ndrx_TPset_error_fmt(TPEOS, "%s: Failed to ndrx_mq_close [%s]: %s",
                                         __func__, conv->reply_q_str, strerror(errno));
       /* ret=FAIL;
        goto out; */
    }
    
    /* Remove the queue */
    NDRX_LOG(log_warn, "UNLINKING: %s %d", conv->reply_q_str, killq);
    if (killq && EXSUCCEED!=ndrx_mq_unlink(conv->reply_q_str))
    {
        NDRX_LOG(log_warn, "Failed to ndrx_mq_unlink [%s]: %s",
                                         conv->reply_q_str, strerror(errno));
        ndrx_TPset_error_fmt(TPEOS, "%s: Failed to ndrx_mq_unlink [%s]: %s",
                                         __func__, conv->reply_q_str, strerror(errno));
        /* ret=FAIL;
        goto out; */
    }
    
    /* At this point we reset the CD - free the CD!!  */
    /* Unregister CD from global tx */
    if (G_atmi_tls->G_atmi_xa_curtx.txinfo)
    {
        /* try to unregister... anyway (even was called with TPNOTRAN)
         * will not find in hash, that's it... 
         */
        atmi_xa_cd_unreg(&(G_atmi_tls->G_atmi_xa_curtx.txinfo->conv_cds), conv->cd);
    }
    
    
    rcv_hash_delall(conv); /* Remove all buffers if left... */
    
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
exprivate int conv_get_cd(long flags)
{
    ATMI_TLS_ENTRY;
    
    int start_cd = G_atmi_tls->conv_cd; /* mark where we began */
    
    /* Just take a next number... for better hash. */
    do {
        G_atmi_tls->conv_cd++;

        if (G_atmi_tls->conv_cd > MAX_CONNECTIONS-1)
        {
            G_atmi_tls->conv_cd=1; /* TODO: Maybe start with 0? */
        }

        if (start_cd==G_atmi_tls->conv_cd)
        {
            NDRX_LOG(log_debug, "Connection descriptors overflow restart!");
            break;
        }
    }
    while (CONV_NO_INITATED!=
            G_atmi_tls->G_tp_conversation_status[G_atmi_tls->conv_cd].status);

    if (CONV_NO_INITATED!=
            G_atmi_tls->G_tp_conversation_status[G_atmi_tls->conv_cd].status)
    {
        NDRX_LOG(log_debug, "All connection descriptors have been taken - FAIL!");
        G_atmi_tls->conv_cd=EXFAIL;
    }
    else
    {
        NDRX_LOG(log_debug, "Got free connection descriptor %d", G_atmi_tls->conv_cd);
    }
    
    if (EXFAIL!=G_atmi_tls->conv_cd && 
            !(flags & TPNOTRAN) && G_atmi_tls->G_atmi_xa_curtx.txinfo)
    {
        NDRX_LOG(log_debug, "Registering conv cd=%d under global "
                "transaction!", G_atmi_tls->conv_cd);
        if (EXSUCCEED!=atmi_xa_cd_reg(&(G_atmi_tls->G_atmi_xa_curtx.txinfo->conv_cds), 
                G_atmi_tls->conv_cd))
        {
            G_atmi_tls->conv_cd=EXFAIL;
        }
    }

    return G_atmi_tls->conv_cd;
}

/**
 * Open the queue on which we are going to listen for the stuff...
 * @param q
 * @return SUCCEED/FAIL
 */
exprivate mqd_t open_conv_q(char *q,  struct mq_attr *q_attr)
{
    mqd_t ret=(mqd_t)EXFAIL;

    ret = ndrx_mq_open_at (q, O_RDWR | O_CREAT, S_IWUSR | S_IRUSR, NULL);

    if ((mqd_t)EXFAIL==ret)
    {
        ndrx_TPset_error_fmt(TPEOS, "%s:Failed to open queue [%s]: %s",  
                __func__, q, strerror(errno));
        goto out;
    }

        /* read queue attributes */
    if (EXFAIL==ndrx_mq_getattr(ret, q_attr))
    {
        ndrx_TPset_error_fmt(TPEOS, "%s: Failed to read attributes "
                "for queue [%s] fd %d: %s",
                __func__, q, ret, strerror(errno));
        /* close queue */
        ndrx_mq_close(ret);
        /* unlink the queue */
        ndrx_mq_unlink(q);

        ret=(mqd_t)EXFAIL;
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
exprivate mqd_t open_reply_q(char *q, struct mq_attr *q_attr)
{
    mqd_t ret=(mqd_t)EXFAIL;

    ret = ndrx_mq_open_at (q, O_RDWR, S_IWUSR | S_IRUSR, NULL);

    if ((mqd_t)EXFAIL==ret)
    {
        ndrx_TPset_error_fmt(TPEOS, "Failed to open queue [%s]: %s",  
                q, strerror(errno));
        goto out;
    }

    /* read queue attributes */
    if (EXFAIL==ndrx_mq_getattr(ret, q_attr))
    {
        ndrx_TPset_error_fmt(TPEOS, "Failed to read attributes for queue [%s] fd %d: %s",
                                 q, ret, strerror(errno));
#if 0
        /* We cannot remove their Q!!! */
        /* close queue */
        ndrx_mq_close(ret);
        /* unlink the queue */
        ndrx_mq_unlink(q);
#endif
        ret=(mqd_t)EXFAIL;
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
exprivate int send_ack(tp_conversation_control_t *conv, long flags)
{
#if CONV_USE_ACK
    int ret=EXSUCCEED;
    tp_conv_ack_t ack;
    char fn[]="get_ack";
    
    memset(&ack, 0, sizeof(ack));
    ack.command_id = ATMI_COMMAND_CONVACK;
    ack.cd=conv->cd;
    
    if (EXSUCCEED!=(ret=ndrx_generic_qfd_send(conv->reply_q, (char *)&ack, sizeof(ack), flags)))
    {
        int err;
        
        CONV_ERROR_CODE(ret, err);

        ndrx_TPset_error_fmt(err, "%s: Failed to send ACK, os err: %s",  __func__, strerror(ret));

        ret=EXFAIL;
        goto out;
    }
    NDRX_LOG(log_debug, "ACK sent for cd %d", conv->cd);
out:
    return ret;
#else
    return EXSUCCEED;
#endif
}

/**
 * Get ACK message
 * @param cd
 * @return
 */
expublic int ndrx_get_ack(tp_conversation_control_t *conv, long flags)
{
#if CONV_USE_ACK
    int ret=EXSUCCEED;
    char buf[ATMI_MSG_MAX_SIZE];
    tp_conv_ack_t *ack = (tp_conv_ack_t *)buf;
    long rply_len;
    unsigned prio;

    if (EXSUCCEED!=ndrx_setup_queue_attrs(&conv->my_q_attr, conv->my_listen_q,
                                    conv->my_listen_q_str, 0L))
    {
        ret=EXFAIL;
        goto out;
    }
    NDRX_LOG(log_debug, "Waiting for ACK");
    rply_len = ndrx_generic_q_receive(conv->my_listen_q, buf, sizeof(buf), &prio, flags);
    
    if (rply_len<sizeof(tp_conv_ack_t))
    {
        ret=EXFAIL;
        ndrx_TPset_error_fmt(TPESYSTEM, "Invalid ACK reply, len: %d expected %d",
                    rply_len, sizeof(tp_command_generic_t));
        goto out;
    }

    if (ack->cd!=conv->cd)
    {
        ret=EXFAIL;
        ndrx_TPset_error_fmt(TPESYSTEM, "Invalid ACK reply, waiting for cd %d got %d",
                    conv->cd, ack->cd);
    }
    else if (ATMI_COMMAND_CONVACK!=ack->command_id)
    {
        ret=EXFAIL;
        ndrx_TPset_error_fmt(TPESYSTEM, "Invalid ACK command %hd",
                    ack->command_id);
    }
    else

    {
        NDRX_LOG(log_debug, "Got ACK for cd %d", conv->cd);
    }

out:
    return ret;
#else
    return EXSUCCEED;
#endif
}

/**
 * Internal implementation of tpconnect.
 * So basically after connect, the picture if following (example):
 * 
 * As client we option this queue: /dom1,cnv,c,clt,atmiclt35,24280,5,1,1, and 
 * this is used for us to receive msgs. Thus if atmiclt35 is dead, we can remove
 * this q (of corse if we are on node 1).
 * 
 * When server accepts connection, it builds something like this:
 * dom2,cnv,s,clt,atmiclt35,24280,1,1,1,srv,atmisv35,10,24228,0,2 and this is
 * used on their side to receive msgs.
 * Thus if atmisv35 is dead and we are on node2, we can remove the q.
 * 
 * @param svc
 * @param data
 * @param len
 * @param flags
 * @return
 */
expublic int ndrx_tpconnect (char *svc, char *data, long len, long flags)
{
    int ret=EXSUCCEED;
    int cd;
    char fn[] = "_tpconnect";
    typed_buffer_descr_t *descr;
    buffer_obj_t *buffer_info;
    char buf[NDRX_MSGSIZEMAX];
    long data_len = MAX_CALL_DATA_SIZE;
    tp_command_call_t *call = (tp_command_call_t *)buf;
    time_t timestamp;
    char send_qstr[NDRX_MAX_Q_SIZE+1];
    char reply_qstr[NDRX_MAX_Q_SIZE+1]; /* special one for conversation */
    char their_qstr[NDRX_MAX_Q_SIZE+1]; /* Their Q (could be bridge service) */
    long revent = 0;
    short command_id=ATMI_COMMAND_CONNECT;
    tp_conversation_control_t *conv;
    int is_bridge;
    ATMI_TLS_ENTRY;
    
    NDRX_LOG(log_debug, "%s: called", __func__);

    /* Check service availability */
    if (EXSUCCEED!=ndrx_shm_get_svc(svc, send_qstr, &is_bridge, NULL))
    {
        NDRX_LOG(log_error, "Service is not available %s by shm", 
                svc);
        ret=EXFAIL;
        ndrx_TPset_error_fmt(TPENOENT, "%s: Service is not available %s by shm", 
                 __func__, svc);
        goto out;
    }
    
    /* Get free connection */
    if (EXFAIL==(cd=conv_get_cd(flags)))
    {
        ndrx_TPset_error_msg(TPELIMIT, "Could not get free connection descriptor");
        ret=EXFAIL;
        goto out;
    }

    conv = &G_atmi_tls->G_tp_conversation_status[cd];
    /* Hmm setup cd? */
    conv->cd = cd;
    
    memset(call, 0, sizeof(*call));

    /* Bug #300 */
    call->clttout = G_atmi_env.time_out;
    
    /*
     * Prepare some data if have something to prepare
     */
    if (NULL!=data)
    {
        /* fill up the details */
        if (NULL==(buffer_info = ndrx_find_buffer(data)))
        {
            ndrx_TPset_error_fmt(TPEINVAL, "Buffer %p not known to system!", __func__);
            ret=EXFAIL;
            goto out;
        }

        descr = &G_buf_descr[buffer_info->type_id];

        /* prepare buffer for call */
        if (EXSUCCEED!=descr->pf_prepare_outgoing(descr, data, len, call->data, 
                &data_len, flags))
        {
            /* not good - error should be already set */
            ret=EXFAIL;
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
    snprintf(reply_qstr, sizeof(reply_qstr), NDRX_CONV_INITATOR_Q, 
            G_atmi_tls->G_atmi_conf.q_prefix,  G_atmi_tls->G_atmi_conf.my_id, cd);
    
    NDRX_LOG(log_debug, "%s/%s/%d reply_qstr: [%s]",
		G_atmi_tls->G_atmi_conf.q_prefix,  G_atmi_tls->G_atmi_conf.my_id, 
                cd, reply_qstr);
    NDRX_STRCPY_SAFE(call->reply_to, reply_qstr);

    /* TODO: Firstly we should open the queue on which to listen right? */
    if ((mqd_t)EXFAIL==(conv->my_listen_q =
                    open_conv_q(reply_qstr, &conv->my_q_attr)))
    {
        NDRX_LOG(log_error, "%s: Failed to open listen queue", __func__);
        ret=EXFAIL;
        goto out;
    }

    NDRX_STRCPY_SAFE(conv->my_listen_q_str, reply_qstr);

    call->command_id = ATMI_COMMAND_CONNECT;

    NDRX_STRNCPY(call->name, svc, XATMI_SERVICE_NAME_LENGTH);
    call->name[XATMI_SERVICE_NAME_LENGTH] = EXEOS;
    call->flags = flags | TPCONV; /* This is conversational call... */
    /* Prepare role flags */
    
    NDRX_STRCPY_SAFE(call->my_id, G_atmi_tls->G_atmi_conf.my_id);
    
    /* TODO: lock call descriptor - possibly add to connection descriptor structs */
    timestamp = time(NULL);
    call->cd = cd;
    /* Generate flags for target now. */
    CONV_TARGET_FLAGS(call);
    call->timestamp = timestamp;
    call->callseq = ndrx_get_next_callseq_shared();
    call->msgseq = NDRX_CONF_MSGSEQ_START; /* start with NDRX_CONF_MSGSEQ_START */
    
    /* Add global transaction info to call (if needed & tx available) */
    if (!(call->flags & TPNOTRAN) && G_atmi_tls->G_atmi_xa_curtx.txinfo)
    {
        NDRX_LOG(log_info, "Current process in global transaction (%s) - "
                "prepare call", G_atmi_tls->G_atmi_xa_curtx.txinfo->tmxid);
        
        atmi_xa_cpy_xai_to_call(call, G_atmi_tls->G_atmi_xa_curtx.txinfo);   
    }
    /* Reset call timer...! */
    ndrx_stopwatch_reset(&call->timer);

    /*
    NDRX_LOG(log_debug, "Sending request to: %s, callseq: %hu", 
      
     *       send_qstr, call->callseq, );
*/
    NDRX_LOG(log_debug, "Sending request to: [%s]: "
                        "cd: %d, timestamp :%d, callseq: %hu, reply from [%s]",
                        send_qstr, call->cd, call->timestamp, call->callseq, call->reply_to);
    /* And then we call out the service. */
    if (EXSUCCEED!=(ret=ndrx_generic_q_send(send_qstr, (char *)call, data_len, flags, 0)))
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

        ndrx_TPset_error_fmt(err, "%s: Failed to send, os err: %s",  __func__, strerror(ret));
        ret=EXFAIL;

        goto out;
    }
    else
    {
        conv->status = CONV_IN_CONVERSATION;
        conv->timestamp = timestamp;
        conv->callseq = call->callseq;
        conv->msgseqout = call->msgseq;
        conv->msgseqin = NDRX_CONF_MSGSEQ_START; /* start with 1 */

        /* Save the flags, alright? */
        conv->flags |= (flags & TPSENDONLY);
        conv->flags |= (flags & TPRECVONLY);
        
    }

    /* So now, we shall receive back handshake, by receving private queue 
     * id on which we can reach the server!
     * TODO: We might want to move to dynamic memory..?
     */
    memset(buf, 0, sizeof(buf));

    if (EXSUCCEED!=ndrx_tprecv(cd, (char **)&buf, &data_len, 0L, &revent, &command_id))
    {
        /* We should have */
        if (ATMI_COMMAND_CONNRPLY!=command_id)
        {
            ndrx_TPset_error_fmt(TPESYSTEM, "%s: Invalid connect handshake reply %d", 
                                 __func__, command_id);
            ret=EXFAIL;
            goto out;
        }
    }
    
    NDRX_STRCPY_SAFE(conv->reply_q_str, buf);
    if (is_bridge)
    {
        NDRX_LOG(log_warn, "Service is bridge");
        NDRX_STRCPY_SAFE(their_qstr, send_qstr);
    }
    else
    {
        NDRX_STRCPY_SAFE(their_qstr, conv->reply_q_str);
    }
    
    NDRX_LOG(log_debug, "Got reply queue for conversation: [%s] - trying to open [%s]",
                                    conv->reply_q_str, their_qstr);
    
    if ((mqd_t)EXFAIL==(conv->reply_q=open_reply_q(their_qstr, &conv->reply_q_attr)))
    {
        NDRX_LOG(log_error, "Cannot establish connection - failed to "
                                    "open reply queue [%s]", conv->reply_q_str);
        ret=EXFAIL;
        goto out;
    }

    /* Put down the flag that we have open connection */
    if (EXSUCCEED==ret)
    {
        M_had_open_con = EXTRUE;
    }

    conv->handshaked=EXTRUE;
    
out:
    /* TODO: Kill conversation if FAILED!!!! */
    NDRX_LOG(log_debug, "%s: ret= %d cd=%d",  __func__, ret);

    if (EXFAIL!=ret)
    	return cd;
    else
        return ret;
}

/**
 * Add message to hash
 * If remove message if exists...
 * @param conv
 * @param msgseq
 * @param buf
 * @return 
 */
exprivate int rcv_hash_add(tp_conversation_control_t *conv,
           unsigned short msgseq, char *buf)
{
    
    int ret = EXSUCCEED;
    char *tmp;
    tpconv_buffer_t * el = NDRX_CALLOC(1, sizeof(tpconv_buffer_t));
    
    if (NULL!=(tmp=rcv_hash_ck(conv, msgseq)))
    {
        NDRX_LOG(log_error, "Dropping existing out of order conversation "
                "msgseq: %hu, ptr: %p",
                msgseq, tmp);
        userlog("Dropping existing out of order conversation "
                "msgseq: %hu, ptr: %p",
                msgseq, tmp);
        NDRX_FREE(tmp);
    }
    
    if (NULL==el)
    {
        ndrx_TPset_error_fmt(TPESYSTEM, "%s: Failed to allocate mem: %s", 
                __func__, strerror(errno));
        EXFAIL_OUT(ret);
    }
    
    el->msgseq = (int)msgseq;
    el->buf = buf;
    
    EXHASH_ADD_INT( conv->out_of_order_msgs, msgseq, el );
    
out:
                                
    return ret;
}

/**
 * Check the message within hash, if found, remove hash entry
 * @param conv
 * @param msgseq
 * @return NULL if item not found, buffer if found
 */
exprivate char * rcv_hash_ck(tp_conversation_control_t *conv, unsigned short msgseq)
{
    char *ret = NULL;
    tpconv_buffer_t * el;
    int seq =  (int)msgseq;
    
    EXHASH_FIND_INT( conv->out_of_order_msgs, &seq, el);
    
    if (NULL!=el)
    {
        ret = el->buf;
        EXHASH_DEL(conv->out_of_order_msgs, el);
        NDRX_FREE(el);
    }
    
    return ret;
}

/**
 * Delete all hash
 * @param conv
 * @param msgseq
 * @return 
 */
exprivate void rcv_hash_delall(tp_conversation_control_t *conv)
{
    tpconv_buffer_t *el = NULL;
    tpconv_buffer_t *elt = NULL;
    
    /* Iterate over the hash! */
    EXHASH_ITER(hh, conv->out_of_order_msgs, el, elt)
    {
         EXHASH_DEL(conv->out_of_order_msgs, el);
         NDRX_FREE(el->buf); /* dealloc system buffer */
         NDRX_FREE(el); /* free hash item */
    }
    
}

/**
 * Receive message from conversation
 * TODO: Add stopwatch for timeout...
 * 
 * @param cd
 * @param data
 * @param len
 * @param flags
 * @param revent
 * @return
 */
expublic int ndrx_tprecv (int cd, char **data, 
                        long *len, long flags, long *revent,
                        short *command_id)
{
    int ret=EXSUCCEED;
    long rply_len;
    unsigned prio;
    size_t rply_bufsz;
    char *rply_buf; /* Allocate dynamically! */
    tp_command_call_t *rply;
    typed_buffer_descr_t *call_type;
    int answ_ok = EXFALSE;
    tp_conversation_control_t *conv;
    ndrx_stopwatch_t t;
    ATMI_TLS_ENTRY;
    NDRX_LOG(log_debug, "%s enter", __func__);

    *revent = 0;
    
    if (!(flags & TPNOTIME))
    {
        ndrx_stopwatch_reset(&t);
    }
    
    /* choose the connection */
    if (NULL==(conv=get_current_connection(cd)))
    {
        ndrx_TPset_error_fmt(TPEINVAL, "%s: Invalid connection descriptor %d",  
                __func__, cd);
        EXFAIL_OUT(ret);
    }

    /* Check are we allowed to receive? */
    if (ATMI_COMMAND_CONVDATA==*command_id && conv->flags & TPSENDONLY)
    {
        ndrx_TPset_error_fmt(TPEPROTO, "%s: Not allowed to receive "
                                    "because flags say: TPSENDONLY!", __func__);
        EXFAIL_OUT(ret);
    }

    /* Change the attributes of the queue to match required */
    if (EXSUCCEED!=ndrx_setup_queue_attrs(&conv->my_q_attr, conv->my_listen_q,
                                    conv->my_listen_q_str, flags))
    {
        EXFAIL_OUT(ret);
    }
    
    /* Check the message in hash?! */
    if (NULL!=(rply_buf = rcv_hash_ck(conv, conv->msgseqin)))
    {
        NDRX_LOG(log_debug, "Message with sequence already in memory: %hu - injecting",
                conv->msgseqin);
        rply = (tp_command_call_t *)rply_buf;
        goto inject_message;
    }

    /* Allocate Enduro/X system buffer */
    NDRX_SYSBUF_ALLOC_WERR_OUT(rply_buf, &rply_bufsz, ret);
    rply = (tp_command_call_t *)rply_buf;
    
    /* TODO: If we keep linked list with call descriptors and if there is
     * none, then we should return something back - FAIL/proto, not? */

    /**
     * We will drop any answers not registered for this call
     */
    while (!answ_ok)
    {
        long spent;
        if (!(flags & TPNOTIME) && 
                (spent=ndrx_stopwatch_get_delta_sec(&t)) > G_atmi_env.time_out)
        {
            NDRX_LOG(log_error, "%s: call expired (spent: %ld sec, tout: %ld sec)", 
                    __func__, spent, G_atmi_env.time_out);
            
            ndrx_TPset_error_fmt(TPETIME, "%s: call expired (spent: %ld sec, tout: %ld sec)", 
                    __func__, spent, G_atmi_env.time_out);
            
            EXFAIL_OUT(ret);
        }

        /* receive the reply back */
        rply_len = ndrx_generic_q_receive(conv->my_listen_q, NULL, NULL, 
                rply_buf, rply_bufsz, &prio, flags);

        
        if (GEN_QUEUE_ERR_NO_DATA==rply_len)
        {
            /* there is no data in reply, nothing to do & nothing to return */
            EXFAIL_OUT(ret);
        }
        else if (EXFAIL==rply_len)
        {
            /* we have failed */
            NDRX_LOG(log_debug, "%s failed to receive answer", __func__);
            ret=EXFAIL;
            goto out;
        }
        else
        {
            /* if answer is not expected, then we receive again! */
            if (conv->cd!=rply->cd)
            {

                NDRX_LOG(log_warn, "Dropping incoming message (not expected): "
                        "expected cd: %d, cd: %d, timestamp :%d, callseq: %hu, reply from [%s]",
                        conv->cd, rply->cd, rply->timestamp, rply->callseq, rply->reply_to);
                /* clear the attributes we got... 
                memset(rply_buf, 0, sizeof(*rply_buf));
                 * Really need a memset?
                 * */
                continue; /* Wait for next message! */
            }
            
inject_message:
            /* we have an answer - prepare buffer */
            /* Check the sequence number */
            if (rply->msgseq!=conv->msgseqin)
            {
                answ_ok=EXFALSE;
                NDRX_LOG(log_info, "Message out of sequence, expected: %hu, "
                        "got: %hu - suspending to hash",
                        conv->msgseqin, rply->msgseq);
                
                /* TODO: Add reply message to the hash of the waiting msgs... */
                if (EXSUCCEED!=rcv_hash_add(conv, rply->msgseq, rply_buf))
                {
                    EXFAIL_OUT(ret);
                }
                
                /* Realloc system buffer */
                NDRX_SYSBUF_ALLOC_WERR_OUT(rply_buf, &rply_bufsz, ret);
                /* switch the ptrs... */
                rply = (tp_command_call_t *)rply_buf;
                
                continue;
            }
            else
            {
                answ_ok=EXTRUE;
                conv->msgseqin++;
                NDRX_LOG(log_info, "msgseq %hu received as expected", 
                        rply->msgseq);
            }

            /* Send ACK? */
            if (conv->handshaked && EXFAIL==send_ack(conv, flags))
            {
                EXFAIL_OUT(ret);
            }

            *command_id=rply->command_id;

            /* Save the last return codes */
            conv->rcode=rply->rcode;
            conv->rval=rply->rval;

            if (rply->sysflags & SYS_FLAG_REPLY_ERROR)
            {
                ndrx_TPset_error_msg(rply->rcode, "Server failed to generate reply");
                conv->revent = *revent = TPEV_SVCERR; /* Server failed. */
                ret=EXFAIL;
                goto out;
            } /* Forced close received. */
            else if (ATMI_COMMAND_DISCONN==rply->command_id)
            {
                conv->revent = *revent=TPEV_DISCONIMM;
                if (EXFAIL==normal_connection_shutdown(conv, EXFALSE))
                {
                    NDRX_LOG(log_error, "Failed to close conversation");
                    ret=EXFAIL;
                    goto out;
                }
                ret=EXFAIL; /* anyway! */
                ndrx_TPset_error(TPEEVENT); /* We have event! */
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
                NDRX_LOG(log_debug, "Buffer type id: %d", rply->buffer_type_id);
                if (rply->data_len > 0)
                {
                    call_type = &G_buf_descr[rply->buffer_type_id];

                    ret=call_type->pf_prepare_incoming(call_type,
                                    rply->data,
                                    rply->data_len,
                                    data,
                                    len,
                                    flags);
                    
                    /* TODO: Check buffer acceptance or do it inside of prepare_incoming? */
                    if (ret==EXFAIL)
                    {
                        goto out;
                    }
                }
                
#if 0
                /* if all OK, then finally check the reply answer */
                if (TPSUCCESS!=rply->rval)
                {
                    ndrx_TPset_error_fmt(TPESVCFAIL, "Service returned %d", rply->rval);
                    ret=EXFAIL;
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
                    G_atmi_tls->M_svc_return_code = rply->rcode;

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
                    if (EXSUCCEED!=normal_connection_shutdown(conv, EXTRUE))
                    {
                        ret=EXFAIL;
                        goto out;
                    }
                    ret=EXFAIL; /* anyway! */
                    ndrx_TPset_error(TPEEVENT); /* We have event! */
                    goto out;
                }

                if (rply->flags & TPSENDONLY)
                {
                    NDRX_LOG(log_debug, "Sender program issued TPRECVONLY "
                                    "- so we become TPSENDONLY!");
                    ret=EXFAIL;
                    conv->revent = *revent = TPEV_SENDONLY;

                    ndrx_TPset_error_fmt(TPEEVENT, "Got event TPEV_SENDONLY");
                    
                    /* Set our conversation as senders */
                    conv->flags|=TPSENDONLY;
                    conv->flags&=~TPRECVONLY;
                }
            } /* else - normal reply/data from send! */
        } /* reply received ok */
    }
out:
    NDRX_LOG(log_debug, "%s return %d",  __func__, ret);

    if (G_atmi_tls->G_atmi_xa_curtx.txinfo)
    {
        if (TPEV_DISCONIMM == *revent ||  TPEV_SVCERR == *revent ||  
                TPEV_SVCFAIL == *revent)
        {
             NDRX_LOG(log_warn, "tprcv error - mark "
                     "transaction as abort only!");    
            /* later should be handled by transaction initiator! */
            G_atmi_tls->G_atmi_xa_curtx.txinfo->tmtxflags |= TMTXFLAGS_IS_ABORT_ONLY;
        }
        /* If we got any reply, and it contains abort only flag
         * (for current transaction, then we abort it...)
         */
        if (0==strcmp(G_atmi_tls->G_atmi_xa_curtx.txinfo->tmxid, rply->tmxid))
                
        {
            if (rply->tmtxflags & TMTXFLAGS_IS_ABORT_ONLY)
            {
                G_atmi_tls->G_atmi_xa_curtx.txinfo->tmtxflags |= TMTXFLAGS_IS_ABORT_ONLY;
            }
            
            /* Update known RMs */
            if ( !(rply->tmtxflags & TMTXFLAGS_IS_ABORT_ONLY) &&
                    EXEOS!=rply->tmknownrms[0] &&
                    EXSUCCEED!=atmi_xa_update_known_rms(
                        G_atmi_tls->G_atmi_xa_curtx.txinfo->tmknownrms, 
                        rply->tmknownrms))
            {
                G_atmi_tls->G_atmi_xa_curtx.txinfo->tmtxflags |= TMTXFLAGS_IS_ABORT_ONLY;
                EXFAIL_OUT(ret);
            }
        }   
    }

    NDRX_FREE(rply_buf);
    
    return ret;
}

/**
 * Receive any un-expected messages.
 * With this we catch the cases when server have been illegally made tpreturn!
 * @return
 */
exprivate void process_unsolicited_messages(int cd)
{
    short command_id=ATMI_COMMAND_CONNUNSOL;
    char *data=NULL;
    long len;
    long revent;

    /* Flush down all messages */
    while (EXSUCCEED==ndrx_tprecv (cd, &data, &len, TPNOBLOCK, &revent, &command_id))
    {
        NDRX_LOG(log_debug, "Ignoring unsolicited message!");
        /* Free up data */
        tpfree(data);
        data=NULL;
    }

}

/**
 * Send the message to conversation
 * @param cd
 * @param data
 * @param len
 * @param flags
 * @param revent
 * @param command_id
 * @return 
 */
expublic int ndrx_tpsend (int cd, char *data, long len, long flags, long *revent,
                            short command_id)
{
    int ret=EXSUCCEED;
    typed_buffer_descr_t *descr;
    buffer_obj_t *buffer_info=NULL;
    char buf[NDRX_MSGSIZEMAX];
    long data_len = MAX_CALL_DATA_SIZE;
    tp_command_call_t *call = (tp_command_call_t *)buf;
    tp_conversation_control_t *conv;
    
    ATMI_TLS_ENTRY;

    NDRX_LOG(log_debug, "%s: called", __func__);
    *revent = 0;

    /* Check the current connection descriptor */
    if (NULL==(conv=get_current_connection(cd)))
    {
        ndrx_TPset_error_fmt(TPEINVAL, "%s: Invalid connection descriptor %d",  __func__, cd);
        EXFAIL_OUT(ret);
    }

    /* Check are we allowed to receive? */
    if (ATMI_COMMAND_CONVDATA==command_id && conv->flags & TPRECVONLY)
    {
        ret=EXFAIL;
        ndrx_TPset_error_fmt(TPEPROTO, "%s: Not allowed to receive "
                                    "because flags say: TPRECVONLY!", __func__);
        goto out;
    }
    
    /* Manage the flags for active connection */
    if (flags & TPRECVONLY)
    {
        NDRX_LOG(log_debug, "%s: Program issued TPRECVONLY", __func__);
        /* Set our conversation as senders */
        conv->flags|=TPRECVONLY;
        conv->flags&=~TPSENDONLY;
    }

    memset(call, 0, sizeof(*call));

    /* Change the mode in which we run.\
     * We may receive message in async mode.
     */
    if (EXSUCCEED!=ndrx_setup_queue_attrs(&conv->reply_q_attr, conv->reply_q,
                                    conv->reply_q_str, flags))
    {
        ret=EXFAIL;
        goto out;
    }

    /* Check for any messages & process them */
    process_unsolicited_messages(cd);
    
    /*
     * Check are we still in connection?
     */
    if (CONV_IN_CONVERSATION!=conv->status)
    {
        ret=EXFAIL;

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
        normal_connection_shutdown(conv, EXFALSE);
        ret=EXFAIL;
        ndrx_TPset_error(TPEEVENT); /* Set that we have event for caller! */
        goto out;
    }
    
    /*
     * Prepare some data if have something to prepare
     */
    if (NULL!=data)
    {
        /* fill up the details */
        if (NULL==(buffer_info = ndrx_find_buffer(data)))
        {
            ndrx_TPset_error_fmt(TPEINVAL, "Buffer %p not known to system!", __func__);
            ret=EXFAIL;
            goto out;
        }

        descr = &G_buf_descr[buffer_info->type_id];

        /* prepare buffer for call */
        if (EXSUCCEED!=descr->pf_prepare_outgoing(descr, data, len, call->data, &data_len, flags))
        {
            /* not good - error should be already set */
            ret=EXFAIL;
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

    call->callseq = conv->callseq;
    call->msgseq = conv->msgseqout;
    
    
    /* So here is little trick for bridge.
     * As it needs to know where reply should go, we should leave here original caller
     * reply address, but pack in different my_listen_q_str
     */
    NDRX_LOG(log_debug, "Our address is: [%s], their reply address must be: [%s]. "
            "Callseq: %hu, msgseq: %hu",
            conv->my_listen_q_str, conv->reply_q_str, 
            call->callseq, call->msgseq);
    
    NDRX_STRCPY_SAFE(call->reply_to, conv->reply_q_str);
     
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
    
    /* Add global transaction info for sending... */
    if (G_atmi_tls->G_atmi_xa_curtx.txinfo)
    {
        NDRX_LOG(log_info, "Current process in global transaction (%s) - "
                "prepare call", G_atmi_tls->G_atmi_xa_curtx.txinfo->tmxid);
        
        atmi_xa_cpy_xai_to_call(call, G_atmi_tls->G_atmi_xa_curtx.txinfo);
    }
    
    /* And then we call out the service. */
    if (EXSUCCEED!=(ret=ndrx_generic_qfd_send(conv->reply_q, (char *)call, data_len, flags)))
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

        ndrx_TPset_error_fmt(err, "%s: Failed to send, os err: %s",  __func__, strerror(ret));
        ret=EXFAIL;

        goto out;
    }
    else
    {
        /* schedule next delivery */
        conv->msgseqout++;
    }

    if (conv->handshaked && EXSUCCEED!=ndrx_get_ack(conv, flags))
    {
        ret=EXFAIL;
        goto out;
    }

out:
    /* TODO: Kill conversation if FAILED!!!! */
    NDRX_LOG(log_debug, "%s: return %d",  __func__, ret);
    return ret;
}


/**
 * Close connection imideatelly
 * @param cd
 * @return SUCCEED/FAIL
 */
expublic int ndrx_tpdiscon (int cd)
{
    int ret=EXSUCCEED;
    long revent;
    tp_conversation_control_t *conv;
    
    /* Check our role */
    if (NULL==(conv=get_current_connection(cd)))
    {
        ndrx_TPset_error_fmt(TPEINVAL, "%s: Invalid connection descriptor %d",  __func__, cd);
        ret=EXFAIL;
        goto out;
    }

    /* Send disconnect command to server */
    if (EXFAIL==ndrx_tpsend (cd, NULL, 0L, 0L, &revent, ATMI_COMMAND_DISCONN))
    {
        NDRX_LOG(log_debug, "Failed to send disconnect to server - IGNORE!");
    }

    /* Close down then connection (We close down this only if we are server!)*/
    
    if (EXFAIL==normal_connection_shutdown(conv, EXTRUE))
    {
        ret=EXFAIL;
        goto out;
    }
    
out:
    NDRX_LOG(log_warn, "%s: return %d",  __func__, ret);
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
