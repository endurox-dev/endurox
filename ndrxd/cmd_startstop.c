/**
 * @brief Start & stop command backend.
 *
 * @file cmd_startstop.c
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

#include "cmd_processor.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Modify reply according the data.
 * @param call
 * @param pm
 */
expublic void reply_mod(command_reply_t *reply, size_t *send_size, mod_param_t *params)
{
    command_reply_pm_t * pm_info = (command_reply_pm_t *)reply;
    pm_node_t *pm = (pm_node_t *)params->mod_param1;

    reply->msg_type = params->param2; /* set specific call type... */
    /* calculate new send size */
    *send_size += (sizeof(command_reply_pm_t) - sizeof(command_reply_t));

    NDRX_STRCPY_SAFE(pm_info->binary_name, pm->binary_name);
    NDRX_STRCPY_SAFE(pm_info->clopt, pm->clopt);
    pm_info->srvid = pm->srvid;
    pm_info->state = pm->state;
    pm_info->pid = pm->pid;

    NDRX_LOG(log_debug, "magic: %ld, pid: %d",
                            pm_info->rply.magic, pm_info->pid);
}

/**
 * Callback to report startup progress
 * @param call
 * @param pm
 * @return
 */
expublic void startup_progress(command_startstop_t * call, pm_node_t *pm, int calltype)
{
    int ret=EXSUCCEED;
    mod_param_t params;

    NDRX_LOG(log_debug, "startup_progress enter, pid: %d", pm->pid);
    memset(&params, 0, sizeof(mod_param_t));

    /* pass to reply process model node */
    params.mod_param1 = (void *)pm;
    params.param2 = calltype;

    if (EXSUCCEED!=simple_command_reply((command_call_t *)call, ret, NDRXD_CALL_FLAGS_RSPHAVE_MORE,
                            /* hook up the reply */
                            &params, reply_mod, 0L, 0, NULL))
    {
        userlog("Failed to send progress back to [%s]", call->call.reply_queue);
    }
    
    NDRX_LOG(log_debug, "startup_progress exit");
}

/**
 * If we failt start the domain, then we have major failure and we should exit.
 * @param args
 * @return
 */
expublic int cmd_start (command_call_t * call, char *data, size_t len, int context)
{
    int ret=EXSUCCEED;
    command_startstop_t *start = (command_startstop_t *)call;
    long processes_started=0;
    
    ret = app_startup(start, startup_progress, &processes_started);

    if (EXSUCCEED!=simple_command_reply(call, ret, 0L, NULL, NULL, 
            processes_started, 0, NULL))
    {
        userlog("Failed to send reply back to [%s]", call->reply_queue);
    }
    
    
    NDRX_LOG(log_warn, "cmd_start returns with status %d", ret);
    
out:
    
    /* Do not want exit ndrxd if failed...! */
    return EXSUCCEED;
}

/**
 * Notification about process...
 * @param args
 * @return
 */
expublic int cmd_notify (command_call_t * call, char *data, size_t len, int context)
{
    int ret=EXSUCCEED;
    srv_status_t * pm_status = (srv_status_t *)call;
    pm_pidhash_t *pm_pid;
    pm_node_t *p_pm=NULL;
    shm_srvinfo_t* srv;
    NDRX_LOG(log_warn, "Got notification for pid:%d srvid:%d",
                pm_status->srvinfo.pid, pm_status->srvinfo.srvid);

    /* Now check the pidhash... */
    pm_pid = pid_hash_get(G_process_model_pid_hash, pm_status->srvinfo.pid);

    if (NULL!=pm_pid)
    {
        int srvid = pm_pid->p_pm->srvid;
        
        /* Bug #214 */
        if (NDRXD_PM_EXIT!=pm_status->srvinfo.state)
        {
            userlog("Server process [%s], srvid %hd, pid %d died", 
                pm_pid->p_pm->conf->binary_name, pm_status->srvinfo.srvid, pm_pid->pid);
        }
                
        /* If we wanted it to stop, but it died, then let it be in exit status
         * which will not try to restart it!
         */
        if (pm_pid->p_pm->state!=NDRXD_PM_STOPPING)
        {
            /* try to use status from shared memory if available */
            srv=ndrxd_shm_getsrv(pm_pid->p_pm->srvid);
            
            if (srv->execerr!=0)
            {
                pm_pid->p_pm->state = srv->execerr;
            }
            else
            {
                pm_pid->p_pm->state = pm_status->srvinfo.state;
            }
        }
        else
        {
            /* so that we do no try again to wake it up! */
            pm_pid->p_pm->state=NDRXD_PM_EXIT;
        }
        
        NDRX_LOG(log_warn, "Removing resources allocated "
                            "for process [%s]", pm_pid->p_pm->binary_name);
        
        /* Find .the pm_p & remove failed process! */

	/* TODO: If the PID if different one than for srvid, then we remove 
         * this thing from
         * pidhash only!!!! - Yeah right this must be fixed.
         * we had an incident in i2nc, when PID 9862 died, other was started, 
         * but we remove that other, not 9862, which died actually!!!
         */
        remove_startfail_process(get_pm_from_srvid(srvid), NULL, pm_pid);

        /* reset shared memory! */
        ndrxd_shm_resetsrv(srvid);
        
    }
    else
    {
        NDRX_LOG(log_warn, "PID %d unknown to system!",
                                pm_status->srvinfo.pid);
    }
    
    NDRX_LOG(log_warn, "cmd_notify returns with status %d", ret);

out:
    return ret;
}

