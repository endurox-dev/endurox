/* 
** EnduroX Queue Server
**
** @file tmqueue.c
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
#include <config.h>
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

#include "tmqd.h"
#include "../libatmisrv/srv_int.h"
#include "tperror.h"
#include "userlog.h"
#include <xa_cmn.h>
#include "thpool.h"
#include "qcommon.h"
/*---------------------------Externs------------------------------------*/
extern int optind, optopt, opterr;
extern char *optarg;
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
public tmqueue_cfg_t G_tmqueue_cfg;
/*---------------------------Statics------------------------------------*/
private int M_init_ok = FALSE;

/*---------------------------Prototypes---------------------------------*/
private int tx_tout_check(void);
private void tm_chk_one_free_thread(void *ptr, int *p_finish_off);
private void tm_chk_one_free_thread_notif(void *ptr, int *p_finish_off);

/**
 * Tmqueue service entry (working thread)
 * @param p_svc - data & len used only...!
 */
void TMQUEUE_TH (void *ptr, int *p_finish_off)
{
    /* Ok we should not handle the commands 
     * TPBEGIN...
     */
    int ret=SUCCEED;
    static __thread int first = TRUE;
    thread_server_t *thread_data = (thread_server_t *)ptr;
    char cmd = EOS;
    int cd;
    
    /**************************************************************************/
    /*                        THREAD CONTEXT RESTORE                          */
    /**************************************************************************/
    UBFH *p_ub = (UBFH *)thread_data->buffer;
    
    /* Do the ATMI init */
    if (first)
    {
        first = FALSE;
        if (SUCCEED!=tpinit(NULL))
        {
            NDRX_LOG(log_error, "Failed to init worker client");
            userlog("tmqueue: Failed to init worker client");
            exit(1);
        }
    }
    
    /* restore context. */
    if (SUCCEED!=tpsrvsetctxdata(thread_data->context_data, SYS_SRV_THREAD))
    {
        userlog("tmqueue: Failed to set context");
        NDRX_LOG(log_error, "Failed to set context");
        exit(1);
    }
    
    cd = thread_data->cd;
    /* free up the transport data.*/
    free(thread_data->context_data);
    free(thread_data);
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
        ret=FAIL;
        goto out;
    }
    NDRX_LOG(log_info, "Got command code: [%c]", cmd);
    
    switch(cmd)
    {
        case TMQ_CMD_ENQUEUE:
            
            /* start new tran... */
            if (SUCCEED!=tmq_enqueue(p_ub))
            {
                FAIL_OUT(ret);
            }
            break;
        case TMQ_CMD_DEQUEUE:
            
            /* start new tran... */
            if (SUCCEED!=tmq_dequeue(&p_ub))
            {
                FAIL_OUT(ret);
            }
            break;
        case TMQ_CMD_NOTIFY:
            
            /* request for printing active transactions */
            if (SUCCEED!=tex_mq_notify(p_ub))
            {
                FAIL_OUT(ret);
            }
            break;
        case TMQ_CMD_MQLQ:
            
            /* request for printing active transactions */
            if (SUCCEED!=tmq_mqlq(p_ub, cd))
            {
                FAIL_OUT(ret);
            }
            break;
        case TMQ_CMD_MQLC:
            
            /* request for printing active transactions */
            if (SUCCEED!=tmq_mqlc(p_ub, cd))
            {
                FAIL_OUT(ret);
            }
            break;
        case TMQ_CMD_MQLM:
            
            /* request for printing active transactions */
            if (SUCCEED!=tmq_mqlm(p_ub, cd))
            {
                FAIL_OUT(ret);
            }
            break;
        case TMQ_CMD_MQRC:
            
            if (SUCCEED!=tmq_mqrc(p_ub))
            {
                FAIL_OUT(ret);
            }
            
            break;
        case TMQ_CMD_MQCH:
            
            if (SUCCEED!=tmq_mqch(p_ub))
            {
                FAIL_OUT(ret);
            }
            break;
        default:
            NDRX_LOG(log_error, "Unsupported command code: [%c]", cmd);
            ret=FAIL;
            break;
    }
    
