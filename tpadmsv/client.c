/**
 * @brief This is client image grabber & mapping
 *   TODO: XATMI client contexts start from 1. Thus avoid the 0 contexts
 *   as we are setting them to -1. Thus the -1 we will allow to update to 1.
 *
 * @file client.c
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

#include <tpadmsv.h>
#include "expr.h"
#include "cpm.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/**
 * Image of the client information
 */
typedef struct 
{
    char clientid[128+1];     /**< myid                                 */
    char name[MAXTIDENT+1];   /**< process name                         */
    char lmid[MAXTIDENT+1];   /**< cluster node id                      */
    /** may be: ACT, SUS (not used), DEA - dead */
    char state[15+1];         /**< state of the client live/dead by pid */
    long pid;                 /**< process PID                          */
    long curconv;             /**< number of conversations process into */
    long contextid;           /**< Multi-threading context id           */
    long curtime;             /**< Current time when process started    */
    int cursor_loaded;        /**< Is loaded into cursor?               */
    
    EX_hash_handle hh;         /**< makes this structure hashable   */
    
} ndrx_adm_client_t;

/**
 * Client class infos mapping table
 */
expublic ndrx_adm_elmap_t ndrx_G_client_map[] =
{  
    /* Driving of the Preparing: */
     {TA_LMID,          TPADM_EL(ndrx_adm_client_t, lmid)} /* key */
    ,{TA_CLIENTID,       TPADM_EL(ndrx_adm_client_t, clientid)} /* key */
    ,{TA_CLTNAME,       TPADM_EL(ndrx_adm_client_t, name)}
    ,{TA_STATE,         TPADM_EL(ndrx_adm_client_t, state)}
    ,{TA_PID,           TPADM_EL(ndrx_adm_client_t, pid)}
    ,{TA_CURCONV,       TPADM_EL(ndrx_adm_client_t, curconv)}
    ,{TA_CONTEXTID,     TPADM_EL(ndrx_adm_client_t, contextid)} /* key */
    ,{TA_CURTIME,       TPADM_EL(ndrx_adm_client_t, curtime)}
    ,{BBADFLDID}
};

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

/**
 * Pre hash of the results to merge the queue and CPM/SHM infos
 */
exprivate ndrx_adm_client_t *M_prehash = NULL;
/*---------------------------Prototypes---------------------------------*/

/**
 * Build up the cursor
 * Scan the queues and add the elements
 * @param clazz class name
 * @param cursnew this is new cursor domain model
 * @param flags not used
 */
