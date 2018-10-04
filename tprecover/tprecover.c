/**
 * @brief ndrxd monitor & recover process.
 *
 * @file tprecover.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * AGPL or Mavimax's license for commercial use.
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <regex.h>
#include <utlist.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#include <ndebug.h>
#include <atmi.h>
#include <atmi_int.h>
#include <typed_buf.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <Exfields.h>

#include "tprecover.h"
/*---------------------------Externs------------------------------------*/
extern int optind, optopt, opterr;
extern char *optarg;
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
static long M_restarts = 0;
static long M_check = 5; /* defaulted to 5 sec */
/*---------------------------Prototypes---------------------------------*/
int start_daemon_recover(void);

/**
 * Monitor ndrxd & recover it if needed.
 * 1. could monitor process existance
 * 2. could do periodical pings to ndrxd for status
 * @param p_svc
 */
void TPRECOVER (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;
    UBFH *p_ub = (UBFH *)p_svc->data;
    
    NDRX_LOG(log_debug, "TPRECOVER got call");
    Bfprint(p_ub, stderr);

    Bchg(p_ub, EXDM_RESTARTS, 1, (char *)&M_restarts, 0L);

    
out:
    tpreturn(  ret==EXSUCCEED?TPSUCCESS:TPFAIL,
                0,
                (char *)p_ub,
                0L,
                0L);
}

/**
 * Periodic poll callback.
 * TODO: We might want to run pings here to ndrxd. If ping fails several times
 * we kill the ndrxd and restart it...!
 * @return 
 */
expublic int poll_timer(void)
{
    int ret=EXSUCCEED;
    if (!ndrx_chk_ndrxd())
    {
        NDRX_LOG(log_error, "ndrxd process missing - respawn!");
        if (EXSUCCEED!=start_daemon_recover())
        {
           EXFAIL_OUT(ret);
        }
    }
    else
    {
        /* todo: might want to check for queue... */
        NDRX_LOG(log_debug, "ndrxd process ok");
    }

out:
    return ret;
}

/**
 * Start ndrxd daemon in recovery mode.
 */
int start_daemon_recover(void)
{
    int ret=EXSUCCEED;
    pid_t pid;
    char    key[NDRX_MAX_KEY_SIZE+3+1];
    /* Log filename for ndrxd */
    char *ndrxd_logfile = getenv(CONF_NDRX_DMNLOG);
    /* clone our self */
    pid = fork();
    
    if( pid == 0)
    {
        FILE *f;
        /* this is child - start EnduroX back-end*/
        sprintf(key, NDRX_KEY_FMT, G_atmi_env.rnd_key);
        char *cmd[] = { "ndrxd", key, "-r", (char *)0 };

        /* Open log file */
        if (NULL==(f=NDRX_FOPEN(ndrxd_logfile, "a")))
        {
            fprintf(stderr, "Failed to open ndrxd log file: %s\n",
                    ndrxd_logfile);
        }
        else
        {
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
        M_restarts++;
        NDRX_LOG(log_error, "Started ndrxd PID %d", pid);
    }
out:
    return ret;
}

/**
 * Discard the deadly ndrxds
 */
void handle_sigchld(int sig)
{
    while (waitpid((pid_t)(-1), 0, WNOHANG) > 0) {}
    signal(SIGCHLD, handle_sigchld);
}

/*
 * Do initialization
 */
int NDRX_INTEGRA(tpsvrinit)(int argc, char **argv)
{
    int ret=EXSUCCEED;
    int c;
    extern char *optarg;

    NDRX_LOG(log_debug, "tpsvrinit called");
    /* Parse command line  */
    while((c = getopt(argc, argv, "c:")) != -1)
    {
        NDRX_LOG(log_debug, "%c = [%s]", c, optarg);
        switch(c)
        {
            case 'c':
                M_check = atoi(optarg);
                NDRX_LOG(log_debug, "check (-c): %d", 
                        M_check);
                break;
            default:
                NDRX_LOG(log_error, "Unknown param %c - 0x%x", c, c);
		EXFAIL_OUT(ret);
                break;
        }
    }

    signal(SIGCHLD, handle_sigchld);

    /* Register timer check.... */
    NDRX_LOG(log_warn, "Config: ndrxd check time: %d sec", M_check);
    if (EXSUCCEED!=tpext_addperiodcb((int)M_check, poll_timer))
    {
        NDRX_LOG(log_error, "tpext_addperiodcb failed: %s",
            tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }

    if (EXSUCCEED!=tpadvertise(NDRX_SYS_SVC_PFX TPRECOVERSVC, TPRECOVER))
    {
        NDRX_LOG(log_error, "Failed to initialize TPRECOVER!");
        EXFAIL_OUT(ret);
    }

out:
    return ret;
}

void NDRX_INTEGRA(tpsvrdone)(void)
{
    /* just for build... */
}
/* vim: set ts=4 sw=4 et smartindent: */
