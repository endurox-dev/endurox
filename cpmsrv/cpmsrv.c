/* 
** Client Process Monitor Server
**
** @file cpmsrv.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, Mavimax, Ltd. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or Mavimax's license for commercial use.
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
** A commercial use license is available from Mavimax, Ltd
** contact@mavimax.com
** -----------------------------------------------------------------------------
*/
#include <ndrx_config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <regex.h>
#include <utlist.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include <ndebug.h>
#include <atmi.h>
#include <atmi_int.h>
#include <typed_buf.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <Exfields.h>
#include <nclopt.h>
#include <signal.h>
#include "cpmsrv.h"
#include "userlog.h"
  
/*---------------------------Externs------------------------------------*/
extern int optind, optopt, opterr;
extern char *optarg;
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
expublic cpmsrv_config_t G_config;
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
exprivate int cpm_callback_timer(void);
exprivate int cpm_bc(UBFH *p_ub, int cd);
exprivate int cpm_sc(UBFH *p_ub, int cd);
exprivate int cpm_pc(UBFH *p_ub, int cd);

/**
 * Client Process Monitor, main thread entry 
 * @param p_svc
 */
void CPMSVC (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;
    UBFH *p_ub = (UBFH *)p_svc->data;
    char cmd[2+1];
    char tag[CPM_TAG_LEN]={EXEOS};
    char subsect[CPM_SUBSECT_LEN]={EXEOS};
    BFLDLEN len = sizeof(cmd);
    
    p_ub = (UBFH *)tprealloc ((char *)p_ub, Bsizeof (p_ub) + 4096);
    
    
    if (EXSUCCEED!=Bget(p_ub, EX_CPMCOMMAND, 0, cmd, &len))
    {
        NDRX_LOG(log_error, "missing EX_CPMCOMMAND!");
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_info, "Got command: [%s]", cmd);
    
    if (0==strcmp(cmd, "bc") || 0==strcmp(cmd, "sc"))
    {
        /* get tag & subsect */
        len=sizeof(tag);
        Bget(p_ub, EX_CPMTAG, 0, tag, &len);
        len=sizeof(subsect);
        Bget(p_ub, EX_CPMSUBSECT, 0, subsect, &len);
        
        if (EXEOS==subsect[0])
        {
            strcpy(subsect, "-");
        }
    }
    else if (0==strcmp(cmd, "pc"))
    {
        /* Just print the stuff */
    }
    else 
    {
        Bchg(p_ub, EX_CPMOUTPUT, 0, "Invalid command!", 0L);
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=load_config())
    {
        Bchg(p_ub, EX_CPMOUTPUT, 0, "Failed to load/parse configuration file!", 0L);
        EXFAIL_OUT(ret);
    }
    
    if (0==strcmp(cmd, "bc"))
    {
        /* boot the process (just mark for startup) */
        if (EXSUCCEED!=cpm_bc(p_ub, p_svc->cd))
        {
            EXFAIL_OUT(ret);
        }
    } 
    else if (0==strcmp(cmd, "sc"))
    {
        /* stop the client process */
        if (EXSUCCEED!=cpm_sc(p_ub, p_svc->cd))
        {
            EXFAIL_OUT(ret);
        }
    }
    else if (0==strcmp(cmd, "pc"))
    {
        /* Just print the stuff */
        cpm_pc(p_ub, p_svc->cd);
    }

out:
    tpreturn(  ret==EXSUCCEED?TPSUCCESS:TPFAIL,
                0,
                (char *)p_ub,
                0L,
                0L);
}

/**
 * Do initialization
 */
