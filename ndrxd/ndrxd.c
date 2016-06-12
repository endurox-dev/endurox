/* 
** Main Entry for EnduroX Daemon process
**
** @file ndrxd.c
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
#include <config.h>
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

/*---------------------------Externs------------------------------------*/
extern int optind, optopt, opterr;
/*---------------------------Macros-------------------------------------*/
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
private int open_pid_file(void)
{
    int ret=SUCCEED;
    FILE *f = NULL;
    
    if (NULL==(f=fopen(G_sys_config.pidfile, "w")))
    {
        NDRX_LOG(log_error, "Failed to open PID file %s for write: %s",
                    G_sys_config.pidfile, strerror(errno));
        userlog("Failed to open PID file %s for write: %s",
                    G_sys_config.pidfile, strerror(errno));
        ret=FAIL;
        goto out;
    }

    if (fprintf(f, "%d", getpid()) < 0)
    {
        NDRX_LOG(log_error, "Failed to write to PID file %s: %s",
                    G_sys_config.pidfile, strerror(errno));
        userlog("Failed to write to PID file %s: %s",
                    G_sys_config.pidfile, strerror(errno));
    }

    fclose(f);
    f=NULL;
out:
    return ret;
}

/**
 * Remove pid file
 * @return SUCCEED/FAIL
 */
private int unlink_pid_file(void)
{
    if (SUCCEED!=unlink(G_sys_config.pidfile))
    {
        NDRX_LOG(log_error, "Failed to unlink to PID file %s: %s",
                    G_sys_config.pidfile, strerror(errno));
        return FAIL;
    }

    return SUCCEED;
}

/**
 * Enter into main loop of the deamon
 * Do receive command.
 * @return
 */
