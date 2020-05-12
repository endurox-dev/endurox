/**
 * @brief Build process model structure
 *
 * @file pmodel.c
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
#include <ndrx_config.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <utlist.h>
#include <errno.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <libgen.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <ndrxd.h>
#include <ndrxdcmn.h>
#include <userlog.h>

#include <nstopwatch.h>
#include <cmd_processor.h>
#include <pthread.h>
#include <nstdutil.h>
#include <bridge_int.h>
#include <atmi_shm.h>
#include <sys_unix.h>
#include <exenvapi.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

/** Step to realloc the CMD line */
#define REALLOC_CMD_STEP    10
#define REALLOC_CMD alloc_args+=REALLOC_CMD_STEP; \
	if (NULL==cmd) \
        	cmd = NDRX_MALLOC(sizeof(char *)*alloc_args); \
	else \
        	cmd = NDRX_REALLOC(cmd, sizeof(char *)*alloc_args); \
        if (NULL==cmd) \
        {\
            int err = errno;\
            fprintf(stderr, "%s: failed to realloc %ld bytes: %s\n", __func__, \
                (long)sizeof(char *)*alloc_args, strerror(err));\
            userlog("%s: failed to realloc %ld bytes: %s\n", __func__, \
                (long)sizeof(char *)*alloc_args, strerror(err));\
            exit(1);\
        }

#define NDRX_PM_SET_ENV(ENV__, VAL__) if (EXSUCCEED!=setenv(ENV__, VAL__, EXTRUE))\
        {\
           int err = errno;\
            userlog("%s: failed to set %s=[%s]: %s", __func__, \
                ENV__, VAL__, strerror(err));\
            exit(1);\
        }

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
exprivate pthread_t M_signal_thread; /* Signalled thread */
exprivate int M_signal_thread_set = EXFALSE; /* Signal thread is set */
exprivate volatile int M_shutdown = EXFALSE; /**< Doing shutdown    */

MUTEX_LOCKDECL(M_forklock);       /**< forking lock, no q ops during fork!  */

/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Initiate process reload
 * @return
 */
expublic int self_sreload(pm_node_t *p_pm)
{
    int ret=EXSUCCEED;
    command_startstop_t call;
    
    memset(&call, 0, sizeof(call));
    
    call.srvid = EXFAIL;
    NDRX_STRCPY_SAFE(call.binary_name, p_pm->binary_name);
    
    NDRX_LOG(log_debug, "Sending sreload command that [%s] must be reloaded, rq [%s]",
            call.binary_name, call.call.reply_queue);
    
    /* 
     * Send the notification that process needs to be reloaded
     */
    ret=cmd_generic_callfl(NDRXD_COM_SRELOADI_RQ, NDRXD_SRC_NDRXD,
                        NDRXD_CALL_TYPE_GENERIC,
                        (command_call_t *)&call, sizeof(call),
                        G_command_state.listenq_str,
                        (mqd_t)EXFAIL,
                        (mqd_t)EXFAIL,
                        G_command_state.listenq_str,
                        0, 
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        EXFALSE, TPNOBLOCK);
    
out:
    return ret;
}


/**
 * Report to ndrxd, that process had sigchld
 * @return
 */
expublic int self_notify(srv_status_t *status, int block)
{
    int ret=EXSUCCEED;
    size_t  send_size = sizeof(srv_status_t);

    LOCKED_DEBUG(log_debug, "About to send: %d bytes/%d svcs",
                        send_size, status->svc_count);
    
    /* we want new q/open + close here,
     * so that we do not interference with our main queue blocked/non blocked flags.
     */
    MUTEX_LOCK_V(M_forklock);
    ret=cmd_generic_callfl(NDRXD_COM_PMNTIFY_RQ, NDRXD_SRC_NDRXD,
                        NDRXD_CALL_TYPE_PM_INFO,
                        (command_call_t *)status, send_size,
                        G_command_state.listenq_str,
                        (mqd_t)EXFAIL,
                        (mqd_t)EXFAIL,
                        G_command_state.listenq_str,
                        0, 
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        EXFALSE, TPNOBLOCK);
    MUTEX_UNLOCK_V(M_forklock);
    
out:
    return ret;
}

/**
 * Handle the dead child detected either by sigwait or signal.
 * @param chldpid
 * @param stat_loc
 */
exprivate void handle_child(pid_t chldpid, int stat_loc)
{
    srv_status_t status;

    memset(&status, 0, sizeof(status));
    
    if (chldpid>0)
    {
        LOCKED_DEBUG(log_warn, "sigchld: PID: %d exit status: %d",
                                           chldpid, stat_loc);
        if (WIFSTOPPED(stat_loc))
        {
            LOCKED_DEBUG(log_warn, "Process is stopped - ignore..");
            return;
        }
        status.srvinfo.pid = chldpid;

        if (WIFEXITED(stat_loc) && (0 == (stat_loc & 0xff)))
        {
            LOCKED_DEBUG(log_error, "Process normal shutdown!");
            status.srvinfo.state = NDRXD_PM_EXIT;
        }
        else if (WIFEXITED(stat_loc) && TPEXIT_ENOENT == WEXITSTATUS(stat_loc))
        {
            LOCKED_DEBUG(log_error, "Binary not found!");
            status.srvinfo.state = NDRXD_PM_ENOENT;
        }
        else
        {
            LOCKED_DEBUG(log_error, "Process abnormal shutdown!");
            status.srvinfo.state = NDRXD_PM_DIED;
        }
        /* NDRX_LOG(log_warn, "Sending notification"); */
        self_notify(&status, EXFALSE);
    }
}

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
    
    LOCKED_DEBUG(log_debug, "check_child_exit - enter...");
    while (!M_shutdown)
    {
	int got_something = 0;

/* seems not working on darwin ... thus just wait for pid.
 * if we do not have any childs, then sleep for 1 sec.
 */
#ifndef EX_OS_DARWIN
        LOCKED_DEBUG(log_debug, "about to sigwait()");
        
        /* Wait for notification signal */
        if (EXSUCCEED!=sigwait(&blockMask, &sig))
        {
            LOCKED_DEBUG(log_warn, "sigwait failed:(%s)", strerror(errno));

        }        
#endif
        
        if (M_shutdown)
        {
            break;
            
        }
        
        LOCKED_DEBUG(log_debug, "about to wait()");
        
        while ((chldpid = wait3(&stat_loc, WNOHANG|WUNTRACED, &rusage)) > 0)
        {
            got_something++;
            handle_child(chldpid, stat_loc);
        }
        
#if EX_OS_DARWIN
        LOCKED_DEBUG(6, "wait: %s", strerror(errno));
        if (!got_something)
        {
            sleep(1);
        }
#endif
    }
   
    LOCKED_DEBUG(log_debug, "check_child_exit terminated");
    
    return NULL;
}


