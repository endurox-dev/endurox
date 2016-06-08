/* 
** Sanity checking routines.
** This will do following:
** - scan all stuff for:
** <prefix>.srv.reply.[binary].[pid] and do following checks:
** - Check is process in system by binary name + pid, if not the remove process
**
** @file sanity.c
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
#include <ntimer.h>

#include <ndebug.h>
#include <cmd_processor.h>
#include <signal.h>
#include <bridge_int.h>

#include "userlog.h"
#include "sys_unix.h"

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

private int check_server(char *qname);
private int check_client(char *qname, int is_xadmin, unsigned nr_of_try);
private int check_long_startup(void);
private int check_dead_processes(void);
/**
 * Master process for sanity checking.
 * @return SUCCEED/FAIL
 */
public int do_sanity_check(void)
{
    int ret=SUCCEED;
    static ndrx_timer_t timer;
    static int first = TRUE;    
    static char    server_prefix[NDRX_MAX_Q_SIZE+1];
    static int     server_prefix_len;
    static char    client_prefix[NDRX_MAX_Q_SIZE+1];
    static int     client_prefix_len;
    static char    xadmin_prefix[NDRX_MAX_Q_SIZE+1];
    static int     xadmin_prefix_len;
    int wasrun = FALSE;
    
    string_list_t* qlist = NULL;
    string_list_t* elt = NULL;
    
    int n;
    static unsigned nr_of_try = 0;

    nr_of_try++;
    
    /* No sanity checks while app config not loaded */
    if (NULL==G_app_config)
        goto out;
    
    if (first)
    {
        ndrx_timer_reset(&timer);
        /* Initialise q prefixes, +1 for skipping initial / */
        sprintf(client_prefix, NDRX_CLT_QREPLY_PFX, G_sys_config.qprefix);
        client_prefix_len=strlen(client_prefix);
        NDRX_LOG(log_debug, "client_prefix=[%s]/%d", client_prefix, 
                            client_prefix_len);
        
        sprintf(xadmin_prefix, NDRX_NDRXCLT_PFX, G_sys_config.qprefix);
        xadmin_prefix_len=strlen(xadmin_prefix);
        NDRX_LOG(log_debug, "xadmin_prefix=[%s]/%d", xadmin_prefix, 
                            xadmin_prefix_len);
        
        sprintf(server_prefix, NDRX_SVR_QREPLY_PFX, G_sys_config.qprefix);
        server_prefix_len=strlen(server_prefix);
        NDRX_LOG(log_debug, "server_prefix=[%s]/%d", server_prefix, 
                            server_prefix_len);
         
        first=FALSE;
    }
     
    if (ndrx_timer_get_delta_sec(&timer)>=G_app_config->sanity)
    {
        wasrun = TRUE;
        NDRX_LOG(log_debug, "Time for sanity checking...");
         
        qlist = ndrx_sys_mqueue_list_make(G_sys_config.qpath, &ret);

        if (SUCCEED!=ret)
        {
            NDRX_LOG(log_error, "posix queue listing failed!");
            FAIL_OUT(ret);
        }

        LL_FOREACH(qlist,elt)
        {
            NDRX_LOG(6, "Checking... [%s]", elt->qname);
            if (0==strncmp(elt->qname, client_prefix, 
                    client_prefix_len))
            {
                check_client(elt->qname, FALSE, nr_of_try);
            }
            else if (0==strncmp(elt->qname, xadmin_prefix, 
                    xadmin_prefix_len)) 
            {
                check_client(elt->qname, TRUE, nr_of_try);
            } 
            /* TODO: We might want to monitor admin queues too! */
            else if (0==strncmp(elt->qname, server_prefix, 
                    server_prefix_len)) 
            {
                check_server(elt->qname);
            }
        }

        /* Will check programs with long startup they will get killed if, 
         * not started in time! */
        /* NOTE: THIS IS FIRST PROCESS WHICH INCREMENTS COUNTERS IN PM! */
        check_long_startup();
        /* Send bridge refresh (if required) */
        
        brd_send_periodrefresh();
        /* Time for PM checking! */
        if (SUCCEED!=check_dead_processes())
        {
            ret=FAIL;
            goto out;
        }
        
        /* Respawn any dead processes */
        do_respawn_check();
        
        /* update queue statistics (if enabled) */
        if (G_app_config->gather_pq_stats)
        {
            pq_run_santiy(TRUE);
        }
    }
    
out:

    if (NULL!=qlist)
    {
        ndrx_string_list_free(qlist);
    }

    /* Reset timer on run */
    if (wasrun)
        ndrx_timer_reset(&timer);

    return ret;
}

