/**
 * @brief Sanity checking routines.
 *   This will do following:
 *   - scan all stuff for:
 *   <prefix>.srv.reply.[binary].[pid] and do following checks:
 *   - Check is process in system by binary name + pid, if not the remove process
 *
 * @file sanity.c
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <memory.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <utlist.h>

#include <ndrstandard.h>
#include <ndrxd.h>
#include <atmi_int.h>
#include <nstopwatch.h>

#include <ndebug.h>
#include <cmd_processor.h>
#include <signal.h>
#include <bridge_int.h>
#include <atmi_shm.h>
#include "userlog.h"
#include "sys_unix.h"

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
expublic unsigned G_sanity_cycle = 0;
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

exprivate int check_server(char *qname);
exprivate int check_client(char *qname, int is_xadmin, unsigned sanity_cycle);
exprivate int check_cnvclt(char *qname);
exprivate int check_cnvsrv(char *qname);
exprivate int check_long_startup(void);
exprivate int check_dead_processes(void);
/**
 * Master process for sanity checking.
 * @param[in] finalchk perform final checks? Remove dread resources...
 * @return SUCCEED/FAIL
 */
expublic int do_sanity_check(int finalchk)
{
    int ret=EXSUCCEED;
    static ndrx_stopwatch_t timer;
    static int first = EXTRUE;    
    static char    server_prefix[NDRX_MAX_Q_SIZE+1];
    static int     server_prefix_len;
    static char    client_prefix[NDRX_MAX_Q_SIZE+1];
    static int     client_prefix_len;
    static char    xadmin_prefix[NDRX_MAX_Q_SIZE+1];
    static int     xadmin_prefix_len;
    
    /* conversational prefixes */
    static char    cnvclt_prefix[NDRX_MAX_Q_SIZE+1]; /* initiator... */
    static int     cnvclt_prefix_len;
    
    static char    cnvsrv_prefix[NDRX_MAX_Q_SIZE+1];
    static int     cnvsrv_prefix_len;
    
    
    int wasrun = EXFALSE;
    
    string_list_t* qlist = NULL;
    string_list_t* elt = NULL;

    G_sanity_cycle++;
    
    /* No sanity checks while app config not loaded */
    if (NULL==G_app_config)
        goto out;
    
    if (first)
    {
        ndrx_stopwatch_reset(&timer);
        /* Initialize q prefixes, +1 for skipping initial / */
        snprintf(client_prefix, sizeof(client_prefix), NDRX_CLT_QREPLY_PFX, 
                G_sys_config.qprefix);
        client_prefix_len=strlen(client_prefix);
        NDRX_LOG(log_debug, "client_prefix=[%s]/%d", client_prefix, 
                            client_prefix_len);
        
        snprintf(xadmin_prefix, sizeof(xadmin_prefix),
                NDRX_NDRXCLT_PFX, G_sys_config.qprefix);
        xadmin_prefix_len=strlen(xadmin_prefix);
        NDRX_LOG(log_debug, "xadmin_prefix=[%s]/%d", xadmin_prefix, 
                            xadmin_prefix_len);
        
        snprintf(server_prefix, sizeof(server_prefix), NDRX_SVR_QREPLY_PFX, 
                G_sys_config.qprefix);
        server_prefix_len=strlen(server_prefix);
        NDRX_LOG(log_debug, "server_prefix=[%s]/%d", server_prefix, 
                            server_prefix_len);
	
	snprintf(cnvclt_prefix, sizeof(cnvclt_prefix), NDRX_CONV_INITATOR_Q_PFX, 
                G_sys_config.qprefix);
        
        cnvclt_prefix_len=strlen(cnvclt_prefix);
        NDRX_LOG(log_debug, "cnvclt_prefix=[%s]/%d", cnvclt_prefix, 
                            cnvclt_prefix_len);
	
	snprintf(cnvsrv_prefix, sizeof(cnvsrv_prefix), NDRX_CONV_SRV_Q_PFX, 
                G_sys_config.qprefix);
        cnvsrv_prefix_len=strlen(cnvsrv_prefix);
        NDRX_LOG(log_debug, "cnvsrv_prefix=[%s]/%d", cnvsrv_prefix, 
                            cnvsrv_prefix_len);
	
        first=EXFALSE;
    }
     
    if (ndrx_stopwatch_get_delta_sec(&timer)>=G_app_config->sanity || finalchk)
    {
        wasrun = EXTRUE;
        NDRX_LOG(log_debug, "Time for sanity checking...");
         
        qlist = ndrx_sys_mqueue_list_make(G_sys_config.qpath, &ret);

        if (EXSUCCEED!=ret)
        {
            NDRX_LOG(log_error, "posix queue listing failed!");
            EXFAIL_OUT(ret);
        }

        LL_FOREACH(qlist,elt)
        {
            NDRX_LOG(6, "Checking... [%s]", elt->qname);
            
            if (0==strncmp(elt->qname, client_prefix, 
                    client_prefix_len))
            {
                check_client(elt->qname, EXFALSE, G_sanity_cycle);
            }
            else if (0==strncmp(elt->qname, xadmin_prefix, 
                    xadmin_prefix_len)) 
            {
                check_client(elt->qname, EXTRUE, G_sanity_cycle);
            } 
            /* TODO: We might want to monitor admin queues too! */
            else if (0==strncmp(elt->qname, server_prefix, 
                    server_prefix_len)) 
            {
                check_server(elt->qname);
            } /*  Bug #112 */
	    else if (0==strncmp(elt->qname, cnvclt_prefix, 
                    cnvclt_prefix_len)) 
            {
                check_cnvclt(elt->qname);
            } /*  Bug #112 */
	    else if (0==strncmp(elt->qname, cnvsrv_prefix, 
                    cnvsrv_prefix_len)) 
            {
                check_cnvsrv(elt->qname);
            }
        }

        /* Will check programs with long startup they will get killed if, 
         * not started in time! */
        /* NOTE: THIS IS FIRST PROCESS WHICH INCREMENTS COUNTERS IN PM! */
        check_long_startup();
        /* Send bridge refresh (if required) */
        
        if (!finalchk)
        {
            brd_send_periodrefresh();
        }
        
        /* Time for PM checking! */
        if (EXSUCCEED!=check_dead_processes())
        {
            ret=EXFAIL;
            goto out;
        }
        
        /* Respawn any dead processes */
        if (!finalchk)
        {
            do_respawn_check();
        }
        
        /* update queue statistics (if enabled) */
        
        if (!finalchk)
        {
            if (G_app_config->gather_pq_stats)
            {
                pq_run_santiy(EXTRUE);
            }
        }
        
#ifdef EX_USE_SYSVQ
        if (EXSUCCEED!=do_sanity_check_sysv(finalchk))
        {
            NDRX_LOG(log_error, "System V sanity checks failed!");
            userlog("System V sanity checks failed!");
            EXFAIL_OUT(ret);
        }
#endif
    }
    
out:

    if (NULL!=qlist)
    {
        ndrx_string_list_free(qlist);
    }

    /* Reset timer on run */
    if (wasrun)
        ndrx_stopwatch_reset(&timer);

    return ret;
}

