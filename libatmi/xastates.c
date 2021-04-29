/**
 * @brief State transition handling of XA transactions
 *
 * @file xastates.c
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
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <dlfcn.h>
#include <limits.h>

#include <atmi.h>
#include <atmi_shm.h>
#include <ndrstandard.h>
#include <ndebug.h>
#include <ndrxdcmn.h>
#include <userlog.h>
#include <xa_cmn.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

/**
 * Static branch driver
 * We will have two drives of all of this:
 * 1. Get new RM status (driven by current stage, status, operation and outcome)
 * 2. Get new TX state (Driven by current TX stage, and RM new status)
 * And the for file TX outcome we should select the stage with lower number...
 */

/* =========================================================================
 * Driving of the PREPARING: 
 * =========================================================================
 */
exprivate rmstatus_driver_t M_rm_status_driver_preparing[] =
{  

    {XA_TX_STAGE_PREPARING, XA_RM_STATUS_ACTIVE, XA_OP_PREPARE, XA_OK,     XA_OK,     XA_RM_STATUS_PREP,          XA_TX_STAGE_COMMITTING},
    {XA_TX_STAGE_PREPARING, XA_RM_STATUS_ACTIVE, XA_OP_PREPARE, XA_RDONLY, XA_RDONLY, XA_RM_STATUS_COMMITTED_RO,  XA_TX_STAGE_COMMITTED},
    /* Shall we perform any action here? */
    {XA_TX_STAGE_PREPARING, XA_RM_STATUS_ACTIVE, XA_OP_PREPARE, XA_RBBASE, XA_RBEND,  XA_RM_STATUS_ABORTED,       XA_TX_STAGE_ABORTING},
    /* Abort immediately... */
    {XA_TX_STAGE_PREPARING, XA_RM_STATUS_ACTIVE, XA_OP_PREPARE, XAER_RMERR,XAER_RMERR,XA_RM_STATUS_ACT_AB,        XA_TX_STAGE_ABORTING},
    /* If no transaction, then assume committed, read only: */
    {XA_TX_STAGE_PREPARING, XA_RM_STATUS_ACTIVE, XA_OP_PREPARE, XAER_NOTA, XAER_NOTA, XA_RM_STATUS_COMMITTED_RO,  XA_TX_STAGE_COMMITTING},
    {XA_TX_STAGE_PREPARING, XA_RM_STATUS_ACTIVE, XA_OP_PREPARE, XAER_INVAL,XAER_INVAL,XA_RM_STATUS_ACT_AB,        XA_TX_STAGE_ABORTING},
    /* for PostgreSQL we have strange situation, that only case to work in distributed way is to mark the transaction as
     * prepared once the processing thread disconnects. Thus even transaction is active, the resource is prepared.
     */
    {XA_TX_STAGE_PREPARING, XA_RM_STATUS_PREP,   XA_OP_NOP,     XA_OK,XA_OK,          XA_RM_STATUS_PREP,          XA_TX_STAGE_COMMITTING},
    /* If recovered from logs where decision is not yet logged */
    {XA_TX_STAGE_PREPARING, XA_RM_STATUS_ACT_AB, XA_OP_NOP,     XA_OK,XA_OK,          XA_RM_STATUS_ACT_AB,        XA_TX_STAGE_ABORTING},
    /* Any error out of the range (catched, we scan from the start) causes abort sequence to run */
    {XA_TX_STAGE_PREPARING, XA_RM_STATUS_ACTIVE, XA_OP_PREPARE, INT_MIN,INT_MAX,      XA_RM_STATUS_ACT_AB,        XA_TX_STAGE_ABORTING},
    
    {EXFAIL}
};

/* =========================================================================
 * Driving of the COMMITTING 
 * =========================================================================
 */
