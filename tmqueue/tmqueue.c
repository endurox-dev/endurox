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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

#include "tmqueue.h"
#include "../libatmisrv/srv_int.h"
#include "tperror.h"
#include "userlog.h"
#include <xa_cmn.h>
#include "thpool.h"
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
/* Wait for one free thread: */
pthread_mutex_t M_wait_th_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t M_wait_th_cond = PTHREAD_COND_INITIALIZER;

/*---------------------------Prototypes---------------------------------*/
private int tx_tout_check(void);
private void tm_chk_one_free_thread(void *ptr);

/**
 * Tmqueue service entry (working thread)
 * @param p_svc - data & len used only...!
 */
void TMQUEUE_TH (void *ptr)
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
        case NDRX_Q_ENQUEUE:
            
            /* start new tran... */
            if (SUCCEED!=tmq_enqueue(p_ub))
            {
                ret=FAIL;
                goto out;
            }
            break;
        case NDRX_Q_DEQUEUE:
            
            /* start new tran... */
            if (SUCCEED!=tmq_dequeue(p_ub))
            {
                ret=FAIL;
                goto out;
            }
            break;
        case NDRX_Q_PRINTQUEUE:
            
            /* request for printing active transactions */
            if (SUCCEED!=tmq_printqueue(p_ub, cd))
            {
                ret=FAIL;
                goto out;
            }
            break;
        default:
            NDRX_LOG(log_error, "Unsupported command code: [%c]", cmd);
            ret=FAIL;
            break;
    }
    
out:
            
    /* Approve the request if all ok */
    if (SUCCEED==ret)
    {
        atmi_xa_approve(p_ub);
    }

    if (SUCCEED!=ret && XA_RDONLY==atmi_xa_get_reason())
    {
        NDRX_LOG(log_debug, "Marking READ ONLY = SUCCEED");
        ret=SUCCEED;
    }

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
    thread_data->context_data = tpsrvgetctxdata();
    
    /* submit the job to thread pool: */
    thpool_add_work(G_tmqueue_cfg.thpool, (void*)TMQUEUE_TH, (void *)thread_data);
    
out:
    if (SUCCEED==ret)
    {
        /* serve next.. 
         * At this point we should know that at least one thread is free
         */
        pthread_mutex_lock(&M_wait_th_mutex);
        
        /* submit the job to verify free thread */
        
        thpool_add_work(G_tmqueue_cfg.thpool, (void*)tm_chk_one_free_thread, NULL);
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
int tpsvrinit(int argc, char **argv)
{
    int ret=SUCCEED;
    signed char c;
    char svcnm[MAXTIDENT+1];
    NDRX_LOG(log_debug, "tpsvrinit called");
    int nodeid;
    
    /* Parse command line  */
    while ((c = getopt(argc, argv, "l:s:p:")) != -1)
    {
        NDRX_LOG(log_debug, "%c = [%s]", c, optarg);
        switch(c)
        {
            case 'd': 
                strcpy(G_tmqueue_cfg.data_dir, optarg);
                NDRX_LOG(log_debug, "QData directory "
                            "set to: [%s]", G_tmqueue_cfg.data_dir);
                break;
            case 'q': 
                strcpy(G_tmqueue_cfg.qspace, optarg);
                NDRX_LOG(log_debug, "Qspace set to: [%s]", G_tmqueue_cfg.qspace);
                break;
            case 's': 
                G_tmqueue_cfg.scan_time = atoi(optarg);
                break;
            case 'p': 
                G_tmqueue_cfg.threadpoolsize = atol(optarg);
                break;
            default:
                /*return FAIL;*/
                break;
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
    
    if (EOS==G_tmqueue_cfg.data_dir[0])
    {
        userlog("TMQ log dir not set!");
        NDRX_LOG(log_error, "TMQ log dir not set!");
        FAIL_OUT(ret);
    }
    NDRX_LOG(log_debug, "Recovery scan time set to [%d]",
                            G_tmqueue_cfg.scan_time);
    
    NDRX_LOG(log_debug, "Worker pool size [%d] threads",
                            G_tmqueue_cfg.threadpoolsize);
    
    NDRX_LOG(log_debug, "About to initialize XA!");
    if (SUCCEED!=atmi_xa_init()) /* will open next... */
    {
        NDRX_LOG(log_error, "Failed to initialize XA driver!");
        FAIL_OUT(ret);
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
        ret = SUCCEED;
    }
                    
    sprintf(svcnm, NDRX_SVC_RM, G_atmi_env.xa_rmid);
    
    if (SUCCEED!=tpadvertise(svcnm, TMQUEUE))
    {
        NDRX_LOG(log_error, "Failed to advertise %s service!", svcnm);
        FAIL_OUT(ret);
    }
    
    
    if (NULL==(G_tmqueue_cfg.thpool = thpool_init(G_tmqueue_cfg.threadpoolsize)))
    {
        NDRX_LOG(log_error, "Failed to initialize thread pool (cnt: %d)!", 
                G_tmqueue_cfg.threadpoolsize);
        FAIL_OUT(ret);
    }
    
    /* Start the background processing */
    background_process_init();
    
    
out:
    return ret;
}

/**
 * Do de-initialization
 */
void tpsvrdone(void)
{
    NDRX_LOG(log_debug, "tpsvrdone called - requesting "
            "background thread shutdown...");
    
    G_bacground_req_shutdown = TRUE;
    
    if (M_init_ok)
    {
        background_wakeup();

        /* Wait to complete */
        pthread_join(G_bacground_thread, NULL);

        /* Wait for threads to finish */
        thpool_wait(G_tmqueue_cfg.thpool);
        thpool_destroy(G_tmqueue_cfg.thpool);
    }
    atmi_xa_close_entry();
    
}

/**
 * Just run down one task via pool, to ensure that at least one
 * thread is free, before we are going to mail poll.
 * @param ptr
 */
private void tm_chk_one_free_thread(void *ptr)
{
    pthread_mutex_lock(&M_wait_th_mutex);
    pthread_cond_signal(&M_wait_th_cond);
    pthread_mutex_unlock(&M_wait_th_mutex);
}