/**
 * 
 * @param qname
 * @param process
 * @param p_pid
 */
exprivate void parse_q(char *qname, int is_server, char *process, pid_t *p_pid, 
                    int *server_id, int is_xadmin)
{   
    char buf[NDRX_MAX_Q_SIZE+1];
    char *p;
    
    NDRX_STRCPY_SAFE(buf, qname);
    
    /* We are client, thus needs to skip the context */
    if (!is_server && !is_xadmin)
    {
        p = strrchr(buf, NDRX_FMT_SEP);
        *p=EXEOS;
    }
    
    /* get over with pid */
    p = strrchr(buf, NDRX_FMT_SEP);
    *p_pid = atoi(p+1);
    *p=EXEOS;
    
    if (is_server)
    {
        /* Return server id, if we are server! */
        p=strrchr(buf, NDRX_FMT_SEP);
        *server_id = atoi(p+1);
        *p=EXEOS;
    }
    
    /* Fix up with process name */
    p=strrchr(buf, NDRX_FMT_SEP);
            
    strcpy(process, p+1);
    
    NDRX_LOG(6, "got process: pid: %d name: [%s]", 
                        *p_pid, process);
}
/**
 * Remove dead process queue from system!
 * @param qname
 * @return 
 */
exprivate int unlink_dead_queue(char *qname)
{
    int ret=EXSUCCEED;
    char    q_str[NDRX_MAX_Q_SIZE+1];
    char    *p;
    
    if ('/'!=qname[0])
    {
        NDRX_STRCPY_SAFE(q_str, "/");
        strcat(q_str, qname);
        p = q_str;
    }
    else
    {
        p = qname;
    }
    
    NDRX_LOG(log_warn, "Unlinking queue [%s]", p);
    if (EXSUCCEED!=ndrx_mq_unlink(p))
    {
        int err = errno;
        NDRX_LOG(log_error, "Failed to unlink dead queue [%s]: %s", 
                p, strerror(err));
        /* Feature #237 */
        userlog("Failed to unlink dead queue [%s]: %s", 
                p, strerror(err));
        ret=EXFAIL;
    }
    
    return ret;
}

