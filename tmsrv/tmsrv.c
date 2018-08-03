/**
 * @brief Tmsrv server - transaction monitor
 *   After that log transaction to hash & to disk for tracking the stuff...
 *   TODO: We should have similar control like "TP_COMMIT_CONTROL" -
 *   either return after stuff logged or after really commit completed.
 *   Error handling:
 *   - System errors we will track via ATMI interface error functions
 *   - XA errors will be tracked via XA error interface
 *   Should we call xa_end for joined transactions? See:
 *   https://www-01.ibm.com/support/knowledgecenter/SSFKSJ_7.0.1/com.ibm.mq.amqzag.doc/fa13870_.htm
 *   TODO: count the XA_RETRY as part of the transaction retry counter.
 *   i.e. if state is not changed counter++
 *   have a new flag for max count to return heuristic and move to background.
 *
 * @file tmsrv.c
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

#include <exnet.h>
#include <ndrxdcmn.h>

#include "tmsrv.h"
#include "../libatmisrv/srv_int.h"
#include "tperror.h"
#include "userlog.h"
#include <xa_cmn.h>
#include "thpool.h"
#include <ubfutil.h>
/*---------------------------Externs------------------------------------*/
extern int optind, optopt, opterr;
extern char *optarg;
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
expublic tmsrv_cfg_t G_tmsrv_cfg;
/*---------------------------Statics------------------------------------*/
exprivate int M_init_ok = EXFALSE;
/* Wait for one free thread: */
pthread_mutex_t M_wait_th_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t M_wait_th_cond = PTHREAD_COND_INITIALIZER;

/*
 * Thread private data
 */
exprivate __thread ndrx_stopwatch_t M_ping_stopwatch;
exprivate __thread int M_thread_first = EXTRUE;
exprivate __thread XID M_ping_xid; /* run pings by this non existent xid */
/*---------------------------Prototypes---------------------------------*/
exprivate int tm_tout_check(void);
exprivate void tm_chk_one_free_thread(void *ptr, int *p_finish_off);

/**
 * Initialize thread
 */
expublic void tm_thread_init(void)
{
    if (EXSUCCEED!=tpinit(NULL))
    {
        NDRX_LOG(log_error, "Failed to init worker client");
        userlog("tmsrv: Failed to init worker client");
        exit(1);
    }

    /* Bug #161 We shall run xa_open() here too, because it is per thread
     * config.
     */
    if (EXSUCCEED!=tpopen())
    {
        NDRX_LOG(log_error, "Worker thread failed to tpopen() - nothing to do, "
                "process will exit");
        userlog("Worker thread failed to tpopen() - nothing to do, "
                "process will exit");
        exit(1);
    }

    ndrx_stopwatch_reset(&M_ping_stopwatch);
    
    atmi_xa_new_xid(&M_ping_xid);
    
}

/**
 * Close the thread session
 */
expublic void tm_thread_uninit(void)
{
    tpclose();
    tpterm();
}

/**
 * Tmsrv service entry (working thread)
 * @param p_svc - data & len used only...!
 */
