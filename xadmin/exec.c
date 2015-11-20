/* 
** Routines for checking & starting daemon process
**
** @file exec.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, ATR Baltic, SIA. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or ATR Baltic's license for commercial use.
** -----------------------------------------------------------------------------
** GPL license:
** 
** This program is free software; you can redistribute it and/or modify it under
** the terms of the GNU General Public License as published by the Free Software
** Foundation; either version 2 of the License, or (at your option) any later
** version.
**
** This program is distributed in the hope that it will be useful, but WITHOUT ANY
** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
** PARTICULAR PURPOSE. See the GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License along with
** this program; if not, write to the Free Software Foundation, Inc., 59 Temple
** Place, Suite 330, Boston, MA 02111-1307 USA
**
** -----------------------------------------------------------------------------
** A commercial use license is available from ATR Baltic, SIA
** contact@atrbaltic.com
** -----------------------------------------------------------------------------
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <mqueue.h>
#include <sys/param.h>
#include <mqueue.h>
#include <sys/resource.h>
#include <wait.h>
#include <sys/wait.h>

#include <ndrstandard.h>
#include <ndebug.h>

#include "ndrx.h"
#include <atmi_int.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/


/**
 * NDRXD process got sigterm.
 * @return
 */
void sign_chld_handler(int sig)
{
    pid_t chldpid;
    int stat_loc;
    struct rusage rusage;
    NDRX_LOG(log_warn, "Got sigchld...");

    memset(&rusage, 0, sizeof(rusage));

    if (0!=(chldpid = wait3(&stat_loc, WNOHANG|WUNTRACED, &rusage)))
    {
        NDRX_LOG(log_warn, "sigchld: PID: %d exit status: %d",
                                           chldpid, stat_loc);

        /* TODO: If this is last pid, then set state in idle? */
        G_config.ndrxd_stat=NDRXD_STAT_NOT_STARTED;

    }
    else
    {
        NDRX_LOG(log_error, "Got sigchild for unknown");
    }
}

/**
 * Check weither idel instance running?
 * @return FALSE - not running
 *         TRUE - running or malfunction.
 */
public int is_ndrxd_running(void)
{
    int ret = FALSE;
    int queue_ok = FALSE;
    FILE *f = NULL;
    __pid_t    pid;
    char    pidbuf[64] = {EOS};

    /* Reset to default - not running! */
    G_config.ndrxd_stat = NDRXD_STAT_NOT_STARTED;

    /* Check queue first  */
    if (FAIL==G_config.ndrxd_q)
        G_config.ndrxd_q = ndrx_mq_open (G_config.ndrxd_q_str, O_WRONLY);

    if (FAIL==G_config.ndrxd_q)
    {
        NDRX_LOG(log_warn, "Failed to open admin queue [%s]: %s",
                G_config.ndrxd_q_str, strerror(errno));
    }
    
    /*
     * We will check:
     * - Is PID existing
     * - Is the name of the pid correct (check /proc/<PID>/cmdline) - should be ndrxd!
     * - Check for command queue
     */
    if (NULL==(f=fopen(G_config.pid_file, "r")))
    {
        NDRX_LOG(log_error, "Failed to open ndrxd PID file: [%s]: %s",
                G_config.pid_file, strerror(errno));
        
        goto out;
    }

    /* Read the PID */
    if (NULL==fgets(pidbuf, sizeof(pidbuf), f))
    {
        NDRX_LOG(log_error, "Failed to read from PID file: [%s]: %s",
                G_config.pid_file, strerror(errno));
        goto out;
    }

    /* Get pid value */
    fprintf(stderr, "ndrxd PID (from PID file): %s\n", pidbuf);

    fclose(f);
    f = NULL;

    pid = atoi(pidbuf);

    if (is_process_running(pid, "ndrxd"))
    {
        if (FAIL!=G_config.ndrxd_q)
        {
            ret=TRUE;
            G_config.ndrxd_stat = NDRXD_STAT_RUNNING;
        }
        else
        {
            /* NOT OK, malfunction */
            NDRX_LOG(log_error, "`ndrxd' is running, but queue [%s] is "
                    "not open - malfunction!",
                    G_config.ndrxd_q_str);
            G_config.ndrxd_stat = NDRXD_STAT_MALFUNCTION;
        }
    }

out:

    /* close any  */
    if (NULL!=f)
        fclose(f);

    if (!ret)
    {
        fprintf(stderr, "EnduroX back-end (ndrxd) is not running\n");
        if (FAIL!=G_config.ndrxd_q)
        {
            mq_close(G_config.ndrxd_q);
            G_config.ndrxd_q = FAIL;
            
            if (system("ndrxd_chkdown.sh"))
            {
                /* Not sure this is safer, but we will remove that queue on behalf of user!
                 * if process is not running.
                 * Maybe do that if in reality no ndrxd for current user is not running?
                 */
                G_config.ndrxd_stat = NDRXD_STAT_MALFUNCTION;
            }
            else
            {
                /* remove resources as ndrxd is not running */
                fprintf(stderr, "ndrxd is not running - system cleanup\n");
                mq_unlink(G_config.ndrxd_q_str);
            }
        }
    }
    return ret;
}

/**
 * Start idle instance of daemon process
 * @return
 */
public int start_daemon_idle(void)
{
    int ret=SUCCEED;
    __pid_t pid;
    char    key[NDRX_MAX_KEY_SIZE+3+1];
    /* clone our self */
    pid = fork();
    
    if( pid == 0)
    {
        FILE *f;
        /* this is child - start EnduroX back-end*/
        sprintf(key, NDRX_KEY_FMT, G_atmi_env.rnd_key);
        char *cmd[] = { "ndrxd", key, (char *)0 };

        /* Open log file */
        if (NULL==(f=fopen(G_config.ndrxd_logfile, "w+")))
        {
            fprintf(stderr, "Failed to open ndrxd log file: %s\n",
                    G_config.ndrxd_logfile);
        }
        else
        {
            /* Redirect stdout, stderr to log file */
            close(1);
            close(2);
            dup(fileno(f));
            dup(fileno(f));
        }


        if (SUCCEED != execvp ("ndrxd", cmd))
        {
            fprintf(stderr, "Failed to start server - ndrxd!\n");
            exit(1);
        }
    }
    else
    {
        int i;
        int started=FALSE;
        /* this is parent for child, wait 1 sec */
        sleep(1);
        started=is_ndrxd_running();
#if 0
        for (i=0; i<40; i++)
        {
            usleep(50000);
            started=is_ndrxd_running();
            if (started)
                break;
        }
#endif
        if (started)
        {
            fprintf(stderr, "ndrxd idle instance started...\n");
            G_config.is_idle = TRUE;
        }
        else if (NDRXD_STAT_NOT_STARTED==G_config.ndrxd_stat)
        {
            fprintf(stderr, "ndrxd idle instance not started (something failed?)!\n");
        }
        else
        {
            fprintf(stderr, "ndrxd instance idle malfunction!\n");
        }
    }
    
    return ret;
}