out:

    ndrx_debug_dump_UBF(log_info, "TMQUEUE return buffer:", p_ub);

    tpreturn(  ret==SUCCEED?TPSUCCESS:TPFAIL,
                0L,
                (char *)p_ub,
                0L,
                0L);
}

/**
 * Entry point for service (main thread)
 * @param p_svc
 */
void TMQUEUE (TPSVCINFO *p_svc)
{
    int ret=SUCCEED;
    UBFH *p_ub = (UBFH *)p_svc->data; /* this is auto-buffer */
    long size;
    char btype[16];
    char stype[16];
    thread_server_t *thread_data = malloc(sizeof(thread_server_t));
    char cmd = EOS;
    
    if (NULL==thread_data)
    {
        userlog("Failed to malloc memory - %s!", strerror(errno));
        NDRX_LOG(log_error, "Failed to malloc memory");
        FAIL_OUT(ret);
    }
    
    if (0==(size = tptypes (p_svc->data, btype, stype)))
    {
        NDRX_LOG(log_error, "Zero buffer received!");
        userlog("Zero buffer received!");
        FAIL_OUT(ret);
    }
    
    /* not using sub-type - on tpreturn/forward for thread it will be auto-free */
    thread_data->buffer =  tpalloc(btype, NULL, size);
    
    if (NULL==thread_data->buffer)
    {
        NDRX_LOG(log_error, "tpalloc failed of type %s size %ld", btype, size);
        FAIL_OUT(ret);
    }
    
    /* copy off the data */
    memcpy(thread_data->buffer, p_svc->data, size);
    thread_data->cd = p_svc->cd;
    
    if (NULL==(thread_data->context_data = tpsrvgetctxdata()))
    {
        NDRX_LOG(log_error, "Failed to get context data!");
        FAIL_OUT(ret);
    }
    
    if (Bget(p_ub, EX_QCMD, 0, (char *)&cmd, 0L))
    {
        NDRX_LOG(log_error, "Failed to read command code!");
        ret=FAIL;
        goto out;
    }
    
    
    /* submit the job to thread pool: */
    if (cmd==TMQ_CMD_NOTIFY)
    {
        thpool_add_work(G_tmqueue_cfg.thpool, (void*)TMQUEUE_TH, (void *)thread_data);
    }
    else
    {
        thpool_add_work(G_tmqueue_cfg.notifthpool, (void*)TMQUEUE_TH, (void *)thread_data);
    }
    
out:
    if (SUCCEED==ret)
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

/*
 * Do initialization
 */
int NDRX_INTEGRA(tpsvrinit)(int argc, char **argv)
{
    int ret=SUCCEED;
    signed char c;
    char svcnm[MAXTIDENT+1];
    NDRX_LOG(log_debug, "tpsvrinit called");
    
    memset(&G_tmqueue_cfg, 0, sizeof(G_tmqueue_cfg));
    
    /* Parse command line  */
    while ((c = getopt(argc, argv, "q:m:s:p:t:")) != -1)
    {
        NDRX_LOG(log_debug, "%c = [%s]", c, optarg);
        switch(c)
        {
            case 'm': /* My qspace.. */ 
                strcpy(G_tmqueue_cfg.qspace, optarg);
                sprintf(G_tmqueue_cfg.qspacesvc, NDRX_SVC_QSPACE, optarg);
                NDRX_LOG(log_debug, "Qspace set to: [%s]", G_tmqueue_cfg.qspace);
                NDRX_LOG(log_debug, "Qspace svc set to: [%s]", G_tmqueue_cfg.qspacesvc);
                break;
                
            case 'q':
                /* Add the queue */
                strcpy(G_tmqueue_cfg.qconfig, optarg);
                NDRX_LOG(log_error, "Loading q config: [%s]", G_tmqueue_cfg.qconfig);
                if (SUCCEED!=tmq_reload_conf(G_tmqueue_cfg.qconfig))
                {
                    NDRX_LOG(log_error, "Failed to read config for: [%s]", G_tmqueue_cfg.qconfig);
                    FAIL_OUT(ret);
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
            default:
                /*return FAIL;*/
                break;
        }
    }
    
    if (EOS==G_tmqueue_cfg.qspace[0])
    {
        NDRX_LOG(log_error, "qspace not set (-m <qspace name> flag)");
        FAIL_OUT(ret);
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
    
    
    /* we should open the XA  */
    NDRX_LOG(log_debug, "About to Open XA Entry!");
    if (SUCCEED!=tpopen())
    {
        FAIL_OUT(ret);
    }
    
    /* Recover the messages from disk */
    if (SUCCEED!=tmq_load_msgs())
    {
        FAIL_OUT(ret);
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
    sprintf(svcnm, NDRX_SVC_TMQ, tpgetnodeid(), tpgetsrvid());
    
    if (SUCCEED!=tpadvertise(svcnm, TMQUEUE))
    {
        NDRX_LOG(log_error, "Failed to advertise %s service!", svcnm);
        FAIL_OUT(ret);
    }

    if (SUCCEED!=tpadvertise(G_tmqueue_cfg.qspacesvc, TMQUEUE))
    {
        NDRX_LOG(log_error, "Failed to advertise %s service!", svcnm);
        FAIL_OUT(ret);
    }
    
    /* service request handlers */
    if (NULL==(G_tmqueue_cfg.thpool = thpool_init(G_tmqueue_cfg.threadpoolsize)))
    {
        NDRX_LOG(log_error, "Failed to initialize thread pool (cnt: %d)!", 
                G_tmqueue_cfg.threadpoolsize);
        FAIL_OUT(ret);
    }
    
    if (NULL==(G_tmqueue_cfg.notifthpool = thpool_init(G_tmqueue_cfg.notifpoolsize)))
    {
        NDRX_LOG(log_error, "Failed to initialize udpate thread pool (cnt: %d)!", 
                G_tmqueue_cfg.notifpoolsize);
        FAIL_OUT(ret);
    }
    
    /* q forward handlers */
    if (NULL==(G_tmqueue_cfg.fwdthpool = thpool_init(G_tmqueue_cfg.fwdpoolsize)))
    {
        NDRX_LOG(log_error, "Failed to initialize fwd thread pool (cnt: %d)!", 
                G_tmqueue_cfg.fwdpoolsize);
        FAIL_OUT(ret);
    }
    
    /* Start the background processing */
    forward_process_init();
    
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
    
    G_forward_req_shutdown = TRUE;
    
    if (M_init_ok)
    {
        forward_shutdown_wake();

        /* Wait to complete */
        pthread_join(G_forward_thread, NULL);

        /* Terminate the threads (request) */
        for (i=0; i<G_tmqueue_cfg.threadpoolsize; i++)
        {
            thpool_add_work(G_tmqueue_cfg.thpool, (void *)tp_thread_shutdown, NULL);
        }
        
        /* update threads */
        for (i=0; i<G_tmqueue_cfg.notifpoolsize; i++)
        {
            thpool_add_work(G_tmqueue_cfg.notifthpool, (void *)tp_thread_shutdown, NULL);
        }
        
        /* forwarder */
        for (i=0; i<G_tmqueue_cfg.fwdpoolsize; i++)
        {
            thpool_add_work(G_tmqueue_cfg.fwdthpool, (void *)tp_thread_shutdown, NULL);
        }
        
        
        /* Wait for threads to finish */
        thpool_wait(G_tmqueue_cfg.thpool);
        thpool_destroy(G_tmqueue_cfg.thpool);
        
        thpool_wait(G_tmqueue_cfg.fwdthpool);
        thpool_destroy(G_tmqueue_cfg.fwdthpool);
    }
    tpclose();
    
}