/**
 * Checks for child exit.
 * We will let mainthread to do all internal struct related work!
 * @return Got child exit
 */
expublic int thread_check_child_exit(void)
{
    pid_t chldpid;
    int stat_loc;
    struct rusage rusage;
    int ret=EXFALSE;

    memset(&rusage, 0, sizeof(rusage));

    while ((chldpid = wait3(&stat_loc, WNOHANG|WUNTRACED, &rusage)) > 0)
    {
        handle_child(chldpid, stat_loc);
    }
    
    return ret;
}

/**
 * Thread main entry...
 * @param arg
 * @return
 */
exprivate void *sigthread_enter(void *arg)
{
    LOCKED_DEBUG(log_error, "***********SIGNAL THREAD START***********");
    thread_check_child_exit();
    LOCKED_DEBUG(log_error, "***********SIGNAL THREAD EXIT***********");
    
    return NULL;
}

/**
 * NDRXD process got sigchld (handling the signal if OS have slipped the thing) out
 * of the mask.
 * @return
 */
void sign_chld_handler(int sig)
{
    pthread_t thread;
    pthread_attr_t pthread_custom_attr;

    pthread_attr_init(&pthread_custom_attr);
    /* clean up resources after exit.. */
    pthread_attr_setdetachstate(&pthread_custom_attr, PTHREAD_CREATE_DETACHED);
    /* set some small stacks size, 1M should be fine! */
    ndrx_platf_stack_set(&pthread_custom_attr);
    pthread_create(&thread, &pthread_custom_attr, sigthread_enter, NULL);

}

/**
 * Initialize polling lib
 * not thread safe.
 * @return
 */
expublic void ndrxd_sigchld_init(void)
{
    sigset_t blockMask;
    pthread_attr_t pthread_custom_attr;
    struct sigaction sa; /* Seem on AIX signal might slip.. */
    char *fn = "ndrxd_sigchld_init";

    NDRX_LOG(log_debug, "%s - enter", fn);
    
    /* our friend AIX, might just ignore the SIG_BLOCK and raise signal
     * Thus we will handle the stuff in as it was in Enduro/X 2.5
     */
    
    sa.sa_handler = sign_chld_handler;
    sigemptyset (&sa.sa_mask);
    sa.sa_flags = SA_RESTART; /* restart system calls please... */
    sigaction (SIGCHLD, &sa, 0);
    
    /* Block the notification signal (do not need it here...) */
    
    sigemptyset(&blockMask);
    sigaddset(&blockMask, SIGCHLD);
    
    /*
    if (pthread_sigmask(SIG_BLOCK, &blockMask, NULL) == -1)
        */
    if (sigprocmask(SIG_BLOCK, &blockMask, NULL) == -1)
    {
        NDRX_LOG(log_always, "%s: sigprocmask failed: %s", fn, strerror(errno));
    }
    
    pthread_attr_init(&pthread_custom_attr);
    
    /* set some small stacks size, 1M should be fine! */
    ndrx_platf_stack_set(&pthread_custom_attr);
    pthread_create(&M_signal_thread, &pthread_custom_attr, 
            check_child_exit, NULL);

    M_signal_thread_set = EXTRUE;
}

/**
 * Un-initialize sigchild monitor thread
 * @return
 */
expublic void ndrxd_sigchld_uninit(void)
{
    char *fn = "ndrxd_sigchld_uninit";
    int err;
    NDRX_LOG(log_debug, "%s - enter", fn);
    
    if (!M_signal_thread_set)
    {
        NDRX_LOG(log_debug, "Signal thread was not initialised, nothing todo...");
        goto out;
    }


    NDRX_LOG(log_debug, "About to cancel signal thread");
    
    /* TODO: have a counter for number of sets, so that we can do 
     * un-init...
     */
    M_shutdown = EXTRUE;
    
    if (EXSUCCEED!=(err=pthread_kill(M_signal_thread, SIGCHLD)))
    {
        NDRX_LOG(log_error, "Failed to kill poll signal thread: %s", strerror(err));
    }
    else
    {

        if (EXSUCCEED!=pthread_join(M_signal_thread, NULL))
        {
            NDRX_LOG(log_error, "Failed to join pthread_join() signal thread: %s", 
                    strerror(errno));
        }
    }
    
    M_signal_thread_set = EXFALSE;
    NDRX_LOG(log_debug, "finished ok");
out:
    return;
}

/**
 * Add PID to PID hash
 * @param pid_hash
 * @return SUCCEED/FAIL
 */
expublic int add_to_pid_hash(pm_pidhash_t **pid_hash, pm_node_t *p_pm)
{
    int hash_key = p_pm->pid % ndrx_get_G_atmi_env()->max_servers;
    int ret=EXSUCCEED;
    pm_pidhash_t *pm_pid  = NDRX_MALLOC(sizeof(pm_pidhash_t));
    memset(pm_pid, 0, sizeof(pm_pidhash_t));

    NDRX_LOG(log_debug, "About to add pid %d", p_pm->pid);

    /* check error */
    if (NULL==pm_pid)
    {
        NDRXD_set_error_fmt(NDRXD_EOS, "failed to allocate pm_pidhash_t (%d bytes): %s",
                            sizeof(pm_pidhash_t), strerror(errno));
        EXFAIL_OUT(ret);
    }

    pm_pid->p_pm = p_pm;
    pm_pid->pid = p_pm->pid;
    NDRX_LOG(log_debug, "Added pid %d to hash with key %d", pm_pid->pid, hash_key);
    DL_APPEND(pid_hash[hash_key], pm_pid);

out:
    return ret;
}

/**
 * Delete from PIDhash
 * @return SUCCEED/FAIL
 */
expublic int delete_from_pid_hash(pm_pidhash_t **pid_hash, pm_pidhash_t *pm_pid)
{
    int ret=EXSUCCEED;

    if (NULL!=pm_pid)
    {
        int hash_key = pm_pid->pid % ndrx_get_G_atmi_env()->max_servers;

        if (NULL!=pm_pid)
        {
            /* delete it */
            NDRX_LOG(log_error, "Removing pid %d from pidhash", pm_pid->pid);
            DL_DELETE(pid_hash[hash_key], pm_pid);
            /* here was memory leak!! */
            NDRX_FREE(pm_pid);
        }
    }
out:
    return ret;
}

