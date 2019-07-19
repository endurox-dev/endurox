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
};

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Build up the cursor
 * Scan the queues and add the elements
 * @param cursnew this is new cursor domain model
 */
expublic int ndrx_adm_client_get(ndrx_adm_cursors_t *cursnew)
{
    int ret = EXSUCCEED;
    
    string_list_t* qlist = NULL;
    string_list_t* elt = NULL;

    
    /* setup the list */
    if (EXSUCCEED!=ndrx_growlist_init(&cursnew->list, 100, sizeof(ndrx_adm_client_t)))
    {
        NDRX_LOG(log_error, "Failed to setup growlist! OOM?");
        EXFAIL_OUT(ret);
    }
    
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
    }
    
    
out:
    
    if (NULL!=qlist)
    {
        ndrx_string_list_free(qlist);
    }

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
