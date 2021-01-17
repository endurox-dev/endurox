/**
 * @brief Return connection infos
 *
 * @file brcon.c
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
 * Clock infos metrics from connected bridges
 */
typedef struct 
{
    long nodeid;   /**< local node id                      */
    long srvid;     /**< Server id generating resposne      */
    long remnodeid; /**< remove node id                     */
    char mode;      /**< Connection mode                    */   
    long fd;         /**< socket FD number                   */
    /* Clock: */
    long conseq;    /**< connection sequence                */
    long lastsync;  /**< last sync time ago (seconds)       */
    long timediff; /**< time diff in seconds between hosts */
    long roundtrip; /**< roundtrip in milliseconds          */
    
    
} ndrx_adm_brcon_t;

/**
 * Bridge clock data
 */
expublic ndrx_adm_elmap_t ndrx_G_brcon_map[] =
{  
    /* Driving of the Preparing: */
     {TA_EX_NODEID,            TPADM_EL(ndrx_adm_brcon_t, nodeid)}
    ,{TA_SRVID,                TPADM_EL(ndrx_adm_brcon_t, srvid)}
    ,{TA_EX_REMNODEID,         TPADM_EL(ndrx_adm_brcon_t, remnodeid)}
    
    ,{TA_EX_FD,                TPADM_EL(ndrx_adm_brcon_t, fd)}
    ,{TA_EX_CONMODE,           TPADM_EL(ndrx_adm_brcon_t, mode)}
    
    ,{TA_EX_LASTSYNC,            TPADM_EL(ndrx_adm_brcon_t, lastsync)}
    ,{TA_EX_TIMEDIFF,          TPADM_EL(ndrx_adm_brcon_t, timediff)}
    ,{TA_EX_ROUNDTRIP,         TPADM_EL(ndrx_adm_brcon_t, roundtrip)}
    ,{BBADFLDID}
};

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

exprivate ndrx_adm_cursors_t *M_cursnew;
exprivate int M_idx;    /**< Current growlist index             */
exprivate string_list_t *M_qlist; /**< list of admin queues  */

/*---------------------------Prototypes---------------------------------*/

/**
 * Process actual infos from bridge
 * @param reply
 * @param reply_len
 * @return EXSUCCED/EXFAIL
 */
exprivate int ndrx_adm_brconinfo_proc_list(command_reply_t *reply, size_t reply_len)
{
    command_reply_brconinfo_t * info = (command_reply_brconinfo_t*)reply;
    int ret = EXSUCCEED;
    ndrx_adm_brcon_t brcon;
    
    if (NDRXD_CALL_TYPE_BRCONINFO!=reply->msg_type)
    {
        /* not payload */
        goto out;
    }

    /* call the bridge */
    memset(&brcon, 0, sizeof(brcon));
    
    brcon.nodeid = info->locnodeid;
    brcon.srvid = info->srvid;
    brcon.fd = info->fd;
    brcon.mode = info->mode; 
    brcon.remnodeid = info->remnodeid;
    brcon.lastsync = info->lastsync;
    brcon.timediff = info->timediffs;
    brcon.roundtrip = info->roundtrip;
       
    if (EXSUCCEED!=ndrx_growlist_add(&M_cursnew->list, (void *)&brcon, M_idx))
    {
        NDRX_LOG(log_error, "Growlist failed - out of memory?");
        EXFAIL_OUT(ret);
    }

    M_idx++;
    
out:
    return ret;
}

/**
 * Bridge admin Q listing
 * @param reply
 * @param reply_len
 * @return 
 */
exprivate int ndrx_adm_blist_proc_list(command_reply_t *reply, size_t reply_len)
{
    command_reply_blist_t * blist_info = (command_reply_blist_t*)reply;
    int ret = EXSUCCEED;

    if (NDRXD_CALL_TYPE_BLIST!=reply->msg_type)
    {
        /* not payload */
        goto out;
    }
    
    NDRX_LOG(log_debug, "Got admin Q for bridge: [%s]", blist_info->qstr);

    if (EXSUCCEED!=ndrx_string_list_add(&M_qlist, blist_info->qstr))
    {
        NDRX_LOG(log_error, "Failed to populate string list");
        EXFAIL_OUT(ret);
    }
    
out:
    return ret;
}

/**
 * Read return clock of connected bridges
 * This assumes that we might handle multiple connections by one bridge
 * (atleast in future).
 * 
 * Firstly we need to get admin queues for the bridges from ndrxd by list call
 * Then each queue needs to request for list bridge infos.
 * 
 * @param clazz class name
 * @param cursnew this is new cursor domain model
 * @param flags not used
 */
expublic int ndrx_adm_brcon_get(char *clazz, ndrx_adm_cursors_t *cursnew, long flags)
{
    int ret = EXSUCCEED;
    string_list_t *qstr;
    
    /* init cursor */
    M_idx = 0;
    M_qlist = NULL;
    
    ndrx_growlist_init(&cursnew->list, 100, sizeof(ndrx_adm_brcon_t));
    
    M_cursnew = cursnew;
    cursnew->map = ndrx_G_brcon_map;
    
    if (EXSUCCEED!=ndrx_adm_list_call(ndrx_adm_blist_proc_list, 
            NDRXD_COM_BLIST_RQ, NDRXD_COM_BLIST_RP, ndrx_get_G_atmi_conf()->ndrxd_q_str))
    {
        NDRX_LOG(log_error, "Failed to call blist");
        EXFAIL_OUT(ret);
    }
    
    LL_FOREACH(M_qlist, qstr)
    {
        /* process the lists */
        if (EXSUCCEED!=ndrx_adm_list_call(ndrx_adm_brconinfo_proc_list, 
                NDRXD_COM_BRCONINFO_RQ, NDRXD_COM_BRCONINFO_RP, qstr->qname))
        {
            NDRX_LOG(log_error, "Failed to call brclockinfo");
            EXFAIL_OUT(ret);
        }
    }
    
out:
    
    if (EXSUCCEED!=ret)
    {
        ndrx_growlist_free(&M_cursnew->list);
    }

    /* free up the list */
    ndrx_string_list_free(M_qlist);

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
