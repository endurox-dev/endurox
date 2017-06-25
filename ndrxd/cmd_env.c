/* 
** Command `pe', `set', `unset' backend
**
** @file cmd_env.c
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
#include <errno.h>

#include <ndrstandard.h>

#include <ndebug.h>
#include <userlog.h>
#include <ndrxd.h>
#include <ndrxdcmn.h>

#include "cmd_processor.h"
#include <atmi_shm.h>
/*---------------------------Externs------------------------------------*/
extern char **environ;
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
expublic void pe_reply_mod(command_reply_t *reply, size_t *send_size, mod_param_t *params)
{
    command_reply_pe_t * pe_info = (command_reply_pe_t *)reply;
    char *env = (char *)params->mod_param1;
    pm_node_svc_t *elt;
    
    reply->msg_type = NDRXD_CALL_TYPE_PE;
    /* calculate new send size */
    *send_size += (sizeof(command_reply_pe_t) - sizeof(command_reply_t));

    /* Copy data to reply structure */
    strncpy(pe_info->env, env, EX_ENV_MAX);
    pe_info->env[EX_ENV_MAX] = EXEOS;
    
    NDRX_LOG(log_debug, "magic: %ld", pe_info->rply.magic);
}

/**
 * Callback to report startup progress
 * @param call
 * @param pm
 * @return
 */
exprivate void pe_progress(command_call_t * call, char *env)
{
    int ret=EXSUCCEED;
    mod_param_t params;

    NDRX_LOG(log_debug, "startup_progress enter");
    memset(&params, 0, sizeof(mod_param_t));

    /* pass to reply process model node */
    params.mod_param1 = (void *)env;

    if (EXSUCCEED!=simple_command_reply(call, ret, NDRXD_REPLY_HAVE_MORE,
                            /* hook up the reply */
                            &params, pe_reply_mod, 0L, 0, NULL))
    {
        userlog("Failed to send progress back to [%s]", call->reply_queue);
    }

    NDRX_LOG(log_debug, "startup_progress exit");
}

/**
 * Call to pe command
 * @param args
 * @return
 */
expublic int cmd_pe (command_call_t * call, char *data, size_t len, int context)
{
    int ret=EXSUCCEED;
    int i = 1;
    char *s = *environ;
    
    for (; s; i++)
    {
        pe_progress(call, s);
        
        s = *(environ+i);
    }

    if (EXSUCCEED!=simple_command_reply(call, ret, 0L, NULL, NULL, 0L, 0, NULL))
    {
        userlog("Failed to send reply back to [%s]", call->reply_queue);
    }
    NDRX_LOG(log_warn, "cmd_ppm returns with status %d", ret);
    
out:
    return ret;
}

/**
 * Call to set command
 * @param args
 * @return
 */
static int cmd_set_common (command_call_t * call, char *data, size_t len, int context)
{
    int ret=EXSUCCEED;
    command_setenv_t *envcall = (command_setenv_t *)call;
    char *name;
    char *value;
    int err_ret = 0;
    char errmsg[RPLY_ERR_MSG_MAX] = {EXEOS};
    
    name = envcall->env;
    if (NULL==(value=strchr(name, '=')))
    {
        userlog("No = in env value [%s]", envcall->env);
        NDRX_LOG(log_error, "No = in env value [%s]", envcall->env);
        
        sprintf(errmsg, "Invalid argument");
        err_ret = NDRXD_EINVAL;
        
        EXFAIL_OUT(ret);
    }

    *value=EXEOS;
    value++;
    
    NDRX_LOG(log_debug, "Setting env: name: [%s] value: [%s]", envcall->env, value);
    userlog("Setting env: name: [%s] value: [%s]", envcall->env, value);
     
    if (EXEOS==*value)
    {
        NDRX_LOG(log_debug, "Unsetting....");
        if (EXSUCCEED!=unsetenv(name))
        {
            int err = errno;
            err_ret = NDRXD_EENVFAIL;
            
            userlog("unsetenv failed for [%s]: %s", envcall->env, strerror(err));
            NDRX_LOG(log_error, "unsetenv failed for [%s]: %s", envcall->env, strerror(err));
            
            
            sprintf(errmsg, "unsetenv() failed: %.200s", strerror(err));
            
            EXFAIL_OUT(ret);
        }
    }
    else if (EXSUCCEED!=setenv(name, value, 1))
    {
        int err = errno;
        err_ret = NDRXD_EENVFAIL;
        userlog("setenv failed for [%s]: %s", envcall->env, strerror(err));   
        NDRX_LOG(log_error, "setenv failed for [%s]: %s", envcall->env, strerror(err));
        
        sprintf(errmsg, "setenv() failed: %.200s", strerror(err));
        EXFAIL_OUT(ret);
    }
    
out:
    if (EXSUCCEED!=simple_command_reply(call, ret, 0L, NULL, NULL, 0L, err_ret, errmsg))
    {
        userlog("Failed to send reply back to [%s]", call->reply_queue);
    }

    NDRX_LOG(log_warn, "cmd_set returns with status %d", ret);
    
    return EXSUCCEED; /* Do not want to break the system! */
}

/**
 * Call to set command
 * @param args
 * @return
 */
expublic int cmd_set (command_call_t * call, char *data, size_t len, int context)
{
    return cmd_set_common (call, data, len, context);
}

/**
 * Call to unset command
 * @param args
 * @return
 */
expublic int cmd_unset (command_call_t * call, char *data, size_t len, int context)
{
    return cmd_set_common (call, data, len, context);
}

