/**
 * @brief EnduroX Queue Server
 *
 * @file tmqueue.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2019, Mavimax, Ltd. All Rights Reserved.
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
#include <ndrx_config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>    /* for getopt */
#include <errno.h>
#include <regex.h>
#include <utlist.h>

#include <ndebug.h>
#include <atmi.h>
#include <atmi_int.h>
#include <typed_buf.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <Exfields.h>

#include <exnet.h>
#include <ndrxdcmn.h>

#include "tmqd.h"
#include "tperror.h"
#include "userlog.h"
#include <xa_cmn.h>
#include <exthpool.h>
#include "qcommon.h"
#include "cconfig.h"
#include <ubfutil.h>
#include <thlock.h>
#include "qtran.h"
#include "../libatmisrv/srv_int.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
expublic tmqueue_cfg_t G_tmqueue_cfg;
/*---------------------------Statics------------------------------------*/
exprivate int M_init_ok = EXFALSE;
exprivate __thread int M_thread_first = EXTRUE;

/* allow only one timeout check at the same time... */
exprivate int volatile M_into_toutchk = EXFALSE;
exprivate MUTEX_LOCKDECL(M_into_toutchk_lock);

/** mark that thrad pool is done with shutdown seq */
exprivate int M_shutdown_ok = EXFALSE;

/** global shutdown indicator */
exprivate int *M_shutdown_ind = NULL;

/** mark globally that timeout processing is in progress
 * do not collide with any worker already running...
 */
/*---------------------------Prototypes---------------------------------*/
exprivate int tx_tout_check(void);
exprivate void tm_chk_one_free_thread(void *ptr, int *p_finish_off);
exprivate void tm_chk_one_free_thread_notif(void *ptr, int *p_finish_off);

/**
 * Initialize thread
 */
expublic void tmq_thread_init(void)
{
    if (EXSUCCEED!=tpinit(NULL))
    {
        NDRX_LOG(log_error, "Failed to init worker client");
        userlog("tmsrv: Failed to init worker client");
        exit(1);
    }
    
    if (EXSUCCEED!=tpopen())
    {
        NDRX_LOG(log_error, "Worker thread failed to tpopen() - nothing to do, "
                "process will exit");
        userlog("Worker thread failed to tpopen() - nothing to do, "
                "process will exit");
        exit(1);
    }
    
}

/**
 * Close the thread session
 */
expublic void tmq_thread_uninit(void)
{
    NDRX_LOG(log_debug, "Into tmq_thread_uninit");
    tpclose();
    tpterm();
}

/**
 * Tmqueue service entry (working thread)
 * @param p_svc - data & len used only...!
 */
