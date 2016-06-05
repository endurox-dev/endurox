/* 
** System utilities
**
** @file sysutil.c
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
#include <stdint.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <dlfcn.h>
#include <sys/mman.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <ndrxdcmn.h>
#include <userlog.h>
#include <ubf.h>
#include <ubfutil.h>
#include <sys_unix.h>
#include <utlist.h>
#include <atmi_shm.h>
#include <unistd.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define KILL_SLEEP_SECS 2
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

/**
 * Check is server running
 */
public int ex_chk_server(char *procname, short srvid)
{
    int ret = FALSE;
    char test_string3[NDRX_MAX_KEY_SIZE+4];
    char test_string4[64];
    string_list_t * list;
     
    sprintf(test_string3, "-k %s", G_atmi_env.rnd_key);
    sprintf(test_string4, "-i %hd", srvid);
    
    list =  ex_sys_ps_list(ex_sys_get_cur_username(), procname, test_string3, test_string4);
    
    if (NULL!=list)
    {
        NDRX_LOG(log_debug, "process %s -i %hd running ok", procname, srvid);
        ret = TRUE;
    }
    else
    {
        NDRX_LOG(log_debug, "process %s -i %hd not running...", procname, srvid);
    }
    
    
    ex_string_list_free(list);
   
    return ret;
}


/**
 * Check is `ndrxd' daemon running
 */
public int ex_chk_ndrxd(void)
{
    int ret = FALSE;
    char test_string3[NDRX_MAX_KEY_SIZE+4];
    string_list_t * list;
     
    sprintf(test_string3, "-k %s", G_atmi_env.rnd_key);
    
    list =  ex_sys_ps_list(ex_sys_get_cur_username(), "ndrxd", test_string3, "");
    
    if (NULL!=list)
    {
        NDRX_LOG(log_debug, "process `ndrxd' running ok");
        ret = TRUE;
    }
    else
    {
        NDRX_LOG(log_debug, "process `ndrxd' not running...");
    }
    
    
    ex_string_list_free(list);
    
    return ret;
}

/**
 * Prase client queue
 * @param pfx
 * @param proc
 * @param pid
 * @param th
 * @return 
 */
public int ex_parse_clt_q(char *q, char *pfx, char *proc, pid_t *pid, long *th)
{
    char tmp[NDRX_MAX_Q_SIZE+1];
    char *token;
    int ret = SUCCEED;
    
    pfx[0] = EOS;
    proc[0] = EOS;
    *pid = 0;
    *th = 0;

    if (NULL==strstr(q, NDRX_CLT_QREPLY_CHK))
    {
        NDRX_LOG(log_debug, "[%s] - not client Q", q);
        ret = FAIL;
        goto out;
    }
            
    strcpy(tmp, q);
    
    /* get the first token */
    token = strtok(tmp, NDRX_FMT_SEP_STR);

    if (NULL!=token)
    {
        strcpy(pfx, token);
    }
    else
    {
        NDRX_LOG(log_error, "missing pfx")
        FAIL_OUT(ret);
    }
    
    
    token = strtok(NULL, NDRX_FMT_SEP_STR);
    if (NULL==token)
    {
        NDRX_LOG(log_error, "missing clt")
        FAIL_OUT(ret);
    }
    
    token = strtok(NULL, NDRX_FMT_SEP_STR);
    if (NULL==token)
    {
        NDRX_LOG(log_error, "missing reply")
        FAIL_OUT(ret);
    }
    
    token = strtok(NULL, NDRX_FMT_SEP_STR);
    
    if (NULL!=token)
    {
        strcpy(proc, token);
    }
    else
    {
        NDRX_LOG(log_error, "missing proc name")
        FAIL_OUT(ret);
    }
    
    token = strtok(NULL, NDRX_FMT_SEP_STR);
    
    if (NULL!=token)
    {
        *pid=atoi(token);
    }
    else
    {
        NDRX_LOG(log_error, "missing proc pid")
        FAIL_OUT(ret);
    }
    
    token = strtok(NULL, NDRX_FMT_SEP_STR);
    
    if (NULL!=token)
    {
        *th=atol(token);
    }
    else
    {
        NDRX_LOG(log_error, "missing proc th")
        FAIL_OUT(ret);
    }
    
out:
    return ret;
}

/**
 * Parse pid from PS output
 * @param psout
 * @param pid
 * @return 
 */
