/**
 * @brief Machine data grabber
 *
 * @file machine.c
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
 * Image of the machine
 */
typedef struct 
{
    char lmid[MAXTIDENT+1];   /**< cluster node id                              */
    long curaccessers;        /**< current accessers on machine server+clt+ctx  */
    long curclients;          /**< current number of client contexts            */
    long curconv;             /**< current number of open conversations         */
    char state[3+1];          /**< state information of the matchine (conn)     */
} ndrx_adm_machine_t;

/**
 * Client class infos mapping table
 */
expublic ndrx_adm_elmap_t ndrx_G_machine_map[] =
{  
    /* Driving of the Preparing: */
    {TA_LMID,                   TPADM_EL(ndrx_adm_machine_t, lmid)}
    ,{TA_CURACCESSERS,          TPADM_EL(ndrx_adm_machine_t, curaccessers)}
    ,{TA_CURCLIENTS,            TPADM_EL(ndrx_adm_machine_t, curclients)}
    ,{TA_CURCONV,               TPADM_EL(ndrx_adm_machine_t, curconv)}
    ,{TA_STATE,                 TPADM_EL(ndrx_adm_machine_t, state)}
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
exprivate int ndrx_adm_machine_proc_list(command_reply_t *reply, size_t reply_len)
{
    command_reply_ppm_t * ppm_info = (command_reply_ppm_t*)reply;
    int ret = EXSUCCEED;
    
    if (NDRXD_CALL_TYPE_PM_PPM!=reply->msg_type)
    {
        /* not payload */
        goto out;
    }
    
    NDRX_LOG(log_debug, "ppm out: [%s]", ppm_info->binary_name);
    if (ppm_info->flags & SRV_KEY_FLAGS_BRIDGE)
    {
        ndrx_adm_machine_t mach;
        memset(&mach, 0, sizeof(mach));

        mach.curaccessers = EXFAIL;
        mach.curclients = EXFAIL;
        mach.curconv = EXFAIL;
        snprintf(mach.lmid, sizeof(mach.lmid), "%hd", ppm_info->nodeid);

        /* Check the state */
        if (ppm_info->flags & SRV_KEY_FLAGS_CONNECTED)
        {
            NDRX_STRCPY_SAFE(mach.state, "ACT");
        }
        else if (NDRXD_PM_RUNNING_OK==ppm_info->state)
        {
            NDRX_STRCPY_SAFE(mach.state, "PEN");
        }
        else
        {
            NDRX_STRCPY_SAFE(mach.state, "INA");
        }

        if (EXSUCCEED!=ndrx_growlist_add(&M_cursnew->list, (void *)&mach, M_idx))
        {
            NDRX_LOG(log_error, "Growlist failed - out of memory?");
            EXFAIL_OUT(ret);
        }

        NDRX_LOG(log_debug, "Machine/Node [%s] state %s added", mach.lmid, mach.state);
        M_idx++;
    }
    
out:
    return ret;
}

/**
 * Read machine data
 * @param clazz class name
 * @param cursnew this is new cursor domain model
 * @param flags not used
 */
expublic int ndrx_adm_machine_get(char *clazz, ndrx_adm_cursors_t *cursnew, long flags)
{
    int ret = EXSUCCEED;
    ndrx_adm_machine_t mach;
    
    string_list_t* qlist = NULL;
    string_list_t* elt = NULL;
    
    int typ;
    /* init cursor */
    M_idx = 0;
    
    ndrx_growlist_init(&cursnew->list, 100, sizeof(ndrx_adm_machine_t));
    
    M_cursnew = cursnew;
    cursnew->map = ndrx_G_machine_map;
    
    if (EXSUCCEED!=ndrx_adm_ppm_call(ndrx_adm_machine_proc_list))
    {
        NDRX_LOG(log_error, "Failed to call PPM");
        EXFAIL_OUT(ret);
    }
    
    /* Add local machine too. */
    memset(&mach, 0, sizeof(mach));
    
    snprintf(mach.lmid, sizeof(mach.lmid), "%ld", tpgetnodeid());
    NDRX_STRCPY_SAFE(mach.state, "ACT");
    
    qlist = ndrx_sys_mqueue_list_make(G_atmi_env.qpath, &ret);
    
    if (EXSUCCEED!=ret)
    {
        NDRX_LOG(log_error, "posix queue listing failed!");
        EXFAIL_OUT(ret);
    }
    
    /* get the usage states / check queues... */
    LL_FOREACH(qlist,elt)
    {
        /* parse the queue..., extract clients.. */
        
        /* if not print all, then skip this queue */
        if (0!=strncmp(elt->qname, 
                G_atmi_env.qprefix_match, G_atmi_env.qprefix_match_len))
        {
            continue;
        }
        
        typ = ndrx_q_type_get(elt->qname);
        
        switch (typ)
        {
            case NDRX_QTYPE_CLTRPLY:
                mach.curclients++;
                mach.curaccessers++;
                break;
            case NDRX_QTYPE_SRVRPLY:
                mach.curaccessers++;
                break;
            case NDRX_QTYPE_CONVINIT:
                mach.curconv++;
                break;
        }
    }
    
    if (EXSUCCEED!=ndrx_growlist_add(&M_cursnew->list, (void *)&mach, M_idx))
    {
        NDRX_LOG(log_error, "Growlist failed - out of memory?");
        EXFAIL_OUT(ret);
    }

    NDRX_LOG(log_debug, "Local Machine/Node [%s] state %s added "
            "curclients=%ld curaccessers=%ld curconv=%ld", 
            mach.lmid, mach.state, mach.curclients, mach.curaccessers, mach.curconv);
    M_idx++;
    
out:
    
    if (NULL!=qlist)
    {
        ndrx_string_list_free(qlist);
    }

    if (EXSUCCEED!=ret)
    {
        ndrx_growlist_free(&M_cursnew->list);
    }

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
