/* 
** Execute client processes (start, stop and signal handling...)
**
** @file cltexec.c
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
#include <libxml/xmlreader.h>
#include <errno.h>
#include <unistd.h>
#include <ndrstandard.h>
#include <userlog.h>
#include <atmi.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>

#include <sys/param.h>
#include <sys_mqueue.h>
#include <sys/resource.h>
#include <sys/wait.h>


#include "cpmsrv.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Handle the child signal
 * @return
 */
public void sign_chld_handler(int sig)
{
    pid_t chldpid;
    int stat_loc;
    struct rusage rusage;

    memset(&rusage, 0, sizeof(rusage));

    while (0<(chldpid = wait3(&stat_loc, WNOHANG|WUNTRACED, &rusage)))
    {
        /* - no debug please... Can cause lockups...
        NDRX_LOG(log_warn, "sigchld: PID: %d exit status: %d",
                                           chldpid, stat_loc);
        */
        
        /* Search for the client & mark it as dead */
        
        cpm_process_t * c = cpm_get_client_by_pid(chldpid);
        
        if (NULL!=c)
        {
            c->dyn.cur_state = CLT_STATE_NOTRUN;
            c->dyn.exit_status = stat_loc;
            /* Set status change time */
            cpm_set_cur_time(c);
        }
        
    }

    /*signal(SIGCHLD, sign_chld_handler);*/
}


public int cpm_killall(void)
{
    int ret = SUCCEED;
    cpm_process_t *c = NULL;
    cpm_process_t *ct = NULL;
    int is_any_running;
    ndrx_timer_t t;
    char *sig_str[3]={"SIGINT","SIGTERM", "SIGKILL"};
    int sig[3]={SIGINT,SIGTERM, SIGKILL};
    int i;
    
    for (i=0; i<3; i++)
    {

    NDRX_LOG(log_warn, "Terminating all with %s", sig_str[i]);
    
    EXHASH_ITER(hh, G_clt_config, c, ct)
    {
        if (CLT_STATE_STARTED==c->dyn.cur_state)
        {
            NDRX_LOG(log_warn, "Killing: %s/%s/%d with %s",
		c->tag, c->subsect, c->dyn.pid, sig_str[i]);
            kill(c->dyn.pid, sig[i]);
        }
    }
    
if (i<2) /*no wait for killl... */
{
    is_any_running = FALSE;
    ndrx_timer_reset(&t);
    do
    {
        sign_chld_handler(0);
        
        EXHASH_ITER(hh, G_clt_config, c, ct)
        {
            if (CLT_STATE_STARTED==c->dyn.cur_state)
            {
                is_any_running = TRUE;
                break;
            }
        }
        
        if (is_any_running)
        {
            usleep(CLT_STEP_INTERVAL_ALL);
        }
    }
    while (is_any_running && ndrx_timer_get_delta_sec(&t) < G_config.kill_interval);
}
    }
    
    NDRX_LOG(log_debug, "cpm_killall done");
    return SUCCEED;
}

/**
 * Kill the process...
 * Firstly -2, then -15, then -9
 * We shall do kill in synchrounus mode.
 * 
 * @param c
 * @return 
 */
public int cpm_kill(cpm_process_t *c)
{
    int ret = SUCCEED;
    ndrx_timer_t t;
    NDRX_LOG(log_warn, "Stopping %s/%s - %s", c->tag, c->subsect, c->stat.command_line);
            
    /* INT interval */
    kill(c->dyn.pid, SIGINT);
    ndrx_timer_reset(&t);
    do
    {
        sign_chld_handler(0);
        if (CLT_STATE_STARTED==c->dyn.cur_state)
        {
            usleep(CLT_STEP_INTERVAL);
        }
    } while (CLT_STATE_STARTED==c->dyn.cur_state && 
            ndrx_timer_get_delta_sec(&t) < G_config.kill_interval);
    
    if (CLT_STATE_STARTED!=c->dyn.cur_state)
        goto out;
    
    NDRX_LOG(log_warn, "%s/%s Did not react on SIGINT, continue with SIGTERM", 
            c->tag, c->subsect);
    
    /* TERM interval */
    kill(c->dyn.pid, SIGTERM);
    ndrx_timer_reset(&t);
    do
    {
        sign_chld_handler(0);
        if (CLT_STATE_STARTED==c->dyn.cur_state)
        {
            usleep(CLT_STEP_INTERVAL);
        }
    } while (CLT_STATE_STARTED==c->dyn.cur_state && 
            ndrx_timer_get_delta_sec(&t) < G_config.kill_interval);
    
    if (CLT_STATE_STARTED!=c->dyn.cur_state)
        goto out;
    
    NDRX_LOG(log_warn, "%s/%s Did not react on SIGTERM, kill with -9", 
                        c->tag, c->subsect);
    
    /* KILL interval */
    kill(c->dyn.pid, SIGKILL);
    ndrx_timer_reset(&t);
    do
    {
        sign_chld_handler(0);
        if (CLT_STATE_STARTED==c->dyn.cur_state)
        {
            usleep(CLT_STEP_INTERVAL);
      
        }
    }
    while (CLT_STATE_STARTED==c->dyn.cur_state && 
            ndrx_timer_get_delta_sec(&t) < G_config.kill_interval);
    
    if (CLT_STATE_STARTED!=c->dyn.cur_state)
        goto out;
    
    NDRX_LOG(log_warn, "%s/%s Did not react on SIGKILL, giving up...", 
                        c->tag, c->subsect);
    
out:
    return ret;
}

