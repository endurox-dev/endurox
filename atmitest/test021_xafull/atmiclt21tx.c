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
    
    if (TX_OK!=(ret=tx_open()))
    {
        NDRX_LOG(log_error, "TESTERROR: tx_open() fail: %d", ret);
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

        Bchg(p_ub, T_STRING_FLD, 0, "TEST HELLO WORLD COMMIT", 0L);

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

        Bchg(p_ub, T_STRING_FLD, 0, "TEST HELLO WORLD ABORT", 0L);

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

        Bchg(p_ub, T_STRING_FLD, 0, "TEST HELLO WORLD SVCFAIL", 0L);

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
            ret=EXFAIL;
            goto out;
        }

        Bchg(p_ub, T_STRING_FLD, 0, "TEST HELLO WORLD TOUT", 0L);

        /* Call Svc1 */
        if (EXFAIL == (ret=tpcall("RUNTX", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0)))
        {
            NDRX_LOG(log_error, "TESTERROR: RUNTX should not return fail!");
        }
        
        sleep(10);

        ret=tx_commit();
        
        if (tperrno!=TX_ROLLBACK)
        {
            NDRX_LOG(log_error, "TESTERROR: tx_commit()==%d must be TX_ROLLBACK!", 
                                                ret, );
            ret=EXFAIL;
            goto out;
        }
        
        ret = EXSUCCEED;
    }    

    /***************************************************************************/
    NDRX_LOG(log_debug, "Call service, but not in tran mode - transaction must not be aborted!");
    /***************************************************************************/
    
    /* set tout to 5 */        
    if (TX_OK!=(ret=tx_set_transaction_timeout(2)))
    {
        NDRX_LOG(log_error, "TESTERROR: tx_set_transaction_timeout() fail: %d", 
                ret);
        EXFAIL_OUT(ret);
    }
    
    for (i=0; i<100; i++)
    {
        if (EXSUCCEED!=tpbegin(5, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: tpbegin() fail: %d:[%s]", 
                                                tperrno, tpstrerror(tperrno));
            ret=EXFAIL;
            goto out;
        }

        Bchg(p_ub, T_STRING_FLD, 0, "TEST HELLO WORLD SVCFAIL", 0L);

        /* Call Svc1 */
        if (EXFAIL != (ret=tpcall("NOTRANFAIL", (char *)p_ub, 0L, (char **)&p_ub, 
                &rsplen,TPNOTRAN)))
        {
            NDRX_LOG(log_error, "TESTERROR: NOTRANFAIL should return fail!");
        }

        ret=tpcommit(0);
        
        if (EXSUCCEED!=ret)
        {
            NDRX_LOG(log_error, "TESTERROR: tpcommit()==%d fail: %d:[%s] - must be SUCCEED!", 
                                                ret, tperrno, tpstrerror(tperrno));
            ret=EXFAIL;
            goto out;
        }
        
        ret = EXSUCCEED;
    }
    
    /* TODO: Test decision logged... - i.e. we need a benchmark
     * 1. say 300 txns with decision logged
     * 2. sleep 
     * 3. and 300 txns with standard completed
     * The 1. shall be faster than 3.
     */
    
    
    /* TODO: Test for transaction control -> chained action 
     * Begin txn
     * getinfo
     * Begin another txn
     * getinfo -> shall be different tx
     *  - check tout 5
     * 
     * set contol to unchained
     * commit -> shall not be in txn.
     * 
     * getinfo
     *   - no txn...
     * 
     */
    

    /***************************************************************************/
    NDRX_LOG(log_debug, "Done...");
    /***************************************************************************/
    
    if (EXSUCCEED!=tpclose())
    {
        NDRX_LOG(log_error, "TESTERROR: tpclose() fail: %d:[%s]", 
                                            tperrno, tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }
    
out:
    if (EXSUCCEED!=ret)
    {
        /* atleast try... */
        if (EXSUCCEED!=tpabort(0))
        {
            NDRX_LOG(log_error, "TESTERROR: tpabort() fail: %d:[%s]", 
                                                tperrno, tpstrerror(tperrno));
        }
        tpclose();
    }

    tpterm();
    
    NDRX_LOG(log_error, "Exiting with %d", ret);
            
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
