/**
 * @brief Library initialization & configuration.
 *   This also serves as init for client part (may be called via atmi.c)
 *   We will support simple threading. Support for tpgetctx() and tpsetctx()
 *   will not be supported.
 *
 * @file init.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or Mavimax's license for commercial use.
 * -----------------------------------------------------------------------------
 * GPL license:
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * -----------------------------------------------------------------------------
 * A commercial use license is available from Mavimax, Ltd
 * contact@mavimax.com
 * -----------------------------------------------------------------------------
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <memory.h>
#include <unistd.h>
#include <sys_mqueue.h>
#include <errno.h>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */

#include <ndrstandard.h>
#include <ndebug.h>
#include <atmi.h>
#include <atmi_int.h>
#include <sys_mqueue.h>
#include <userlog.h>
#include <tperror.h>
#include <xa_cmn.h>
#include <atmi_shm.h>
#include <sys_unix.h>
#include <atmi_tls.h>
#include <cconfig.h>
#include <typed_view.h>
#include <atmi_cache.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define MAX_CONTEXTS                1000
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/

int G_srv_id = EXFAIL; /* If we are server, then this will be server ID */
volatile int G_is_env_loaded = 0; /* Is environment initialised */
/* NOTE: THIS BELLOW ONE IS NOT INITIALIZED FOR NDRXD! */
atmi_lib_env_t G_atmi_env; /* ATMI library environmental configuration */
/* List of context slots... */
long M_contexts[MAX_CONTEXTS];

MUTEX_LOCKDECL(M_env_lock);

/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Return free context id...
 * We should have linked list of contexts...
 * @return 
 */
expublic long ndrx_ctxid_op(int make_free, long ctxid)
{
    MUTEX_LOCK;
    {
        static int first = EXTRUE;
        long ret=EXFAIL;
        long i;
        
        if (first)
        {
            /* Invalidate context slots */
            memset(M_contexts, EXFAIL, sizeof(M_contexts));
            first = EXFALSE;
        }
        
        /* TODO: Check for boundary!? */
        if (make_free)
        {
            NDRX_LOG(log_debug, "Marking context %ld as free", ctxid);
            M_contexts[ctxid] = EXFAIL;
        }
        else
        {
            for (i=0; i<MAX_CONTEXTS; i++)
            {
                if (EXFAIL==M_contexts[i])
                {
                    NDRX_LOG(log_debug, "Got free context id=%ld", i);
                    M_contexts[i] = i;
                    ret = i;
                    break;
                }
            }
        }
        
out:     
        NDRX_LOG(log_debug, "Returning context id=%ld", ret);
        MUTEX_UNLOCK;
        return ret;
    }
}


/**
 * One time system init
 * @return 
 */
exprivate int ndrx_init_once(void)
{
    int ret = EXSUCCEED;
    char buf[NDRX_MSGSIZEMAX]; /* test the stack. */
    /* reset callstates to default */
    /* memset(&G_call_state, 0, sizeof(G_call_state)); */
    
out:
    return ret;
}
/**
 * This function loads common environment variables.
 * This should be called by any binary which uses atmi library!
 * If this fails, then binary should fail too!
 * @return 
 */