void TPTMSRV_TH (void *ptr, int *p_finish_off)
{
    /* Ok we should not handle the commands 
     * TPBEGIN...
     */
    int ret=EXSUCCEED;
    thread_server_t *thread_data = (thread_server_t *)ptr;
    char cmd = EXEOS;
    int cd;
    
    /**************************************************************************/
    /*                        THREAD CONTEXT RESTORE                          */
    /**************************************************************************/
    UBFH *p_ub = (UBFH *)thread_data->buffer;
    
    /* Do the ATMI init, if needed */
    if (M_thread_first)
    {
        tm_thread_init();
        M_thread_first = EXFALSE;
    }
    
    /* run the ping (will be skipped if thread is already pinged in time) */
    if (G_tmsrv_cfg.ping_time > 0)
    {
        tm_ping_db(NULL, NULL);
    }
    
    /* restore context. */
    if (EXSUCCEED!=tpsrvsetctxdata(thread_data->context_data, SYS_SRV_THREAD))
    {
        userlog("tmsrv: Failed to set context");
        NDRX_LOG(log_error, "Failed to set context");
        exit(1);
    }
    
    cd = thread_data->cd;
    /* free up the transport data.*/
    NDRX_FREE(thread_data->context_data);
    NDRX_FREE(thread_data);
    /**************************************************************************/
    
    /* get some more stuff! */
    if (Bunused (p_ub) < 4096)
    {
        p_ub = (UBFH *)tprealloc ((char *)p_ub, Bsizeof (p_ub) + 4096);
    }
    
    ndrx_debug_dump_UBF(log_info, "TPTMSRV call buffer:", p_ub);
    
    if (Bget(p_ub, TMCMD, 0, (char *)&cmd, 0L))
    {
        NDRX_LOG(log_error, "Failed to read command code!");
        ret=EXFAIL;
        goto out;
    }
    NDRX_LOG(log_info, "Got command code: [%c]", cmd);
    
    switch(cmd)
    {
        case ATMI_XA_TPBEGIN:
            
            /* start new tran... */
            if (EXSUCCEED!=tm_tpbegin(p_ub))
            {
                ret=EXFAIL;
                goto out;
            }
            break;
        case ATMI_XA_TPCOMMIT:
            
            /* start new tran... */
            if (EXSUCCEED!=tm_tpcommit(p_ub))
            {
                ret=EXFAIL;
                goto out;
            }
            break;
        case ATMI_XA_TPABORT:
            
            /* start new tran... */
            if (EXSUCCEED!=tm_tpabort(p_ub))
            {
                ret=EXFAIL;
                goto out;
            }
            break;
        case ATMI_XA_PRINTTRANS:
            
            /* request for printing active transactions */
            if (EXSUCCEED!=tm_tpprinttrans(p_ub, cd))
            {
                ret=EXFAIL;
                goto out;
            }
            break;
        case ATMI_XA_ABORTTRANS:
            
            /* request for printing active transactions */
            if (EXSUCCEED!=tm_aborttrans(p_ub))
            {
                ret=EXFAIL;
                goto out;
            }
            break;
        case ATMI_XA_COMMITTRANS:
            
            /* request for printing active transactions */
            if (EXSUCCEED!=tm_committrans(p_ub))
            {
                ret=EXFAIL;
                goto out;
            }
            break;
        case ATMI_XA_TMPREPARE:
            
            /* prepare the stuff locally */
            if (EXSUCCEED!=tm_tmprepare(p_ub))
            {
                ret=EXFAIL;
                goto out;
            }
            break;
        case ATMI_XA_TMCOMMIT:
            
            /* prepare the stuff locally */
            if (EXSUCCEED!=tm_tmcommit(p_ub))
            {
                ret=EXFAIL;
                goto out;
            }
            break;
        case ATMI_XA_TMABORT:
            
            /* abort the stuff locally */
            if (EXSUCCEED!=tm_tmabort(p_ub))
            {
                ret=EXFAIL;
                goto out;
            }
            break;
        case ATMI_XA_TMREGISTER:
            /* Some binary is telling as the different RM is involved
             * in transaction.
             */
            if (EXSUCCEED!=tm_tmregister(p_ub))
            {
                ret=EXFAIL;
                goto out;
            }
            break;
        default:
            NDRX_LOG(log_error, "Unsupported command code: [%c]", cmd);
            ret=EXFAIL;
            break;
    }
    
out:
            
    /* Approve the request if all ok */
    if (EXSUCCEED==ret)
    {
        atmi_xa_approve(p_ub);
    }

    if (EXSUCCEED!=ret && XA_RDONLY==atmi_xa_get_reason())
    {
        NDRX_LOG(log_debug, "Marking READ ONLY = SUCCEED");
        ret=EXSUCCEED;
    }

    ndrx_debug_dump_UBF(log_info, "TPTMSRV return buffer:", p_ub);

    tpreturn(  ret==EXSUCCEED?TPSUCCESS:TPFAIL,
                0L,
                (char *)p_ub,
                0L,
                0L);
    
    /* note this is thread and it does not do long jump */
    /* 
     * If there was very long processing session and ping is required (detected
     * by func). Then run it here.
     */
    if (G_tmsrv_cfg.ping_time > 0)
    {
        tm_ping_db(NULL, NULL);
    }
}

/**
 * Entry point for service (main thread)
 * @param p_svc
 */
