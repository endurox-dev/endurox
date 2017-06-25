/* 
** Command's `psc' backend
**
** @file cmd_psc.c
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
#include <exhash.h>

#include <ndrstandard.h>

#include <ndebug.h>
#include <userlog.h>
#include <ndrxd.h>
#include <ndrxdcmn.h>

#include "cmd_processor.h"
#include "bridge_int.h"
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
expublic void psc_reply_mod(command_reply_t *reply, size_t *send_size, mod_param_t *params)
{
    command_reply_psc_t * psc_info = (command_reply_psc_t *)reply;
    pm_node_t *p_pm = (pm_node_t *)params->mod_param1;
    pm_node_svc_t *elt;
    /* we must be attached to shm! */
    shm_srvinfo_t * srv_shm = ndrxd_shm_getsrv(p_pm->srvid);
    int svc=0;
    /* nothing to do! */
    if (NULL==srv_shm)
    {
        userlog("No shm record for srvid %d!", p_pm->srvid);
        return;
    }
    
    reply->msg_type = NDRXD_CALL_TYPE_SVCINFO;
    /* calculate new send size */
    *send_size += (sizeof(command_reply_psc_t) - sizeof(command_reply_t));

    strcpy(psc_info->binary_name, p_pm->binary_name);
    psc_info->srvid = p_pm->srvid;
    psc_info->state = p_pm->state;
    psc_info->nodeid = ndrx_get_G_atmi_env()->our_nodeid;
    /* Now build up stats? */
    
    /* Prepare service details... */
    DL_FOREACH(p_pm->svcs,elt)
    {
        psc_info->svcdet[svc].done=srv_shm->svc_succeed[svc];
        psc_info->svcdet[svc].fail=srv_shm->svc_fail[svc];
        psc_info->svcdet[svc].max=srv_shm->max_rsp_msec[svc];
        psc_info->svcdet[svc].min=srv_shm->min_rsp_msec[svc];
        psc_info->svcdet[svc].last=srv_shm->last_rsp_msec[svc];
        psc_info->svcdet[svc].status=srv_shm->svc_status[svc];
        strcpy(psc_info->svcdet[svc].svc_nm, elt->svc.svc_nm);
        strcpy(psc_info->svcdet[svc].fn_nm, elt->svc.fn_nm);
        svc++;
    }
    psc_info->svc_count = svc;

    /* additional size of services! */
    *send_size += psc_info->svc_count * sizeof(command_reply_psc_det_t);

    NDRX_LOG(log_debug, "magic: %ld", psc_info->rply.magic);
}


/**
 * Modify reply according the data (for bridge)
 * @param call
 * @param pm
 */
expublic void psc_reply_mod_br(command_reply_t *reply, size_t *send_size, mod_param_t *params)
{
    command_reply_psc_t * psc_info = (command_reply_psc_t *)reply;
    
    bridgedef_t *br = (bridgedef_t *)params->mod_param1;
    /*Service listing*/
    bridgedef_svcs_t *rs = (bridgedef_svcs_t *)params->mod_param3;
    int i;
    int svc=0;
    
    reply->msg_type = NDRXD_CALL_TYPE_SVCINFO;
    /* calculate new send size */
    *send_size += (sizeof(command_reply_psc_t) - sizeof(command_reply_t));

    strcpy(psc_info->binary_name, "N/A");
    psc_info->srvid = EXFAIL;
    psc_info->state = EXFAIL;
    psc_info->nodeid = br->nodeid;
    /* Bridge svc id: */
    psc_info->srvid = br->srvid;
    /* Now build up stats? */
    
    /* Prepare service details... */
    for (i=0; i<rs->count; i++)
    {
        psc_info->svcdet[svc].done=EXFAIL;
        psc_info->svcdet[svc].fail=EXFAIL;
        psc_info->svcdet[svc].max=EXFAIL;
        psc_info->svcdet[svc].min=EXFAIL;
        psc_info->svcdet[svc].last=EXFAIL;
        psc_info->svcdet[svc].status=0;
        strcpy(psc_info->svcdet[svc].svc_nm, rs->svc_nm);
        strcpy(psc_info->svcdet[svc].fn_nm, "N/A");
        svc++;
    }
    psc_info->svc_count = svc;
    
    /* additional size of services! */
    *send_size += psc_info->svc_count * sizeof(command_reply_psc_det_t);
    
    NDRX_LOG(log_debug, "magic: %ld", psc_info->rply.magic);
}

/**
 * Callback to report startup progress
 * @param call
 * @param pm
 * @return
 */
exprivate void psc_progress(command_call_t * call, pm_node_t *pm)
{
    int ret=EXSUCCEED;
    mod_param_t params;

    NDRX_LOG(log_debug, "startup_progress enter");
    memset(&params, 0, sizeof(mod_param_t));

    /* pass to reply process model node */
    params.mod_param1 = (void *)pm;

    if (EXSUCCEED!=simple_command_reply(call, ret, NDRXD_REPLY_HAVE_MORE,
                            /* hook up the reply */
                            &params, psc_reply_mod, 0L, 0, NULL))
    {
        userlog("Failed to send progress back to [%s]", call->reply_queue);
    }

    NDRX_LOG(log_debug, "startup_progress exit");
}

/**
 * Process bridge service
 * @param call
 * @param pm
 */
exprivate void psc_progress_br(command_call_t * call, bridgedef_t *br, bridgedef_svcs_t *brs)
{
    int ret=EXSUCCEED;
    mod_param_t params;

    NDRX_LOG(log_debug, "startup_progress enter");
    memset(&params, 0, sizeof(mod_param_t));

    /* pass to reply process model node */
    params.mod_param1 = br;
    params.mod_param3 = brs;

    if (EXSUCCEED!=simple_command_reply(call, ret, NDRXD_REPLY_HAVE_MORE,
                            /* hook up the reply */
                            &params, psc_reply_mod_br, 0L, 0, NULL))
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
expublic int cmd_psc (command_call_t * call, char *data, size_t len, int context)
{
    int ret=EXSUCCEED;
    pm_node_t *pm;
    /*Bridge listing*/
    bridgedef_t *br = NULL;
    bridgedef_t *brt = NULL;
    /*Service listing*/
    bridgedef_svcs_t *brs;
    bridgedef_svcs_t *brst;
    
    /* list all services from all servers, right? */
    DL_FOREACH(G_process_model, pm)
    {
        if (NDRXD_PM_RUNNING_OK==pm->state)
            psc_progress(call, pm);
    }
    
    /* Process bridges? */
    EXHASH_ITER(hh, G_bridge_hash, br, brt)
    {
        EXHASH_ITER(hh, br->theyr_services, brs, brst)
        {
            psc_progress_br(call, br, brs);
        }
    }
    
    if (EXSUCCEED!=simple_command_reply(call, ret, 0L, NULL, NULL, 0L, 0, NULL))
    {
        userlog("Failed to send reply back to [%s]", call->reply_queue);
    }
    NDRX_LOG(log_warn, "cmd_psc returns with status %d", ret);
    
out:
    return ret;
}

