/* 
** Command's `ppm' backend
**
** @file cmd_ppm.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, ATR Baltic, SIA. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or ATR Baltic's license for commercial use.
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
** A commercial use license is available from ATR Baltic, SIA
** contact@atrbaltic.com
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
#include <atmi_shm.h>
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
public void ppm_reply_mod(command_reply_t *reply, size_t *send_size, mod_param_t *params)
{
    command_reply_ppm_t * ppm_info = (command_reply_ppm_t *)reply;
    pm_node_t *p_pm = (pm_node_t *)params->mod_param1;
    pm_node_svc_t *elt;
    
    reply->msg_type = NDRXD_CALL_TYPE_PM_PPM;
    /* calculate new send size */
    *send_size += (sizeof(command_reply_ppm_t) - sizeof(command_reply_t));

    /* Copy data to reply structure */
    strcpy(ppm_info->binary_name, p_pm->binary_name);
    ppm_info->srvid = p_pm->srvid;
    ppm_info->state = p_pm->state;
    ppm_info->reqstate = p_pm->reqstate;
    ppm_info->autostart = p_pm->autostart;   
    ppm_info->exec_seq_try = p_pm->exec_seq_try;
    ppm_info->last_startup = p_pm->rsptimer;
    ppm_info->num_term_sigs = p_pm->num_term_sigs;
    ppm_info->last_sig = p_pm->last_sig;
    ppm_info->autokill = p_pm->autokill;
    ppm_info->pid = p_pm->pid;
    ppm_info->state_changed = p_pm->state_changed;
    ppm_info->flags = p_pm->flags;
    ppm_info->nodeid = p_pm->nodeid;

    NDRX_LOG(log_debug, "magic: %ld", ppm_info->rply.magic);
}

/**
 * Callback to report startup progress
 * @param call
 * @param pm
 * @return
 */
private void ppm_progress(command_call_t * call, pm_node_t *pm)
{
    int ret=SUCCEED;
    mod_param_t params;

    NDRX_LOG(log_debug, "startup_progress enter");
    memset(&params, 0, sizeof(mod_param_t));

    /* pass to reply process model node */
    params.mod_param1 = (void *)pm;

    if (SUCCEED!=simple_command_reply(call, ret, NDRXD_REPLY_HAVE_MORE,
                            /* hook up the reply */
                            &params, ppm_reply_mod, 0L, 0, NULL))
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
public int cmd_ppm (command_call_t * call, char *data, size_t len, int context)
{
    int ret=SUCCEED;
    pm_node_t *pm;
    
    /* list all services from all servers, right? */
    DL_FOREACH(G_process_model, pm)
    {
            ppm_progress(call, pm);
    }

    if (SUCCEED!=simple_command_reply(call, ret, 0L, NULL, NULL, 0L, 0, NULL))
    {
        userlog("Failed to send reply back to [%s]", call->reply_queue);
    }
    NDRX_LOG(log_warn, "cmd_ppm returns with status %d", ret);
    
out:
    return ret;
}

