/**
 * @brief ReLoad config (reload) command.
 *
 * @file cmd_reload.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2023, Mavimax, Ltd. All Rights Reserved.
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <sys/param.h>

#include <ndrstandard.h>
#include <ndebug.h>

#include <ndrx.h>
#include <ndrxdcmn.h>
#include <atmi_int.h>
#include <gencall.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/


/**
 * Process response back.
 * @param reply
 * @param reply_len
 * @return
 */
expublic int reload_rsp_process(command_reply_t *reply, size_t reply_len)
{
    if (NDRXD_CALL_TYPE_PM_RELERR==reply->msg_type)
    {
        command_reply_reload_t * err_info = (command_reply_reload_t*)reply;
        if (NDRXD_EREBBINARYRUN==err_info->error)
        {
            printf("Err: srvid: %d. Old binary `%s' must be shutdown to replace it with `%s'!\n",
                    err_info->srvid, err_info->old_binary, err_info->new_binary);
        }
        else if (NDRXD_EBINARYRUN==err_info->error)
        {
            printf("Err: srvid: %d. Old binary `%s' must be shutdown in order to"
                    " remove it!\n",
                    err_info->srvid, err_info->old_binary);
        }
        else
        {
            printf("New conf. error: %d\n", err_info->error);
        }
    }
    else
    {
        printf("OK\n");
    }
    
    return EXSUCCEED;
}

/**
 * ReLoad config, generic call.
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
expublic int cmd_reload(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    command_call_t call;
    memset(&call, 0, sizeof(call));

    return cmd_generic_listcall(p_cmd_map->ndrxd_cmd, NDRXD_SRC_ADMIN,
                        NDRXD_CALL_TYPE_GENERIC,
                        &call, sizeof(call),
                        G_config.reply_queue_str,
                        G_config.reply_queue,
                        G_config.ndrxd_q,
                        G_config.ndrxd_q_str,
                        argc, argv,
                        p_have_next,
                        G_call_args,
                        EXFALSE,
                        G_config.listcall_flags);
}

/**
 * Test new configuration.
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
expublic int cmd_testcfg(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    command_call_t call;
    memset(&call, 0, sizeof(call));

    return cmd_generic_listcall(p_cmd_map->ndrxd_cmd, NDRXD_SRC_ADMIN,
                        NDRXD_CALL_TYPE_GENERIC,
                        &call, sizeof(call),
                        G_config.reply_queue_str,
                        G_config.reply_queue,
                        G_config.ndrxd_q,
                        G_config.ndrxd_q_str,
                        argc, argv,
                        p_have_next,
                        G_call_args,
                        EXFALSE,
                        G_config.listcall_flags);
}
/* vim: set ts=4 sw=4 et smartindent: */
