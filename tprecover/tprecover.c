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
static long M_check = 5; /**< defaulted to 5 sec */
/**
 * ping timeout... (seconds to wait for ping) 
 * WARNING ! As the process will be blocked in this time
 * the backpings from ndrxd may stall too. Thus check ndrxconfig.xml *ping_max*
 * setting, as ndrxd might kill the tprecover in this time.
 * But usually ndrxd shall be fast to respond, thus ping timeout to 20 should
 * be fine.
 */
static int M_ping_tout = 20;
static int M_ping_max = 3; /**< max ping attempts with out success to kill ndrxd */

static int M_bad_pings = 0; /**< bad pings reset at exec */
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
 * We might want to run pings here to ndrxd. If ping fails several times
 * we kill the ndrxd and restart it...!
 * @return 
 */
expublic int poll_timer(void)
{
    int ret=EXSUCCEED;
    int seq;
    long tim;
    int ndrxd_stat = ndrx_chk_ndrxd();
    
    if (M_bad_pings > M_ping_max && !ndrxd_stat)
    {
        NDRX_LOG(log_always, "WARNING ! bad_pings=%d ping_max=%d and "
                "ndrxd not running: respawn", 
                M_bad_pings, M_ping_max);
        
        if (EXSUCCEED!=start_daemon_recover())
        {
           EXFAIL_OUT(ret);
        }
        
        M_bad_pings = 0;
    }
    else if (!ndrxd_stat)
    {
        M_bad_pings++;
        NDRX_LOG(log_always, "ndrxd not present (or resources issue for process listing...)"
                    "increase bad_pings=%d ping_max=%d", 
                    M_bad_pings, M_ping_max);
    }
    else
    {
        /* perform ping of ndrxd... */
        
        if (EXSUCCEED!=ndrx_ndrxd_ping(&seq, &tim, ndrx_get_G_atmi_conf()->reply_q, 
                ndrx_get_G_atmi_conf()->reply_q_str))
        {
            M_bad_pings++;
            
            NDRX_LOG(log_info, "ndrxd_ping_seq=%d bad_pings=%d: timeout or system error", 
                    seq, M_bad_pings);
        }
        else
        {
            NDRX_LOG(log_error, "ndrxd_ping_seq=%d time=%ld ms", seq, tim);
        }
        
        if (M_bad_pings > M_ping_max)
        {
            /* get ndrxd pid... */
            pid_t ndrxd_pid = ndrx_ndrxd_pid_get();
            NDRX_LOG(log_always, "WARNING ! bad_pings=%d ping_max=%d -> kill %d %d", 
                    M_bad_pings, M_ping_max, SIGKILL, (int)ndrxd_pid);
            if (EXSUCCEED!=kill(ndrxd_pid, SIGKILL))
            {
                NDRX_LOG(log_error, "Failed to kill %d: %s", 
                        (int)ndrxd_pid, strerror(errno));
            }
        }
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
    pid = ndrx_fork();
    
    if( pid == 0)
    {
        FILE *f;
        /* this is child - start EnduroX back-end */
        snprintf(key, sizeof(key), NDRX_KEY_FMT, G_atmi_env.rnd_key);
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
    while((c = getopt(argc, argv, "c:t:m:")) != -1)
    {
        NDRX_LOG(log_debug, "%c = [%s]", c, optarg);
        switch(c)
        {
            case 'c':
                M_check = atoi(optarg);
                NDRX_LOG(log_debug, "check (-c): %d", 
                        M_check);
                break;
            case 't':
                M_ping_tout = atoi(optarg);
                break;
            case 'm':
                M_ping_max = atoi(optarg);
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
    NDRX_LOG(log_warn, "Config: ndrxd ping timeout: %d sec", M_ping_tout);
    NDRX_LOG(log_warn, "Config: max pings for kill ndrxd: %d", M_ping_max);
    
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
    
    if (EXSUCCEED!=tptoutset(M_ping_tout))
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