/**
 * 
 * @param qname
 * @param process
 * @param p_pid
 */
private void parse_q(char *qname, int is_server, char *process, pid_t *p_pid, 
                    int *server_id, int is_xadmin)
{   
    char buf[NDRX_MAX_Q_SIZE+1];
    char *p;
    int len;
    
    strcpy(buf, qname);
    
    /* We are client, thus needs to skip the context */
    if (!is_server && !is_xadmin)
    {
        p = strrchr(buf, NDRX_FMT_SEP);
        *p=EOS;
    }
    
    /* get over with pid */
    p = strrchr(buf, NDRX_FMT_SEP);
    *p_pid = atoi(p+1);
    *p=EOS;
    
    if (is_server)
    {
        /* Return server id, if we are server! */
        p=strrchr(buf, NDRX_FMT_SEP);
        *server_id = atoi(p+1);
        *p=EOS;
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
private int unlink_dead_queue(char *qname)
{
    int ret=SUCCEED;
    char    q_str[NDRX_MAX_Q_SIZE+1];
    char    *p;
    
    if ('/'!=qname[0])
    {
        strcpy(q_str, "/");
        strcat(q_str, qname);
        p = q_str;
    }
    else
    {
        p = qname;
    }
    
    NDRX_LOG(log_warn, "Unlinking queue [%s]", p);
    if (SUCCEED!=ex_mq_unlink(p))
    {
        NDRX_LOG(log_error, "Failed to unlink dead queue [%s]: %s", 
                p, strerror(errno));
        ret=FAIL;
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
public int remove_server_queues(char *process, pid_t pid, int srv_id, char *rplyq)
{
    char    q_str[NDRX_MAX_Q_SIZE+1];
    int     rplyq_unlink = FALSE;
    char    *p;

    if (NULL==rplyq)
    {
        sprintf(q_str, NDRX_SVR_QREPLY, G_sys_config.qprefix, process, srv_id, pid);
        
        p = q_str;
        if (!ndrx_q_exists(q_str)) 
        {
            NDRX_LOG(log_info, "Seems like reply queue [%s] does not"
                    " exists - nothing to do: %s", q_str, strerror(errno));
        }
        else
        {
            rplyq_unlink=TRUE;
        }
    }
    else
    {
        p = rplyq;
        rplyq_unlink=TRUE;
    }

    if (rplyq_unlink)
    {
        unlink_dead_queue(p);
    }

    sprintf(q_str, NDRX_ADMIN_FMT, G_sys_config.qprefix, process, srv_id, pid);
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
    
    return SUCCEED;
}
/**
 * Check running servers...
 * If self Q is full, then is not deadly situation, as it will be handled by next check.
 * Also if dead q is unlinked. The process will be removed by check PM.
 * @return 
 */
private int check_server(char *qname)
{
    char    process[NDRX_MAX_Q_SIZE+1];
    pid_t pid;
    int     srv_id;
    char buf[ATMI_MSG_MAX_SIZE];
    srv_status_t *status = (srv_status_t *)buf;
    int ret=SUCCEED;
    
    
    parse_q(qname, TRUE, process, &pid, &srv_id, FALSE);
    
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
        if (SUCCEED!=self_notify(status, FALSE))
        {
            NDRX_LOG(log_warn, "Failed to send self notification "
                    "- exit dead process check for a while!");
            ret=FAIL;
            goto out;
        }
    
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
private int check_client(char *qname, int is_xadmin, unsigned nr_of_try)
{
    char    process[NDRX_MAX_Q_SIZE+1];
    pid_t pid;
    /* Used for cache, so that we do not check multi threaded process
     * multiple times... */
    static unsigned prev_nr_of_try;
    static int first = TRUE;
    static char prev_process[NDRX_MAX_Q_SIZE+1];
    static pid_t prev_pid;
    static int prev_was_unlink=FALSE;
    
    if (first)
    {
        prev_nr_of_try = nr_of_try-1;
        first=FALSE;
    }
    
    parse_q(qname, FALSE, process, &pid, 0, is_xadmin);
    
    if (nr_of_try == prev_nr_of_try &&
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
    strcpy(prev_process, process);
    prev_nr_of_try = nr_of_try;
    
    if (!ndrx_sys_is_process_running(pid, process))
    {
        unlink_dead_queue(qname);
        prev_was_unlink = TRUE;
    }
    else
    {
        prev_was_unlink = FALSE;
    }
    
out:
    return SUCCEED;
}

/* TODO: We might want to check queues against shared memory... but not sure
 * shm might badly initialized. But anyway if shared memory exists, then caller to queue
 * firstly will check the shared memory and only after that it will call the server.
 */

/**
 * Kill the process
 * @param p_pm
 * @return 
 */
private int send_kill(pm_node_t *p_pm, int sig, int delta)
{
    NDRX_LOG(log_warn, "Killing PID: %d/%s/%d with signal -%d", 
            p_pm->pid, p_pm->binary_name, p_pm->srvid, sig);
    if (SUCCEED!=kill(p_pm->pid, sig))
    {
        NDRX_LOG(log_error, "Failed to kill PID %d with error: %s",
                p_pm->pid, strerror(errno));
    }
    
    return SUCCEED;
}

/**
 * This function will deal with programs which perofrms long starup!
 * I.e. if they do not start in time, they will be killed!
 * @return 
 */
private int check_long_startup(void)
{
    int ret=SUCCEED;
    pm_node_t *p_pm;
    int delta;
    
    DL_FOREACH(G_process_model, p_pm)
    {
        /* PM Counter increment! */
        p_pm->rsptimer++;
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
            /* Send ping command to server */
            srv_send_ping (p_pm);
        }
        
        /* If still starting */
        if (p_pm->autokill)
        {
            NDRX_LOG(6, "proc: %s/%d rsp: %ld sty ping timer: %ld sty", 
                    p_pm->binary_name, p_pm->srvid,
                    p_pm->rsptimer, 
                    p_pm->pingtimer);
            if (!p_pm->killreq)
            {
                if (NDRXD_PM_STARTING==p_pm->state &&
                    (delta=p_pm->rsptimer) > p_pm->conf->start_max)
                {
                    NDRX_LOG(log_debug, "Startup too long - "
                                                    "requesting kill");
                    p_pm->killreq=TRUE;
                }
                else if (NDRXD_PM_RUNNING_OK==p_pm->state && p_pm->conf->pingtime &&
                    (delta=p_pm->rsptimer) > p_pm->conf->ping_max)
                {
                    NDRX_LOG(log_debug, "Ping response not in time - "
                                        "requesting kill (ping_time=%d delta=%d ping_max=%d)",
					p_pm->conf->pingtime, delta, p_pm->conf->ping_max);
                    p_pm->killreq=TRUE;
                }
                else if (NDRXD_PM_STOPPING==p_pm->state &&
                    (delta = p_pm->rsptimer) > p_pm->conf->end_max)
                {
                    NDRX_LOG(log_debug, "Server did not exit in time "
                                                            "- requesting kill");
                    p_pm->killreq=TRUE;
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
private int check_dead_processes(void)
{
    int ret=SUCCEED;
    pm_node_t *p_pm;
    int delta;
    char buf[ATMI_MSG_MAX_SIZE];
    srv_status_t *status = (srv_status_t *)buf;
    
    DL_FOREACH(G_process_model, p_pm)
    {
        /* If still starting */
        if (p_pm->state>=NDRXD_PM_MIN_RUNNING &&
            p_pm->state<=NDRXD_PM_MAX_RUNNING &&
                p_pm->state_changed > G_app_config->checkpm)
        {
            if (!ndrx_sys_is_process_running(p_pm->pid, p_pm->binary_name))
            {
                NDRX_LOG(log_warn, "Pid %d/%s in state %d is actually dead",
                        p_pm->pid, p_pm->binary_name, p_pm->state);
                /*Send self notification*/
                
                memset(buf, 0, sizeof(buf));
                
                status->srvinfo.pid = p_pm->pid;
                status->srvinfo.state = NDRXD_PM_DIED;
                status->srvinfo.srvid = p_pm->srvid;
                
                NDRX_LOG(log_debug, "Sending self notification "
                                    "about dead process...");
                
                if (SUCCEED!=self_notify(status, FALSE))
                {
                    NDRX_LOG(log_warn, "Failed to send self notification "
                            "- exit dead process check for a while!");
                    ret=FAIL;
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
private int check_svc_shm(void)
{
    return SUCCEED;
}
