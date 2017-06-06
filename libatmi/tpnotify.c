/* 
** Send notification message to client
**
** @file tpnotify.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, Mavimax, Ltd. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or Mavimax's license for commercial use.
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
** A commercial use license is available from Mavimax, Ltd
** contact@mavimax.com
** -----------------------------------------------------------------------------
*/

/*---------------------------Includes-----------------------------------*/
#include <stdio.h>
#include <stdarg.h>
#include <memory.h>
#include <stdlib.h>
#include <errno.h>
#include <sys_mqueue.h>
#include <fcntl.h>

#include <atmi.h>
#include <ndebug.h>
#include <tperror.h>
#include <typed_buf.h>
#include <atmi_int.h>

#include "../libatmisrv/srv_int.h"
#include "ndrxd.h"
#include <thlock.h>
#include <xa_cmn.h>
#include <atmi_shm.h>
#include <atmi_tls.h>
#include <utlist.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * tpnotify & tpbroadcast core.
 * We will not support TPACK (currently) as it looks like seperate q
 * is needed for this and standard reply Q can cause some deadlocks...
 * 
 * @param clientid optional client id to send data to
 * @param p_clientid_myid parsed client id (target to send data to)
 * @param data XATMI allocated data or NULL
 * @param len data len
 * @param flags XATMI flags TPREGEXMATCH - do regexp match over the username, cltname, nodeid
 * @param dest_node If doing TPCALL_BRCALL then node id to deliver to
 * @param usrname string/regexp or NULL
 * @param cltname string/regexp or NULL
 * @param nodeid string/regexp or NULL
 * @param ex_flags So TPCALL_BROADCAST - when doing broadcast, TPCALL_BRCALL - call bridge
 * @return 
 */
public int _tpnotify(CLIENTID *clientid, TPMYID *p_clientid_myid, 
        char *data, long len, long flags, 
        int dest_node, char *usrname,  char *cltname, char *nodeid, /* RFU */
        int ex_flags)
{
    int ret=SUCCEED;
    char buf[ATMI_MSG_MAX_SIZE];
    tp_notif_call_t *call=(tp_notif_call_t *)buf;
    typed_buffer_descr_t *descr;
    buffer_obj_t *buffer_info;
    long data_len = MAX_CALL_DATA_SIZE;
    char send_q[NDRX_MAX_Q_SIZE+1];
    char *fn = "_tpnotify";
    time_t timestamp;
    int is_bridge;
    int tpcall_cd;
    ATMI_TLS_ENTRY;
    
    NDRX_LOG(log_debug, "%s enter", fn);

    /* Might want to remove in future... but it might be dangerous!*/
    memset(call, 0, sizeof(tp_notif_call_t));
    
    /* Check service availability by SHM? 
     * TODO: We might want to check the flags, the service might be marked for shutdown!
     */
    if (ex_flags & TPCALL_BRCALL)
    {
        /* If this is a bridge call, then format the bridge Q */
#ifdef EX_USE_POLL
        {
            int tmp_is_bridge;
            char tmpsvc[MAXTIDENT+1];
            
            snprintf(tmpsvc, sizeof(tmpsvc), NDRX_SVC_BRIDGE, dest_node);

            if (SUCCEED!=ndrx_shm_get_svc(tmpsvc, send_q, &tmp_is_bridge))
            {
                NDRX_LOG(log_error, "Failed to get bridge svc: [%s]", 
                        tmpsvc);
                FAIL_OUT(ret);
            }
        }
#else
        snprintf(send_q, sizeof(send_q), NDRX_SVC_QBRDIGE, 
                G_atmi_tls->G_atmi_conf.q_prefix, dest_node);
#endif
        
        is_bridge=TRUE;
    }
    else
    {
        /* we are sending to client directly thus setup the q properly... */
        
        if (SUCCEED!=ndrx_myid_convert_to_q(p_clientid_myid, send_q, 
                sizeof(send_q)))
        {
            _TPset_error_fmt(TPEINVAL, "Failed to translate client data [%s] to Q", 
                    clientid->clientdata);
            FAIL_OUT(ret);
        }
    }
    
    if (NULL!=data)
    {
        if (NULL==(buffer_info = ndrx_find_buffer(data)))
        {
            _TPset_error_fmt(TPEINVAL, "Buffer %p not known to system!", fn);
            FAIL_OUT(ret);
        }
    }

    if (NULL!=data)
    {
        descr = &G_buf_descr[buffer_info->type_id];
        /* prepare buffer for call */
        if (SUCCEED!=descr->pf_prepare_outgoing(descr, data, len, call->data, 
                &data_len, flags))
        {
            /* not good - error should be already set */
            FAIL_OUT(ret);
        }
    }
    else
    {
        data_len=0;
    }

    /* OK, now fill up the details */
    call->data_len = data_len;
    
    data_len+=sizeof(tp_notif_call_t);

    if (NULL==data)
        call->buffer_type_id = BUF_TYPE_NULL;
    else
        call->buffer_type_id = buffer_info->type_id;

    NDRX_STRCPY_SAFE(call->reply_to, G_atmi_tls->G_atmi_conf.reply_q_str);
    
    /* If call to bridge, then it is broadcast... */
    if (!(ex_flags & TPCALL_BROADCAST))
    {
        call->command_id = ATMI_COMMAND_BROADCAST;
    }
    else
    {
        call->command_id = ATMI_COMMAND_TPNOTIFY;
    }
    
    call->flags = flags;
    timestamp = time(NULL);
    
    /* lock call descriptor */
    if (flags & TPACK)
    {
        NDRX_LOG(log_warn, "TPACK set but not supported. Ignoring...");
        tpcall_cd = 0;
    }
    else
    {
        NDRX_LOG(log_debug, "TPACK not set, => cd=0 - no reply needed");
        tpcall_cd = 0;
    }
    
    call->cd = tpcall_cd;
    call->timestamp = timestamp;
    
    if (NULL!=usrname)
    {
        NDRX_STRCPY_SAFE(call->usrname, usrname);
    }
    else
    {
        call->usrname_isnull = TRUE;
    }
    
    if (NULL!=cltname)
    {
        NDRX_STRCPY_SAFE(call->cltname, cltname);
    }
    else
    {
        call->cltname_isnull = TRUE;
    }
    
    if (NULL!=nodeid)
    {
        NDRX_STRCPY_SAFE(call->nodeid, nodeid);
    }
    else
    {
        call->nodeid_isnull = TRUE;
    }
    
    /* Reset call timer */
    ndrx_timer_reset(&call->timer);
    
    NDRX_STRCPY_SAFE(call->my_id, G_atmi_tls->G_atmi_conf.my_id); /* Setup my_id */
    
    NDRX_LOG(log_debug, "Sending notification request to: [%s] my_id=[%s] "
            "reply_to=[%s] cd=%d callseq=%u", 
            send_q, call->my_id, call->reply_to, tpcall_cd, call->callseq);
    
    NDRX_DUMP(log_dump, "Sending away...", (char *)call, data_len);

    if (SUCCEED!=(ret=generic_q_send(send_q, (char *)call, data_len, flags, 
            NDRX_MSGPRIO_NOTIFY)))
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
        FAIL_OUT(ret);
    }
    
