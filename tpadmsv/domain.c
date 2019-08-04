/**
 * @brief Domain data grabber
 *
 * @file domain.c
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
#include <atmi_shm.h>

#include "tpadmsv.h"
#include "expr.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/**
 * Image of the machine
 */
typedef struct 
{
    char domainid[MAXTIDENT+1];/**< cluster node id                            */
    char state[3+1];           /**< current number of open conversations       */
    long curqueues;            /**< current number of queue                    */
    long curservers;           /**< number of server in domain (local)         */
    long curservices;          /**< number of services, including cluster      */
    
} ndrx_adm_domain_t;

/**
 * Client class infos mapping table
 */
expublic ndrx_adm_elmap_t ndrx_G_domain_map[] =
{  
    /* Driving of the Preparing: */
    {TA_DOMAINID,               TPADM_EL(ndrx_adm_domain_t, domainid)}
    ,{TA_STATE,                 TPADM_EL(ndrx_adm_domain_t, state)}
    ,{TA_CURQUEUES,             TPADM_EL(ndrx_adm_domain_t, curqueues)}
    ,{TA_CURSERVERS,            TPADM_EL(ndrx_adm_domain_t, curservers)}
    ,{TA_CURSERVICES,           TPADM_EL(ndrx_adm_domain_t, curservices)}
    ,{BBADFLDID}
};

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Read domain data
 * @param clazz class name
 * @param cursnew this is new cursor domain model
 * @param flags not used
 */
expublic int ndrx_adm_domain_get(char *clazz, ndrx_adm_cursors_t *cursnew, long flags)
{
    int ret = EXSUCCEED;
    ndrx_adm_domain_t dom;
    string_list_t* qlist = NULL;
    string_list_t* elt = NULL;
    int typ;
    /* this is single entry... */
    
    cursnew->map = ndrx_G_domain_map;
    
    memset(&dom, 0, sizeof(dom));
    
    ndrx_growlist_init(&cursnew->list, 100, sizeof(ndrx_adm_domain_t));
    
    snprintf(dom.domainid, sizeof(dom.domainid), "%ld", tpgetnodeid());
    NDRX_STRCPY_SAFE(dom.state, "ACT");
    
    /* counter number of queues: */
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
        dom.curqueues++;
        
        /* extract server reply queues */
        typ = ndrx_q_type_get(elt->qname);
        
        if (NDRX_QTYPE_SRVADM==typ)
        {
            dom.curservers++;
        }
    }
    
    /* get count of services, from ndrxd? */
    dom.curservices = ndrx_shm_get_svc_count();
    
    
    if (EXSUCCEED!=ndrx_growlist_add(&cursnew->list, (void *)&dom, 0))
    {
        NDRX_LOG(log_error, "Growlist failed for domain - out of memory?");
        EXFAIL_OUT(ret);
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
