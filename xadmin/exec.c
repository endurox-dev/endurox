/* 
 * @brief Routines for checking & starting daemon process
 *
 * @file exec.c
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
#include <memory.h>
#include <errno.h>
#include <fcntl.h>

#include <sys_mqueue.h>
#include <sys/param.h>
#include <sys_mqueue.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

#include <ndrstandard.h>
#include <ndebug.h>

#include "ndrx.h"
#include <atmi_int.h>
#include <sys_unix.h>
#include <userlog.h>
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
/*    NDRX_LOG(log_warn, "Got sigchld..."); - debug might cause locking?*/

    memset(&rusage, 0, sizeof(rusage));

    if (0!=(chldpid = wait3(&stat_loc, WNOHANG|WUNTRACED, &rusage)))
    {
/*        NDRX_LOG(log_warn, "sigchld: PID: %d exit status: %d",
                                           chldpid, stat_loc); - this too 
        can cause some lockups... */

        /* TODO: If this is last pid, then set state in idle? */
        G_config.ndrxd_stat=NDRXD_STAT_NOT_STARTED;

    }
    else
    {
/*        NDRX_LOG(log_error, "Got sigchild for unknown"); */
    }
}

/**
 * Check weither idel instance running?
 * @return FALSE - not running
 *         TRUE - running or malfunction.
 */
expublic int is_ndrxd_running(void)
{
    int ret = EXFALSE;
    int queue_ok = EXFALSE;
    FILE *f = NULL;
    pid_t    pid;
    char    pidbuf[64] = {EXEOS};

    /* Reset to default - not running! */
    G_config.ndrxd_stat = NDRXD_STAT_NOT_STARTED;

    /* Check queue first  */
    if ((mqd_t)EXFAIL==G_config.ndrxd_q)
        G_config.ndrxd_q = ndrx_mq_open_at_wrp (G_config.ndrxd_q_str, O_WRONLY);

    if ((mqd_t)EXFAIL==G_config.ndrxd_q)
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
    if (NULL==(f=NDRX_FOPEN(G_config.pid_file, "r")))
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

    NDRX_FCLOSE(f);
    f = NULL;

    pid = atoi(pidbuf);

    if (ndrx_sys_is_process_running(pid, "ndrxd"))
    {
        if ((mqd_t)EXFAIL!=G_config.ndrxd_q)
        {
            ret=EXTRUE;
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
        NDRX_FCLOSE(f);

    if (!ret)
    {
        fprintf(stderr, "Enduro/X back-end (ndrxd) is not running\n");
        if ((mqd_t)EXFAIL!=G_config.ndrxd_q)
        {
            ndrx_mq_close(G_config.ndrxd_q);
            G_config.ndrxd_q = (mqd_t)EXFAIL;
            
            if (ndrx_chk_ndrxd())
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
                ndrx_mq_unlink(G_config.ndrxd_q_str);
            }
        }
    }
    return ret;
}

/**
 * Start idle instance of daemon process
 * @return
 */
expublic int start_daemon_idle(void)
{
    int ret=EXSUCCEED;
    pid_t pid;
    char    key[NDRX_MAX_KEY_SIZE+3+1];
    /* clone our self */
    pid = fork();
    
    if( pid == 0)
    {
        FILE *f;

        /*Bug #176 close resources */
        if (G_config.ndrxd_q != (mqd_t)EXFAIL)
            ndrx_mq_close(G_config.ndrxd_q);

        if (G_config.reply_queue != (mqd_t)EXFAIL)
            ndrx_mq_close(G_config.reply_queue);

        /* this is child - start EnduroX back-end*/
        snprintf(key, sizeof(key), NDRX_KEY_FMT, ndrx_get_G_atmi_env()->rnd_key);
        char *cmd[] = { "ndrxd", key, (char *)0 };

        /* Open log file */
        if (NULL==(f=NDRX_FOPEN(G_config.ndrxd_logfile, "a")))
        {
            fprintf(stderr, "Failed to open ndrxd log file: %s\n",
                    G_config.ndrxd_logfile);
        }
        else
        {
		
	    /* Bug #176 */
            if (EXSUCCEED!=fcntl(fileno(f), F_SETFD, FD_CLOEXEC))
            {
                userlog("WARNING: Failed to set FD_CLOEXEC: %s", strerror(errno));
            }
	    
            /* Redirect stdout, stderr to log file */
            close(1);
            close(2);
            if (EXFAIL==dup(fileno(f)))
            {
                userlog("%s: Failed to dup(1): %s", __func__, strerror(errno));
            }

            if (EXFAIL==dup(fileno(f)))
            {
                userlog("%s: Failed to dup(2): %s", __func__, strerror(errno));
            }
        }


        if (EXSUCCEED != execvp ("ndrxd", cmd))
        {
            fprintf(stderr, "Failed to start server - ndrxd!\n");
            exit(1);
        }
    }
    else
    {
        int i;
        int started=EXFALSE;
        /* this is parent for child, wait 1 sec  */
        sleep(1);
        started=is_ndrxd_running();

#define MAX_WSLEEP	5
	/* give another 5 seconds... to start ndrxd */
	if (!started)
	{
        	for (i=0; i<MAX_WSLEEP; i++)
        	{
			fprintf(stderr, ">>> still not started, waiting %d/%d\n",
                                    i, MAX_WSLEEP);
            		sleep(1);
            		started=is_ndrxd_running();
            		if (started)
                		break;
		}
        }

        if (started)
        {
            fprintf(stderr, ">>> ndrxd idle instance started.\n");
            G_config.is_idle = EXTRUE;
        }
        else if (NDRXD_STAT_NOT_STARTED==G_config.ndrxd_stat)
        {
            fprintf(stderr, ">>> ndrxd idle instance not started (something failed?)!\n");
        }
        else
        {
            fprintf(stderr, ">>> ndrxd instance idle malfunction!\n");
        }
    }
    
    return ret;
}