/**
 * Remove server queues
 * @param process
 * @param pid
 * @param srv_id
 * @param rplyq if null, then reply queue will be built from scratch.
 * @return 
 */
expublic int remove_server_queues(char *process, pid_t pid, int srv_id, char *rplyq)
{
    char    q_str[NDRX_MAX_Q_SIZE+1];
    int     rplyq_unlink = EXFALSE;
    char    *p;

    if (NULL==rplyq)
    {
        snprintf(q_str, sizeof(q_str), NDRX_SVR_QREPLY, 
                G_sys_config.qprefix, process, srv_id, pid);
        
        p = q_str;
        if (!ndrx_q_exists(q_str)) 
        {
            NDRX_LOG(log_info, "Seems like reply queue [%s] does not"
                    " exists - nothing to do: %s", q_str, strerror(errno));
        }
        else
        {
            rplyq_unlink=EXTRUE;
        }
    }
    else
    {
        p = rplyq;
        rplyq_unlink=EXTRUE;
    }

    if (rplyq_unlink)
    {
        unlink_dead_queue(p);
    }

    snprintf(q_str, sizeof(q_str), NDRX_ADMIN_FMT, G_sys_config.qprefix, 
            process, srv_id, pid);
    /* Note - admin_q_str already contains / in front! */
    /*If exists admin queue, but process does not exists, then remove admin q too! */

    if (!ndrx_q_exists(q_str))
    {
        NDRX_LOG(log_info, "Seems like admin queue [%s] does not"
                " exists - nothing to do: %s", q_str, strerror(errno));
    }
    else
    {
        unlink_dead_queue(q_str);
    }
    
    return EXSUCCEED;
}
/**
 * Check running servers...
 * If self Q is full, then is not deadly situation, as it will be handled by next check.
 * Also if dead q is unlinked. The process will be removed by check PM.
 * @return 
 */
exprivate int check_server(char *qname)
{
    char    process[NDRX_MAX_Q_SIZE+1];
    pid_t pid;
    int     srv_id;
    char buf[NDRX_MSGSIZEMAX];
    srv_status_t *status = (srv_status_t *)buf;
    int ret=EXSUCCEED;
    
    memset((char *)status, 0, sizeof(srv_status_t));
    
    parse_q(qname, EXTRUE, process, &pid, &srv_id, EXFALSE);
    
    if (!ndrx_sys_is_process_running(pid, process))
    {      
        /* And finally we send to our selves notification that pid is dead
         * so that system takes care of it's removal! */
        remove_server_queues(process, pid, srv_id, qname);
    
        status->srvinfo.pid = pid;
        status->srvinfo.state = NDRXD_PM_DIED;
        status->srvinfo.srvid = srv_id;
        NDRX_LOG(log_debug, "Sending self notification "
                            "about dead process...");
        if (EXSUCCEED!=self_notify(status, EXFALSE))
        {
            NDRX_LOG(log_warn, "Failed to send self notification "
                    "- exit dead process check for a while!");
            ret=EXFAIL;
            goto out;
        }
        
        /* Remove any conv queues... */
    
    }
out:
    return ret;
}

