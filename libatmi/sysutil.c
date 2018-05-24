/* 
** System utilities
**
** @file sysutil.c
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
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <errno.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <signal.h>

#include <ndrstandard.h>
#include <exhash.h>
#include <ndebug.h>
#include <ndrxdcmn.h>
#include <userlog.h>
#include <ubf.h>
#include <ubfutil.h>
#include <sys_unix.h>
#include <sys_mqueue.h>
#include <utlist.h>
#include <atmi_shm.h>
#include <exregex.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/**
 * List of queues (for queued messages)
 */
typedef struct qcache_hash qcache_hash_t;
struct qcache_hash
{
    char svcq[NDRX_MAX_Q_SIZE+1]; /* hash by this */
    char svcq_full[NDRX_MAX_Q_SIZE+1]; /* full queue name */
    
    EX_hash_handle hh; /* makes this structure hashable        */
};

MUTEX_LOCKDECL(M_q_cache_lock); /* lock the queue cache */
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

exprivate qcache_hash_t *M_qcache = NULL; /* queue cache for non shm mode. */

/**
 * Check is server running
 */
expublic int ndrx_chk_server(char *procname, short srvid)
{
    int ret = EXFALSE;
    char test_string3[NDRX_MAX_KEY_SIZE+4];
    char test_string4[64];
    string_list_t * list;
     
    snprintf(test_string3, sizeof(test_string3), "-k %s", G_atmi_env.rnd_key);
    snprintf(test_string4, sizeof(test_string4), "-i %hd", srvid);
    
    list =  ndrx_sys_ps_list(ndrx_sys_get_cur_username(), procname, 
            test_string3, test_string4, "");
    
    if (NULL!=list)
    {
        NDRX_LOG(log_debug, "process %s -i %hd running ok", procname, srvid);
        ret = EXTRUE;
    }
    else
    {
        NDRX_LOG(log_debug, "process %s -i %hd not running...", procname, srvid);
    }
    
    
    ndrx_string_list_free(list);
   
    return ret;
}


/**
 * Check is `ndrxd' daemon running
 */
