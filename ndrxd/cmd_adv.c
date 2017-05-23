/* 
** Advertise related command back-end
** Currently only for SRVADV comand i.e. server requests advertise.
**
** @file cmd_adv.c
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

#include "ndebug.h"
#include "userlog.h"
#include <ndrxd.h>
#include <ndrxdcmn.h>
#include <atmi.h>
#include <cmd_processor.h>
#include <bridge_int.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Advertise service, requested by server.
 * @param call
 * @param data
 * @param len
 * @param context
 * @return  
 */
public int cmd_srvadv (command_call_t * call, char *data, size_t len, int context)
{
    int ret=SUCCEED;
    command_dynadvertise_t * adv = (command_dynadvertise_t *)call;
    pm_node_t *p_pm = get_pm_from_srvid(adv->srvid);
    pm_node_svc_t *chk=NULL;
    pm_node_svc_t *new_svc=NULL;
    int found = FALSE;
    
    if (NULL==p_pm)
    {
        NDRX_LOG(log_error, "No such server with id: %d", adv->srvid);
    }
    else
    {
        NDRX_LOG(log_error, "Server id=%d ok, binary: [%s], adding service: [%s], func: [%s]", 
                adv->srvid, p_pm->binary_name, adv->svc_nm, adv->fn_nm);
        
        /* Check the stuff it must not exist already for server! */
        DL_FOREACH(p_pm->svcs,chk)
        {
            if (0==strcmp(chk->svc.svc_nm, adv->svc_nm))
            {
                found=TRUE;
                break;
            }
        }
        
        if (found)
        {
            NDRX_LOG(log_error, "Service already advertised! Svc: [%s] Func: [%s]",
                    chk->svc.svc_nm, chk->svc.fn_nm);
            /* Keep ret SUCCEED, as not deadly stuff */
            goto out;
        }
        
        new_svc=NDRX_MALLOC(sizeof(pm_node_svc_t));
        if (NULL==new_svc)
        {
            NDRX_LOG(log_always, "Failed to allocate memory for new "
                                "service: %s", strerror(errno) );
            ret=FAIL;
            goto out;
        }
        memset((char *)new_svc, 0, sizeof(new_svc));

        /* Fill up details */
        strcpy(new_svc->svc.svc_nm, adv->svc_nm);
        strcpy(new_svc->svc.fn_nm, adv->fn_nm);
        
        brd_begin_diff();
        
        /* Add to linked list & add to svc shm */
        DL_APPEND(p_pm->svcs, new_svc);
        
        /* add stuff to bridge service hash */
        if (SUCCEED!=brd_add_svc_to_hash(new_svc->svc.svc_nm))
        {
            ret=FAIL;
            goto out;
        }
        
        brd_end_diff();
    }
    
out:
    return ret;
}
