/**
 * @brief Command's `shm_psrv' backend
 *
 * @file cmd_shm_psrv.c
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
#include <utlist.h>

#include <ndrstandard.h>

#include <ndebug.h>
#include <userlog.h>
#include <ndrxd.h>
#include <ndrxdcmn.h>

#include "cmd_processor.h"
#include <atmi_shm.h>
/*---------------------------Externs------------------------------------*/
extern ndrx_shm_t G_srvinfo;
extern int G_max_servers;
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
expublic void shm_psrv_reply_mod(command_reply_t *reply, size_t *send_size, mod_param_t *params)
{
    command_reply_shm_psrv_t * shm_psrv_info = (command_reply_shm_psrv_t *)reply;
    shm_srvinfo_t *p_shm = (shm_srvinfo_t *)params->mod_param1;
    pm_node_svc_t *elt;
    
    reply->msg_type = NDRXD_CALL_TYPE_PM_SHM_PSRV;
    /* calculate new send size */
    *send_size += (sizeof(command_reply_shm_psrv_t) - sizeof(command_reply_t));
    
    /* Copy data to reply structure */
    shm_psrv_info->last_call_flags = p_shm->last_call_flags;
    shm_psrv_info->last_command_id = p_shm->last_command_id;
    strcpy(shm_psrv_info->last_reply_q, p_shm->last_reply_q);
    shm_psrv_info->slot = params->param2;
    shm_psrv_info->srvid =p_shm->srvid; 
    shm_psrv_info->status = p_shm->status;

    NDRX_LOG(log_debug, "magic: %ld", shm_psrv_info->rply.magic);
}

/**
 * Callback to report startup progress
 * @param call
 * @param pm
 * @return
 */
exprivate void shm_psrv_progress(command_call_t * call, shm_srvinfo_t *p_shm, int slot)
{
    int ret=EXSUCCEED;
    mod_param_t params;
    
    NDRX_LOG(log_debug, "startup_progress enter");
    memset(&params, 0, sizeof(mod_param_t));

    /* pass to reply process model node */
    params.mod_param1 = (void *)p_shm;
    params.param2 = slot;

    if (EXSUCCEED!=simple_command_reply(call, ret, NDRXD_REPLY_HAVE_MORE,
                            /* hook up the reply */
                            &params, shm_psrv_reply_mod, 0L, 0, NULL))
    {
        userlog("Failed to send progress back to [%s]", call->reply_queue);
    }

    NDRX_LOG(log_debug, "startup_progress exit");
}

/**
 * Call to psc command
 * @param args
 * @return
 */
expublic int cmd_shm_psrv (command_call_t * call, char *data, size_t len, int context)
{
    int ret=EXSUCCEED;
    int i;
    shm_srvinfo_t *srvinfo = (shm_srvinfo_t *) G_srvinfo.mem;
    
    /* We assume shm is OK! */
    for (i=0; i<G_max_servers; i++)
    {
        if (srvinfo[i].srvid)
        {
            shm_psrv_progress(call, &srvinfo[i], i);
        }
    }

    if (EXSUCCEED!=simple_command_reply(call, ret, 0L, NULL, NULL, 0L, 0, NULL))
    {
        userlog("Failed to send reply back to [%s]", call->reply_queue);
    }
    NDRX_LOG(log_warn, "cmd_shm_psrv returns with status %d", ret);
    
out:
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