expublic int ndrx_adm_client_get(char *clazz, ndrx_adm_cursors_t *cursnew, long flags)
{
    int ret = EXSUCCEED;
    int typ;
    string_list_t* qlist = NULL;
    string_list_t* elt = NULL;
    ndrx_qdet_t qdet;
    TPMYID myid;
    ndrx_adm_client_t clt;
    ndrx_adm_client_t *p_clt, *p_clt2, *cel, *clet;
    int cltshm_attached = EXFALSE;
    ndrx_sem_t *sem = NULL;
    ndrx_shm_t *shm = NULL;
    ndrx_clt_shm_t *el;
    long l1, l2;
    int idx=0;
    int i;
    cursnew->map = ndrx_G_client_map;
    /* setup the list */
    ndrx_growlist_init(&cursnew->list, 100, sizeof(ndrx_adm_client_t));
    
    qlist = ndrx_sys_mqueue_list_make(G_atmi_env.qpath, &ret);
    
    if (EXSUCCEED!=ret)
    {
        NDRX_LOG(log_error, "posix queue listing failed!");
        EXFAIL_OUT(ret);
    }
     
    /* seems we need to go over twice, because we might get conversational q first */
    LL_FOREACH(qlist,elt)
    {
        /* parse the queue..., extract clients.. */
        
        /* if not print all, then skip this queue */
        if (0!=strncmp(elt->qname, 
                G_atmi_env.qprefix_match, G_atmi_env.qprefix_match_len))
        {
            continue;
        }
        /* extract clients... get
        typ = ndrx_q_type_get(elt->qname);
        */
        typ = ndrx_q_type_get(elt->qname);
        
        
        if (NDRX_QTYPE_CLTRPLY==typ)
        {
            if (EXSUCCEED==ndrx_qdet_parse_cltqstr(&qdet, elt->qname))
            {
                memset(&clt, 0, sizeof(clt));
                clt.pid = qdet.pid;
                NDRX_STRCPY_SAFE(clt.clientid, elt->qname);
                clt.contextid = qdet.contextid;
                NDRX_STRCPY_SAFE(clt.name, qdet.binary_name);
                snprintf(clt.lmid, sizeof(clt.lmid), "%ld", tpgetnodeid());
                
                if (EXSUCCEED==kill(qdet.pid, 0))
                {
                    NDRX_STRCPY_SAFE(clt.state, "ACT");
                }
                else
                {
                    NDRX_STRCPY_SAFE(clt.state, "DEA");
                }
            
                if (EXSUCCEED!=ndrx_growlist_add(&cursnew->list, (void *)&clt, idx))
                {
                    NDRX_LOG(log_error, "Growlist failed - out of memory?");
                    EXFAIL_OUT(ret);
                }
                
                NDRX_LOG(log_debug, "client [%s] state %s added (Q)", 
                        clt.clientid, clt.state);
                idx++;
            }
        }        
    } /* LL_FOREACH(qlist,elt) */
    
    /* scan for conv Qs and update the stats... */
    LL_FOREACH(qlist,elt)
    {
        /* parse the queue..., extract clients.. */
        
        /* if not print all, then skip this queue */
        if (0!=strncmp(elt->qname, 
                G_atmi_env.qprefix_match, G_atmi_env.qprefix_match_len))
        {
            continue;
        }
        /* extract clients... get
        typ = ndrx_q_type_get(elt->qname);
        */
        typ = ndrx_q_type_get(elt->qname);
        
        if (NDRX_QTYPE_CONVINIT==typ)
        {
            /* parse: NDRX_CONV_INITATOR_Q_PFX and search for client if so */
            if (EXSUCCEED==ndrx_cvnq_parse_client(elt->qname, &myid))
            {
                /* search for client... */
                for (i=0; i<=cursnew->list.maxindexused; i++)
                {
                    p_clt = (ndrx_adm_client_t *) (cursnew->list.mem + i*sizeof(ndrx_adm_client_t));
                    
                    /* reset binary_name to common len... */
                    myid.binary_name[MAXTIDENT] = EXEOS;
                    if (p_clt->pid == myid.pid
                            && p_clt->contextid == myid.contextid
                            && 0==strcmp(p_clt->name, myid.binary_name)
                            && myid.nodeid == atoi(p_clt->lmid)
                            )
                    {
                        p_clt->curconv++;
                        NDRX_LOG(log_debug, "client [%s] currconv incremented to %ld", 
                                p_clt->name, p_clt->curconv);
                        break;
                    }
                }
                
            } /* If q parse OK */
        } /* NDRX_CLT_QREPLY_PFX==typ */
        
    } /* LL_FOREACH(qlist,elt) */
        
    
    /* open the SHM & scan for clients -> think about opening at startup?
     * may be we could conserve some resources.
     * On the other hand cpmsrv can live it's life and manage it's shm
     * as it wishes?
     */
    
    if (EXSUCCEED==ndrx_cltshm_init(EXTRUE))
    {
        char *p;
        cltshm_attached = EXTRUE;
        
        sem = ndrx_cltshm_sem_get();
        shm = ndrx_cltshm_mem_get();

        /* scan for the elements */
        if (EXSUCCEED!=ndrx_sem_rwlock(sem, 0, NDRX_SEM_TYP_READ))
        {
            goto out;
        }
        
        NDRX_LOG(log_debug, "Build up the hash of SHM");
        for (i=0; i<G_atmi_env.max_clts; i++)
        {
            el = NDRX_CPM_INDEX(shm->mem, i);
            
            if (el->flags & NDRX_CPM_MAP_ISUSED && 
                        ndrx_sys_is_process_running_by_pid(el->pid))
            {
                int err;
                p_clt = NDRX_CALLOC(1, sizeof(ndrx_adm_client_t));
                
                err = errno;
                
                if (NULL==p_clt)
                {
                    NDRX_LOG(log_error, "Failed to calloc of %d bytes failed: %s",
                            sizeof(ndrx_adm_client_t), strerror(err));
                    userlog("Failed to calloc of %d bytes failed: %s",
                            sizeof(ndrx_adm_client_t), strerror(err));
                    EXFAIL_OUT(ret);
                }
                
                p_clt->pid = el->pid;
                
                snprintf(p_clt->clientid, sizeof(p_clt->clientid),
                        "%ld/%s", tpgetnodeid(), el->key);
                
                p = strchr(p_clt->clientid, NDRX_CPM_SEP);
                if (NULL!=p)
                {
                    *p = '/';
                }
                
                /* set start time */
                p_clt->curtime = (long)el->stattime;
                p_clt->curconv = EXFAIL;
                p_clt->contextid = EXFAIL;
                snprintf(p_clt->lmid, sizeof(clt.lmid), "%ld", tpgetnodeid());
                
                /* set binary name */
                NDRX_STRCPY_SAFE(p_clt->name, el->procname);
                NDRX_STRCPY_SAFE(p_clt->state, "ACT");
                
                EXHASH_ADD_LONG(M_prehash, pid, p_clt);
                
                NDRX_LOG(log_debug, "Hashed pid=%d - %s", p_clt->pid, p_clt->clientid)
                
            } /* if used */
        }
        
        ndrx_sem_rwunlock(sem, 0, NDRX_SEM_TYP_WRITE);
    }
    
    /* merge the clients.. */
    NDRX_LOG(log_debug, "Merge clients Q + SHM");
    
    for (i=0; i<=cursnew->list.maxindexused; i++)
    {
        /* check the hash, update */
        p_clt = (ndrx_adm_client_t *) (cursnew->list.mem + i*sizeof(ndrx_adm_client_t));
        
        EXHASH_FIND_LONG(M_prehash, &(p_clt->pid), p_clt2);
        
        if (NULL!=p_clt2)
        {
            NDRX_LOG(log_debug, "PID %d matched with SHM", (int)p_clt->pid)
            /* update the client */
            snprintf(p_clt->clientid, sizeof(p_clt->clientid), "%s/%ld",
                    p_clt2->clientid, p_clt->contextid);
            
            /* update startup time */
            p_clt->curtime = p_clt2->curtime;
            p_clt2->cursor_loaded = EXTRUE;
        }
    }
    
    /* load the hash elems with out other data to cursor */
    NDRX_LOG(log_debug, "Add HASH/SHM data");
    EXHASH_ITER(hh, M_prehash, cel, clet)
    {
        if (!cel->cursor_loaded)
        {
            /* Add to cursor as much data as we have */
            memcpy(&clt, cel, sizeof(clt));
            if (EXSUCCEED!=ndrx_growlist_add(&cursnew->list, (void *)&clt, idx))
            {
                NDRX_LOG(log_error, "Growlist failed - out of memory?");
                EXFAIL_OUT(ret);
            }

            NDRX_LOG(log_debug, "client [%s] state %s added (SHM)", clt.clientid, clt.state);
            idx++;
        }
    }
    
    
out:
    
    if (cltshm_attached)
    {
        ndrx_cltshm_detach();
    }
    
    if (NULL!=qlist)
    {
        ndrx_string_list_free(qlist);
    }

    if (EXSUCCEED!=ret)
    {
        ndrx_growlist_free(&cursnew->list);
    }

    /* clean up the hash list */
    EXHASH_ITER(hh, M_prehash, cel, clet)
    {
        EXHASH_DEL(M_prehash, cel);
        NDRX_FREE(cel);
    }

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
