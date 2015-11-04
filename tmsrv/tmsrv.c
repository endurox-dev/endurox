/* 
** Tmsrv server - transaction monitor
** TODO: Create framework for error processing.
** After that log transaction to hash & to disk for tracking the stuff...
** TODO: We should have similar control like "TP_COMMIT_CONTROL" -
** either return after stuff logged or after really commit completed.
** TODO: Also we have to think about conversations, how xa will act there.
** Error handling:
** - System errors we will track via ATMI interface error functions
** - XA errors will be tracked via XA error interface
** TODO: We need a periodical callback from checking for transaction time-outs!
** TODO: Should we call xa_end for joined transactions? See:
** https://www-01.ibm.com/support/knowledgecenter/SSFKSJ_7.0.1/com.ibm.mq.amqzag.doc/fa13870_.htm
**
** @file tmsrv.c
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

#include "tmsrv.h"
#include "../libatmisrv/srv_int.h"
#include "tperror.h"
#include "userlog.h"
#include <uuid/uuid.h>
#include <xa_cmn.h>
/*---------------------------Externs------------------------------------*/
extern int optind, optopt, opterr;
extern char *optarg;
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
public tmsrv_cfg_t G_tmsrv_cfg;
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
private int tx_tout_check(void);

/**
 * Tmsrv service entry
 * @param p_svc - data & len used only...!
 */
