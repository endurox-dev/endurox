/* 
** Contains network processing part of the bridge.
**
** @file network.c
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

/* gpg-me - optional */
#ifndef DISABLEGPGME
#include "gpgme_encrypt.h"
#endif
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
int M_is_gpg_init = EXFALSE;

#ifndef DISABLEGPGME
pgpgme_enc_t M_enc;
#endif
/*---------------------------Prototypes---------------------------------*/
exprivate int br_process_msg_th(void *ptr, int *p_finish_off);

/**
 * Logging function for encryption
 */
void og_callback(int lev, char *msg)
{
    NDRX_LOG(lev, "%s", msg);
}

#ifndef DISABLEGPGME
/**
 * Set the signer (either when we send, or we receive!)
 */
static int br_set_signer(char *name)
{
    int ret=EXSUCCEED;
    /* Set the signer if PGP encryption is enabled. */	
    if (0!=G_bridge_cfg.gpg_signer[0])
    {
        NDRX_LOG(log_debug, "Setting GPG signer to [%s]",
                                        name);

        if (EXSUCCEED!=pgpa_set_signer(&M_enc, name))
        {
            NDRX_LOG(log_always, "GPG set signer fail: "
                                        "apierr=%d gpg_meerr=%d: %s", 
                    gpga_aerrno(), gpga_gerrno(), 
                    gpga_strerr(gpga_aerrno(), gpga_gerrno()));

            userlog("GPG set signer (%s) fail: apierr=%d gpg_meerr=%d: %s ", 
                    name,
                    gpga_aerrno(), gpga_gerrno(), 
                    gpga_strerr(gpga_aerrno(), gpga_gerrno()));

            ret=EXFAIL;
            goto out;
        }
    }
out:
    return ret;
}
/**
 * Initialize GPG library 
 */
static int br_init_gpg(void)
{
	int ret=EXSUCCEED;
	
	if (EXSUCCEED!=pgpa_init(&M_enc,og_callback, 
			/* use signing if signer set! */
			(0==G_bridge_cfg.gpg_signer[0]?EXFALSE:EXTRUE)))
	{
		
            NDRX_LOG(log_always, 
                    "GPG init fail: apierr=%d gpg_meerr=%d: %s ", 
                    gpga_aerrno(), gpga_gerrno(), 
                    gpga_strerr(gpga_aerrno(), gpga_gerrno()));

            userlog("GPG init fail: apierr=%d gpg_meerr=%d: %s ", 
                    gpga_aerrno(), gpga_gerrno(), 
                    gpga_strerr(gpga_aerrno(), gpga_gerrno()));

            ret=EXFAIL;
            goto out;
	}

	/* we sing with singing key... */
	if (EXSUCCEED!=br_set_signer(G_bridge_cfg.gpg_signer))
	{
            ret=EXFAIL;
            goto out;
	}

	NDRX_LOG(log_debug, "Setting GPG recipient to [%s]",
					G_bridge_cfg.gpg_recipient);
	
	if (EXSUCCEED!=pgpa_set_recipient(&M_enc, G_bridge_cfg.gpg_recipient))
	{
            NDRX_LOG(log_always, "GPG set recipient fail: "
                                        "apierr=%d gpg_meerr=%d: %s", 
                    gpga_aerrno(), gpga_gerrno(), 
                    gpga_strerr(gpga_aerrno(), gpga_gerrno()));

            userlog("GPG set recipient (%s) fail: apierr=%d gpg_meerr=%d: %s ", 
                    G_bridge_cfg.gpg_recipient,
                    gpga_aerrno(), gpga_gerrno(), 
                    gpga_strerr(gpga_aerrno(), gpga_gerrno()));

            ret=EXFAIL;
            goto out;
	}
	
	
out:
	return ret;
}

#endif /* ifndef DISABLEGPGME */


/**
 * Process message from network (wrapper) for dispatching to thread.
 * @param net
 * @param buf
 * @param len
 * @return 
 */
