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
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

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
    ,{BBADFLDID}
};

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
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
            /*
            expublic int ndrx_qdet_parse_cltqstr(ndrx_qdet_t *qdet, char *qstr)
            */
            if (EXSUCCEED==ndrx_myid_parse_qname(elt->qname, &myid))
            {
                clt.pid = myid.pid;
                NDRX_STRCPY_SAFE(clt.clientid, elt->qname);
                clt.contextid = myid.contextid;
                NDRX_STRCPY_SAFE(clt.name, myid.binary_name);
                snprintf(clt.lmid, sizeof(clt.lmid), "%d", myid.nodeid);
                
                if (EXSUCCEED==kill(myid.pid, 0))
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

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