/**
 * Check running clients...
 * ----------------
 * Needs optimization for threads.
 * We could use scandir instead of readdir.
 * Sort the output, and cache client checks, so that we do not test process for
 * existance for number of threads used...
 * See http://stackoverflow.com/questions/5102863/how-to-sort-files-in-some-directory-by-the-names-on-linux
 * ---------------- => DONE
 * @return 
 */
exprivate int check_client(char *qname, int is_xadmin, unsigned sanity_cycle)
{
    char process[NDRX_MAX_Q_SIZE+1];
    pid_t pid;
    /* Used for cache, so that we do not check multi threaded process
     * multiple times... */
    static unsigned prev_sanity_cycle;
    static int first = EXTRUE;
    static char prev_process[NDRX_MAX_Q_SIZE+1];
    static pid_t prev_pid;
    static int prev_was_unlink=EXFALSE;
    
    if (first)
    {
        prev_sanity_cycle = sanity_cycle-1;
        first=EXFALSE;
    }
    
    parse_q(qname, EXFALSE, process, &pid, 0, is_xadmin);
    
    if (sanity_cycle == prev_sanity_cycle &&
            0==strcmp(process, prev_process) &&
            pid == prev_pid)
    {
        NDRX_LOG(6, "Multi-threaded process [%s]/%d already checked "
                        "at this sanity check", process, pid);
        if (prev_was_unlink)
        {
            NDRX_LOG(log_warn, "Previous same process (different "
                    "thread was unlink) - unlink this q [%s] too ", qname);
            unlink_dead_queue(qname);
        }
        goto out;
    }
    
    /* Fill the prev stuff */
    prev_pid = pid;
    NDRX_STRCPY_SAFE(prev_process, process);
    prev_sanity_cycle = sanity_cycle;
    
    if (!ndrx_sys_is_process_running(pid, process))
    {
        userlog("Client process [%s], pid %d died", process, pid);
        
        unlink_dead_queue(qname);
        prev_was_unlink = EXTRUE;
        
        /* Remove any conv queues... */
        
    }
    else
    {
        prev_was_unlink = EXFALSE;
    }
    
out:
    return EXSUCCEED;
}

/* TODO: We might want to check queues against shared memory... but not sure
 * shm might badly initialized. But anyway if shared memory exists, then caller to queue
 * firstly will check the shared memory and only after that it will call the server.
 */

/**
 * Kill the process - #76 we will kill the reported PID (real server PID)
 * @param p_pm
 * @return 
 */
exprivate int send_kill(pm_node_t *p_pm, int sig, int delta)
{
    NDRX_LOG(log_warn, "Killing PID: %d (ppid: %d)/%s/%d with signal -%d", 
            p_pm->svpid, p_pm->pid, p_pm->binary_name, p_pm->srvid, sig);
    userlog("Killing PID: %d (ppid: %d)/%s/%d with signal -%d", 
            p_pm->svpid, p_pm->pid, p_pm->binary_name, p_pm->srvid, sig);
    if (EXSUCCEED!=kill(p_pm->svpid, sig))
    {
        NDRX_LOG(log_error, "Failed to kill PID %d (ppid: %d) with error: %s",
                p_pm->svpid, p_pm->pid, strerror(errno));
    }
    
    return EXSUCCEED;
}

/**
 * This function will deal with programs which perofrms long starup!
 * I.e. if they do not start in time, they will be killed!
 * @return 
 */