/**
 * Compares two field defs and returns equality result
 * @param a
 * @param b
 * @return 0 - equal/ -1 - not equal
 */
exprivate int pid_hash_cmp(pm_pidhash_t *a, pm_pidhash_t *b)
{
    return (a->pid==b->pid?EXSUCCEED:EXFAIL);
}

/**
 * Get field entry from int hash
 */
expublic pm_pidhash_t *pid_hash_get(pm_pidhash_t **pid_hash, pid_t pid)
{
    /* Get the linear array key */
    int hash_key = pid % ndrx_get_G_atmi_env()->max_servers; /* Simple mod based hash */
    pm_pidhash_t *ret=NULL;
    pm_pidhash_t tmp;
    
    tmp.pid=pid;
    
    if (NULL!=pid_hash && NULL!=pid_hash[hash_key])
    {
        DL_SEARCH(pid_hash[hash_key], ret, &tmp, pid_hash_cmp);
        NDRX_LOG(log_debug, "Search for pid %d to hash with key %d, result: 0x%lx",
                            tmp.pid, hash_key, ret);
    }
    else
    {
       NDRX_LOG(log_warn, "Empty PID hashes. Cannot process pid %d", pid);
    }

    return ret;
}

/**
 * This will build initial process model
 * So this is linked list with all processes.
 *
 * Shall this support re-load?
 * In that case we should check that servers with existing ID's are not running,
 * but configuration is different?
 * - That probably could be done in another stange, there we should check existing
 * ones, if some process names are different, then reject!?!?
 */
expublic int build_process_model(conf_server_node_t *p_server_conf,
                                pm_node_t **p_pm_model, /* proces model linked list */
                                pm_node_t **p_pm_hash/* Hash table models */)
{
    int ret=EXSUCCEED;
    conf_server_node_t *p_conf;
    pm_node_t   *p_pm;
    char tmp[PATH_MAX+1];
    int cnt;
    char *p;
    NDRX_LOG(log_debug, "build_process_model enter");

    DL_FOREACH(p_server_conf, p_conf)
    {
        for (cnt=0; cnt<p_conf->max; cnt++)
        {
            /* Now prepare add node to list + hash table */
            p_pm = NDRX_CALLOC(1, sizeof(pm_node_t));
            if (NULL==p_pm)
            {
                /* Set return error code? */
                NDRXD_set_error_fmt(NDRXD_EOS, "Failed to allocate pm_node_t: `%s'",
                                                    strerror(errno));
                ret=EXFAIL;
                goto out;
            }
            
            p_pm->conf = p_conf; /* keep the reference to config entry */

            /* format the process model entry */
            NDRX_STRCPY_SAFE(p_pm->binary_name, p_conf->binary_name);
            /*
            NDRX_STRCPY_SAFE(p_pm->binary_name_real, p_conf->binary_name_real);
            */
            /* get the path of the binary... */
            if (p_conf->reloadonchange)
            {
                
                if (EXEOS!=p_pm->conf->fullpath[0])
                {
                    NDRX_LOG(log_debug, "Reusing full path from config...");
                    NDRX_STRCPY_SAFE(p_pm->binary_path, p_pm->conf->fullpath);
                }
                else if (NULL==ndrx_get_executable_path(p_pm->binary_path, 
                        sizeof(p_pm->binary_path), p_pm->binary_name))
                {
                    NDRX_LOG(log_error, "Failed to get path for executable [%s] "
                            "`reloadonchange' will not be able to monitor it!",
                            p_pm->binary_name);
                }
                else
                {
                    NDRX_LOG(log_info, "Got binary path: [%s]",
                            p_pm->binary_path);
                }
            }
            
            p_pm->srvid = p_conf->srvid+cnt;
            /* Request state is stopped */
            p_pm->reqstate = NDRXD_PM_NOT_STARTED;
            
            /* This must autostart! */
            if (p_conf->min > cnt)
            {
                p_pm->autostart=EXTRUE;
            }
            p_pm->autokill = p_conf->autokill;
            
            snprintf(p_pm->clopt, sizeof(p_pm->clopt), "-k %s -i %d %s", 
                    ndrx_get_G_atmi_env()->rnd_key, p_pm->srvid, p_conf->clopt);
            
            /* substitute env... - Feature #331 */
            
            snprintf(tmp, sizeof(tmp), "%d", (int)p_pm->srvid);
            NDRX_PM_SET_ENV(CONF_NDRX_SVSRVID, tmp);
            
            ndrx_str_env_subs_len(p_pm->clopt, sizeof(p_pm->clopt));
            
            /* build process real name  */
            if (EXEOS!=p_pm->conf->cmdline[0])
            {
                NDRX_STRCPY_SAFE(tmp, p_pm->conf->cmdline);

                /* substitute env */

                if (EXSUCCEED!=setenv(CONF_NDRX_SVPROCNAME, p_pm->conf->binary_name, EXTRUE))
                {
                    NDRX_LOG(log_error, "%s: failed to set %s=[%s]: %s", __func__, 
                        CONF_NDRX_SVPROCNAME, p_pm->conf->binary_name, strerror(errno));
                    EXFAIL_OUT(ret);
                }

                if (EXSUCCEED!=setenv(CONF_NDRX_SVCLOPT, p_pm->conf->clopt, EXTRUE))
                {
                    NDRX_LOG(log_error, "%s: failed to set %s=[%s]: %s", __func__, 
                        CONF_NDRX_SVCLOPT, p_pm->conf->clopt, strerror(errno));

                    EXFAIL_OUT(ret);
                }

                /* format the cmdline */
                ndrx_str_env_subs_len(tmp, sizeof(tmp));

                /* extract binary name for the path... */
                p = strtok(tmp, " \t");

                if (NULL==p)
                {
                    NDRX_LOG(log_error, "Missing process name in server's <cmdline> tag for [%s]",
                            p_pm->conf->binary_name);
                    EXFAIL_OUT(ret);
                }

                /* get the base name */
                p = basename(tmp);
                NDRX_STRCPY_SAFE(p_pm->binary_name_real, p);

                NDRX_LOG(log_debug, "Extracted real binary name [%s] from [%s]", 
                        p_pm->binary_name_real, p);

                /* unset variables */
                unsetenv(CONF_NDRX_SVPROCNAME);
                unsetenv(CONF_NDRX_SVCLOPT);
            }
            else
            {
                NDRX_STRCPY_SAFE(p_pm->binary_name_real, p_pm->binary_name);
            }

            unsetenv(CONF_NDRX_SVSRVID);
            
            /* substitute env... - Feature #331, END */

            /* now check the hash table for server instance entry */
            if (p_pm->srvid < 1 || p_pm->srvid>ndrx_get_G_atmi_env()->max_servers)
            {
                /* Invalid srvid  */
                NDRXD_set_error_fmt(NDRXD_ESRVCIDINV, "Invalid server id `%d'", 
                        p_pm->srvid);
                ret = EXFAIL;
                goto out;
            }
            else if (NULL!=p_pm_hash[p_pm->srvid])
            {
                /* Duplicate srvid */
                NDRXD_set_error_fmt(NDRXD_ESRVCIDDUP, "Duplicate server id `%d'", 
                        p_pm->srvid);
                ret = EXFAIL;
                goto out;
            }
            else
            {
                NDRX_LOG(log_debug, "adding %s:%d", p_pm->binary_name, 
                        p_pm->srvid);
                /* Add it to has & model? */
                DL_APPEND(*p_pm_model, p_pm);
                /* Add it to the hash */
                p_pm_hash[p_pm->srvid] = p_pm;
            }
        }/* for */
    } /* DL_FOREACH */

out:
    NDRX_LOG(log_debug, "build_process_model return %d", ret);
    return ret;
}

