/**
 * @brief Tmsrv server - transaction monitor
 *   After that log transaction to hash & to disk for tracking the stuff...
 *   TODO: We should have similar control like "TP_COMMIT_CONTROL" -
 *   either return after stuff logged or after really commit completed.
 *   Error handling:
 *   - System errors we will track via ATMI interface error functions
 *   - XA errors will be tracked via XA error interface
 *
 * @file tmapi.c
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
#include <exbase64.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define XID_RECOVER_BLOCK_SZ        1000
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/******************************************************************************/
/*                               TP API COMMANDS                              */
/******************************************************************************/

/**
 * TP API
 * TP entry for abort.
 * @param p_ub
 * @return 
 */
expublic int tm_tpabort(UBFH *p_ub)
{
    int ret = EXSUCCEED;
    atmi_xa_tx_info_t xai;
    atmi_xa_log_t *p_tl = NULL;
    
    NDRX_LOG(log_debug, "tm_tpabort() called");
    
    /* 1. get transaction from hash */
    if (EXSUCCEED!=atmi_xa_read_tx_info(p_ub, &xai, XA_TXINFO_NOBTID))
    {
        NDRX_LOG(log_error, "Failed to read transaction data");
        /* - will assume that completed OK!
        atmi_xa_set_error_msg(p_ub, TPESYSTEM, NDRX_XA_ERSN_INVPARAM, 
                "Invalid transaction data (missing fields)");
        */
        atmi_xa_set_error_fmt(p_ub, TPEINVAL, NDRX_XA_ERSN_NOTX, 
                "Transaction with xid [%s] not logged - probably was tout+abort", 
                xai.tmxid);
        ret=EXSUCCEED;
        goto out;
    }
    
    /* read tx from hash */
    if (NULL==(p_tl = tms_log_get_entry(xai.tmxid, NDRX_LOCK_WAIT_TIME)))
    {
        NDRX_LOG(log_error, "Transaction with xid [%s] not logged", 
                xai.tmxid);
        atmi_xa_set_error_fmt(p_ub, TPEPROTO, NDRX_XA_ERSN_NOTX, 
                "Transaction with xid [%s] not logged", xai.tmxid);
        EXFAIL_OUT(ret);
    }
    
    /* Switch the state to aborting... */

    if (EXSUCCEED!=tms_log_stage(p_tl, XA_TX_STAGE_ABORTING))    
    {
        NDRX_LOG(log_error, "Failed to log ABORTING stage!");
        atmi_xa_set_error_fmt(p_ub, TPESYSTEM, NDRX_XA_ERSN_LOGFAIL, 
                "Cannot log [%s] tx!", xai.tmxid);
        EXFAIL_OUT(ret);
    }
    
    /* Call internal version of abort */
    if (EXSUCCEED!=(ret=tm_drive(&xai, p_tl, XA_OP_ROLLBACK, EXFAIL, 0L)))
    {
        atmi_xa_set_error_fmt(p_ub, ret, NDRX_XA_ERSN_RMCOMMITFAIL, 
                "Distributed transaction process did not finish completely");
        ret = EXSUCCEED;
    }
        
out:
        
    return ret;
}

/**
 * TP API
 * Commit the transaction (master entry)
 * Transaction initiator ask to commit the global transaction.
 * @param p_ub
 * @return 
 */