exprivate rmstatus_driver_t M_rm_status_driver_committing[] =
{  
    {XA_TX_STAGE_COMMITTING, XA_RM_STATUS_PREP,   XA_OP_COMMIT,  XA_OK,     XA_OK,     XA_RM_STATUS_COMMITTED,     XA_TX_STAGE_COMMITTED},
    {XA_TX_STAGE_COMMITTING, XA_RM_STATUS_PREP,   XA_OP_COMMIT,  XAER_RMERR,XAER_RMERR,XA_RM_STATUS_COMMIT_HEURIS, XA_TX_STAGE_COMMITTED_HEURIS},
    {XA_TX_STAGE_COMMITTING, XA_RM_STATUS_PREP,   XA_OP_COMMIT,  XA_RETRY,  XA_RETRY,  XA_RM_STATUS_PREP,          XA_TX_STAGE_COMMITTING},
    {XA_TX_STAGE_COMMITTING, XA_RM_STATUS_PREP,   XA_OP_COMMIT,  XA_HEURHAZ,XA_HEURHAZ,XA_RM_STATUS_COMMIT_HAZARD, XA_TX_STAGE_COMMITTED_HAZARD},
    {XA_TX_STAGE_COMMITTING, XA_RM_STATUS_PREP,   XA_OP_COMMIT,  XA_HEURCOM,XA_HEURCOM,XA_RM_STATUS_COMMIT_HEURIS, XA_TX_STAGE_COMMITTED_HEURIS},
    
    /* =============================================
     * TODO: test this case, output? 
     * Isn't this be swapped with bellow?
     */
    {XA_TX_STAGE_COMMITTING, XA_RM_STATUS_PREP,   XA_OP_COMMIT,  XA_HEURRB,XA_HEURRB,  XA_RM_STATUS_ABORT_HEURIS,  XA_TX_STAGE_COMMITTED_HEURIS},
    /* If only one RM, then we might switch back to aborted: */
    {XA_TX_STAGE_COMMITTING, XA_RM_STATUS_PREP,   XA_OP_COMMIT,  XA_HEURMIX,XA_HEURMIX,XA_RM_STATUS_COMMIT_HEURIS, XA_TX_STAGE_ABORTED},
    /* ============================================= */
    /* Also here shouldn't it be HEURIS? */
    {XA_TX_STAGE_COMMITTING, XA_RM_STATUS_PREP,   XA_OP_COMMIT,  XA_RBBASE, XA_RBEND,  XA_RM_STATUS_ABORTED,       XA_TX_STAGE_ABORTED},
    /* ============================================= */
    
    {XA_TX_STAGE_COMMITTING, XA_RM_STATUS_PREP,   XA_OP_COMMIT,  XAER_NOTA, XAER_NOTA, XA_RM_STATUS_COMMITTED_RO,  XA_TX_STAGE_COMMITTED},
    /* these are RO reported by end prep */
    {XA_TX_STAGE_COMMITTING, XA_RM_STATUS_COMMITTED_RO,XA_OP_NOP,XA_OK, XA_OK,         XA_RM_STATUS_COMMITTED_RO,  XA_TX_STAGE_COMMITTED},
    /* TODO: Add NOP status voting by RM status, thought if we loose the infos after the restart of the HEURIS/HAZARD no one would see
     * these responses too, as there is no process after the restart waiting for response.
     */
    {EXFAIL}
};

/* =========================================================================
 * Driving of ABORTING:  
 * =========================================================================
 */
