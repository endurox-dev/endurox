/**
 * @brief Distributed Transaction Processing:
 *      The TX (Transaction Demarcation) Specification implementation X/Open
 *
 * @file tx.c
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <dlfcn.h>

#include <atmi.h>
#include <atmi_tls.h>
#include <ndrstandard.h>
#include <ndebug.h>
#include <ndrxd.h>
#include <ndrxdcmn.h>
#include <userlog.h>
#include <tx.h>
#include <Exfields.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Map the tperror to tx error
 * @param dbg debug msg
 * @param tpret Tp return code
 * @param allow_error is TX_ERROR code allowed?
 * @return TX_* error code
 */
exprivate int tx_map_error1(const char *dbg, int tpret, int allow_error)
{
    int ret = TX_FAIL;
    int tperr;
    
    if (0==tpret)
    {
        tperr = EXSUCCEED;
    }
    else
    {
        tperr = tperrno;
    }
    
    NDRX_LOG(log_debug, "tpret=%d tperr=%d", tpret, tperr);
    
    switch (tperr)
    {
        case 0:
            ret = TX_OK;
            break;
        case TPEPROTO:
            ret = TX_PROTOCOL_ERROR;
            break;
        case TPESVCERR:
        case TPETIME:
            if (allow_error)
            {
                ret = TX_ERROR;
            }
            else
            {
                ret = TX_FAIL;
            }
            break;
        case TPESYSTEM:
        case TPEOS:
        case TPERMERR:
        case TPEINVAL:
            ret = TX_FAIL;
            break;
        case TPEHAZARD:
            ret = TX_HAZARD;
            break;
        case TPEHEURISTIC:
            ret = TX_MIXED;
            break;
        case TPEMATCH:
        case TPEABORT:
            ret = TX_ROLLBACK;
            break;
    }
    
    if (TX_OK==ret)
    {
        NDRX_LOG(log_debug, "%s: TX_OK", dbg);
    }
    else
    {
        NDRX_LOG(log_error, "%s tp error %d mapped to tx %d: %s", dbg, 
                tperr, ret, tpstrerror(tperr));
    }
    
    return ret;
}

/**
 * Begin transaction
 * @return TX_ return code
 */
expublic int tx_begin(void)
{
    int ret;
    ATMI_TLS_ENTRY;
    
    ret = tpbegin(G_atmi_tls->tx_transaction_timeout, 0);
    
    /* map tp error to tx */
    ret = tx_map_error1(__func__, ret, EXTRUE);
    
    return ret;
   
}

/**
 * Close resource manager
 * @return TX_ return code
 */
expublic int tx_close(void)
{
    int ret;
    
    ret = tpclose();
    
    ret = tx_map_error1(__func__, ret, EXTRUE);
    
    return ret;
}

/**
 * Commit transaction, chain the next transaction if needed.
 * If chained mode is set for transactions, start new TRAN if possible.
 * If not then append errors with NO_BEGIN code (i.e. chain start failed).
 * @return TX_ return code
 */
expublic int tx_commit(void)
{
    int ret, ret2;
    long flags = 0;
    ATMI_TLS_ENTRY;
    
    if (TX_COMMIT_DECISION_LOGGED==G_atmi_tls->tx_commit_return)
    {
        flags|=TPTXCOMMITDLOG;
    }
    
    ret = tpcommit(flags);
    
    ret = tx_map_error1(__func__, ret, EXFALSE);
    
    /* chain the next transaction if required */
    
    if (TX_CHAINED==G_atmi_tls->tx_transaction_control)
    {
        if (TX_PROTOCOL_ERROR == ret ||  TX_FAIL==ret)
            
        {
            NDRX_LOG(log_error, "Fatal error cannot chain tx");
        }
        else
        {
            ret2 = tpbegin(G_atmi_tls->tx_transaction_timeout, 0);
            ret2 = tx_map_error1("tx_commit next tran begin: ", ret2, EXTRUE);
            
            /* if fail + fail base... */
            if (TX_OK!=ret2)
            {
                ret2 += TX_NO_BEGIN;
            }
        }
        
        ret = ret2;
    }
    
    NDRX_LOG(log_debug, "returns %d", ret);
    return ret;
}

