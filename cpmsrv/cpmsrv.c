/* 
** Client Process Monitor Server
**
** @file cpmsrv.c
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

#include "cpmsrv.h"
#include "userlog.h"
  
/*---------------------------Externs------------------------------------*/
extern int optind, optopt, opterr;
extern char *optarg;
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
public cpmsrv_config_t G_config;
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
private int cpm_callback_timer(void);
private int cpm_bc(UBFH *p_ub, int cd);
private int cpm_sc(UBFH *p_ub, int cd);
private int cpm_pc(UBFH *p_ub, int cd);

/**
 * Client Process Monitor, main thread entry 
 * @param p_svc
 */
void CPMSVC (TPSVCINFO *p_svc)
{
    int ret=SUCCEED;
    UBFH *p_ub = (UBFH *)p_svc->data;
    char cmd[2+1];
    char tag[CPM_TAG_LEN]={EOS};
    char subsect[CPM_SUBSECT_LEN]={EOS};
    BFLDLEN len = sizeof(cmd);
    
    p_ub = (UBFH *)tprealloc ((char *)p_ub, Bsizeof (p_ub) + 4096);
    
    
    if (SUCCEED!=Bget(p_ub, EX_CPMCOMMAND, 0, cmd, &len))
    {
        NDRX_LOG(log_error, "missing EX_CPMCOMMAND!");
        FAIL_OUT(ret);
    }
    
    NDRX_LOG(log_info, "Got command: [%s]", cmd);
    
    /* Reload the config */
    if (SUCCEED!=load_config())
    {
        NDRX_LOG(log_error, "Failed to load client config!");
        FAIL_OUT(ret);
    }
    
    if (0==strcmp(cmd, "bc") || 0==strcmp(cmd, "sc"))
    {
        /* get tag & subsect */
        len=sizeof(tag);
        Bget(p_ub, EX_CPMTAG, 0, tag, &len);
        len=sizeof(subsect);
        Bget(p_ub, EX_CPMSUBSECT, 0, subsect, &len);
        
        if (EOS==subsect[0])
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
        FAIL_OUT(ret);
    }
    
    if (SUCCEED!=load_config())
    {
        Bchg(p_ub, EX_CPMOUTPUT, 0, "Failed to load/parse configuration file!", 0L);
        FAIL_OUT(ret);
    }
    
    if (0==strcmp(cmd, "bc"))
    {
        /* boot the process (just mark for startup) */
        if (SUCCEED!=cpm_bc(p_ub, p_svc->cd))
        {
            FAIL_OUT(ret);
        }
    } 
    else if (0==strcmp(cmd, "sc"))
    {
        /* stop the client process */
        if (SUCCEED!=cpm_sc(p_ub, p_svc->cd))
        {
            FAIL_OUT(ret);
        }
    }
    else if (0==strcmp(cmd, "pc"))
    {
        /* Just print the stuff */
        cpm_pc(p_ub, p_svc->cd);
    }

out:
    tpreturn(  ret==SUCCEED?TPSUCCESS:TPFAIL,
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
    int ret=SUCCEED;
    signed char c;
    struct sigaction sa;
    NDRX_LOG(log_debug, "tpsvrinit called");
    
    /* Get the env */
    if (NULL==(G_config.config_file = getenv(CONF_NDRX_CONFIG)))
    {
        NDRX_LOG(log_error, "%s missing env", CONF_NDRX_CONFIG);
        userlog("%s missing env", CONF_NDRX_CONFIG);
        FAIL_OUT(ret);
    }
    
    G_config.chk_interval = FAIL;
    G_config.kill_interval = FAIL;
    
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
    
    if (FAIL==G_config.chk_interval)
    {
        G_config.chk_interval = CLT_CHK_INTERVAL_DEFAULT;
    }
    
    if (FAIL==G_config.kill_interval)
    {
        G_config.chk_interval = CLT_KILL_INTERVAL_DEFAULT;
    }
    
    sa.sa_handler = sign_chld_handler;
    sigemptyset (&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction (SIGCHLD, &sa, 0);

    /* signal(SIGCHLD, sign_chld_handler); */
    
    /* Load initial config */
    if (SUCCEED!=load_config())
    {
        NDRX_LOG(log_error, "Failed to load client config!");
        FAIL_OUT(ret);
    }
    
    if (SUCCEED!=tpadvertise(NDRX_SVC_CPM, CPMSVC))
    {
        NDRX_LOG(log_error, "Failed to initialize CPMSVC!");
        FAIL_OUT(ret);
    }
    
    /* Register callback timer */
    if (SUCCEED!=tpext_addperiodcb(G_config.chk_interval, cpm_callback_timer))
    {
            ret=FAIL;
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
}

/**
 * Callback function for periodic timer
 * We need a timer here cause SIGCHILD causes poller interrupts.
 * @return 
 */
private int cpm_callback_timer(void)
{
    int ret = SUCCEED;
    cpm_process_t *c = NULL;
    cpm_process_t *ct = NULL;
    static int first = TRUE;
    static ndrx_timer_t t;
    
    if (first)
    {
        first = FALSE;
        ndrx_timer_reset(&t);
    }
    
    if (ndrx_timer_get_delta_sec(&t) < G_config.chk_interval)
    {
        goto out;
    }
    
    ndrx_timer_reset(&t);
    
    
    NDRX_LOG(log_debug, "cpm_callback_timer() enter");
            
    /* Mark config as not refreshed */
    HASH_ITER(hh, G_clt_config, c, ct)
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
    return SUCCEED;
}

/**
 * Boot the client process
 * @param p_ub
 * @param cd
 * @return 
 */
private int cpm_bc(UBFH *p_ub, int cd)
{
    int ret = SUCCEED;
    
    char tag[CPM_TAG_LEN+1];
    char subsect[CPM_SUBSECT_LEN+1];
    cpm_process_t * c;
    char debug[256];
    
    if (SUCCEED!=Bget(p_ub, EX_CPMTAG, 0, tag, 0L))
    {
        NDRX_LOG(log_error, "Missing EX_CPMTAG!");
    }
    
    if (SUCCEED!=Bget(p_ub, EX_CPMSUBSECT, 0, subsect, 0L))
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
        FAIL_OUT(ret);
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
private int cpm_sc(UBFH *p_ub, int cd)
{
    int ret = SUCCEED;
    
    char tag[CPM_TAG_LEN+1];
    char subsect[CPM_SUBSECT_LEN+1];
    cpm_process_t * c;
    char debug[256];
    
    if (SUCCEED!=Bget(p_ub, EX_CPMTAG, 0, tag, 0L))
    {
        FAIL_OUT(ret);
        NDRX_LOG(log_error, "Missing EX_CPMTAG!");
    }
    
    if (SUCCEED!=Bget(p_ub, EX_CPMSUBSECT, 0, subsect, 0L))
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
        FAIL_OUT(ret);
    }
    else
    {
        c->dyn.req_state = CLT_STATE_NOTRUN;
        
        if (CLT_STATE_STARTED ==  c->dyn.cur_state)
        {
            if (SUCCEED==cpm_kill(c))
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
private int cpm_pc(UBFH *p_ub, int cd)
{
    int ret = SUCCEED;
    long revent;
    cpm_process_t *c = NULL;
    cpm_process_t *ct = NULL;
    char output[256];
    char buffer [80];
    struct tm * timeinfo;
    
    NDRX_LOG(log_info, "cpm_pc: listing clients");
    /* Remove dead un-needed processes (killed & not in new config) */
    HASH_ITER(hh, G_clt_config, c, ct)
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
        
        if (SUCCEED!=Bchg(p_ub, EX_CPMOUTPUT, 0, output, 0L))
        {
            NDRX_LOG(log_error, "Failed to read fields: [%s]", 
                Bstrerror(Berror));
            FAIL_OUT(ret);
        }
        
        if (FAIL == tpsend(cd,
                            (char *)p_ub,
                            0L,
                            0,
                            &revent))
        {
            NDRX_LOG(log_error, "Send data failed [%s] %ld",
                                tpstrerror(tperrno), revent);
            FAIL_OUT(ret);
        }
        else
        {
            NDRX_LOG(log_debug,"sent ok");
        }
    }
    
out:

    return ret;
}
