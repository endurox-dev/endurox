/**
 * @brief Command's `shm_psvc' backend
 *
 * @file cmd_shm_psvc.c
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
#include <ndrx_config.h>
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
expublic void shm_psvc_reply_mod(command_reply_t *reply, size_t *send_size, mod_param_t *params)
{
    command_reply_shm_psvc_t * shm_psvc_info = (command_reply_shm_psvc_t *)reply;
    shm_svcinfo_t *p_shm = (shm_svcinfo_t *)params->mod_param1;
    int cnt;
    
    reply->msg_type = NDRXD_CALL_TYPE_PM_SHM_PSVC;
    /* calculate new send size */
    *send_size += (sizeof(command_reply_shm_psvc_t) - sizeof(command_reply_t));

    /* Copy data to reply structure */
    NDRX_STRCPY_SAFE(shm_psvc_info->service, p_shm->service);
    shm_psvc_info->flags = p_shm->flags;
    shm_psvc_info->slot = params->param2;
    shm_psvc_info->srvs = p_shm->srvs;
    /* cluster fields: */
    shm_psvc_info->csrvs = p_shm->csrvs;
    shm_psvc_info->totclustered = p_shm->totclustered;
    shm_psvc_info->cnodes_max_id = p_shm->cnodes_max_id;
    memcpy(shm_psvc_info->cnodes, p_shm->cnodes, sizeof(p_shm->cnodes));
    
    shm_psvc_info->resids[0] = 0;

#if defined(EX_USE_POLL) || defined(EX_USE_SYSVQ)
    /* copy the number of elements */
    cnt = shm_psvc_info->srvs - shm_psvc_info->csrvs;
    if (cnt > CONF_NDRX_MAX_SRVIDS_XADMIN)
    {
        cnt = CONF_NDRX_MAX_SRVIDS_XADMIN;
    }
    memcpy(shm_psvc_info->resids, p_shm->resids, cnt*sizeof(int));
#endif
    
    /*
    for (i=0; i< CONF_NDRX_NODEID_COUNT; i++)
    {
        NDRX_LOG(log_debug, "cnodes: %d=>%d", i, p_shm->cnodes[i].srvs);
        NDRX_LOG(log_debug, "shm_ps: %d=>%d", i, shm_psvc_info->cnodes[i].srvs);
    }
    */
    
    NDRX_LOG(log_debug, "magic: %ld", shm_psvc_info->rply.magic);
}

/**
 * Callback to report startup progress
 * @param call
 * @param pm
 * @return
 */
exprivate void shm_psvc_progress(command_call_t * call, shm_svcinfo_t *p_shm, int slot)
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
                            &params, shm_psvc_reply_mod, 0L, 0, NULL))
    {
        userlog("Failed to send progress back to [%s]", call->reply_queue);
    }
    

    NDRX_LOG(log_debug, "startup_progress exit");
}

/**
 * Call to psc command - print services
 * @param args
 * @return
 */
expublic int cmd_shm_psvc (command_call_t * call, char *data, size_t len, int context)
{
    int ret=EXSUCCEED;
    int i;
    shm_svcinfo_t *svcinfo = (shm_svcinfo_t *) G_svcinfo.mem;
    
    /* We assume shm is OK! */
    for (i=0; i<G_max_svcs; i++)
    {
        /* have some test on servs count so that we avoid any core dumps
         *  for un-init memory access of service string due to race conditions
         */
        if (SHM_SVCINFO_INDEX(svcinfo, i)->srvs>0 && 
                EXEOS!=SHM_SVCINFO_INDEX(svcinfo, i)->service[0])
        {
            shm_psvc_progress(call, SHM_SVCINFO_INDEX(svcinfo, i), i);
        }
    }

    if (EXSUCCEED!=simple_command_reply(call, ret, 0L, NULL, NULL, 0L, 0, NULL))
    {
        userlog("Failed to send reply back to [%s]", call->reply_queue);
    }
    NDRX_LOG(log_warn, "cmd_shm_psvc returns with status %d", ret);
    
out:
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
