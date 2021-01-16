/**
 * @brief Contains network processing part of the bridge.
 *
 * @file network.c
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
#include <typed_buf.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <Exfields.h>
#include <userlog.h>

#include <exnet.h>
#include <exproto.h>
#include <ndrxdcmn.h>

#include "bridge.h"
#include "../libatmisrv/srv_int.h"

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
exprivate int br_process_msg_th(void *ptr, int *p_finish_off);

/**
 * Process message from network (wrapper) for dispatching to thread.
 * @param net
 * @param buf NOTE! we reset the incoming to NULL, as we forward data to thread
 * @param len
 * @return 
 */
expublic int br_process_msg(exnetcon_t *net, char **buf, int len)
{
    int ret = EXSUCCEED;
    net_brmessage_t *thread_data;
    char *fn = "br_process_msg";
    
    thread_data = NDRX_FPMALLOC(sizeof(net_brmessage_t), 0);
    
    if (NULL==thread_data)
    {
        int err = errno;
        NDRX_LOG(log_error, "Failed to allocate net_brmessage_t: %s", 
                strerror(err));
        
        userlog("Failed to allocate net_brmessage_t: %s", 
                strerror(err));
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_debug, "%s: multi thread mode - dispatching to worker", fn);
    
    thread_data->buf = *buf;
    *buf = NULL;/* indicate the we finish off the data buffer */
    
    thread_data->len = len;
    thread_data->net = net;
    
    if (EXSUCCEED!=ndrx_thpool_add_work2(G_bridge_cfg.thpool_fromnet, (void*)br_process_msg_th, 
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
                NDRX_SYSBUF_FREE(thread_data->buf);
            }
            NDRX_FPFREE(thread_data);
        }
    }

    return ret;
}

/**
 * Dump the tpcall
 * @param buf message received from net
 */
exprivate void br_dump_tp_command_call(char *buf)
{
    tp_command_call_t *extra_debug = (tp_command_call_t *)buf;
    /* Have some more debug out there: */

    NDRX_LOG(log_debug, "timer = (%ld %ld) %d", 
            extra_debug->timer.t.tv_sec,
            extra_debug->timer.t.tv_nsec,
            ndrx_stopwatch_get_delta_sec(&extra_debug->timer) );

    NDRX_LOG(log_debug, "callseq  %hu",   extra_debug->callseq);
    NDRX_LOG(log_debug, "msgseq   %hu",   extra_debug->msgseq);
    NDRX_LOG(log_debug, "cd       %d",    extra_debug->cd);
    NDRX_LOG(log_debug, "my_id    [%s]",  extra_debug->my_id);
    NDRX_LOG(log_debug, "reply_to [%s]",  extra_debug->reply_to);
    NDRX_LOG(log_debug, "name     [%s]",  extra_debug->name);
}

/**
 * Dump the tpcall
 * @param buf message received from net
 */
exprivate void br_dump_tp_notif_call(char *buf)
{
    tp_notif_call_t *extra_debug = (tp_notif_call_t *)buf;
    /* Have some more debug out there: */

    NDRX_LOG(log_debug, "timer = (%ld %ld) %d", 
            extra_debug->timer.t.tv_sec,
            extra_debug->timer.t.tv_nsec,
            ndrx_stopwatch_get_delta_sec(&extra_debug->timer) );

    NDRX_LOG(log_debug, "callseq          %hu",   extra_debug->callseq);
    NDRX_LOG(log_debug, "msgseq           %hu",   extra_debug->msgseq);
    NDRX_LOG(log_debug, "cd               %d",    extra_debug->cd);
    NDRX_LOG(log_debug, "my_id            [%s]",  extra_debug->my_id);
    NDRX_LOG(log_debug, "reply_to         [%s]",  extra_debug->reply_to);
    NDRX_LOG(log_debug, "destclient       [%s]",  extra_debug->destclient);
    NDRX_LOG(log_debug, "cltname          [%s]",  extra_debug->cltname);
    NDRX_LOG(log_debug, "cltname_isnull   [%d]",  extra_debug->cltname_isnull);
    NDRX_LOG(log_debug, "nodeid           [%s]",  extra_debug->nodeid);
    NDRX_LOG(log_debug, "nodeid_isnull    [%d]",  extra_debug->nodeid_isnull);
    NDRX_LOG(log_debug, "usrname          [%s]",  extra_debug->usrname);
    NDRX_LOG(log_debug, "usrname_isnull   [%d]",  extra_debug->usrname_isnull);
}