exprivate int check_long_startup(void)
{
    int ret=EXSUCCEED;
    pm_node_t *p_pm;
    int delta;
    int cksum_reload_sent = EXFALSE; /* for now single binary only at one cycle */
    
    DL_FOREACH(G_process_model, p_pm)
    {
        /* PM Counter increment! */
        p_pm->rspstwatch++;
		/* Increment ping stopwatch (if was issued) */
        if (SANITY_CNT_IDLE!=p_pm->pingstwatch)
        {
            p_pm->pingstwatch++;
        }
        
        if (p_pm->conf->pingtime)
        {
            p_pm->pingtimer++;
        }
        p_pm->last_sig++;
        p_pm->state_changed++;
        
        /* send ping to server, if require */
        if (!p_pm->killreq && NDRXD_PM_RUNNING_OK==p_pm->state &&
                p_pm->conf->pingtime && p_pm->pingtimer > p_pm->conf->pingtime)
        {
            /* Reset ping timer */
            p_pm->pingtimer = SANITY_CNT_START;
            
            /* start to watch the ping response time: */
            if (SANITY_CNT_IDLE==p_pm->pingstwatch)
            {
                p_pm->pingstwatch = SANITY_CNT_START;
            }
            
            /* Send ping command to server */
            srv_send_ping (p_pm);
        }
        
        /* If still starting */
        if (p_pm->autokill)
        {
            NDRX_LOG(6, "proc: %s/%d ping stopwatch: %ld, rsp: %ld sty ping timer: %ld sty", 
                    p_pm->binary_name, p_pm->srvid,
                    p_pm->pingstwatch,
                    p_pm->rspstwatch,
                    p_pm->pingtimer);
            if (!p_pm->killreq)
            {
                if (NDRXD_PM_STARTING==p_pm->state &&
                    (delta=p_pm->rspstwatch) > p_pm->conf->start_max)
                {
                    NDRX_LOG(log_error, "Startup too long - requesting "
                            "kill pid=%d/bin=%s/srvid=%d",
                             p_pm->pid, p_pm->binary_name, p_pm->srvid);
                    /* Support #276 */
                    userlog("Startup too long - requesting "
                            "kill pid=%d/bin=%s/srvid=%d",
                             p_pm->pid, p_pm->binary_name, p_pm->srvid);
                    p_pm->killreq=EXTRUE;
                }
                else if (NDRXD_PM_RUNNING_OK==p_pm->state && p_pm->conf->pingtime &&
                    (delta=p_pm->pingstwatch) > p_pm->conf->ping_max)
                {
                    NDRX_LOG(log_error, "Ping response not in time - "
                                        "requesting kill (ping_time=%d delta=%d "
                                        "ping_max=%d) pid=%d/%s/srvid=%d",
					p_pm->conf->pingtime, delta, p_pm->conf->ping_max,
                                        p_pm->pid, p_pm->binary_name, p_pm->srvid);
                    /* Support #276 */
                    userlog("Ping response not in time - "
                                        "requesting kill (ping_time=%d delta=%d "
                                        "ping_max=%d) pid=%d/bin=%s/srvid=%d",
					p_pm->conf->pingtime, delta, p_pm->conf->ping_max,
                                        p_pm->pid, p_pm->binary_name, p_pm->srvid);
                    p_pm->killreq=EXTRUE;
                }
                else if (NDRXD_PM_STOPPING==p_pm->state &&
                    (delta = p_pm->rspstwatch) > p_pm->conf->end_max)
                {
                    NDRX_LOG(log_error, "Server did not exit in time "
                                                            "- requesting kill");
                    p_pm->killreq=EXTRUE;
                }
            }
            
            if (p_pm->killreq)
            {
                if (0==p_pm->num_term_sigs)
                {
                    /* Send signal INT => -2 */
                    send_kill(p_pm, SIGINT, 0);
                    /* Reset the signal time counter */
                    p_pm->last_sig = SANITY_CNT_START;
                    p_pm->num_term_sigs++;
                }
                else if (p_pm->num_term_sigs > 0 
                        && p_pm->last_sig > p_pm->conf->killtime)
                {
                    if (1==p_pm->num_term_sigs)
                    {
                        /* Send signal TERM => -15 */
                        send_kill(p_pm, SIGTERM, 0);
                    }
                    else
                    {
                        /* Send signal KILL => -9 */
                        send_kill(p_pm, SIGKILL, 0);
                    }
                    
                    /* Send proper signal  */
                    if (p_pm->num_term_sigs<2)
                        p_pm->num_term_sigs++;
                    p_pm->last_sig = SANITY_CNT_START;
                }
            }
        } /* If process still starting! */
        
        /* check the restart if needed by checksum */
        /* TODO: We need some hash list here so we caulcate checsums only one binary
         * not the all instances. And only issue one update per checksum change.
         */
        if (p_pm->conf->reloadonchange && EXEOS!=p_pm->binary_path[0] 
                && !cksum_reload_sent
                && ndrx_file_exists(p_pm->binary_path))
        {
            if (roc_check_binary(p_pm->binary_path, G_sanity_cycle))
            {
                NDRX_LOG(log_warn, "Cksums differ reload...");
                /* Send reload command */
                if (EXSUCCEED!=self_sreload(p_pm))
                {
                    NDRX_LOG(log_warn, "Failed to send self notification "
                            "about changed process - ignore!");
                }
                cksum_reload_sent=EXTRUE;
            }
        }
        
    }/* DL_FOREACH */
    
out:
    return ret;
}