/**
 *
 * @param srvid
 * @return
 */
expublic pm_node_t * get_pm_from_srvid(int srvid)
{
    if (srvid>=0 && srvid<ndrx_get_G_atmi_env()->max_servers)
    {
        return G_process_model_hash[srvid];
    }
    else
    {
        NDRX_LOG(log_error, "Got invalid srvid %d", srvid);
        return NULL;
    }
}

/**
 * Returns server admin q formatted.
 * @param p_pm
 * @return 
 */
expublic char * get_srv_admin_q(pm_node_t * p_pm)
{
    static char ret[NDRX_MAX_Q_SIZE+1];
    
    snprintf(ret, sizeof(ret), NDRX_ADMIN_FMT, G_sys_config.qprefix, 
            p_pm->binary_name_real, p_pm->srvid, p_pm->svpid);
    
    return ret;
}

/**
 * Remove resources used by process which failed to start.
 * I.e. this should remove process from PIDhash & and any services advertised.
 * @param p_pm
 * @param svcnm - single service to un-register, optional
 * @param pm_pid - Optional, if provided then PIDs of p_pm & pm_pid ir tested.
 *                 If they are different then only record from pidhash is removed
 *                 with PID of pm_pid.
 * @return EXSUCCEED/EXFAIL
 */
expublic int remove_startfail_process(pm_node_t *p_pm, char *svcnm, 
        pm_pidhash_t *pm_pid)
{
    int ret=EXSUCCEED;
    pm_node_svc_t *elt, *tmp;
    
    if (NULL==p_pm)
        goto out;

    if (NULL!=pm_pid && p_pm->pid!=pm_pid->pid)
    {
        NDRX_LOG(log_warn, "Process Model SRV/PID=%d/%d but given "
                "PID Hash SRV/PID=%d/%d - thus remove later from pidhash only!",
                p_pm->srvid, p_pm->pid, pm_pid->p_pm->srvid, pm_pid->pid);
        
        /* Remove only pidhash entry.
         * This might happen in cases if binary was externally started or some 
         * kind of delays in startup caused two instances to be started?
         */
        if (EXFAIL!=pm_pid->pid && 0!=pm_pid->pid)
        {
            delete_from_pid_hash(G_process_model_pid_hash,
                                pid_hash_get(G_process_model_pid_hash, pm_pid->pid));
        }
        
        goto out;
        
    }
    
    if (NULL==svcnm)
    {
        /* Remote from pidhash */
        if (EXFAIL!=p_pm->pid && 0!=p_pm->pid)
        {
            delete_from_pid_hash(G_process_model_pid_hash,
                                pid_hash_get(G_process_model_pid_hash, p_pm->pid));
        }
        /* Reset signal counter */
        p_pm->num_term_sigs=0;
     
        /* Reset kill request */
        p_pm->killreq = EXFALSE;
        
        /* Remove any queues used... */
        remove_server_queues(p_pm->binary_name_real, p_pm->pid, p_pm->srvid, NULL);
        
    }
    
    /* delete any services allocated */
    if (p_pm->flags&SRV_KEY_FLAGS_BRIDGE)
    {
        brd_del_bridge(p_pm->nodeid);
        /* Remove bridge related flags. */
        p_pm->flags&=~SRV_KEY_FLAGS_BRIDGE;
        p_pm->flags&=~SRV_KEY_FLAGS_SENDREFERSH;
        p_pm->flags&=~SRV_KEY_FLAGS_CONNECTED;
        p_pm->nodeid = 0;
    }
    
    brd_begin_diff();
    /* Delete the stuff out !!! Warning ! if server did add to shm but did not notify us
     * there is chance that some services will be left hanging in SHM */
    DL_FOREACH_SAFE(p_pm->svcs,elt,tmp)
    {
        int last;
        if (NULL==svcnm || 0==strcmp(elt->svc.svc_nm, svcnm))
        {
            NDRX_LOG(log_warn, "Removing pid's %d service [%s]", p_pm->pid, 
                    elt->svc.svc_nm);
            /* Remove from shm */
            
            /* ###################### CRITICAL SECTION ###################### */
            /* So we make this part critical... */
            if (EXSUCCEED!=ndrx_lock_svc_op(__func__))
            {
                ret=EXFAIL;
                goto out;
            }
            
            /* TODO: We need to uninstall by RQADDR! */
            ndrxd_shm_uninstall_svc(elt->svc.svc_nm, &last, p_pm->resid);

#if defined(EX_USE_SYSVQ)
            
            /* nothing todo here! As we do not have per service queues */
            if (last)
            {
                NDRX_LOG(log_debug, "Service [%s] will be zapped by "
                        "RQADDR sanity checks", elt->svc.svc_nm);
            }
            
#elif defined(EX_USE_POLL)
            /* for poll() queues must be always removed. */
            remove_service_q(elt->svc.svc_nm, p_pm->srvid, (mqd_t)EXFAIL, NULL);
#else
            if (last)
            {
                remove_service_q(elt->svc.svc_nm, p_pm->srvid, (mqd_t)EXFAIL, NULL);
            }
#endif
            
            /* Remove the lock! */
            ndrx_unlock_svc_op(__func__);
            /* ###################### CRITICAL SECTION, END ################# */
            
            /*  Delete it from our bridge view */
            brd_del_svc_from_hash(elt->svc.svc_nm);
            
            /* Delete out it from list */
            DL_DELETE(p_pm->svcs,elt);
            /* Fee up resources */
            NDRX_FREE(elt);
            
            if (NULL!=svcnm)
                break; /* This was one we wanted to remove! */
        }
    }
    brd_end_diff();
    
out:
    return ret;
}