expublic int ndrx_load_common_env(void)
{
    int ret=EXSUCCEED;
    char *p;
    
    MUTEX_LOCK_V(M_env_lock);
    
    if (G_is_env_loaded)
    {
        NDRX_LOG(log_debug, "env already loaded...");
        goto out;
    }
    
    if (EXSUCCEED!=ndrx_init_once())
    {
        NDRX_LOG(log_error, "Init once failed");
        EXFAIL_OUT(ret);
    }
    
    /*
     * Here is the main entry point for common-confg
     * everything start with debug, thus read the system-wide config here.
     * NDRX_CCONF - optional, if set use CCONF, if not set fallback to old-style
     * NDRX_CCTAG - optional, if set use as sub-section
     */
     
    if (EXSUCCEED!=ndrx_cconfig_load())
    {
        fprintf(stderr, "GENERAL CONFIGURATION ERROR\n");
        exit(EXFAIL);
    }
    
    /* Read MAX servers */
    p = getenv(CONF_NDRX_SRVMAX);
    
    if (NULL==p)
    {
        fprintf(stderr, "********************************************************************************\n");
        fprintf(stderr, "**                         CONFIGURATION ERROR !                              **\n");
        fprintf(stderr, "**                         ... now worry                                      **\n");
        fprintf(stderr, "**                                                                            **\n");
        fprintf(stderr, "** Enduro/X Application server is not in proper environment or not configured **\n");
        fprintf(stderr, "**                                                                            **\n");
        fprintf(stderr, "** Possible causes:                                                           **\n");
        fprintf(stderr, "** - Classical environment variables are not loaded (see ex_env(5) man page)  **\n");
        fprintf(stderr, "** - Or Common-Config NDRX_CCONFIG env variable is not set                    **\n");
        fprintf(stderr, "** See \"Getting Started Tutorial\" in order to get system up-and-running       **\n");
        fprintf(stderr, "** More info can be found here http://www.endurox.org/dokuwiki                **\n");
        fprintf(stderr, "**                                                                            **\n");
        fprintf(stderr, "** Process is now terminating with failure                                    **\n");
        fprintf(stderr, "********************************************************************************\n");
        exit(EXFAIL);
    }
    
    if (NULL==p)
    {
        /* Write to ULOG? */
        NDRX_LOG(log_error, "Missing config key %s - FAIL", CONF_NDRX_SRVMAX);
        userlog("Missing config key %s - FAIL", CONF_NDRX_SRVMAX);
        ret=EXFAIL;
        goto out;
    }
    else
    {
        G_atmi_env.max_servers = atoi(p);
        NDRX_LOG(log_debug, "Max servers set to %d", G_atmi_env.max_servers);
    }

    /* Read MAX SVCs per server */
    p = getenv(CONF_NDRX_SVCMAX);
    if (NULL==p)
    {
        /* Write to ULOG? */
        NDRX_LOG(log_error, "Missing config key %s - FAIL", CONF_NDRX_SVCMAX);
        userlog("Missing config key %s - FAIL", CONF_NDRX_SVCMAX);
        ret=EXFAIL;
        goto out;
    }
    else
    {
        G_atmi_env.max_svcs = atoi(p);
        NDRX_LOG(log_debug, "Max services set to %d", G_atmi_env.max_servers);
    }


    p = getenv(CONF_NDRX_RNDK);
    if (NULL==p)
    {
        /* Write to ULOG? */
        NDRX_LOG(log_error, "Missing config key %s - FAIL", CONF_NDRX_RNDK);
        userlog("Missing config key %s - FAIL", CONF_NDRX_RNDK);
        ret=EXFAIL;
        goto out;
    }
    else
    {
        NDRX_STRCPY_SAFE(G_atmi_env.rnd_key, p);
        NDRX_LOG(log_debug, "Random key set to: [%s]", G_atmi_env.rnd_key);
    }

    p = getenv(CONF_NDRX_MSGMAX);
    if (NULL==p)
    {
        /* Write to ULOG? */
        NDRX_LOG(log_error, "Missing config key %s - FAIL", CONF_NDRX_MSGMAX);
        userlog("Missing config key %s - FAIL", CONF_NDRX_MSGMAX);
        ret=EXFAIL;
        goto out;
    }
    else
    {
        G_atmi_env.msg_max = atoi(p);
        NDRX_LOG(log_debug, "Posix queue msg_max set to: [%d]",
                            G_atmi_env.msg_max);
    }

    p = getenv(CONF_NDRX_MSGSIZEMAX);
    if (NULL==p)
    {
        /* Write to ULOG? */
        NDRX_LOG(log_error, "Missing config key %s - FAIL", CONF_NDRX_MSGSIZEMAX);
        userlog("Missing config key %s - FAIL", CONF_NDRX_MSGSIZEMAX);
        ret=EXFAIL;
        goto out;
    }
    else
    {
        G_atmi_env.msgsize_max = atoi(p);
        NDRX_LOG(log_debug, "Posix queue msgsize_max set to: [%d]",
                            G_atmi_env.msgsize_max);
    }

    p = getenv(CONF_NDRX_TESTMODE);
    if (NULL!=p)
    {
        G_atmi_env.testmode = atoi(p);
        NDRX_LOG(log_debug, "Test mode set to: %d", G_atmi_env.testmode);
    }
    
    p = getenv(CONF_NDRX_QPREFIX);
    if (NULL==p)
    {
        /* Write to ULOG? */
        NDRX_LOG(log_error, "Missing config key %s - FAIL", CONF_NDRX_QPREFIX);
        userlog("Missing config key %s - FAIL", CONF_NDRX_QPREFIX);
        EXFAIL_OUT(ret);
    }
    else
    {
        NDRX_STRCPY_SAFE(G_atmi_env.qprefix, p);
        
        snprintf(G_atmi_env.qprefix_match, sizeof(G_atmi_env.qprefix_match),
                "%s%c", G_atmi_env.qprefix, NDRX_FMT_SEP);
        
        G_atmi_env.qprefix_match_len = strlen(G_atmi_env.qprefix_match);
        
        NDRX_LOG(log_debug, "Posix queue prefix set to: [%s], "
                            "match string: [%s] (len: %d)",
                            G_atmi_env.qprefix, G_atmi_env.qprefix_match,
                            G_atmi_env.qprefix_match_len);
    }
    
    p = getenv(CONF_NDRX_QPATH);
    if (NULL==p)
    {
        /* Write to ULOG? */
        NDRX_LOG(log_error, "Missing config key %s - FAIL", CONF_NDRX_QPATH);
        userlog("Missing config key %s - FAIL", CONF_NDRX_QPATH);
        ret=EXFAIL;
        goto out;
    }
    else
    {
        NDRX_STRCPY_SAFE(G_atmi_env.qpath, p);
        
        NDRX_LOG(log_debug, "Posix queue queue path set to: [%s]",
                            G_atmi_env.qpath);
    }
    
    p = getenv(CONF_NDRX_IPCKEY);
    if (NULL==p)
    {
        /* Write to ULOG? */
        NDRX_LOG(log_error, "Missing config key %s - FAIL", CONF_NDRX_IPCKEY);
        userlog("Missing config key %s - FAIL", CONF_NDRX_IPCKEY);
        ret=EXFAIL;
        goto out;
    }
    else
    {
	int tmpkey;

	/* bsd warning of long: */
        sscanf(p, "%x", &tmpkey);
	G_atmi_env.ipckey = tmpkey;

        NDRX_LOG(log_debug, "SystemV SEM IPC Key set to: [%x]",
                            G_atmi_env.ipckey);
    }
    
    /* Get timeout */
    if (NULL!=(p=getenv(CONF_NDRX_TOUT)))
    {
        G_atmi_env.time_out = atoi(p);
        NDRX_LOG(log_debug, "Using comms timeout: %d", G_atmi_env.time_out);
    }
    
    /* Get node id */
    if (NULL!=(p=getenv(CONF_NDRX_NODEID)))
    {
        G_atmi_env.our_nodeid = atoi(p);
        
        if (G_atmi_env.our_nodeid<CONF_NDRX_NODEID_MIN || 
                G_atmi_env.our_nodeid>CONF_NDRX_NODEID_MAX)
        {
            NDRX_LOG(log_error, "Invalid [%s] setting! Min: %d, Max %d, got: %hd",
                    CONF_NDRX_NODEID, CONF_NDRX_NODEID_MIN, CONF_NDRX_NODEID_MAX,
                    G_atmi_env.our_nodeid);
            ret=EXFAIL;
            goto out;
        }
        NDRX_LOG(log_debug, "Cluster node id=%hd", G_atmi_env.our_nodeid);
    }
    
    /* Get load balance settings */
    
    if (NULL!=(p=getenv(CONF_NDRX_LDBAL)))
    {
        G_atmi_env.ldbal = atoi(p);
        
        if (G_atmi_env.ldbal<0 || 
                G_atmi_env.ldbal>100)
        {
            NDRX_LOG(log_error, "%s - invalid: min 0, max 100, got: %d",
                    CONF_NDRX_LDBAL, G_atmi_env.ldbal);
            ret=EXFAIL;
            goto out;
        }
        NDRX_LOG(log_debug, "%s set to %d", 
                CONF_NDRX_LDBAL, G_atmi_env.ldbal);
    }
    
    /* CONF_NDRX_CLUSTERISED */
    if (NULL!=(p=getenv(CONF_NDRX_CLUSTERISED)))
    {
        G_atmi_env.is_clustered = atoi(p);
        
        NDRX_LOG(log_debug, "[%s] says: We run in %s mode", 
                CONF_NDRX_CLUSTERISED,
                G_atmi_env.is_clustered?"cluster":"non cluster/single node");
    }
    
    
    /* <XA Protocol configuration - currently optional...> */
    
    /* resource id: */
    if (NULL!=(p=getenv(CONF_NDRX_XA_RES_ID)))
    {
        G_atmi_env.xa_rmid = atoi(p);
        NDRX_LOG(log_debug, "[%s]: XA Resource ID: %d", 
                CONF_NDRX_XA_RES_ID,
                G_atmi_env.xa_rmid);
    }
    
    /* Open string: */
    if (NULL!=(p=getenv(CONF_NDRX_XA_OPEN_STR)))
    {
        NDRX_STRCPY_SAFE(G_atmi_env.xa_open_str, p);
        NDRX_LOG(log_debug, "[%s]: XA Open String: [%s]", 
                CONF_NDRX_XA_OPEN_STR,
                G_atmi_env.xa_open_str);
    }
    
    /* Close string: */
    if (NULL!=(p=getenv(CONF_NDRX_XA_CLOSE_STR)))
    {
        NDRX_STRCPY_SAFE(G_atmi_env.xa_close_str, p);
        NDRX_LOG(log_debug, "[%s]: XA Close String: [%s]", 
                CONF_NDRX_XA_CLOSE_STR,
                G_atmi_env.xa_close_str);
    }
    else
    {
        NDRX_STRCPY_SAFE(G_atmi_env.xa_close_str, G_atmi_env.xa_open_str);
        NDRX_LOG(log_debug, "[%s]: XA Close String defaulted to: [%s]", 
                CONF_NDRX_XA_CLOSE_STR,
                G_atmi_env.xa_close_str);
    }
    
    /* Driver lib: */
    if (NULL!=(p=getenv(CONF_NDRX_XA_DRIVERLIB)))
    {
        NDRX_STRCPY_SAFE(G_atmi_env.xa_driverlib, p);
        NDRX_LOG(log_debug, "[%s]: Enduro/X XA Driver lib (.so): [%s]", 
                CONF_NDRX_XA_DRIVERLIB,
                G_atmi_env.xa_driverlib);
    }
    
    /* Resource manager lib: */
    if (NULL!=(p=getenv(CONF_NDRX_XA_RMLIB)))
    {
        NDRX_STRCPY_SAFE(G_atmi_env.xa_rmlib, p);
        NDRX_LOG(log_debug, "[%s]: Resource manager lib (.so): [%s]", 
                CONF_NDRX_XA_RMLIB,
                G_atmi_env.xa_rmlib);
    }
    
    if (NULL!=(p=getenv(CONF_NDRX_XA_FLAGS)))
    {
        NDRX_STRCPY_SAFE(G_atmi_env.xa_flags, p);
        NDRX_LOG(log_debug, "[%s]: XA Enduro/X specific flags: [%s]", 
                CONF_NDRX_XA_FLAGS, G_atmi_env.xa_flags);
    }
    else
    {
        G_atmi_env.xa_flags[0] = EXEOS;
    }
    
    if (NULL!=(p=getenv(CONF_NDRX_XA_LAZY_INIT)))
    {
        G_atmi_env.xa_lazy_init = atoi(p);
    }
    
    NDRX_LOG(log_debug, "[%s]: Lazy XA Init: %s", 
                CONF_NDRX_XA_LAZY_INIT,
                G_atmi_env.xa_lazy_init?"TRUE":"FALSE");
    
    /* If enabled, then validate the config */
    if (G_atmi_env.xa_rmid)
    {
        if (
            EXEOS==G_atmi_env.xa_open_str[0] ||
            EXEOS==G_atmi_env.xa_close_str[0] ||
            EXEOS==G_atmi_env.xa_driverlib[0] ||
            EXEOS==G_atmi_env.xa_rmlib[0])
        {
            NDRX_LOG(log_error, "Invalid XA configuration, missing "
                    "%s or %s or %s or %s keys...",
                    CONF_NDRX_XA_OPEN_STR,
                    CONF_NDRX_XA_CLOSE_STR,
                    CONF_NDRX_XA_DRIVERLIB,
                    CONF_NDRX_XA_RMLIB);
            EXFAIL_OUT(ret);
        }
        else
        {
            NDRX_LOG(log_debug, "XA config ok");
        }
        
        if (!G_atmi_env.xa_lazy_init)
        {
            NDRX_LOG(log_debug, "Loading XA driver...");
            if (EXSUCCEED!=atmi_xa_init())
            {
                NDRX_LOG(log_error, "Failed to load XA driver!!!");
                EXFAIL_OUT(ret);
            }
        }
    }
    
    /* </XA Protocol configuration> */
    
    
    /* <poll() mode configuration> */
    
    /* Number of semaphores used for shared memory protection: 
     * - for epoll() mode only 1 needed
     * - for poll() more is better as it will be balanced for every
     *   service/shared memory access (due to round robin sending to service)
     */
    if (NULL!=(p=getenv(CONF_NDRX_NRSEMS)))
    {
        G_atmi_env.nrsems = atoi(p);
        
        if (!G_atmi_env.nrsems)
        {
            G_atmi_env.nrsems = CONF_NDRX_NRSEMS_DFLT;
        }
        
    }
    else
    {
        G_atmi_env.nrsems = CONF_NDRX_NRSEMS_DFLT;
    }
    
    if (G_atmi_env.nrsems < 2)
    {
        G_atmi_env.nrsems = 2;
    }
    
    NDRX_LOG(log_debug, "[%s]: Number of services shared memory semaphores "
                "set to: %d (used only for poll() mode) (default: %d)", 
                CONF_NDRX_NRSEMS, G_atmi_env.nrsems, CONF_NDRX_NRSEMS_DFLT);
    
    /* Max servers per service: */
    if (NULL!=(p=getenv(CONF_NDRX_MAXSVCSRVS)))
    {
        G_atmi_env.maxsvcsrvs = atoi(p);
        
        if (!G_atmi_env.maxsvcsrvs)
        {
            G_atmi_env.maxsvcsrvs = CONF_NDRX_MAXSVCSRVS_DFLT;
        }
        
    }
    else
    {
        G_atmi_env.maxsvcsrvs = CONF_NDRX_MAXSVCSRVS_DFLT;
    }
    
    
    NDRX_LOG(log_debug, "[%s]: Max number of local servers per service "
                "set to: %d (used only for poll() mode) (default: %d)", 
                CONF_NDRX_MAXSVCSRVS, G_atmi_env.maxsvcsrvs, CONF_NDRX_MAXSVCSRVS_DFLT);
    
    /* </poll() mode configuration> */

    /* Init the util lib.. */ 
    if (EXSUCCEED!=ndrx_atmiutil_init())
    {
       NDRX_LOG(log_error, "ndrx_atmiutil_init() failed");
       EXFAIL_OUT(ret);
    }
   
    NDRX_LOG(log_debug, "env loaded ok");
    G_is_env_loaded = EXTRUE;
out:
    MUTEX_UNLOCK_V(M_env_lock);
    return ret;
}
/**
 * Close open client session
 * @return
 */