void TPTMSRV (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;
    UBFH *p_ub = (UBFH *)p_svc->data; /* this is auto-buffer */
    long size;
    char btype[16];
    char stype[16];
    thread_server_t *thread_data = NDRX_MALLOC(sizeof(thread_server_t));
    
    if (NULL==thread_data)
    {
        userlog("Failed to malloc memory - %s!", strerror(errno));
        NDRX_LOG(log_error, "Failed to malloc memory");
        EXFAIL_OUT(ret);
    }
    
    if (0==(size = tptypes (p_svc->data, btype, stype)))
    {
        NDRX_LOG(log_error, "Zero buffer received!");
        userlog("Zero buffer received!");
        EXFAIL_OUT(ret);
    }
    
#if 0
        - Why?
    /* not using sub-type - on tpreturn/forward for thread it will be auto-free */
    thread_data->buffer =  tpalloc(btype, NULL, size);
    
    if (NULL==thread_data->buffer)
    {
        NDRX_LOG(log_error, "tpalloc failed of type %s size %ld", btype, size);
        EXFAIL_OUT(ret);
    }
    
    /* copy off the data */
    memcpy(thread_data->buffer, p_svc->data, size);
#endif
    thread_data->buffer=p_svc->data;
    thread_data->cd = p_svc->cd;
    thread_data->context_data = tpsrvgetctxdata();
    
    /* submit the job to thread pool: */
    thpool_add_work(G_tmsrv_cfg.thpool, (void*)TPTMSRV_TH, (void *)thread_data);
    
out:
    if (EXSUCCEED==ret)
    {
        /* serve next.. 
         * At this point we should know that at least one thread is free
         */
        pthread_mutex_lock(&M_wait_th_mutex);
        
        /* submit the job to verify free thread */
        
        thpool_add_work(G_tmsrv_cfg.thpool, (void*)tm_chk_one_free_thread, NULL);
        pthread_cond_wait(&M_wait_th_cond, &M_wait_th_mutex);
        pthread_mutex_unlock(&M_wait_th_mutex);
        
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

/*
 * Do initialization
 */
int NDRX_INTEGRA(tpsvrinit)(int argc, char **argv)
{
    int ret=EXSUCCEED;
    signed char c;
    char svcnm[MAXTIDENT+1];
    NDRX_LOG(log_debug, "tpsvrinit called");
    int nodeid;
    
    memset(&G_tmsrv_cfg, 0, sizeof(G_tmsrv_cfg));
    
    G_tmsrv_cfg.ping_mode_jointran = EXTRUE;
    
    /* Parse command line  */
    while ((c = getopt(argc, argv, "P:t:s:l:c:m:p:r:R")) != -1)
    {
        NDRX_LOG(log_debug, "%c = [%s]", c, optarg);
        switch(c)
        {
            case 't': 
                G_tmsrv_cfg.dflt_timeout = atol(optarg);
                NDRX_LOG(log_debug, "Default transaction time-out "
                            "set to: [%ld]", G_tmsrv_cfg.dflt_timeout);
                break;
                /* status directory: */
            case 'l': 
                NDRX_STRCPY_SAFE(G_tmsrv_cfg.tlog_dir, optarg);
                NDRX_LOG(log_debug, "Status directory "
                            "set to: [%s]", G_tmsrv_cfg.tlog_dir);
                break;
            case 's': 
                G_tmsrv_cfg.scan_time = atoi(optarg);
                break;
            case 'c': 
                /* Time for time-out checking... */
                G_tmsrv_cfg.tout_check_time = atoi(optarg);
                break;
            case 'm': 
                G_tmsrv_cfg.max_tries = atol(optarg);
                break;
            case 'p': 
                G_tmsrv_cfg.threadpoolsize = atol(optarg);
                break;
            case 'r': 
                G_tmsrv_cfg.xa_retries = atoi(optarg);
                break;
            case 'R':
                /* in this case use tran listing (xa_recover)*/
                G_tmsrv_cfg.ping_mode_jointran = EXFALSE;
                break;
            case 'P':
                /* Ping will run with timeout timer interval...
                 * will work with RECON flags (which must be set for this case)
                 */
                G_tmsrv_cfg.ping_time = atoi(optarg);

                if (G_tmsrv_cfg.ping_time < 0)
                {
                    NDRX_LOG(log_error, "ERROR ! invalid value %d for -P. Must be >=0",
                            G_tmsrv_cfg.ping_time);
                    EXFAIL_OUT(ret);
                }
                
                break;
            default:
                /*return FAIL;*/
                break;
        }
    }
    
    /* Check the parameters & default them if needed */
    if (0>=G_tmsrv_cfg.scan_time)
    {
        G_tmsrv_cfg.scan_time = SCAN_TIME_DFLT;
    }
    
    if (0>=G_tmsrv_cfg.max_tries)
    {
        G_tmsrv_cfg.max_tries = MAX_TRIES_DFTL;
    }
    
    if (0>=G_tmsrv_cfg.tout_check_time)
    {
        G_tmsrv_cfg.tout_check_time = TOUT_CHECK_TIME;
    }
    
    if (0>=G_tmsrv_cfg.threadpoolsize)
    {
        G_tmsrv_cfg.threadpoolsize = THREADPOOL_DFLT;
    }
    
    if (0>=G_tmsrv_cfg.xa_retries)
    {
        G_tmsrv_cfg.xa_retries = XA_RETRIES_DFLT;
    }
    
    if (EXEOS==G_tmsrv_cfg.tlog_dir[0])
    {
        userlog("TMS log dir not set!");
        NDRX_LOG(log_error, "TMS log dir not set!");
        EXFAIL_OUT(ret);
    }
    NDRX_LOG(log_debug, "Recovery scan time set to [%d]",
                            G_tmsrv_cfg.scan_time);
    
    NDRX_LOG(log_debug, "Tx max tries set to [%d]",
                            G_tmsrv_cfg.max_tries);
    
    NDRX_LOG(log_debug, "Worker pool size [%d] threads",
                            G_tmsrv_cfg.threadpoolsize);
    
    NDRX_LOG(log_debug, "Foreground retries in stage [%d]",
                            G_tmsrv_cfg.xa_retries);
    
    NDRX_LOG(log_debug, "About to initialize XA!");
    
    if (EXSUCCEED!=atmi_xa_init()) /* will open next... */
    {
        NDRX_LOG(log_error, "Failed to initialize XA driver!");
        EXFAIL_OUT(ret);
    }
    
    if (G_tmsrv_cfg.ping_time > 0)
    {

        NDRX_LOG(log_info, "DB PING & connection recovery enabled, interval: %d "
                "(same as -c number)", 
                G_tmsrv_cfg.tout_check_time);
        
        if (G_tmsrv_cfg.ping_mode_jointran)
        {
            NDRX_LOG(log_warn, "PING by JOIN to non existent transaction");
        }
        else
        {
            NDRX_LOG(log_warn, "PING by RECOVER transaction");
        }
        
        if (G_atmi_env.xa_recon_times <=1)
        {
            NDRX_LOG(log_always, "ERROR ! Using -P (ping) to be effective, please "
                    "ensure that NDRX_XA_FLAGS=RECON... is set to tries count > 1!");
            EXFAIL_OUT(ret);
        }
    }
    else
    {
        NDRX_LOG(log_info, "DB PING disabled (-P not set)");
    }
    
    /* we should open the XA  */
    
    NDRX_LOG(log_debug, "About to Open XA Entry!");
    ret = atmi_xa_open_entry();
    if( XA_OK != ret )
    {
        userlog("xa_open failed error %d", ret);
        NDRX_LOG(log_error, "xa_open failed");
    }
    else
    {
        NDRX_LOG(log_error, "xa_open ok");
        ret = EXSUCCEED;
    }
                
    /* All OK, about to advertise services */
    nodeid = tpgetnodeid();
    if (nodeid<1)
    {
        NDRX_LOG(log_error, "Failed to get current node_id");
        EXFAIL_OUT(ret);
    }
    
    /* very generic version/only Resource ID known */
    
    snprintf(svcnm, sizeof(svcnm), NDRX_SVC_RM, G_atmi_env.xa_rmid);
    
    if (EXSUCCEED!=tpadvertise(svcnm, TPTMSRV))
    {
        NDRX_LOG(log_error, "Failed to advertise %s service!", svcnm);
        EXFAIL_OUT(ret);
    }
    
    /* generic instance: */
    snprintf(svcnm, sizeof(svcnm), NDRX_SVC_TM, nodeid, G_atmi_env.xa_rmid);
    
    if (EXSUCCEED!=tpadvertise(svcnm, TPTMSRV))
    {
        NDRX_LOG(log_error, "Failed to advertise %s service!", svcnm);
        EXFAIL_OUT(ret);
    }
    
    /* specific instance */
    snprintf(svcnm, sizeof(svcnm), NDRX_SVC_TM_I, nodeid, G_atmi_env.xa_rmid, 
            G_server_conf.srv_id);
    
    if (EXSUCCEED!=tpadvertise(svcnm, TPTMSRV))
    {
        NDRX_LOG(log_error, "Failed to advertise %s service!", svcnm);
        EXFAIL_OUT(ret);
    }
    
    if (NULL==(G_tmsrv_cfg.thpool = thpool_init(G_tmsrv_cfg.threadpoolsize)))
    {
        NDRX_LOG(log_error, "Failed to initialize thread pool (cnt: %d)!", 
                G_tmsrv_cfg.threadpoolsize);
        EXFAIL_OUT(ret);
    }
    
    /* Start the background processing */
    background_process_init();
    
    
    /* Register timer check (needed for time-out detection) */
    if (EXSUCCEED!=tpext_addperiodcb(G_tmsrv_cfg.tout_check_time, tm_tout_check))
    {
            ret=EXFAIL;
            NDRX_LOG(log_error, "tpext_addperiodcb failed: %s",
                            tpstrerror(tperrno));
    }
    
    M_init_ok = EXTRUE;
    
out:
    return ret;
}

/**
 * Do de-initialization
 */
void NDRX_INTEGRA(tpsvrdone)(void)
{
    int i;
    NDRX_LOG(log_debug, "tpsvrdone called - requesting "
            "background thread shutdown...");
    
    G_bacground_req_shutdown = EXTRUE;
    
    if (M_init_ok)
    {
        background_wakeup();

        /* Terminate the threads */
        for (i=0; i<G_tmsrv_cfg.threadpoolsize; i++)
        {
            thpool_add_work(G_tmsrv_cfg.thpool, (void *)tm_thread_shutdown, NULL);
        }
        
        /* Wait to complete */
        pthread_join(G_bacground_thread, NULL);

        /* Wait for threads to finish */
        thpool_wait(G_tmsrv_cfg.thpool);
        thpool_destroy(G_tmsrv_cfg.thpool);
    }
    atmi_xa_close_entry();
    
}

/**
 * Periodic main thread callback for 
 * (will be done by threadpoll)
 * @return 
 */
exprivate void tx_tout_check_th(void *ptr)
{
    long tspent;
    atmi_xa_log_list_t *tx_list;
    atmi_xa_log_list_t *el, *tmp;
    atmi_xa_tx_info_t xai;
    atmi_xa_log_t *p_tl;
    
    /* Create a copy of hash, iterate and check each tx for timeout condition
     * If so then initiate internal abort call
     */
    NDRX_LOG(log_dump, "Timeout check (processing...)");
    tx_list = tms_copy_hash2list(COPY_MODE_FOREGROUND | COPY_MODE_ACQLOCK);
        
    LL_FOREACH_SAFE(tx_list,el,tmp)
    {
        NDRX_LOG(log_debug, "Checking [%s]...", el->p_tl.tmxid);
        if ((tspent = ndrx_stopwatch_get_delta_sec(&el->p_tl.ttimer)) > 
                el->p_tl.txtout && XA_TX_STAGE_ACTIVE==el->p_tl.txstage)
        {
            NDRX_LOG(log_error, "XID [%s] timed out "
                    "(spent %ld, limit: %ld sec) - aborting...!", 
                    el->p_tl.tmxid, tspent, 
                    el->p_tl.txtout);
            
            userlog("XID [%s] timed out "
                    "(spent %ld, limit: %ld sec) - aborting...!", 
                    el->p_tl.tmxid, tspent, 
                    el->p_tl.txtout);
            
            if (NULL!=(p_tl = tms_log_get_entry(el->p_tl.tmxid)))
            {
                XA_TX_COPY((&xai), p_tl);

                tms_log_stage(p_tl, XA_TX_STAGE_ABORTING);
                /* NOTE: We might want to move this to background processing
                 * because for example, oracle in some cases does long aborts...
                 * thus it slows down general processing
                 * BUT: if we want to move it to background, we should protect
                 * transaction log from concurrent access, e.g.
                 * - background does the abort()
                 * - meanwhile foreground calls commit()
                 * This can be reached with per transaction locking...
                 */
                tm_drive(&xai, p_tl, XA_OP_ROLLBACK, EXFAIL);
            }
        }
        LL_DELETE(tx_list,el);
        NDRX_FREE(el);
    }
out:    
    return;
}

/**
 * Run the DB ping...
 * It will try to list the transactions from DB. Non invasive method.
 * The recover entry will automatically reconnect to DB if connection failed
 * We will try to join non existent transaction...
 */
expublic void tm_ping_db(void *ptr, int *p_finish_off)
{
    int delta = ndrx_stopwatch_get_delta_sec(&M_ping_stopwatch);
    int ret;
    unsigned long tid = (unsigned long)ndrx_gettid();
    int is_ping_ok;
    
    /* Do the ATMI init, if needed */
    if (M_thread_first)
    {
        tm_thread_init();
        M_thread_first = EXFALSE;
    }
    
    if (delta >= G_tmsrv_cfg.ping_time)
    {
        ndrx_stopwatch_reset(&M_ping_stopwatch);
        NDRX_LOG(log_debug, "RMID: %hd TID: %lu: Running ping", 
                G_atmi_env.xa_rmid, tid);
        
        if (G_tmsrv_cfg.ping_mode_jointran)
        {
            if (EXSUCCEED==(ret = atmi_xa_start_entry(&M_ping_xid, TMJOIN, EXTRUE)) || 
                atmi_xa_get_reason()!=XAER_NOTA)
            {
                is_ping_ok = EXFALSE;
            }
            else
            {
                is_ping_ok = EXTRUE;
            }
        }
        else
        {
            if (0>(ret = atmi_xa_recover_entry(&M_ping_xid, 1, G_atmi_env.xa_rmid, 
                TMSTARTRSCAN|TMENDRSCAN)))
            {
                is_ping_ok = EXFALSE;
            }
            else
            {
                is_ping_ok = EXTRUE;
            }
        }
        
        if (!is_ping_ok)
        {
            /* Ping error/ulog */
            NDRX_LOG(log_error, "RMID: %hd TID: %lu ERROR ! DB PING FAILED: %s", 
                    G_atmi_env.xa_rmid, tid, tpstrerror(tperrno));
            userlog("RMID: %hd TID: %lu ERROR ! DB PING FAILED: %s", 
                    G_atmi_env.xa_rmid, tid, tpstrerror(tperrno));
        }
        else
        {
            /* for tests needs higher debug level to reduce space */
            NDRX_LOG(G_atmi_env.testmode?log_error:log_debug,
		"RMID %hd TID: %lu: PING OK %d", 
                G_atmi_env.xa_rmid, tid, ret);
        }
    }

}

/**
 * Callback routine for scheduled timeout checks.
 * @return 
 */
exprivate int tm_tout_check(void)
{
    int i;
    NDRX_LOG(log_dump, "Timeout check (submit job...)");
    
    thpool_add_work(G_tmsrv_cfg.thpool, (void*)tx_tout_check_th, NULL);
    
    /* RUN PINGs... over the all threads... */
    if (G_tmsrv_cfg.ping_time > 0)
    {
        for (i=0; i<G_tmsrv_cfg.threadpoolsize; i++)
        {
            thpool_add_work(G_tmsrv_cfg.thpool, (void*)tm_ping_db, NULL);
        }
    }
    
    return EXSUCCEED;
}

/**
 * Just run down one task via pool, to ensure that at least one
 * thread is free, before we are going to mail poll.
 * @param ptr
 */
exprivate void tm_chk_one_free_thread(void *ptr, int *p_finish_off)
{
    pthread_mutex_lock(&M_wait_th_mutex);
    pthread_cond_signal(&M_wait_th_cond);
    pthread_mutex_unlock(&M_wait_th_mutex);
}

/**
 * Shutdown the thread
 * @param arg
 * @param p_finish_off
 */
expublic void tm_thread_shutdown(void *ptr, int *p_finish_off)
{
    tm_thread_uninit();
    
    *p_finish_off = EXTRUE;
}

/* vim: set ts=4 sw=4 et cindent: */