expublic int tm_tpcommit(UBFH *p_ub)
{
    int ret = EXSUCCEED;
    atmi_xa_tx_info_t xai;
    atmi_xa_log_t *p_tl = NULL;
    int do_abort = EXFALSE;
    long tmflags;
    NDRX_LOG(log_debug, "tm_tpcommit() called");
    
    /* 1. get transaction from hash */
    if (EXSUCCEED!=atmi_xa_read_tx_info(p_ub, &xai, XA_TXINFO_NOBTID))
    {
        NDRX_LOG(log_error, "Failed to read transaction data");
        atmi_xa_set_error_msg(p_ub, TPESYSTEM, NDRX_XA_ERSN_INVPARAM, 
                "Invalid transaction data (missing fields)");
        EXFAIL_OUT(ret);
    }
    
    /* tx.h support: */
    if (EXSUCCEED!=Bget(p_ub, TMTXFLAGS, 0, (char *)&tmflags, 0L))
    {
        NDRX_LOG(log_error, "Failed to get TMTXFLAGS: %s", Bstrerror(Berror));
        atmi_xa_set_error_msg(p_ub, TPESYSTEM, NDRX_XA_ERSN_INVPARAM, 
                "Failed to read TMTXFLAGS");
        EXFAIL_OUT(ret);
    }
    
    /* read tx from hash */
    if (NULL==(p_tl = tms_log_get_entry(xai.tmxid, NDRX_LOCK_WAIT_TIME)))
    {
        NDRX_LOG(log_error, "Transaction with xid [%s] not logged", 
                xai.tmxid);
        /* - Will regsiter as rolled back!!!
        atmi_xa_set_error_fmt(p_ub, TPEPROTO, NDRX_XA_ERSN_NOTX, 
                "Transaction with xid [%s] not logged", xai.tmxid);
         */
        atmi_xa_set_error_fmt(p_ub, TPEABORT, NDRX_XA_ERSN_NOTX, 
                "Transaction with xid [%s] not logged - probably was tout+abort", xai.tmxid);
        EXFAIL_OUT(ret);
    }
    
    /* Open log file 
     * - now we open the file at the start of the transaction.
    if (EXSUCCEED!=tms_open_logfile(p_tl, "w"))
    {
        NDRX_LOG(log_error, "Failed to open log file");
        atmi_xa_set_error_msg(p_ub, TPESYSTEM, NDRX_XA_ERSN_INVPARAM, 
                "Failed to open log file");
        do_abort = EXTRUE;
        EXFAIL_OUT(ret);
    }
    */
    
    /* Log that we start commit... */
    if (EXSUCCEED!=tms_log_stage(p_tl, XA_TX_STAGE_PREPARING))    
    {
        NDRX_LOG(log_error, "Failed to log tx - abort!");
        do_abort = EXTRUE;
        EXFAIL_OUT(ret);
    }
    
    /* Drive - it will auto-rollback if needed... */
    if (EXSUCCEED!=(ret=tm_drive(&xai, p_tl, XA_OP_COMMIT, EXFAIL, tmflags)))
    {
        atmi_xa_set_error_msg(p_ub, ret, NDRX_XA_ERSN_RMCOMMITFAIL, 
                "Transaction did not complete fully");
        ret=EXFAIL;
        goto out;
    }
    
out:
                
    /* Rollback the work done! */
    if (do_abort)
    {
        NDRX_LOG(log_warn, "About to rollback transaction!");
        
        tms_log_stage(p_tl, XA_TX_STAGE_ABORTING);

        /* Call internal version of abort */
        if (EXSUCCEED!=(ret=tm_drive(&xai, p_tl, XA_OP_COMMIT, EXFAIL, 0L)))
        {
            atmi_xa_override_error(p_ub, ret);
            ret=EXFAIL;
        }
    }

    return ret;
}

/**
 * TP API
 * Start new XA transaction
 * @param p_ub
 * @return 
 */