expublic int ndrx_tpterm (void)
{
    int ret=EXSUCCEED;
    char fn[] = "_tpterm";
    
    ATMI_TLS_ENTRY;
    
    NDRX_LOG(log_debug, "%s called", fn);

    if (!G_atmi_tls->G_atmi_is_init)
    {
        NDRX_LOG(log_debug, "%s ATMI is not initialized - "
				"nothing to do.", fn);
        goto out;
    }
    
    if (!G_atmi_tls->G_atmi_conf.is_client)
    {
        ret=EXFAIL;
        ndrx_TPset_error_msg(TPEPROTO, "tpterm called from server!");
        goto out;
    }

    /* Close client connections */
    if (EXSUCCEED!=close_open_client_connections())
    {
        ret=EXFAIL;
        ndrx_TPset_error_msg(TPESYSTEM, "Failed to close conversations!");
        goto out;
    }

    /* Shutdown client queues */
    if (0!=G_atmi_tls->G_atmi_conf.reply_q)
    {
        if (EXFAIL==ndrx_mq_close(G_atmi_tls->G_atmi_conf.reply_q))
        {
            NDRX_LOG(log_warn, "Failed to close [%s]: %s",
                                G_atmi_tls->G_atmi_conf.reply_q_str, strerror(errno));
            /* nothing to do with this error!
            _TPset_error_fmt(TPEOS, "Failed to close [%s]: %s",
                                G_atmi_conf.reply_q_str, strerror(errno));
            ret=FAIL;
            goto out;
             */
        }
    }

    if (EXEOS!=G_atmi_tls->G_atmi_conf.reply_q_str[0])
    {
        NDRX_LOG(log_debug, "Unlinking [%s]", G_atmi_tls->G_atmi_conf.reply_q_str);
        if (EXFAIL==ndrx_mq_unlink(G_atmi_tls->G_atmi_conf.reply_q_str))
        {
            NDRX_LOG(log_warn, "Failed to unlink [%s]: %s",
                                G_atmi_tls->G_atmi_conf.reply_q_str, strerror(errno));
    
            /* really no error!
            _TPset_error_fmt(TPEOS, "Failed to unlink [%s]: %s",
                                G_atmi_conf.reply_q_str, strerror(errno));
            ret=FAIL;
            goto out;
            */
        }
    }

    /* Fee up context, should be last otherwise we might unlink other thread's
     * opened queue in this context!!! */
    ndrx_ctxid_op(EXTRUE, G_atmi_tls->G_atmi_conf.contextid);
    
    /* Un init the library */
    G_atmi_tls->G_atmi_is_init = EXFALSE;
    NDRX_LOG(log_debug, "%s: ATMI library un-initialized", fn);
    
    /* close also  */
    atmi_xa_uninit();

out:
    NDRX_LOG(log_debug, "%s returns %d", fn, ret);
    return ret;
}