/**
 * Check any dead processes, which we think are running ok, but actually 
 * the PID does not exists!
 * If self q is full, it will be removed by next try.
 * @return 
 */
exprivate int check_dead_processes(void)
{
    int ret=EXSUCCEED;
    pm_node_t *p_pm;
    char buf[NDRX_MSGSIZEMAX];
    srv_status_t *status = (srv_status_t *)buf;
    
    DL_FOREACH(G_process_model, p_pm)
    {
        /* If still starting */
        if (p_pm->state>=NDRXD_PM_MIN_RUNNING &&
            p_pm->state<=NDRXD_PM_MAX_RUNNING &&
                p_pm->state_changed > G_app_config->checkpm)
        {
            if (!ndrx_sys_is_process_running(p_pm->svpid, p_pm->binary_name_real))
            {
                NDRX_LOG(log_warn, "Pid %d/%s/%s in state %d is actually dead",
                        p_pm->pid, p_pm->binary_name, p_pm->binary_name_real, 
                        p_pm->state);
                /*Send self notification*/
                
                memset(buf, 0, sizeof(buf));
                
                status->srvinfo.pid = p_pm->pid;
                status->srvinfo.state = NDRXD_PM_DIED;
                status->srvinfo.srvid = p_pm->srvid;
                
                NDRX_LOG(log_debug, "Sending self notification "
                                    "about dead process...");
                
                if (EXSUCCEED!=self_notify(status, EXFALSE))
                {
                    NDRX_LOG(log_warn, "Failed to send self notification "
                            "- exit dead process check for a while!");
                    ret=EXFAIL;
                    goto out;
                }
            }
        } /* If process still starting! */
    }/* DL_FOREACH */
    
out:
    return ret;   
}

/**
 * This checks the shared memory of services with linked lists for each proces in PM
 * @return 
 */
exprivate int check_svc_shm(void)
{
    return EXSUCCEED;
}

