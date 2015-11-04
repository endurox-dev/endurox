/* 
** Un-advertise related command back-end
**
** @file cmd_unadv.c
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

#include <ndrstandard.h>

#include "ndebug.h"
#include "userlog.h"
#include <ndrxd.h>
#include <ndrxdcmn.h>
#include <atmi.h>
#include <cmd_processor.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/


/**
 * Unadvertise service, call from xadmin.
 * @param args
 * @return 
 */
public int cmd_xadunadv (command_call_t * call, char *data, size_t len, int context)
{
    int ret=SUCCEED;
    command_dynadvertise_t *unadv_xa = (command_dynadvertise_t *)call;
    command_dynadvertise_t call_srv;
    pm_node_t * p_pm;
    
    memset(&call_srv, 0, sizeof(call_srv));
    
    if (NULL==(p_pm = get_pm_from_srvid(unadv_xa->srvid)))
    {
        NDRXD_set_error_fmt(NDRXD_EINVPARAM, "Invalid server id %d",
                                    unadv_xa->srvid);
        ret=FAIL;
        goto out;
    }
    
    /*Fill some details for unadvertise*/
    strcpy(call_srv.svc_nm, unadv_xa->svc_nm);
    
    /* Call the server */
    ret = cmd_generic_call(NDRXD_COM_NXDUNADV_RQ, NDRXD_SRC_ADMIN,
            NDRXD_CALL_TYPE_GENERIC,
            &call_srv, sizeof(call_srv),
            G_command_state.listenq_str,
            G_command_state.listenq,
            FAIL,
            get_srv_admin_q(p_pm),
            0, NULL,
            NULL,
            NULL,
            NULL,
            FALSE);
out:
    if (SUCCEED!=simple_command_reply(call, ret, 0L, NULL, NULL, 0L))
    {
        userlog("Failed to send reply back to [%s]", call->reply_queue);
    }

    NDRX_LOG(log_warn, "cmd_unadv_xadmin returns with status %d", ret);
    
    return SUCCEED; /* Do not want to break the system! */
}

/**
 * Unadvertise service, requested by server - remove from service array
 * and remove from shm.
 * @param call
 * @param data
 * @param len
 * @param context
 * @return  
 */
public int cmd_srvunadv (command_call_t * call, char *data, size_t len, int context)
{
    int ret=SUCCEED;
    command_dynadvertise_t * unadv = (command_dynadvertise_t *)call;
    pm_node_t *p_pm = get_pm_from_srvid(unadv->srvid);
    
    if (NULL==p_pm)
    {
        NDRX_LOG(log_error, "No such server with id: %d", unadv->srvid);
    }
    else
    {
        NDRX_LOG(log_error, "Server id=%d ok, binary: [%s], removing service: [%s]", 
                unadv->srvid, p_pm->binary_name, unadv->svc_nm);
        remove_startfail_process(p_pm, unadv->svc_nm, NULL);
    }
    
out:
    return ret;
}
