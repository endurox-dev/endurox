/**
 * @brief Server data grabber
 *
 * @file server.c
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
 * Image of the server
 */
typedef struct 
{
    char lmid[MAXTIDENT+1];     /**< Machine / cluster node id              */
    long srvid;                 /**< Server ID                              */
    char rqaddr[128+1];   /**< Request address if any, used by SysV   */
    char state[3+1];            /**< Server state                           */
    long timerestart;           /**< Santiy cycles from last start          */ 
    long pid;                   /**< Server process PID                     */
    char servername[78+1];      /**< binary name                            */
    char clopt[256+1];          /**< Clopt / real name                      */
    long generation;            /**< Number of restarts for the server proc */
} ndrx_adm_server_t;

/**
 * Client class infos mapping table
 */
expublic ndrx_adm_elmap_t ndrx_G_server_map[] =
{  
    /* Driving of the Preparing: */
    {TA_LMID,                      TPADM_EL(ndrx_adm_server_t, lmid)}
    ,{TA_SRVID,                      TPADM_EL(ndrx_adm_server_t, srvid)}
    ,{TA_RQADDR,                    TPADM_EL(ndrx_adm_server_t, rqaddr)}
    ,{TA_STATE,                     TPADM_EL(ndrx_adm_server_t, state)}
    ,{TA_TIMERESTART,               TPADM_EL(ndrx_adm_server_t, timerestart)}
    ,{TA_PID,                       TPADM_EL(ndrx_adm_server_t, pid)}
    ,{TA_SERVERNAME,                TPADM_EL(ndrx_adm_server_t, servername)}
    ,{TA_CLOPT,                     TPADM_EL(ndrx_adm_server_t, clopt)}
    ,{TA_GENERATION,                TPADM_EL(ndrx_adm_server_t, generation)}
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
exprivate int ndrx_adm_server_proc_list(command_reply_t *reply, size_t reply_len)
{
    command_reply_ppm_t * ppm_info = (command_reply_ppm_t*)reply;
    int ret = EXSUCCEED;
    
    if (NDRXD_CALL_TYPE_PM_PPM!=reply->msg_type)
    {
        /* not payload */
        goto out;
    }
    
    NDRX_LOG(log_debug, "ppm out: [%s]", ppm_info->binary_name);
    
    ndrx_adm_server_t srv;
    memset(&srv, 0, sizeof(srv));

    srv.srvid = ppm_info->srvid;
    srv.pid = ppm_info->pid;
    NDRX_STRCPY_SAFE(srv.rqaddr, ppm_info->rqaddress);
    srv.timerestart = ppm_info->state_changed;
    srv.generation = ppm_info->exec_seq_try + 1; /* number of consecutive restarts */
    snprintf(srv.lmid, sizeof(srv.lmid), "%ld", tpgetnodeid());

    if (NDRXD_PM_RUNNING_OK==ppm_info->state)
    {
        NDRX_STRCPY_SAFE(srv.state, "ACT");
    }
    else if (NDRXD_PM_STARTING==ppm_info->state)
    {
        NDRX_STRCPY_SAFE(srv.state, "RES");
    }
    else if (NDRXD_PM_STOPPING==ppm_info->state)
    {
        NDRX_STRCPY_SAFE(srv.state, "CLE");
    }
    else
    {
        NDRX_STRCPY_SAFE(srv.state, "DEA");
    }

    NDRX_STRCPY_SAFE(srv.servername, ppm_info->binary_name);
    NDRX_STRCPY_SAFE(srv.clopt, ppm_info->binary_name_real);

    if (EXSUCCEED!=ndrx_growlist_add(&M_cursnew->list, (void *)&srv, M_idx))
    {
        NDRX_LOG(log_error, "Growlist failed - out of memory?");
        EXFAIL_OUT(ret);
    }

    NDRX_LOG(log_debug, "Server [%s]/%ld state %s added", srv.servername, 
            srv.srvid, srv.state);
    M_idx++;    
out:
    return ret;
}

/**
 * Read server data
 * @param clazz class name
 * @param cursnew this is new cursor domain model
 * @param flags not used
 */
expublic int ndrx_adm_server_get(char *clazz, ndrx_adm_cursors_t *cursnew, long flags)
{
    int ret = EXSUCCEED;

    /* init cursor */
    M_idx = 0;
    
    ndrx_growlist_init(&cursnew->list, 100, sizeof(ndrx_adm_server_t));
    
    M_cursnew = cursnew;
    cursnew->map = ndrx_G_server_map;
    
    if (EXSUCCEED!=ndrx_adm_ppm_call(ndrx_adm_server_proc_list))
    {
        NDRX_LOG(log_error, "Failed to call PPM");
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