/**
 * Callback to report startup progress
 * @param call
 * @param pm
 * @return
 */
expublic void shutdown_progress(command_call_t * call, pm_node_t *pm, int calltype)
{
    int ret=EXSUCCEED;
    mod_param_t params;

    NDRX_LOG(log_debug, "shutdown_progress enter");
    memset(&params, 0, sizeof(mod_param_t));

    /* pass to reply process model node */
    params.mod_param1 = (void *)pm;
    params.param2 = calltype;

    if (EXSUCCEED!=simple_command_reply(call, ret, NDRXD_CALL_FLAGS_RSPHAVE_MORE,
                            /* hook up the reply */
                            &params, reply_mod, 0L, 0, NULL))
    {
        userlog("Failed to send progress back to [%s]", call->reply_queue);
    }

    NDRX_LOG(log_debug, "shutdown_progress exit");
}

/**
 * If we failt start the domain, then we have major failure and we should exit.
 * @param args
 * @return
 */
expublic int cmd_stop (command_call_t * call, char *data, size_t len, int context)
{
    int ret=EXSUCCEED;
    command_startstop_t *stop = (command_startstop_t *)call;
    long processes_shutdown=0;
    
    ret = app_shutdown(stop, shutdown_progress, &processes_shutdown);

    if (stop->complete_shutdown)
    {
        NDRX_LOG(log_warn, "Unlinking pid-file before shutdown down reply...");
        ndrxd_unlink_pid_file(EXFALSE);
    }
    
    if (EXSUCCEED!=simple_command_reply(call, ret, 0L, NULL, NULL, 
            processes_shutdown, 0, NULL))
    {
        userlog("Failed to send reply back to [%s]", call->reply_queue);
    }

    NDRX_LOG(log_warn, "cmd_start returns with status %d", ret);

    /* We should request app domain shutdown (if it was full shutdown?)
     * ndrxd main will hang running while there still will be processes, but
     * shutdown was requested....
     */
    if (stop->complete_shutdown)
    {
        NDRX_LOG(log_warn, "Putting system in shutdown state...");
        G_sys_config.stat_flags |= NDRXD_STATE_SHUTDOWN;
    }
    
out:
    /* Do not want exit if failed..! */
    return EXSUCCEED;
}



/**
 * Simple reply on abort.
 * @param call
 * @param data
 * @param len
 * @param context
 * @return 
 */
expublic int cmd_abort (command_call_t * call, char *data, size_t len, int context)
{
    int ret=EXSUCCEED;

    if (EXSUCCEED!=simple_command_reply(call, ret, 0L, NULL, NULL, 0L, 0, NULL))
    {
        userlog("Failed to send reply back to [%s]", call->reply_queue);
    }

    /* Do not want exit if failed..! */
    return EXSUCCEED; /* Do not want to break the system! */
}

/**
 * Reload services...
 * @param args
 * @return
 */
expublic int cmd_sreload (command_call_t * call, char *data, size_t len, int context)
{
    int ret=EXSUCCEED;
    command_startstop_t *start = (command_startstop_t *)call;
    long processes_started=0;
    
    ret = app_sreload(start, startup_progress, shutdown_progress, &processes_started);

    if (EXSUCCEED!=simple_command_reply(call, ret, 0L, NULL, NULL, 
            processes_started, 0, NULL))
    {
        userlog("Failed to send reply back to [%s]", call->reply_queue);
    }
    
    
    NDRX_LOG(log_warn, "cmd_start returns with status %d", ret);
    
out:
    
    /* Do not want exit ndrxd if failed...! */
    return EXSUCCEED;
}

/**
 * Reload services...  (internal version
 * @param args
 * @return
 */
expublic int cmd_sreloadi (command_call_t * call, char *data, size_t len, int context)
{
    int ret=EXSUCCEED;
    command_startstop_t *start = (command_startstop_t *)call;
    long processes_started=0;
    
    ret = app_sreload(start, NULL, NULL, &processes_started);
    
    NDRX_LOG(log_warn, "cmd_start returns with status %d", ret);
    
out:
    
    /* Do not want exit ndrxd if failed...! */
    return EXSUCCEED;
}

/* vim: set ts=4 sw=4 et smartindent: */
