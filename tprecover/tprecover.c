/* 
** ndrxd monitor & recover process.
**
** @file tprecover.c
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
static long M_check = FAIL;
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
    int ret=SUCCEED;
    UBFH *p_ub = (UBFH *)p_svc->data;

    /* TODO! */
    
    NDRX_LOG(log_debug, "TPRECOVER got call");
    Bfprint(p_ub, stderr);

    Bchg(p_ub, EXDM_RESTARTS, 1, (char *)&M_restarts, 0L);

    
out:
    tpreturn(  ret==SUCCEED?TPSUCCESS:TPFAIL,
                0,
                (char *)p_ub,
                0L,
                0L);
}

/**
 * Periodic poll callback.
 * @return 
 */
public int poll_timer(void)
{
    int ret=SUCCEED;
    if (!ndrx_chk_ndrxd())
    {
        NDRX_LOG(log_error, "ndrxd process missing - respawn!");
        if (SUCCEED!=start_daemon_recover())
        {
           FAIL_OUT(ret);
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
    int ret=SUCCEED;
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
        if (NULL==(f=fopen(ndrxd_logfile, "w+")))
        {
            fprintf(stderr, "Failed to open ndrxd log file: %s\n",
                    ndrxd_logfile);
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
int tpsvrinit(int argc, char **argv)
{
    int ret=SUCCEED;
    signed char c;

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
                NDRX_LOG(log_error, "Unknown param %c", c);
                FAIL_OUT(ret);
                break;
        }
    }

    signal(SIGCHLD, handle_sigchld);

    /* Register timer check.... */
    if (SUCCEED!=tpext_addperiodcb((int)M_check, poll_timer))
    {
        NDRX_LOG(log_error, "tpext_addperiodcb failed: %s",
            tpstrerror(tperrno));
        FAIL_OUT(ret);
    }

    if (SUCCEED!=tpadvertise(NDRX_SYS_SVC_PFX TPRECOVERSVC, TPRECOVER))
    {
        NDRX_LOG(log_error, "Failed to initialize TPRECOVER!");
        FAIL_OUT(ret);
    }

out:
    return ret;
}