int main_loop()
{
    int finished = FALSE;
    int ret=SUCCEED;

    if (SUCCEED!=open_pid_file())
    {
        ret=FAIL;
        goto out;
    }
    /* Open queue & receive command */
    while (!finished && SUCCEED==ret && 
            /* stop processing if shutdown requested! */
            !(G_sys_config.stat_flags & NDRXD_STATE_SHUTDOWN && is_srvs_down()))
    {
        /* Process command */
        if (FAIL==command_wait_and_run(&finished, NDRXD_CTX_ZERO))
        {
            ret=FAIL;
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
public int init_cmdline_opts(int argc, char **argv)
{
    int ret=SUCCEED;
    int c;
    extern char *optarg;
    
    /* Parse command line */
    while ((c = getopt(argc, argv, "h?rk:")) != -1)
    {
        switch(c)
        {
            case 'r':
                fprintf(stderr, "Entering in restart mode");
                G_sys_config.restarting  = TRUE;
                break;
            case 'k':
                /* No need for random key parsing yet */
                break;
            case 'h': case '?':
                printf("usage: %s [-r restart] [-k random key]\n",
                        argv[0]);
                
                return FAIL;

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
    int ret=SUCCEED;
    char *p;

    /* common env loader will init the debug lib
     * which might call `ps' for process name
     * by popen(). which causes problems with
     * SIGCHLD handlers. Thus handle them after libinit
     */
    if (SUCCEED!=ndrx_load_common_env())
    {
        NDRX_LOG(log_error, "Failed to load common env");
        ret=FAIL;
        goto out;
    }

    /* We will ignore all stuff requesting shutdown! */
    signal(SIGHUP, SIG_IGN);
    signal(SIGTERM, SIG_IGN);
    signal(SIGINT, SIG_IGN);
    signal(SIGCHLD, sign_chld_handler);

    /********* Grab the configuration params  *********/
    G_sys_config.qprefix = getenv(CONF_NDRX_QPREFIX);
    if (NULL==G_sys_config.qprefix)
    {
        /* Write to ULOG? */
        NDRX_LOG(log_error, "Missing config key %s - FAIL", CONF_NDRX_QPREFIX);
        userlog("Missing config key %s - FAIL", CONF_NDRX_QPREFIX);
        ret=FAIL;
        goto out;
    }

    G_sys_config.config_file = getenv(CONF_NDRX_CONFIG);
    if (NULL==G_sys_config.config_file)
    {
        /* Write to ULOG? */
        NDRX_LOG(log_error, "Missing config key %s - FAIL", CONF_NDRX_CONFIG);
        userlog("Missing config key %s - FAIL", CONF_NDRX_CONFIG);
        ret=FAIL;
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
    
    if (SUCCEED!=cmd_processor_init())
    {
        /* Write to ULOG? */
        NDRX_LOG(log_error, "Failed to initialize command processor!");
        userlog("Failed to initailize command processor!");
        ret=FAIL;
        goto out;
    }
    
    /* Read PID file... */
    G_sys_config.pidfile = getenv(CONF_NDRX_DPID);

    if (NULL==G_sys_config.pidfile)
    {
        /* Write to ULOG? */
        NDRX_LOG(log_error, "Missing config key %s - FAIL", CONF_NDRX_DPID);
        userlog("Missing config key %s - FAIL", CONF_NDRX_DPID);
        ret=FAIL;
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
        ret=FAIL;
        goto out;
    }
    else
    {
        NDRX_LOG(log_error, "Using QPath [%s]", G_sys_config.qpath);
    }

    
    /* semaphores must go first!: */
    if (SUCCEED!=ndrxd_sem_init(G_sys_config.qprefix))
    {
        ret=FAIL;
        NDRX_LOG(log_error, "Failed to initialise share memory lib");
        goto out;
    }
    /* and then shm: initialise shared memory */
    if (SUCCEED!=shm_init(G_sys_config.qprefix,
                            ndrx_get_G_atmi_env()->max_servers,
                            ndrx_get_G_atmi_env()->max_svcs))
    {
        ret=FAIL;
        NDRX_LOG(log_error, "Failed to initialise share memory lib");
        goto out;
    }
    
    /* Open shared memory */
    if (G_sys_config.restarting)
    {
        
        if (SUCCEED!=ndrx_sem_attach_all())
        {
            ret=FAIL;
            NDRX_LOG(log_error, "Failed to attach to Semaphores");
            goto out;
        }
        else
        {
            NDRX_LOG(log_error, "Attached to semaphores OK");
        }
        
        if (SUCCEED!=ndrx_shm_attach_all(NDRX_SHM_LEV_SVC | NDRX_SHM_LEV_SRV | NDRX_SHM_LEV_BR))
        {
            ret=FAIL;
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
        if (SUCCEED!=ndrxd_sem_open_all())
        {
            ret=FAIL;
            NDRX_LOG(log_error, "Failed to open semaphores!");
            goto out;
        }
        
        if (SUCCEED!=ndrxd_shm_open_all())
        {
            ret=FAIL;
            NDRX_LOG(log_error, "Failed to open shared memory segments!");
            goto out;
        }
    }
    
#if 0
    /* Do the initialization... */
    if (FAIL==load_config(M_config_file))
    {
        /* Write to ULOG? */
        NDRX_LOG(log_error, "Configuration failed!");
        ret=FAIL;
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
    unlink_pid_file();
    
    return SUCCEED;
}

/*
 * Program main entry
 */
int main(int argc, char** argv) {

    int ret=SUCCEED;
    /* Do some init */
    memset(&G_sys_config, 0, sizeof(G_sys_config));
    
    if (SUCCEED!=init_cmdline_opts(argc, argv))
    {
        NDRX_LOG(log_error, "ndrxd - command line args failed");
        ret=FAIL;
        goto out;
    }
    else if (SUCCEED!=main_init(argc, argv))
    {
        NDRX_LOG(log_error, "ndrxd - INIT FAILED!");
        ret=FAIL;
        goto out;
    }
    /* If we are doing restart, the do actions needed for restart! */
    if (G_sys_config.restarting && SUCCEED!=do_restart_actions())
    {
        ret=FAIL;
        goto out;
    }

    ret=main_loop();

out:

    main_uninit();

    NDRX_LOG(log_error, "exiting %d", ret);

    return ret;
}