/**
 * Start single process...
 * TODO: Add SIGCHLD handler here!
 * @param pm
 * @return SUCCEED/FAIL
 */
expublic int start_process(command_startstop_t *cmd_call, pm_node_t *p_pm,
            void (*p_startup_progress)(command_startstop_t *call, pm_node_t *p_pm, int calltype),
            long *p_processes_started,
            int no_wait,
            int *doabort)
{
    int ret=EXSUCCEED;
    pid_t pid;

    /* prepare args for execution... */
    char cmd_str[PATH_MAX];
    char **cmd = NULL; /* splitted pointers.. */
    char separators[]   = " ,\t\n";
    char *token;
    int numargs;
    int alloc_args;
    char tmp[256];

    NDRX_LOG(log_warn, "*********processing for startup %s/%d*********",
                                    p_pm->binary_name, p_pm->srvid);
    
    if (NDRXD_PM_RUNNING_OK==p_pm->state)
    {

        NDRX_LOG(log_warn, "Not starting %s/%d, already in "
                                      "running state!",
                                      p_pm->binary_name, p_pm->srvid);
        goto out;
    }

   /* Send notification, that we are about to start? */
    p_pm->state = NDRXD_PM_STARTING;
    p_pm->state_changed = SANITY_CNT_START;

    if (NULL!=p_startup_progress)
        p_startup_progress(cmd_call, p_pm, NDRXD_CALL_TYPE_PM_STARTING);
    
    /* calculate the checksum of the process */
    if (p_pm->conf->reloadonchange && EXEOS!=p_pm->binary_path[0])
    {
        roc_mark_as_reloaded(p_pm->binary_path, G_sanity_cycle);
    }
    
    /* clone our self */
    
    /*
     * During the fork, no queue ops shall be done! This can cause
     * System V event thread corruption.
     */
    MUTEX_LOCK_V(M_forklock);
    pid = ndrx_fork();
    
    if( pid == 0)
    {
        char sysflags_str[30];
        long sysflags = 0;
        
        /* Export startup mode */
        
        if (G_sys_config.fullstart)
        {
            sysflags |= NDRX_PRC_SYSFLAGS_FULLSTART;
        }
        
        snprintf(sysflags_str, sizeof(sysflags_str), "%ld", sysflags);
        
        if (EXSUCCEED!=setenv(CONF_NDRX_SYSFLAGS, sysflags_str, EXTRUE))
        {
            userlog("Failed to set env: %s", strerror(errno));
            exit(1);
        }
        
        /* Bug #176: close parent resources - not needed any more... */
        ndrxd_shm_close_all();

        /* for System V child event thread is not resumed
         * but it is not a big deal, this just deallocates the resources
         * after exec they will be free'd anyway.
         */
    	if (EXSUCCEED!=ndrx_mq_close(G_command_state.listenq))
        {
            userlog("Failed to close: [%s] err: %s",
                                     G_command_state.listenq_str, strerror(errno));
        }
        
        /* some small delay so that parent gets time for PIDhash setup! */
        usleep(9000);
        
        /* this is child - start EnduroX back-end*/
        /*fprintf(stderr, "starting with: [%s]", p_pm->clopt);*/
        
        /* export intermediate variables 
         * CONF_NDRX_SVPROCNAME -> binary_name
         * CONF_NDRX_SVCLOPT-> dynamic clopt built
         */
        
        NDRX_PM_SET_ENV(CONF_NDRX_SVPROCNAME, p_pm->binary_name);
        NDRX_PM_SET_ENV(CONF_NDRX_SVCLOPT, p_pm->clopt);        
        /* export server id... */
        snprintf(tmp, sizeof(tmp), "%d", (int)p_pm->srvid);
        NDRX_PM_SET_ENV(CONF_NDRX_SVSRVID, tmp);
        
        /* ... and export PID 
         * This might be a parent process of the script
         * thus we need to pass it down to th client process.
         */
        snprintf(tmp, sizeof(tmp), "%d", (int)getpid());
        NDRX_PM_SET_ENV(CONF_NDRX_SVPPID, tmp);

        alloc_args = 0;
        REALLOC_CMD;
        
        if (EXEOS!=p_pm->conf->cmdline[0])
        {
            NDRX_STRCPY_SAFE(cmd_str, p_pm->conf->cmdline);
            
            /* format the cmdline */
            ndrx_str_env_subs_len(cmd_str, sizeof(cmd_str));
            
            numargs=0;    
            token = strtok(cmd_str, separators);
            while( token != NULL )
            {
                /* the alloc args are zero based index.. */
                if (numargs+1 >= alloc_args)
                {
                    /* realloc the cmd storage */
                    REALLOC_CMD;
                }
                cmd[numargs] = token;
                token = strtok( NULL, separators );
                numargs++;
            }
            cmd[numargs] = NULL;
            
        }
        else
        {
            NDRX_STRCPY_SAFE(cmd_str, p_pm->clopt);

            numargs=1;
                
            if (EXEOS!=p_pm->conf->fullpath[0])
            {
                cmd[0] = p_pm->conf->fullpath;
            }
            else
            {
                cmd[0] = p_pm->binary_name;
            }
            
            token = strtok(cmd_str, separators);
            while( token != NULL )
            {
                /* the alloc args are zero based index.. */
                if (numargs+1 >= alloc_args)
                {
                    /* realloc the cmd storage */
                    REALLOC_CMD;
                }
                
                cmd[numargs] = token;
                token = strtok( NULL, separators );
                numargs++;
            }
            cmd[numargs] = NULL;
        }
        
        /*  Override environment, if there is such thing */
        if (EXEOS!=p_pm->conf->env[0])
        {
            if (EXSUCCEED!=ndrx_load_new_env(p_pm->conf->env))
            {
                userlog("Failed to load custom env from: %s!", 
                        p_pm->conf->env);
                exit(1);
            }
        }
        
        if (EXEOS!=p_pm->conf->cctag[0])
        {
            if (EXSUCCEED!=setenv(NDRX_CCTAG, p_pm->conf->cctag, EXTRUE))
            {
                userlog("Cannot set [%s] to [%s]: %s", 
                        NDRX_CCTAG, p_pm->conf->cctag, strerror(errno));
                exit(1);
            }
        }
        
        /* process env variables defined in ndrxconfig.xml */
        
        if (EXSUCCEED!=ndrx_ndrxconf_envs_applyall(p_pm->conf->envgrouplist,
                p_pm->conf->envlist))
        {
            userlog("Failed to load config from ndrxconfig.xml!");
            exit(1);
        }
        
        /* free up the allocate resources */
        
        if (EXSUCCEED != execvp (cmd[0], cmd))
        {
            int err = errno;
            fprintf(stderr, "Failed to start server [%s], error: %d, %s\n",
                                cmd[0], err, strerror(err));
            
            /* free up the list, so that we do not report memory leak... 
             * in case if binary not started.
             */
            NDRX_FREE(cmd);
            
            if (ENOENT==err)
                exit(TPEXIT_ENOENT);
            else
                exit(1);
        }
    }
    else if (EXFAIL!=pid)
    {
        ndrx_stopwatch_t timer;
        int finished = EXFALSE;
        
        /* parent unlock */
        MUTEX_UNLOCK_V(M_forklock);
        
        /* Add stuff to PIDhash */
        p_pm->pid = pid;
        /* currently assume they are the same */
        p_pm->svpid = pid;
        add_to_pid_hash(G_process_model_pid_hash, p_pm);
        
        /* Reset PM timer */
        p_pm->rspstwatch = SANITY_CNT_START;
        
        /* Set requested state to started */
        p_pm->reqstate=NDRXD_PM_RUNNING_OK;
        
        /* Each time we try to start it we increment the try counter: */
        if (p_pm->exec_seq_try+1 < RESPAWN_CNTR_MAX)
        {
            p_pm->exec_seq_try++;
        }
        
        /* Do bellow stuff only if we can wait! */
        if (!no_wait)
        {
            /* this is parent for child - sleep some seconds, then check for PID... */
            /* TODO: Replace sleep with wait call from service - wait for message? */
            /*usleep(250000);  250 milli seconds */
            ndrx_stopwatch_reset(&timer);

            do
            {
                NDRX_LOG(log_debug, "Waiting for response from srv...");
                /* do command processing for now */
                command_wait_and_run(&finished, doabort);
                /* check the status? */
            } while (ndrx_stopwatch_get_delta(&timer) < p_pm->conf->srvstartwait && 
                            NDRXD_PM_STARTING==p_pm->state && !(*doabort));
            
            if (NDRXD_PM_RUNNING_OK==p_pm->state && p_pm->conf->sleep_after)
            {
                ndrx_stopwatch_t sleep_timer;
                ndrx_stopwatch_reset(&sleep_timer);
                
                do
                {
                    NDRX_LOG(log_debug, "In process after start sleep...");
                    command_wait_and_run(&finished, doabort);
                } while (ndrx_stopwatch_get_delta_sec(&sleep_timer) < p_pm->conf->sleep_after);
                
            }
            
            /* Check for process name & pid */
            if (ndrx_sys_is_process_running(p_pm->svpid, p_pm->binary_name_real))
            {
                /*Should be set at info: p_pm->state = NDRXD_PM_RUNNING;*/
                NDRX_LOG(log_debug, "binary %s/%s, srvid %d started with pid %d/%d",
                            p_pm->binary_name,p_pm->binary_name_real, p_pm->srvid, 
                        pid, p_pm->svpid);
                (*p_processes_started)++;
            }
            else if (NDRXD_PM_NOT_STARTED==p_pm->state)
            {
                /* Assume died? */
                p_pm->state = NDRXD_PM_DIED;
                p_pm->state_changed = SANITY_CNT_START;
                NDRX_LOG(log_debug, "binary %s, srvid %d failed to start",
                            p_pm->binary_name, p_pm->srvid, pid);
            }
        }
    }
    else
    {
        /* parent unlock */
        MUTEX_UNLOCK_V(M_forklock);
        
        NDRXD_set_error_fmt(NDRXD_EOS, "Fork failed: %s", strerror(errno));
        p_pm->state = NDRXD_PM_DIED;
        p_pm->state_changed = SANITY_CNT_START;
        /*
        ret=FAIL;
         */
    }

    NDRX_LOG(log_debug, "PID of started process is %d", p_pm->pid);
            if (NULL!=p_startup_progress)
                p_startup_progress(cmd_call, p_pm, NDRXD_CALL_TYPE_PM_STARTED);

out:
    return ret;
}

