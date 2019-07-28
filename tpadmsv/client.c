/**
 * @brief This is client image grabber & mapping
 *
 * @file client.c
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
    char clientid[78+1];      /**< myid                                 */
    char name[MAXTIDENT+1];   /**< process name                         */
    char lmid[MAXTIDENT+1];   /**< cluster node id                      */
    /** may be: ACT, SUS (not used), DEA - dead */
    char state[15+1];         /**< state of the client live/dead by pid */
    long pid;                 /**< process PID                          */
    long curconv;             /**< number of conversations process into */
    long contextid;           /**< Multi-threading context id           */
    long curtime;             /**< Current time when process started    */
    
    EX_hash_handle hh;         /**< makes this structure hashable   */
    
} ndrx_adm_client_t;

/**
 * Client class infos mapping table
 */
expublic ndrx_adm_elmap_t ndrx_G_client_map[] =
{  
    /* Driving of the Preparing: */
    {TA_CLIENTID,       EXOFFSET(ndrx_adm_client_t, clientid)}
    ,{TA_CLTNAME,       EXOFFSET(ndrx_adm_client_t, name)}
    ,{TA_LMID,          EXOFFSET(ndrx_adm_client_t, lmid)}
    ,{TA_STATE,         EXOFFSET(ndrx_adm_client_t, state)}
    ,{TA_PID,           EXOFFSET(ndrx_adm_client_t, pid)}
    ,{TA_CURCONV,       EXOFFSET(ndrx_adm_client_t, curconv)}
    ,{TA_CONTEXTID,     EXOFFSET(ndrx_adm_client_t, contextid)}
    ,{TA_CURTIME,       EXOFFSET(ndrx_adm_client_t, curtime)}
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
 * Clean up the hash
 */
exprivate void ndrx_adm_client_hash_cleanup(void)
{
    /* TODO: Clean up the hash after cursor is filled */
}

/**
 * Lookup and update, if exists
 * @param [in] pid pid to search for
 * @param [in] el element to update with (data take from)
 * @return EXTRUE found and update, EXFALSE - not found
 */
exprivate int ndrx_adm_client_getupd(pid_t pid, ndrx_clt_shm_t *el)
{
    ndrx_adm_client_t *cltres = NULL;
    int found = EXFALSE;
    char *p;
    
    EXHASH_FIND_INT(M_prehash, pid, cltres);
    
    if (NULL!=el)
    {
        NDRX_STRCPY_SAFE(cltres->clientid, el->key);
        /* replace FS with / */
        p = strchr(cltres->clientid, NDRX_CPM_SEP);
        if (NULL!=p)
        {
            *p = '/';
        }
        
        /* set start time */
        cltres->curtime = (long)el->stattime;
        
        found = EXTRUE;
        
    }
    
    return found;
}

/* TODO: FILL The hash from Q and from SHM.
 * Transfer from HASH to Cursor
 */

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
    ndrx_adm_client_t *p_clt;
    
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
            memset(&clt, 0, sizeof(clt));
            
            if (EXSUCCEED==ndrx_qdet_parse_cltqstr(&qdet, elt->qname))
            {
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
                
                NDRX_LOG(log_debug, "client [%s] state %s added", clt.name, clt.state);
                idx++;
            }
        }
        else if (NDRX_QTYPE_CONVINIT==typ)
        {
            /* parse: NDRX_CONV_INITATOR_Q_PFX and search for client if so */
            if (EXSUCCEED==ndrx_cvnq_parse_client(elt->qname, &myid))
            {
                /* search for client... */
                for (i=0; i<=cursnew->list.maxindexused; i++)
                {
                    p_clt = (ndrx_adm_client_t *) (cursnew->list.mem + i*sizeof(ndrx_adm_client_t));
                    
                    /* reset binary_name to common len... */
                    myid.binary_name[MAXTIDENT];
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