exprivate rmstatus_driver_t M_rm_status_driver_aborting[] =
{  
    {XA_TX_STAGE_ABORTING, XA_RM_STATUS_ACT_AB, XA_OP_ROLLBACK, XA_OK,     XA_OK,     XA_RM_STATUS_ABORTED,        XA_TX_STAGE_ABORTED},
    {XA_TX_STAGE_ABORTING, XA_RM_STATUS_ACTIVE, XA_OP_ROLLBACK, XA_OK,     XA_OK,     XA_RM_STATUS_ABORTED,        XA_TX_STAGE_ABORTED},
    {XA_TX_STAGE_ABORTING, XA_RM_STATUS_PREP,   XA_OP_ROLLBACK, XA_OK,     XA_OK,     XA_RM_STATUS_ABORTED,        XA_TX_STAGE_ABORTED},
    /* these are RO reported by end prep */
    {XA_TX_STAGE_ABORTING, XA_RM_STATUS_COMMITTED_RO,XA_OP_NOP, XA_OK,     XA_OK,     XA_RM_STATUS_ABORTED,        XA_TX_STAGE_ABORTED},
    
    {XA_TX_STAGE_ABORTING, XA_RM_STATUS_ACT_AB, XA_OP_ROLLBACK, XA_RDONLY, XA_RDONLY, XA_RM_STATUS_ABORTED,        XA_TX_STAGE_ABORTED},
    {XA_TX_STAGE_ABORTING, XA_RM_STATUS_ACTIVE, XA_OP_ROLLBACK, XA_RDONLY, XA_RDONLY, XA_RM_STATUS_ABORTED,        XA_TX_STAGE_ABORTED},
    {XA_TX_STAGE_ABORTING, XA_RM_STATUS_PREP,   XA_OP_ROLLBACK, XA_RDONLY, XA_RDONLY, XA_RM_STATUS_ABORTED,        XA_TX_STAGE_ABORTED},
    
    /* TODO: is it really aborted in case if we get RMERR? */
    {XA_TX_STAGE_ABORTING, XA_RM_STATUS_ACT_AB, XA_OP_ROLLBACK, XAER_RMERR, XAER_RMERR,XA_RM_STATUS_ABORTED,       XA_TX_STAGE_ABORTED},
    {XA_TX_STAGE_ABORTING, XA_RM_STATUS_ACTIVE, XA_OP_ROLLBACK, XAER_RMERR, XAER_RMERR,XA_RM_STATUS_ABORTED,       XA_TX_STAGE_ABORTED},
    {XA_TX_STAGE_ABORTING, XA_RM_STATUS_PREP,   XA_OP_ROLLBACK, XAER_RMERR, XAER_RMERR,XA_RM_STATUS_ABORTED,       XA_TX_STAGE_ABORTED},
    
    {XA_TX_STAGE_ABORTING, XA_RM_STATUS_ACT_AB, XA_OP_ROLLBACK, XAER_RMERR, XAER_RMERR,XA_RM_STATUS_ABORTED,       XA_TX_STAGE_ABORTED},
    {XA_TX_STAGE_ABORTING, XA_RM_STATUS_ACTIVE, XA_OP_ROLLBACK, XAER_RMERR, XAER_RMERR,XA_RM_STATUS_ABORTED,       XA_TX_STAGE_ABORTED},
    {XA_TX_STAGE_ABORTING, XA_RM_STATUS_PREP,   XA_OP_ROLLBACK, XAER_RMERR, XAER_RMERR,XA_RM_STATUS_ABORTED,       XA_TX_STAGE_ABORTED},
    
    {XA_TX_STAGE_ABORTING, XA_RM_STATUS_ACT_AB, XA_OP_ROLLBACK, XA_HEURHAZ, XA_HEURHAZ,XA_RM_STATUS_ABORT_HAZARD,  XA_TX_STAGE_ABORTED_HAZARD},
    {XA_TX_STAGE_ABORTING, XA_RM_STATUS_ACTIVE, XA_OP_ROLLBACK, XA_HEURHAZ, XA_HEURHAZ,XA_RM_STATUS_ABORT_HAZARD,  XA_TX_STAGE_ABORTED_HAZARD},
    {XA_TX_STAGE_ABORTING, XA_RM_STATUS_PREP,   XA_OP_ROLLBACK, XA_HEURHAZ, XA_HEURHAZ,XA_RM_STATUS_ABORT_HAZARD,  XA_TX_STAGE_ABORTED_HAZARD},
    
    {XA_TX_STAGE_ABORTING, XA_RM_STATUS_ACT_AB, XA_OP_ROLLBACK, XA_HEURRB, XA_HEURRB,  XA_RM_STATUS_ABORT_HEURIS,  XA_TX_STAGE_ABORTED_HEURIS},
    {XA_TX_STAGE_ABORTING, XA_RM_STATUS_ACTIVE, XA_OP_ROLLBACK, XA_HEURRB, XA_HEURRB,  XA_RM_STATUS_ABORT_HEURIS,  XA_TX_STAGE_ABORTED_HEURIS},
    {XA_TX_STAGE_ABORTING, XA_RM_STATUS_PREP,   XA_OP_ROLLBACK, XA_HEURRB, XA_HEURRB,  XA_RM_STATUS_ABORT_HEURIS,  XA_TX_STAGE_ABORTED_HEURIS},
    
    /* TODO: Test this: */
    {XA_TX_STAGE_ABORTING, XA_RM_STATUS_ACT_AB, XA_OP_ROLLBACK, XA_HEURCOM, XA_HEURCOM,XA_RM_STATUS_COMMIT_HEURIS, XA_TX_STAGE_COMMITTED_HEURIS},
    {XA_TX_STAGE_ABORTING, XA_RM_STATUS_ACTIVE, XA_OP_ROLLBACK, XA_HEURCOM, XA_HEURCOM,XA_RM_STATUS_COMMIT_HEURIS, XA_TX_STAGE_COMMITTED_HEURIS},
    {XA_TX_STAGE_ABORTING, XA_RM_STATUS_PREP,   XA_OP_ROLLBACK, XA_HEURCOM, XA_HEURCOM,XA_RM_STATUS_COMMIT_HEURIS, XA_TX_STAGE_COMMITTED_HEURIS},
    
    {XA_TX_STAGE_ABORTING, XA_RM_STATUS_ACT_AB, XA_OP_ROLLBACK, XA_HEURMIX, XA_HEURMIX,XA_RM_STATUS_ABORT_HEURIS,  XA_TX_STAGE_ABORTED_HEURIS},
    {XA_TX_STAGE_ABORTING, XA_RM_STATUS_ACTIVE, XA_OP_ROLLBACK, XA_HEURMIX, XA_HEURMIX,XA_RM_STATUS_ABORT_HEURIS,  XA_TX_STAGE_ABORTED_HEURIS},
    {XA_TX_STAGE_ABORTING, XA_RM_STATUS_PREP,   XA_OP_ROLLBACK, XA_HEURMIX, XA_HEURMIX,XA_RM_STATUS_ABORT_HEURIS,  XA_TX_STAGE_ABORTED_HEURIS},
    
    /* TODO: shall we continue with aborting? If we get RMFAIL? */
    {XA_TX_STAGE_ABORTING, XA_RM_STATUS_ACT_AB, XA_OP_ROLLBACK, XAER_RMFAIL, XAER_RMFAIL,XA_RM_STATUS_ACTIVE,      XA_TX_STAGE_ABORTING},
    {XA_TX_STAGE_ABORTING, XA_RM_STATUS_ACTIVE, XA_OP_ROLLBACK, XAER_RMFAIL, XAER_RMFAIL,XA_RM_STATUS_ACTIVE,      XA_TX_STAGE_ABORTING},
    {XA_TX_STAGE_ABORTING, XA_RM_STATUS_PREP,   XA_OP_ROLLBACK, XAER_RMFAIL, XAER_RMFAIL,XA_RM_STATUS_PREP,        XA_TX_STAGE_ABORTING},
    
    {XA_TX_STAGE_ABORTING, XA_RM_STATUS_ACT_AB, XA_OP_ROLLBACK, XAER_NOTA, XAER_NOTA,    XA_RM_STATUS_ABORTED,     XA_TX_STAGE_ABORTED},
    {XA_TX_STAGE_ABORTING, XA_RM_STATUS_ACTIVE, XA_OP_ROLLBACK, XAER_NOTA, XAER_NOTA,    XA_RM_STATUS_ABORTED,     XA_TX_STAGE_ABORTED},
    {XA_TX_STAGE_ABORTING, XA_RM_STATUS_PREP,   XA_OP_ROLLBACK, XAER_NOTA, XAER_NOTA,    XA_RM_STATUS_ABORTED,     XA_TX_STAGE_ABORTED},

    /* TODO: shall we continue with aborting? If we get XAER_PROTO? */
    {XA_TX_STAGE_ABORTING, XA_RM_STATUS_ACT_AB, XA_OP_ROLLBACK, XAER_PROTO, XAER_PROTO,  XA_RM_STATUS_ACTIVE,      XA_TX_STAGE_ABORTING},
    {XA_TX_STAGE_ABORTING, XA_RM_STATUS_ACTIVE, XA_OP_ROLLBACK, XAER_PROTO, XAER_PROTO,  XA_RM_STATUS_ACTIVE,      XA_TX_STAGE_ABORTING},
    {XA_TX_STAGE_ABORTING, XA_RM_STATUS_PREP,   XA_OP_ROLLBACK, XAER_PROTO, XAER_PROTO,  XA_RM_STATUS_PREP,        XA_TX_STAGE_ABORTING},
    
    /* Our extension allow to retry for abort case (must have for tmq) */
    {XA_TX_STAGE_ABORTING, XA_RM_STATUS_ACT_AB, XA_OP_ROLLBACK, XA_RETRY, XA_RETRY,      XA_RM_STATUS_ACTIVE,      XA_TX_STAGE_ABORTING},
    {XA_TX_STAGE_ABORTING, XA_RM_STATUS_ACTIVE, XA_OP_ROLLBACK, XA_RETRY, XA_RETRY,      XA_RM_STATUS_ACTIVE,      XA_TX_STAGE_ABORTING},
    {XA_TX_STAGE_ABORTING, XA_RM_STATUS_PREP,   XA_OP_ROLLBACK, XA_RETRY, XA_RETRY,      XA_RM_STATUS_PREP,        XA_TX_STAGE_ABORTING},
    
    /* Other unknown RMstauses we can ignore here, as they have finalized the transaction already, and we live already in ABORTING stage */
    
    {EXFAIL}
};

