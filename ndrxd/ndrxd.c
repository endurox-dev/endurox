/**
 * @brief Main Entry for EnduroX Daemon process
 *
 * @file ndrxd.c
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
#include <ndrx_config.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <memory.h>
#include <unistd.h>

#include <ndrstandard.h>
#include <ndrxd.h>
#include <atmi_int.h>
#include <atmi_shm.h>

#include <ndebug.h>
#include <cmd_processor.h>
#include <signal.h>

#include "userlog.h"
#include "atmi_cache.h"

/*---------------------------Externs------------------------------------*/
extern int optind, optopt, opterr;
/*---------------------------Macros-------------------------------------*/
#define NDRX_Q_TRYLOCK_TIME 10  /* time in which we must get the q lock */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
sys_config_t        G_sys_config;           /* Deamon configuration     */
/*---------------------------Prototypes---------------------------------*/

/**
 * Open PID file...
 * @return 
 */
exprivate int open_pid_file(void)
{
    int ret=EXSUCCEED;
    FILE *f = NULL;
    
    if (NULL==(f=NDRX_FOPEN(G_sys_config.pidfile, "w")))
    {
        NDRX_LOG(log_error, "Failed to open PID file %s for write: %s",
                    G_sys_config.pidfile, strerror(errno));
        userlog("Failed to open PID file %s for write: %s",
                    G_sys_config.pidfile, strerror(errno));
        ret=EXFAIL;
        goto out;
    }

    if (fprintf(f, "%d", getpid()) < 0)
    {
        NDRX_LOG(log_error, "Failed to write to PID file %s: %s",
                    G_sys_config.pidfile, strerror(errno));
        userlog("Failed to write to PID file %s: %s",
                    G_sys_config.pidfile, strerror(errno));
    }

    NDRX_FCLOSE(f);
    f=NULL;
out:
    return ret;
}

/**
 * Remove pid file
 * @param [in] second_call general call at the main() flow (not after shutdown)
 * @return SUCCEED/FAIL
 */
expublic int ndrxd_unlink_pid_file(int second_call)
{
    /* second time no error checking... */
    if (second_call && !ndrx_file_exists(G_sys_config.pidfile))
    {
        return EXSUCCEED;
    }
    
    if (EXSUCCEED!=unlink(G_sys_config.pidfile))
    {
        NDRX_LOG(log_error, "Failed to unlink to PID file %s: %s",
                    G_sys_config.pidfile, strerror(errno));
        return EXFAIL;
    }

    return EXSUCCEED;
}

/**
 * Enter into main loop of the deamon
 * Do receive command.
 * @return
 */
int main_loop()
{
    int finished = EXFALSE;
    int ret=EXSUCCEED;

    if (EXSUCCEED!=open_pid_file())
    {
        ret=EXFAIL;
        goto out;
    }
    /* Open queue & receive command */
    while (!finished && EXSUCCEED==ret && 
            /* stop processing if shutdown requested! */
            !(G_sys_config.stat_flags & NDRXD_STATE_SHUTDOWN && is_srvs_down()))
    {
        /* Process command */
        if (EXFAIL==command_wait_and_run(&finished, NDRXD_CTX_ZERO))
        {
            ret=EXFAIL;
            goto out;
        }
        
    }

out:
    return ret;
}

/**
 * NDRXD process got sigterm.
 * @return 
 */
void clean_shutdown(int sig)
{
    NDRX_LOG(log_warn, "Initiating ndrxd shutdown, signal: %d", sig);
    G_sys_config.stat_flags |= NDRXD_STATE_SHUTDOWN;
}

/**
 * Process command line options
 * @param argc
 * @param argv
 * @return 
 */
expublic int init_cmdline_opts(int argc, char **argv)
{
    int ret=EXSUCCEED;
    int c;
    extern char *optarg;
    
    /* Parse command line */
    while ((c = getopt(argc, argv, "h?rk:")) != -1)
    {
        switch(c)
        {
            case 'r':
                fprintf(stderr, "Entering in restart mode");
                G_sys_config.restarting  = EXTRUE;
                break;
            case 'k':
                /* No need for random key parsing yet */
                break;
            case 'h': case '?':
                printf("usage: %s [-r restart] [-k random key]\n",
                        argv[0]);
                
                return EXFAIL;

                break;
        }
    }
    
out:
    return ret;
}