/**
 * Do updates for reply Q init.
 * @param qd
 * @return 
 */
expublic int tp_internal_init_upd_replyq(mqd_t reply_q, char *reply_q_str)
{
    int ret=EXSUCCEED;
    char fn[]="tp_internal_init";
    ATMI_TLS_ENTRY;
    
    G_atmi_tls->G_atmi_conf.reply_q = reply_q;
    NDRX_STRCPY_SAFE(G_atmi_tls->G_atmi_conf.reply_q_str, reply_q_str);
    if (EXFAIL==ndrx_mq_getattr(reply_q, &G_atmi_tls->G_atmi_conf.reply_q_attr))
    {
        ndrx_TPset_error_fmt(TPEOS, "%s: Failed to read attributes for queue fd %d: %s",
                            fn, reply_q, strerror(errno));
        ret=EXFAIL;
        goto out;
    }
    
out:
    return ret;
}
 
/**
 * Roll in ATMI library configuration.
 * @param init_data
 * @return SUCCEED/FAIL
 */
expublic int tp_internal_init(atmi_lib_conf_t *init_data)
{
    int ret=EXSUCCEED;
    char fn[]="tp_internal_init";
    static int first = EXTRUE;
    int sem_fail = EXFALSE;
    ATMI_TLS_ENTRY;
    /* we connect to semaphore  */
    /* Check that if we are client (in server staging, then close current queues) */
    if (G_atmi_tls->G_atmi_is_init && G_atmi_tls->G_atmi_conf.is_client)
    {
        if (!init_data->is_client)
        {
            NDRX_LOG(log_debug, "Staged to server - "
                                "shutting down client session");
            
            /*  attach to server shm segment. */
            ndrx_shm_attach_all(NDRX_SHM_LEV_SRV);
        }
        else
        {
            NDRX_LOG(log_debug, "Client re-initialisation - "
                                "shutting down old session");
        }

        if (EXFAIL==ndrx_mq_close(G_atmi_tls->G_atmi_conf.reply_q))
        {
            NDRX_LOG(log_warn, "Failed to close [%s]: %s",
                                G_atmi_tls->G_atmi_conf.reply_q_str, strerror(errno));
        }

        NDRX_LOG(log_debug, "Unlinking [%s]", G_atmi_tls->G_atmi_conf.reply_q_str);
        
        if (EXFAIL==ndrx_mq_unlink(G_atmi_tls->G_atmi_conf.reply_q_str))
        {
            NDRX_LOG(log_warn, "Failed to unlink [%s]: %s",
                                G_atmi_tls->G_atmi_conf.reply_q_str, strerror(errno));
        }
    }

    /* Copy the configuration here */
    G_atmi_tls->G_atmi_conf = *init_data;
    G_atmi_tls->G_atmi_is_init = 1;
    
    /* reset last call (server side stuff) */
    memset(&G_atmi_tls->G_last_call, 0, sizeof(G_atmi_tls->G_last_call));
    
    /* reset conversation info */
    memset(&G_atmi_tls->G_tp_conversation_status, 0, 
            sizeof(G_atmi_tls->G_tp_conversation_status));

    /* reset our acceptance info */
    memset(&G_atmi_tls->G_accepted_connection, 0, 
            sizeof(G_atmi_tls->G_accepted_connection));

    /* read queue attributes -  only if Q was open...*/
    if (init_data->reply_q && EXFAIL==ndrx_mq_getattr(init_data->reply_q, 
            &G_atmi_tls->G_atmi_conf.reply_q_attr))
    {
        ndrx_TPset_error_fmt(TPEOS, "%s: Failed to read attributes for queue [%s] fd %d: %s",
                            fn, init_data->reply_q_str, init_data->reply_q, strerror(errno));
        ret=EXFAIL;
        goto out;
    }

    /* format the name of ndrxd queue: */
    snprintf(G_atmi_tls->G_atmi_conf.ndrxd_q_str, 
            sizeof(G_atmi_tls->G_atmi_conf.ndrxd_q_str), NDRX_NDRXD, 
            G_atmi_tls->G_atmi_conf.q_prefix);
    NDRX_LOG(log_debug, "NDRXD queue: [%s]", G_atmi_tls->G_atmi_conf.ndrxd_q_str);
    
    /* we attach to shared mem & semaphores only once. */
    MUTEX_LOCK;
    {
        if (first)
        {
#if 0
            /* Will load uppon request... */
            NDRX_LOG(log_info, "About to load view files...");
            /* Load the view files (if any defined for the system) */
            if (NULL!=getenv(CONF_VIEWDIR))
            {
                if (EXSUCCEED!=ndrx_view_chkload_directories())
                {
                    MUTEX_UNLOCK;
                    
                    NDRX_LOG(log_error, "Failed to load view files!");
                    
                    EXFAIL_OUT(ret);
                }
            }
            else
            {
                NDRX_LOG(log_warn, "%s not set, not loading views", CONF_VIEWDIR);
            }
#endif
            
            /* Init semaphores first. */
            ndrxd_sem_init(G_atmi_tls->G_atmi_conf.q_prefix);
            
            /* Try to attach to semaphore array */
            if (EXSUCCEED!=ndrx_sem_attach_all())
            {
                NDRX_LOG(log_error, "Failed to attache to semaphores!!");
                sem_fail = EXTRUE;
                /*ret=FAIL;
                goto out;*/
            }
            
            /* Attach to client shared memory? */
            if (EXSUCCEED==ndrx_shm_init(G_atmi_tls->G_atmi_conf.q_prefix, 
                        G_atmi_env.max_servers, G_atmi_env.max_svcs))
            {
                if (init_data->is_client)
                {
                    if (EXSUCCEED==ndrx_shm_attach_all(NDRX_SHM_LEV_SVC | NDRX_SHM_LEV_BR) &&
                            sem_fail)
                    {
                        NDRX_LOG(log_error, "SHM ok, but sem fail -"
                                " cannot operate in this mode!");
                        MUTEX_UNLOCK;
                        EXFAIL_OUT(ret);
                    }
                }
                else
                {
                    /* In case of server we attach to both shared memory blocks */
                    if (EXSUCCEED==ndrx_shm_attach_all(NDRX_SHM_LEV_SVC | 
                                        NDRX_SHM_LEV_SRV | NDRX_SHM_LEV_BR) &&
                            sem_fail)
                    
                    {
                        NDRX_LOG(log_error, "SHM ok, but sem fail -"
                                " cannot operate in this mode!");
                        MUTEX_UNLOCK;
                        EXFAIL_OUT(ret);
                    }
                }
            }


            /* Read the init cache */
            if (EXSUCCEED!=ndrx_cache_init(NDRX_TPCACH_INIT_NORMAL))
            {
                NDRX_LOG(log_error, "Cache init failed");
                MUTEX_UNLOCK;
                EXFAIL_OUT(ret);
            }
            
            first = EXFALSE;
        }
        MUTEX_UNLOCK;
    }
    
out:
    return ret;
}

