/**
 * @brief Queue data grabber
 *
 * @file queue.c
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
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/**
 * Image of the queue
 * + OID of rqaddr (q)
 */
typedef struct 
{
    char lmid[MAXTIDENT+1];     /**< node id(last 30 chars)                 */
    char rqaddr[NDRX_MAX_Q_SIZE+1];/**< queue name (last 30 chars)          */
    char state[3+1];            /**< const: ACT - active machine            */
    long nqueued;               /**< Number of messages queued              */
    long rqid;                  /**< Queue ID (for System V only)           */
} ndrx_adm_queue_t;


/**
 * Client class infos mapping table
 */
expublic ndrx_adm_elmap_t ndrx_G_queue_map[] =
{  
    /* Driving of the Preparing: */
     {TA_LMID,                  TPADM_EL(ndrx_adm_queue_t, lmid)}
    ,{TA_RQADDR,                TPADM_EL(ndrx_adm_queue_t, rqaddr)}
    ,{TA_STATE,                 TPADM_EL(ndrx_adm_queue_t, state)}
    ,{TA_NQUEUED,               TPADM_EL(ndrx_adm_queue_t, nqueued)}
    /* TA_RQID for System V mapping... 
     * Resolve from ndrx_svqshm_get(char *qstr, mode_t mode, int oflag)
     */
    ,{TA_RQID,                  TPADM_EL(ndrx_adm_queue_t, rqid)}
    ,{BBADFLDID}
};

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Read queue data
 * @param clazz class name
 * @param cursnew this is new cursor domain model
 * @param flags not used
 */
expublic int ndrx_adm_queue_get(char *clazz, ndrx_adm_cursors_t *cursnew, long flags)
{
    int ret = EXSUCCEED;
    string_list_t* qlist = NULL;
    string_list_t* elt = NULL;
    ndrx_adm_queue_t q;
    int idx = 0;
    struct mq_attr att;
    
    cursnew->map = ndrx_G_queue_map;
    
    ndrx_growlist_init(&cursnew->list, 100, sizeof(ndrx_adm_queue_t));
    
    qlist = ndrx_sys_mqueue_list_make(G_atmi_env.qpath, &ret);
    
    if (EXSUCCEED!=ret)
    {
        NDRX_LOG(log_error, "posix queue listing failed!");
        EXFAIL_OUT(ret);
    }
    
    /* get the usage states / check queues... */
    LL_FOREACH(qlist,elt)
    {
        /* if not print all, then skip this queue */
        if (0!=strncmp(elt->qname, 
                G_atmi_env.qprefix_match, G_atmi_env.qprefix_match_len))
        {
            continue;
        }
    
        memset(&q, 0, sizeof(q));
        
        NDRX_STRCPY_SAFE(q.rqaddr, elt->qname);
        
#ifdef EX_USE_SYSVQ
        q.rqid = ndrx_svqshm_get(q.rqaddr, 0, 0);
#endif
        
        snprintf(q.lmid, sizeof(q.lmid), "%ld", tpgetnodeid());
        NDRX_STRCPY_SAFE(q.state, "ACT");
        
        /* get stats... */
        if (EXSUCCEED!=ndrx_get_q_attr(elt->qname, &att))
        {
            /* skip this one... */
            continue;
        }

        q.nqueued = att.mq_curmsgs;
        
        if (EXSUCCEED!=ndrx_growlist_add(&cursnew->list, (void *)&q, idx))
        {
            NDRX_LOG(log_error, "Growlist on queue failed - out of memory?");
            EXFAIL_OUT(ret);
        }
        
        idx++;
    }
    
out:
    if (NULL!=qlist)
    {
        ndrx_string_list_free(qlist);
    }

    if (EXSUCCEED!=ret)
    {
        ndrx_growlist_free(&cursnew->list);
    }

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
