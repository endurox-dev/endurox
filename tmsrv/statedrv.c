/**
 * @brief Transaction state driver (uses libatmi/xastates.c for driving)
 *
 * @file statedrv.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * AGPL or Mavimax's license for commercial use.
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
#include <xa_cmn.h>
#include <sys_test.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/**
 * branch tid vote entry
 */
typedef struct 
{
    short rmid; /**< RM ID is voting    */
    char stage; /**< to stage           */
    long btid;  /**< with branch id     */
} btid_vote_t;

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Do one try for transaciton processing using state machine defined in atmilib
 * @param p_xai - xa info structure
 * @param p_tl - transaction log
 * @param [in] flags shared tran system and tp flags
 * @return TPreturn code.
 */
expublic int tm_drive(atmi_xa_tx_info_t *p_xai, atmi_xa_log_t *p_tl, int master_op,
                        short rmid, long flags)
{
    int ret = EXSUCCEED;
    int i;
    int again;
    rmstatus_driver_t* vote_txstage;
    txstage_descriptor_t* descr;
    /* char stagearr[NDRX_MAX_RMS];*/
    ndrx_growlist_t stagearr; /**< grow list of voted stages */
    btid_vote_t vote;
    
    int min_in_group;
    int min_in_overall;
    int try=0;
    int was_retry;
    int is_tx_finished = EXFALSE;
    
    NDRX_LOG(log_info, "tm_drive() enter from xid=[%s] flags=%ld", 
            p_xai->tmxid, flags);
    
    memset(&stagearr, 0, sizeof(stagearr));
    
    do
    {
        short new_txstage = 0;
        int op_code = 0;
        int op_ret = 0;
        int op_reason = 0;
        int op_tperrno = 0;
        atmi_xa_rm_status_btid_t *el, *elt;
        was_retry = EXFALSE;
        
        again = EXFALSE;
        
        if (NULL==(descr = xa_stage_get_descr(p_tl->txstage)))
        {
            NDRX_LOG(log_error, "Invalid stage %hd", p_tl->txstage);
            ret=TPESYSTEM;
            goto out;
        }
        
        NDRX_LOG(log_info, "Entered in stage: %s", descr->descr);
        
        if (NULL!=stagearr.mem)
        {
            /* reset the momory, no reset needed. */
            stagearr.maxindexused = -1;
        }
        else if (0==stagearr.size) /* not initialized */
        {
            /* this does not allocate memory */
            ndrx_growlist_init(&stagearr, 100, sizeof(btid_vote_t));
        }
         
        for (i=0; i<NDRX_MAX_RMS; i++)
        {
            EXHASH_ITER(hh, p_tl->rmstatus[i].btid_hash, el, elt)
            {

                NDRX_LOG(log_info, "RMID: %hd status %c", 
                                            i+1, el->rmstatus);

                op_reason = XA_OK;
                op_tperrno = 0;
                op_code = xa_status_get_op(p_tl->txstage, el->rmstatus);
                switch (op_code)
                {
                    case XA_OP_NOP:
                        NDRX_LOG(log_info, "OP_NOP");
                        break;
                    case XA_OP_PREPARE:
                        NDRX_LOG(log_error, "Prepare RMID %d", i+1);
                        if (EXSUCCEED!=(op_ret = tm_prepare_combined(p_xai, i+1, el->btid)))
                        {
                            op_reason = atmi_xa_get_reason();
                            op_tperrno = tperrno;
                        }
                        break;
                    case XA_OP_COMMIT:
                        NDRX_LOG(log_info, "Commit RMID %d", i+1);
                        
                        /* system test entry point
                         * for case when tmsrv is unable to complete...
                         */
                        if (NDRX_SYSTEST_ENBLD && ndrx_systest_case(NDRX_SYSTEST_TMSCOMMIT))
                        {
                            op_reason = XAER_RMERR;
                            op_tperrno = TPESVCERR;
                        }
                        else if (EXSUCCEED!=(op_ret = tm_commit_combined(p_xai, i+1, el->btid)))
                        {
                            op_reason = atmi_xa_get_reason();
                            op_tperrno = tperrno;
                        }
                        break;
                    case XA_OP_ROLLBACK:
                        NDRX_LOG(log_info, "Rollback RMID %d", i+1);
                        if (EXSUCCEED!=(op_ret = tm_rollback_combined(p_xai, i+1, el->btid)))
                        {
                            op_reason = atmi_xa_get_reason();
                            op_tperrno = tperrno;
                        }
                        break;
                    default:
                        NDRX_LOG(log_error, "Invalid opcode %d", op_code);
                        ret=TPESYSTEM;
                        goto out;
                        break;
                }
                NDRX_LOG(log_info, "Operation tperrno: %d, xa return code: %d",
                                         op_tperrno, op_reason);

                if (op_reason==XA_RETRY) 
                {
                    was_retry = EXTRUE;   
                }

                /* Now get the transition of the state/vote */
                if (XA_OP_NOP == op_code)
                {
                    if (NULL==(vote_txstage = xa_status_get_next_by_new_status(p_tl->txstage, 
                            el->rmstatus)))
                    {
                        NDRX_LOG(log_info, "No stage info for %hd/%c - ignore", p_tl->txstage, 
                            el->rmstatus);
                        /*
                        ret=TPESYSTEM;
                        goto out;
                        */
                        continue;
                    }
                }
                else
                {
                    if (NULL==(vote_txstage = xa_status_get_next_by_op(p_tl->txstage, 
                            el->rmstatus, op_code, op_reason)))
                    {
                        NDRX_LOG(log_error, "Invalid stage for %hd/%c/%d/%d", 
                                p_tl->txstage, el->rmstatus, op_code, op_reason);
                        ret=TPESYSTEM;
                        goto out;
                    }

                    /* Log RM status change... */
                    tms_log_rmstatus(p_tl, el, vote_txstage->next_rmstatus, 
                            tperrno, op_reason);
                }
                /* Stage switching... */
                vote.btid = el->btid;
                vote.rmid = el->rmid;
                vote.stage = vote_txstage->next_txstage;
                
                if (EXSUCCEED!=ndrx_growlist_append(&stagearr, &vote))
                {
                    NDRX_LOG(log_error, "Failed to add rmid=%hd, btid=%hd to "
                            "stagearr with stage=%c",
                            vote.rmid, vote.btid, vote.stage);
                    ret=TPESYSTEM;
                    goto out;
                }
                
                if ((descr->txs_stage_min>vote_txstage->next_txstage ||
                        descr->txs_max_complete<vote_txstage->next_txstage) && descr->allow_jump)
                {
                    NDRX_LOG(log_warn, "Stage group left!");

                    new_txstage = vote_txstage->next_txstage;
                    /* switch the stage */
                    again = EXTRUE;
                    break;
                }

                /* Maybe we need some kind of arrays to put return stages in? 
                 We need to put all states in one array.
                 1. If there is any stage in the min & max ranges => Stick with the lowest from range
                 2. If there is nothing in range, but have stuff outside, then take lowest from outside
                 */
            }
        }
        
        if (!new_txstage)
        {
            min_in_group = XA_TX_STAGE_MAX_NEVER;
            min_in_overall = XA_TX_STAGE_MAX_NEVER;
            /* Calculate from array */
            for (i=0; i<=stagearr.maxindexused; i++)
            {
                btid_vote_t *ve = stagearr.mem+sizeof(btid_vote_t)*i;
                
                NDRX_LOG(log_info, "RM %hd btid=%ld votes for stage: %d", 
                        ve->rmid, ve->btid, ve->stage);

                /* Bug #150 */
                if (ve->stage < min_in_overall)
                {
                    min_in_overall = ve->stage;
                    NDRX_LOG(log_debug, "min_in_overall=>%d", min_in_overall);
                }

                /* what is this? Descr and vote_txstage will be last
                 * from the loop - wrong!
                 * We play with next stages from arr: stagearr[i]
                 * What is group? Seems like same type of staging, i.e.
                 * still committing
                 */
                if (descr->txs_stage_min<=ve->stage && 
                        descr->txs_max_complete>=ve->stage &&
                        min_in_group < ve->stage)
                {
                    min_in_group = ve->stage;
                    NDRX_LOG(log_debug, "min_in_group=>%d", min_in_group);
                }
            }/* for */
            
            if (min_in_group!=XA_TX_STAGE_MAX_NEVER)
            {
                new_txstage=min_in_group;
                NDRX_LOG(log_debug, "New tx stage set by min_in_group=>%d", new_txstage);
            }
            else
            {
                new_txstage=min_in_overall;
                NDRX_LOG(log_debug, "New tx stage set by min_in_overall=>%d", new_txstage);
            }
            
            if (XA_TX_STAGE_MAX_NEVER==new_txstage)
            {
                NDRX_LOG(log_error, "Stage not switched - assume MAX COMPLETED!");
                new_txstage=descr->txs_max_complete;
                /*
                ret=TPESYSTEM;
                goto out;
                */
            }
            
        } /* calc stage */
        
        /* Finally switch the stage & run again! */
        if (new_txstage!=descr->txstage && new_txstage!=XA_TX_STAGE_MAX_NEVER)
        {
            tms_log_stage(p_tl, new_txstage);
            again = EXTRUE;
        }
        
        /* if switched to committing & requested descision logged, then return */
        if ( (flags & TP_CMT_LOGGED) && XA_TX_STAGE_COMMITTING == descr->txstage)
        {
            NDRX_LOG(log_info, "Decision logged for commit return");
            goto out;
        }
        
        if (was_retry)
        {
            try++;
            
            NDRX_LOG(log_warn, "XA_RETRY: current try: %d, max (-r): %d", 
                        try, G_tmsrv_cfg.xa_retries);
            
            if (try<G_tmsrv_cfg.xa_retries)
            {
                again = EXTRUE;
                NDRX_LOG(log_warn, "Retry on XA_RETRY");
            }
        }
        else
        {
            /* reset counter if no retry */
            try = 0;
        }
        
    } while (again);
    
    /* Check are we complete */
    if (descr->txstage >=descr->txs_min_complete &&
            descr->txstage <=descr->txs_max_complete)
    {
        NDRX_LOG(log_info, "Transaction completed - remove logs");
        
        /* p_tl becomes invalid! */
        tms_remove_logfile(p_tl);
        
        is_tx_finished = EXTRUE;
    }
    
    /* map stage to return code */
    ret = xa_txstage2tperrno(descr->txstage, master_op);
    
out:          

    /* Bug #199 if system error occurs transaction 
     * 
     */
    if (!is_tx_finished)
    {
        /* move transaction to background */
        if (!p_tl->is_background)
        {
            NDRX_LOG(log_info, "Transaction not completed - leave "
                    "to background");
            p_tl->is_background = EXTRUE;
        }
        else
        {
            NDRX_LOG(log_info, "Transaction not completed - will be processed with next"
                    "background cycle (if not expired)");
        }
        
        /* Unlock the transaction */
        tms_unlock_entry(p_tl);
    }

    if (NULL!=stagearr.mem)
    {
        ndrx_growlist_free(&stagearr);
    }

    NDRX_LOG(log_info, "tm_drive() returns %d", ret);
    return ret;
}
/* vim: set ts=4 sw=4 et smartindent: */
