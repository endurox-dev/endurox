/**
 * @brief Respawning dead server processes.
 *
 * @file respawn.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or Mavimax's license for commercial use.
 * -----------------------------------------------------------------------------
 * GPL license:
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * -----------------------------------------------------------------------------
 * A commercial use license is available from Mavimax, Ltd
 * contact@mavimax.com
 * -----------------------------------------------------------------------------
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <memory.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>

#include <utlist.h>

#include <ndrstandard.h>
#include <ndrxd.h>
#include <atmi_int.h>
#include <nstopwatch.h>

#include <ndebug.h>
#include <cmd_processor.h>
#include <signal.h>

#include "userlog.h"

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * This will check the list of dead processes and will try to start them up
 * if the wait time have been reached.
 * @return 
 */
expublic int do_respawn_check(void)
 {
    int ret=EXSUCCEED;
    pm_node_t *p_pm;
    long delta;
    int wasrun = EXFALSE;
    int abort = EXFALSE;
    
    /* No sanity checks while app config not loaded */
    if (NULL==G_app_config)
        goto out;
        
    NDRX_LOG(6, "Time for respawning checking...");

    DL_FOREACH(G_process_model, p_pm)
    {
        NDRX_LOG(6, "Proc: %s, Reqstate %d, curstate %d", 
		 p_pm->binary_name, p_pm->reqstate, p_pm->state);
        if (NDRXD_PM_RUNNING_OK==p_pm->reqstate && 
                ( NDRXD_PM_DIED==p_pm->state 
                ||NDRXD_PM_EXIT==p_pm->state
                ||NDRXD_PM_ENOENT==p_pm->state))
        {
		
	    if (!p_pm->conf->respawn)
	    {
		    NDRX_LOG(6, "respawn param is off -> continue with next...");
		    continue;
	    }
            delta = p_pm->rspstwatch;
            NDRX_LOG(6, "Respawn delta: %ld", delta);
            
            /* Check is it time for startup? */
            if ( delta > G_app_config->restart_min+p_pm->exec_seq_try*G_app_config->restart_step ||
                    delta > G_app_config->restart_max)
            {
                long processes_started=0;
                long schedule_next;
                NDRX_LOG(log_warn, "Respawning server: srvid: %d,"
                        " name: [%s], seq try: %d, already not running: %d secs",
                        p_pm->srvid, p_pm->binary_name, p_pm->exec_seq_try, delta);
                start_process(NULL, p_pm, NULL, &processes_started, EXTRUE, &abort);

                /***Just for info***/
                schedule_next = G_app_config->restart_min+p_pm->exec_seq_try*G_app_config->restart_step;
                if (schedule_next>G_app_config->restart_max)
                    schedule_next = G_app_config->restart_max;

                NDRX_LOG(log_warn, "next try after: %d sty",
                        schedule_next);
            }
        }
    }/*DL_FOREACH*/     
    
out:

    return ret;
}


/* vim: set ts=4 sw=4 et smartindent: */
