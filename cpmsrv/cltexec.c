/**
 * @brief Execute client processes (start, stop and signal handling...)
 *
 * @file cltexec.c
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
#include <sys/param.h>
#include <sys_mqueue.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <libxml/xmlreader.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include <ndrstandard.h>
#include <userlog.h>
#include <atmi.h>
#include <exenvapi.h>

#include "cpmsrv.h"
#include "../libatmisrv/srv_int.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

/**
 * Support #459 
 * locked debug use signal-thread
 * Fact is if we forking, we might get lock on localtime_r
 * by signal thread, and the forked process will try to debug
 * but will be unable because there will be left over lock on localtime_r
 * from the signal thread which no more exists after the fork.
 */
#define LOCKED_DEBUG(lev, fmt, ...) MUTEX_LOCK_V(M_forklock);\
                                    NDRX_LOG(lev, fmt, ##__VA_ARGS__);\
                                    MUTEX_UNLOCK_V(M_forklock);

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
exprivate pthread_t M_signal_thread; /* Signalled thread */
exprivate int M_signal_thread_set = EXFALSE; /* Signal thread is set */
exprivate MUTEX_LOCKDECL(M_forklock);       /**< forking lock, no q ops during fork!  */
/*---------------------------Prototypes---------------------------------*/

#if EX_CPM_NO_THREADS
/**
 * Handle the child signal
 * @return
 */
expublic void sign_chld_handler(int sig)
{
    pid_t chldpid;
    int stat_loc;
    cpm_process_t * c;
    struct rusage rusage;

    memset(&rusage, 0, sizeof(rusage));

    NDRX_LOG(log_debug, "About to wait3()");
    while (0<(chldpid = wait3(&stat_loc, WNOHANG|WUNTRACED, &rusage)))
    {
        /* - no debug please... Can cause lockups... - not using singlals thus no tproblem... */
        NDRX_LOG(log_warn, "sigchld: PID: %d exit status: %d",
                                           chldpid, stat_loc);
        
        /* Search for the client & mark it as dead */
        cpm_lock_config();
        
        c = cpm_get_client_by_pid(chldpid);
        
        if (NULL!=c)
        {
            c->dyn.cur_state = CLT_STATE_NOTRUN;
            c->dyn.exit_status = stat_loc;
            /* Set status change time */
            cpm_set_cur_time(c);
            
            /* update shared memory to stopped... */
            ndrx_cltshm_setpos(c->key, EXFAIL, NDRX_CPM_MAP_WASUSED, NULL);
            
        }
        else
        {
            NDRX_LOG(log_error, "PID not found %d in client registry!",chldpid);
        }

        cpm_unlock_config(); /* we are done... */
        
    }

    /*signal(SIGCHLD, sign_chld_handler);*/
}

#else

/**
 * Checks for child exit.
 * We will let mainthread to do all internal struct related work!
 * @return Got child exit
 */
exprivate void * check_child_exit(void *arg)
{
    pid_t chldpid;
    int stat_loc;
    sigset_t blockMask;
    int sig;
    struct rusage rusage;
        
    sigemptyset(&blockMask);
    sigaddset(&blockMask, SIGCHLD);

    if (pthread_sigmask(SIG_BLOCK, &blockMask, NULL) == -1)
    {
        LOCKED_DEBUG(log_always, "%s: pthread_sigmask failed (thread): %s",
            __func__, strerror(errno));
    }
    
    LOCKED_DEBUG(log_debug, "check_child_exit - enter...");
    for (;;)
    {
        int got_something = 0;

        /* macos may return form sigwait with out carrying any singal
         */
        LOCKED_DEBUG(log_debug, "about to sigwait()");
        sig=0;
        do
        {
            if (EXSUCCEED!=sigwait(&blockMask, &sig))         /* Wait for notification signal */
            {
                LOCKED_DEBUG(log_warn, "sigwait failed:(%s)", strerror(errno));

            }
        } while (0==sig);
        
        LOCKED_DEBUG(log_debug, "about to wait()");
        /*
        while ((chldpid = wait(&stat_loc)) >= 0)
         */  
        while ((chldpid = wait3(&stat_loc, WNOHANG|WUNTRACED, &rusage)) > 0)
        {
            /* Bug #108 01/04/2015, mvitolin
             * If config file is changed by foreground thread in this time,
             * then we must synchronize with them.
             */
            cpm_lock_config();
            
            cpm_process_t * c = cpm_get_client_by_pid(chldpid);
            got_something++;
                   
            if (NULL!=c)
            {
                c->dyn.cur_state = CLT_STATE_NOTRUN;
                
                
                /* these bellow use some debug and time funcs
                 * thus lock all
                 */
                MUTEX_LOCK_V(M_forklock);
                /* update shared memory to stopped... */
                ndrx_cltshm_setpos(c->key, EXFAIL, NDRX_CPM_MAP_WASUSED, NULL);
                
                c->dyn.exit_status = stat_loc;
                /* Set status change time */
                /* have some lock due to time use... */
                cpm_set_cur_time(c);
                MUTEX_UNLOCK_V(M_forklock);
            }
            
            cpm_unlock_config(); /* we are done... */
            
        }
    }
   
/*
    - not reached.
    NDRX_LOG(log_debug, "check_child_exit: %s", strerror(errno));
    return NULL;
*/
}

/**
 * Initialize polling lib
 * not thread safe.
 * @return
 */
expublic void cpm_sigchld_init(void)
{
    pthread_attr_t pthread_custom_attr;
    pthread_attr_t pthread_custom_attr_dog;
    struct sigaction sa; /* Seem on AIX signal might slip.. */
    char *fn = "ndrxd_sigchld_init";

    NDRX_LOG(log_debug, "%s - enter", fn);
    
    /* our friend AIX, might just ignore the SIG_BLOCK and raise signal
     * Thus we will handle the stuff in as it was in Enduro/X 2.5
     */
    
#if 0
    /* sa.sa_handler = sign_chld_handler; they are blocked... */
    sigemptyset (&sa.sa_mask);
    sa.sa_flags = SA_RESTART; /* restart system calls please... */
    sigaction (SIGCHLD, &sa, 0);
#endif
    
    pthread_attr_init(&pthread_custom_attr);
    pthread_attr_init(&pthread_custom_attr_dog);
    
    /* set some small stacks size, 1M should be fine! */
    /* pthread_attr_setstacksize(&pthread_custom_attr, 2048*1024); */
    
    ndrx_platf_stack_set(&pthread_custom_attr);
    
    pthread_create(&M_signal_thread, &pthread_custom_attr, 
            check_child_exit, NULL);

    M_signal_thread_set = EXTRUE;
}
#endif


/**
 * Un-initialize sigchild monitor thread
 * @return
 */
expublic void cpm_sigchld_uninit(void)
{
    char *fn = "ndrxd_sigchld_uninit";

    NDRX_LOG(log_debug, "%s - enter", fn);
    
    if (!M_signal_thread_set)
    {
        NDRX_LOG(log_debug, "Signal thread was not initialized, nothing to do...");
        goto out;
    }

    NDRX_LOG(log_debug, "About to cancel signal thread");
    
    /* TODO: have a counter for number of sets, so that we can do 
     * un-init...
     */
    if (EXSUCCEED!=pthread_cancel(M_signal_thread))
    {
        NDRX_LOG(log_error, "Failed to kill poll signal thread: %s", strerror(errno));
    }
    else
    {
        void * res = EXSUCCEED;
        if (EXSUCCEED!=pthread_join(M_signal_thread, &res))
        {
            NDRX_LOG(log_error, "Failed to join pthread_join() signal thread: %s", 
                    strerror(errno));
        }

        if (res == PTHREAD_CANCELED)
        {
            NDRX_LOG(log_info, "Signal thread canceled ok!")
        }
        else
        {
            NDRX_LOG(log_info, "Signal thread failed to cancel "
                    "(should not happen!!)");
        }
    }
    
    M_signal_thread_set = EXFALSE;
    NDRX_LOG(log_debug, "finished ok");
out:
    return;
}

/**
 * Perform test on PID
 * @param c client process
 */
expublic void cpm_pidtest(cpm_process_t *c)
{
    if (CLT_STATE_STARTED==c->dyn.cur_state && c->dyn.shm_read)
    {
        /* check the pid status, as we might be booted with existing
         * shared memory, thus we do not get any sig childs...
         * if we requested the stop, assume exit ok
         * if not requested, assume died
         */
        if (!ndrx_sys_is_process_running_by_pid(c->dyn.pid))
        {
            NDRX_LOG(log_info, "Process [%s]/%d exited by pid test",
                    c->stat.command_line, (int)c->dyn.pid);
            /* update shared memory to stopped... */
            ndrx_cltshm_setpos(c->key, EXFAIL, NDRX_CPM_MAP_WASUSED, NULL);

            if (CLT_STATE_NOTRUN==c->dyn.req_state )
            {   
                c->dyn.exit_status = 0;
            }
            else
            {
                c->dyn.exit_status = EXFAIL;
            }

            c->dyn.cur_state = CLT_STATE_NOTRUN;

            /* Set status change time */
            cpm_set_cur_time(c);
        }
    }
}

/**
 * Killall client running
 * @return SUCCEED
 */
expublic int cpm_killall(void)
{
    int ret = EXSUCCEED;
    cpm_process_t *c = NULL, *ct = NULL;
    int is_any_running;
    ndrx_stopwatch_t t;
    char *sig_str[3]={"SIGINT","SIGTERM", "SIGKILL"};
    int sig[3]={SIGINT,SIGTERM, SIGKILL};
    int i;
    int was_chld_kill;
    string_list_t* cltchildren = NULL;
    
    for (i=0; i<3; i++)
    {
        NDRX_LOG(log_warn, "Terminating all with %s", sig_str[i]);

        EXHASH_ITER(hh, G_clt_config, c, ct)
        {
            /* 
             * still check the pid, not? If running from shared mem blocks?
             */
            cpm_pidtest(c);
            
            if (CLT_STATE_STARTED==c->dyn.cur_state)
            {
                NDRX_LOG(log_warn, "Killing: %s/%s/%d with %s",
                    c->tag, c->subsect, c->dyn.pid, sig_str[i]);
                
                
                /* if we kill with -9, then kill all childrent too
                 * this is lengthly operation, thus only for emergency kill only
                 */
                was_chld_kill = EXFALSE;
                if ((SIGKILL==sig[i] && (c->stat.flags & CPM_F_KILL_LEVEL_LOW)) ||
                        c->stat.flags & CPM_F_KILL_LEVEL_HIGH)
                {
                    was_chld_kill = EXTRUE;
                    ndrx_proc_children_get_recursive(&cltchildren, c->dyn.pid);
                }
                
                kill(c->dyn.pid, sig[i]);
                
                if (was_chld_kill)
                {
                    ndrx_proc_kill_list(cltchildren);
                    ndrx_string_list_free(cltchildren);
                    cltchildren=NULL;
                }
            } /* for client in hash */
        } /* for attempt */

        if (i<2) /*no wait for kill... */
        {
            is_any_running = EXFALSE;
            ndrx_stopwatch_reset(&t);
            do
            {
#ifdef EX_CPM_NO_THREADS /* Bug #234 - have feedback at shutdown when no threads used */
                /* Process any dead child... */
                sign_chld_handler(SIGCHLD);
#endif
                EXHASH_ITER(hh, G_clt_config, c, ct)
                {
                    if (CLT_STATE_STARTED==c->dyn.cur_state)
                    {
                        is_any_running = EXTRUE;
                        break;
                    }
                }

                if (is_any_running)
                {
                    usleep(CLT_STEP_INTERVAL_ALL);
                }
            }
            while (is_any_running && 
                    ndrx_stopwatch_get_delta_sec(&t) < G_config.kill_interval);
        }
    }
    
    NDRX_LOG(log_debug, "cpm_killall done");
    return EXSUCCEED;
}

/**
 * Kill the process...
 * Firstly -2, then -15, then -9
 * We shall do kill in synchronous mode.
 * 
 * @param c
 * @return 
 */
expublic int cpm_kill(cpm_process_t *c)
{
    int ret = EXSUCCEED;
    ndrx_stopwatch_t t;
    string_list_t* cltchildren = NULL;
        
    NDRX_LOG(log_warn, "Stopping %s/%s - %s", c->tag, c->subsect, c->stat.command_line);
            
    /* INT interval */
    if (c->stat.flags & CPM_F_KILL_LEVEL_HIGH)
    {
        ndrx_proc_children_get_recursive(&cltchildren, c->dyn.pid);
    }
    
    kill(c->dyn.pid, SIGINT);
    
    if (c->stat.flags & CPM_F_KILL_LEVEL_HIGH)
    {
        ndrx_proc_kill_list(cltchildren);
        ndrx_string_list_free(cltchildren);
        cltchildren=NULL;
    }
    
    ndrx_stopwatch_reset(&t);
    do
    {
#ifdef EX_CPM_NO_THREADS /* Bug #234 - have feedback at shutdown when no threads used */
        /* Process any dead child... */
        sign_chld_handler(SIGCHLD);
#endif
        /* sign_chld_handler(0); */
        if (CLT_STATE_STARTED==c->dyn.cur_state)
        {
            usleep(CLT_STEP_INTERVAL);
        }
    } while (CLT_STATE_STARTED==c->dyn.cur_state && 
            ndrx_stopwatch_get_delta_sec(&t) < G_config.kill_interval);
    
    if (CLT_STATE_STARTED!=c->dyn.cur_state)
        goto out;
    
    NDRX_LOG(log_warn, "%s/%s Did not react on SIGINT, continue with SIGTERM", 
            c->tag, c->subsect);
    
    /* TERM interval */
    if (c->stat.flags & CPM_F_KILL_LEVEL_HIGH)
    {
        ndrx_proc_children_get_recursive(&cltchildren, c->dyn.pid);
    }
    
    kill(c->dyn.pid, SIGTERM);
    
    if (c->stat.flags & CPM_F_KILL_LEVEL_HIGH)
    {
        ndrx_proc_kill_list(cltchildren);
        ndrx_string_list_free(cltchildren);
        cltchildren=NULL;
    }
    
    
    ndrx_stopwatch_reset(&t);
    do
    {
#ifdef EX_CPM_NO_THREADS /* Bug #234 - have feedback at shutdown when no threads used */
        /* Process any dead child... */
        sign_chld_handler(SIGCHLD);
#endif
        /* if running from shared memory, do the check... */
        
        cpm_pidtest(c);
        /* sign_chld_handler(0); */
        if (CLT_STATE_STARTED==c->dyn.cur_state)
        {
            usleep(CLT_STEP_INTERVAL);
        }
    } while (CLT_STATE_STARTED==c->dyn.cur_state && 
            ndrx_stopwatch_get_delta_sec(&t) < G_config.kill_interval);
    
    if (CLT_STATE_STARTED!=c->dyn.cur_state)
        goto out;
    
    NDRX_LOG(log_warn, "%s/%s Did not react on SIGTERM, kill with -9", 
                        c->tag, c->subsect);
    
    /* KILL interval */
    
    /* OK we are here to kill -9, then we shall killall children processes too */
    
    /* if we kill with -9, then kill all children too
     * this is lengthly operation, thus only for emergency kill only 
     */
    
    
    if (c->stat.flags & CPM_F_KILL_LEVEL_LOW)
    {
        ndrx_proc_children_get_recursive(&cltchildren, c->dyn.pid);
    }
    
    kill(c->dyn.pid, SIGKILL);
    
    if (c->stat.flags & CPM_F_KILL_LEVEL_LOW)
    {
        ndrx_proc_kill_list(cltchildren);
        ndrx_string_list_free(cltchildren);
        cltchildren=NULL;
    }

    ndrx_stopwatch_reset(&t);
    do
    {
#ifdef EX_CPM_NO_THREADS /* Bug #234 - have feedback at shutdown when no threads used */
        /* Process any dead child... */
        sign_chld_handler(SIGCHLD);
#endif
        if (CLT_STATE_STARTED==c->dyn.cur_state)
        {
            usleep(CLT_STEP_INTERVAL);
      
        }
    }
    while (CLT_STATE_STARTED==c->dyn.cur_state && 
            ndrx_stopwatch_get_delta_sec(&t) < G_config.kill_interval);
    
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
expublic int cpm_exec(cpm_process_t *c)
{
    pid_t pid;
    char cmd_str[PATH_MAX];
    char *cmd[PATH_MAX]; /* splitted pointers.. */
    char separators[] = CPM_CMDLINE_SEP;
    char *token;
    int numargs = 0;
    int fd_stdout;
    int fd_stderr;
    int ret = EXSUCCEED;

    NDRX_LOG(log_warn, "*********processing for startup %s *********", 
            c->stat.command_line);
    
    c->dyn.was_started = EXTRUE; /* We tried to start...                */
    c->dyn.shm_read = EXFALSE;  /* We try to boot it, not attached      */
    /* clone our self */
    MUTEX_LOCK_V(M_forklock);
    pid = ndrx_fork();

    /* MUTEX_UNLOCK_V(M_forklock); mutex is not valid for child...
     * thus unlock bellow...
    */
    if( pid == 0)
    {
        /* close parent resources... Bug #176 
         * this will be closed by ndrx_atfork handler
	atmisrv_un_initialize(EXTRUE);*/
        
        /* some small delay so that parent gets time for PIDhash setup! */
        usleep(9000);
        
        /* Add env variables for NDRX_CLTTAG / NDRX_CLTSUBSECT */
        
        /* Fixes for #367 */
        if (EXSUCCEED!=setenv(NDRX_CLTTAG, c->tag, EXTRUE))
        {
            NDRX_LOG(log_error, "Cannot set [%s] to [%s]: %s", 
                    NDRX_CLTTAG, c->tag, strerror(errno));
            userlog("Cannot set [%s] to [%s]: %s", 
                    NDRX_CLTTAG, c->tag, strerror(errno));
            exit(1);
        }
        
        /* Fixes for #367 */
        if (EXSUCCEED!=setenv(NDRX_CLTSUBSECT, c->subsect, EXTRUE))
        {
            NDRX_LOG(log_error, "Cannot set [%s] to [%s]: %s", 
                    NDRX_CLTSUBSECT, c->subsect, strerror(errno));
            userlog("Cannot set [%s] to [%s]: %s", 
                    NDRX_CLTSUBSECT, c->subsect, strerror(errno));
            exit(1);
        }
        
        NDRX_STRCPY_SAFE(cmd_str, c->stat.command_line);

        token = strtok(cmd_str, separators);
        while( token != NULL )
        {
            cmd[numargs] = token;
            token = strtok( NULL, separators );
            numargs++;
        }
        cmd[numargs] = NULL;
    
        /*  Override environment, if there is such thing */
        if (EXEOS!=c->stat.env[0])
        {
            if (EXSUCCEED!=ndrx_load_new_env(c->stat.env))
            {
                userlog("Failed to load custom env from: %s!", c->stat.env);
                exit(1);
            }
        }
        
        if (EXEOS!=c->stat.cctag[0])
        {
            if (EXSUCCEED!=setenv(NDRX_CCTAG, c->stat.cctag, EXTRUE))
            {
                userlog("Cannot set [%s] to [%s]: %s", 
                        NDRX_CCTAG, c->stat.cctag, strerror(errno));
                exit(1);
            }
        }
        
        /* load envs from xml config */
        if (EXSUCCEED!=ndrx_ndrxconf_envs_apply(c->stat.envs))
        {
            userlog("Cannot load XML env for %s/%s: %s", 
                    NDRX_CCTAG, c->tag,c->subsect, strerror(errno));
            exit(1);
        }
        
        /* Change working dir */
        if (EXEOS!=c->stat.wd[0])
        {
            if (EXSUCCEED!=chdir(c->stat.wd))
            {
                int err = errno;
                
                NDRX_LOG(log_error, "Failed to change working diretory: %s - %s!", 
                        c->stat.wd, strerror(err));
                userlog("Failed to change working diretory: %s - %s!", 
                        c->stat.wd, strerror(err));
                exit(1);
            }
        }
        
        /* make stdout go to file */
        if (EXEOS!=c->stat.log_stdout[0] &&
                EXFAIL!=(fd_stdout = open(c->stat.log_stdout, 
               O_WRONLY| O_CREAT  | O_APPEND, S_IRUSR | S_IWUSR)))
        {
            dup2(fd_stdout, 1); 
            close(fd_stdout);
        }
        
        if (EXEOS!=c->stat.log_stderr[0] &&
                EXFAIL!=(fd_stderr = open(c->stat.log_stderr, 
                O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR)))
        {
            dup2(fd_stderr, 2);   /* make stderr go to file */
            close(fd_stderr);
        }
        
        /* reset signal handlers */
        signal(SIGINT, SIG_DFL);
        signal(SIGTERM, SIG_DFL);
        
        if (EXSUCCEED != execvp (cmd[0], cmd))
        {
            int err = errno;
            NDRX_LOG(log_error, "Failed to start client, error: %d, %s", 
                    err, strerror(err));
            exit (err);
        }
    }
    else if (EXFAIL!=pid)
    {
        MUTEX_UNLOCK_V(M_forklock);
        cpm_set_cur_time(c);
        c->dyn.pid = pid;
        c->dyn.cur_state = CLT_STATE_STARTED;
        
        /* extract the procname from command line */
        
        /* updated shared memory... */
        if (EXSUCCEED!=ndrx_cltshm_setpos(c->key, c->dyn.pid, 
                NDRX_CPM_MAP_ISUSED|NDRX_CPM_MAP_WASUSED|NDRX_CPM_MAP_CPMPROC, 
                c->stat.procname))
        {
            NDRX_LOG(log_error, "Failed to register client in CPM SHM/mem full, check %s param", 
                    CONF_NDRX_CLTMAX);
            userlog("Failed to register client in CPM SHM/mem full, check %s param", 
                    CONF_NDRX_CLTMAX);
        }   
    }
    else
    {
        MUTEX_UNLOCK_V(M_forklock);
        userlog("Failed to fork: %s", strerror(errno));
    }
    
out:
    return ret;
}
/* vim: set ts=4 sw=4 et smartindent: */