public int ex_get_pid_from_ps(char *psout, pid_t *pid)
{
    char tmp[PATH_MAX+1];
    char *token;
    int ret = SUCCEED;
    
    strcpy(tmp, psout);
    /* get the first token */
    token = strtok(tmp, "\t ");

    if (NULL==token)
    {
        NDRX_LOG(log_error, "missing first ps -ef column")
        FAIL_OUT(ret);
    }
    
    token = strtok(NULL, "\t ");
    if (NULL==token)
    {
        NDRX_LOG(log_error, "missing pid in ps -ef output")
        FAIL_OUT(ret);
    }   
    else
    {
        *pid = atoi(token);
    }
    
out:
    return ret;
}

/**
 * Kill the system running (the xadmin dies last...)
 */
public int ex_down_sys(char *qprefix, char *qpath, int is_force)
{
    int ret = SUCCEED;
    int signals[] = {SIGTERM, SIGKILL};
    int i;
    string_list_t* qlist = NULL;
    string_list_t* srvlist = NULL;
    string_list_t* xadminlist = NULL;
    string_list_t* elt = NULL;
    char pfx[NDRX_MAX_Q_SIZE+1];
    char proc[NDRX_MAX_Q_SIZE+1];
    pid_t pid;
    long th;
    char test_string2[NDRX_MAX_KEY_SIZE+4];
    char srvinfo[NDRX_MAX_SHM_SIZE];
    char svcinfo[NDRX_MAX_SHM_SIZE];
    char brinfo[NDRX_MAX_SHM_SIZE];
    char *shm[] = {srvinfo, svcinfo, brinfo};
    char *ndrxd_pid_file = getenv(CONF_NDRX_DPID);
    int max_signals = 2;
    int was_any = FALSE;
    NDRX_LOG(log_warn, "****** Forcing system down ******");
    
    
    sprintf(srvinfo, NDRX_SHM_SRVINFO, qprefix);
    sprintf(svcinfo, NDRX_SHM_SVCINFO, qprefix);
    sprintf(brinfo,  NDRX_SHM_BRINFO,  qprefix);
    
     
    sprintf(test_string2, "-k %s", G_atmi_env.rnd_key);
    
    NDRX_LOG(log_warn, "Removing all client processes..");
    
    if (is_force)
    {
        signals[0] = SIGKILL;
    }
    
    /* list all queues */
    qlist = ex_sys_mqueue_list_make(qpath, &ret);
    
    if (SUCCEED!=ret)
    {
        NDRX_LOG(log_error, "posix queue listing failed... continue...!");
        ret = SUCCEED;
        qlist = NULL;
    }
    
    for (i=0; i<max_signals; i++)
    {
        LL_FOREACH(qlist,elt)
        {
            NDRX_LOG(log_debug, "Testing q [%s]", elt->qname);
            
            /* if not print all, then skip this queue */
            if (0!=strncmp(elt->qname, 
                    qprefix, strlen(qprefix)))
            {
                continue;
            }
            
            /* Parse out process name & pid */
            NDRX_LOG(log_warn, "processing q: [%s]", elt->qname);
            
            if (SUCCEED==ex_parse_clt_q(elt->qname, pfx, proc, &pid, &th) &&
                    0!=strcmp(proc, "xadmin"))
            {
                 NDRX_LOG(log_error, "! killing  sig=%d pfx=[%s] proc=[%s] "
                         "pid=[%d] th=[%ld]", signals[i], pfx, proc, pid, th);
                 if (SUCCEED!=kill(pid, signals[i]))
                 {
                     NDRX_LOG(log_error, "failed to kill with signal %d pid %d: %s",
                             signals[i], pid, strerror(errno));
                 }
                 was_any = TRUE;
            }
        }
        
        if (0==i && was_any)
        {
            sleep(KILL_SLEEP_SECS);
        }
    }
    
    /* kill all servers */
    NDRX_LOG(log_warn, "Removing server processes...");
    
    was_any = FALSE;
    srvlist = ex_sys_ps_list(ex_sys_get_cur_username(), test_string2, "", "");
    
    for (i=0; i<max_signals; i++)
    {
        LL_FOREACH(srvlist,elt)
        {
            /* Parse out process name & pid */
            NDRX_LOG(log_warn, "processing proc: [%s]", elt->qname);
            
            if (SUCCEED==ex_get_pid_from_ps(elt->qname, &pid))
            {
                 NDRX_LOG(log_error, "! killing  sig=%d "
                         "pid=[%d]", signals[i], pid);
                 
                 if (SUCCEED!=kill(pid, signals[i]))
                 {
                     NDRX_LOG(log_error, "failed to kill with signal %d pid %d: %s",
                             signals[i], pid, strerror(errno));
                 }
                 was_any = TRUE;
            }
        }
        if (0==i && was_any)
        {
            sleep(KILL_SLEEP_SECS);
        }
    }

    /* remove all xadmins... */
    NDRX_LOG(log_warn, "Removing other xadmins...");
    was_any = FALSE;
    xadminlist = ex_sys_ps_list(ex_sys_get_cur_username(), "xadmin", "", "");
    
    for (i=0; i<max_signals; i++)
    {
        LL_FOREACH(xadminlist,elt)
        {
            /* Parse out process name & pid */
            NDRX_LOG(log_warn, "processing proc: [%s]", elt->qname);
            
            if (SUCCEED==ex_get_pid_from_ps(elt->qname, &pid) && pid!=getpid())
            {
                 NDRX_LOG(log_error, "! killing  sig=%d "
                         "pid=[%d]", signals[i], pid);
                 
                 if (SUCCEED!=kill(pid, signals[i]))
                 {
                     NDRX_LOG(log_error, "failed to kill with signal %d pid %d: %s",
                             signals[i], pid, strerror(errno));
                 }
                 
                 was_any = TRUE;
            }
        }
        
        if (0==i && was_any)
        {
            sleep(KILL_SLEEP_SECS);
        }
    }
    
    /* Remove all queues */
    NDRX_LOG(log_warn, "Removing queues...");
    
    LL_FOREACH(qlist,elt)
    {
        /* if not print all, then skip this queue */
        if (0!=strncmp(elt->qname, 
                qprefix, strlen(qprefix)))
        {
            continue;
        }

        /* Parse out process name & pid */
        NDRX_LOG(log_warn, "Removing q: [%s]", elt->qname);

        if (SUCCEED!=ex_mq_unlink(elt->qname))
        {
            NDRX_LOG(log_error, "failed to remove q [%s]: %s",
                    elt->qname, strerror(errno));
        }
    }
    
    NDRX_LOG(log_warn, "Removing shared memory...");
    
    for (i=0; i<N_DIM(shm); i++)
    {
        NDRX_LOG(log_warn, "Unlinking [%s]", shm[i]);
        
        if (SUCCEED!=shm_unlink(shm[i]))
        {
            NDRX_LOG(log_warn, "shm_unlink [%s] failed: %s (ignore)...", 
                    shm[i], strerror(errno));
        }
    }
    
    NDRX_LOG(log_warn, "Removing semaphores...");
    
    ndrxd_sem_delete_with_init(qprefix);
    
    NDRX_LOG(log_warn, "Removing ndrxd pid file");
    
    if (NULL!=ndrxd_pid_file && EOS!=ndrxd_pid_file[0])
    {
        if (SUCCEED!=unlink(ndrxd_pid_file))
        {
            NDRX_LOG(log_error, "Failed to unlink [%s]: %s", 
                    ndrxd_pid_file, strerror(errno));
        }
    }
    else
    {
        NDRX_LOG(log_error, "Missing ndrxd PID file...");
    }
    
    NDRX_LOG(log_warn, "****** Done ******");
    
out:

    ex_string_list_free(qlist);
    ex_string_list_free(srvlist);
    ex_string_list_free(xadminlist);
    
    
    return ret;
}


