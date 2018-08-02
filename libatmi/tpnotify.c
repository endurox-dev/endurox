/* 
 * @brief Send notification message to client
 *
 * @file tpnotify.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or ATR Baltic's license for commercial use.
 * -----------------------------------------------------------------------------
 * GPL license:
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * -----------------------------------------------------------------------------
 * A commercial use license is available from Mavimax, Ltd
 * contact@mavimax.com
 * -----------------------------------------------------------------------------
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
#include "userlog.h"
#include "exregex.h"
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
expublic int ndrx_tpnotify(CLIENTID *clientid, TPMYID *p_clientid_myid, 
        char *cltq, /* client q already built by broadcast */
        char *data, long len, long flags, 
        int dest_node, char *nodeid, char *usrname,  char *cltname,
        int ex_flags)
{
    int ret=EXSUCCEED;
    char buf[NDRX_MSGSIZEMAX];
    tp_notif_call_t *call=(tp_notif_call_t *)buf;
    typed_buffer_descr_t *descr;
    buffer_obj_t *buffer_info;
    long data_len = MAX_CALL_DATA_SIZE;
    char send_q[NDRX_MAX_Q_SIZE+1];
    time_t timestamp;
    int is_bridge;
    int tpcall_cd;
    long local_node = tpgetnodeid();
    ATMI_TLS_ENTRY;
    
    NDRX_LOG(log_debug, "%s enter", __func__);

    /* Might want to remove in future... but it might be dangerous!*/
    memset(call, 0, sizeof(tp_notif_call_t));
   
    /* Set the dest client id */
    if (NULL!=clientid)
    {
        NDRX_STRCPY_SAFE(call->destclient, clientid->clientdata);
    }
    
    call->destnodeid = 0;
    /* Check service availability by SHM? 
     */
    if (ex_flags & TPCALL_BRCALL)
    {
        call->destnodeid = dest_node;
        /* If this is a bridge call, then format the bridge Q */
#ifdef EX_USE_POLL
        {
            int tmp_is_bridge;
            char tmpsvc[MAXTIDENT+1];
            
            snprintf(tmpsvc, sizeof(tmpsvc), NDRX_SVC_BRIDGE, dest_node);

            if (EXSUCCEED!=ndrx_shm_get_svc(tmpsvc, send_q, &tmp_is_bridge,
                NULL))
            {
                NDRX_LOG(log_error, "Failed to get bridge svc: [%s]", 
                        tmpsvc);
                EXFAIL_OUT(ret);
            }
        }
#else
        snprintf(send_q, sizeof(send_q), NDRX_SVC_QBRDIGE, 
                G_atmi_tls->G_atmi_conf.q_prefix, dest_node);
#endif
        
        is_bridge=EXTRUE;
    }
    else if (NULL!=cltq)
    {
        /* We have Q build already */
        NDRX_STRCPY_SAFE(send_q, cltq);
    }
    else
    {
        /* we are sending to client directly thus setup the q properly... 
         * But client could resist on different cluster node
         * Thus we need to check their node...
         */
        
        if (EXSUCCEED!=ndrx_myid_convert_to_q(p_clientid_myid, send_q, 
                sizeof(send_q)))
        {
            ndrx_TPset_error_fmt(TPEINVAL, "Failed to translate client data [%s] to Q", 
                    clientid->clientdata);
            EXFAIL_OUT(ret);
        }
        
        if (p_clientid_myid->nodeid!=local_node)
        {
            NDRX_LOG(log_info, "The client [%s] resists on another node [%ld], "
                    "thus send in cluster", clientid->clientdata, 
                    (long)p_clientid_myid->nodeid);
            
            /* Will do call in recursive way so that cluster code picks up
             * this dispatch...
             */
            return ndrx_tpnotify(clientid, p_clientid_myid, 
                        cltq, /* client q already built by broadcast */
                        data, len, flags, 
                        p_clientid_myid->nodeid, 
                        nodeid, usrname, cltname,ex_flags | TPCALL_BRCALL);
            
        }
    }
    
    if (NULL!=data)
    {
        if (NULL==(buffer_info = ndrx_find_buffer(data)))
        {
            ndrx_TPset_error_fmt(TPEINVAL, "Buffer %p not known to system!", __func__);
            EXFAIL_OUT(ret);
        }
    }

    if (NULL!=data)
    {
        descr = &G_buf_descr[buffer_info->type_id];
        /* prepare buffer for call */
        if (EXSUCCEED!=descr->pf_prepare_outgoing(descr, data, len, call->data, 
                &data_len, flags))
        {
            /* not good - error should be already set */
            EXFAIL_OUT(ret);
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
    if (ex_flags & TPCALL_BROADCAST)
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
        call->usrname[0] = EXEOS;
        call->usrname_isnull = EXTRUE;
    }
    
    if (NULL!=cltname)
    {
        NDRX_STRCPY_SAFE(call->cltname, cltname);
    }
    else
    {
        call->cltname[0] = EXEOS;
        call->cltname_isnull = EXTRUE;
    }
    
    if (NULL!=nodeid)
    {
        NDRX_STRCPY_SAFE(call->nodeid, nodeid);
    }
    else
    {
        call->nodeid[0] = EXEOS;
        call->nodeid_isnull = EXTRUE;
    }
    
    /* Reset call timer */
    ndrx_stopwatch_reset(&call->timer);
    
    NDRX_STRCPY_SAFE(call->my_id, G_atmi_tls->G_atmi_conf.my_id); /* Setup my_id */
    
    NDRX_LOG(log_debug, "Sending notification request to: [%s] my_id=[%s] "
            "reply_to=[%s] cd=%d callseq=%u", 
            send_q, call->my_id, call->reply_to, tpcall_cd, call->callseq);
    
    NDRX_DUMP(log_dump, "Sending away...", (char *)call, data_len);

    if (EXSUCCEED!=(ret=ndrx_generic_q_send(send_q, (char *)call, data_len, flags, 
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
        ndrx_TPset_error_fmt(err, "%s: Failed to send, os err: %s", __func__, strerror(ret));
        EXFAIL_OUT(ret);
    }
    
out:
    NDRX_LOG(log_debug, "%s return %d", __func__, ret);
    return ret;
}

/**
 * Entry point when notification received...
 * @param buf buffer received (full) from Q
 * @param len buffer len
 */
expublic void ndrx_process_notif(char *buf, long len)
{
    int ret = EXSUCCEED;
    char *odata = NULL;
    long olen = 0;
    typed_buffer_descr_t *call_type;
    expublic buffer_obj_t * typed_buf = NULL;
    tp_notif_call_t *notif = (tp_notif_call_t *) buf;
    
    NDRX_LOG(log_debug, "%s: Got notification from: [%s], data len: %d: ", 
            __func__, notif->my_id, notif->data_len);
    
    if (NULL==G_atmi_tls->p_unsol_handler)
    {
        NDRX_LOG(log_warn, "Unsol handler not set - dropping message");
        goto out;
    }
    
    if (G_atmi_tls->client_init_data.flags & TPU_IGN)
    {
        NDRX_LOG(log_warn, "TPU_IGN have been set - dropping message");
        goto out;
    }
    
    /* Convert only if we have data */
    if (notif->data_len > 0)
    {
        NDRX_LOG(log_debug, "%s: data received", __func__);
        call_type = &G_buf_descr[notif->buffer_type_id];

        if (EXSUCCEED==(call_type->pf_prepare_incoming(call_type,
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
            EXFAIL_OUT(ret);
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
expublic int ndrx_tpchkunsol(void) 
{
    int ret = EXSUCCEED;
    char *pbuf = NULL;
    size_t pbuf_len;
    long rply_len;
    int num_applied = 0;
    unsigned prio;
    tpmemq_t *tmp;
    tp_notif_call_t *notif;
    
    /* Allocate the buffer... to put data into */
    NDRX_SYSBUF_MALLOC_OUT(pbuf, &pbuf_len, ret);

    NDRX_LOG(log_debug, "Into %s", __func__);
    do
    {
        rply_len = ndrx_generic_q_receive(G_atmi_tls->G_atmi_conf.reply_q, 
                G_atmi_tls->G_atmi_conf.reply_q_str,
                &(G_atmi_tls->G_atmi_conf.reply_q_attr),
                pbuf, pbuf_len, &prio, TPNOBLOCK);
        
        NDRX_LOG(log_debug, "%s: %lu", __func__, (long)rply_len);
        
        if (rply_len<=0)
        {
            NDRX_LOG(log_warn, "%s: No message (%ld)", __func__,  rply_len);
            goto out;
        }

        notif=(tp_notif_call_t *) pbuf;

        /* could use %zu,  but we must be max cross platform... */
        NDRX_LOG(log_info, "%s: got message, len: %ld, command id: %d", 
                __func__, rply_len, notif->command_id);

        if (ATMI_COMMAND_TPNOTIFY == notif->command_id ||
                ATMI_COMMAND_BROADCAST == notif->command_id)
        {
            num_applied++;
            NDRX_LOG(log_info, "Got unsol command");
            ndrx_process_notif(pbuf, rply_len);
	    /* are we sure we want to have free here of sysbuf? 
            NDRX_FREE(pbuf);
            pbuf = NULL;
	     */
        }
        else
        {
            NDRX_LOG(log_info, "got non unsol command - enqueue");

            if (NULL==(tmp = NDRX_CALLOC(1, sizeof(tpmemq_t))))
            {
                int err = errno;
                NDRX_LOG(log_error, "Failed to alloc: %s", strerror(err));
                userlog("Failed to alloc: %s", strerror(err));
                EXFAIL_OUT(ret);
            }

            tmp->buf = pbuf;
	    pbuf = NULL; /* save the buffer... */
            tmp->len = pbuf_len;
            tmp->data_len = rply_len;

            DL_APPEND(G_atmi_tls->memq, tmp); 
        }
        /* Note loop will be terminated if not message in Q */
    } while (EXSUCCEED==ret);
out:

    if (NULL!=pbuf)
    {
        NDRX_FREE(pbuf);
    }

    NDRX_LOG(log_debug, "%s returns %d (applied msgs: %d)", __func__, 
            ret, num_applied);

    if (EXSUCCEED==ret)
    {
        return num_applied;
    }
    else
    {
        return ret;
    }
}

/**
 * Match the given node against the dispatch arguments
 * @param nodeid_str nodeid to test (string)
 * @param nodeid nodeid/lmid in tpbroadcast
 * @param regexp_nodeid compiled regexp (if TPREGEXMATCH used)
 * @param flags flags passed to tpbrodcast
 * @return TRUE node accepted, FALSE not accepted
 */
exprivate int match_nodeid(char *nodeid_str,  char *nodeid, 
        regex_t *regexp_nodeid, long flags)
{
    int ret = EXFALSE;
    
    if (NULL!=nodeid)
    {
        if (EXEOS==nodeid[0])
        {
            NDRX_LOG(log_info, "Nodeid %s (nodeid=EOS)", nodeid_str);
            ret = EXTRUE;
        }
        else if ((flags & TPREGEXMATCH ) && 
                EXSUCCEED==ndrx_regexec(regexp_nodeid, nodeid_str))
        {
            NDRX_LOG(log_info, "Nodeid %s matched local node by regexp",
                    nodeid_str);
            ret = EXTRUE;
        }
        else if (0==strcmp(nodeid, nodeid_str))
        {
            NDRX_LOG(log_info, "Nodeid %s matched by nodeid str param",
                    nodeid_str);
            ret = EXTRUE;
        }
        else
        {
            NDRX_LOG(log_info, "Nodeid %s did not match nodeid param [%s] => "
                    "skip node for broadcast",
                    nodeid_str, nodeid);
        }
    }
    else
    {
        NDRX_LOG(log_info, "nodeid param NULL, local node %s matched for broadcast",
                    nodeid_str);
            ret = EXTRUE;
    }
    
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
expublic int ndrx_tpbroadcast_local(char *nodeid, char *usrname, char *cltname, 
        char *data,  long len, long flags, int dispatch_local)
{
    int ret = EXSUCCEED;
    string_list_t* qlist = NULL;
    string_list_t* elt = NULL;
    int typ;
    TPMYID myid;
    ndrx_qdet_t qdet;
    CLIENTID cltid;
    regex_t regexp_nodeid;
    int     regexp_nodeid_comp = EXFALSE;
    
    regex_t regexp_usrname;
    int     regexp_usrname_comp = EXFALSE;
    
    regex_t regexp_cltname;
    int     regexp_cltname_comp = EXFALSE;
    char nodeid_str[16];
    
    int local_node_ok = EXFALSE;
    int cltname_ok;
    
    long local_nodeid = tpgetnodeid();
    
    char connected_nodes[CONF_NDRX_NODEID_COUNT+1] = {EXEOS};
    
    
    /* if the username is  */
    if (flags & TPREGEXMATCH)
    {
        /* Setup regexp matches (if any) */
        
        if (NULL!=nodeid)
        {
            if (EXSUCCEED!=ndrx_regcomp(&regexp_nodeid, nodeid))
            {
                ndrx_TPset_error_fmt(TPEINVAL, "Failed to compile nodeid=[%s] regexp",
                        __func__, nodeid);
                NDRX_LOG(log_error, "Failed to compile nodeid=[%s]", nodeid);
                EXFAIL_OUT(ret);
            }
            else
            {
                regexp_nodeid_comp = EXTRUE;
            }
        }
        
        if (NULL!=usrname)
        {
            if (EXSUCCEED!=ndrx_regcomp(&regexp_usrname, usrname))
            {
                ndrx_TPset_error_fmt(TPEINVAL, "Failed to compile usrname=[%s] regexp",
                        __func__, nodeid);
                NDRX_LOG(log_error, "Failed to compile usrname=[%s]", usrname);
                EXFAIL_OUT(ret);
            }
            else
            {
                regexp_usrname_comp = EXTRUE;
            }
        }
        
        if (NULL!=cltname)
        {
            if (EXSUCCEED!=ndrx_regcomp(&regexp_cltname, cltname))
            {
                ndrx_TPset_error_fmt(TPEINVAL, "Failed to compile cltname=[%s] regexp",
                        __func__, cltname);
                NDRX_LOG(log_error, "Failed to compile cltname=[%s]", cltname);
                EXFAIL_OUT(ret);
            }
            else
            {
                regexp_cltname_comp = EXTRUE;
            }
        }
    }
    
    /* So list all client queues locally
     * Match them
     * Build client ID
     * and run _tpnotify
     */
    
    /* Check our node here */
    snprintf(nodeid_str, sizeof(nodeid_str), "%ld", local_nodeid);
    
    local_node_ok=match_nodeid(nodeid_str,  nodeid,  &regexp_nodeid, flags);
    
    if (local_node_ok)
    {
        /* list all queues */
        qlist = ndrx_sys_mqueue_list_make(G_atmi_env.qpath, &ret);

        if (EXSUCCEED!=ret)
        {
            NDRX_LOG(log_error, "posix queue listing failed... continue...!");
            ret = EXSUCCEED;
            qlist = NULL;
        }

        LL_FOREACH(qlist,elt)
        {
            /* if not print all, then skip this queue */
            if (0!=strncmp(elt->qname, 
                    G_atmi_env.qprefix_match, G_atmi_env.qprefix_match_len))
            {
                continue;
            }
            
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
                NDRX_LOG(log_debug, "Got client Q: [%s] - extract CLIENTID",
                        elt->qname);

                /* parse q details */
                if (EXSUCCEED!=ndrx_qdet_parse_cltqstr(&qdet, elt->qname))
                {
                    NDRX_LOG(log_error, "Failed to parse Q details!");
                    EXFAIL_OUT(ret);
                }

                /* Check client name */
                cltname_ok = EXFALSE;
                
                if (NULL!=cltname)
                {
                    if (EXEOS==cltname[0])
                    {
                        NDRX_LOG(log_info, "Process matched broadcast [%s] "
                                "(cltname=EOS)", 
                                elt->qname);
                        cltname_ok = EXTRUE;
                    }
                    else if ((flags & TPREGEXMATCH )
                        && EXSUCCEED==ndrx_regexec(&regexp_cltname, qdet.binary_name))
                    {
                        NDRX_LOG(log_info, "Process [%s]/[%s] matched broadcast "
                                "by regexp",
                                elt->qname, qdet.binary_name);
                        cltname_ok = EXTRUE;
                    }
                    else if (0==strcmp(cltname, qdet.binary_name))
                    {
                        NDRX_LOG(log_info, "Process [%s]/[%s] matched by "
                                "cltname str param [%s]",
                                elt->qname, qdet.binary_name, cltname);
                        cltname_ok = EXTRUE;
                    }
                    else
                    {
                        NDRX_LOG(log_info, "Process [%s]/[%s] did not match "
                                "cltname param [%s] => "
                                "skip process for broadcast",
                                elt->qname, qdet.binary_name, cltname);
                    }
                }
                else
                {
                    NDRX_LOG(log_info, "cltname param NULL, process [%s]/[%s] "
                            "matched for broadcast",
                                elt->qname, qdet.binary_name);
                        cltname_ok = EXTRUE;
                }
                
                if (cltname_ok)
                {
                    /* Build myid */
                    if (EXSUCCEED!=ndrx_myid_convert_from_qdet(&myid, &qdet, local_nodeid))
                    {
                        NDRX_LOG(log_error, "Failed to build MYID from QDET!");
                        EXFAIL_OUT(ret);
                    }

                    /* Build my_id string */
                    ndrx_myid_to_my_id_str(&myid, cltid.clientdata);

                    NDRX_LOG(log_info, "Build client id string: [%s]",
                            cltid.clientdata);

                    if (EXSUCCEED!=ndrx_tpnotify(&cltid, &myid, elt->qname,
                        data, len, flags,  0, nodeid, usrname, cltname, 0))
                    {
                        NDRX_LOG(log_debug, "Failed to notify [%s] with buffer len: %d", 
                                cltid.clientdata, len);
                        userlog("Failed to notify [%s] with buffer len: %d", 
                                cltid.clientdata, len);
                    }
                }
            } /* if client reply q */
        } /* for each q */
    } /* local node ok */
    
    /* Process cluster nodes... */
    
    /* Check the nodes to broadcast to 
     * In future if we choose to route transaction via different nodes
     * then we need to broadcast the notification to all machines.
     */
    if (!dispatch_local)
    {
        NDRX_LOG(log_info, "Dispatching over any connected nodes");
        if (EXSUCCEED==ndrx_shm_birdge_getnodesconnected(connected_nodes))
        {
            int i;
            int len = strlen(connected_nodes);
            for (i=0; i<len; i++)
            {
                /* Sending stuff to connected nodes (if any matched) */
                snprintf(nodeid_str, sizeof(nodeid_str), "%d", 
                        (int)connected_nodes[i]);

                if (match_nodeid(nodeid_str,  nodeid,  &regexp_nodeid, flags))
                {
                    NDRX_LOG(log_debug, "Node id %d accepted for broadcast", 
                            (int)connected_nodes[i]);

                    if (EXSUCCEED!=ndrx_tpnotify(NULL, NULL, NULL,
                            data, len, flags, 
                            (long)connected_nodes[i], nodeid, usrname, cltname, 
                            (TPCALL_BRCALL | TPCALL_BROADCAST)))
                    {
                        NDRX_LOG(log_debug, "Failed to notify [%s] with buffer len: %d", 
                                cltid.clientdata, len);
                        userlog("Failed to notify [%s] with buffer len: %d", 
                                cltid.clientdata, len);
                    }
                }
            } /* for each node */
        } /* get cluster connected nodes */
    } /* is local dispatch? */
    else
    {
        NDRX_LOG(log_info, "Skip the cluster, local dispatch only...");
    }
    
out:

    ndrx_string_list_free(qlist);

    if (regexp_nodeid_comp)
    {
        ndrx_regfree(&regexp_nodeid);
    }

    if (regexp_usrname_comp)
    {
        ndrx_regfree(&regexp_usrname);
    }

    if (regexp_cltname_comp)
    {
        ndrx_regfree(&regexp_cltname);
    }

    NDRX_LOG(log_debug, "%s returns %d", __func__, ret);
    
    return ret;
}

