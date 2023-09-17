/**
 * @brief Singleton group lock provider
 *   At normal start it tries to lock immeditaly (however this gives slight risk
 *   that if node 2 just lost the lock, cpmsrv/ndrxd would not have enought time
 *   to kill the processes). 
 *
 * @file exsinglesv.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2023, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * AGPL (with Java and Go exceptions) or Mavimax's license for commercial use.
 * See LICENSE file for full text.
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

#include <ndebug.h>
#include <atmi.h>
#include <atmi_int.h>
#include <typed_buf.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <ubfutil.h>
#include <cconfig.h>
#include "exsinglesv.h"
#include <singlegrp.h>
#include <lcfint.h>
#include <exthpool.h>
#include <Exfields.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define PROGSECTION "@exsinglesv"
#define MIN_SGREFRESH_CEOFFICIENT   3 /**< Minimum devider to use faults            */
#define DEFAULT_CHECK_INTERVAL      5 /**< Default lock refresh interval            */
#define DEFAULT_THREADS             5 /**< Default number of threads                */
#define DEFAULT_SVCTOUT             3 /**< Default timeout for svc to disk fallback */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
expublic ndrx_exsinglesv_conf_t ndrx_G_exsinglesv_conf; /**< Configuration */

exprivate int M_init_ok_thpool = EXFALSE;      /**< is init ok? */
/*---------------------------Prototypes---------------------------------*/

/**
 * Un-init procedure.
 * Close any open lock files.
 * @param normal_unlock shall we perform normal unlock (i.e. we were locked)
 * @param force_unlock shall we force unlock (i.e. we were not locked)
 */
expublic void ndrx_exsinglesv_uninit(int normal_unlock, int force_unlock)
{
    int do_unlock=EXFALSE;

    if (ndrx_G_exsinglesv_conf.locked1)
    {
        ndrx_exsinglesv_file_unlock(NDRX_LOCK_FILE_1);

        if (normal_unlock || force_unlock)
        {
            do_unlock = EXTRUE;
        }
    }
    else if (force_unlock)
    {
        do_unlock = EXTRUE;
    }

    if (ndrx_G_exsinglesv_conf.locked2)
    {
        ndrx_exsinglesv_file_unlock(NDRX_LOCK_FILE_2);
    }

    if (do_unlock)
    {
        ndrx_sg_shm_t * sg = ndrx_sg_get(ndrx_G_exsinglesv_conf.procgrp_lp_no);

        if (NULL!=sg)
        {
            ndrx_sg_unlock(sg, 
                force_unlock?NDRX_SG_RSN_LOCKE:NDRX_SG_RSN_NORMAL);
        }
    }

    ndrx_G_exsinglesv_conf.is_locked=EXFALSE;
}

/**
 * Shutdown the thread
 * @param arg
 * @param p_finish_off
 */
expublic void thread_shutdown(void *ptr, int *p_finish_off)
{
    *p_finish_off = EXTRUE;
}

/**
 * Load error code to UBF buffer
 * @param p_ub UBF buffer
 * @param error_code error code
 * @param fmt format string
 */
expublic void ndrx_exsinglesv_set_error_fmt(UBFH *p_ub, long error_code, const char *fmt, ...)
{
    char msg[MAX_TP_ERROR_LEN+1] = {EXEOS};
    va_list ap;

    va_start(ap, fmt);
    (void) vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    TP_LOG(log_error, "Setting error %ld: [%s]", error_code, msg);

    Bchg(p_ub, EX_TPERRNO, 0, (char *)&error_code, 0L);
    Bchg(p_ub, EX_TPSTRERROR, 0, msg, 0L);
}

/**
 * Entry point for service (main thread)
 * @param p_svc
 */