/**
 * Kill process by mask.
 * @param m
 * @return 
 */
public int ex_killall(char *mask)
{
    string_list_t* plist = NULL;
    string_list_t* elt = NULL;
    int signals[] = {SIGTERM, SIGKILL};
    pid_t pid;
    int was_any = FALSE;
    int i;
    
    int ret = FAIL;
    
    plist = ex_sys_ps_list(mask, "", "", "");
    
    for (i=0; i<2; i++)
    {
        LL_FOREACH(plist,elt)
        {
            /* Parse out process name & pid */
            NDRX_LOG(log_warn, "processing proc: [%s]", elt->qname);
            
            if (SUCCEED==ex_get_pid_from_ps(elt->qname, &pid) && pid!=getpid() && pid!=0)
            {
                 NDRX_LOG(log_error, "! killing  sig=%d "
                         "pid=[%d]", signals[i], pid);
                 
                 if (SUCCEED!=kill(pid, signals[i]))
                 {
                     NDRX_LOG(log_error, "failed to kill with signal %d pid %d: %s",
                             signals[i], pid, strerror(errno));
                 }
                 was_any = TRUE;
                 ret = SUCCEED;
            }
        }
        if (0==i && was_any)
        {
            sleep(KILL_SLEEP_SECS);
        }
    }
    
    ex_string_list_free(plist);
    
    return ret;
}

/**
 * Checks for queue existance
 * @param qpath
 * @return TRUE/FALSE
 */
public int ex_q_exists(char *qpath)
{
    mqd_t tmp = ex_mq_open(qpath, O_RDONLY, O_NONBLOCK, NULL);
    
    if ((mqd_t)FAIL!=tmp)
    {
        ex_mq_close(tmp);
        return TRUE;
    }
    
    return FALSE;
}
