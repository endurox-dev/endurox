/**
 * @brief Servers SRVINFO command from EnduroX server (which reports it's status, etc...)
 *
 * @file cmd_srvinfo.c
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>


#include <ndrstandard.h>
#include <ndebug.h>
#include <userlog.h>
#include <ndrxd.h>
#include <ndrxdcmn.h>
#include <atmi_shm.h>

#include <bridge_int.h>

#include "cmd_processor.h"
#include "utlist.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Process Server Info request.
 * No response required, this is just for info!
 * @param args
 * @return SUCCEED?
 */
expublic int cmd_srvinfo (command_call_t * call, char *data, size_t len, int context)
{
    int ret=EXSUCCEED;
    srv_status_t * srvinfo = (srv_status_t *)call;
    pm_node_t * p_pm;
    pm_node_t * p_pm_chk;
    pm_pidhash_t *pm_pid;
    int i;
    int srvid;
    /* So what we should do here?
     * 1. Mark that service is running.
     * 2. Populate server's linked list with advertised services.
     *
     * We expect to get this message in case if we start up or we re-attach to
     * existing runtime where ndrxd where exit dead.
     */

    /* refresh the state */
    NDRX_LOG(log_debug, "Call from server: %d, state: %d",
                            srvinfo->srvinfo.srvid, srvinfo->srvinfo.state);
    
    srvid=srvinfo->srvinfo.srvid;
    if (srvid>=0 && srvid<ndrx_get_G_atmi_env()->max_servers
        /* Fix for BSD Bug #129 - seems that in some test cases we might get
         * late messages from exiting queue or processes which only now boots from
         * previous test case
         * thus drop such messages, if PM is not loaded
         */
        && NULL!=G_process_model_hash)
    {
        p_pm = G_process_model_hash[srvid];
    }
    else
    {
        NDRX_LOG(log_error, "Corrupted server id: %d - ignore request", 
                srvid);
        goto out;
    }
    
    if (NULL==p_pm)
    {
        NDRX_LOG(log_error, "Unknown server id %d - ignore request!", 
                        srvinfo->srvinfo.srvid);
        goto out;
    }
    
    /* TODO: If server was shutdown, but we got call, then update PID! */
    
    /* crosscheck PID?  - if we start process in manual mode, then this does not matter!
     * better update PID!
     * In case if booted from script too, then PID shall be used the one the
     * server reports and not the actual one with which server was booted.
     */
    pm_pid =pid_hash_get(G_process_model_pid_hash, srvinfo->srvinfo.pid);

    if (NULL!=pm_pid)
    {
        p_pm_chk = pm_pid->p_pm;
        if (srvinfo->srvinfo.srvid!=p_pm_chk->srvid)
        {
            NDRX_LOG(log_error, "Got message from pid=%d, srvid=%d but not "
                                "match for server id %d for same PID in system!!!",
                                srvinfo->srvinfo.pid, srvinfo->srvinfo.srvid, 
                                p_pm_chk->srvid);   
            goto out;
        }
        
        /* Check the state of original of current process */
        if (NDRXD_PM_STARTING == p_pm_chk->state)
        {
            NDRX_LOG(log_warn, "Binary was starting up, updating status");
            
            NDRX_STRCPY_SAFE(p_pm->binary_name_real, 
                    srvinfo->srvinfo.binary_name_real);
            p_pm->svpid = srvinfo->srvinfo.svpid; /* save real pid */
            
            p_pm->state = srvinfo->srvinfo.state;
            p_pm->state_changed = SANITY_CNT_START;
            /* Assume we inter in running state, thus reset ping timer */
            p_pm->pingtimer = SANITY_CNT_START; /* reset rsp timer */
            p_pm->rspstwatch = SANITY_CNT_START;
            p_pm->pingstwatch = SANITY_CNT_IDLE;
            p_pm->killreq = EXFALSE;
            
            p_pm->exec_seq_try = 0; /* Reset counter as we are good */
            p_pm->flags = srvinfo->srvinfo.flags; /* save flags */
            p_pm->nodeid = srvinfo->srvinfo.nodeid; /* Save node id */
            
        }
        else if (p_pm_chk->state != NDRXD_PM_RUNNING_OK)
        {
            NDRX_LOG(log_warn, "We assume that this PID is good "
                    "one & will remove old one!");
            
            NDRX_STRCPY_SAFE(p_pm->binary_name_real, 
                    srvinfo->srvinfo.binary_name_real);
            p_pm->svpid = srvinfo->srvinfo.svpid; /* save real pid */
            
            p_pm->state = srvinfo->srvinfo.state;
            p_pm->state_changed = SANITY_CNT_START;
            /* Assume we inter in running state, thus reset ping timer */
            p_pm->pingtimer = SANITY_CNT_START;
            p_pm->rspstwatch = SANITY_CNT_START; /* reset rsp timer */
            p_pm->pingstwatch = SANITY_CNT_IDLE;
            p_pm->killreq = EXFALSE;
            
            remove_startfail_process(p_pm_chk, NULL, NULL);
            /* reset shared memory! */
            ndrxd_shm_resetsrv(p_pm_chk->srvid);
            /* Now install stuff in pidhash */
            
            p_pm->pid = srvinfo->srvinfo.pid;
            add_to_pid_hash(G_process_model_pid_hash, p_pm);
            p_pm->exec_seq_try = 0;  /* Reset counter as we are good */
            /* Bridge stuff: */
            p_pm->flags = srvinfo->srvinfo.flags; /* save flags */
            p_pm->nodeid = srvinfo->srvinfo.nodeid; /* Save node id */
            
        }
        else
        {
            NDRX_LOG(log_warn, "Existing server already runs OK "
                    "this is some mistake!");
            /* TODO: We could referesh service list! */
            goto out;
        }
    }
    else
    {
        /* This could be external process (not started by ndrxd)! - 
         * so we register it in PID list */
        NDRX_LOG(log_warn, "Seems like server is not started by ndrxd! "
                "The pid %d was not found in PID hash", srvinfo->srvinfo.pid);
        
        
        NDRX_STRCPY_SAFE(p_pm->binary_name_real, 
                    srvinfo->srvinfo.binary_name_real);
        p_pm->svpid = srvinfo->srvinfo.svpid; /* save real pid */
        
        p_pm->pid = srvinfo->srvinfo.pid;
        p_pm->state = srvinfo->srvinfo.state;
        p_pm->reqstate = NDRXD_PM_RUNNING_OK;/* If process is now running, 
                                              * then assume we want it keep running! */
        p_pm->killreq = EXFALSE;
        
        /* reset ping timer */
        p_pm->pingtimer = SANITY_CNT_START;
        p_pm->rspstwatch = SANITY_CNT_START; /* restart rsp timer */
        p_pm->pingstwatch = SANITY_CNT_IDLE;
        
        /* Bridge stuff: */
        p_pm->flags = srvinfo->srvinfo.flags; /* save flags */
        p_pm->nodeid = srvinfo->srvinfo.nodeid; /* Save node id */
        
        add_to_pid_hash(G_process_model_pid_hash, p_pm);
    }
    
    /* Load the list of the services into structures? */
    brd_begin_diff();
    for (i=0; i<srvinfo->svc_count; i++)
    {
        /* OK, now populate the stuff stuff, hmm we migth update 
         * if there is already entry (if we request this on runtime?!?!)
         */
        pm_node_svc_t *svc_info = (pm_node_svc_t *)NDRX_MALLOC(sizeof(pm_node_svc_t));
        memset((char *)svc_info, 0, sizeof(svc_info));
        if (NULL==svc_info)
        {
            NDRXD_set_error_fmt(NDRXD_EOS, "Failed to allocate pm_node_svc_t(%d)!",
                                            sizeof(pm_node_svc_t));
            ret=EXFAIL;
            goto out;
        }

        /* initialize service info sent from server. */
        svc_info->svc = srvinfo->svcs[i];
        NDRX_LOG(log_debug, "Server [%s] advertises: svc: [%s] func: [%s]",
                                p_pm->binary_name, svc_info->svc.svc_nm, 
                svc_info->svc.fn_nm);
        /* Add this to the list */
        DL_APPEND(p_pm->svcs, svc_info);
        /* add stuff to bridge service hash */
        if (EXSUCCEED!=brd_add_svc_to_hash(svc_info->svc.svc_nm))
        {
            ret=EXFAIL;
            goto out;
        }
    }
    
    if (srvinfo->srvinfo.flags & SRV_KEY_FLAGS_BRIDGE)
    {
        NDRX_LOG(log_debug, "Server is bridge - update bridge table");
        if (EXSUCCEED!=brd_addupd_bridge(srvinfo))
        {
            ret=EXFAIL;
            goto out;
        }
    }
    brd_end_diff();
    /*
    p_pm->state = NDRXD_PM_RUNNING_OK;
    */
    /* TODO: Put server by it self in shm (initialize stats, etc...?) */
    
out:
    return ret;
}
/* vim: set ts=4 sw=4 et smartindent: */
