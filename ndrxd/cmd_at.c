/* 
** Command's `psc' backend
**
** @file cmd_at.c
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
#include <utlist.h>

#include <ndrstandard.h>

#include <ndebug.h>
#include <userlog.h>
#include <ndrxd.h>
#include <ndrxdcmn.h>

#include "cmd_processor.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
/**
 * Call to at command
 * @param args
 * @return
 */
expublic int cmd_at (command_call_t * call, char *data, size_t len, int context)
{
    int ret=EXSUCCEED;
    long cmd_in_progress;

    if (NULL!=G_last_interactive_call)
    {
        /* make attachemnt */
        NDRX_LOG(log_debug, "Attaching ndrxd, from: [%s] to: [%s]",
                G_last_interactive_call->reply_queue, call->reply_queue);
        strcpy(G_last_interactive_call->reply_queue, call->reply_queue);

        cmd_in_progress=G_last_interactive_call->command;
        /* Remove deadly flag from calst call...! */
        G_last_interactive_call->flags&=~NDRXD_CALL_FLAGS_DEADQ;
    }
    else
    {
        ret=EXFAIL;
        NDRXD_set_error(NDRXD_ENONICONTEXT);
    }

    if (EXSUCCEED!=simple_command_reply(call, ret, 0L, NULL, NULL, cmd_in_progress,
            0, NULL))
    {
        userlog("Failed to send reply back to [%s]", call->reply_queue);
    }

    NDRX_LOG(log_warn, "cmd_config_load returns with status %d", ret);

    return EXSUCCEED; /* Do not want to break the system! */
}