void EXSINGLE (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;
    UBFH *p_ub = (UBFH *)p_svc->data; /* this is auto-buffer */
    long size;
    char btype[16];
    char stype[16];
    ndrx_thread_server_t *thread_data = NDRX_MALLOC(sizeof(ndrx_thread_server_t));
    char cmd[NDRX_SGMAX_CMDLEN+1]="";
    BFLDLEN cmd_bufsz=sizeof(cmd);
    threadpool pool;
    void (*p_func)(void*, int *);


    if (NULL==thread_data)
    {
        userlog("Failed to malloc memory - %s!", strerror(errno));
        TP_LOG(log_error, "Failed to malloc memory");
        EXFAIL_OUT(ret);
    }

    thread_data->buffer = p_svc->data; /*the buffer is not made free by thread */
    thread_data->cd = p_svc->cd;

    if (Bget(p_ub, EX_COMMAND, 0, (char *)&cmd, &cmd_bufsz))
    {
        TP_LOG(log_error, "Failed to read command code!");
        ret=EXFAIL;
        goto out;
    }

    if (0==strcmp(NDRX_SGCMD_VERIFY, cmd))
    {
        /* local thpool */
        pool=ndrx_G_exsinglesv_conf.thpool_local;
        p_func=ndrx_exsingle_local;
    }
    else if (0==strcmp(NDRX_SGCMD_QUERY, cmd))
    {
        /* remote thpool */
        pool=ndrx_G_exsinglesv_conf.thpool_remote;
        p_func=ndrx_exsingle_remote;
    }
    else
    {
        ndrx_exsinglesv_set_error_fmt((UBFH *)p_svc->data, TPEINVAL, "Unknown command [%s]", cmd);
        EXFAIL_OUT(ret);
    }

    if (NULL==(thread_data->context_data = tpsrvgetctxdata()))
    {
        TP_LOG(log_error, "Failed to get context data!");
        EXFAIL_OUT(ret);
    }

    /* send to pool */
    ndrx_thpool_add_work(pool, p_func, (void *)thread_data);

out:

    if (EXSUCCEED==ret)
    {
        /* serve next.. */
        tpcontinue();
    }
    else
    {
        /* return error back */
        tpreturn(  TPFAIL,
                0L,
                (char *)p_ub,
                0L,
                0L);
    }

}

/**
 * Do initialization
 * Have a local MIB & shared MIB
 */