void TMQUEUE_TH (void *ptr, int *p_finish_off)
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
    
    /* Do the ATMI init */
    if (M_thread_first)
    {
        tmq_thread_init();
        M_thread_first = EXFALSE;
    }
    
    /* restore context. */
    if (EXSUCCEED!=tpsrvsetctxdata(thread_data->context_data, SYS_SRV_THREAD))
    {
        userlog("tmqueue: Failed to set context");
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
    
    ndrx_debug_dump_UBF(log_info, "TMQUEUE call buffer:", p_ub);
    
    if (Bget(p_ub, EX_QCMD, 0, (char *)&cmd, 0L))
    {
        NDRX_LOG(log_error, "Failed to read command code!");
        ret=EXFAIL;
        goto out;
    }
    NDRX_LOG(log_info, "Got command code: [%c]", cmd);
    
    switch(cmd)
    {
        case TMQ_CMD_ENQUEUE:
            
            /* start new tran... */
            if (EXSUCCEED!=tmq_enqueue(p_ub))
            {
                EXFAIL_OUT(ret);
            }
            break;
        case TMQ_CMD_DEQUEUE:
            
            /* start new tran... */
            if (EXSUCCEED!=tmq_dequeue(&p_ub))
            {
                EXFAIL_OUT(ret);
            }
            break;
        case TMQ_CMD_MQLQ:
            
            if (EXSUCCEED!=tmq_mqlq(p_ub, cd))
            {
                EXFAIL_OUT(ret);
            }
            break;
        case TMQ_CMD_MQLC:
            
            if (EXSUCCEED!=tmq_mqlc(p_ub, cd))
            {
                EXFAIL_OUT(ret);
            }
            break;
        case TMQ_CMD_MQLM:
            
            if (EXSUCCEED!=tmq_mqlm(p_ub, cd))
            {
                EXFAIL_OUT(ret);
            }
            break;
        case TMQ_CMD_MQRC:
            
            if (EXSUCCEED!=tmq_mqrc(p_ub))
            {
                EXFAIL_OUT(ret);
            }
            
            break;
        case TMQ_CMD_MQCH:
            
            if (EXSUCCEED!=tmq_mqch(p_ub))
            {
                EXFAIL_OUT(ret);
            }
            break;
        case TMQ_CMD_STARTTRAN:
        case TMQ_CMD_ABORTTRAN:
        case TMQ_CMD_PREPARETRAN:
        case TMQ_CMD_COMMITRAN:
            
            /* start Q space transaction */
            if (XA_OK!=ndrx_xa_qminiservce(p_ub, cmd))
            {
                EXFAIL_OUT(ret);
            }
            
            break;
            
        default:
            NDRX_LOG(log_error, "Unsupported command code: [%c]", cmd);
            ret=EXFAIL;
            break;
    }
    
out:

    ndrx_debug_dump_UBF(log_info, "TMQUEUE return buffer:", p_ub);

    tpreturn(  ret==EXSUCCEED?TPSUCCESS:TPFAIL,
                0L,
                (char *)p_ub,
                0L,
                0L);
}


/**
 * Periodic main thread callback for 
 * (will be done by threadpoll)
 * @return 
 */
exprivate void tx_tout_check_th(void *ptr)
{
    long tspent;
    qtran_log_list_t *tx_list;
    qtran_log_list_t *el, *tmp;
    qtran_log_t *p_tl;
    int in_progress;
    int locke;
    XID xid;
    /* Create a copy of hash, iterate and check each tx for timeout condition
     * If so then initiate internal abort call
     */
    
    MUTEX_LOCK_V(M_into_toutchk_lock);
    
    in_progress=M_into_toutchk;
    
    /* do lock if was free */
    if (!in_progress)
    {
        M_into_toutchk=EXTRUE;
    }
            
    MUTEX_UNLOCK_V(M_into_toutchk_lock);
    
    if (in_progress)
    {
        /* nothing todo... */
        goto out;
    }
            
    NDRX_LOG(log_dump, "Timeout check (processing...)");
    
    /* Do the ATMI init, if needed 
     */
    if (M_thread_first)
    {
        tmq_thread_init();
        M_thread_first = EXFALSE;
    }
    
    tx_list = tmq_copy_hash2list(COPY_MODE_FOREGROUND | COPY_MODE_ACQLOCK);
        
    LL_FOREACH_SAFE(tx_list,el,tmp)
    {
        NDRX_LOG(log_debug, "Checking [%s]...", el->p_tl.tmxid);
        if ((tspent = ndrx_stopwatch_get_delta_sec(&el->p_tl.ttimer)) > 
                G_tmqueue_cfg.dflt_timeout && XA_TX_STAGE_ACTIVE==el->p_tl.txstage)
        {
            
            /* get the finally the entry and process... */
            if (NULL!=(p_tl = tmq_log_get_entry(el->p_tl.tmxid, 0, &locke)))
            {
                if (XA_TX_STAGE_ACTIVE==p_tl->txstage)
                {
                    
                    NDRX_LOG(log_error, "TMXID Q [%s] timed out "
                        "(spent %ld, limit: %ld sec) - aborting...!", 
                        el->p_tl.tmxid, tspent, 
                        G_tmqueue_cfg.dflt_timeout);
            
                    userlog("TMXID Q [%s] timed out "
                            "(spent %ld, limit: %ld sec) - aborting...!", 
                            el->p_tl.tmxid, tspent, 
                            G_tmqueue_cfg.dflt_timeout);
                    
                    /* do abort...! */
                    
                    el->p_tl.is_abort_only=EXTRUE;
                    
                    if (NULL==atmi_xa_deserialize_xid(el->p_tl.tmxid, &xid))
                    {
                        NDRX_LOG(log_error, "Failed to deserialize tmxid [%s]", 
                                el->p_tl.tmxid);
                        tmq_log_unlock(p_tl);
                        goto next;
                    }
                    
                    /* try to rollback the stuff...! */
                    if (EXSUCCEED!=atmi_xa_rollback_entry(&xid, 0))
                    {
                        NDRX_LOG(log_error, "Failed to abort tmxid:[%s]", 
                                el->p_tl.tmxid);
                        tmq_log_unlock(p_tl);
                        goto next;
                    }
                    
                    /* Transaction must be removed at this point */
                    
                }
                else
                {
                    NDRX_LOG(log_error, "Q TMXID [%s] was-tout but found in progress "
                        "(txstage %hd spent %ld, limit: %ld sec) - aborting...!", 
                        el->p_tl.tmxid, el->p_tl.txstage, tspent, G_tmqueue_cfg.dflt_timeout);
                }
            }
        }
next:
        LL_DELETE(tx_list,el);
        NDRX_FPFREE(el);
        
    }
    
    
out:    

    /* if was not in progress then we locked  */
    MUTEX_LOCK_V(M_into_toutchk_lock);

    if (!in_progress)
    {
        M_into_toutchk=EXFALSE;
    }   

    MUTEX_UNLOCK_V(M_into_toutchk_lock);
    
    return;
}