/**
 * Bridge have received message.
 * Got message from Network.
 * But we have a problem here, as multiple threads are doing receive, there
 * is possibility that conversational is also received in out of order...
 * and submitting to specific thread pool will not solve the issue.
 * 
 * The other option is to do ack on every conversational message.
 * 
 * Or do message sequencing and reordering at the client end (i.e. consume
 * the out of order messages in the process memory, sort the messages by sequence
 * number, when message is received or we are going to look for the message, then
 * check the queue in memory. If we have something there and looks like correct
 * message sequence number, then use it.
 * 
 * The queue will be dumped after the conversation is closed.
 * 
 * @param net
 * @param buf
 * @param len
 * @return 
 */
exprivate int br_process_msg_th(void *ptr, int *p_finish_off)
{
    int ret=EXSUCCEED;
    char *tmp=NULL;
    /* Also we could thing something better! which does not eat so much stack*/
    char *tmp_clr=NULL;
    net_brmessage_t *p_netmsg = (net_brmessage_t *)ptr;
    
    p_netmsg->call = (cmd_br_net_call_t *)p_netmsg->buf;
    
    
    if (G_bridge_cfg.common_format)
    {
        size_t tmp_buf_len;
        long tmp_len = p_netmsg->len;
        
        NDRX_SYSBUF_MALLOC_OUT(tmp, tmp_buf_len, ret);
        
        NDRX_LOG(log_debug, "Convert message from network... (tmp buf = %p, size: %ld)", 
                tmp, NDRX_MSGSIZEMAX);
        
        if (EXFAIL==exproto_proto2ex(p_netmsg->buf, tmp_len,  tmp, &tmp_len, 
                NDRX_MSGSIZEMAX))
        {
            NDRX_LOG(log_error, "Failed to convert incoming message!");
            EXFAIL_OUT(ret);
        }
        
        /* If allocated previously  */
        NDRX_SYSBUF_FREE(p_netmsg->buf);
        
        p_netmsg->buf = tmp;
        p_netmsg->len = tmp_len;
        
        /* Switch ptr to converted one.! */
        p_netmsg->call = (cmd_br_net_call_t *)tmp;
        /* Should ignore len field...! */
        p_netmsg->call->len = tmp_len - EXOFFSET(cmd_br_net_call_t, buf);
        
        /*
        NDRX_LOG(log_debug, "Got c len=%ld bytes (br refresh %d) - internal %ld", 
                tmp_len, sizeof(bridge_refresh_t), call->len);
        */
        NDRX_DUMP(log_debug, "Got converted packet: ", p_netmsg->call->buf, 
                p_netmsg->call->len);
    }
    
    NDRX_LOG(log_debug, "Got message from net.");
    
    
    if (BR_NET_CALL_MAGIC!=p_netmsg->call->br_magic)
    {
        NDRX_LOG(log_error, "Got bridge message, but invalid magic: got"
                " %p, expected: %p", p_netmsg->call->br_magic, BR_NET_CALL_MAGIC);
        userlog("Got bridge message, but invalid magic: got"
                " %p, expected: %p", p_netmsg->call->br_magic, BR_NET_CALL_MAGIC);
        goto out;
    }
    
    if (BR_NET_CALL_MSG_TYPE_ATMI==p_netmsg->call->msg_type ||
            BR_NET_CALL_MSG_TYPE_NOTIF==p_netmsg->call->msg_type)
    {
        tp_command_generic_t *gen_command = (tp_command_generic_t *)p_netmsg->call->buf;

        NDRX_LOG(log_debug, "ATMI message, command id=%d", 
                gen_command->command_id);
        
        switch (gen_command->command_id)
        {
            
            case ATMI_COMMAND_TPCALL:
            case ATMI_COMMAND_CONNECT:
                NDRX_LOG(log_debug, "tpcall or connect");
                br_dump_tp_command_call(p_netmsg->call->buf);
                /* If this is a call, then we should append caller address */
                if (EXSUCCEED!=br_tpcall_pushstack((tp_command_call_t *)gen_command))
                {
                    EXFAIL_OUT(ret);
                }
                /* Call service */
                NDRX_LOG(log_debug, "About to call service...");
                ret=br_submit_to_service((tp_command_call_t *)gen_command, 
                        p_netmsg->call->len);
                break;
             
            /* tpreply & conversation goes via reply Q */
            case ATMI_COMMAND_TPREPLY:
            case ATMI_COMMAND_CONVDATA:
            case ATMI_COMMAND_CONNRPLY:
            case ATMI_COMMAND_DISCONN:
            case ATMI_COMMAND_CONNUNSOL:
            case ATMI_COMMAND_CONVACK:
            case ATMI_COMMAND_SHUTDOWN:
                br_dump_tp_command_call(p_netmsg->call->buf);
                /* TODO: So this is reply... we should pop the stack and decide 
                 * where to send the message, either to service replyQ
                 * or other node 
                 */
                NDRX_LOG(log_debug, "Reply back to caller/bridge");
                ret = br_submit_reply_to_q((tp_command_call_t *)gen_command, 
                        p_netmsg->call->len);
                break;
            case ATMI_COMMAND_TPFORWARD:
                br_dump_tp_command_call(p_netmsg->call->buf);
                /* not used */
                break;
            case ATMI_COMMAND_BROADCAST:
            case ATMI_COMMAND_TPNOTIFY:
            {
                tp_notif_call_t * p_notif = (tp_notif_call_t *)gen_command;
                /* Call the reply Q
                 * If this is broadcast, then we send it to broadcast server
                 * If this is notification, then send to client proc only.
                 */
                NDRX_LOG(log_debug, "Sending tpnotify to client queue... "
                        "(flags got: %ld, regex: %d)",
                        p_notif->flags, p_notif->flags & TPREGEXMATCH);
                br_dump_tp_notif_call(p_netmsg->call->buf);
                
/*
                ret = br_submit_reply_to_q_notif((tp_notif_call_t *)gen_command, 
                        p_netmsg->call->len, NULL);
  */            
                ret = br_submit_to_service_notif((tp_notif_call_t *)gen_command, 
                        p_netmsg->call->len);
                
            }   
                break;
        }
    }
    else if (BR_NET_CALL_MSG_TYPE_NDRXD==p_netmsg->call->msg_type)
    {
        command_call_t *icall = (command_call_t *)p_netmsg->call->buf;
        int call_len = p_netmsg->call->len;
        
        /* TODO: Might want to check the buffers sizes to minimum */
        NDRX_LOG(log_debug, "NDRX message, call_len=%d", call_len);
        
        switch (icall->command)
        {
            case NDRXD_COM_BRCLOCK_RQ:
                ret = br_calc_clock_diff(icall);
                break;
            case NDRXD_COM_BRREFERSH_RQ:
                ret = br_submit_to_ndrxd(icall, call_len);
                break;
            default:
                NDRX_LOG(log_debug, "Unsupported bridge command: %d",
                            icall->command);
                break;
        }
         
    }
out:
              
    if (NULL!=p_netmsg->buf)
    {
        NDRX_SYSBUF_FREE(p_netmsg->buf);
    }

    NDRX_FPFREE(p_netmsg);

    return ret;
}

