/**
 * @brief `at' command implementation
 *
 * @file cmd_at.c
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
 * Special resposne processing for at!
 * This will actually will wait for data back from ndrxd!
 * @param reply
 * @param reply_len
 * @return
 */
expublic int at_rsp_process(command_reply_t *reply, size_t reply_len)
{
    return cmd_generic_listcall(reply->userfld1, NDRXD_SRC_ADMIN,
                            NDRXD_CALL_TYPE_GENERIC,
                            NULL, 0,
                            G_config.reply_queue_str,
                            G_config.reply_queue,
                            G_config.ndrxd_q,
                            G_config.ndrxd_q_str,
                            0, NULL,
                            NULL,
                            G_call_args,
                            EXTRUE,
                            G_config.listcall_flags);
}

/**
 * Get service listings
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
expublic int cmd_cat(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret;
    command_call_t call;
    memset(&call, 0, sizeof(call));

    /* Then get listing... */
    ret=cmd_generic_listcall(p_cmd_map->ndrxd_cmd, NDRXD_SRC_ADMIN,
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
    
    return ret;
}
/* vim: set ts=4 sw=4 et smartindent: */
