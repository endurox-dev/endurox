/**
 * @brief Enduro/X Queue processing (ATMI/NDRXD queues).
 *   Addtional processing:
 *   Internal message queue used for cases when target
 *   service queues are full. So that if we try to send, we do not
 *   block the whole bridge, but instead we leave the message in internal linked
 *   list and try later. If tries are exceeded time-out value, then we just drop
 *   the message.
 *
 * @file queue.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2019, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * AGPL (with Java and Go exceptions) or Mavimax's license for commercial use.
 * See LICENSE file for full text.
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <regex.h>
#include <utlist.h>

#include <ndebug.h>
#include <atmi.h>
#include <atmi_int.h>
#include <atmi_shm.h>
#include <typed_buf.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <Exfields.h>
#include <gencall.h>

#include <exnet.h>
#include <ndrxdcmn.h>
#include "bridge.h"
#include "../libatmisrv/srv_int.h"
#include "userlog.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

#define     PACK_TYPE_TONDRXD   1   /**< Send message NDRXD                   */
#define     PACK_TYPE_TOSVC     2   /**< Send to service, use timer (their)   */
#define     PACK_TYPE_TORPLYQ   3   /**< Send to reply q, use timer (internal)*/

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
exprivate in_msg_t *M_in_q = NULL;  /**< Linked list with incoming message in Q */
MUTEX_LOCKDECL(M_in_q_lock);        /**< Queue lock the queue cache             */
exprivate int M_qrun_issued = EXFALSE;/**< Indicate the that there is q runner job in
                                     * progress, because periodic timer then could
                                     * submit job several times
                                     */
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

exprivate int br_got_message_from_q_th(void *ptr, int *p_finish_off);
exprivate int br_process_error(char *buf, int len, int err, in_msg_t* from_q, 
        int pack_type, char *destqstr);


/**
 * Run queue from thread.
 * This is started from main thread periodic runner.
 * The special flag is used to indicate if run job was in queue
 * @param ptr not used
 * @param p_finish_off not used
 * @return EXSUCCEED;
 */
exprivate int br_run_q_th(void *ptr, int *p_finish_off)
{
    int ret = EXSUCCEED;
    in_msg_t *el, *elt;
    
    /**
     * Possible dead lock if service puts back in queue/ 
     * do the unlock in the middle to allow adding msg?
     * 
     * If delete is allowed only from this thread, then we should sync only
     * on adding..
     * 
     */
    MUTEX_LOCK_V(M_in_q_lock);
    
    /* loop runs in locked mode.. */
    DL_FOREACH_SAFE(M_in_q, el, elt)
    {
        MUTEX_UNLOCK_V(M_in_q_lock);
        
        el->tries++;
        NDRX_LOG(log_warn, "Processing late delivery of %p/%d [%s] try %d/%d", 
                el->buffer, el->len, el->destqstr, el->tries, G_bridge_cfg.qretries);
        
        if (el->tries <= G_bridge_cfg.qretries)
        {
            if (EXSUCCEED!=(ret=ndrx_generic_q_send(el->destqstr, (char *)el->buffer, 
                    el->len, TPNOBLOCK, 0)))
            {
                NDRX_LOG(log_error, "Failed to send message to [%s]: %s",
                        el->destqstr, tpstrerror(ret));
            }
            else
            {
                /* locking here needed.. */
                MUTEX_LOCK_V(M_in_q_lock);
                /* remove from Q - ok */
                DL_DELETE(M_in_q, el);
                NDRX_FREE(el->buffer);
                NDRX_FREE(el);
                MUTEX_UNLOCK_V(M_in_q_lock);
            }
        }
        else
        {
            br_process_error((char *)el->buffer, 
                    el->len, EXFAIL, el, PACK_TYPE_TOSVC, el->destqstr);
        }
        
        MUTEX_LOCK_V(M_in_q_lock);
    }
    
    MUTEX_UNLOCK_V(M_in_q_lock);
    
    M_qrun_issued = EXFALSE;
    
    return ret;
}

/**
 * Perform periodic run of the queue
 */