int NDRX_INTEGRA(tpsvrinit)(int argc, char **argv)
{
    int ret=EXSUCCEED;
    ndrx_inicfg_t *cfg = NULL;
    ndrx_inicfg_section_keyval_t *params = NULL;
    ndrx_inicfg_section_keyval_t *el, *elt;
    char *p;
    int ndrx_sgrefresh;
    char svcnm[MAXTIDENT+1]={EXEOS};

    /* Only singleton server sections needed */
    char *sections[] = {"@singlesv",
                    NULL};

    if (EXSUCCEED!=ndrx_exsinglesv_sm_comp())
    {
        TP_LOG(log_error, "Statemachine error");
        userlog("Statemachine error");
        EXFAIL_OUT(ret);
    }

    memset(&ndrx_G_exsinglesv_conf, 0, sizeof(ndrx_G_exsinglesv_conf));

    /* set default: */
    ndrx_G_exsinglesv_conf.chkinterval = EXFAIL;
    ndrx_G_exsinglesv_conf.locked_wait = EXFAIL;
    ndrx_G_exsinglesv_conf.threads = DEFAULT_THREADS;
    ndrx_G_exsinglesv_conf.svc_timeout = DEFAULT_SVCTOUT;
    ndrx_G_exsinglesv_conf.clock_tolerance = EXFAIL;
    
    if (EXSUCCEED!=ndrx_cconfig_load_sections(&cfg, sections))
    {
        TP_LOG(log_error, "Failed to load configuration");
        EXFAIL_OUT(ret);
    }

    /* Load params by using ndrx_inicfg_get_subsect() */
    if (EXSUCCEED!=ndrx_cconfig_get_cf(cfg, PROGSECTION, &params))
    {
        TP_LOG(log_error, "Failed to load configuration section [%s]", PROGSECTION);
        EXFAIL_OUT(ret);
    }

    /* Iterate over params */
    EXHASH_ITER(hh, params, el, elt)
    {
        TP_LOG(log_info, "Param: [%s]=[%s]", el->key, el->val);

        /* read the params such as:
        [@exsinglegrp/<CCTAG>]
        singlegrp=4
        lockfile_1=/some/file.1
        lockfile_2=/some/file2
        exec_on_bootlocked=/some/script/to/exec.sh
        exec_on_locked=/some/script/to/exec.sh
        interval=3
        # this will make tototally to wait 6 seconds before taking over
        # (in case if we lock in non boot order)
        locked_wait=2
        to the global vars
        # number of threads for local requests
        # and number of threads for remote requests
        threads=5
        */
       
       /* read value from NDRX_SINGLEGRPLP env */
       if (0==strcmp(el->key, "lockfile_1"))
       {
            NDRX_STRCPY_SAFE(ndrx_G_exsinglesv_conf.lockfile_1, el->val);
       }
       else if (0==strcmp(el->key, "lockfile_2"))
       {
            NDRX_STRCPY_SAFE(ndrx_G_exsinglesv_conf.lockfile_2, el->val);
       }
       else if (0==strcmp(el->key, "exec_on_bootlocked"))
       {
            NDRX_STRCPY_SAFE(ndrx_G_exsinglesv_conf.exec_on_bootlocked, el->val);
       }
       else if (0==strcmp(el->key, "exec_on_locked"))
       {
            NDRX_STRCPY_SAFE(ndrx_G_exsinglesv_conf.exec_on_locked, el->val);
       }
       else if (0==strcmp(el->key, "chkinterval"))
       {
            ndrx_G_exsinglesv_conf.chkinterval = atoi(el->val);
       }
       /* Number of cycles to wait to proceed with group locking in shared
        * memory. This is needed in case if we take over, then let other node
        * to kill all the processes. This shall be larger than other nodes sanity cycle
        * length. This setting is number of chkinterval cycles.
        */
       else if (0==strcmp(el->key, "locked_wait"))
       {
            ndrx_G_exsinglesv_conf.locked_wait = atoi(el->val);
       }
       else if (0==strcmp(el->key, "threads"))
       {
            ndrx_G_exsinglesv_conf.threads = atoi(el->val);
       }
       else if (0==strcmp(el->key, "svctout"))
       {
            ndrx_G_exsinglesv_conf.svc_timeout = atoi(el->val);
       }/*
       else if (0==strcmp(el->key, "clock_tolerance"))
       {
            ndrx_G_exsinglesv_conf.clock_tolerance = atoi(el->val);
       }*/
       else
       {
            TP_LOG(log_debug, "Unknown parameter [%s]", el->key);
            EXFAIL_OUT(ret);
       }
    }

    p=getenv(CONF_NDRX_PROCGRP_LP_NO);

    if (NULL==p)
    {
        TP_LOG(log_error, "Missing %s environment variable", 
            CONF_NDRX_PROCGRP_LP_NO);
        EXFAIL_OUT(ret);
    }

    ndrx_G_exsinglesv_conf.procgrp_lp_no = atoi(p);

    /* check is it valid against singleton groups */
    if (!ndrx_sg_is_valid(ndrx_G_exsinglesv_conf.procgrp_lp_no))
    {
        TP_LOG(log_error, "Invalid singleton process group number [%d], "
            "check %s env setting",
            ndrx_G_exsinglesv_conf.procgrp_lp_no, CONF_NDRX_PROCGRP_LP_NO);
        EXFAIL_OUT(ret);
    }

    if (EXEOS==ndrx_G_exsinglesv_conf.lockfile_1[0])
    {
        TP_LOG(log_error, "Invalid lockfile_1");
        EXFAIL_OUT(ret);
    }

    if (EXEOS==ndrx_G_exsinglesv_conf.lockfile_2[0])
    {
        TP_LOG(log_error, "Invalid lockfile_2");
        EXFAIL_OUT(ret);
    }

    if (0==strcmp(ndrx_G_exsinglesv_conf.lockfile_1, ndrx_G_exsinglesv_conf.lockfile_2))
    {
        TP_LOG(log_error, "lockfile_1 and lockfile_2 shall be different");
        EXFAIL_OUT(ret);
    }

    ndrx_sgrefresh = ndrx_G_libnstd_cfg.sgrefreshmax;

    if (0>=ndrx_G_exsinglesv_conf.chkinterval)
    {        
        ndrx_G_exsinglesv_conf.chkinterval = ndrx_sgrefresh/MIN_SGREFRESH_CEOFFICIENT;

        /* generate error */
        if (ndrx_G_exsinglesv_conf.chkinterval<=0)
        {
            TP_LOG(log_error, "Invalid value for %s env setting. "
                "To use defaults, it shall be atleast %d", 
                CONF_NDRX_SGREFRESH, MIN_SGREFRESH_CEOFFICIENT);
            userlog("Invalid value for %s env setting. "
                "To use defaults, it shall be atleast %d", 
                CONF_NDRX_SGREFRESH, MIN_SGREFRESH_CEOFFICIENT);
            EXFAIL_OUT(ret);
        }
    }

    if (EXFAIL==ndrx_G_exsinglesv_conf.locked_wait)
    {
        /* giver other node time to detect and shutdown 
         * basically if other system has default 30 sec refresh time,
         * then in those 30 sec they should detect that lock is expired
         * and shutdown all the processes. So we shall wait twice the time.
         */
        ndrx_G_exsinglesv_conf.locked_wait = ndrx_sgrefresh/ndrx_G_exsinglesv_conf.chkinterval*2;
    }

    if (ndrx_G_exsinglesv_conf.threads<1)
    {
        ndrx_G_exsinglesv_conf.threads=DEFAULT_THREADS;
    }

/*
    if (ndrx_G_exsinglesv_conf.clock_tolerance < 0 ||
            ndrx_G_exsinglesv_conf.clock_tolerance >= 
                (ndrx_G_exsinglesv_conf.locked_wait*ndrx_G_exsinglesv_conf.chkinterval)
        )
    {
        TP_LOG(log_info, "clock_tolerance value %d, defaulting to check interval %d",
                ndrx_G_exsinglesv_conf.clock_tolerance, ndrx_G_exsinglesv_conf.chkinterval);
        ndrx_G_exsinglesv_conf.clock_tolerance = ndrx_G_exsinglesv_conf.chkinterval;
    }
*/
    /* call server for results */
    snprintf(svcnm, sizeof(svcnm), NDRX_SVC_SINGL, tpgetnodeid(), ndrx_G_exsinglesv_conf.procgrp_lp_no);

    /* Dump the configuration to the log file */
    TP_LOG(log_info, "svcnm = [%s]", svcnm);
    TP_LOG(log_info, "procgrp_lp_no=%d", ndrx_G_exsinglesv_conf.procgrp_lp_no);
    TP_LOG(log_info, "lockfile_1=[%s]", ndrx_G_exsinglesv_conf.lockfile_1);
    TP_LOG(log_info, "lockfile_2=[%s]", ndrx_G_exsinglesv_conf.lockfile_2);
    TP_LOG(log_info, "exec_on_bootlocked=[%s]", ndrx_G_exsinglesv_conf.exec_on_bootlocked);
    TP_LOG(log_info, "exec_on_locked=[%s]", ndrx_G_exsinglesv_conf.exec_on_locked);
    
    /* Key timing configuration: */
    TP_LOG(log_info, "ndrx_sgrefresh=%d", ndrx_sgrefresh);
    TP_LOG(log_info, "chkinterval=%d", ndrx_G_exsinglesv_conf.chkinterval);
    TP_LOG(log_info, "locked_wait=%d (number of chkinterval cycles)", 
        ndrx_G_exsinglesv_conf.locked_wait);
    TP_LOG(log_info, "threads=%d", ndrx_G_exsinglesv_conf.threads);
    TP_LOG(log_info, "svc_timeout=%s", ndrx_G_exsinglesv_conf.svc_timeout);
    /* TP_LOG(log_info, "clock_tolerance=%s", ndrx_G_exsinglesv_conf.clock_tolerance); 

    /* Validate check interval: */
    if (ndrx_G_exsinglesv_conf.chkinterval*MIN_SGREFRESH_CEOFFICIENT > ndrx_sgrefresh)
    {
        TP_LOG(log_warn, "WARNING: `%s' (%d) shall be at least %d times "
                "bigger than 'chkinterval' (%d)",
                CONF_NDRX_SGREFRESH, ndrx_sgrefresh, MIN_SGREFRESH_CEOFFICIENT,
                ndrx_G_exsinglesv_conf.chkinterval);
        userlog("WARNING: `%s' (%d) shall be at least %d times "
                "bigger than 'chkinterval' (%d)",
                CONF_NDRX_SGREFRESH, ndrx_sgrefresh, MIN_SGREFRESH_CEOFFICIENT,
                ndrx_G_exsinglesv_conf.chkinterval);
    }

    /* start the threads... */

    /* service request handlers */
    if (NULL==(ndrx_G_exsinglesv_conf.thpool_local = ndrx_thpool_init(ndrx_G_exsinglesv_conf.threads,
            NULL, NULL, NULL, 0, NULL)))
    {
        TP_LOG(log_error, "Failed to initialize local thread pool (cnt: %d)!",
                ndrx_G_exsinglesv_conf.threads);
        EXFAIL_OUT(ret);
    }

    if (NULL==(ndrx_G_exsinglesv_conf.thpool_remote = ndrx_thpool_init(ndrx_G_exsinglesv_conf.threads,
            NULL, NULL, NULL, 0, NULL)))
    {
        TP_LOG(log_error, "Failed to initialize remote thread pool (cnt: %d)!",
                ndrx_G_exsinglesv_conf.threads);
        EXFAIL_OUT(ret);
    }

    M_init_ok_thpool = EXTRUE;

    /* advertise our service ... */
    if (EXSUCCEED!=tpadvertise(svcnm, EXSINGLE))
    {
        TP_LOG(log_error, "Failed to advertise service [%s]: %s", 
            svcnm, tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    /* Register timer check.... */
    if (EXSUCCEED==ret &&
            EXSUCCEED!=tpext_addperiodcb(ndrx_G_exsinglesv_conf.chkinterval, 
            ndrx_exsinglesv_sm_run))
    {
        TP_LOG(log_error, "tpext_addperiodcb failed: %s",
                        tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }

    p=getenv(CONF_NDRX_RESPAWN);

    if (NULL!=p && 0==strcmp(p, "1"))
    {
        TP_LOG(log_warn, "Lock server respawn after the crash, "
            "will use locked_wait for first lock");
            ndrx_G_exsinglesv_conf.first_boot = EXFALSE;
    }
    else
    {
        ndrx_G_exsinglesv_conf.first_boot = EXTRUE;
    }
    /* perform first check
    * so that on boot, the first locked node would boot without any interruptions */
    if (EXSUCCEED!=ndrx_exsinglesv_sm_run())
    {
        TP_LOG(log_error, "Failed to perform lock check");
        EXFAIL_OUT(ret);
    }

    ndrx_G_exsinglesv_conf.first_boot = EXFALSE;
    /* report us as lock provider */
    tpext_configprocgrp_lp (ndrx_G_exsinglesv_conf.procgrp_lp_no);

    M_init_ok_thpool=EXTRUE;

out:

    if (NULL!=params)
    {
        ndrx_keyval_hash_free(params);
    }

    if (NULL!=cfg)
    {
        ndrx_inicfg_free(cfg);
    }

    return ret;
}

void NDRX_INTEGRA(tpsvrdone)(void)
{
    int i;

    /* un-init thread pool */

    /* Terminate the threads (request) */
    for (i=0; i<ndrx_G_exsinglesv_conf.threads; i++)
    {
        ndrx_thpool_add_work(ndrx_G_exsinglesv_conf.thpool_local, (void *)thread_shutdown, NULL);
    }

    /* update threads */
    for (i=0; i<ndrx_G_exsinglesv_conf.threads; i++)
    {
        ndrx_thpool_add_work(ndrx_G_exsinglesv_conf.thpool_remote, (void *)thread_shutdown, NULL);
    }

    /* unlock the group from SHM and unlock / close the lockfiles */
    ndrx_exsinglesv_uninit(EXTRUE, EXFALSE);
    
}

/* vim: set ts=4 sw=4 et smartindent: */
