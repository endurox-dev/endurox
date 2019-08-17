/**
 * @brief Machine data grabber
 *
 * @file svcgrp.c
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
#include <fcntl.h>

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
#include <gencall.h>
#include "tpadmsv.h"
#include "expr.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
    
/**
 * + OID HASH
 * Image of the service group / dynamic service info
 */
typedef struct 
{
    char servicename[MAXTIDENT+1];      /**< Service name                     */
    char srvgrp[MAXTIDENT+1];           /**< Server group                     */
    char state[3+1];                    /**< Current service state, "ACT" const */
    char lmid[MAXTIDENT+1];             /**< Node id                          */
    long srvid;                         /**< Server process id                */
    char svcrname[MAXTIDENT+1];         /**< Function name                    */
    long ncompleted;                    /**< Requests completed succ + fail   */
    long totsuccnum;                    /**< Total success calls              */
    long totsfailnum;                   /**< Total number failures            */
    long lastexectimeusec;              /**< Last exec time                   */
    long maxexectimeusec;               /**< max exec time                    */
    long minexectimeusec;               /**< min exec time                    */
} ndrx_adm_svcgrp_t;

/**
 * Service group class infos mapping table
 */
expublic ndrx_adm_elmap_t ndrx_G_svcgrp_map[] =
{  
    /* Driving of the Preparing: */
     {TA_LMID,                  TPADM_EL(ndrx_adm_svcgrp_t, lmid)}
    ,{TA_SERVICENAME,            TPADM_EL(ndrx_adm_svcgrp_t, servicename)}
    ,{TA_SRVGRP,                TPADM_EL(ndrx_adm_svcgrp_t, srvgrp)}
    ,{TA_STATE,                 TPADM_EL(ndrx_adm_svcgrp_t, state)}
    ,{TA_SRVID,                 TPADM_EL(ndrx_adm_svcgrp_t, srvid)}
    ,{TA_SVCRNAM,               TPADM_EL(ndrx_adm_svcgrp_t, svcrname)}
    ,{TA_NCOMPLETED,            TPADM_EL(ndrx_adm_svcgrp_t, ncompleted)}
    ,{TA_TOTSUCCNUM,            TPADM_EL(ndrx_adm_svcgrp_t, totsuccnum)}
    ,{TA_TOTSFAILNUM,           TPADM_EL(ndrx_adm_svcgrp_t, totsfailnum)}
    ,{TA_LASTEXECTIMEUSEC,      TPADM_EL(ndrx_adm_svcgrp_t, lastexectimeusec)}
    ,{TA_MAXEXECTIMEUSEC,       TPADM_EL(ndrx_adm_svcgrp_t, maxexectimeusec)}
    ,{TA_MINEXECTIMEUSEC,       TPADM_EL(ndrx_adm_svcgrp_t, minexectimeusec)}
    ,{BBADFLDID}
};

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

exprivate ndrx_adm_cursors_t *M_cursnew;
exprivate int M_idx = 0;    /**< Current growlist index */
/*---------------------------Prototypes---------------------------------*/

/**
 * Fill any cluster data here, filter by bridge processes
 * @param reply
 * @param reply_len
 * @return 
 */
exprivate int ndrx_adm_svcgrp_proc_list(command_reply_t *reply, size_t reply_len)
{
    command_reply_psc_t * psc_info = (command_reply_psc_t*)reply;
    int ret = EXSUCCEED;
    int i;
    if (NDRXD_CALL_TYPE_SVCINFO!=reply->msg_type)
    {
        /* not payload */
        goto out;
    }
    
    NDRX_LOG(log_debug, "psc out srvid: [%dd]", psc_info->srvid);
    
    for (i=0; i<psc_info->svc_count; i++)
    {
        
        ndrx_adm_svcgrp_t svc;
        memset(&svc, 0, sizeof(svc));
        
        NDRX_STRCPY_SAFE(svc.servicename, psc_info->svcdet[i].svc_nm);
        snprintf(svc.srvgrp, sizeof(svc.srvgrp), "%d/%d", psc_info->nodeid, 
                psc_info->srvid);
        NDRX_STRCPY_SAFE(svc.state, "ACT");
        snprintf(svc.lmid, sizeof(svc.lmid), "%d", psc_info->nodeid);
        svc.srvid = psc_info->srvid;
        NDRX_STRCPY_SAFE(svc.svcrname,psc_info->svcdet[i].fn_nm);
        svc.ncompleted = psc_info->svcdet[i].done+psc_info->svcdet[i].fail;
        
        if (svc.ncompleted < -1)
        {
            svc.ncompleted = -1;
        }
        svc.totsuccnum = psc_info->svcdet[i].done;
        svc.totsfailnum = psc_info->svcdet[i].fail;
        svc.lastexectimeusec = psc_info->svcdet[i].last *1000; /* msec -> usec */
        svc.maxexectimeusec = psc_info->svcdet[i].max*1000; /* msec -> usec */
        svc.minexectimeusec = psc_info->svcdet[i].min*1000; /* msec -> usec */
        
        if (svc.lastexectimeusec < -1)
        {
            svc.lastexectimeusec = -1;
        }
        
        if (svc.maxexectimeusec < -1)
        {
            svc.maxexectimeusec = -1;
        }
        
        if (svc.minexectimeusec < -1)
        {
            svc.minexectimeusec = -1;
        }
        
        if (EXSUCCEED!=ndrx_growlist_add(&M_cursnew->list, (void *)&svc, M_idx))
        {
            NDRX_LOG(log_error, "Growlist failed - out of memory?");
            EXFAIL_OUT(ret);
        }

        NDRX_LOG(log_debug, "Service added [%s] / [%s]", svc.servicename, svc.srvgrp);
        M_idx++;
        
    }
    
out:
    return ret;
}

/**
 * Read svcgrp data
 * @param clazz class name
 * @param cursnew this is new cursor domain model
 * @param flags not used
 */
expublic int ndrx_adm_svcgrp_get(char *clazz, ndrx_adm_cursors_t *cursnew, long flags)
{
    int ret = EXSUCCEED;
    
    /* init cursor */
    M_idx = 0;
    
    ndrx_growlist_init(&cursnew->list, 100, sizeof(ndrx_adm_svcgrp_t));
    
    M_cursnew = cursnew;
    cursnew->map = ndrx_G_svcgrp_map;
    
    if (EXSUCCEED!=ndrx_adm_psc_call(ndrx_adm_svcgrp_proc_list))
    {
        NDRX_LOG(log_error, "Failed to call PSC");
        EXFAIL_OUT(ret);
    }
    
out:
    
    if (EXSUCCEED!=ret)
    {
        ndrx_growlist_free(&M_cursnew->list);
    }

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