/**
 * Fill in transaction info block
 * @param txinfo
 * @return TX_OK/
 */
expublic int tx_info(TXINFO * txinfo)
{
    int ret = TX_OK; /* not in tran mode */
    UBFH *p_ub = NULL;
    short   txstage;  /* In what state we are */
    ATMI_TLS_ENTRY;
    
    txinfo->transaction_control = G_atmi_tls->tx_transaction_control;
    txinfo->transaction_timeout = G_atmi_tls->tx_transaction_timeout;
    txinfo->when_return = G_atmi_tls->tx_commit_return;
    
    if (!G_atmi_tls->G_atmi_xa_curtx.txinfo)
    {
        txinfo->xid.formatID = EXFAIL;
        goto out;
    }
    else
    {
        ret = 1; /* we are in transaction */
        txinfo->transaction_state = TX_ACTIVE;
        /* copy off xid */
        atmi_xa_deserialize_xid(G_atmi_tls->G_atmi_xa_curtx.txinfo->tmxid, &G_atmi_tls->xid);
        
        /* check are we marked for rollback? */
        
        if (G_atmi_tls->G_atmi_xa_curtx.txinfo->tmtxflags & TMTXFLAGS_IS_ABORT_ONLY)
        {
            /* is it rollback only? */
            txinfo->transaction_state = TX_ROLLBACK_ONLY;
        }
        else
        {
            /* check with TM, what actually status we have? */
            if (NULL==(p_ub=atmi_xa_call_tm_generic(ATMI_XA_STATUS, EXFALSE, EXFAIL, 
                G_atmi_tls->G_atmi_xa_curtx.txinfo, 0L)))
            {
                int tperr = tperrno;
                NDRX_LOG(log_error, "Tran info failed with: %d", tperr);
                
                if (TPEMATCH==tperr)
                {
                    NDRX_LOG(log_debug, "Not matched by TM -> TX_TIMEOUT_ROLLBACK_ONLY");
                    txinfo->transaction_state = TX_TIMEOUT_ROLLBACK_ONLY;
                    
                    /* switch tran state? */
                    G_atmi_tls->G_atmi_xa_curtx.txinfo->tmtxflags !=TMTXFLAGS_IS_ABORT_ONLY;
                }
                else
                {
                    /* return the error if */
                    ret = tx_map_error1(__func__, EXFAIL, EXFALSE);
                }
                goto out;
            } /* if transaction not found.. */
            
            /* got transaction infos, check the state? 
             * if if is rollback state the return that...
             */
            
            if (EXSUCCEED!=Bget(p_ub, TMTXSTAGE, 0, (char *)&txstage, 0L))
            {
                NDRX_LOG(log_error, "Failed to get TMTXSTAGE from tmsrv: %s", 
                        Bstrerror(Berror));
                ret = TX_FAIL;
                goto out;
            }
            
            /* analyze the stage */
            NDRX_LOG(log_debug, "txstage=%hd", txstage)
            if (txstage >= XA_TX_STAGE_ABORTING  && txstage <=XA_TX_STAGE_ABORTING)
            {
                NDRX_LOG(log_warn, "TM is rolling back..!");
                txinfo->transaction_state = TX_ROLLBACK;
            }
        } /* if not local abort only.. */
    } /* if have global tx */
    
out:    
            
    if (NULL!=p_ub)
    {
        tpfree((char *)p_ub);
    }
        
    return ret;
}

/**
 * Open resource manager
 * @return TX_* errors
 */
expublic int tx_open(void)
{
    int ret;
    
    ret = tpopen();
    
    ret = tx_map_error1(__func__, ret, EXFALSE);
    
    return ret;
}

/**
 * Rollback transaction
 * @return TX_* errors
 */