/**
 * If Stage/State not in list, then assume XA_OP_NOP
 */
expublic txaction_driver_t G_txaction_driver[] =
{  
    {XA_TX_STAGE_PREPARING, XA_RM_STATUS_ACTIVE, 		XA_OP_PREPARE},
    {XA_TX_STAGE_COMMITTING,XA_RM_STATUS_PREP, 			XA_OP_COMMIT},
    {XA_TX_STAGE_ABORTING,  XA_RM_STATUS_ACTIVE, 		XA_OP_ROLLBACK},
    {XA_TX_STAGE_ABORTING,  XA_RM_STATUS_PREP, 	    	        XA_OP_ROLLBACK},
    {EXFAIL}
};

/**
 * State descriptors
 */
expublic txstage_descriptor_t G_state_descriptor[] =
{
/* txstage                     txs_stage_min                 txs_min_complete             txs_max_complete  descr   allow_jump */
{XA_TX_STAGE_NULL,             XA_TX_STAGE_NULL,             XA_TX_STAGE_NULL,            XA_TX_STAGE_NULL, "NULL", EXFALSE},
{XA_TX_STAGE_ACTIVE,           XA_TX_STAGE_ACTIVE,           XA_TX_STAGE_NULL,            XA_TX_STAGE_NULL, "ACTIVE", EXFALSE},
{XA_TX_STAGE_ABORTING,         XA_TX_STAGE_ABORTING,         XA_TX_STAGE_ABORTED_HAZARD,  XA_TX_STAGE_ABORTED, "ABORTING", EXFALSE},
/* Left for compliance: */
{XA_TX_STAGE_ABORTED_HAZARD,   XA_TX_STAGE_ABORTED_HAZARD,   XA_TX_STAGE_ABORTED_HAZARD,  XA_TX_STAGE_ABORTED_HAZARD, "ABORTED_HAZARD", EXFALSE},
/* Left for compliance: */
{XA_TX_STAGE_ABORTED_HEURIS,   XA_TX_STAGE_ABORTED_HEURIS,   XA_TX_STAGE_ABORTED_HEURIS,  XA_TX_STAGE_ABORTED_HEURIS, "ABORTED_HEURIS", EXFALSE},
/* Left for compliance: */
{XA_TX_STAGE_ABORTED,          XA_TX_STAGE_ABORTED,          XA_TX_STAGE_ABORTED,         XA_TX_STAGE_ABORTED, "ABORTED", EXFALSE},
/* This assumes that after preparing follows committing, and this is the group. The completed min is: XA_TX_STAGE_COMMITTED_HAZARD
 * and not XA_TX_STAGE_PREPARING! */
{XA_TX_STAGE_PREPARING,        XA_TX_STAGE_PREPARING,        XA_TX_STAGE_COMMITTED_HAZARD,XA_TX_STAGE_COMMITTED, "PREPARING", EXTRUE},
{XA_TX_STAGE_COMMITTING,       XA_TX_STAGE_COMMITTING,       XA_TX_STAGE_COMMITTED_HAZARD,XA_TX_STAGE_COMMITTED, "COMMITTING", EXFALSE},
/* Left for compliance: */
{XA_TX_STAGE_COMMITTED_HAZARD, XA_TX_STAGE_COMMITTED_HAZARD, XA_TX_STAGE_COMMITTED_HAZARD,XA_TX_STAGE_COMMITTED_HAZARD, "COMMITTED_HAZARD", EXFALSE},
/* Left for compliance: */
{XA_TX_STAGE_COMMITTED_HEURIS, XA_TX_STAGE_COMMITTED_HEURIS, XA_TX_STAGE_COMMITTED_HEURIS,XA_TX_STAGE_COMMITTED_HEURIS, "COMMITTED_HEURIS", EXFALSE},
/* Left for compliance: */
{XA_TX_STAGE_COMMITTED,        XA_TX_STAGE_COMMITTED,        XA_TX_STAGE_COMMITTED,       XA_TX_STAGE_COMMITTED, "COMMITTED", EXFALSE},
{EXFAIL, 0, 0, 0, "FAIL"}
};

