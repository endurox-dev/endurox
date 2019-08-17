/**
 * @brief Service data grabber -> basic info about services in system.
 *
 * @file service.c
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <regex.h>
#include <utlist.h>
#include <unistd.h>
#include <signal.h>

#include <ndebug.h>
#include <atmi.h>
#include <atmi_int.h>
#include <typed_buf.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <Exfields.h>
#include <Excompat.h>
#include <ubfutil.h>
#include <sys_unix.h>

#include "tpadmsv.h"
#include "expr.h"
#include "atmi_shm.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/**
 * Image of the queue
 * + OID HASH.
 */
typedef struct 
{
    char service[MAXTIDENT+1];    /**< Service name                     */
    char state[3+1];        /**< Service state ACT only                 */
} ndrx_adm_service_t;

/**
 * Service class infos mapping table
 */
expublic ndrx_adm_elmap_t ndrx_G_service_map[] =
{  
    /* Driving of the Preparing: */
    {TA_SERVICENAME,             TPADM_EL(ndrx_adm_service_t, service)}
    ,{TA_STATE,                  TPADM_EL(ndrx_adm_service_t, state)}
    ,{BBADFLDID}
};

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Read service data
 * @param clazz class name
 * @param cursnew this is new cursor domain model
 * @param flags not used
 */
expublic int ndrx_adm_service_get(char *clazz, ndrx_adm_cursors_t *cursnew, long flags)
{
    int ret = EXSUCCEED;
    shm_svcinfo_t *svcinfo = (shm_svcinfo_t *) G_svcinfo.mem;
    int i;
    ndrx_adm_service_t svc;
    int idx = 0;
    
    cursnew->map = ndrx_G_service_map;
    
    ndrx_growlist_init(&cursnew->list, 100, sizeof(ndrx_adm_service_t));
    
    if (!ndrx_shm_is_attached(&G_svcinfo))
    {
        /* no SHM infos */
        NDRX_LOG(log_debug, "SHM not attached -> no service count");
        ret=EXFAIL;
        goto out;
    }
    
    /* Scan the service memory and add some infos up here...
     * well will not show dead services and they might be overwritten with
     * out lock.
     */
    for (i=0; i< G_max_svcs; i++)
    {
        shm_svcinfo_t* ent = SHM_SVCINFO_INDEX(svcinfo, i);

        if (ent->flags & NDRXD_SVCINFO_INIT)
        {
            memset(&svc, 0, sizeof(svc));
            
            /* WARNING ! we might get incomplete readings here! */
            NDRX_STRCPY_SAFE(svc.service, ent->service);
            
            if (ent->srvs > 0)
            {
                NDRX_STRCPY_SAFE(svc.state, "ACT");
            }
            else
            {
                NDRX_STRCPY_SAFE(svc.state, "INA");
            }
            
            if (EXSUCCEED!=ndrx_growlist_add(&cursnew->list, (void *)&svc, idx))
            {
                NDRX_LOG(log_error, "Growlist failed - out of memory?");
                EXFAIL_OUT(ret);
            }
            idx++;
        }
    }
    
out:

    if (EXSUCCEED!=ret)
    {
        ndrx_growlist_free(&cursnew->list);
    }

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