int NDRX_INTEGRA(tpsvrinit)(int argc, char **argv)
{
    int ret=EXSUCCEED;
    signed char c;
    struct sigaction sa;
    sigset_t wanted; 
    NDRX_LOG(log_debug, "tpsvrinit called");
    
    /* Get the env */
    if (NULL==(G_config.config_file = getenv(CONF_NDRX_CONFIG)))
    {
        NDRX_LOG(log_error, "%s missing env", CONF_NDRX_CONFIG);
        userlog("%s missing env", CONF_NDRX_CONFIG);
        EXFAIL_OUT(ret);
    }
    
    G_config.chk_interval = EXFAIL;
    G_config.kill_interval = EXFAIL;
    
    /* Parse command line  */
    while ((c = getopt(argc, argv, "i:k:")) != -1)
    {
        NDRX_LOG(log_debug, "%c = [%s]", c, optarg);
        switch(c)
        {
            case 'i': 
                G_config.chk_interval = atoi(optarg);
                break;
            case 'k': 
                G_config.kill_interval = atoi(optarg);
                break;
            default:
                /*return FAIL;*/
                break;
        }
    }
    
    if (EXFAIL==G_config.chk_interval)
    {
        G_config.chk_interval = CLT_CHK_INTERVAL_DEFAULT;
    }
    
    if (EXFAIL==G_config.kill_interval)
    {
        G_config.chk_interval = CLT_KILL_INTERVAL_DEFAULT;
    }
#if 0
    /* < seems with out this, sigaction on linux does not work... >*/
    sigemptyset(&wanted); 

    sigaddset(&wanted, SIGCHLD); 
    if (EXSUCCEED!=pthread_sigmask(SIG_UNBLOCK, &wanted, NULL) )
    {
        NDRX_LOG(log_error, "pthread_sigmask failed for SIGCHLD: %s", strerror(errno));
        EXFAIL_OUT(ret);
    }
    /* </ seems with out this, sigaction on linux does not work... >*/
    
    sa.sa_handler = sign_chld_handler;
    sigemptyset (&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (EXFAIL==sigaction (SIGCHLD, &sa, 0))
    {
        NDRX_LOG(log_error, "sigaction failed for SIGCHLD: %s", strerror(errno));
        EXFAIL_OUT(ret);
    }
#endif

#ifndef EX_CPM_NO_THREADS
    ndrxd_sigchld_init();
#endif
    /* signal(SIGCHLD, sign_chld_handler); */
    
    /* Load initial config */
    if (EXSUCCEED!=load_config())
    {
        NDRX_LOG(log_error, "Failed to load client config!");
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=tpadvertise(NDRX_SVC_CPM, CPMSVC))
    {
        NDRX_LOG(log_error, "Failed to initialize CPMSVC!");
        EXFAIL_OUT(ret);
    }
    
    /* Register callback timer */
    if (EXSUCCEED!=tpext_addperiodcb(G_config.chk_interval, cpm_callback_timer))
    {
            ret=EXFAIL;
            NDRX_LOG(log_error, "tpext_addperiodcb failed: %s",
                            tpstrerror(tperrno));
    }
    
    NDRX_LOG(log_info, "Config file: [%s]", G_config.config_file );
    NDRX_LOG(log_info, "Process checking interval (-i): [%d]", G_config.chk_interval);
    NDRX_LOG(log_info, "Process kill interval (-i): [%d]", G_config.kill_interval);
    
    /* Process the timer now.... */
    cpm_start_all(); /* Mark all to be started */
    cpm_callback_timer(); /* Start them all. */
    
out:
    return ret;
}

/**
 * Do de-initialization
 */
void NDRX_INTEGRA(tpsvrdone)(void)
{
    cpm_process_t *c = NULL;
    cpm_process_t *ct = NULL;
    
    NDRX_LOG(log_debug, "tpsvrdone called - shutting down client processes...");
    
    cpm_killall();
    
#ifndef EX_CPM_NO_THREADS
    ndrxd_sigchld_uninit();
#endif

}

/**
 * Callback function for periodic timer
 * We need a timer here cause SIGCHILD causes poller interrupts.
 * @return 
 */
exprivate int cpm_callback_timer(void)
{
    int ret = EXSUCCEED;
    cpm_process_t *c = NULL;
    cpm_process_t *ct = NULL;
    static int first = EXTRUE;
    static ndrx_stopwatch_t t;
    
    if (first)
    {
        first = EXFALSE;
        ndrx_stopwatch_reset(&t);
    }

#ifdef EX_CPM_NO_THREADS
    /* Process any dead child... */
    sign_chld_handler(SIGCHLD);
#endif
    
    if (ndrx_stopwatch_get_delta_sec(&t) < G_config.chk_interval)
    {
        goto out;
    }
    
    ndrx_stopwatch_reset(&t);
    
    
    NDRX_LOG(log_debug, "cpm_callback_timer() enter");
            
    /* Mark config as not refreshed */
    EXHASH_ITER(hh, G_clt_config, c, ct)
    {
        NDRX_LOG(log_debug, "%s/%s req %d cur %d", 
                c->tag, c->subsect, c->dyn.req_state, c->dyn.cur_state);
        
        if ((CLT_STATE_NOTRUN==c->dyn.cur_state ||
                CLT_STATE_STARTING==c->dyn.cur_state)  &&
                CLT_STATE_STARTED==c->dyn.req_state)
        {
            /* Try to boot the process... */
            cpm_exec(c);
        }
    }
    /* handle any signal 
    sign_chld_handler(SIGCHLD); */
    
out:
    return EXSUCCEED;
}

/**
 * Boot the client process
 * @param p_ub
 * @param cd
 * @return 
 */
exprivate int cpm_bc(UBFH *p_ub, int cd)
{
    int ret = EXSUCCEED;
    
    char tag[CPM_TAG_LEN+1];
    char subsect[CPM_SUBSECT_LEN+1];
    cpm_process_t * c;
    char debug[256];
    
    if (EXSUCCEED!=Bget(p_ub, EX_CPMTAG, 0, tag, 0L))
    {
        NDRX_LOG(log_error, "Missing EX_CPMTAG!");
    }
    
    if (EXSUCCEED!=Bget(p_ub, EX_CPMSUBSECT, 0, subsect, 0L))
    {
        strcpy(subsect, "-");
    }
    
    c = cpm_client_get(tag, subsect);
    
    if (NULL==c)
    {
        sprintf(debug, "Client process %s/%s not found!", tag, subsect);
        NDRX_LOG(log_error, "%s", debug);
        userlog("%s!", debug);
        Bchg(p_ub, EX_CPMOUTPUT, 0, debug, 0L);
        EXFAIL_OUT(ret);
    }
    else
    {
        if (CLT_STATE_STARTED != c->dyn.cur_state)
        {
            sprintf(debug, "Client process %s/%s marked for start", tag, subsect);
            NDRX_LOG(log_info, "%s", debug);
            Bchg(p_ub, EX_CPMOUTPUT, 0, debug, 0L);

            c->dyn.cur_state = CLT_STATE_STARTING;
            c->dyn.req_state = CLT_STATE_STARTED;
        }
        else
        {
            sprintf(debug, "Process %s/%s already marked "
                                "for startup or running...", tag, subsect);
            NDRX_LOG(log_info, "%s", debug);
            Bchg(p_ub, EX_CPMOUTPUT, 0, debug, 0L);
        }
    }
    
out:
    return ret;
}


/**
 * Stop the client process
 * @param p_ub
 * @param cd
 * @return 
 */
exprivate int cpm_sc(UBFH *p_ub, int cd)
{
    int ret = EXSUCCEED;
    
    char tag[CPM_TAG_LEN+1];
    char subsect[CPM_SUBSECT_LEN+1];
    cpm_process_t * c;
    char debug[256];
    
    if (EXSUCCEED!=Bget(p_ub, EX_CPMTAG, 0, tag, 0L))
    {
        NDRX_LOG(log_error, "Missing EX_CPMTAG!");
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=Bget(p_ub, EX_CPMSUBSECT, 0, subsect, 0L))
    {
        strcpy(subsect, "-");
    }
    
    c = cpm_client_get(tag, subsect);
    
    if (NULL==c)
    {
        sprintf(debug, "Client process %s/%s not found!", tag, subsect);
        NDRX_LOG(log_error, "%s", debug);
        userlog("%s!", debug);
        Bchg(p_ub, EX_CPMOUTPUT, 0, debug, 0L);
        EXFAIL_OUT(ret);
    }
    else
    {
        c->dyn.req_state = CLT_STATE_NOTRUN;
        
        if (CLT_STATE_STARTED ==  c->dyn.cur_state)
        {
            if (EXSUCCEED==cpm_kill(c))
            {
                sprintf(debug, "Client process %s/%s stopped", tag, subsect);
                NDRX_LOG(log_info, "%s", debug);
                Bchg(p_ub, EX_CPMOUTPUT, 0, debug, 0L);
            }
            else
            {
                sprintf(debug, "Failed to stop %s/%s!", tag, subsect);
                NDRX_LOG(log_info, "%s", debug);
                Bchg(p_ub, EX_CPMOUTPUT, 0, debug, 0L);
            }
        }
        else
        {
            sprintf(debug, "Client process %s/%s not running already...", 
                    tag, subsect);
            NDRX_LOG(log_info, "%s", debug);
            Bchg(p_ub, EX_CPMOUTPUT, 0, debug, 0L);
        }
    }
    
out:
    return ret;
}

/**
 * Print clients
 * @param p_ub
 * @param cd - call descriptor
 * @return 
 */
exprivate int cpm_pc(UBFH *p_ub, int cd)
{
    int ret = EXSUCCEED;
    long revent;
    cpm_process_t *c = NULL;
    cpm_process_t *ct = NULL;
    char output[256];
    char buffer [80];
    struct tm * timeinfo;
    
    NDRX_LOG(log_info, "cpm_pc: listing clients");
    /* Remove dead un-needed processes (killed & not in new config) */
    EXHASH_ITER(hh, G_clt_config, c, ct)
    {
        
        NDRX_LOG(log_info, "cpm_pc: %s/%s", c->tag, c->subsect);
        
        timeinfo = localtime (&c->dyn.stattime);
        strftime (buffer,80,"%c",timeinfo);
    
        if (CLT_STATE_STARTED == c->dyn.cur_state)
        {
            sprintf(output, "%s/%s - running pid %d (%s)",
                                c->tag, c->subsect, c->dyn.pid, buffer);
        }
        else if (CLT_STATE_STARTING == c->dyn.cur_state && 
                c->dyn.req_state != CLT_STATE_NOTRUN)
        {
            sprintf(output, "%s/%s - starting (%s)",c->tag, c->subsect, buffer);
        }
        else if (c->dyn.was_started && (c->dyn.req_state == CLT_STATE_STARTED) )
        {
            sprintf(output, "%s/%s - dead %d (%s)", c->tag, c->subsect, 
                    c->dyn.exit_status, buffer);
        }
        else if (c->dyn.was_started && (c->dyn.req_state == CLT_STATE_NOTRUN) )
        {
            sprintf(output, "%s/%s - shutdown (%s)", c->tag, c->subsect, 
                    buffer);
        }
        else
        {
            sprintf(output, "%s/%s - not started", c->tag, c->subsect);
        }
        
        if (EXSUCCEED!=Bchg(p_ub, EX_CPMOUTPUT, 0, output, 0L))
        {
            NDRX_LOG(log_error, "Failed to read fields: [%s]", 
                Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
        
        if (EXFAIL == tpsend(cd,
                            (char *)p_ub,
                            0L,
                            0,
                            &revent))
        {
            NDRX_LOG(log_error, "Send data failed [%s] %ld",
                                tpstrerror(tperrno), revent);
            EXFAIL_OUT(ret);
        }
        else
        {
            NDRX_LOG(log_debug,"sent ok");
        }
    }
    
out:

    return ret;
}