/**
 * Stop single process
 * @param p_pm
 * @return SUCCEED/FAIL
 */
expublic int stop_process(command_startstop_t *cmd_call, pm_node_t *p_pm,
            void (*p_shutdown_progress)(command_call_t *call, pm_node_t *pm, int calltype),
            long *p_processes_shutdown,
            int *abort)
{
    int ret=EXSUCCEED;
    command_call_t call;
    ndrx_stopwatch_t timer;
    int finished = EXFALSE;
    int first_shutdown;
    long start_state;
    char *srv_queue;
    int retry;
    char fn[] = "stop_process";

    memset(&call, 0, sizeof(call));
    NDRX_LOG(log_debug, "%s: Enter", fn);


    /* Check process initially, do it really requires shutdown? */
    NDRX_LOG(log_warn, "processing shutdown %s/%d",
                                p_pm->binary_name, p_pm->srvid);

    /* Request that we stay stopped! */
    p_pm->reqstate = NDRXD_PM_NOT_STARTED;
    
    if (NDRXD_PM_RUNNING_OK!=p_pm->state &&
            NDRXD_PM_STOPPING!=p_pm->state &&
            NDRXD_PM_STARTING!=p_pm->state)
    {
        NDRX_LOG(log_debug, "Process already in non-runabled "
                                    "state: %d", p_pm->state);
        /* set the state to shutdown! */
        p_pm->state = NDRXD_PM_EXIT;
        p_pm->state_changed = SANITY_CNT_START;
        goto out;
    }

    /* Send notification, that we are about to stop? */
    p_pm->state = NDRXD_PM_STOPPING;
    p_pm->state_changed = SANITY_CNT_START;
    /* Reset response timer */
    p_pm->rspstwatch = SANITY_CNT_START;
    /* Reset ping too */
    p_pm->pingstwatch = SANITY_CNT_IDLE;
    

    if (NULL!=cmd_call && NULL!=p_shutdown_progress)
        p_shutdown_progress((command_call_t*)cmd_call, p_pm, NDRXD_CALL_TYPE_PM_STOPPING);

    /* Form a call queue, probably we need move all stuff to atmienv!  
    snprintf(srv_queue, sizeof(srv_queue), NDRX_ADMIN_FMT, G_sys_config.qprefix, 
            p_pm->binary_name, p_pm->srvid, p_pm->pid);*/

    ndrx_stopwatch_reset(&timer);
    first_shutdown=EXTRUE;
    retry=EXFALSE;
    start_state = p_pm->state;
    do
    {
        /* Request the shutdown 
         * in case if first request
         * or if server slipped initial shutdown request
         * if states are changed that means that still we are in running range
         * otherwise the bellow while should terminate
         */
        if (first_shutdown || start_state!=p_pm->state || retry)
        {
            srv_queue = get_srv_admin_q(p_pm);
            NDRX_LOG(log_debug, "%s: calling up: [%s] (retry: %d start_state: %ld cur_state: %ld)", 
                    fn, srv_queue, retry, start_state, p_pm->state);
            if (EXSUCCEED!=(ret=cmd_generic_call_2(NDRXD_COM_SRVSTOP_RQ, NDRXD_SRC_ADMIN,
                            NDRXD_CALL_TYPE_GENERIC,
                            &call, sizeof(call),
                            G_command_state.listenq_str,
                            G_command_state.listenq,
                            (mqd_t)EXFAIL,
                            srv_queue,
                            0, NULL,
                            NULL,
                            NULL,
                            NULL,
                            EXFALSE,
                            EXFALSE,
                            NULL, NULL, TPNOTIME|TPNOBLOCK, NULL)))
            {
                /*goto out; Ignore this condition, just get the status of binary... */
                /* if server was starting, the admin possibly q was not open */
                retry=EXTRUE;
            }
            else
            {
                start_state=p_pm->state;
                retry=EXFALSE;
            }
        }
        
        NDRX_LOG(log_debug, "Waiting for response from srv... state: %d",
                                        p_pm->state); 
        /* do command processing for now */
        command_wait_and_run(&finished, abort);
        
        
        /* check the status? 
         * in case if during the shutdown process have changed state
         * i.e. from one started state switched to another started
         * state, that could indicate that process was in slow
         * startup and did ignore our shutdown request..
         * Thus if state is changed we should restart the call
         */
        first_shutdown=EXFALSE;
        
    } while (ndrx_stopwatch_get_delta(&timer) < p_pm->conf->srvstopwait &&
                    !PM_NOT_RUNNING(p_pm->state) &&
                    !(*abort));
    
    /* grab some stats... */
    if (PM_NOT_RUNNING(p_pm->state))
    {
        if (NULL!=p_processes_shutdown)
            (*p_processes_shutdown)++;
    }
    
    /* Send the progress of the shtudown (detail) */
    if (NULL!=cmd_call && NULL!=p_shutdown_progress)
        p_shutdown_progress((command_call_t*)cmd_call, p_pm, NDRXD_CALL_TYPE_PM_STOPPED);

out:
    NDRX_LOG(log_debug, "%s: Exit (%d)", fn, ret);
    return ret;
}