expublic int ndrx_chk_ndrxd(void)
{
    int ret = EXFALSE;
    char test_string3[NDRX_MAX_KEY_SIZE+4];
    string_list_t * list;
     
    snprintf(test_string3, sizeof(test_string3), "-k %s", G_atmi_env.rnd_key);
    
    list =  ndrx_sys_ps_list(ndrx_sys_get_cur_username(), "ndrxd", 
            test_string3, "", "");
    
    if (NULL!=list)
    {
        NDRX_LOG(log_debug, "process `ndrxd' running ok");
        ret = EXTRUE;
    }
    else
    {
        NDRX_LOG(log_debug, "process `ndrxd' not running...");
    }
    
    
    ndrx_string_list_free(list);
    
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
expublic int ndrx_parse_clt_q(char *q, char *pfx, char *proc, pid_t *pid, long *th)
{
    char tmp[NDRX_MAX_Q_SIZE+1];
    char *token;
    int ret = EXSUCCEED;
    
    pfx[0] = EXEOS;
    proc[0] = EXEOS;
    *pid = 0;
    *th = 0;

    if (NULL==strstr(q, NDRX_CLT_QREPLY_CHK))
    {
        NDRX_LOG(log_debug, "[%s] - not client Q", q);
        ret = EXFAIL;
        goto out;
    }
            
    NDRX_STRCPY_SAFE(tmp, q);
    
    /* get the first token */
    token = strtok(tmp, NDRX_FMT_SEP_STR);

    if (NULL!=token)
    {
        strcpy(pfx, token);
    }
    else
    {
        NDRX_LOG(log_error, "missing pfx")
        EXFAIL_OUT(ret);
    }
    
    
    token = strtok(NULL, NDRX_FMT_SEP_STR);
    if (NULL==token)
    {
        NDRX_LOG(log_error, "missing clt")
        EXFAIL_OUT(ret);
    }
    
    token = strtok(NULL, NDRX_FMT_SEP_STR);
    if (NULL==token)
    {
        NDRX_LOG(log_error, "missing reply")
        EXFAIL_OUT(ret);
    }
    
    token = strtok(NULL, NDRX_FMT_SEP_STR);
    
    if (NULL!=token)
    {
        strcpy(proc, token);
    }
    else
    {
        NDRX_LOG(log_error, "missing proc name")
        EXFAIL_OUT(ret);
    }
    
    token = strtok(NULL, NDRX_FMT_SEP_STR);
    
    if (NULL!=token)
    {
        *pid=atoi(token);
    }
    else
    {
        NDRX_LOG(log_error, "missing proc pid")
        EXFAIL_OUT(ret);
    }
    
    token = strtok(NULL, NDRX_FMT_SEP_STR);
    
    if (NULL!=token)
    {
        *th=atol(token);
    }
    else
    {
        NDRX_LOG(log_error, "missing proc th")
        EXFAIL_OUT(ret);
    }
    
out:
    return ret;
}





/**
 * Kill the system running (the xadmin dies last...)
 */
expublic int ndrx_down_sys(char *qprefix, char *qpath, int is_force)
{
    int ret = EXSUCCEED;
#define DOWN_KILL_SIG   1
    int signals[] = {SIGTERM, SIGKILL};
    int i;
    string_list_t* qlist = NULL;
    string_list_t* srvlist = NULL;
    string_list_t* srvlist2 = NULL;
    string_list_t* ndrxdlist = NULL;
    string_list_t* cpmsrvs = NULL;
    string_list_t* xadminlist = NULL;
    string_list_t* cltchildren = NULL;
    string_list_t* elt = NULL;
    string_list_t* elt2 = NULL;
    string_list_t* qclts = NULL;
    char pfx[NDRX_MAX_Q_SIZE+1];
    char proc[NDRX_MAX_Q_SIZE+1];
    pid_t pid, ppid;
    long th;
    char test_string2[NDRX_MAX_KEY_SIZE+4];
    char srvinfo[NDRX_MAX_SHM_SIZE];
    char svcinfo[NDRX_MAX_SHM_SIZE];
    char brinfo[NDRX_MAX_SHM_SIZE];
    char *shm[] = {srvinfo, svcinfo, brinfo};
    char *ndrxd_pid_file = getenv(CONF_NDRX_DPID);
    int max_signals = 2;
    int was_any = EXFALSE;
    pid_t my_pid = getpid();
    char *username;
    char env_mask[PATH_MAX];
    regex_t srv2rex;
    int srv2rex_compiled = EXFALSE;
    NDRX_LOG(log_warn, "****** Forcing system down ******");
    
    
    snprintf(srvinfo, sizeof(srvinfo), NDRX_SHM_SRVINFO, qprefix);
    snprintf(svcinfo, sizeof(svcinfo), NDRX_SHM_SVCINFO, qprefix);
    snprintf(brinfo, sizeof(brinfo),  NDRX_SHM_BRINFO,  qprefix);
    
     
    snprintf(test_string2, sizeof(test_string2), "-k %s", G_atmi_env.rnd_key);
    

    if (is_force)
    {
        signals[0] = SIGKILL;
    }
    
    /* list all queues */
    qlist = ndrx_sys_mqueue_list_make(qpath, &ret);
    
    if (EXSUCCEED!=ret)
    {
        NDRX_LOG(log_error, "posix queue listing failed... continue...!");
        ret = EXSUCCEED;
        qlist = NULL;
    }
    
    username = ndrx_sys_get_cur_username();
    
    /* THIS IS FIRST!!!! We do not want continues respawning!!!
     * kill any ndrxd, as they can respawn xatmi servers 
     * But... tprecover might running and restoring ndrxd back
     * thus we need somehow in loop kill both with -9
     */
    NDRX_LOG(log_debug, "Killing the ndrxd and tprecover...");
    do
    {
        was_any = EXFALSE;
        
        ndrxdlist = ndrx_sys_ps_list(username, test_string2, 
                "", "", "[\\s/ ]*ndrxd[\\s ]");
        
        srvlist = ndrx_sys_ps_list(username, test_string2, 
                "", "", "[\\s/ ]*tprecover[\\s ]");
        
        LL_FOREACH(ndrxdlist,elt)
        {
            if (EXSUCCEED==ndrx_proc_pid_get_from_ps(elt->qname, &pid))
            {
                 NDRX_LOG(log_error, "! killing (ndrxd)  sig=%d "
                         "pid=[%d] (%s)", signals[DOWN_KILL_SIG], pid, elt->qname);

                 if (EXSUCCEED!=kill(pid, signals[DOWN_KILL_SIG]))
                 {
                     NDRX_LOG(log_error, "failed to kill with signal %d pid %d: %s",
                             signals[i], pid, strerror(errno));
                 }
                 else
                 {
                    was_any = EXTRUE;
                 }
            }
        }
        
        LL_FOREACH(srvlist,elt)
        {
            if (EXSUCCEED==ndrx_proc_pid_get_from_ps(elt->qname, &pid))
            {
                 NDRX_LOG(log_error, "! killing (tprecover)  sig=%d "
                         "pid=[%d] (%s)", signals[DOWN_KILL_SIG], pid, elt->qname);

                 if (EXSUCCEED!=kill(pid, signals[DOWN_KILL_SIG]))
                 {
                     NDRX_LOG(log_error, "failed to kill with signal %d pid %d: %s",
                             signals[i], pid, strerror(errno));
                 }
                 else
                 {
                    was_any = EXTRUE;
                 }
            }
        }
        
        ndrx_string_list_free(ndrxdlist);
        ndrxdlist = NULL;
        ndrx_string_list_free(srvlist);
        srvlist = NULL;
        
    } while(was_any);
    
    /* Get the pid of cpmsrv - from queue, I guess or from PS output..? */
    
    /* 
     * List child processes of the cpmsrv
     * and kill the cpmsrv and the child processes
     */
    NDRX_LOG(log_debug, "Searching child processes of the cpmsrv");
            
    cpmsrvs = ndrx_sys_ps_list(username, test_string2, 
                "", "", "[\\s/ ]*cpmsrv[\\s ]");
    
    LL_FOREACH(cpmsrvs,elt2)
    {
        /* List the children of the cpmsrv... */
        if (EXSUCCEED==ndrx_proc_pid_get_from_ps(elt2->qname, &ppid))
        {
            
            NDRX_LOG(log_warn, "CPMSRV PID = %d, extracting children", ppid);
            
            qclts = ndrx_sys_ps_getchilds(ppid);
            was_any = EXFALSE;
            
            NDRX_LOG(log_warn, "! Children extracted, about kill the cpmsrv...");
            /* At this moment we must kill the CPM, as it will spawn the children 
             * The children list extract and parent can be killed
             */
            if (EXSUCCEED!=kill(ppid, signals[0]))
            {
                NDRX_LOG(log_error, "failed to kill with signal %d pid %d: %s",
                        signals[i], ppid, strerror(errno));
            }

            sleep(EX_KILL_SLEEP_SECS);
            
            if (EXSUCCEED!=kill(ppid, signals[1]))
            {
                NDRX_LOG(log_error, "failed to kill with signal %d pid %d: %s",
                        signals[i], ppid, strerror(errno));
            }
            
            NDRX_LOG(log_warn, "Now kill the child processes one by one");
            for (i=0; i<max_signals; i++)
            {
                LL_FOREACH(qclts,elt)
                {
                    /* Parse out process name & pid */
                    NDRX_LOG(log_warn, "processing proc: [%s]", elt->qname);

                    if (EXSUCCEED==ndrx_proc_pid_get_from_ps(elt->qname, &pid))
                    {
                        if (0==i)
                        {
                            ndrx_proc_children_get_recursive(&cltchildren, pid);
                        }

                        NDRX_LOG(log_error, "! killing  sig=%d "
                                "pid=[%d] mypid=[%d]", signals[i], pid, my_pid);

                        if (EXSUCCEED!=kill(pid, signals[i]))
                        {
                            NDRX_LOG(log_error, "failed to kill with signal %d pid %d: %s",
                                    signals[i], pid, strerror(errno));
                        }
                        was_any = EXTRUE;
                    }
                }
                
                if (0==i && was_any)
                {
                    sleep(EX_KILL_SLEEP_SECS);
                }
            }
            
            ndrx_string_list_free(qclts);
            qclts = NULL;
            
            /* kill the children of the children */
            ndrx_proc_kill_list(cltchildren);
            ndrx_string_list_free(cltchildren);
            
            cltchildren = NULL;
            
            qclts = NULL;
        }
        else
        {
            NDRX_LOG(log_error, "Failed to extract pid from: [%s]", elt->qname);
        }
    }
    
    /* 
     * kill all servers 
     */
    was_any = EXFALSE;
    NDRX_LOG(log_warn, "Removing server processes for user [%s] and key [%s]", 
        username, test_string2);
    
    srvlist = ndrx_sys_ps_list(username, test_string2, 
                "", "", "");
    
    for (i=0; i<max_signals; i++)
    {
        LL_FOREACH(srvlist,elt)
        {
            /* Parse out process name & pid */
            NDRX_LOG(log_warn, "processing proc: [%s]", elt->qname);
            
            if (EXSUCCEED==ndrx_proc_pid_get_from_ps(elt->qname, &pid))
            {
                 NDRX_LOG(log_error, "! killing  sig=%d "
                         "pid=[%d] (%s)", signals[i], pid, elt->qname);
                 
                 if (EXSUCCEED!=kill(pid, signals[i]))
                 {
                     NDRX_LOG(log_error, "failed to kill with signal %d pid %d: %s",
                             signals[i], pid, strerror(errno));
                 }
                 was_any = EXTRUE;
            }
        }
        if (0==i && was_any)
        {
            sleep(EX_KILL_SLEEP_SECS);
        }
    }

    /* Kill servers by looking up environment variables!!!
     * needs to implement API calls for linux/mac/freebsd/aix/solaris
     */
    
    was_any = EXFALSE;
    
    snprintf(env_mask, sizeof(env_mask), "%s.{0,2}%s.*-i [0-9]+.*--",
                CONF_NDRX_SVCLOPT, test_string2);
    
    /* Compile regex */
    if (EXSUCCEED!=ndrx_regcomp(&srv2rex, env_mask))
    {
        NDRX_LOG(log_error, "Failed to compile regexp [%s]", env_mask);
        EXFAIL_OUT(ret);
    }
    srv2rex_compiled = EXTRUE;
    
    NDRX_LOG(log_warn, "Removing server processes for user [%s] and env mask [%s]", 
        username, test_string2);
    
    srvlist2 = ndrx_sys_ps_list(username, "", 
                "", "", "");
            
    for (i=0; i<max_signals; i++)
    {
        LL_FOREACH(srvlist2,elt)
        {
            /* Parse out process name & pid */
            NDRX_LOG(log_warn, "processing proc: [%s]", elt->qname);
            
            if (EXSUCCEED==ndrx_proc_pid_get_from_ps(elt->qname, &pid) &&
                    EXTRUE==ndrx_sys_env_test(pid, &srv2rex))
            {
                 NDRX_LOG(log_error, "! killing  sig=%d "
                         "pid=[%d] (%s)", signals[i], pid, elt->qname);
                 
                 if (EXSUCCEED!=kill(pid, signals[i]))
                 {
                     NDRX_LOG(log_error, "failed to kill with signal %d pid %d: %s",
                             signals[i], pid, strerror(errno));
                 }
                 was_any = EXTRUE;
            }
        }
        if (0==i && was_any)
        {
            sleep(EX_KILL_SLEEP_SECS);
        }
    }
    
    NDRX_LOG(log_warn, "Removing all client processes.. (by Q)");
    /* Kill the children against the Q 
     * DO this after the servers. So that servers have no chance to respawn
     * any client
     */
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
            
            if (EXSUCCEED==ndrx_parse_clt_q(elt->qname, pfx, proc, &pid, &th) &&
                    0!=strcmp(proc, "xadmin"))
            {
                if (0==i)
                {
                    ndrx_proc_children_get_recursive(&cltchildren, pid);
                }
                
                NDRX_LOG(log_error, "! killing  sig=%d pfx=[%s] proc=[%s] "
                        "pid=[%d] th=[%ld]", signals[i], pfx, proc, pid, th);
                if (EXSUCCEED!=kill(pid, signals[i]))
                {
                    NDRX_LOG(log_error, "failed to kill with signal %d pid %d: %s",
                            signals[i], pid, strerror(errno));
                }
                else
                {
                   was_any = EXTRUE;
                }
            }
        }
        
        if (0==i && was_any)
        {
            sleep(EX_KILL_SLEEP_SECS);
        }
    }
    
    /* kill the children of the children */
    ndrx_proc_kill_list(cltchildren);
    ndrx_string_list_free(cltchildren);
    cltchildren = NULL;

    /* remove all xadmins... */
    NDRX_LOG(log_warn, "Removing other xadmins...");
    was_any = EXFALSE;
    xadminlist = ndrx_sys_ps_list(username, "xadmin", 
        "", "", "");
    
    for (i=0; i<max_signals; i++)
    {
        LL_FOREACH(xadminlist,elt)
        {
            /* Parse out process name & pid */
            NDRX_LOG(log_warn, "processing proc: [%s]", elt->qname);
            
            if (EXSUCCEED==ndrx_proc_pid_get_from_ps(elt->qname, &pid) && pid!=my_pid)
            {
                 NDRX_LOG(log_error, "! killing  sig=%d "
                         "pid=[%d] mypid=[%d]", signals[i], pid, my_pid);
                 
                 if (EXSUCCEED!=kill(pid, signals[i]))
                 {
                     NDRX_LOG(log_error, "failed to kill with signal %d pid %d: %s",
                             signals[i], pid, strerror(errno));
                 }
                 else
                 {
                    was_any = EXTRUE;
                 }
            }
        }
        
        if (0==i && was_any)
        {
            sleep(EX_KILL_SLEEP_SECS);
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

        if (EXSUCCEED!=ndrx_mq_unlink(elt->qname))
        {
            NDRX_LOG(log_error, "failed to remove q [%s]: %s",
                    elt->qname, strerror(errno));
        }
    }
    
    NDRX_LOG(log_warn, "Removing shared memory...");
    
    for (i=0; i<N_DIM(shm); i++)
    {
        NDRX_LOG(log_warn, "Unlinking [%s]", shm[i]);
        
        if (EXSUCCEED!=shm_unlink(shm[i]))
        {
            NDRX_LOG(log_warn, "shm_unlink [%s] failed: %s (ignore)...", 
                    shm[i], strerror(errno));
        }
    }
    
    NDRX_LOG(log_warn, "Removing semaphores...");
    
    ndrxd_sem_delete_with_init(qprefix);
    
    NDRX_LOG(log_warn, "Removing ndrxd pid file");
    
    if (NULL!=ndrxd_pid_file && EXEOS!=ndrxd_pid_file[0])
    {
        if (EXSUCCEED!=unlink(ndrxd_pid_file))
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

    ndrx_string_list_free(qlist);
    ndrx_string_list_free(srvlist);
    ndrx_string_list_free(srvlist2);
    ndrx_string_list_free(xadminlist);
    ndrx_string_list_free(cpmsrvs);
    ndrx_string_list_free(qclts);
    ndrx_string_list_free(ndrxdlist);
    ndrx_string_list_free(cltchildren);
    
    if (srv2rex_compiled)
    {
        ndrx_regfree(&srv2rex);
    }
    
    return ret;
}


/**
 * Kill process by mask.
 * @param m
 * @return 
 */
expublic int ndrx_killall(char *mask)
{
    string_list_t* plist = NULL;
    string_list_t* elt = NULL;
    int signals[] = {SIGTERM, SIGKILL};
    pid_t pid;
    int was_any = EXFALSE;
    int i;
    
    int ret = EXFAIL;
    
    plist = ndrx_sys_ps_list(mask, "", "", "", "");
    
    for (i=0; i<2; i++)
    {
        LL_FOREACH(plist,elt)
        {
            /* Parse out process name & pid */
            NDRX_LOG(log_warn, "processing proc: [%s]", elt->qname);
            
            if (EXSUCCEED==ndrx_proc_pid_get_from_ps(elt->qname, &pid) && pid!=getpid() && pid!=0)
            {
                 NDRX_LOG(log_error, "! killing  sig=%d "
                         "pid=[%d]", signals[i], pid);
                 
                 if (EXSUCCEED!=kill(pid, signals[i]))
                 {
                     NDRX_LOG(log_error, "failed to kill with signal %d pid %d: %s",
                             signals[i], pid, strerror(errno));
                 }
                 was_any = EXTRUE;
                 ret = EXSUCCEED;
            }
        }
        if (0==i && was_any)
        {
            sleep(EX_KILL_SLEEP_SECS);
        }
    }
    
    ndrx_string_list_free(plist);
    
    return ret;
}

/**
 * Checks for queue existance
 * @param qpath
 * @return TRUE/FALSE
 */
expublic int ndrx_q_exists(char *qpath)
{
    mqd_t tmp = ndrx_mq_open(qpath, O_RDONLY, O_NONBLOCK, NULL);
    
    if ((mqd_t)EXFAIL!=tmp)
    {
        ndrx_mq_close(tmp);
        return EXTRUE;
    }
    
    return EXFALSE;
}

/**
 * Check service in cache. If found but cannot open, then remove from cache
 * @param q
 * @return SUCCEED (found & replaced)/FAIL
 */
exprivate int chk_cached_svc(char *svcq, char *svcq_full)
{
    qcache_hash_t * ret = NULL;
   
    MUTEX_LOCK_V(M_q_cache_lock);
    
    EXHASH_FIND_STR( M_qcache, svcq, ret);
    
    if (NULL==ret)
    {
        NDRX_LOG(log_debug, "Service q [%s] not in cache", svcq);
        goto out;
    }
    else
    {
        NDRX_LOG(log_debug, "Service q [%s] found in cache, testing...", svcq);
        if (ndrx_q_exists(svcq))
        {
            NDRX_LOG(log_debug, "cached queue exists ok");
        }
        else
        {
            NDRX_LOG(log_warn, "Cached queue [%s] does not exists", svcq);
            EXHASH_DEL(M_qcache, ret);
            NDRX_FREE(ret);
            ret=NULL;
        }
    }
    
out:
    MUTEX_UNLOCK_V(M_q_cache_lock);
    
    if (NULL==ret)
        return EXFAIL;
    else
        return EXSUCCEED;
}

/**
 * Add add full queue mapping to cache
 * @param q
 * @param fullq
 * @return 
 */
exprivate int add_cached_svc(char *svcq, char *svcq_full)
{
    qcache_hash_t * ret = NDRX_CALLOC(1, sizeof(qcache_hash_t));
    
    MUTEX_LOCK_V(M_q_cache_lock);
    
    if (NULL==ret)
    {
        NDRX_LOG(log_error, "Failed to alloc qcache_hash_t: %s", strerror(errno));
        userlog("Failed to alloc qcache_hash_t: %s", strerror(errno));
    }
    
    NDRX_STRCPY_SAFE(ret->svcq, svcq);
    NDRX_STRCPY_SAFE(ret->svcq_full, svcq_full);
    
    EXHASH_ADD_STR( M_qcache, svcq, ret );
    
    MUTEX_UNLOCK_V(M_q_cache_lock);
    
    if (NULL!=ret)
        return EXSUCCEED;
    else
        return EXFAIL;
}

/**
 * Return cached queue (usable in poll mode).
 * @param q
 * @return SUCCEED/FAIL
 */
expublic int ndrx_get_cached_svc_q(char *q)
{
    int ret=EXSUCCEED;
    int found = EXFALSE;
    string_list_t* qlist = NULL;
    string_list_t* elt = NULL;
    char svcq[NDRX_MAX_Q_SIZE+1];
    
    NDRX_STRCPY_SAFE(svcq, q);
    
    if (EXSUCCEED==chk_cached_svc(svcq, q))
    {
        NDRX_LOG(log_info, "Got cached service: [%s]", q);
        return EXSUCCEED;
    }
    
    qlist = ndrx_sys_mqueue_list_make(G_atmi_env.qpath, &ret);
    
    if (EXSUCCEED!=ret)
    {
        NDRX_LOG(log_error, "posix queue listing failed!");
        EXFAIL_OUT(ret);
    }
    
    strcat(q, NDRX_FMT_SEP_STR);
    
    LL_FOREACH(qlist,elt)
    {
        /* if not print all, then skip this queue */
        if (0==strncmp(elt->qname,  q, strlen(q)))
        {
            strcpy(q, elt->qname);
            NDRX_LOG(log_debug, "Non shm mode, found Q: [%s]", q);
            found = EXTRUE;
            break;
        }
    }
    
    if (!found)
    {
        NDRX_LOG(log_error, "No servers for [%s] according to Q list", q);
        EXFAIL_OUT(ret);
    }
    
    /* save the server in cache... */
    add_cached_svc(svcq, q);
    
out:
    if (NULL!=qlist)
    {
        ndrx_string_list_free(qlist);
    }
    return ret;
}