expublic void br_run_q(void)
{
    
    /* check if there is something is in q & not locked, then submit to
     * incoming worker thread to finish this off...
     */
    
    /* try lock, not cannot lock, then already in progress... 
     * Also the locking would mean that other worker thread would lock up
     * if trying to add to runner Q.
     * So better would be to do lock for each message.
     */
    if (EXSUCCEED==MUTEX_TRYLOCK_V(M_in_q_lock))
    {
        if (!M_qrun_issued && NULL!=M_in_q)
        {
            /* submit the job... */
            M_qrun_issued = EXTRUE;
            
            thpool_add_work(G_bridge_cfg.thpool_fromnet, (void *)br_run_q_th, NULL);
        }
        
        MUTEX_UNLOCK_V(M_in_q_lock);
    }
}

/**
 * Enqueue the message for delayed send.
 * @param call
 * @param len
 * @param from_q
 * @return 
 */
exprivate int br_add_to_q(char *buf, int len, int pack_type, char *destq)
{
    int ret=EXSUCCEED;
    in_msg_t *msg;
    
    if (NULL==(msg=NDRX_CALLOC(1, sizeof(in_msg_t))))
    {
        NDRX_ERR_MALLOC(sizeof(in_msg_t));
        EXFAIL_OUT(ret);
    }
    
    NDRX_SYSBUF_MALLOC_WERR_OUT(msg->buffer, NULL, ret);
    
    /*fill in the details*/
    msg->pack_type = pack_type;
    msg->len = len;
    NDRX_STRCPY_SAFE(msg->destqstr, destq);
    memcpy(msg->buffer, buf, len);
    
    ndrx_stopwatch_reset(&msg->trytime);
    
    /* Bug #465 moved  after the adding to Q */
    NDRX_LOG(log_warn, "Message %p/%d [%s] added to in-mem queue "
            "for late delivery...", msg->buffer, msg->len, msg->destqstr);
    
    MUTEX_LOCK_V(M_in_q_lock);
    DL_APPEND(M_in_q, msg);
    MUTEX_UNLOCK_V(M_in_q_lock);
    
out:

    if (NULL==msg->buffer && NULL!=msg)
    {
        NDRX_FREE(msg);
    }

    return ret;
}


/**
 * Generate error to network if required.
 * We detect the packet type here.
 * @param buf
 * @param len
 * @param pack_type
 * @return 
 */
exprivate int br_generate_error_to_net(char *buf, int len, int pack_type, long rcode)
{
    int ret=EXSUCCEED;
    
    switch(pack_type)
    {
        case PACK_TYPE_TONDRXD:
            /* Will not generate any error here*/
            break;
        case PACK_TYPE_TOSVC:
            /* So This was TPCALL, we might want to send error reply back. */
        {
            tp_command_call_t *call = (tp_command_call_t *)buf;
            
            if (!(call->flags & TPNOREPLY))
            {
                NDRX_LOG(log_warn, "Sending back error reply");
                reply_with_failure(TPNOBLOCK, call, NULL, NULL, rcode);
            }
        }
            break;
        case PACK_TYPE_TORPLYQ:
            /* Will not generate any error here*/
            break;
        default:
            NDRX_LOG(log_warn, "Nothing to do for pack_type=%d", 
                    pack_type);
            break;
    }
    
out:
    return ret;
}

/**
 * So we got q send error.
 * @param destqstr dest queue where message sent attempt was made
 *  in case of NULL, no enqueue required (failed to resolve service q, nothing todo)
 */