/**
 * Start whole application. If configuration is not loaded, then this will
 * initiate configuration load.
 * @return SUCCEED/FAIL
 */
expublic int app_startup(command_startstop_t *call,
        void (*p_startup_progress)(command_startstop_t *call, pm_node_t *pm, int calltype),
        long *p_processes_started) /* have some progress feedback */
{
    int ret=EXSUCCEED;
    pm_node_t *p_pm;
    int abort = EXFALSE;
    NDRX_LOG(log_warn, "Starting application domain");

    if (NULL==G_app_config && EXSUCCEED!=load_active_config(&G_app_config,
                &G_process_model, &G_process_model_hash, &G_process_model_pid_hash))
    {
        ret=EXFAIL;
        goto out;
    }

    /* OK, now loop throught the stuff */
    G_sys_config.stat_flags |= NDRXD_STATE_DOMSTART;

    if (EXFAIL!=call->srvid)
    {
        /* Check the servid... */
        if (call->srvid>=0 && call->srvid<ndrx_get_G_atmi_env()->max_servers)
        {
            pm_node_t *p_pm_srvid = G_process_model_hash[call->srvid];

            if (NULL!=p_pm_srvid)
            {
                start_process(call, p_pm_srvid, p_startup_progress,
                                    p_processes_started, EXFALSE, &abort);
            }
            else
            {
                NDRX_LOG(log_error, "Srvid: %d not initialized", call->srvid);
            }
        }
        else
        {
            NDRX_LOG(log_error, "Invalid srvid: %d", call->srvid);
        }
    }
    else /* process this if request for srvnm or full startup... */
    {
        if (EXEOS==call->binary_name[0])
        {
            G_sys_config.fullstart=EXTRUE;
        }
        
        DL_FOREACH(G_process_model, p_pm)
        {
            /* if particular binary shutdown requested (probably we could add some index!?) */
            
            /* Bug #306 auto start affects also server boot by name
             * Only autostart instances needs to be booted.
             */
            if (p_pm->autostart &&
                    ((EXEOS!=call->binary_name[0] && 0==strcmp(call->binary_name, p_pm->binary_name)) ||
                    /* Do full startup if requested autostart! */
                    (EXEOS==call->binary_name[0] ))) /* or If full shutdown requested */
            {
                start_process(call, p_pm, p_startup_progress, 
                        p_processes_started, EXFALSE, &abort);
                
                if (abort)
                {
                    NDRX_LOG(log_warn, "Aborting app domain startup!");
                    NDRXD_set_error_fmt(NDRXD_EABORT, "App domain startup aborted!");
                    ret=EXFAIL;
                    goto out;
                }
            }
        } /* DL_FORACH pm. */
        
        if (G_sys_config.fullstart)
        {
            /* downgrade back to partial start */
            G_sys_config.fullstart=EXFALSE;
        }
    }
out:
    return ret;
}