/**
 * Needs new table: state-to-tpreturn code mapper. 
 */
expublic txstate2tperrno_t G_txstage2tperrno[] =
{
{XA_TX_STAGE_NULL,             XA_OP_COMMIT, TPESYSTEM},
{XA_TX_STAGE_ACTIVE,           XA_OP_COMMIT, TPESYSTEM},
{XA_TX_STAGE_ABORTING,         XA_OP_COMMIT, TPEHAZARD},
{XA_TX_STAGE_ABORTED_HAZARD,   XA_OP_COMMIT, TPEHAZARD},
{XA_TX_STAGE_ABORTED_HEURIS,   XA_OP_COMMIT, TPEHEURISTIC},
/* Extra check on exit, if abort was requested, then translate to SUCCEED */
{XA_TX_STAGE_ABORTED,          XA_OP_COMMIT, TPEABORT},
{XA_TX_STAGE_PREPARING,        XA_OP_COMMIT, TPESYSTEM},
{XA_TX_STAGE_COMMITTING,       XA_OP_COMMIT, TPEHAZARD},
{XA_TX_STAGE_COMMITTED_HAZARD, XA_OP_COMMIT, TPEHAZARD},
{XA_TX_STAGE_COMMITTED_HEURIS, XA_OP_COMMIT, TPEHEURISTIC},
{XA_TX_STAGE_COMMITTED,        XA_OP_COMMIT, EXSUCCEED},

{XA_TX_STAGE_NULL,             XA_OP_ROLLBACK, TPESYSTEM},
{XA_TX_STAGE_ACTIVE,           XA_OP_ROLLBACK, TPESYSTEM},
{XA_TX_STAGE_ABORTING,         XA_OP_ROLLBACK, TPEHAZARD},
{XA_TX_STAGE_ABORTED_HAZARD,   XA_OP_ROLLBACK, TPEHAZARD},
{XA_TX_STAGE_ABORTED_HEURIS,   XA_OP_ROLLBACK, TPEHEURISTIC},
/* Extra check on exit, if abort was requested, then translate to SUCCEED */
{XA_TX_STAGE_ABORTED,          XA_OP_ROLLBACK, EXSUCCEED},
{XA_TX_STAGE_PREPARING,        XA_OP_ROLLBACK, TPESYSTEM},
{XA_TX_STAGE_COMMITTING,       XA_OP_ROLLBACK, TPEHAZARD},
{XA_TX_STAGE_COMMITTED_HAZARD, XA_OP_ROLLBACK, TPEHAZARD},
{XA_TX_STAGE_COMMITTED_HEURIS, XA_OP_ROLLBACK, TPEHEURISTIC},
{XA_TX_STAGE_COMMITTED,        XA_OP_ROLLBACK, TPEHEURISTIC},

{EXFAIL}
};

