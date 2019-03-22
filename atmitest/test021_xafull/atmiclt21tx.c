/**
 * @brief XA transaction processing - tx.h tests
 *
 * @file atmiclt21tx.c
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
#include <math.h>
#include <unistd.h>

#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>
#include <test.fd.h>
#include <ndrstandard.h>
#include <nstopwatch.h>
#include <tx.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/*
 * Do the test call to the server
 */
int main(int argc, char** argv) {

    UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, 9216);
    long rsplen;
    int i=0;
    int ret=EXSUCCEED;
    ndrx_stopwatch_t w;
    long delta1, delta2;
    TXINFO txinf;
    TXINFO txinf2;
    /***************************************************************************/
    NDRX_LOG(log_debug, "Test errors... try begin with not open");
    /***************************************************************************/
    
    if (TX_PROTOCOL_ERROR!=(ret=tx_begin()))
    {
        NDRX_LOG(log_error, "TESTERROR: tx_begin() fail: %d, must be %d", 
                ret, TX_PROTOCOL_ERROR);
        EXFAIL_OUT(ret);
    }
    
    if (TX_PROTOCOL_ERROR!=(ret=tx_commit()))
    {
        NDRX_LOG(log_error, "TESTERROR: tx_commit() fail: %d, must be %d", 
                ret, TX_PROTOCOL_ERROR);
        EXFAIL_OUT(ret);
    }
    
    if (TX_PROTOCOL_ERROR!=(ret=tx_rollback()))
    {
        NDRX_LOG(log_error, "TESTERROR: tx_rollback() fail: %d, must be %d", 
                ret, TX_PROTOCOL_ERROR);
        EXFAIL_OUT(ret);
    }
    
    if (TX_OK!=(ret=tx_open()))
    {
        NDRX_LOG(log_error, "TESTERROR: tx_open() fail: %d", ret);
        EXFAIL_OUT(ret);
    }
    
    if (0!=(ret=tx_info(&txinf)))
    {
        NDRX_LOG(log_error, "TESTERROR: tx_info()==%d must be in 0 (not in tran)", 
                ret);
        EXFAIL_OUT(ret);
    }
    
    /***************************************************************************/
    NDRX_LOG(log_debug, "Testing normal tx processing - commit() ...");
    /***************************************************************************/
    
    /* set tout to 5 */        
    if (TX_OK!=(ret=tx_set_transaction_timeout(5)))
    {
        NDRX_LOG(log_error, "TESTERROR: tx_set_transaction_timeout() fail: %d", 
                ret);
        EXFAIL_OUT(ret);
    }

    for (i=0; i<100; i++)
    {
        if (TX_OK!=(ret=tx_begin()))
        {
            NDRX_LOG(log_error, "TESTERROR: tx_begin() fail: %d", ret);
            EXFAIL_OUT(ret);
        }

        Bchg(p_ub, T_STRING_FLD, 0, "TXAPI COMMIT", 0L);

        /* Call Svc1 */
        if (EXFAIL == (ret=tpcall("RUNTX", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0)))
        {
            NDRX_LOG(log_error, "TX3SVC failed: %s", tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }

        if (TX_OK!=(ret=tx_commit()))
        {
            NDRX_LOG(log_error, "TESTERROR: tx_commit()==%d", ret);
            EXFAIL_OUT(ret);
        }
    }
    
    /***************************************************************************/
    NDRX_LOG(log_debug, "Testing normal tx processing - abort() ...");
    /***************************************************************************/
    for (i=0; i<100; i++)
    {
        if (TX_OK!=(ret=tx_begin()))
        {
            NDRX_LOG(log_error, "TESTERROR: tx_begin() %d", ret);
            EXFAIL_OUT(ret);
        }

        Bchg(p_ub, T_STRING_FLD, 0, "TXAPI ABORT", 0L);

        /* Call Svc1 */
        if (EXFAIL == (ret=tpcall("RUNTX", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0)))
        {
            NDRX_LOG(log_error, "TX3SVC failed: %s", tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }

        if (TX_OK!=(ret=tx_rollback()))
        {
            NDRX_LOG(log_error, "TESTERROR: tx_rollback()==%d", ret);
            EXFAIL_OUT(ret);
        }
    }
    
    /***************************************************************************/
    NDRX_LOG(log_debug, "Service returns fail");
    /***************************************************************************/
    for (i=0; i<100; i++)
    {
        if (TX_OK!=(ret=tx_begin()))
        {
            NDRX_LOG(log_error, "TESTERROR: tx_begin() fail: %d", ret);
            ret=EXFAIL;
            goto out;
        }

        Bchg(p_ub, T_STRING_FLD, 0, "TXAPI SVCFAIL", 0L);

        /* Call Svc1 */
        if (EXFAIL != (ret=tpcall("RUNTXFAIL", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0)))
        {
            NDRX_LOG(log_error, "RUNTXFAIL should return fail!");
        }

        ret=tx_commit();
        
        if (TX_ROLLBACK!=ret)
        {
            NDRX_LOG(log_error, "TESTERROR: tx_commit()==%d (!=TX_ROLLBACK)", ret);
            ret=EXFAIL;
            goto out;
        }
        
        ret = EXSUCCEED;
    }    
    /***************************************************************************/
    NDRX_LOG(log_debug, "Transaction time-out (by tmsrv)");
    /***************************************************************************/
    
    /* set tout to 5 */        
    if (TX_OK!=(ret=tx_set_transaction_timeout(2)))
    {
        NDRX_LOG(log_error, "TESTERROR: tx_set_transaction_timeout() fail: %d", 
                ret);
        EXFAIL_OUT(ret);
    }
    
    for (i=0; i<5; i++)
    {
        if (EXSUCCEED!=(ret=tx_begin()))
        {
            NDRX_LOG(log_error, "TESTERROR: tx_begin() fail: %d", ret);
            EXFAIL_OUT(ret);
        }

        Bchg(p_ub, T_STRING_FLD, 0, "TXAPI TOUT", 0L);

        /* Call Svc1 */
        if (EXFAIL == (ret=tpcall("RUNTX", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0)))
        {
            NDRX_LOG(log_error, "TESTERROR: RUNTX should not return fail!");
            EXFAIL_OUT(ret);
        }
        
        if (1!=(ret=tx_info(&txinf)))
        {
            NDRX_LOG(log_error, "TESTERROR: tx_info()==%d fail", ret);
            EXFAIL_OUT(ret);
        }
        
        /* transaction must rolled back... */
        if (TX_ACTIVE!=txinf.transaction_state)
        {
            NDRX_LOG(log_error, "TESTERROR: tx_info not active: %d expected %d", 
                    txinf.transaction_state, TX_ACTIVE);
            EXFAIL_OUT(ret);
        }
        
        sleep(10);

        /* get transaction info, are we in state of rollback by TMSRV? */
        if (1!=(ret=tx_info(&txinf)))
        {
            NDRX_LOG(log_error, "TESTERROR: tx_info()==%d fail", ret);
            EXFAIL_OUT(ret);
        }
        
        /* transaction must rolled back... */
        if (TX_TIMEOUT_ROLLBACK_ONLY!=txinf.transaction_state)
        {
            NDRX_LOG(log_error, "TESTERROR: tx_info not rollback state: %d expected %d", 
                    txinf.transaction_state, TX_TIMEOUT_ROLLBACK_ONLY);
            EXFAIL_OUT(ret);
        }
        
        /* check other flags... */
        if (2!=txinf.transaction_timeout)
        {
            NDRX_LOG(log_error, "TESTERROR: tx_info tout: %d expected %d", 
                    txinf.transaction_timeout, 2);
            EXFAIL_OUT(ret);
        }
        
        if (TX_COMMIT_COMPLETED!=txinf.when_return)
        {
            NDRX_LOG(log_error, "TESTERROR: tx_info when_return: %d expected %d", 
                    txinf.when_return, TX_COMMIT_COMPLETED);
            EXFAIL_OUT(ret);
        }
        
        if (TX_UNCHAINED!=txinf.transaction_control)
        {
            NDRX_LOG(log_error, "TESTERROR: tx_info transaction_control: %d expected %d", 
                    txinf.transaction_control, TX_UNCHAINED);
            EXFAIL_OUT(ret);
        }
        
        ret=tx_commit();
        
        if (ret!=TX_ROLLBACK)
        {
            NDRX_LOG(log_error, "TESTERROR: tx_commit()==%d must be TX_ROLLBACK!", 
                                                ret);
            EXFAIL_OUT(ret);
        }
        
        ret = EXSUCCEED;
    }    

    /***************************************************************************/
    NDRX_LOG(log_debug, "Call service, but not in tran mode - transaction must not be aborted!");
    /***************************************************************************/
    
    /* set tout to 5 */        
    if (TX_OK!=(ret=tx_set_transaction_timeout(5)))
    {
        NDRX_LOG(log_error, "TESTERROR: tx_set_transaction_timeout() fail: %d", 
                ret);
        EXFAIL_OUT(ret);
    }
    for (i=0; i<200; i++)
    {
        if (TX_OK!=(ret=tx_begin()))
        {
            NDRX_LOG(log_error, "TESTERROR: tx_begin() fail: %d", ret);
            ret=EXFAIL;
            goto out;
        }

        Bchg(p_ub, T_STRING_FLD, 0, "TEST HELLO WORLD SVCFAIL", 0L);

        /* Call Svc1 */
        if (EXFAIL != (ret=tpcall("NOTRANFAIL", (char *)p_ub, 0L, (char **)&p_ub, 
                &rsplen,TPNOTRAN)))
        {
            NDRX_LOG(log_error, "TESTERROR: NOTRANFAIL should return fail!");
            EXFAIL_OUT(ret);
        }

        ret=tx_commit();
        
        if (TX_OK!=ret)
        {
            NDRX_LOG(log_error, "TESTERROR: tx_commit()==%d fail!", ret);
            ret=EXFAIL;
            goto out;
        }
    }
    
    /* TODO: Test decision logged... - i.e. we need a benchmark
     * 1. say 300 txns with decision logged
     * 2. sleep 
     * 3. and 300 txns with standard completed
     * The 1. shall be faster than 3.
     */
    /***************************************************************************/
    NDRX_LOG(log_debug, "take time for 300txns with logged!");
    /***************************************************************************/
    
    
    ndrx_stopwatch_reset(&w);
    if (TX_OK!=(ret=tx_set_commit_return(TX_COMMIT_DECISION_LOGGED)))
    {
        NDRX_LOG(log_error, "TESTERROR: tx_set_commit_return() fail: %d", 
                ret);
        EXFAIL_OUT(ret);
    }
     
    for (i=0; i<300; i++)
    {
        if (TX_OK!=(ret=tx_begin()))
        {
            NDRX_LOG(log_error, "TESTERROR: tx_begin() fail: %d", ret);
            EXFAIL_OUT(ret);
        }
        /* needs newline for solaris grep, otherwise we get 1 line */
        Bchg(p_ub, T_STRING_FLD, 0, "TXAPI LOGGED\n", 0L);



        /* Call Svc1 */
        if (EXFAIL == (ret=tpcall("RUNTX", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0)))
        {
            NDRX_LOG(log_error, "TX3SVC failed: %s", tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }

        if (TX_OK!=(ret=tx_commit()))
        {
            NDRX_LOG(log_error, "TESTERROR: tx_commit()==%d at %d", ret, i);
            EXFAIL_OUT(ret);
        }
    }
    
    delta1 = ndrx_stopwatch_get_delta(&w);
    
    NDRX_LOG(log_debug, "Sleep 60 -> wait for complete..!");
    sleep(60);
    /***************************************************************************/
    NDRX_LOG(log_debug, "take time for 300txns with completed!");
    /***************************************************************************/
    
    ndrx_stopwatch_reset(&w);
    
    if (TX_OK!=(ret=tx_set_commit_return(TX_COMMIT_COMPLETED)))
    {
        NDRX_LOG(log_error, "TESTERROR: tx_set_transaction_timeout() fail: %d", 
                ret);
        EXFAIL_OUT(ret);
    }
    
    if (TP_CMT_COMPLETE!=(ret=tpscmt(TP_CMT_LOGGED)))
    {
        NDRX_LOG(log_error, "TESTERROR: tpscmt() fail %d: %s", 
                ret, tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    if (TP_CMT_LOGGED!=(ret=tpscmt(TP_CMT_COMPLETE)))
    {
        NDRX_LOG(log_error, "TESTERROR: tpscmt() fail %d: %s", 
                ret, tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    if (TP_CMT_COMPLETE!=(ret=tpscmt(TP_CMT_COMPLETE)))
    {
        NDRX_LOG(log_error, "TESTERROR: tpscmt() fail %d: %s", 
                ret, tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
     
    for (i=0; i<300; i++)
    {
        if (TX_OK!=(ret=tx_begin()))
        {
            NDRX_LOG(log_error, "TESTERROR: tx_begin() fail: %d", ret);
            EXFAIL_OUT(ret);
        }

        Bchg(p_ub, T_STRING_FLD, 0, "TXAPI FULL\n", 0L);

        /* Call Svc1 */
        if (EXFAIL == (ret=tpcall("RUNTX", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0)))
        {
            NDRX_LOG(log_error, "TX3SVC failed: %s", tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }

        if (TX_OK!=(ret=tx_commit()))
        {
            NDRX_LOG(log_error, "TESTERROR: tx_commit()==%d at %d", ret, i);
            EXFAIL_OUT(ret);
        }
    }
    
    delta2 = ndrx_stopwatch_get_delta(&w);
    
    NDRX_LOG(log_warn, "Logged %ld msec vs %ld msec completed", delta1, delta2);
    /*  might not be true as commit takes very short operation here in test..
    if (delta2 <= delta1)
    {
        NDRX_LOG(log_error, "TESTERROR: Logged is slower or equal to full "
                "complete %ld vs %ld!", delta1, delta2);
        ret=EXFAIL;
        goto out;
    }
    */
    
    /***************************************************************************/
    NDRX_LOG(log_debug, "Testing chained ops...");
    /***************************************************************************/
    
    if (TX_OK!=(ret=tx_set_transaction_control(TX_CHAINED)))
    {
        NDRX_LOG(log_error, "TESTERROR: tx_set_transaction_control() fail: %d", 
                ret);
        EXFAIL_OUT(ret);
    }
    
    /* Test for transaction control -> chained action 
     * Begin txn
     * getinfo
     * commit
     * Begin another txn
     * getinfo -> shall be different tx
     * abort
     * getinfo -> shall be another tx
     * set contol to unchained
     * commit -> shall not be in txn.
     * getinfo
     *   - no txn...
     */
    
    if (TX_OK!=(ret=tx_begin()))
    {
        NDRX_LOG(log_error, "TESTERROR: tx_begin() fail: %d", ret);
        ret=EXFAIL;
        goto out;
    }
    
    if (1!=(ret=tx_info(&txinf)))
    {
        NDRX_LOG(log_error, "TESTERROR: tx_info()==%d fail", ret);
        EXFAIL_OUT(ret);
    }
    
    if (TX_OK!=(ret=tx_commit()))
    {
        NDRX_LOG(log_error, "TESTERROR: tx_commit() fail: %d", ret);
        ret=EXFAIL;
        goto out;
    }
    
    txinf2.transaction_state = TX_ROLLBACK_ONLY;
    if (1!=(ret=tx_info(&txinf2)))
    {
        NDRX_LOG(log_error, "TESTERROR: tx_info()==%d fail", ret);
        EXFAIL_OUT(ret);
    }
    
    if (TX_ACTIVE!=txinf2.transaction_state)
    {
        NDRX_LOG(log_error, "TESTERROR: chained tran not active: %d vs %d", 
                txinf2.transaction_state, TX_ACTIVE);
        EXFAIL_OUT(ret);
    }
    
    if (0==memcmp(&txinf.xid, &txinf2.xid, sizeof(XID)))
    {
        NDRX_LOG(log_error, "TESTERROR: XID not changed!");
        EXFAIL_OUT(ret);
    }

    if (TX_OK!=(ret=tx_rollback()))
    {
        NDRX_LOG(log_error, "TESTERROR: tx_rollback() fail: %d", ret);
        ret=EXFAIL;
        goto out;
    }
    
    txinf.transaction_state = TX_ROLLBACK_ONLY;
    if (1!=(ret=tx_info(&txinf)))
    {
        NDRX_LOG(log_error, "TESTERROR: tx_info()==%d fail", ret);
        EXFAIL_OUT(ret);
    }
    
    if (TX_ACTIVE!=txinf2.transaction_state)
    {
        NDRX_LOG(log_error, "TESTERROR: chained tran not active: %d vs %d", 
                txinf2.transaction_state, TX_ACTIVE);
        EXFAIL_OUT(ret);
    }
    
    if (0==memcmp(&txinf.xid, &txinf2.xid, sizeof(XID)))
    {
        NDRX_LOG(log_error, "TESTERROR: XID not changed!");
        EXFAIL_OUT(ret);
    }
    
    /* try some error.. with commit, maybe close tm? */
#if 0
    if (TX_OK!=tx_close())
    {
        NDRX_LOG(log_error, "TESTERROR: tx_close() fail: %d", ret);
        EXFAIL_OUT(ret);
    }
    
    if (TX_PROTOCOL_ERROR!=(ret=tx_rollback()))
    {
        NDRX_LOG(log_error, "TESTERROR: tx_rollback() fail: "
                "%d not %d (TX_PROTOCOL_ERROR)", ret, TX_PROTOCOL_ERROR);
        ret=EXFAIL;
        goto out;
    }
    
    if (TX_OK!=tx_open())
    {
        NDRX_LOG(log_error, "TESTERROR: tx_open() fail: %d", ret);
        EXFAIL_OUT(ret);
    }
#endif
    
    if (TX_OK!=(ret=tx_set_transaction_control(TX_UNCHAINED)))
    {
        NDRX_LOG(log_error, "TESTERROR: tx_set_transaction_control() fail: %d", 
                ret);
        EXFAIL_OUT(ret);
    }
    
    /* finish it off... */
    if (TX_OK!=(ret=tx_rollback()))
    {
        NDRX_LOG(log_error, "TESTERROR: tx_rollback() fail: %d", ret);
        ret=EXFAIL;
        goto out;
    }
    
    
    /* we shall be outside of global tran -> timed out by RM */
    sleep(5);
    
    /***************************************************************************/
    NDRX_LOG(log_debug, "Done...");
    /***************************************************************************/
    
    if (TX_OK!=(ret=tx_close()))
    {
        NDRX_LOG(log_error, "TESTERROR: tx_close() fail: %d", ret);
        EXFAIL_OUT(ret);
    }
    
out:
    if (EXSUCCEED!=ret)
    {
        /* atleast try... */
        if (TX_OK!=(ret=tx_rollback()))
        {
            NDRX_LOG(log_error, "TESTERROR: tx_rollback() fail: %d", ret);
        }
        
        tx_close();
    }

    tpterm();
    
    NDRX_LOG(log_error, "Exiting with %d", ret);
            
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