exprivate int br_process_error(char *buf, int len, int err, 
        in_msg_t* from_q, int pack_type, char *destqstr)
{
    long rcode = TPESVCERR;
    
    if (err==ENOENT)
    {
        rcode = TPENOENT;
    }
    
    /* So this is initial call */
    if (NULL==from_q && NULL!=destqstr)
    {
        /* This will be processed by queue runner */
        if (EAGAIN==err)
        {
            /* Put stuff in queue */
            br_add_to_q(buf, len, pack_type, destqstr);
        }
        else
        {
            /* TODO: Ignore the error or send error back - TPNOENT probably (if this is request) */
            br_generate_error_to_net(buf, len, pack_type, rcode);
        }
    }
    else
    {
        NDRX_LOG(log_debug, "Got error processing from Q");
        
        /* In this case we should handle the stuff!?!?
         * Generate reply (only if needed), Not sure TPNOTENT or svcerr?
         * SVCERR could be better because initially we thought that service exists.
         */
        br_generate_error_to_net(buf, len, pack_type, rcode);
        
        NDRX_DUMP(log_warn, "Discarding message", buf, len);
        
        userlog("Discarding message: %p/%d dest q: target q: [%s]", buf, len, 
                destqstr?destqstr:"(null)");
        
        if (EAGAIN!=err && NULL!=from_q)
        {
            MUTEX_LOCK_V(M_in_q_lock);
            /* Generate error reply */
            DL_DELETE(M_in_q, from_q);
            NDRX_FREE(from_q->buffer);
            NDRX_FREE(from_q);
            MUTEX_LOCK_V(M_in_q_lock);
        }
    }
    
    return EXSUCCEED;
}

/**
 * Send stuff directly to NDRXD
 */
expublic int br_submit_to_ndrxd(command_call_t *call, int len)
{
    int ret=EXSUCCEED;
    char *qstr = ndrx_get_G_atmi_conf()->ndrxd_q_str;
    
    if (EXSUCCEED!=(ret=ndrx_generic_q_send(qstr, 
            (char *)call, len, TPNOBLOCK, 0)))
    {
        NDRX_LOG(log_error, "Failed to send message to ndrxd!");
        br_process_error((char *)call, len, ret, NULL, PACK_TYPE_TONDRXD, qstr);
    }
    
out:
    return EXSUCCEED;    
}

/**
 * Submit to service. We should do this via Q
 */
expublic int br_submit_to_service(tp_command_call_t *call, int len)
{
    int ret=EXSUCCEED;
    char svc_q[NDRX_MAX_Q_SIZE+1];
    int is_bridge = EXFALSE;

    /* Resolve the service in SHM 
     *   sprintf(svc_q, NDRX_SVC_QFMT, G_server_conf.q_prefix, call->name); */

    if (EXSUCCEED!=ndrx_shm_get_svc(call->name, svc_q, &is_bridge, NULL))
    {
        NDRX_LOG(log_error, "Failed to get local service [%s] for bridge call!",
                call->name);
        userlog("Failed to get local service [%s] for bridge call!", call->name);
        br_process_error((char *)call, len, ret, NULL, PACK_TYPE_TOSVC, NULL);
        EXFAIL_OUT(ret);
    }

    
    NDRX_LOG(log_debug, "Calling service: %s", svc_q);
    if (EXSUCCEED!=(ret=ndrx_generic_q_send(svc_q, (char *)call, len, TPNOBLOCK, 0)))
    {
        NDRX_LOG(log_error, "Failed to send message to local ATMI service!");
        br_process_error((char *)call, len, ret, NULL, PACK_TYPE_TOSVC, svc_q);
    }
    /* TODO: Check the result, if called failed, then reply back with error? */
    
out:
    return EXSUCCEED;    
}

/**
 * Send message to broadcast service
 * @param call
 * @param len
 * @param from_q
 * @return 
 */
expublic int br_submit_to_service_notif(tp_notif_call_t *call, int len)
{
    int ret=EXSUCCEED;
    char svcnm[MAXTIDENT];
    int is_bridge = EXFALSE;
    char svc_q[NDRX_MAX_Q_SIZE+1];
    
    snprintf(svcnm, sizeof(svcnm), NDRX_SVC_TPBROAD, tpgetnodeid());

    if (EXSUCCEED!=ndrx_shm_get_svc(svcnm, svc_q, &is_bridge, NULL))
    {
        NDRX_LOG(log_error, "Failed to get local service [%s] for bridge call!",
                svcnm);
        userlog("Failed to get local service [%s] for bridge call!", svcnm);
        
        br_process_error((char *)call, len, ret, NULL, PACK_TYPE_TOSVC, NULL);
        
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_debug, "Calling broadcast server: %s with flags %ld", 
            svc_q, call->flags);
    
    /* TODO: Shouldn't we add TPNOBLOCK?? */
    if (EXSUCCEED!=(ret=ndrx_generic_q_send(svc_q, (char *)call, len, 
            /* call->flags ??*/TPNOBLOCK, 0)))
    {
        NDRX_LOG(log_error, "Failed to send message to ndrxd!");
        br_process_error((char *)call, len, ret, NULL, PACK_TYPE_TOSVC, svc_q);
    }
    
    /* TODO: Check the result, if called failed, then reply back with error? */
    
out:
    return EXSUCCEED;    
}