expublic int tm_tpbegin(UBFH *p_ub)
{
    int ret=EXSUCCEED;
    XID xid; /* handler for new XID */
    atmi_xa_tx_info_t xai;
    int do_rollback=EXFALSE;
    char xid_str[NDRX_XID_SERIAL_BUFSIZE];
    long txtout;
    long tmflags;
    long btid = EXFAIL;
    NDRX_LOG(log_debug, "tm_tpbegin() called");
    
    if (EXSUCCEED!=Bget(p_ub, TMTXFLAGS, 0, (char *)&tmflags, 0L))
    {
        NDRX_LOG(log_error, "Failed to read TMTXFLAGS!");
        EXFAIL_OUT(ret);   
    }
    
    atmi_xa_new_xid(&xid);
    
    /* we should start new transaction... (only if static...) 
    if (!(tmflags & TMTXFLAGS_DYNAMIC_REG))
    {
        if (EXSUCCEED!=(ret = atmi_xa_start_entry(&xid, 0, EXFALSE)))
        {
            NDRX_LOG(log_error, "Failed to start new transaction!");
            atmi_xa_set_error_fmt(p_ub, TPETRAN, NDRX_XA_ERSN_NONE, 
                    "Failed to start new transaction, "
                    "xa error: %d [%s]", ret, atmi_xa_geterrstr(ret));
            goto out;
        }
        
        xai.tmknownrms[0] = G_atmi_env.xa_rmid;
        xai.tmknownrms[1] = EXEOS;
    }
    else
    {
        
    }
    */
    
    /* mark that we know about this RM */
    xai.tmknownrms[0] = G_atmi_env.xa_rmid;
    xai.tmknownrms[1] = EXEOS;
    
    atmi_xa_serialize_xid(&xid, xid_str);
    
    /* load the XID into buffer */
    NDRX_STRCPY_SAFE(xai.tmxid, xid_str);
    xai.tmrmid = G_atmi_env.xa_rmid;
    xai.tmnodeid = G_atmi_env.our_nodeid;
    xai.tmsrvid = G_server_conf.srv_id;
    
    
    /* Currently time-out is handled only locally by TM */
    if (EXSUCCEED!=Bget(p_ub, TMTXTOUT, 0, (char *)&txtout, 0L) || 0>=txtout)
    {
        txtout = G_tmsrv_cfg.dflt_timeout;
        NDRX_LOG(log_debug, "TX tout defaulted to: %ld", txtout);
    }
    else
    {
        NDRX_LOG(log_debug, "TX tout: %ld", txtout);
    }
    
    /* Only for static...
    if (!(tmflags & TMTXFLAGS_DYNAMIC_REG))
    {
        if (EXSUCCEED!=(ret = atmi_xa_end_entry(&xid, TMSUCCESS)))
        {
            NDRX_LOG(log_error, "Failed to end XA api!");
            atmi_xa_set_error_fmt(p_ub, TPETRAN, NDRX_XA_ERSN_NONE, 
                    "Failed to start end transaction, "
                    "xa error: %d [%s]", ret, atmi_xa_geterrstr(ret));
            goto out;
        }
    }
    */
    
    if (EXSUCCEED!=atmi_xa_load_tx_info(p_ub, &xai))
    {
        NDRX_LOG(log_error, "Failed to load tx info!");
        atmi_xa_set_error_fmt(p_ub, TPETRAN, NDRX_XA_ERSN_NONE, 
                    "Failed to return tx info!");
        do_rollback = EXTRUE;
        ret=EXFAIL;
        goto out;
    }
    
    /* Log into journal */
    if (EXSUCCEED!=tms_log_start(&xai, txtout, tmflags, &btid))
    {
        NDRX_LOG(log_error, "Failed to log the transaction!");
        atmi_xa_set_error_fmt(p_ub, TPETRAN, NDRX_XA_ERSN_LOGFAIL, 
                    "Failed to log the transaction!");
        do_rollback = EXTRUE;
        ret=EXFAIL;
        goto out;
    }
    
    /* TID to log */
    if (EXSUCCEED!=Bchg(p_ub, TMTXBTID, 0, (char *)&btid, 0L))
    {
        NDRX_LOG(log_error, "Failed to set TMTXBTID: %s", Bstrerror(Berror));
        atmi_xa_set_error_fmt(p_ub, TPESYSTEM, NDRX_XA_ERSN_UBFERR, 
                    "Failed to set TMTXBTID: %s", Bstrerror(Berror));
        
        /* How about closing log the file? */
        do_rollback = EXTRUE;
        ret=EXFAIL;
        goto out;
    }
    
out:
    
    /* We should abort the transaction right now */
    if (do_rollback)
    {
        ret = tm_rollback_local(p_ub, &xai, btid);
    }

    return ret;
}

/******************************************************************************/
/*                         TRANSACTION BRANCH COMMANDS                        */
/******************************************************************************/

/**
 * Register resource manager under given transaction
 * @param p_ub
 * @return 
 */