/**
 * Standard initialization
 * @param argc
 * @param argv
 * @return
 */
int main_init(int argc, char** argv)
{
    int ret=EXSUCCEED;
    char *p;
    
#if 0
    
#ifdef EX_USE_EMQ
    /* We need to get lock in */
    emq_set_lock_timeout(NDRX_Q_TRYLOCK_TIME);
#endif
    
#endif

    /* common env loader will init the debug lib
     * which might call `ps' for process name
     * by popen(). which causes problems with
     * SIGCHLD handlers. Thus handle them after libinit
     */
    if (EXSUCCEED!=ndrx_load_common_env())
    {
        NDRX_LOG(log_error, "Failed to load common env");
        EXFAIL_OUT(ret);
    }
    
    /* We will ignore all stuff requesting shutdown! */
    signal(SIGHUP, SIG_IGN);
    signal(SIGTERM, SIG_IGN);
    signal(SIGINT, SIG_IGN);

    /********* Grab the configuration params  *********/
    G_sys_config.qprefix = getenv(CONF_NDRX_QPREFIX);
    if (NULL==G_sys_config.qprefix)
    {
        /* Write to ULOG? */
        NDRX_LOG(log_error, "Missing config key %s - FAIL", CONF_NDRX_QPREFIX);
        userlog("Missing config key %s - FAIL", CONF_NDRX_QPREFIX);
        ret=EXFAIL;
        goto out;
    }

    G_sys_config.config_file = getenv(CONF_NDRX_CONFIG);
    if (NULL==G_sys_config.config_file)
    {
        /* Write to ULOG? */
        NDRX_LOG(log_error, "Missing config key %s - FAIL", CONF_NDRX_CONFIG);
        userlog("Missing config key %s - FAIL", CONF_NDRX_CONFIG);
        ret=EXFAIL;
        goto out;
    }
    
    /* Override Q sizes */
    p = getenv(CONF_NDRX_DQMAX);
    if (NULL!=p)
    {
        /* Override MAX Q size */
        ndrx_get_G_atmi_env()->msg_max = atol(p);
        NDRX_LOG(log_debug, "NDRXD Max Q size to: %d (override)",
                            ndrx_get_G_atmi_env()->msg_max);
    }
    else
    {
        NDRX_LOG(log_debug, "NDRXD Max Q size to: %d (default)",
                            ndrx_get_G_atmi_env()->msg_max);
    }

    /* Command wait param */
    p = getenv(CONF_NDRX_CMDWAIT);
    if (NULL==p)
    {
        /* use default */
        G_sys_config.cmd_wait_time = COMMAND_WAIT_DEFAULT;
        NDRX_LOG(log_debug, "Command wait time defaulted to %ld sec",
                            G_sys_config.cmd_wait_time);
    }
    else
    {
        G_sys_config.cmd_wait_time = atol(p);
        NDRX_LOG(log_debug, "Command wait time set to %ld sec",
                            G_sys_config.cmd_wait_time);
    }
    
    if (EXSUCCEED!=cmd_processor_init())
    {
        /* Write to ULOG? */
        NDRX_LOG(log_error, "Failed to initialize command processor!");
        userlog("Failed to initailize command processor!");
        ret=EXFAIL;
        goto out;
    }
    
    /* Read PID file... */
    G_sys_config.pidfile = getenv(CONF_NDRX_DPID);

    if (NULL==G_sys_config.pidfile)
    {
        /* Write to ULOG? */
        NDRX_LOG(log_error, "Missing config key %s - FAIL", CONF_NDRX_DPID);
        userlog("Missing config key %s - FAIL", CONF_NDRX_DPID);
        ret=EXFAIL;
        goto out;
    }
    else
    {
        NDRX_LOG(log_error, "Using PID file [%s]", G_sys_config.pidfile);
    }
    
    G_sys_config.qpath = getenv(CONF_NDRX_QPATH);
    
    if (NULL==G_sys_config.qpath)
    {
        /* Write to ULOG? */
        NDRX_LOG(log_error, "Missing config key %s - FAIL", CONF_NDRX_QPATH);
        userlog("Missing config key %s - FAIL", CONF_NDRX_QPATH);
        ret=EXFAIL;
        goto out;
    }
    else
    {
        NDRX_LOG(log_error, "Using QPath [%s]", G_sys_config.qpath);
    }

    
    /* semaphores must go first!: */
    if (EXSUCCEED!=ndrxd_sem_init(G_sys_config.qprefix))
    {
        ret=EXFAIL;
        NDRX_LOG(log_error, "Failed to initialise share memory lib");
        goto out;
    }
    /* and then shm: initialise shared memory */
    if (EXSUCCEED!=ndrx_shm_init(G_sys_config.qprefix,
                            ndrx_get_G_atmi_env()->max_servers,
                            ndrx_get_G_atmi_env()->max_svcs))
    {
        ret=EXFAIL;
        NDRX_LOG(log_error, "Failed to initialise share memory lib");
        goto out;
    }
    
    /* Open shared memory */
    if (G_sys_config.restarting)
    {
        
        if (EXSUCCEED!=ndrx_sem_attach_all())
        {
            ret=EXFAIL;
            NDRX_LOG(log_error, "Failed to attach to Semaphores");
            goto out;
        }
        else
        {
            NDRX_LOG(log_error, "Attached to semaphores OK");
        }
        
        if (EXSUCCEED!=ndrx_shm_attach_all(NDRX_SHM_LEV_SVC | NDRX_SHM_LEV_SRV | NDRX_SHM_LEV_BR))
        {
            ret=EXFAIL;
            NDRX_LOG(log_error, "Failed to attach to shared memory segments");
            goto out;
        }
        else
        {
            NDRX_LOG(log_error, "Attached to share memory");
        }
        
    }
    else 
    {
        /* Semaphores are first */
        if (EXSUCCEED!=ndrxd_sem_open_all())
        {
            ret=EXFAIL;
            NDRX_LOG(log_error, "Failed to open semaphores!");
            goto out;
        }
        
        if (EXSUCCEED!=ndrxd_shm_open_all())
        {
            ret=EXFAIL;
            NDRX_LOG(log_error, "Failed to open shared memory segments!");
            goto out;
        }
    }
    
    ndrxd_sigchld_init();
    
#if 0
    /* Do the initialization... */
    if (EXFAIL==load_config(M_config_file))
    {
        /* Write to ULOG? */
        NDRX_LOG(log_error, "Configuration failed!");
        ret=EXFAIL;
        goto out;
    }
#endif
    
out:
    return ret;
}