/**
 * Submit reply to service. We should do this via Q for ATMI command
 * @param call
 * @param len
 * @param from_q
 * @return 
 */
expublic int br_submit_reply_to_q(tp_command_call_t *call, int len)
{
    char reply_to[NDRX_MAX_Q_SIZE+1];
    int ret=EXSUCCEED;
    
    /* TODO??: We have problem here, because of missing reply_to */
    if (EXSUCCEED!=fill_reply_queue(call->callstack, call->reply_to, reply_to))
    {
        NDRX_LOG(log_error, "Failed to send message to ndrxd!");
        goto out;
    }
    
    NDRX_LOG(log_debug, "Reply to Q: %s", reply_to);
    if (EXSUCCEED!=(ret=ndrx_generic_q_send(reply_to, (char *)call, len, TPNOBLOCK, 0)))
    {
        NDRX_LOG(log_error, "Failed to send message to %s!", reply_to);
        br_process_error((char *)call, len, ret, NULL, PACK_TYPE_TORPLYQ, reply_to);
        goto out;
    }
    
out:
    return EXSUCCEED;    
}

/**
 * Thread entry wrapper...
 * Sending message to network
 * @param buf
 * @param len
 * @param msg_type
 * @return 
 */
expublic int br_got_message_from_q(char *buf, int len, char msg_type)
{
    int ret = EXSUCCEED;
    xatmi_brmessage_t *thread_data;
    int finish_off = EXFALSE;
    char *fn = "br_got_message_from_q";
    
    if (0==G_bridge_cfg.threadpoolsize)
    {
        xatmi_brmessage_t thread_data_stat;
        
        NDRX_LOG(log_debug, "%s: single thread mode", fn);
        thread_data_stat.threaded=EXFALSE;
        thread_data_stat.buf = buf;
        thread_data_stat.len = len;
        thread_data_stat.msg_type = msg_type;
        
        return br_got_message_from_q_th((void *)&thread_data_stat, &finish_off);  /* <<<< RETURN!!!! */
    }
    
    NDRX_LOG(log_debug, "%s: threaded mode - dispatching to worker", fn);
    
    thread_data = NDRX_MALLOC(sizeof(xatmi_brmessage_t));

    if (NULL==thread_data)
    {
        int err = errno;
        NDRX_LOG(log_error, "Failed to allocate xatmi_brmessage_t: %s", 
                strerror(err));
        
        userlog("Failed to allocate xatmi_brmessage_t: %s", 
                strerror(err));
        EXFAIL_OUT(ret);
    }
    
    thread_data->buf = ndrx_memdup(buf, len);
    thread_data->threaded=EXTRUE;
    thread_data->len = len;
    thread_data->msg_type = msg_type;
    
    if (EXSUCCEED!=thpool_add_work(G_bridge_cfg.thpool_tonet, 
            (void*)br_got_message_from_q_th, 
            (void *)thread_data))
    {
        EXFAIL_OUT(ret);
    }
    
out:
            
    if (EXSUCCEED!=ret)
    {
        if (NULL!=thread_data)
        {
            if (NULL!=thread_data->buf)
            {
                NDRX_FREE(thread_data->buf);
            }
            NDRX_FREE(thread_data);
        }
    }
    return ret;
}


/**
 * At this point we got message from Q, so we should forward it to network.
 * Q basically is XATMI sub-system
 * Thread processing
 * @param buf
 * @param len
 * @param msg_type
 * @return 
 */