/**
 * Send message to other bridge.
 * Might want to use async call, as there Net stack could be full & blocked.
 * @param buf
 * @param len
 * @return 
 */
expublic int br_send_to_net(char *buf, int len, char msg_type, int command_id)
{
    int ret=EXSUCCEED;
    char *fn = "br_send_to_net";
    char *tmp2 = NULL;
    size_t tmp2_len;
    char **snd;
    long snd_len;
    int use_hdr = EXFALSE;
    char smallbuf[sizeof(cmd_br_net_call_t) + sizeof(char *)];
    cmd_br_net_call_t *call = (cmd_br_net_call_t *)smallbuf;
    
    NDRX_LOG(log_debug, "%s: sending %d bytes", fn, len);
    
    if (NULL==G_bridge_cfg.con)
    {
        NDRX_LOG(log_error, "Bridge is not connected!!!");
        EXFAIL_OUT(ret);
    }
    
    /*do some optimisation memset(tmp, 0, sizeof(tmp)); */
    call->br_magic = BR_NET_CALL_MAGIC;
    call->msg_type = msg_type;
    call->command_id = command_id;
    call->len = len;
    
    if (G_bridge_cfg.common_format)
    {
        /* get away from this memcpy somehow? 
         * Enduro/X 8.0 - no mem copy anymore!
         */
        memcpy(call->buf, &buf, sizeof(char *));
            
        NDRX_LOG(log_debug, "Convert message to network...");
        /* do some more optimization: memset(tmp2, 0, sizeof(tmp2)); */
        NDRX_SYSBUF_MALLOC_OUT(tmp2, tmp2_len, ret);
        
        snd = &tmp2;
        snd_len = 0;
        
        /* Set the output buffer size border. */
        if (EXSUCCEED!=exproto_ex2proto((char *)call, len + sizeof(*call), tmp2, 
                &snd_len, tmp2_len))
        {
            ret=EXFAIL;
            goto out;
        }
        
    }
    else
    {
        /* faster route, no copy */
        use_hdr=EXTRUE;
    }
    
    
    /* Might want to move this stuff to Q */
    
    /* the connection object is created by main thread
     * and calls are dispatched by main thread too. Thus 
     * existence of con must be atomic.
     *  */
    if (NULL!=G_bridge_cfg.con)
    {
        /* Lock to network */
        exnet_rwlock_read(G_bridge_cfg.con);
                
        if (exnet_is_connected(G_bridge_cfg.con))
        {
            if (use_hdr)
            {
                if (EXSUCCEED!=exnet_send_sync(G_bridge_cfg.con, (char *)call, 
                        sizeof(cmd_br_net_call_t), (char *)buf, len, 0, 0))
                {
                    NDRX_LOG(log_error, "Failed to submit message to network");
                    ret=EXFAIL;
                }
            }
            else
            {
                /* slower, prev memcopy */
                if (EXSUCCEED!=exnet_send_sync(G_bridge_cfg.con, NULL, 0,
                        (char *)*snd, snd_len, 0, 0))
                {
                    NDRX_LOG(log_error, "Failed to submit message to network");
                    ret=EXFAIL;
                }
            }
        }
        else
        {
            NDRX_LOG(log_error, "Node disconnected - cannot send");
            ret=EXFAIL;
        }
        
        /* unlock the network */
        exnet_rwlock_unlock(G_bridge_cfg.con); 
        
        if (EXSUCCEED!=ret)
        {
            goto out;
        }
        
    }
    else
    {
        NDRX_LOG(log_error, "Node disconnected - cannot send");
        EXFAIL_OUT(ret);
    }
    
out:
                
    if (NULL!=tmp2)
    {
        NDRX_SYSBUF_FREE(tmp2);
    }

    return ret;
}
/* vim: set ts=4 sw=4 et smartindent: */
