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
#include <assert.h>

#include <exnet.h>
#include <ndrxdcmn.h>
#include "bridge.h"
#include "../libatmisrv/srv_int.h"
#include "userlog.h"
#include "cgreen/assertions.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

exprivate int br_got_message_from_q_th(void *ptr, int *p_finish_off);

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
        br_process_error((char *)call, len, ret, NULL, PACK_TYPE_TONDRXD, qstr, 
                NULL);
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
        br_process_error((char *)call, len, ret, NULL, PACK_TYPE_TOSVC, NULL, NULL);
        EXFAIL_OUT(ret);
    }

    
    NDRX_LOG(log_debug, "Calling service: %s", svc_q);
    if (EXSUCCEED!=(ret=ndrx_generic_q_send(svc_q, (char *)call, len, TPNOBLOCK, 0)))
    {
        NDRX_LOG(log_error, "Failed to send message to local ATMI service!");
        br_process_error((char *)call, len, ret, NULL, PACK_TYPE_TOSVC, svc_q, NULL);
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
        
        br_process_error((char *)call, len, ret, NULL, PACK_TYPE_TOSVC, NULL, NULL);
        
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_debug, "Calling broadcast server: %s with flags %ld", 
            svc_q, call->flags);
    
    /* TODO: Shouldn't we add TPNOBLOCK?? */
    if (EXSUCCEED!=(ret=ndrx_generic_q_send(svc_q, (char *)call, len, 
            /* call->flags ??*/TPNOBLOCK, 0)))
    {
        NDRX_LOG(log_error, "Failed to send message to ndrxd!");
        br_process_error((char *)call, len, ret, NULL, PACK_TYPE_TOSVC, svc_q, NULL);
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
        br_process_error((char *)call, len, ret, NULL, PACK_TYPE_TORPLYQ, reply_to, NULL);
        goto out;
    }
    
out:
    return EXSUCCEED;    
}

/**
 * Thread entry wrapper...
 * Sending message to network
 * @param buf double ptr to dispatch msg
 * @param len
 * @param msg_type
 * @return 
 */
expublic int br_got_message_from_q(char **buf, int len, char msg_type)
{
    int ret = EXSUCCEED;
    xatmi_brmessage_t *thread_data;
    char *fn = "br_got_message_from_q";
    
    NDRX_LOG(log_debug, "%s: threaded mode - dispatching to worker", fn);
    
    thread_data = NDRX_FPMALLOC(sizeof(xatmi_brmessage_t), 0);

    if (NULL==thread_data)
    {
        int err = errno;
        NDRX_LOG(log_error, "Failed to allocate xatmi_brmessage_t: %s", 
                strerror(err));
        
        userlog("Failed to allocate xatmi_brmessage_t: %s", 
                strerror(err));
        EXFAIL_OUT(ret);
    }
    
    thread_data->buf = *buf;
    *buf = NULL;
    thread_data->len = len;
    thread_data->msg_type = msg_type;
    
    /* limit the sending rate... to avoid internal buffers... */
    if (EXSUCCEED!=ndrx_thpool_add_work2(G_bridge_cfg.thpool_tonet, 
            (void*)br_got_message_from_q_th, 
            (void *)thread_data, 0, G_bridge_cfg.threadpoolbufsz))
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

            /* maybe move to non-threaded env...
             * as threads will be stopped by threadpool stop
             */
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
        
        /* request for clock infos */
        if (NDRXD_COM_BRCONINFO_RQ==call->command)
        {
            NDRX_LOG(log_debug, "Clock infos request");
            
            if (EXSUCCEED!=br_coninfo(call))
            {
                EXFAIL_OUT(ret);
            }
        }
        else
        {
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
    }
out:
                
    NDRX_SYSBUF_FREE(p_xatmimsg->buf);
    NDRX_FPFREE(p_xatmimsg);
    
    return ret;
}
/* vim: set ts=4 sw=4 et smartindent: */