void TPTMSRV (TPSVCINFO *p_svc)
{
    /* Ok we should not handle the commands 
     * TPBEGIN...
     */
    UBFH *p_ub = (UBFH *)p_svc->data;
    int ret=SUCCEED;
    
    char cmd = EOS;
    
    /* get some more stuff! */
    if (Bunused (p_ub) < 4096)
    {
        p_ub = (UBFH *)tprealloc ((char *)p_ub, Bsizeof (p_ub) + 4096);
    }
    
    ndrx_debug_dump_UBF(log_info, "TPTMSRV call buffer:", p_ub);
    
    if (Bget(p_ub, TMCMD, 0, (char *)&cmd, 0L))
    {
        NDRX_LOG(log_error, "Failed to read command code!");
        ret=FAIL;
        goto out;
    }
    NDRX_LOG(log_info, "Got command code: [%c]", cmd);
    
    switch(cmd)
    {
        case ATMI_XA_TPBEGIN:
            
            /* start new tran... */
            if (SUCCEED!=tm_tpbegin(p_ub))
            {
                ret=FAIL;
                goto out;
            }
            break;
        case ATMI_XA_TPCOMMIT:
            
            /* start new tran... */
            if (SUCCEED!=tm_tpcommit(p_ub))
            {
                ret=FAIL;
                goto out;
            }
            break;
        case ATMI_XA_TPABORT:
            
            /* start new tran... */
            if (SUCCEED!=tm_tpabort(p_ub))
            {
                ret=FAIL;
                goto out;
            }
            break;
        case ATMI_XA_PRINTTRANS:
            
            /* request for printing active transactions */
            if (SUCCEED!=tm_tpprinttrans(p_ub, p_svc->cd))
            {
                ret=FAIL;
                goto out;
            }
            break;
        case ATMI_XA_ABORTTRANS:
            
            /* request for printing active transactions */
            if (SUCCEED!=tm_aborttrans(p_ub))
            {
                ret=FAIL;
                goto out;
            }
            break;
        case ATMI_XA_COMMITTRANS:
            
            /* request for printing active transactions */
            if (SUCCEED!=tm_committrans(p_ub))
            {
                ret=FAIL;
                goto out;
            }
            break;
        case ATMI_XA_TMPREPARE:
            
            /* prepare the stuff locally */
            if (SUCCEED!=tm_tmprepare(p_ub))
            {
                ret=FAIL;
                goto out;
            }
            break;
        case ATMI_XA_TMCOMMIT:
            
            /* prepare the stuff locally */
            if (SUCCEED!=tm_tmcommit(p_ub))
            {
                ret=FAIL;
                goto out;
            }
            break;
        case ATMI_XA_TMABORT:
            
            /* abort the stuff locally */
            if (SUCCEED!=tm_tmabort(p_ub))
            {
                ret=FAIL;
                goto out;
            }
            break;
        case ATMI_XA_TMREGISTER:
            /* Some binary is telling as the different RM is involved
             * in transaction.
             */
            if (SUCCEED!=tm_tmregister(p_ub))
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

    ndrx_debug_dump_UBF(log_info, "TPTMSRV return buffer:", p_ub);

    tpreturn(  ret==SUCCEED?TPSUCCESS:TPFAIL,
                0L,
                (char *)p_ub,
                0L,
                0L);
}

/*
 * Do initialization
 */
int tpsvrinit(int argc, char **argv)
{
    int ret=SUCCEED;
    char c;
    char svcnm[MAXTIDENT+1];
    NDRX_LOG(log_debug, "tpsvrinit called");
    int nodeid;
    /* Parse command line  */
    while ((c = getopt(argc, argv, "t:s:l:c:m:")) != -1)
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
                strcpy(G_tmsrv_cfg.tlog_dir, optarg);
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
    
    if (EOS==G_tmsrv_cfg.tlog_dir[0])
    {
        userlog("TMS log dir not set!");
        NDRX_LOG(log_error, "TMS log dir not set!");
        FAIL_OUT(ret);
    }
    NDRX_LOG(log_debug, "Recovery scan time set to [%d]",
                            G_tmsrv_cfg.scan_time);
    
    NDRX_LOG(log_debug, "Tx max tries set to [%d]",
                            G_tmsrv_cfg.max_tries);
    
    NDRX_LOG(log_debug, "Tx tout check time [%d]",
                            G_tmsrv_cfg.tout_check_time);
    
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
                
    /* All OK, about to advertise services */
    nodeid = tpgetnodeid();
    if (nodeid<1)
    {
        NDRX_LOG(log_error, "Failed to get current node_id");
        FAIL_OUT(ret);
    }
    
    /* very generic version/only Resource ID known */
    
    sprintf(svcnm, NDRX_SVC_RM, G_atmi_env.xa_rmid);
    
    if (SUCCEED!=tpadvertise(svcnm, TPTMSRV))
    {
        NDRX_LOG(log_error, "Failed to advertise %s service!", svcnm);
        FAIL_OUT(ret);
    }
    
    /* generic instance: */
    sprintf(svcnm, NDRX_SVC_TM, nodeid, G_atmi_env.xa_rmid);
    
    if (SUCCEED!=tpadvertise(svcnm, TPTMSRV))
    {
        NDRX_LOG(log_error, "Failed to advertise %s service!", svcnm);
        FAIL_OUT(ret);
    }
    
    /* specific instance */
    sprintf(svcnm, NDRX_SVC_TM_I, nodeid, G_atmi_env.xa_rmid, G_server_conf.srv_id);
    if (SUCCEED!=tpadvertise(svcnm, TPTMSRV))
    {
        NDRX_LOG(log_error, "Failed to advertise %s service!", svcnm);
        FAIL_OUT(ret);
    }
    
    /* Start the background processing */
    background_process_init();
    
    
    /* Register timer check (needed for time-out detection) */
    if (SUCCEED==ret &&
            SUCCEED!=tpext_addperiodcb(G_tmsrv_cfg.tout_check_time, tx_tout_check))
    {
            ret=FAIL;
            NDRX_LOG(log_error, "tpext_addperiodcb failed: %s",
                            tpstrerror(tperrno));
    }
    
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
    background_wakeup();
    
    /* Wait to complete */
    pthread_join(G_bacground_thread, NULL);
    
    atmi_xa_close_entry();
    
}

/**
 * Periodic main thread callback for 
 * @return 
 */
private int tx_tout_check(void)
{
    int ret = SUCCEED;
    long tspent;
    atmi_xa_log_list_t *tx_list;
    atmi_xa_log_list_t *el, *tmp;
    atmi_xa_tx_info_t xai;
    
    /* Create a copy of hash, iterate and check each tx for timeout condition
     * If so then initiate internal abort call
     */
    
    tx_list = tms_copy_hash2list(COPY_MODE_FOREGROUND | COPY_MODE_ACQLOCK);
        
    LL_FOREACH_SAFE(tx_list,el,tmp)
    {
        if ((tspent = n_timer_get_delta_sec(&el->p_tl->ttimer)) > 
                el->p_tl->txtout && XA_TX_STAGE_ACTIVE==el->p_tl->txstage)
        {
            NDRX_LOG(log_error, "XID [%s] timed out "
                    "(spent %ld, limit: %ld sec) - aborting...!", 
                    el->p_tl->tmxid, tspent, 
                    el->p_tl->txtout);
            
            userlog("XID [%s] timed out "
                    "(spent %ld, limit: %ld sec) - aborting...!", 
                    el->p_tl->tmxid, tspent, 
                    el->p_tl->txtout);
            
            XA_TX_COPY((&xai), el->p_tl);
            
            tms_log_stage(el->p_tl, XA_TX_STAGE_ABORTING);
            tm_drive(&xai, el->p_tl, XA_OP_ROLLBACK, FAIL);
        }
        free(el);
    }
out:    
    return ret;
}