/*---------------------------Prototypes---------------------------------*/

/**
 * Get next RM status/txstage by current operation.
 * @param txstage - current tx stage
 * @param rmstatus - current RM status
 * @param op - operation done
 * @param op_retcode - operation return code
 * @return NULL or transition descriptor
 */
expublic rmstatus_driver_t* xa_status_get_next_by_op(short txstage, char rmstatus, 
                                                    int op, int op_retcode)
{
    rmstatus_driver_t *ret=NULL;
    
    switch (txstage)
    {
        case XA_TX_STAGE_PREPARING:
            ret = M_rm_status_driver_preparing;
            break;
        case XA_TX_STAGE_COMMITTING:
            ret = M_rm_status_driver_committing;
            break;    
        case XA_TX_STAGE_ABORTING:
            ret = M_rm_status_driver_aborting;
            break;
        default:
            return NULL;
    }
    
    while (EXFAIL!=ret->txstage)
    {
        if (ret->txstage == txstage &&
                ret->rmstatus == rmstatus &&
                ret->op == op &&
                op_retcode>=ret->min_retcode && 
                op_retcode<=ret->max_retcode
            )
        {
            break;
        }
        ret++;
    } 
    
    if (EXFAIL==ret->txstage)
    {
        ret=NULL;
    }
    
    return ret;
}

