/**
 * @brief Command's `appconfig' backend
 *   Allow dynamically change ndrxconfig's appconfig section values
 *
 * @file cmd_appconfig.c
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
#include <utlist.h>

#include <ndrstandard.h>

#include <ndebug.h>
#include <userlog.h>
#include <ndrxd.h>
#include <ndrxdcmn.h>

#include "cmd_processor.h"
#include <atmi_shm.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define OFSZ(s,e)   EXOFFSET(s,e), EXELEM_SIZE(s,e)
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/


#define NDRX_ARGS_BOOL                  1     /**< boolean type               */
#define NDRX_ARGS_INT                   2     /**< integer type               */
#define NDRX_ARGS_STRING                3     /**< string type                */

/**
 * Config field mappings
 */
static ndrx_args_loader_t M_appcfg_map[] = 
{
    {OFSZ(config_t,sanity),         "sanity",           NDRX_ARGS_INT, 0, 0, 0, INT_MAX},
    {OFSZ(config_t,checkpm),        "checkpm",          NDRX_ARGS_INT, 0, 0, 0, INT_MAX},
    {OFSZ(config_t,brrefresh),      "brrefresh",        NDRX_ARGS_INT, 0, 0, 0, INT_MAX},
    {OFSZ(config_t,restart_min),    "restart_min",      NDRX_ARGS_INT, 0, 0, 0, INT_MAX},
    {OFSZ(config_t,restart_step),   "restart_step",     NDRX_ARGS_INT, 0, 0, 0, INT_MAX},
    {OFSZ(config_t,restart_max),    "restart_max",      NDRX_ARGS_INT, 0, 0, 0, INT_MAX},
    {OFSZ(config_t,restart_to_check),"restart_to_check",NDRX_ARGS_INT, 0, 0, 0, INT_MAX},
    {OFSZ(config_t,gather_pq_stats), "gather_pq_stats", NDRX_ARGS_BOOL, 0, 0, 0, 0},
    {OFSZ(config_t,rqaddrttl),      "rqaddrttl",        NDRX_ARGS_INT, 0, 0, 0, INT_MAX},
    {EXFAIL}
};

/*---------------------------Prototypes---------------------------------*/

/**
 * Modify reply according the data.
 * @param call
 * @param pm
 */
expublic void appconfig_reply_mod(command_reply_t *reply, size_t *send_size, 
        mod_param_t *params)
{
    command_reply_appconfig_t * rply = (command_reply_appconfig_t *)reply;
    command_appconfig_t *req = (command_appconfig_t *)params->mod_param1;
    
    reply->msg_type = NDRXD_CALL_TYPE_APPCONFIG;
    /* calculate new send size */
    *send_size += (sizeof(command_reply_appconfig_t) - sizeof(command_reply_t));
    
    /* fill up the data */
    NDRX_STRCPY_SAFE(rply->setting, req->setting);
    
    /* setting value is buffer in request structure... */
    NDRX_STRCPY_SAFE(rply->svalue, req->svalue);
    
    NDRX_LOG(log_debug, "magic: %ld", rply->rply.magic);
}


/**
 * Call to appconfig command
 * @param args
 * @return
 */
expublic int cmd_appconfig (command_call_t * call, char *data, size_t len, 
        int context)
{
    int ret=EXSUCCEED;
    command_appconfig_t *cfg = (command_appconfig_t *)call;
    mod_param_t params;
    /*
     * Re-use admin error library
    char errbuf[RPLY_ERR_MSG_MAX] = {EXEOS};
    int error_code = NDRXD_EMINVAL;
     * */
    char *errbuf;
    int *error_code;
    int errbufsz;
    
    memset(&params, 0, sizeof(mod_param_t));
    params.mod_param1 = (void *)call;
    
    errbuf =  NDRXD_error_res_get(&error_code, &errbufsz);
    
    /* process argument mapping 
     * if value is empty is empty, then it is get
     * if value is not empty, then it it set
     */
    /* test is config loaded? */
    if (NULL==G_app_config)
    {
        NDRXD_set_error_msg(NDRXD_ECFGLDED, "App config not loaded - idle instance");
    }
    else if (EXEOS!=cfg->svalue[0])
    {
        if (EXSUCCEED!=ndrx_args_loader_set(M_appcfg_map, G_app_config, 
            cfg->setting, cfg->svalue, errbuf, errbufsz))
        {
            *error_code = NDRXD_EINVAL;
            NDRX_LOG(log_error, "Failed to apply [%s] setting", cfg->setting);
        }
    }
    else
    {
        if (EXSUCCEED!=ndrx_args_loader_get(M_appcfg_map, G_app_config, 
            cfg->setting, cfg->svalue, sizeof(cfg->svalue), errbuf, errbufsz))
        {
            *error_code = NDRXD_EINVAL;
            NDRX_LOG(log_error, "Failed to get [%s] setting", cfg->setting);
        }
    }

    if (EXSUCCEED!=simple_command_reply(call, ret, 0L, &params, appconfig_reply_mod, 
            0L, 0, NULL))
    {
        userlog("Failed to send reply back to [%s]", call->reply_queue);
    }
    
out:
    NDRX_LOG(log_warn, "cmd_appconfig returns with status %d", ret);
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