/*

 Picture here with conv quueues can be follwing:
Msg queued Q name
---------- ---------------------------------------------------------------------
0          /dom2,sys,bg,ndrxd
0          /dom2,cnv,s,srv,atmisv35,19,32175,0,2,1,srv,atmisv35,10,32157,0,2
0          /dom2,cnv,s,srv,atmisv35,18,32173,0,2,1,srv,atmisv35,19,32175,0,2
0          /dom2,cnv,s,srv,atmisv35,17,32171,0,2,1,srv,atmisv35,18,32173,0,2
0          /dom2,cnv,s,srv,atmisv35,16,32169,0,2,1,srv,atmisv35,17,32171,0,2
0          /dom2,cnv,s,srv,atmisv35,15,32167,0,2,1,srv,atmisv35,16,32169,0,2
0          /dom2,cnv,s,srv,atmisv35,14,32165,0,2,1,srv,atmisv35,15,32167,0,2
0          /dom2,cnv,s,srv,atmisv35,13,32163,0,2,1,srv,atmisv35,14,32165,0,2
0          /dom2,cnv,s,srv,atmisv35,12,32161,0,2,1,srv,atmisv35,13,32163,0,2
0          /dom2,cnv,s,srv,atmisv35,11,32159,0,2,1,srv,atmisv35,12,32161,0,2
0          /dom2,cnv,s,clt,atmiclt35,32218,2,1,1,srv,atmisv35,11,32159,0,2
1          /dom2,cnv,c,srv,atmisv35,19,32175,0,2,1
1          /dom2,cnv,c,srv,atmisv35,18,32173,0,2,1
1          /dom2,cnv,c,srv,atmisv35,17,32171,0,2,1
1          /dom2,cnv,c,srv,atmisv35,16,32169,0,2,1
1          /dom2,cnv,c,srv,atmisv35,15,32167,0,2,1
1          /dom2,cnv,c,srv,atmisv35,14,32165,0,2,1
1          /dom2,cnv,c,srv,atmisv35,13,32163,0,2,1
1          /dom2,cnv,c,srv,atmisv35,12,32161,0,2,1
1          /dom2,cnv,c,srv,atmisv35,11,32159,0,2,1
0          /dom2,cnv,c,srv,atmisv35,10,32157,0,2,1
0          /dom1,sys,bg,xadmin,32241
0          /dom1,sys,bg,ndrxd
0          /dom1,cnv,s,clt,atmiclt35,32218,3,1,1,srv,atmisv35,12,32137,0,1
0          /dom1,cnv,s,clt,atmiclt35,32218,1,1,1,srv,atmisv35,11,32135,0,1
0          /dom1,cnv,c,clt,atmiclt35,32218,5,1,1
0          /dom1,cnv,c,clt,atmiclt35,32218,4,1,1
0          /dom1,cnv,c,clt,atmiclt35,32218,3,1,1
0          /dom1,cnv,c,clt,atmiclt35,32218,2,1,1
0          /dom1,cnv,c,clt,atmiclt35,32218,1,1,1
Test exiting with: 
 
 */

/**
 * Check the conversational initiator. We will kill the queue if any of the processes
 * in our cluster node are dead.
 * @return 
 */
exprivate int check_cnvclt(char *qname)
{
    int ret = EXSUCCEED;
    TPMYID myid;
    
    if (EXSUCCEED==ndrx_cvnq_parse_client(qname, &myid))
    {
        if (EXFALSE==ndrx_myid_is_alive(&myid))
        {
            ndrx_myid_dump(log_debug, &myid, "process is dead, remove the queue");
            
            unlink_dead_queue(qname);
        }
    }
    
out:
    return ret;
}

/**
 * Check conversation server accepted Q
 * @param qname queue name
 * @return SUCCEED
 */
exprivate int check_cnvsrv(char *qname)
{
    int ret = EXSUCCEED;
    TPMYID myid1, myid2;
    
    /* check start with srv, or ctl, then detect the length of the halve
     * and parse other part.
     * We are interested in other part, if it is dead, then kill the Q.
     */
    if (EXSUCCEED==ndrx_cvnq_parse_server(qname, &myid1, &myid2))
    {
        if (EXFALSE==ndrx_myid_is_alive(&myid2))
        {
            ndrx_myid_dump(log_debug, &myid2, "process is dead, remove the queue");
            unlink_dead_queue(qname);
        }
    }
   
out:
    return ret;
}

/**
 * Perform final sanity checks - ndrxd is exiting
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrxd_sanity_finally(void)
{
    int ret = EXSUCCEED;
    
    /*
#ifdef EX_USE_SYSVQ
    
    if (EXSUCCEED!=ndrxd_sysv_finally())
    {
        ret = EXFAIL;
    }
    
#endif
     */
    
    ret = do_sanity_check(EXTRUE);
    
out:
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