expublic int br_process_msg(exnetcon_t *net, char *buf, int len)
{
    int ret = EXSUCCEED;
    net_brmessage_t *thread_data;
    char *fn = "br_process_msg";
    
    thread_data = NDRX_MALLOC(sizeof(net_brmessage_t));
    
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
    
    thread_data->buf_malloced = EXTRUE;
    thread_data->buf = ndrx_memdup(buf, len);
    
    thread_data->len = len;
    thread_data->net = net;
    
    if (EXSUCCEED!=thpool_add_work(G_bridge_cfg.thpool, (void*)br_process_msg_th, 
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
    
    if (G_bridge_cfg.threadpoolsize)
    {
        BR_THREAD_ENTRY;
    }
    
#ifndef DISABLEGPGME
    /* Decrypt the mssages, if decryption is used... */
    /* use GPG encryption */
    if (EXEOS!=G_bridge_cfg.gpg_recipient[0])
    {
        int clr_len;
        NDRX_SYSBUF_MALLOC_OUT(tmp_clr, &clr_len, ret);
        
	if (!M_is_gpg_init)
	{
            if (EXSUCCEED==br_init_gpg())
            {
                M_is_gpg_init = EXTRUE;
                NDRX_LOG(log_error, "GPG init OK");
            }
            else
            {
                NDRX_LOG(log_error, "GPG init fail");
                EXFAIL_OUT(ret);
            }
	}
	
	/* Encrypt the message */
	if (EXSUCCEED!=pgpa_decrypt(&M_enc, p_netmsg->buf, p_netmsg->len, 
				tmp_clr, &clr_len))
	{
            NDRX_LOG(log_always, "GPG msg decryption failed: "
                                        "apierr=%d gpg_meerr=%d: %s", 
                    gpga_aerrno(), gpga_gerrno(), 
                    gpga_strerr(gpga_aerrno(), gpga_gerrno()));

            userlog("GPG decryption failed: apierr=%d gpg_meerr=%d: %s ", 
                    gpga_aerrno(), gpga_gerrno(), 
                    gpga_strerr(gpga_aerrno(), gpga_gerrno()));
            EXFAIL_OUT(ret);
	}
	else
	{
            NDRX_LOG(log_debug, "Msg Decrypt OK, len: %d", clr_len);
	}
	
        /* So this was buffer by memdup.. */
        NDRX_FREE(p_netmsg->buf);
        p_netmsg->buf = tmp_clr;

        p_netmsg->len = clr_len;
	p_netmsg->call = (cmd_br_net_call_t *)p_netmsg->buf;
    }
#endif
    
    if (G_bridge_cfg.common_format)
    {
        long tmp_len = p_netmsg->len;
        NDRX_LOG(log_debug, "Convert message from network...");
        
        NDRX_SYSBUF_MALLOC_OUT(tmp, NULL, ret);
        
        if (EXSUCCEED!=exproto_proto2ex(p_netmsg->buf, tmp_len,  tmp, &tmp_len))
        {
            NDRX_LOG(log_error, "Failed to convert incoming message!");
            EXFAIL_OUT(ret);
        }
        
        /* If allocated previously  */
        NDRX_FREE(p_netmsg->buf);
        
        p_netmsg->buf = tmp;
        p_netmsg->len = tmp_len;
        p_netmsg->buf_malloced = EXTRUE;
        
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
            case ATMI_COMMAND_EVPOST:
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
                        p_netmsg->call->len, NULL);
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
                        p_netmsg->call->len, NULL);
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
                        p_netmsg->call->len, NULL);
                
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
                ret = br_submit_to_ndrxd(icall, call_len, NULL);
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
        NDRX_FREE(p_netmsg->buf);
    }

    NDRX_FREE(p_netmsg);

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
    
    char tmp[NDRX_MSGSIZEMAX];
    char tmp2[NDRX_MSGSIZEMAX];
    char tmp_enc[NDRX_MSGSIZEMAX]; /* Not the best way, but we atleas we are clear... */
    char *snd;
    long snd_len;
    
    cmd_br_net_call_t *call = (cmd_br_net_call_t *)tmp;
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
    memcpy(call->buf, buf, len);
    
    snd = tmp;
    snd_len = len+sizeof(cmd_br_net_call_t); /* Is this correct? */
    
    /* use common format */
    if (G_bridge_cfg.common_format)
    {
        NDRX_LOG(log_debug, "Convert message to network...");
        /* do some more optimization: memset(tmp2, 0, sizeof(tmp2)); */
        
        snd = tmp2;
        
        snd_len = 0;
        
        if (EXSUCCEED!=exproto_ex2proto((char *)call, snd_len, tmp2, &snd_len))
        {
            ret=EXFAIL;
            goto out;
        }    
    }
    
#ifndef DISABLEGPGME
    /* use GPG encryption */
    if (EXEOS!=G_bridge_cfg.gpg_recipient[0])
    {
	int enc_len = NDRX_MSGSIZEMAX;
	if (!M_is_gpg_init)
	{
            if (EXSUCCEED==br_init_gpg())
            {
                    M_is_gpg_init = EXTRUE;
                    NDRX_LOG(log_error, "GPG init OK");
            }
            else
            {
                    NDRX_LOG(log_error, "GPG init fail");
                    EXFAIL_OUT(ret);
            }
	}
	
	/* Encrypt the message */
	if (EXSUCCEED!=pgpa_encrypt(&M_enc, snd, snd_len, 
				tmp_enc, &enc_len))
	{
            NDRX_LOG(log_always, "GPG msg encryption failed: "
                                        "apierr=%d gpg_meerr=%d: %s", 
                    gpga_aerrno(), gpga_gerrno(), 
                    gpga_strerr(gpga_aerrno(), gpga_gerrno()));

            userlog("GPG encryption failed: apierr=%d gpg_meerr=%d: %s ", 
                    gpga_aerrno(), gpga_gerrno(), 
                    gpga_strerr(gpga_aerrno(), gpga_gerrno()));
            EXFAIL_OUT(ret);
	}
	else
	{
            NDRX_LOG(log_debug, "Msg Encryption OK, "
                             "len: %d", enc_len);
	}
	
	snd = tmp_enc;
	snd_len = enc_len;
    }
#endif
    
    /* Might want to move this stuff to Q */
    if (EXSUCCEED!=exnet_send_sync(G_bridge_cfg.con, (char *)snd, snd_len, 0, 0))
    {
        NDRX_LOG(log_error, "Failed to submit message to network");
        EXFAIL_OUT(ret);
    }
    
out:
    return ret;
}
