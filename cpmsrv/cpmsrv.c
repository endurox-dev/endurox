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
#include <exregex.h>
  
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
exprivate int cpm_rc(UBFH *p_ub, int cd);

exprivate int cpm_bc_obj(UBFH *p_ub, int cd, char *tag, char *subsect, cpm_process_t * c, int *p_nr_proc);
exprivate int cpm_sc_obj(UBFH *p_ub, int cd, char *tag, char *subsect, cpm_process_t * c, int *p_nr_proc);
exprivate int cpm_rc_obj(UBFH *p_ub, int cd, char *tag, char *subsect, cpm_process_t * c, int *p_nr_proc);

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
    
    if (0==strcmp(cmd, "bc") || 0==strcmp(cmd, "sc") || 0==strcmp(cmd, "rc"))
    {
        /* get tag & subsect */
        len=sizeof(tag);
        Bget(p_ub, EX_CPMTAG, 0, tag, &len);
        len=sizeof(subsect);
        Bget(p_ub, EX_CPMSUBSECT, 0, subsect, &len);
        
        if (EXEOS==subsect[0])
        {
            NDRX_STRCPY_SAFE(subsect, "-");
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
    else if (0==strcmp(cmd, "rc"))
    {
        /* Just print the stuff */
        cpm_rc(p_ub, p_svc->cd);
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
 * Send message user.
 * @param p_ub
 * @param cd
 * @param msg
 */
exprivate void cpm_send_msg(UBFH *p_ub, int cd, char *msg)
{
    long revent;
    
    Bchg(p_ub, EX_CPMOUTPUT, 0, msg, 0L);
    
    if (EXFAIL == tpsend(cd,
                        (char *)p_ub,
                        0L,
                        0,
                        &revent))
    {
        NDRX_LOG(log_error, "Send data failed [%s] %ld",
                            tpstrerror(tperrno), revent);
    }
    else
    {
        NDRX_LOG(log_debug,"sent ok");
    }
}

/**
 * Stop single client
 * @param p_ub
 * @param cd
 * @param tag
 * @param subsect
 * @param c
 * @return EXSUCCEED/EXFAIL
 */
exprivate int cpm_sc_obj(UBFH *p_ub, int cd, char *tag, char *subsect, 
        cpm_process_t * c, int *p_nr_proc)
{
    int ret = EXSUCCEED;
    long revent;
    char debug[256];
    
    c = cpm_client_get(tag, subsect);
    
    if (NULL==c)
    {
        snprintf(debug, sizeof(debug), "Client process %s/%s not found!", 
                tag, subsect);
        NDRX_LOG(log_error, "%s", debug);
        userlog("%s!", debug);
        Bchg(p_ub, EX_CPMOUTPUT, 0, debug, 0L);
        EXFAIL_OUT(ret);
    }
    else
    {
        (*p_nr_proc)++;
        
        c->dyn.req_state = CLT_STATE_NOTRUN;
        
        if (CLT_STATE_STARTED ==  c->dyn.cur_state)
        {
            if (EXSUCCEED==cpm_kill(c))
            {
                snprintf(debug, sizeof(debug), "Client process %s/%s stopped", 
                        tag, subsect);
                NDRX_LOG(log_info, "%s", debug);
                Bchg(p_ub, EX_CPMOUTPUT, 0, debug, 0L);
            }
            else
            {
                snprintf(debug, sizeof(debug), "Failed to stop %s/%s!", 
                        tag, subsect);
                NDRX_LOG(log_info, "%s", debug);
                Bchg(p_ub, EX_CPMOUTPUT, 0, debug, 0L);
            }
        }
        else
        {
            snprintf(debug, sizeof(debug), "Client process %s/%s not running already...", 
                    tag, subsect);
            NDRX_LOG(log_info, "%s", debug);
            Bchg(p_ub, EX_CPMOUTPUT, 0, debug, 0L);
        }
    }
out:

    if (EXFAIL == tpsend(cd,
                        (char *)p_ub,
                        0L,
                        0,
                        &revent))
    {
        NDRX_LOG(log_error, "Send data failed [%s] %ld",
                            tpstrerror(tperrno), revent);
    }
    else
    {
        NDRX_LOG(log_debug,"sent ok");
    }

                
    return ret;
}

/**
 * Reboot the client
 * @param p_ub
 * @param cd
 * @param tag
 * @param subsect
 * @param c
 * @return 
 */
exprivate int cpm_rc_obj(UBFH *p_ub, int cd, char *tag, char *subsect, 
        cpm_process_t * c, int *p_nr_proc)
{   
    int ret = EXSUCCEED;
    int dum;
    /* will do over those binary which are request to be started... */
    if (CLT_STATE_STARTED ==  c->dyn.req_state)
    {
        (*p_nr_proc)++;
        /* restart if any running... */
        NDRX_LOG(log_debug, "[%s]/[%s] running - restarting...", tag, subsect);
        
        NDRX_LOG(log_debug, "About to stop...");
        if (EXSUCCEED!=cpm_sc_obj(p_ub, cd, tag, subsect, c, &dum))
        {
            NDRX_LOG(log_error, "Failed to stop [%s]/[%s]", tag, subsect);
            EXFAIL_OUT(ret);
        }
        
        NDRX_LOG(log_debug, "About to start...");
        if (EXSUCCEED!=cpm_bc_obj(p_ub, cd, tag, subsect, c, &dum))
        {
            NDRX_LOG(log_error, "Failed to start [%s]/[%s]", tag, subsect);
            EXFAIL_OUT(ret);
        }
        
        
    }
out:
    return ret;
}

/**
 * Process single object and send the results to client
 * @param p_ub
 * @param tag
 * @param subsect
 * @param c
 * @return 
 */
exprivate int cpm_bc_obj(UBFH *p_ub, int cd, char *tag, char *subsect, 
        cpm_process_t * c, int *p_nr_proc)
{
    int ret = EXSUCCEED;
    long revent;
    char debug[256];
    
    NDRX_LOG(log_debug, "Into %s: p_ub=%p, cd=%d, tag=[%s] subsect=[%s], c=%p",
            __func__, p_ub, cd, tag, subsect, c);
    
    if (NULL==c)
    {
        snprintf(debug, sizeof(debug), "Client process %s/%s not found!", 
                tag, subsect);
        NDRX_LOG(log_error, "%s", debug);
        userlog("%s!", debug);
        Bchg(p_ub, EX_CPMOUTPUT, 0, debug, 0L);
        EXFAIL_OUT(ret);
    }
    else
    {
        (*p_nr_proc)++;
        
        if (CLT_STATE_STARTED != c->dyn.cur_state)
        {
            snprintf(debug, sizeof(debug), "Client process %s/%s marked for start", 
                    tag, subsect);
            NDRX_LOG(log_info, "%s", debug);
            Bchg(p_ub, EX_CPMOUTPUT, 0, debug, 0L);

            c->dyn.cur_state = CLT_STATE_STARTING;
            c->dyn.req_state = CLT_STATE_STARTED;
        }
        else
        {
            snprintf(debug, sizeof(debug), "Process %s/%s already marked "
                                "for startup or running...", tag, subsect);
            NDRX_LOG(log_info, "%s", debug);
            Bchg(p_ub, EX_CPMOUTPUT, 0, debug, 0L);
        }
    }
    
out:
         
    if (EXFAIL == tpsend(cd,
                        (char *)p_ub,
                        0L,
                        0,
                        &revent))
    {
        NDRX_LOG(log_error, "Send data failed [%s] %ld",
                            tpstrerror(tperrno), revent);
    }
    else
    {
        NDRX_LOG(log_debug,"sent ok");
    }

                
    return ret;
}

/**
 * Start/Stop client the client process
 * @param p_ub
 * @param cd
 * @return 
 */
exprivate int cpm_bcscrc(UBFH *p_ub, int cd, 
        int(*p_func)(UBFH *, int, char*, char*,cpm_process_t *, int *p_nr_proc), 
        char *finish_msg)
{
    long twait = 0;
    int ret = EXSUCCEED;
    char msg[256];
    cpm_process_t *c = NULL, *ct = NULL;
    
    char tag[CPM_TAG_LEN+1];
    char subsect[CPM_SUBSECT_LEN+1];
    
    char regex_tag[CPM_TAG_LEN * 2 + 2 + 1]; /* all symbols can be escaped, 
                                            * have ^$ start/end and EOS */
    char regex_subsect[CPM_SUBSECT_LEN * 2 + 2 + 1];
    
    regex_t r_comp_tag;
    int r_comp_tag_alloc = EXFALSE;
    
    regex_t r_comp_subsect;
    int r_comp_subsect_alloc = EXFALSE;
    
    int nr_proc = 0;
    
    if (EXSUCCEED!=Bget(p_ub, EX_CPMTAG, 0, tag, 0L))
    {
        NDRX_LOG(log_error, "Missing EX_CPMTAG!");
    }
    
    if (EXSUCCEED!=Bget(p_ub, EX_CPMSUBSECT, 0, subsect, 0L))
    {
        NDRX_STRCPY_SAFE(subsect, "-");
    }
    
    Bget(p_ub, EX_CPMWAIT, 0, (char *)&twait, 0L);
    
    if (NULL==strchr(tag,CLT_WILDCARD) && NULL==strchr(subsect, CLT_WILDCARD))
    {
        c = cpm_client_get(tag, subsect);
        if (EXSUCCEED!=(p_func(p_ub, cd, tag, subsect, c, &nr_proc)))
        {
            NDRX_LOG(log_error, "%s: cpm_bc_obj failed", __func__);
            EXFAIL_OUT(ret);
        }
    }
    else
    {
        /* Expand to regular expressions... */
        ndrx_regasc_cpyesc(regex_tag, tag, '^', '$', '%', ".*");
        NDRX_LOG(log_debug, "Got regex tag: [%s]", tag);
        if (EXSUCCEED!=ndrx_regcomp(&r_comp_tag, regex_tag))
        {
            NDRX_LOG(log_error, "Failed to compile regexp of tag!");
            
            snprintf(msg, sizeof(msg), "Failed to compile regexp of tag[%s]!",
                    regex_tag);    
            cpm_send_msg(p_ub, cd, msg);
        }
        r_comp_tag_alloc=EXTRUE;
        
        
        ndrx_regasc_cpyesc(regex_subsect, subsect, '^', '$', '%', ".*");
        NDRX_LOG(log_debug, "Got regex subsect: [%s]", subsect);
        
        if (EXSUCCEED!=ndrx_regcomp(&r_comp_subsect, regex_subsect))
        {
            NDRX_LOG(log_error, "Failed to compile regexp of subsect!");
            
            snprintf(msg, sizeof(msg), "Failed to compile regexp of subsect[%s]!",
                    regex_subsect);
            cpm_send_msg(p_ub, cd, msg);
        }
        r_comp_subsect_alloc=EXTRUE;
        
        /* loop over the hash */
        EXHASH_ITER(hh, G_clt_config, c, ct)
        {
            if (EXSUCCEED==ndrx_regexec(&r_comp_tag, c->tag) && 
                    EXSUCCEED==ndrx_regexec(&r_comp_subsect, c->subsect))
            {
                int cur_nr_proc = nr_proc;
                NDRX_LOG(log_debug, "[%s]/[%s] - matched", c->tag, c->subsect);
                
                
                if (EXSUCCEED!=p_func(p_ub, cd, c->tag, c->subsect, c, &nr_proc))
                {
                    NDRX_LOG(log_error, "Matched process [%s]/[%s] failed to start/stop",
                            c->tag, c->subsect);
                }
                
                if (cur_nr_proc!=nr_proc && twait > 0)
                {
                    NDRX_LOG(log_debug, "Sleeping %d millisec", twait);
                    usleep(twait*1000);
                }
            }
            else
            {
                NDRX_LOG(log_debug, "[%s]/[%s] - NOT matched", c->tag, c->subsect);
            }
        }
        snprintf(msg, sizeof(msg), "%d client(s) %s.", nr_proc, finish_msg);
        cpm_send_msg(p_ub, cd, msg);
    }
        
out:
                
    if (r_comp_tag_alloc)
    {
        ndrx_regfree(&r_comp_tag);
    }

    if (r_comp_subsect_alloc)
    {
        ndrx_regfree(&r_comp_subsect);
    }

    return ret;
}

/**
 * Boot client
 * @param p_ub
 * @param cd
 * @return 
 */
exprivate int cpm_bc(UBFH *p_ub, int cd)
{
    NDRX_LOG(log_debug, "Into %s", __func__);
    return cpm_bcscrc(p_ub, cd, cpm_bc_obj, "marked for started");
}


/**
 * Stop the client process
 * @param p_ub
 * @param cd
 * @return 
 */
exprivate int cpm_sc(UBFH *p_ub, int cd)
{   
    NDRX_LOG(log_debug, "Into %s", __func__);
    return cpm_bcscrc(p_ub, cd, cpm_sc_obj, "stopped");
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
            snprintf(output, sizeof(output), "%s/%s - running pid %d (%s)",
                                c->tag, c->subsect, c->dyn.pid, buffer);
        }
        else if (CLT_STATE_STARTING == c->dyn.cur_state && 
                c->dyn.req_state != CLT_STATE_NOTRUN)
        {
            snprintf(output, sizeof(output), "%s/%s - starting (%s)",c->tag, 
                    c->subsect, buffer);
        }
        else if (c->dyn.was_started && (c->dyn.req_state == CLT_STATE_STARTED) )
        {
            snprintf(output, sizeof(output), "%s/%s - dead %d (%s)", c->tag, c->subsect, 
                    c->dyn.exit_status, buffer);
        }
        else if (c->dyn.was_started && (c->dyn.req_state == CLT_STATE_NOTRUN) )
        {
            snprintf(output, sizeof(output), "%s/%s - shutdown (%s)", c->tag, c->subsect, 
                    buffer);
        }
        else
        {
            snprintf(output, sizeof(output), "%s/%s - not started", c->tag, c->subsect);
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

/**
 * Reload clients (restart one by one)
 * @param p_ub
 * @param cd
 * @return 
 */
exprivate int cpm_rc(UBFH *p_ub, int cd)
{   
    NDRX_LOG(log_debug, "Into %s", __func__);
    return cpm_bcscrc(p_ub, cd, cpm_rc_obj, "restarted");
}