/**
 * Uninit app.
 * @return
 */
int main_uninit(void)
{
    /* Remove signal handling thread */
    ndrxd_sigchld_uninit();
    
    /* Remove semaphores */
    ndrxd_sem_close_all();
    /* Remove semaphores */
    ndrxd_sem_delete();
    
    /* close shm */
    ndrxd_shm_close_all();

    /* Remove any shared memory segments (even if was not open!) */
    ndrxd_shm_delete();

    /* close & unlink message queue */
    cmd_close_queue();

    /* Remove pid file */
    ndrxd_unlink_pid_file(EXTRUE);
    
    return EXSUCCEED;
}

/*
 * Program main entry
 */
int main(int argc, char** argv) {

    int ret=EXSUCCEED;
    /* Do some init */
    memset(&G_sys_config, 0, sizeof(G_sys_config));
    
    if (EXSUCCEED!=init_cmdline_opts(argc, argv))
    {
        NDRX_LOG(log_error, "ndrxd - command line args failed");
        ret=EXFAIL;
        goto out;
    }
    else if (EXSUCCEED!=main_init(argc, argv))
    {
        NDRX_LOG(log_error, "ndrxd - INIT FAILED!");
        ret=EXFAIL;
        goto out;
    }
    /* If we are doing restart, the do actions needed for restart! */
    if (G_sys_config.restarting && EXSUCCEED!=do_restart_actions())
    {
        ret=EXFAIL;
        goto out;
    }

    ret=main_loop();

out:

    main_uninit();

    NDRX_LOG(log_error, "exiting %d", ret);

    return ret;
}
/* vim: set ts=4 sw=4 et smartindent: */