/**
 * Stop whole application.
 * App domain must be started in order to do shutdown.
 * @return SUCCEED/FAIL
 */
expublic int app_shutdown(command_startstop_t *call,
        /* have some progress feedback */
        void (*p_shutdown_progress)(command_call_t *call, pm_node_t *pm, int calltype),
        long *p_processes_shutdown)
{
    int ret=EXSUCCEED;
    int abort=EXFALSE;
    pm_node_t *p_pm;
    
    NDRX_LOG(log_warn, "Stopping application domain");


    /* OK, now loop throught the stuff 
    G_sys_config.stat_flags |= NDRXD_STATE_SHUTDOWN;
*/
    if (EXFAIL!=call->srvid)
    {
        /* Check the servid... */
        if (call->srvid>=0 && call->srvid<ndrx_get_G_atmi_env()->max_servers)
        {
            pm_node_t *p_pm_srvid = G_process_model_hash[call->srvid];
            
            if (NULL!=p_pm_srvid)
            {
                stop_process(call, p_pm_srvid, p_shutdown_progress,
                                    p_processes_shutdown, &abort);
            }
            else
            {
                NDRX_LOG(log_error, "Srvid: %d not initialized", call->srvid);
            }
        }
        else
        {
            NDRX_LOG(log_error, "Invalid srvid: %d", call->srvid);
        }
    }
    else if (G_process_model) /* process this if request for srvnm or full shutdown... */
    {
        int i;
        DL_REVFOREACH(G_process_model, p_pm, i)
        {
            /* if particular binary shutdown requested (probably we could add some index!?) */
            if ((EXEOS!=call->binary_name[0] && 0==strcmp(call->binary_name, p_pm->binary_name)) ||
                    (EXEOS==call->binary_name[0] &&  /* or If full shutdown requested */
                    /* is if binary is not protected, or we run complete shutdown... */
                    (!p_pm->conf->isprotected || call->complete_shutdown))) 
            {
                stop_process(call, p_pm, p_shutdown_progress, 
                        p_processes_shutdown, &abort);
                
                if (abort)
                {
                    NDRX_LOG(log_warn, "Aborting app domain shutdown!");
                    NDRXD_set_error_fmt(NDRXD_EABORT, "App domain shutdown aborted!");
                    ret=EXFAIL;
                    goto out;
                }
            }
        } /* For each pm. */
    }
    
    /* set state that we are shutdowned...
     * Hmm we should reply back and at next loop go down?
     * Or stay in idle?
     Not now, somehow needs to sync...
    G_sys_config.stat_flags &= ~NDRXD_STATE_SHUTDOWN;
    G_sys_config.stat_flags |= NDRXD_STATE_SHUTDOWNED;
    */
out:
    return ret;
}

/**
 * Test process mode, it should be shutdown...!
 * @return 
 */
expublic int is_srvs_down(void)
{
    pm_node_t *p_pm;
    int is_down = EXTRUE;
    
    DL_FOREACH(G_process_model, p_pm)
    {
        if (PM_RUNNING(p_pm->state))
        {
            NDRX_LOG(6, "All servers not down...");
            is_down=EXFALSE;
        }
    } /* DL_FORACH pm. */
    
    return is_down;
}

/**
 * Reload services...
 * @return SUCCEED/FAIL
 */
expublic int app_sreload(command_startstop_t *call,
        void (*p_startup_progress)(command_startstop_t *call, pm_node_t *pm, int calltype),
        void (*p_shutdown_progress)(command_call_t *call, pm_node_t *pm, int calltype),
        long *p_processes_started) /* have some progress feedback */
{
    int ret=EXSUCCEED;
    pm_node_t *p_pm;
    int abort = EXFALSE;
    NDRX_LOG(log_warn, "Starting application domain");

    /*
    if (NULL==G_app_config && SUCCEED!=load_active_config(&G_app_config,
                &G_process_model, &G_process_model_hash, &G_process_model_pid_hash))
    {
        ret=FAIL;
        goto out;
    }
     */
    
    if (NULL==G_app_config)
    {
        NDRX_LOG(log_error, "No configuration loaded!");
        ret=EXFAIL;
        goto out;
    }

    /* OK, now loop throught the stuff 
    G_sys_config.stat_flags |= NDRXD_STATE_DOMSTART;
     * */

    if (EXFAIL!=call->srvid)
    {
        /* Check the servid... */
        if (call->srvid>=0 && call->srvid<ndrx_get_G_atmi_env()->max_servers)
        {
            pm_node_t *p_pm_srvid = G_process_model_hash[call->srvid];

            if (NULL!=p_pm_srvid)
            {
                /* Firstly stop it */
                stop_process(call, p_pm_srvid, p_shutdown_progress, 
                        NULL, &abort);

                /* Then start it */
                if (!abort)
                {
                    start_process(call, p_pm_srvid, p_startup_progress,
                                        p_processes_started, EXFALSE, &abort);
                }
                
                if (abort)
                {
                    NDRX_LOG(log_warn, "Aborting app domain startup!");
                    NDRXD_set_error_fmt(NDRXD_EABORT, "App domain startup aborted!");
                    ret=EXFAIL;
                    goto out;
                }
            }
            else
            {
                NDRX_LOG(log_error, "Srvid: %d not initialized", call->srvid);
            }
        }
        else
        {
            NDRX_LOG(log_error, "Invalid srvid: %d", call->srvid);
        }
    }
    else /* process this if request for srvnm or full startup... */
    {
        DL_FOREACH(G_process_model, p_pm)
        {
            /* if particular binary shutdown requested (probably we could add some index!?) */
            if ((EXEOS!=call->binary_name[0] 
                    && 0==strcmp(call->binary_name, p_pm->binary_name)) ||
                    /* Do full startup if requested autostart! */
                    /* or If full shutdown requested */
                    (EXEOS==call->binary_name[0] && p_pm->autostart))
            {
                
                stop_process(call, p_pm, p_shutdown_progress, 
                        NULL, &abort);
                
                if (!abort)
                {
                    start_process(call, p_pm, p_startup_progress, 
                            p_processes_started, EXFALSE, &abort);
                }
                
                if (abort)
                {
                    NDRX_LOG(log_warn, "Aborting app domain startup!");
                    NDRXD_set_error_fmt(NDRXD_EABORT, "App domain startup aborted!");
                    ret=EXFAIL;
                    goto out;
                }
            }
        } /* DL_FORACH pm. */
    }
    
out:
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
