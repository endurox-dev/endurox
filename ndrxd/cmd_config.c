/**
 * @brief Command with prefix config. Supported commands:
 *   1. config.load     - for first time
 *   2. config.reload   - for configuration re-load
 *
 * @file cmd_config.c
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
#include <ndrxd.h>
#include <cmd_processor.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Commad for loading initial configuration.
 * If initial config is set, then just respond with OK.
 * @param args
 * @return 
 */
expublic int cmd_config_load (command_call_t * call, char *data, size_t len, int context)
{
    int ret=EXSUCCEED;

    ret = load_active_config(&G_app_config, &G_process_model,
                &G_process_model_hash, &G_process_model_pid_hash);

    if (EXSUCCEED!=simple_command_reply(call, ret, 0L, NULL, NULL, 0, 0, NULL))
    {
        userlog("Failed to send reply back to [%s]", call->reply_queue);
    }

    NDRX_LOG(log_warn, "cmd_config_load returns with status %d", ret);
    
    return EXSUCCEED; /* Do not want to break the system! */
}

