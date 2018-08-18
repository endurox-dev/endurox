/**
 * @brief Reload configuration call
 *
 * @file cmd_reload.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or Mavimax's license for commercial use.
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include <ndrstandard.h>

#include "ndebug.h"
#include "userlog.h"
#include "cmd_processor.h"
#include <ndrxd.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Modify reply according the data.
 * @param call
 * @param pm
 */
expublic void reload_reply_mod(command_reply_t *reply, size_t *send_size, mod_param_t *params)
{
    command_reply_reload_t * err_info = (command_reply_reload_t *)reply;
   
    reply->msg_type = NDRXD_CALL_TYPE_PM_RELERR;
    /* calculate new send size */
    *send_size += (sizeof(command_reply_reload_t) - sizeof(command_reply_t));

    if (NULL!=params->mod_param1)
        strcpy(err_info->old_binary, params->mod_param1);

    if (NULL!=params->mod_param3)
        strcpy(err_info->new_binary, params->mod_param3);

    err_info->error = params->param2;
    err_info->srvid = (int)params->param4;

    NDRX_LOG(log_debug, "magic: %ld", err_info->rply.magic);
}

/**
 * Callback to report startup progress
 * @param call
 * @param pm
 * @return
 */
expublic void reload_error(command_call_t * call, int srvid, char *old_bin, char *new_bin, int error)
{
    int ret=EXSUCCEED;
    mod_param_t params;

    NDRX_LOG(log_debug, "reload_error enter");
    memset(&params, 0, sizeof(mod_param_t));

    /* pass to reply process model node */
    params.mod_param1 = old_bin;
    params.param2 = error;
    params.mod_param3 = new_bin;
    params.param4 = srvid;

    if (EXSUCCEED!=simple_command_reply(call, ret, NDRXD_REPLY_HAVE_MORE,
                            /* hook up the reply */
                            &params, reload_reply_mod, 0L, 0, NULL))
    {
        userlog("Failed to send progress back to [%s]", call->reply_queue);
    }

    NDRX_LOG(log_debug, "reload_error exit");
}

/**
 * Reload configuration
 * @param args
 * @return
 */
expublic int cmd_testcfg (command_call_t * call, char *data, size_t len, int context)
{
    int ret=EXSUCCEED;

    ret = test_config(EXFALSE, call, reload_error);

    if (EXSUCCEED!=simple_command_reply(call, ret, 0L, NULL, NULL, 0L, 0, NULL))
    {
        userlog("Failed to send reply back to [%s]", call->reply_queue);
    }

    NDRX_LOG(log_warn, "cmd_testcfg returns with status %d", ret);

    return EXSUCCEED; /* Do not want to break the system! */
}


/**
 * Reload configuration
 * @param args
 * @return 
 */
expublic int cmd_reload (command_call_t * call, char *data, size_t len, int context)
{
    int ret=EXSUCCEED;

    ret = test_config(EXTRUE, call, reload_error);

    if (EXSUCCEED!=simple_command_reply(call, ret, 0L, NULL, NULL, 0L, 0, NULL))
    {
        userlog("Failed to send reply back to [%s]", call->reply_queue);
    }

    NDRX_LOG(log_warn, "cmd_reload returns with status %d", ret);
    
    return EXSUCCEED; /* Do not want to break the system! */
}
/* vim: set ts=4 sw=4 et smartindent: */