exprivate int br_got_message_from_q_th(void *ptr, int *p_finish_off)
{
    int ret=EXSUCCEED;
    /* Get threaded data */
    xatmi_brmessage_t *p_xatmimsg = (xatmi_brmessage_t *)ptr;
    char *buf = p_xatmimsg->buf;
    int len = p_xatmimsg->len;
    char msg_type = p_xatmimsg->msg_type;
    
    BR_THREAD_ENTRY;
    
    NDRX_DUMP(log_debug, "Got message from Q:", buf, len);
    
    if (msg_type==BR_NET_CALL_MSG_TYPE_ATMI)
    {
        tp_command_generic_t *gen_command = (tp_command_generic_t *)buf;
        NDRX_LOG(log_debug, "Got from Q ATMI message: %d", 
                gen_command->command_id);
        
        switch (gen_command->command_id)
        {

            case ATMI_COMMAND_TPCALL:
            case ATMI_COMMAND_CONNECT:
                
                NDRX_LOG(log_debug, "TPCALL/CONNECT from Q");
                /* Adjust the clock */
                br_clock_adj((tp_command_call_t *)buf, EXTRUE);
                /* Send stuff to network, adjust clock.*/
                ret=br_send_to_net(buf, len, BR_NET_CALL_MSG_TYPE_ATMI, 
                        gen_command->command_id);
                
                if (EXSUCCEED!=ret)
                {
                    /* Generate TPNOENT */
                }
                
                break;
                
            /* tpreply & conversation goes via reply Q */
            case ATMI_COMMAND_TPREPLY:
            case ATMI_COMMAND_CONVDATA:
            case ATMI_COMMAND_CONNRPLY:
            case ATMI_COMMAND_DISCONN:
            case ATMI_COMMAND_CONNUNSOL:
            case ATMI_COMMAND_CONVACK:
            case ATMI_COMMAND_SHUTDOWN:
                
                NDRX_LOG(log_debug, "TPREPLY/CONVERSATION from Q");
                
                /* Adjust the clock */
                br_clock_adj((tp_command_call_t *)buf, EXTRUE);
                
                ret=br_send_to_net(buf, len, BR_NET_CALL_MSG_TYPE_ATMI, 
                        gen_command->command_id);
                
                if (EXSUCCEED!=ret)
                {
                    NDRX_LOG(log_error, "Failed to send reply to "
                            "net - nothing todo");
                    ret=EXSUCCEED;
                }
               
                break;
            case ATMI_COMMAND_TPFORWARD:
                /* not used */
                break;
            case ATMI_COMMAND_SELF_SD:
                G_shutdown_nr_got++;
            
                NDRX_LOG(log_warn, "Got shutdown req %d of %d", 
                        G_shutdown_nr_got, G_shutdown_nr_wait);

                break;
            case  ATMI_COMMAND_BROADCAST:
            case  ATMI_COMMAND_TPNOTIFY:
                
                NDRX_LOG(log_info, "Sending tpnotify/broadcast:");
                
                /* Translate to notification... */
                ret=br_send_to_net(buf, len, BR_NET_CALL_MSG_TYPE_NOTIF, 
                        gen_command->command_id);
                
                if (EXSUCCEED!=ret)
                {
                    NDRX_LOG(log_error, "Failed to send reply to "
                            "net - nothing todo");
                    ret=EXSUCCEED;
                }       
                break;
        }
    }
    else if (msg_type==BR_NET_CALL_MSG_TYPE_NDRXD)
    {
        command_call_t *call = (command_call_t *)buf;
        NDRX_LOG(log_debug, "Got from Q NDRXD message");
        /* I guess we can forward these directly but firstly check the type
         * we do not want to send any spam to other machine, do we?
         * Hmm but lets try out?
         */
        if (EXSUCCEED!=br_send_to_net(buf, len, msg_type, call->command))
        {
            EXFAIL_OUT(ret);
        }
    }
out:
                
    if (p_xatmimsg->threaded)
    {
        NDRX_FREE(p_xatmimsg->buf);
        NDRX_FREE(p_xatmimsg);
    }
    
    return ret;
}
/* vim: set ts=4 sw=4 et smartindent: */