/**
 * Callback routine for scheduled timeout checks.
 * TODO: if we add shutdown handlers, then check here is all completed
 * before we inject back the shutdown msg...
 * @return 
 */
exprivate int tm_tout_check(void)
{
    NDRX_LOG(log_dump, "Timeout check (submit job...)");
    
    
    if (NULL==M_shutdown_ind)
    {
        /* no shutdown requested... yet... */
        ndrx_thpool_add_work(G_tmqueue_cfg.notifthpool, (void*)tx_tout_check_th, NULL);
    }
    else if (M_shutdown_ok)
    {
        ndrx_sv_do_shutdown("Application shutdown sequence", M_shutdown_ind);
    }
    
    
    return EXSUCCEED;
}


/**
 * Entry point for service (main thread)
 * @param p_svc
 */
void TMQUEUE (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;
    UBFH *p_ub = (UBFH *)p_svc->data; /* this is auto-buffer */
    long size;
    char btype[16];
    char stype[16];
    thread_server_t *thread_data = NDRX_MALLOC(sizeof(thread_server_t));
    char cmd = EXEOS;
    
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
    
    /* not using sub-type - on tpreturn/forward for thread it will be auto-free */
#if 0
        - Why? mvitolin 25/01/2017
    thread_data->buffer =  tpalloc(btype, NULL, size);
    
    if (NULL==thread_data->buffer)
    {
        NDRX_LOG(log_error, "tpalloc failed of type %s size %ld", btype, size);
        EXFAIL_OUT(ret);
    }
    
    /* copy off the data */
    memcpy(thread_data->buffer, p_svc->data, size);
#endif

    thread_data->buffer = p_svc->data; /*the buffer is not made free by thread */
    thread_data->cd = p_svc->cd;
    
    if (NULL==(thread_data->context_data = tpsrvgetctxdata()))
    {
        NDRX_LOG(log_error, "Failed to get context data!");
        EXFAIL_OUT(ret);
    }
    
    if (Bget(p_ub, EX_QCMD, 0, (char *)&cmd, 0L))
    {
        NDRX_LOG(log_error, "Failed to read command code!");
        ret=EXFAIL;
        goto out;
    }
    
    /* submit the job to thread pool: 
     * For transaction finalization use different thread pool
     */
    if (cmd==TMQ_CMD_STARTTRAN||
            cmd==TMQ_CMD_PREPARETRAN||
            cmd==TMQ_CMD_ABORTTRAN||
            cmd==TMQ_CMD_COMMITRAN)
    {
        ndrx_thpool_add_work(G_tmqueue_cfg.notifthpool, (void*)TMQUEUE_TH, (void *)thread_data);
    }
    else
    {
        ndrx_thpool_add_work(G_tmqueue_cfg.thpool, (void*)TMQUEUE_TH, (void *)thread_data);
    }
    
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
 * Just justdown the forwarder and forward threads
 * the main/notif threads will be terminated at the end.
 * @param ptr data, not used
 */
exprivate void shutdowncb_th(void *ptr)
{
    int i;
    
    NDRX_LOG(log_info, "Shutdown sequence started...");
    G_forward_req_shutdown = EXTRUE;
    
    if (M_init_ok)
    {
        forward_shutdown_wake();
        
        /* Wait to complete */
        pthread_join(G_forward_thread, NULL);
        
        for (i=0; i<G_tmqueue_cfg.fwdpoolsize; i++)
        {
            ndrx_thpool_add_work(G_tmqueue_cfg.fwdthpool, (void *)tmq_thread_shutdown, NULL);
        }
        
        ndrx_thpool_wait(G_tmqueue_cfg.fwdthpool);
        ndrx_thpool_destroy(G_tmqueue_cfg.fwdthpool);
    }
    
    M_shutdown_ok=EXTRUE;
}

/**
 * Shutdown sequencer
 * So that we terminate all processing string in the right order
 * @param shutdown_req ptr to indicator
 * @return SUCCEED
 */
exprivate int shutdowncb(int *shutdown_req)
{
    /* submit shutdown job */
    M_shutdown_ind = shutdown_req;
    ndrx_thpool_add_work(G_tmqueue_cfg.shutdownseq, (void*)shutdowncb_th, NULL);
    return EXSUCCEED;
}

/*
 * Do initialization
 */
int tpsvrinit(int argc, char **argv)
{
    int ret=EXSUCCEED;
    signed char c;
    char svcnm[MAXTIDENT+1];
    NDRX_LOG(log_debug, "tpsvrinit called");
    
    memset(&G_tmqueue_cfg, 0, sizeof(G_tmqueue_cfg));
    
    /* Parse command line  */
    while ((c = getopt(argc, argv, "q:m:s:p:t:f:u:c:")) != -1)
    {
        if (optarg)
        {
            NDRX_LOG(log_debug, "%c = [%s]", c, optarg);
        }
        else
        {
            NDRX_LOG(log_debug, "got %c", c);
        }

        switch(c)
        {
            case 'm': 
                
#if 0
                /* My qspace.. */ 
                NDRX_STRCPY_SAFE(G_tmqueue_cfg.qspace, optarg);
                snprintf(G_tmqueue_cfg.qspacesvc, sizeof(G_tmqueue_cfg.qspacesvc),
                        NDRX_SVC_QSPACE, optarg);
                NDRX_LOG(log_debug, "Qspace set to: [%s]", G_tmqueue_cfg.qspace);
                NDRX_LOG(log_debug, "Qspace svc set to: [%s]", G_tmqueue_cfg.qspacesvc);          
#endif
                /* Ask to convert: */
                NDRX_LOG(log_error, "ERROR ! Please convert queue settings to NDRX_XA_OPEN_STR (dir=,qspace=)");
                EXFAIL_OUT(ret);
                
                break;
                
            case 'q':
                /* Add the queue */
                NDRX_STRCPY_SAFE(G_tmqueue_cfg.qconfig, optarg);
                NDRX_LOG(log_error, "Loading q config: [%s]", G_tmqueue_cfg.qconfig);
                if (EXSUCCEED!=tmq_reload_conf(G_tmqueue_cfg.qconfig))
                {
                    NDRX_LOG(log_error, "Failed to read config for: [%s]", G_tmqueue_cfg.qconfig);
                    EXFAIL_OUT(ret);
                }
                break;
            case 's': 
                G_tmqueue_cfg.scan_time = atoi(optarg);
                break;
            case 'p': 
                G_tmqueue_cfg.threadpoolsize = atol(optarg);
                break;
            case 'u': 
                G_tmqueue_cfg.notifpoolsize = atol(optarg);
                break;
            case 'f': 
                G_tmqueue_cfg.fwdpoolsize = atol(optarg);
                break;
            case 't': 
                G_tmqueue_cfg.dflt_timeout = atol(optarg);
                break;
            case 'c': 
                /* Time for time-out checking... */
                G_tmqueue_cfg.tout_check_time = atoi(optarg);
                break;
            default:
                /*return FAIL;*/
                break;
        }
    }
    
    if (ndrx_get_G_cconfig())
    {
        if (EXSUCCEED!=tmq_reload_conf(NULL))
        {
            NDRX_LOG(log_error, "Failed to read CCONFIG's @queue section!");
            EXFAIL_OUT(ret);
        }
    }
    
    /* Check the parameters & default them if needed */
    if (0>=G_tmqueue_cfg.scan_time)
    {
        G_tmqueue_cfg.scan_time = SCAN_TIME_DFLT;
    }
    
    if (0>=G_tmqueue_cfg.threadpoolsize)
    {
        G_tmqueue_cfg.threadpoolsize = THREADPOOL_DFLT;
    }
    
    if (0>=G_tmqueue_cfg.notifpoolsize)
    {
        G_tmqueue_cfg.notifpoolsize = THREADPOOL_DFLT;
    }
    
    if (0>=G_tmqueue_cfg.fwdpoolsize)
    {
        G_tmqueue_cfg.fwdpoolsize = THREADPOOL_DFLT;
    }
    
    if (0>=G_tmqueue_cfg.dflt_timeout)
    {
        G_tmqueue_cfg.dflt_timeout = TXTOUT_DFLT;
    }
    
    if (0>=G_tmqueue_cfg.tout_check_time)
    {
        G_tmqueue_cfg.tout_check_time = TOUT_CHECK_TIME;
    }
    
    NDRX_LOG(log_info, "Recovery scan time set to [%d]",
                            G_tmqueue_cfg.scan_time);
    
    NDRX_LOG(log_info, "Worker pool size [%d] threads",
                            G_tmqueue_cfg.threadpoolsize);
    
    NDRX_LOG(log_info, "Worker  notify-loop-back pool size [%d] threads",
                            G_tmqueue_cfg.notifpoolsize);
    
    NDRX_LOG(log_info, "Forward pool size [%d] threads",
                            G_tmqueue_cfg.fwdpoolsize);
    
    NDRX_LOG(log_info, "Local transaction tout set to: [%ld]", 
            G_tmqueue_cfg.dflt_timeout );
    
    NDRX_LOG(log_info, "Periodic timeout-check time: [%d]", 
            G_tmqueue_cfg.tout_check_time);
    
    /* we should open the XA  */
    NDRX_LOG(log_debug, "About to Open XA Entry!");
    
    if (EXSUCCEED!=tpopen())
    {
        EXFAIL_OUT(ret);
    }
    
    /* we shall read the Q space now... */
    
    /* Recover the messages from disk */
    if (EXSUCCEED!=tmq_load_msgs())
    {
        EXFAIL_OUT(ret);
    }
    
    /* abort all active transactions! */
    if (EXSUCCEED!=tmq_log_abortall())
    {
        EXFAIL_OUT(ret);
    }
    
    /*
     * So QSPACE is Service name.
     * Each tmqsrv will advertize:
     * - <QSPACE> - For Auto queues, you can start multiple executables
     *            - For manual queues (doing tpdequeue()) - only one space is possible
     *            - processes does the queue mirroring in memory.
     * - <QSPACE>-<NODE_ID>-<SRVID> - To this XA driver will send ACKs.
     * 
     * Also.. when we will recover from disk we will have to ensure the correct order
     * of the enqueued messages. We can use time-stamp for doing ordering.
     */
    snprintf(svcnm, sizeof(svcnm), NDRX_SVC_TMQ, tpgetnodeid(), tpgetsrvid());
    
    if (EXSUCCEED!=tpadvertise(svcnm, TMQUEUE))
    {
        NDRX_LOG(log_error, "Failed to advertise %s service!", svcnm);
        EXFAIL_OUT(ret);
    }

    if (EXSUCCEED!=tpadvertise(ndrx_G_qspacesvc, TMQUEUE))
    {
        NDRX_LOG(log_error, "Failed to advertise %s service!", svcnm);
        EXFAIL_OUT(ret);
    }
    
    /* service request handlers */
    if (NULL==(G_tmqueue_cfg.thpool = ndrx_thpool_init(G_tmqueue_cfg.threadpoolsize,
            NULL, NULL, NULL, 0, NULL)))
    {
        NDRX_LOG(log_error, "Failed to initialize thread pool (cnt: %d)!", 
                G_tmqueue_cfg.threadpoolsize);
        EXFAIL_OUT(ret);
    }
    
    if (NULL==(G_tmqueue_cfg.notifthpool = ndrx_thpool_init(G_tmqueue_cfg.notifpoolsize,
            NULL, NULL, NULL, 0, NULL)))
    {
        NDRX_LOG(log_error, "Failed to initialize udpate thread pool (cnt: %d)!", 
                G_tmqueue_cfg.notifpoolsize);
        EXFAIL_OUT(ret);
    }
    
    /* q forward handlers */
    if (NULL==(G_tmqueue_cfg.fwdthpool = ndrx_thpool_init(G_tmqueue_cfg.fwdpoolsize,
            NULL, NULL, NULL, 0, NULL)))
    {
        NDRX_LOG(log_error, "Failed to initialize fwd thread pool (cnt: %d)!", 
                G_tmqueue_cfg.fwdpoolsize);
        EXFAIL_OUT(ret);
    }
    
    if (NULL==(G_tmqueue_cfg.shutdownseq = ndrx_thpool_init(1,
            NULL, NULL, NULL, 0, NULL)))
    {
        NDRX_LOG(log_error, "Failed to initialize shutdown thread pool!");
        EXFAIL_OUT(ret);
    }
    
    /* Register timer check (needed for time-out detection) */
    if (EXSUCCEED!=tpext_addperiodcb(G_tmqueue_cfg.tout_check_time, tm_tout_check))
    {
        NDRX_LOG(log_error, "tpext_addperiodcb failed: %s",
                        tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    /* set shutdown callback handler
     * so that we can initiate shutdown sequence
     */
    if (EXSUCCEED!=ndrx_tpext_addbshutdowncb(shutdowncb))
    {
        NDRX_LOG(log_error, "Failed to add shutdown sequencer callback!");
        EXFAIL_OUT(ret);
    }
    
    /* Start the background processing */
    if (EXSUCCEED!=forward_process_init())
    {
        NDRX_LOG(log_error, "Failed to initialize fwd process thread");
        EXFAIL_OUT(ret);
    }

    /* Bug #565 */
    M_init_ok=EXTRUE;
    
out:
    return ret;
}

/**
 * Do de-initialization
 */
void tpsvrdone(void)
{
    int i;
    
    NDRX_LOG(log_debug, "tpsvrdone called - requesting "
            "background thread shutdown...");
    
    if (M_init_ok)
    {
        
        /* Terminate the threads (request) */
        for (i=0; i<G_tmqueue_cfg.threadpoolsize; i++)
        {
            ndrx_thpool_add_work(G_tmqueue_cfg.thpool, (void *)tmq_thread_shutdown, NULL);
        }
        
        /* update threads */
        for (i=0; i<G_tmqueue_cfg.notifpoolsize; i++)
        {
            ndrx_thpool_add_work(G_tmqueue_cfg.notifthpool, (void *)tmq_thread_shutdown, NULL);
        }
        
        /* terminate the showdown thread... */
        ndrx_thpool_add_work(G_tmqueue_cfg.shutdownseq, (void *)tmq_thread_shutdown, NULL);
        
        
        ndrx_thpool_wait(G_tmqueue_cfg.shutdownseq);
        ndrx_thpool_destroy(G_tmqueue_cfg.shutdownseq);
        
    }
    
    tpclose();
    
}

/**
 * Shutdown the thread
 * @param arg
 * @param p_finish_off
 */
expublic void tmq_thread_shutdown(void *ptr, int *p_finish_off)
{
    tmq_thread_uninit();
    
    *p_finish_off = EXTRUE;
}
/* vim: set ts=4 sw=4 et smartindent: */
