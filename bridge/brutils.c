/* 
** Common bridge utilites.
**
** @file brutils.c
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

#include <exnet.h>
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

/**
 * Push our node on call stack.
 * @param call
 * @return 
 */
public int br_tpcall_pushstack(tp_command_call_t *call)
{
    int ret=SUCCEED;
    char us = (char)G_bridge_cfg.nodeid;
    int len = strlen(call->callstack);
    
    if (len+1>CONF_NDRX_NODEID_COUNT)
    {
        NDRX_LOG(log_error, "Stack too deep! TODO: Reply with SYSERR back to caller!");
        FAIL_OUT(ret);
    }
    
    /* We are OK, just stack us */
    call->callstack[len] = us;
    call->callstack[len+1] = EOS;
    
    br_dump_nodestack(call->callstack, "Stack after node push");
out:
    return ret;
}

/**
 * We shall detect here if this is conversational message
 * if so, then choose conv pools by one. This is needed to keep the order
 * of the messages, one conv shall go only via one channel on each end
 * otherwise the random order of messages causes handshakes and terminate
 * messages to fly to the caller faster than data, thus corrupting the
 * FIFO like order of the messages.
 * @param msg_type  message type
 * @param buf  message buffer
 * @param p_pool  where to install the number of conv pool to process.
 * @return FAIL if not in conv, otherwise conversational cd
 */
public int br_get_conv_cd(char msg_type, char *buf, int *p_pool)
{
    int conv_cd = FAIL;
    char *fn = "br_get_conv_cd";
     if (BR_NET_CALL_MSG_TYPE_ATMI==msg_type)
    {
        tp_command_generic_t *gen_command = (tp_command_generic_t *)buf;
        
        if (gen_command->command_id == ATMI_COMMAND_TPREPLY)
        {
            tp_command_call_t *call = (tp_command_call_t *)buf;
            
            if (call->sysflags & SYS_CONVERSATION)
            {
                /* Go conversational reply */
                conv_cd = call->cd;
            }
        }
        else
        {
            switch (gen_command->command_id)
            {

                case ATMI_COMMAND_CONNECT:
                case ATMI_COMMAND_CONVDATA:
                case ATMI_COMMAND_CONNRPLY:
                case ATMI_COMMAND_DISCONN:
                case ATMI_COMMAND_CONNUNSOL:
                case ATMI_COMMAND_CONVACK:
                {
                    tp_command_call_t *call = (tp_command_call_t *)buf;
                    
                    conv_cd = call->cd;
                    break;
                }

            }
        }
    }
    
    if (FAIL!=conv_cd)
    {
        /* As usually CD starts with 1, we shall substract the 1  */
        *p_pool = (conv_cd % G_bridge_cfg.cnvnrofpools) - 1;

        if (*p_pool < 0)
        {
            NDRX_LOG(log_error, "! ERROR: Conversational CD =< 0! %d", conv_cd);
            userlog("! ERROR: Conversational CD =< 0! %d", conv_cd);
            FAIL_OUT(conv_cd);
        }
    }
    
out:
    NDRX_LOG(log_info, "br_get_conv_cd: ret %d", conv_cd);
    return conv_cd;
}