/**
 * If we got NOP on current stage action for RM, then we lookup
 * this table, to get it's wote for next txstage.
 * @param txstage - current tx stage
 * @param next_rmstatus - RM status (after NOP...)
 * @return NULL or transition descriptor
 */
expublic rmstatus_driver_t* xa_status_get_next_by_new_status(short   txstage, 
                                                    char next_rmstatus)
{
    rmstatus_driver_t *ret=NULL;
    
    switch (txstage)
    {
        case XA_TX_STAGE_PREPARING:
            ret = M_rm_status_driver_preparing;
            break;
        case XA_TX_STAGE_COMMITTING:
            ret = M_rm_status_driver_committing;
            break;    
        case XA_TX_STAGE_ABORTING:
            ret = M_rm_status_driver_aborting;
            break;
        default:
            return NULL;
    }
    
    while (EXFAIL!=ret->txstage)
    {
        if (ret->txstage == txstage &&
                ret->next_rmstatus == next_rmstatus
            )
        {
            break;
        }
        ret++;
    }
    
    if (EXFAIL==ret->txstage)
    {
        ret=NULL;
    }
    
    return ret;
}

/**
 * Get operation to be done for current RM in given state
 * @param txstage current tx stage
 * @param rmstatus curren RM status
 * @return operation to be done or NOP
 */
expublic int xa_status_get_op(short txstage, char rmstatus)
{
    txaction_driver_t *ret = G_txaction_driver;
    
    while (EXFAIL!=ret->txstage)
    {
        if (ret->txstage == txstage &&
                ret->rmstatus == rmstatus
            )
        {
            break;
        }
        ret++;
    }
    
    if (EXFAIL!=ret->txstage)
    {
        return ret->op;
    }
    else
    {
        return XA_OP_NOP;
    }
}

/**
 * Get stage descriptor
 * @param txstage
 * @return NULL or stage descriptor
 */
expublic txstage_descriptor_t* xa_stage_get_descr(short txstage)
{
    txstage_descriptor_t* ret = G_state_descriptor;
    
    while (EXFAIL!=ret->txstage)
    {
        if (ret->txstage == txstage)
        {
            break;
        }
        ret++;
    }
    
    if (EXFAIL==ret->txstage)
        ret = NULL;
    
    return ret;
}

/**
 * Translate stage to tperrno
 * @param txstage
 * @return 
 */
expublic int xa_txstage2tperrno(short txstage, int master_op)
{
    txstate2tperrno_t* ret = G_txstage2tperrno;
    
    while (EXFAIL!=ret->txstage)
    {
        if (ret->txstage == txstage &&
                ret->master_op == master_op
                )
        {
            break;
        }
        ret++;
    }
    
    if (EXFAIL==ret->txstage)
        return TPESYSTEM;
    
    return ret->tpe;
}

/* vim: set ts=4 sw=4 et smartindent: */
