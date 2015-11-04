/* 
** Contains network processing part of the bridge.
**
** @file network.c
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
#include <ndrxdcmn.h>

#include "bridge.h"
#include "../libatmisrv/srv_int.h"
#include "gpgme_encrypt.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
int M_is_gpg_init = FALSE;
pgpgme_enc_t M_enc;
/*---------------------------Prototypes---------------------------------*/

/**
 * Logging function for encryption
 */
void og_callback(int lev, char *msg)
{
	NDRX_LOG(lev, "%s", msg);
}

/**
 * Set the signer (either when we send, or we receive!)
 */
static int br_set_signer(char *name)
{
	int ret=SUCCEED;
	/* Set the signer if PGP encryption is enabled. */	
	if (0!=G_bridge_cfg.gpg_signer[0])
	{
		NDRX_LOG(log_debug, "Setting GPG signer to [%s]",
						name);
	
		if (SUCCEED!=pgpa_set_signer(&M_enc, name))
		{
			NDRX_LOG(log_always, "GPG set signer fail: "
						    "apierr=%d gpg_meerr=%d: %s", 
				gpga_aerrno(), gpga_gerrno(), 
				gpga_strerr(gpga_aerrno(), gpga_gerrno()));
		
			userlog("GPG set signer (%s) fail: apierr=%d gpg_meerr=%d: %s ", 
				name,
				gpga_aerrno(), gpga_gerrno(), 
				gpga_strerr(gpga_aerrno(), gpga_gerrno()));
		
			ret=FAIL;
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
	int ret=SUCCEED;
	
	if (SUCCEED!=pgpa_init(&M_enc,og_callback, 
			/* use signing if signer set! */
			(0==G_bridge_cfg.gpg_signer[0]?FALSE:TRUE)))
	{
		
		NDRX_LOG(log_always, 
			"GPG init fail: apierr=%d gpg_meerr=%d: %s ", 
			gpga_aerrno(), gpga_gerrno(), 
			gpga_strerr(gpga_aerrno(), gpga_gerrno()));
		
		userlog("GPG init fail: apierr=%d gpg_meerr=%d: %s ", 
			gpga_aerrno(), gpga_gerrno(), 
			gpga_strerr(gpga_aerrno(), gpga_gerrno()));
		
		ret=FAIL;
		goto out;
	}

	/* we sing with singing key... */
	if (SUCCEED!=br_set_signer(G_bridge_cfg.gpg_signer))
	{
		ret=FAIL;
		goto out;
	}

	NDRX_LOG(log_debug, "Setting GPG recipient to [%s]",
					G_bridge_cfg.gpg_recipient);
	
	if (SUCCEED!=pgpa_set_recipient(&M_enc, G_bridge_cfg.gpg_recipient))
	{
		NDRX_LOG(log_always, "GPG set recipient fail: "
					    "apierr=%d gpg_meerr=%d: %s", 
			gpga_aerrno(), gpga_gerrno(), 
			gpga_strerr(gpga_aerrno(), gpga_gerrno()));
		
		userlog("GPG set recipient (%s) fail: apierr=%d gpg_meerr=%d: %s ", 
			G_bridge_cfg.gpg_recipient,
			gpga_aerrno(), gpga_gerrno(), 
			gpga_strerr(gpga_aerrno(), gpga_gerrno()));
		
		ret=FAIL;
		goto out;
	}
	
	
out:
	return ret;
}

/**
 * Bridge have received message.
 * Got message from Network.
 * @param net
 * @param buf
 * @param len
 * @return 
 */
public int br_process_msg(exnetcon_t *net, char *buf, int len)
{
    int ret=SUCCEED;
    char tmp[ATMI_MSG_MAX_SIZE];
    /* Also we could thing something better! which does not eat so much stack*/
    char tmp_clr[ATMI_MSG_MAX_SIZE];
    cmd_br_net_call_t *call = (cmd_br_net_call_t *)buf;
    
    
    /* Decrypt the mssage, if decryption is used... */
    /* use GPG encryption */
    if (EOS!=G_bridge_cfg.gpg_recipient[0])
    {
	int clr_len = ATMI_MSG_MAX_SIZE;
	if (!M_is_gpg_init)
	{
		if (SUCCEED==br_init_gpg())
		{
			M_is_gpg_init = TRUE;
			NDRX_LOG(log_error, "GPG init OK");
		}
		else
		{
			NDRX_LOG(log_error, "GPG init fail");
			FAIL_OUT(ret);
		}
	}
	
	/* Encrypt the message */
	if (SUCCEED!=pgpa_decrypt(&M_enc, buf, len, 
				tmp_clr, &clr_len))
	{
		NDRX_LOG(log_always, "GPG msg decryption failed: "
					    "apierr=%d gpg_meerr=%d: %s", 
			gpga_aerrno(), gpga_gerrno(), 
			gpga_strerr(gpga_aerrno(), gpga_gerrno()));
		
		userlog("GPG decryption failed: apierr=%d gpg_meerr=%d: %s ", 
			gpga_aerrno(), gpga_gerrno(), 
			gpga_strerr(gpga_aerrno(), gpga_gerrno()));
		FAIL_OUT(ret);
	}
	else
	{
		NDRX_LOG(log_debug, "Msg Decrypt OK, len: %d", clr_len);
	}
	
	buf = tmp_clr;
	call = (cmd_br_net_call_t *)tmp_clr;
	len = clr_len;
    }
    
    if (G_bridge_cfg.common_format)
    {
        long tmp_len = len;
        NDRX_LOG(log_debug, "Convert message from network...");
        /* do some optimisation: memset(tmp, 0, sizeof(tmp)); */
        
        if (SUCCEED!=exproto_proto2ex(buf, tmp_len,  tmp, &tmp_len))
        {
            NDRX_LOG(log_error, "Failed to convert incoming message!");
            ret=FAIL;
            goto out;
        }
        
        /* Switch ptr to converted one.! */
        call = (cmd_br_net_call_t *)tmp;
        /* Should ignore len field...! */
        call->len = tmp_len - OFFSET(cmd_br_net_call_t, buf);
        
        /*
        NDRX_LOG(log_debug, "Got c len=%ld bytes (br refresh %d) - internal %ld", 
                tmp_len, sizeof(bridge_refresh_t), call->len);
        */
        NDRX_DUMP(log_debug, "Got converted packet: ", call->buf, call->len);
    }
    
    NDRX_LOG(log_debug, "Got message from net.");
    
    
    if (BR_NET_CALL_MAGIC!=call->br_magic)
    {
        NDRX_LOG(log_error, "Got bridge message, but invalid magic: got"
                " %p, expected: %p", call->br_magic, BR_NET_CALL_MAGIC);
        goto out;
    }
    
    if (BR_NET_CALL_MSG_TYPE_ATMI==call->msg_type)
    {
        tp_command_generic_t *gen_command = (tp_command_generic_t *)call->buf;
        
        /* Have some more debug out there: */
        {
            tp_command_call_t *extra_debug = (tp_command_call_t *)call->buf;
            NDRX_LOG(log_debug, "timer = (%ld %ld) %d", 
                    extra_debug->timer.t.tv_sec,
                    extra_debug->timer.t.tv_nsec,
                    n_timer_get_delta_sec(&extra_debug->timer) );

            NDRX_LOG(log_debug, "callseq  %hd",   extra_debug->callseq);
            NDRX_LOG(log_debug, "cd       %d",    extra_debug->cd);
            NDRX_LOG(log_debug, "my_id    [%s]",  extra_debug->my_id);
            NDRX_LOG(log_debug, "reply_to [%s]",  extra_debug->reply_to);
            NDRX_LOG(log_debug, "name     [%s]",  extra_debug->name);
        }
        
        NDRX_LOG(log_debug, "ATMI message, command id=%d", 
                gen_command->command_id);
        
        switch (gen_command->command_id)
        {
            
            case ATMI_COMMAND_TPCALL:
            case ATMI_COMMAND_EVPOST:
            case ATMI_COMMAND_CONNECT:
                NDRX_LOG(log_debug, "tpcall or connect");
                /* If this is a call, then we should append caller address */
                if (SUCCEED!=br_tpcall_pushstack((tp_command_call_t *)gen_command))
                {
                    FAIL_OUT(ret);
                }
                /* Call service */
                NDRX_LOG(log_debug, "About to call service...");
                ret=br_submit_to_service((tp_command_call_t *)gen_command, call->len, NULL);
                break;
             
            /* tpreply & conversation goes via reply Q */
            case ATMI_COMMAND_TPREPLY:
            case ATMI_COMMAND_CONVDATA:
            case ATMI_COMMAND_CONNRPLY:
            case ATMI_COMMAND_DISCONN:
            case ATMI_COMMAND_CONNUNSOL:
            case ATMI_COMMAND_CONVACK:
            case ATMI_COMMAND_SHUTDOWN:
                /* TODO: So this is reply... we should pop the stack and decide 
                 * where to send the message, either to service replyQ
                 * or other node 
                 */
                NDRX_LOG(log_debug, "Reply back to caller/bridge");
                ret = br_submit_reply_to_q((tp_command_call_t *)gen_command, call->len, NULL);
                break;
            case ATMI_COMMAND_TPFORWARD:
                /* not used */
                break;
#if 0
            case ATMI_COMMAND_CONVDATA:
                
                break;
            case ATMI_COMMAND_CONNRPLY:
                
                break;
            case ATMI_COMMAND_DISCONN:
                
                break;
            case ATMI_COMMAND_CONNUNSOL:
                
                break;
            case ATMI_COMMAND_CONVACK:
                
                break;
            case ATMI_COMMAND_SHUTDOWN:
                
                break;
#endif
        }
    }
    else if (BR_NET_CALL_MSG_TYPE_NDRXD==call->msg_type)
    {
        command_call_t *icall = (command_call_t *)call->buf;
        int call_len = call->len;
        
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
    return ret;
}



/**
 * Send message to other bridge.
 * Might want to use async call, as there Net stack could be full & blocked.
 * @param buf
 * @param len
 * @return 
 */
public int br_send_to_net(char *buf, int len, char msg_type, int command_id)
{
    int ret=SUCCEED;
    char *fn = "br_send_to_net";
    
    char tmp[ATMI_MSG_MAX_SIZE];
    char tmp2[ATMI_MSG_MAX_SIZE];
    char tmp_enc[ATMI_MSG_MAX_SIZE]; /* Not the best way, but we atleas we are clear... */
    char *snd;
    long snd_len;
    
    cmd_br_net_call_t *call = (cmd_br_net_call_t *)tmp;
    NDRX_LOG(log_debug, "%s: sending %d bytes", fn, len);
    
    if (NULL==G_bridge_cfg.con)
    {
        NDRX_LOG(log_error, "Bridge is not connected!!!");
        FAIL_OUT(ret);
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
        
        if (SUCCEED!=exproto_ex2proto((char *)call, snd_len, tmp2, &snd_len))
        {
            ret=FAIL;
            goto out;
        }    
    }
    
    /* use GPG encryption */
    if (EOS!=G_bridge_cfg.gpg_recipient[0])
    {
	int enc_len = ATMI_MSG_MAX_SIZE;
	if (!M_is_gpg_init)
	{
		if (SUCCEED==br_init_gpg())
		{
			M_is_gpg_init = TRUE;
			NDRX_LOG(log_error, "GPG init OK");
		}
		else
		{
			NDRX_LOG(log_error, "GPG init fail");
			FAIL_OUT(ret);
		}
	}
	
	/* Encrypt the message */
	if (SUCCEED!=pgpa_encrypt(&M_enc, snd, snd_len, 
				tmp_enc, &enc_len))
	{
		NDRX_LOG(log_always, "GPG msg encryption failed: "
					    "apierr=%d gpg_meerr=%d: %s", 
			gpga_aerrno(), gpga_gerrno(), 
			gpga_strerr(gpga_aerrno(), gpga_gerrno()));
		
		userlog("GPG encryption failed: apierr=%d gpg_meerr=%d: %s ", 
			gpga_aerrno(), gpga_gerrno(), 
			gpga_strerr(gpga_aerrno(), gpga_gerrno()));
		FAIL_OUT(ret);
	}
	else
	{
		NDRX_LOG(log_debug, "Msg Encryption OK, "
				 "len: %d", enc_len);
	}
	
	snd = tmp_enc;
	snd_len = enc_len;
    }
    
    /* Might want to move this stuff to Q */
    if (SUCCEED!=exnet_send_sync(G_bridge_cfg.con, (char *)snd, snd_len, 0, 0))
    {
        NDRX_LOG(log_error, "Failed to submit message to network");
        FAIL_OUT(ret);
    }
    
out:
    return ret;
}