out:
    NDRX_LOG(log_debug, "%s return %d", fn, ret);
    return ret;
}

/**
 * Entry point when notification received...
 * @param buf buffer received (full) from Q
 * @param len buffer len
 */
public void ndrx_process_notif(char *buf, long len)
{
    int ret = SUCCEED;
    char *odata = NULL;
    long olen = 0;
    typed_buffer_descr_t *call_type;
    public buffer_obj_t * typed_buf = NULL;
    tp_notif_call_t *notif = (tp_notif_call_t *) buf;
    
    NDRX_LOG(log_debug, "%s: Got notification from: [%s], data len: %d: ", 
            __func__, notif->my_id, notif->data_len);
    
    if (NULL==G_atmi_tls->p_unsol_handler)
    {
        NDRX_LOG(log_warn, "Unsol handler not set - dropping message");
        FAIL_OUT(ret);
    }
    
    /* Convert only if we have data */
    if (notif->data_len > 0)
    {
        NDRX_LOG(log_debug, "%s: data received", __func__);
        call_type = &G_buf_descr[notif->buffer_type_id];

        if (SUCCEED==(call_type->pf_prepare_incoming(call_type,
                        notif->data,
                        notif->data_len,
                        &odata,
                        &olen,
                        0L)))
        {
            typed_buf = ndrx_find_buffer(odata);
        }
        else
        {
            NDRX_LOG(log_error, "Failed to prepare incoming unsolicited notification");
            FAIL_OUT(ret);
        }
    }
    else
    {
        NDRX_LOG(log_debug, "%s: no data received - empty invocation", __func__);
    }
    
    NDRX_LOG(log_debug, "Unsol handler set to %p - invoking (buffer: %p)", 
            G_atmi_tls->p_unsol_handler, odata);
    
    /* Hmm we shall mark the buffer somehow, that this is ours, so that
     * we can later free. As the user can realloc it...
     */
    G_atmi_tls->p_unsol_handler(odata, olen, 0);

out:    

    if (NULL!=typed_buf)
    {
        NDRX_LOG(log_debug, "About to free buffer %p", typed_buf->buf);
        tpfree(typed_buf->buf);
    }

    return;
}

