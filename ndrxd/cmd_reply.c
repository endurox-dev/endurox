/* 
** Common reply handler on requests
**
** @file cmd_reply.c
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <ndrxd.h>
#include <ndrxdcmn.h>

#include <cmd_processor.h>
#include <atmi_int.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define     REPLY_DEAD_TIMEOUT          3   /* Allow 3 sec for Q to process, otherwise assume it is dead! */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/*
 * Generic reply provider
 */
expublic int simple_command_reply(command_call_t * call,
                        int status, long flags,
                        mod_param_t *mod_params,
                        void (*p_mod)(command_reply_t *reply, size_t *send_size,
                                        mod_param_t *mod_params),
                        long userfld1, int error_code, char *error_msg)
{
    int ret=EXSUCCEED;
    command_reply_t *reply;
    char reply_buf[ATMI_MSG_MAX_SIZE];
    size_t send_size = sizeof(command_reply_t);

    memset(reply_buf, 0, sizeof(reply_buf));
    reply = (command_reply_t *)reply_buf;
    
    if (call->flags & NDRXD_CALL_FLAGS_DEADQ)
    {
        NDRX_LOG(log_error, "Reply queue already dead - no reply back!");
        return EXFAIL;
    }

    /* form up the reply */
    reply->magic = NDRX_MAGIC;
    reply->command = call->command+1; /* Make reponse */
    NDRX_LOG(log_debug, "Reply command: %d", reply->command);
    reply->status = status;
    reply->msg_type = NDRXD_CALL_TYPE_GENERIC;
    reply->msg_src = NDRXD_SRC_NDRXD; /* from NDRXD */
    reply->flags = flags;             /* Response flags */
    reply->userfld1 = userfld1;       /* pass back user field... */
    reply->error_code = error_code;
    
    if (NULL!=error_msg)
    {
        strcpy(reply->error_msg, error_msg);
    }

    /* If error is set, then load the message */
    if (NDRXD_is_error())
    {
        /* put error details in response */
        reply->error_code = ndrxd_errno;
        /* Append with error message */
        NDRX_STRNCPY(reply->error_msg, ndrxd_strerror(reply->error_code), RPLY_ERR_MSG_MAX-1);
        reply->error_msg[RPLY_ERR_MSG_MAX-1] = EXEOS;
    }

    /* apply modifications? */
    if (NULL!=p_mod)
        p_mod(reply, &send_size, mod_params);
    
    /* Do reply with ATMI helper function */
    ret = ndrx_generic_q_send_2(call->reply_queue, 
            (char *)reply, send_size, 0, REPLY_DEAD_TIMEOUT, 0);
    
    if (EXSUCCEED!=ret)
    {
        NDRX_LOG(log_error, "Marking reply queue as dead!");
        call->flags|=NDRXD_CALL_FLAGS_DEADQ;
        ret=EXFAIL;
    }
    
    return ret;
}