/**
 * Initialize client. Not sure should this be called by server process?
 * Maybe... or we will create separate part for server to initialize with monitor
 * @return SUCCEED/FAIL
 */
expublic int tpinit (TPINIT * init_data)
{
    int ret=EXSUCCEED;
    atmi_lib_conf_t conf;
    char reply_q[NDRX_MAX_Q_SIZE+1];
    char my_id[NDRX_MAX_ID_SIZE+1];
    char *p;
    char read_clt_name[MAXTIDENT+1]={EXEOS};
    static pid_t pid;
    ATMI_TLS_ENTRY;
    
    if (G_atmi_tls->G_atmi_is_init)
    {
        NDRX_LOG(log_info, "ATMI already initialized...");
        goto out;
    }

    memset(&conf, 0, sizeof(conf));
    /* assume that client only can call this */
    conf.is_client = 1;
    
    /* Load common environment for client - this should be synced...*/
    if (EXSUCCEED!=ndrx_load_common_env())
    {
        NDRX_LOG(log_error, "Failed to load common env");
        ret=EXFAIL;
        goto out;
    }
    
    /* Load the queue prefix */
    if (NULL==(p=getenv(CONF_NDRX_QPREFIX)))
    {
        ndrx_TPset_error_msg(TPEINVAL, "Env NDRX_QPREFIX not set");
        ret=EXFAIL;
        goto out;
    }
    else
    {
        NDRX_STRCPY_SAFE(conf.q_prefix, p);
        NDRX_LOG(log_debug, "Got prefix [%s]", conf.q_prefix);
        
    }

    /* Get the PID of the process */
    pid = getpid();
    
    NDRX_STRCPY_SAFE(read_clt_name, EX_PROGNAME);
    NDRX_LOG(log_debug, "Got PROGNAME [%s]", read_clt_name);
    
    /* Get new context id. Threading support only for clients... */
    conf.contextid = ndrx_ctxid_op(EXFALSE, EXFAIL);
    NDRX_DBG_SETTHREAD(conf.contextid);
    
    /* Format my ID */
    if (EXFAIL==G_srv_id)
    {
        snprintf(my_id, sizeof(my_id), NDRX_MY_ID_CLT, 
                /* we always assume that name is process name...!
                 * This is how enduro/x is built.
                 */
                /*init_data!=NULL?init_data->cltname:*/read_clt_name, 
                pid, 
                conf.contextid, 
                G_atmi_env.our_nodeid);
        
        NDRX_STRCPY_SAFE(conf.my_id, my_id);
    }
    else
    {
        snprintf(my_id, sizeof(my_id), NDRX_MY_ID_SRV, 
                /*init_data!=NULL?init_data->cltname:*/read_clt_name, 
                G_srv_id, 
                pid,
                conf.contextid, /* Bug #119 server multicontext fixes... */
                G_atmi_env.our_nodeid);
        NDRX_STRCPY_SAFE(conf.my_id, my_id);
    }

    NDRX_LOG(log_debug, "my_id=[%s]", conf.my_id);

    snprintf(reply_q, sizeof(reply_q), NDRX_CLT_QREPLY, conf.q_prefix,
                read_clt_name, pid, conf.contextid); /* no client name if no provided */

    /* at first try to un-link existing queue */
    ndrx_mq_unlink(reply_q);

    NDRX_STRCPY_SAFE(conf.reply_q_str, reply_q);
    /* now try to open the queue, by default we will have blocked access */
    NDRX_LOG(log_debug, "About to open client queue [%s]", reply_q);
    conf.reply_q = ndrx_mq_open_at(reply_q, O_RDONLY | O_CREAT, S_IWUSR | S_IRUSR, NULL);
    NDRX_LOG(log_debug, "After client Q open: %d", conf.reply_q);
    if ((mqd_t)EXFAIL==conf.reply_q)
    {
        ndrx_TPset_error_fmt(TPEOS, "Failed to open queue [%s] errno: %s", 
                    conf.reply_q_str, strerror(errno));
        EXFAIL_OUT(ret);
    }

    NDRX_LOG(log_debug, "Client queue [%s] opened mqd_t=%d.",
		conf.reply_q_str, conf.reply_q);

    if (NULL!=init_data)
    {
        memcpy(&G_atmi_tls->client_init_data, init_data, sizeof(*init_data));
    }
    /* do the library initialisation */
    ret=tp_internal_init(&conf);
    
out:
    return ret;
}

/**
 * Shutdown the thread
 * @param arg
 * @param p_finish_off
 */
expublic void tp_thread_shutdown(void *ptr, int *p_finish_off)
{
    tpterm();
    
    *p_finish_off = EXTRUE;
}
/* vim: set ts=4 sw=4 et smartindent: */
