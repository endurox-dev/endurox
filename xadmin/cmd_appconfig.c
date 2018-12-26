/**
 * @brief NDRXD active application configuration dynamic change
 *
 * @file cmd_appconfig.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * AGPL or Mavimax's license for commercial use.
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
#include <unistd.h>

#include <ndrstandard.h>
#include <ndebug.h>

#include <ndrx.h>
#include <ndrxdcmn.h>
#include <atmi_int.h>
#include <gencall.h>
#include <nclopt.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Response for application config (value of current setting)
 * @param reply
 * @param reply_len
 * @return EXSUCCEED/error code
 */
expublic int appconfig_rsp_process(command_reply_t *reply, size_t reply_len)
{
    command_reply_appconfig_t * cfg = (command_reply_appconfig_t*)reply;
    
    if (cfg->rply.error_code)
    {
        fprintf(stderr, XADMIN_ERROR_FORMAT_PFX "%s\n", cfg->rply.error_msg);
        return cfg->rply.error_code;
    }
    else
    {
        fprintf(stdout, "%s\n", cfg->svalue);
        return EXSUCCEED;
    }
}

/**
 * Change ndrxd appconfig on the fly (dynamically without reloading ndrxconfig.xml)
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
expublic int cmd_appconfig(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret=EXSUCCEED;
    command_appconfig_t call;
    
    NDRX_STRCPY_SAFE(call.setting, argv[1]);
    
    if (argc==3)
    {
        NDRX_STRCPY_SAFE(call.svalue, argv[2]);
    }
    else
    {
        call.svalue[0] = EXEOS;
    }
    
    if (EXSUCCEED!=cmd_generic_listcall(p_cmd_map->ndrxd_cmd, NDRXD_SRC_ADMIN,
                        NDRXD_CALL_TYPE_GENERIC,
                        (command_call_t *)&call, sizeof(call),
                        G_config.reply_queue_str,
                        G_config.reply_queue,
                        G_config.ndrxd_q,
                        G_config.ndrxd_q_str,
                        argc, argv,
                        p_have_next,
                        G_call_args,
                        EXFALSE,
                        G_config.listcall_flags))
    {
        EXFAIL_OUT(ret);
    }
            
out:
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