expublic int tm_tmregister(UBFH *p_ub)
{
    int ret = EXSUCCEED;
    short   callerrm;
    int is_already_logged = EXFALSE;
    atmi_xa_tx_info_t xai;
    long tmflags = 0;
    long btid=EXFAIL;
    
    
    /* TODO: Get flags! */
    
    if (EXSUCCEED!=Bget(p_ub, TMCALLERRM, 0, (char *)&callerrm, 0L))
    {
        atmi_xa_set_error_fmt(p_ub, TPESYSTEM, NDRX_XA_ERSN_INVPARAM, 
                    "Missing TMCALLERRM field: %s!", Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=atmi_xa_read_tx_info(p_ub, &xai, XA_TXINFO_NOBTID))
    {
        atmi_xa_set_error_fmt(p_ub, TPESYSTEM, NDRX_XA_ERSN_INVPARAM, 
                    "Failed to read transaction info!");
        EXFAIL_OUT(ret);
    }
    
    /* read BTID (if have one in their side) */
    
    if (EXSUCCEED!=Bget(p_ub, TMTXBTID, 0, (char *)&btid, NULL) && Berror!=BNOTPRES)
    {
        NDRX_LOG(log_error, "Failed to get TMTXBTID: %s", Bstrerror(Berror));
        
        atmi_xa_set_error_fmt(p_ub, TPESYSTEM, NDRX_XA_ERSN_UBFERR, 
                     "Failed to get TMTXBTID: %s", Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=Bget(p_ub, TMTXFLAGS, 0, (char *)&tmflags, 0L))
    {
        atmi_xa_set_error_fmt(p_ub, TPESYSTEM, NDRX_XA_ERSN_INVPARAM, 
                    "Missing TMTXFLAGS in buffer");
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=tms_log_addrm(&xai, callerrm, &is_already_logged, &btid, tmflags))
    {
        atmi_xa_set_error_fmt(p_ub, TPESYSTEM, NDRX_XA_ERSN_RMLOGFAIL, 
                    "Failed to log new RM!");
        EXFAIL_OUT(ret);
    }
    
    if (is_already_logged)
    {
        tmflags|=TMTXFLAGS_RMIDKNOWN;
    }
    
    /* return new TID */
    if (!Bpres(p_ub, TMTXBTID, 0) &&
            EXSUCCEED!=Bchg(p_ub, TMTXBTID, 0, (char *)&btid, 0L))
    {
        atmi_xa_set_error_fmt(p_ub, TPESYSTEM, NDRX_XA_ERSN_UBFERR, 
                    "Failed to set TMTXBTID!");
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=Bchg(p_ub, TMTXFLAGS, 0, (char *)&tmflags, 0L))
    {
        atmi_xa_set_error_fmt(p_ub, TPESYSTEM, NDRX_XA_ERSN_UBFERR, 
                    "Failed to set TMTXFLAGS!");
        EXFAIL_OUT(ret);
    }
    
out:
    return ret; 
}

/**
 * Report transaction status of the branch
 * @param p_ub
 * @return 
 */
expublic int tm_rmstatus(UBFH *p_ub)
{
    int ret = EXSUCCEED;
    short   callerrm;
    int is_already_logged = EXFALSE;
    atmi_xa_tx_info_t xai;
    long tmflags = 0;
    long btid=EXFAIL;
    char rmstatus;
    
    
    /* TODO: Get flags! */
    
    if (EXSUCCEED!=Bget(p_ub, TMCALLERRM, 0, (char *)&callerrm, 0L))
    {
        atmi_xa_set_error_fmt(p_ub, TPESYSTEM, NDRX_XA_ERSN_INVPARAM, 
                    "Missing TMCALLERRM field: %s!", Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=atmi_xa_read_tx_info(p_ub, &xai, XA_TXINFO_NOBTID))
    {
        atmi_xa_set_error_fmt(p_ub, TPESYSTEM, NDRX_XA_ERSN_INVPARAM, 
                    "Failed to read transaction info!");
        EXFAIL_OUT(ret);
    }
    
    /* read BTID (if have one in their side) */
    
    if (EXSUCCEED!=Bget(p_ub, TMTXBTID, 0, (char *)&btid, NULL))
    {
        atmi_xa_set_error_fmt(p_ub, TPESYSTEM, NDRX_XA_ERSN_INVPARAM, 
                    "Missing TMTXBTID!");
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=Bget(p_ub, TMTXRMSTATUS, 0, (char *)&rmstatus, 0L))
    {
        atmi_xa_set_error_fmt(p_ub, TPESYSTEM, NDRX_XA_ERSN_INVPARAM, 
                    "Missing TMTXRMSTATUS in buffer");
        EXFAIL_OUT(ret);
    }

    if (EXSUCCEED!=tms_log_chrmstat(xai, callerrm, 
        btid, rmstatus, p_ub))
    {
        atmi_xa_set_error_fmt(p_ub, TPESYSTEM, NDRX_XA_ERSN_RMLOGFAIL, 
                    "Failed to log new RM!");
        EXFAIL_OUT(ret);
    }
    
    if (is_already_logged)
    {
        tmflags|=TMTXFLAGS_RMIDKNOWN;
    }
    
    /* return new TID */
    if (!Bpres(p_ub, TMTXBTID, 0) &&
            EXSUCCEED!=Bchg(p_ub, TMTXBTID, 0, (char *)&btid, 0L))
    {
        atmi_xa_set_error_fmt(p_ub, TPESYSTEM, NDRX_XA_ERSN_UBFERR, 
                    "Failed to set TMTXBTID!");
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=Bchg(p_ub, TMTXFLAGS, 0, (char *)&tmflags, 0L))
    {
        atmi_xa_set_error_fmt(p_ub, TPESYSTEM, NDRX_XA_ERSN_UBFERR, 
                    "Failed to set TMTXFLAGS!");
        EXFAIL_OUT(ret);
    }
    
out:
    return ret; 
}


/**
 * Do the internal prepare of transaction (request sent from other TM)
 * @param p_ub
 * @return 
 */
expublic int tm_tmprepare(UBFH *p_ub)
{
    int ret = EXSUCCEED;
    atmi_xa_tx_info_t xai;
    
    NDRX_LOG(log_debug, "tm_tmprepare called.");
    /* read xai from FB... */
    if (EXSUCCEED!=atmi_xa_read_tx_info(p_ub, &xai, 0))
    {
        atmi_xa_set_error_fmt(p_ub, TPESYSTEM, NDRX_XA_ERSN_INVPARAM, 
                    "Failed to read transaction info!");
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=(ret = tm_prepare_local(p_ub, &xai, xai.btid)))
    {
        EXFAIL_OUT(ret);
    }
    
out:
    return ret;
}

/**
 * Do the internal commit of transaction (request sent from other TM)
 * @param p_ub
 * @return 
 */
expublic int tm_tmcommit(UBFH *p_ub)
{
    int ret = EXSUCCEED;
    atmi_xa_tx_info_t xai;
    
    NDRX_LOG(log_debug, "tm_tmcommit called.");
    /* read xai from FB... */
    if (EXSUCCEED!=atmi_xa_read_tx_info(p_ub, &xai, 0))
    {
        atmi_xa_set_error_fmt(p_ub, TPESYSTEM, NDRX_XA_ERSN_INVPARAM, 
                    "Failed to read transaction info!");
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=(ret = tm_commit_local(p_ub, &xai, xai.btid)))
    {
        EXFAIL_OUT(ret);
    }
    
out:
    return ret;
}

/**
 * Local transaction branch abort command. Sent from Master TM
 * @param p_ub
 * @return 
 */
expublic int tm_tmabort(UBFH *p_ub)
{
    int ret = EXSUCCEED;
    atmi_xa_tx_info_t xai;
    
    NDRX_LOG(log_debug, "tm_tmabort called.");
    if (EXSUCCEED!=atmi_xa_read_tx_info(p_ub, &xai, 0))
    {
        atmi_xa_set_error_fmt(p_ub, TPESYSTEM, NDRX_XA_ERSN_INVPARAM, 
                    "Failed to read transaction info!");
        EXFAIL_OUT(ret);
    }
    
    /* read xai from FB... */
    if (EXSUCCEED!=(ret = tm_rollback_local(p_ub, &xai, xai.btid)))
    {
        EXFAIL_OUT(ret);
    }
    
out:
    return ret;
}

/******************************************************************************/
/*                         COMMAND LINE API                                   */
/******************************************************************************/
/**
 * Return list of transactions
 * @param p_ub
 * @param cd - call descriptor
 * @return 
 */
expublic int tm_tpprinttrans(UBFH *p_ub, int cd)
{
    int ret = EXSUCCEED;
    long revent;
    atmi_xa_log_list_t *tx_list;
    atmi_xa_log_list_t *el, *tmp;
    
    /* Get the lock! */
    tms_tx_hash_lock();
    
    tx_list = tms_copy_hash2list(COPY_MODE_FOREGROUND | COPY_MODE_BACKGROUND);
        
    LL_FOREACH_SAFE(tx_list,el,tmp)
    {
        
        /* Erase FB & setup the info there... */
        Bproj(p_ub, NULL); /* clear the FB! */
        if (EXSUCCEED!=tms_log_cpy_info_to_fb(p_ub, &(el->p_tl), EXTRUE))
        {
            EXFAIL_OUT(ret);
        }
        
        
        /* Bfprint(p_ub, stderr); */
        
        if (EXFAIL == tpsend(cd,
                            (char *)p_ub,
                            0L,
                            0,
                            &revent))
        {
            NDRX_LOG(log_error, "Send data failed [%s] %ld",
                                tpstrerror(tperrno), revent);
            EXFAIL_OUT(ret);
        }
        else
        {
            NDRX_LOG(log_debug,"sent ok");
        }
        
        LL_DELETE(tx_list, el);
        NDRX_FREE(el);
    }
    
out:
    /* TODO: might want to kill the list if FAIL!!!! */
    tms_tx_hash_unlock();

    return ret;
}

/**
 * Abort transaction.
 * @param p_ub
 * @param cd - call descriptor
 * @return 
 */
expublic int tm_aborttrans(UBFH *p_ub)
{
    int ret = EXSUCCEED;
    atmi_xa_log_t *p_tl;
    char tmxid[NDRX_XID_SERIAL_BUFSIZE+1];
    short tmrmid = EXFAIL;
    atmi_xa_tx_info_t xai;
    /* We should try to abort transaction
     Thus basically we need to lock the transaction on which we work on.
     Otherwise, we can conflict with background.
     */
    background_lock();
    
    if (EXSUCCEED!=Bget(p_ub, TMXID, 0, tmxid, 0L))
    {
        NDRX_LOG(log_error, "Failed to read TMXID: %s", 
                Bstrerror(Berror));
        atmi_xa_set_error_msg(p_ub, TPESYSTEM, 0, "Protocol error, missing TMXID!");
        EXFAIL_OUT(ret);
    }
    
    /* optional */
    Bget(p_ub, TMTXRMID, 0, (char *)&tmrmid, 0L);
    
    /* Lookup for log. And then try to abort... */
    if (NULL==(p_tl = tms_log_get_entry(tmxid, NDRX_LOCK_WAIT_TIME)))
    {
        /* Generate error */
        atmi_xa_set_error_fmt(p_ub, TPEMATCH, 0, "Transaction not found (%s)!", 
                tmxid);
        EXFAIL_OUT(ret);
    }
    
    /* Try to abort stuff... */
    
    /* init xai for tl... */
    XA_TX_COPY((&xai), p_tl);
    
    NDRX_LOG(log_debug, "Got RMID: [%hd]", tmrmid);
    
    /* Switch transaction to aborting (if not already) */
    tms_log_stage(p_tl, XA_TX_STAGE_ABORTING);
    if (EXSUCCEED!=(ret=tm_drive(&xai, p_tl, XA_OP_ROLLBACK, tmrmid, 0L)))
    {
        atmi_xa_set_error_fmt(p_ub, ret, NDRX_XA_ERSN_RMERR, 
                "Failed to abort transaction");
        ret=EXFAIL;
        goto out;
    }
    
out:
    background_unlock();

    return ret;
}

/**
 * Get transaction status
 * @param p_ub request buffer
 * @return 
 */
expublic int tm_status(UBFH *p_ub)
{
    int ret = EXSUCCEED;
    atmi_xa_log_t *p_tl = NULL;
    char tmxid[NDRX_XID_SERIAL_BUFSIZE+1];
    short tmrmid = EXFAIL;
    
    /* We should try to abort transaction
     Thus basically we need to lock the transaction on which we work on.
     Otherwise, we can conflict with background.
     */
    background_lock();
    
    if (EXSUCCEED!=Bget(p_ub, TMXID, 0, tmxid, 0L))
    {
        NDRX_LOG(log_error, "Failed to read TMXID: %s", 
                Bstrerror(Berror));
        atmi_xa_set_error_msg(p_ub, TPESYSTEM, 0, "Protocol error, missing TMXID!");
        EXFAIL_OUT(ret);
    }
    
    /* optional */
    Bget(p_ub, TMTXRMID, 0, (char *)&tmrmid, 0L);
    
    /* Lookup for log. And then try to abort... */
    if (NULL==(p_tl = tms_log_get_entry(tmxid, NDRX_LOCK_WAIT_TIME)))
    {
        /* Generate error */
        atmi_xa_set_error_fmt(p_ub, TPEMATCH, 0, "Transaction not found (%s)!", 
                tmxid);
        EXFAIL_OUT(ret);
    }
    
    /* Return full status of the transaction... */
    if (EXSUCCEED!=tms_log_cpy_info_to_fb(p_ub, p_tl, EXFALSE))
    {
        EXFAIL_OUT(ret);
    }
    
out:
    
    if (NULL!=p_tl)
    {
        tms_unlock_entry(p_tl);
    }
        
    background_unlock();

    return ret;
}

/**
 * Commit transaction.
 * @param p_ub
 * @param cd - call descriptor
 * @return 
 */
expublic int tm_committrans(UBFH *p_ub)
{
    int ret = EXSUCCEED;
    atmi_xa_log_t *p_tl;
    char tmxid[NDRX_XID_SERIAL_BUFSIZE+1];
    atmi_xa_tx_info_t xai;
    /* We should try to commit transaction
     Thus basically we need to lock the transaction on which we work on.
     Otherwise, we can conflict with background.
     */
    background_lock();
    
    if (EXSUCCEED!=Bget(p_ub, TMXID, 0, tmxid, 0L))
    {
        NDRX_LOG(log_error, "Failed to read TMXID: %s", 
                Bstrerror(Berror));
        atmi_xa_set_error_msg(p_ub, TPESYSTEM, 0, "Protocol error, missing TMXID!");
        EXFAIL_OUT(ret);
    }
    
    /* Lookup for log. And then try to commit... */
    if (NULL==(p_tl = tms_log_get_entry(tmxid, NDRX_LOCK_WAIT_TIME)))
    {
        /* Generate error */
        atmi_xa_set_error_fmt(p_ub, TPEMATCH, 0, "Transaction not found (%s)!", 
                tmxid);
        EXFAIL_OUT(ret);
    }
    
    /* Try to commit stuff, only if stage is prepared...! */
    if (XA_TX_STAGE_COMMITTING!=p_tl->txstage)
    {
        atmi_xa_set_error_fmt(p_ub, TPEINVAL, 0, 
                "Transaction not in PREPARED stage!");
        /* we shall unlock the transaction here! */
        tms_unlock_entry(p_tl);
        EXFAIL_OUT(ret);
    }
    
    /* init xai for tl... */
    XA_TX_COPY((&xai), p_tl);
    
    if (EXSUCCEED!=(ret=tm_drive(&xai, p_tl, XA_OP_COMMIT, EXFAIL, 0L)))
    {
        atmi_xa_set_error_fmt(p_ub, ret, NDRX_XA_ERSN_RMCOMMITFAIL, 
                "Failed to commit transaction!");
        ret=EXFAIL;
        goto out;
    }
    
out:
    background_unlock();

    return ret;
}

/**
 * Return list of in doubt local transactions.
 * This returns info directly from RM
 * @param p_ub
 * @param cd - call descriptor
 * @return 
 */
expublic int tm_recoverlocal(UBFH *p_ub, int cd)
{
    int ret = EXSUCCEED;
    long revent;
    XID arraxid[XID_RECOVER_BLOCK_SZ];
    long flags = TMSTARTRSCAN;
    int i;
    char tmp[1024];
    size_t out_len = sizeof(tmp);
    
    while ((ret = atmi_xa_recover_entry(arraxid, XID_RECOVER_BLOCK_SZ, G_atmi_env.xa_rmid, 
            flags)) > 0)
    {       
        /* reset first */
        if (TMNOFLAGS!=flags)
        {
            flags = TMNOFLAGS;
        }
        
        NDRX_LOG(log_debug, "Recovered txns %d flags: %ld", ret, flags);
        
        for (i=0; i<ret; i++)
        {
            /* generate xid as base64 string? */
            ndrx_xa_base64_encode((unsigned char *)&arraxid[i], sizeof(arraxid[i]), 
                    &out_len, tmp);
            /* tmp[out_len] = EXEOS; */
            
            NDRX_LOG(log_debug, "Recovered xid: [%s]", tmp);
            
            if (EXSUCCEED!=Bchg(p_ub, TMXID, 0, tmp, 0))
            {
                NDRX_LOG(log_error, "Failed to set TMXID to [%s]", tmp);
                EXFAIL_OUT(ret);
            }
            
            if (EXFAIL == tpsend(cd,
                                (char *)p_ub,
                                0L,
                                0,
                                &revent))
            {
                NDRX_LOG(log_error, "Send data failed [%s] %ld",
                                    tpstrerror(tperrno), revent);
                EXFAIL_OUT(ret);
            }
            else
            {
                NDRX_LOG(log_debug,"sent ok");
            }
        }
        
        if (ret < XID_RECOVER_BLOCK_SZ)
        {
            /* this is EOF according to the spec */
            break;
        }
        
    }
    
out:

    /* close the "cursor"  - no need we scan till end..
    atmi_xa_recover_entry(NULL, 0, G_atmi_env.xa_rmid, TMENDRSCAN);
     * */

    return ret;
}

/**
 * Perform local operation on single xid and send results back to server.
 * @param p_ub UBF buffer for connection
 * @param cd connection descriptor
 * @param cmd op code
 * @param xid ptr XID
 * @return EXSUCCEED/EXFAIL
 */
exprivate int tm_proclocal_single(UBFH *p_ub, int cd, char cmd, XID *xid)
{
    int ret = EXSUCCEED;
    char tmp[1024];
    size_t out_len = sizeof(tmp);
    long revent;
        
    atmi_xa_unset_error(p_ub);
    ndrx_TPunset_error();
    
    switch (cmd)
    {
        case ATMI_XA_COMMITLOCAL:
            ret = atmi_xa_commit_entry(xid, TMNOFLAGS);
            break;
        case ATMI_XA_ABORTLOCAL:
            ret = atmi_xa_rollback_entry(xid, TMNOFLAGS);
            break;
        case ATMI_XA_FORGETLOCAL:
            ret = atmi_xa_forget_entry(xid, TMNOFLAGS);
            break;
        default:
            NDRX_LOG(log_error, "Invalid Opcode: %c", cmd);
            EXFAIL_OUT(ret);
            break;
    }
    
    /* load the result in UBF */
    ndrx_TPset_error_ubf(p_ub);
    
    ret = EXSUCCEED;
    
    ndrx_xa_base64_encode((unsigned char *)xid, sizeof(*xid), 
                    &out_len, tmp);
    /* tmp[out_len] = EXEOS; */
            
    if (EXSUCCEED!=Bchg(p_ub, TMXID, 0, tmp, 0))
    {
        NDRX_LOG(log_error, "Failed to set TMXID to [%s]", tmp);
        EXFAIL_OUT(ret);
    }

    if (EXFAIL == tpsend(cd,
                        (char *)p_ub,
                        0L,
                        0,
                        &revent))
    {
        NDRX_LOG(log_error, "Send data failed [%s] %ld",
                            tpstrerror(tperrno), revent);
        EXFAIL_OUT(ret);
    }
    else
    {
        NDRX_LOG(log_debug,"sent ok");
    }
    
out:
    return ret;
}

/**
 * Process manually the local transactions.
 * Return the list of processed + status
 * @param cmd ATMI_XA_COMMITLOCAL / ATMI_XA_ABORTLOCAL / ATMI_XA_FORGETLOCAL
 * @param p_ub
 * @param cd - call descriptor
 * @return 
 */
expublic int tm_proclocal(char cmd, UBFH *p_ub, int cd)
{
    int ret = EXSUCCEED;
    
    XID one;
    char onestr[sizeof(XID)*2];
    long flags = TMSTARTRSCAN;
    XID arraxid[XID_RECOVER_BLOCK_SZ];
    int i;
    size_t out_len = 0;
    BFLDLEN len;
    
    /* if there is single tran, then process it, if not, then loop over */
    
    if (Bpres(p_ub, TMXID, 0))
    {
        NDRX_LOG(log_debug, "XID present -> process single");
        len = sizeof(onestr);
        if (EXSUCCEED!=Bget(p_ub, TMXID, 0, onestr, &len))
        {
            NDRX_LOG(log_error, "Failed to get TMXID: %s", Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
        
        ndrx_xa_base64_decode((unsigned char *)onestr, strlen(onestr), 
                &out_len, (char *)&one);
        
        if (EXSUCCEED!=tm_proclocal_single(p_ub, cd, cmd, &one))
        {
            NDRX_DUMP(log_error, "Failed to process local xid", &one, sizeof(one));
            EXFAIL_OUT(ret);
        }
            
    } /* we process one by one otherwise we get improper one by one process */
    else while ((ret = atmi_xa_recover_entry(arraxid, XID_RECOVER_BLOCK_SZ, G_atmi_env.xa_rmid, 
            flags)) > 0)
    {       
        /* reset first */
        if (TMNOFLAGS!=flags)
        {
            flags = TMNOFLAGS;
        }
        
        NDRX_LOG(log_debug, "Recovered txns %d flags: %ld", ret, flags);
        /*
        atmi_xa_recover_entry(NULL, 0, G_atmi_env.xa_rmid, TMENDRSCAN);
        */
        
        for (i=0; i<ret; i++)
        {   
            if (EXSUCCEED!=tm_proclocal_single(p_ub, cd, cmd, &arraxid[i]))
            {
                NDRX_DUMP(log_error, "Failed to process local xid", &arraxid[i], 
                        sizeof(arraxid[i]));
                EXFAIL_OUT(ret);
            }
        }
        
        if (ret < XID_RECOVER_BLOCK_SZ)
        {
            /* this is EOF according to the spec */
            break;
        }
        
    }
    
out:

    return ret;
}






/* vim: set ts=4 sw=4 et smartindent: */