/**
 * Boot the client
 * @param c
 * @return 
 */
public int cpm_exec(cpm_process_t *c)
{
    pid_t pid;
    char cmd_str[PATH_MAX];
    char *cmd[PATH_MAX]; /* splitted pointers.. */
    char separators[]   = " ,\t\n";
    char *token;
    int numargs = 0;
    int fd_stdout;
    int fd_stderr;
    int ret = SUCCEED;

    NDRX_LOG(log_warn, "*********processing for startup %s *********", 
            c->stat.command_line);
    
    c->dyn.was_started = TRUE; /* We tried to start... */
    
    /* clone our self */
    pid = fork();

    if( pid == 0)
    {
        /* some small delay so that parent gets time for PIDhash setup! */
        usleep(9000);

        strcpy(cmd_str, c->stat.command_line);

        token = strtok(cmd_str, separators);
        while( token != NULL )
        {
            cmd[numargs] = token;
            token = strtok( NULL, separators );
            numargs++;
        }
        cmd[numargs] = NULL;
    
        /*  Override environment, if there is such thing */
        if (EOS!=c->stat.env[0])
        {
            if (SUCCEED!=ndrx_load_new_env(c->stat.env))
            {
                userlog("Failed to load custom env from: %s!\n", c->stat.env);
                exit(1);
            }
        }
        
        if (EOS!=c->stat.cctag[0])
        {
            if (SUCCEED!=setenv(NDRX_CCTAG, c->stat.cctag, TRUE))
            {
                userlog("Cannot set [%s] to [%s]: %s\n", 
                        NDRX_CCTAG, c->stat.cctag, strerror(errno));
                exit(1);
            }
        }
        
        /* Change working dir */
        if (EOS!=c->stat.wd[0])
        {
            if (SUCCEED!=chdir(c->stat.wd))
            {
                int err = errno;
                
                NDRX_LOG(log_error, "Failed to change working diretory: %s - %s!\n", 
                        c->stat.wd, strerror(err));
                userlog("Failed to change working diretory: %s - %s!\n", 
                        c->stat.wd, strerror(err));
                exit(1);
            }
        }
        
        /* make stdout go to file */
        if (EOS!=c->stat.log_stdout[0] &&
                FAIL!=(fd_stdout = open(c->stat.log_stdout, 
               O_WRONLY| O_CREAT  | O_APPEND, S_IRUSR | S_IWUSR)))
        {
            dup2(fd_stdout, 1); 
            close(fd_stdout);
        }
        
        if (EOS!=c->stat.log_stderr[0] &&
                FAIL!=(fd_stderr = open(c->stat.log_stderr, 
                O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR)))
        {
            dup2(fd_stderr, 2);   /* make stderr go to file */
            close(fd_stderr);
        }
        
        /* reset signal handlers */
        signal(SIGINT, SIG_DFL);
        signal(SIGTERM, SIG_DFL);
        
        if (SUCCEED != execvp (cmd[0], cmd))
        {
            int err = errno;
            NDRX_LOG(log_error, "Failed to start client, error: %d, %s\n", err, strerror(err));
            exit (err);
        }
    }
    else if (FAIL!=pid)
    {
        cpm_set_cur_time(c);
        c->dyn.pid = pid;
        c->dyn.cur_state = CLT_STATE_STARTED;
    }
    else
    {
        userlog("Failed to fork: %s", strerror(errno));
    }
    
out:
    return ret;
}