expublic int tx_rollback(void)
{
    int ret, ret2;
    
    ret = tpabort(0L);
    
    ret = tx_map_error1(__func__, ret, EXFALSE);
    
    /* chain the next transaction if required */
    
    if (TX_CHAINED==G_atmi_tls->tx_transaction_control)
    {
        if (TX_PROTOCOL_ERROR == ret || TX_FAIL==ret)
            
        {
            NDRX_LOG(log_error, "Fatal error cannot chain tx");
        }
        else
        {
            ret2 = tpbegin(G_atmi_tls->tx_transaction_timeout, 0);
            ret2 = tx_map_error1("tx_commit next tran begin: ", ret2, EXTRUE);
            
            /* if fail + fail base... */
            if (TX_OK!=ret2)
            {
                ret2 += TX_NO_BEGIN;
            }
        }
        
        ret = ret2;
    }
    
    NDRX_LOG(log_debug, "returns %d", ret);
    return ret;
}

/**
 * Set when to return from commit. Either when transaction is completed
 * or when transaction is logged in persisted device for further background
 * completion.
 * The default is TX_COMMIT_COMPLETED.
 * @param cr TX_COMMIT_DECISION_LOGGED or TX_COMMIT_COMPLETED
 * @return TX_OK/TX_EINVAL/TX_PROTOCOL_ERROR
 */
expublic int tx_set_commit_return(COMMIT_RETURN cr)
{
    int ret = TX_OK;
    ATMI_TLS_ENTRY;
    
    if (!G_atmi_tls->G_atmi_xa_curtx.is_xa_open)
    {
        ret = TX_PROTOCOL_ERROR;
        goto out;
    }
    
    if (TX_COMMIT_DECISION_LOGGED!=cr &&
            TX_COMMIT_COMPLETED!=cr)
    {
        NDRX_LOG(log_error, "Invalid value: commit return %ld", (long)cr);
        ret = TX_EINVAL;
        goto out;
    }
    
    G_atmi_tls->tx_commit_return = cr;
    NDRX_LOG(log_info, "Commit return set to %ld", 
            (long)G_atmi_tls->tx_commit_return);
    
out:
    return ret;
}

/**
 * Set what commit/rollback will do:- either open next tran
 * or leave with out transaction
 * @param tc TX_UNCHAINED/TX_CHAINED 
 * @return TX_OK/TX_EINVAL
 */
expublic int tx_set_transaction_control(TRANSACTION_CONTROL tc)
{
    int ret = TX_OK;
    ATMI_TLS_ENTRY;
    
    if (!G_atmi_tls->G_atmi_xa_curtx.is_xa_open)
    {
        ret = TX_PROTOCOL_ERROR;
        goto out;
    }
    
    if (TX_UNCHAINED!=tc &&
            TX_CHAINED!=tc)    
    {
        NDRX_LOG(log_error, "Invalid value: transaction control %ld", (long)tc);
        ret = TX_EINVAL;
        goto out;
    }
    
    G_atmi_tls->tx_transaction_control = tc;
    NDRX_LOG(log_info, "Transaction control set to %ld", 
            (long)G_atmi_tls->tx_transaction_control);
    
out:
    return ret;
}

/**
 * Set seconds for the transaction to time out by transaction manager
 * @param tt timeout in seconds. 0 disables tout (i.e. uses maximum time by tmsrv)
 * @return TX_OK/TX_EINVAL/TX_PROTOCOL_ERROR/
 */
expublic int tx_set_transaction_timeout(TRANSACTION_TIMEOUT tt)
{
    int ret = TX_OK;
    ATMI_TLS_ENTRY;
    
    if (!G_atmi_tls->G_atmi_xa_curtx.is_xa_open)
    {
        ret = TX_PROTOCOL_ERROR;
        goto out;
    }
    
    if (tt < 0)
    {
        NDRX_LOG(log_error, "Invalid value: timeout %ld", (int)tt);
        ret = TX_EINVAL;
        goto out;
    }
    
    G_atmi_tls->tx_transaction_timeout = tt;
    
    NDRX_LOG(log_info, "Transaction timeout out set to %ld", 
            (long)G_atmi_tls->tx_transaction_timeout);
    
out:
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
