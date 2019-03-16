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
    
    ret = tpcommit(0L);
    
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
 * @return 
 */
expublic int tx_info(TXINFO * txinfo)
{
    int ret = TX_OK; /* not in tran mode */
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
        
        /* TODO: copy off xid */
        
        
        /* check are we marked for rollback? */
        
        if (G_atmi_tls->G_atmi_xa_curtx.txinfo->tmtxflags & TMTXFLAGS_IS_ABORT_ONLY)
        {
            //
        }
        
    }
    
    
out:
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

expublic int tx_rollback(void)
{
    
}

expublic int tx_set_commit_return(COMMIT_RETURN cr)
{
    
}

expublic int tx_set_transaction_control(TRANSACTION_CONTROL tc)
{
    
}

expublic int tx_set_transaction_timeout(TRANSACTION_TIMEOUT tt)
{
    
}





/* vim: set ts=4 sw=4 et smartindent: */
