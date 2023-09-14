/**
 * @brief Respawning dead server processes.
 *
 * @file respawn.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2023, Mavimax, Ltd. All Rights Reserved.
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
#include <lcfint.h>
#include <singlegrp.h>

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
 * 
 * Forbid running function in restart mode, respawns with wait, can cause
 * that, and that can break the logic of sequential order startups (required
 * for singleton group boots at failover).
 * 
 * @return 
 */
expublic int do_respawn_check(void)
 {
    int ret=EXSUCCEED;
    pm_node_t *p_pm;
    long delta;
    int abort = EXFALSE;
    int nrgrps = ndrx_G_libnstd_cfg.pgmax;
    int sg_groups[nrgrps];
    static int into_respawn = EXFALSE;
    int we_run = EXFALSE;
    int singleton_attempt;

    /* No sanity checks while app config not loaded */
    if (NULL==G_app_config)
    {
        goto out;
    }

    if (into_respawn)
    {
        NDRX_LOG(log_debug, "do_respawn_check: recursive call, bypass the run!");
        goto out;
    }

    we_run=EXTRUE;
    into_respawn=EXTRUE;

    NDRX_LOG(6, "Time for respawning checking...");

    /* use snapshoo checks here... */
    ndrx_sg_get_lock_snapshoot(sg_groups, &nrgrps, 0);

    DL_FOREACH(G_process_model, p_pm)
    {
        NDRX_LOG(6, "Proc: %s, Reqstate %d, curstate %d",
            p_pm->binary_name, p_pm->reqstate, p_pm->state);

        if ((NDRXD_PM_RUNNING_OK==p_pm->reqstate || NDRXD_PM_RESTART==p_pm->reqstate) 
                && PM_NOT_RUNNING(p_pm->state))
        {
            if (NDRXD_PM_RESTART==p_pm->reqstate)
            {
                NDRX_LOG(log_warn, "Proc: %s (Srvid: %d), Reqstate %d, curstate %d "
                    "starting as restart was requested",
                    p_pm->binary_name, p_pm->srvid, p_pm->reqstate, p_pm->state);
                p_pm->reqstate=NDRXD_PM_RUNNING_OK;
            }
            else if (!p_pm->conf->respawn)
            {
                NDRX_LOG(6, "respawn param is off -> continue with next...");
                continue;
            }

            if ( NDRXD_PM_WAIT==p_pm->state 
                && ndrx_ndrxconf_procgroups_is_singleton(G_app_config->procgroups, p_pm->conf->procgrp_no)
                && p_pm->conf->procgrp_no > 0)
            {
                singleton_attempt=EXTRUE;
            }
            else
            {
                singleton_attempt=EXFALSE;
            }

            /*
            delta = p_pm->rspstwatch;
            */
            delta = p_pm->state_changed;

            NDRX_LOG(6, "Respawn delta: %ld singleton_attempt: %d", delta, singleton_attempt);

            /* Check is it time for startup? Note that exec_seq_try is incremented prior respawn
             * check. Thus the first cycle of sanity is without try increment.
             */
            if ( (p_pm->exec_seq_try<=1 && delta >= G_app_config->restart_min)
		    || (p_pm->exec_seq_try>1 
                           && delta >= (G_app_config->restart_min+(p_pm->exec_seq_try-1)*G_app_config->restart_step))
                    || (delta >= G_app_config->restart_max)
                    || singleton_attempt)
            {
                long processes_started=0;
                long schedule_next;
                int do_wait = EXFALSE;

                /* 
                 * If process is in group and previous state was "wait", then we shall wait for process response
                 * (if it is bootable, checked by start_process), to keep the order of booting.
                 */
                if ( singleton_attempt
                    && !(NDRX_SG_NO_ORDER & sg_groups[p_pm->conf->procgrp_no-1]) )
                {
                    /* Ordering required, lets wait */
                    do_wait = EXTRUE;
                }

                NDRX_LOG(log_warn, "Respawning server: srvid: %d,"
                        " name: [%s], seq try: %d, already not running: %d secs, singleton_attempt: %d, do_wait: %d",
                        p_pm->srvid, p_pm->binary_name, p_pm->exec_seq_try, delta, singleton_attempt, do_wait);

                /* if not doing singleton_attempt, then it is respawn... */
                start_process(NULL, p_pm, NULL, &processes_started, do_wait, &abort, sg_groups, !singleton_attempt);

                /***Just for info***/
                schedule_next = G_app_config->restart_min+p_pm->exec_seq_try*G_app_config->restart_step;
                if (schedule_next>G_app_config->restart_max)
                    schedule_next = G_app_config->restart_max;

                NDRX_LOG(log_warn, "next try after: %d sty",
                        schedule_next);
            }
        }

    }/*DL_FOREACH*/

    /* set the boot flag */
    ndrx_mark_singlegrp_srv_booted(nrgrps, sg_groups);

out:

    /* restore state as runnable.. */
    if (we_run)
    {
        into_respawn=EXFALSE;
    }

    return ret;
}


/* vim: set ts=4 sw=4 et smartindent: */