/**
 * So we get the messages, just check first, as it must be with higher
 * priority than zero, it should arrive first. If we get any other type messages
 * Just push them to in memory queue which will be later processed by _tpgetrply()
 * @return SUCCEED/FAIL
 */
public int _tpchkunsol(void) 
{
    int ret = SUCCEED;
    char *pbuf = NULL;
    size_t pbuf_len;
    size_t rply_len;
    unsigned prio;
    tpmemq_t *tmp;
    tp_notif_call_t *notif;
    
    /* Allocate the buffer... to put data into */
    NDRX_SYSBUF_MALLOC_OUT(pbuf, &pbuf_len, ret);

    rply_len = generic_q_receive(G_atmi_tls->G_atmi_conf.reply_q, pbuf,
                                           pbuf_len, &prio, TPNOBLOCK);
    if (rply_len<=0)
    {
        NDRX_LOG(log_warn, "%s: No message (%lu)", __func__, (unsigned long)rply_len);
        goto out;
    }
    
    notif=(tp_notif_call_t *) pbuf;
    
    /* could use %zu,  but we must be max cross platform... */
    NDRX_LOG(log_info, "%s: got message, len: %lu, command id: %d", 
            __func__, (unsigned long)rply_len, notif->command_id);
    
    if (ATMI_COMMAND_TPNOTIFY == notif->command_id ||
            ATMI_COMMAND_BROADCAST == notif->command_id)
    {
        NDRX_LOG(log_info, "Got unsol command");
        ndrx_process_notif(pbuf, rply_len);
        NDRX_FREE(pbuf);
        pbuf = NULL;
    }
    else
    {
        NDRX_LOG(log_info, "got non unsol command - enqueue");
        
        if (NULL==(tmp = NDRX_CALLOC(1, sizeof(tpmemq_t))))
        {
            int err = errno;
            NDRX_LOG(log_error, "Failed to alloc: %s", strerror(err));
            userlog("Failed to alloc: %s", strerror(err));
            FAIL_OUT(ret);
        }
        
        tmp->buf = pbuf;
        tmp->len = pbuf_len;
        tmp->data_len = rply_len;

        DL_APPEND(G_atmi_tls->memq, tmp); 
    }
out:

    if (NULL!=pbuf)
    {
        NDRX_FREE(pbuf);
    }
    NDRX_LOG(log_debug, "%s returns %d", __func__, ret);

    return ret;
}

/**
 * Local broadcast, sends the message to 
 * @param nodeid NULL, empty string or regexp of cluster nodes. not used on local
 * @param usrname NULL, empty string or regexp of username. Not used.
 * @param cltname NULL, empty string or regexp of client name. Used.
 * @param data Data to broadcast to process...
 * @param len Data len
 * @param flags here TPREGEXMATCH flag affects node/usr/clt
 * @return SUCCEED/FAIL
 */
public int _tpbroadcast_local(char *nodeid, char *usrname, char *cltname, 
        char *data,  long len, long flags)
{
    int ret = SUCCEED;
    string_list_t* qlist = NULL;
    string_list_t* elt = NULL;
    int typ;
    /* So list all client queues locally
     * Match them
     * Build client ID
     * and run _tpnotify
     */
      
    /* list all queues */
    qlist = ndrx_sys_mqueue_list_make(G_atmi_env.qpath, &ret);
    
    if (SUCCEED!=ret)
    {
        NDRX_LOG(log_error, "posix queue listing failed... continue...!");
        ret = SUCCEED;
        qlist = NULL;
    }
    
    LL_FOREACH(qlist,elt)
    {
        /* currently we will match cltname only and will work on
         * server & client reply qs 
         * because server can have reply q too... as we know.
         */
        
        typ = ndrx_q_type_get(elt->qname);
        
        if (NDRX_QTYPE_CLTRPLY==typ)
        {
            /* This is our client, lets broadcast to it... 
             * Build client id..
             */
            NDRX_LOG(log_debug, "Got client Q: [%s] - extract CLIENTID", elt->qname);
        }
    }
    
out:
    ndrx_string_list_free(qlist);   
    return ret;
}