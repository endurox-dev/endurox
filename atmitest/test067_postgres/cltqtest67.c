/**
 * @brief PostgreSQL PQ TMSRV driver tests / branch transactions - client
 *   Perform local calls with help of PQ commands.
 *   the server process shall run the code with Embedded SQL
 *
 * @file cltqtest67.c
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
#include <stdlib.h>
#include <memory.h>
#include <math.h>

#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>
#include <test.fd.h>
#include <ndrstandard.h>
#include <nstopwatch.h>
#include <fcntl.h>
#include <unistd.h>
#include <nstdutil.h>
#include "test67.h"
#include <libpq-fe.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Run generic queue tests
 * transaction/insert/queue add - abort (test 0/no msg)
 * transaction/insert/queue add - commit (test count ok / have messages OK)
 * Say 100 msg·
 */
expublic int q_run(UBFH **pp_ub)
{
    int ret = EXSUCCEED;
    long i, j;
    long len;
    long tmp;
    long rsplen;
    TPQCTL qc;
    
    for (i=0; i<3; i++)
    {
        sql_mktab();
        
        /* start tran... */
        if (EXSUCCEED!=tpbegin(15, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to begin: %s", tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
        
        for (j=0; j<100; j++)
        {
            if (EXFAIL==Bchg(*pp_ub, T_LONG_FLD, 0, (char *)&j, 0))
            {
                NDRX_LOG(log_debug, "TESTERROR: Failed to set T_LONG_FLD: %s", 
                        Bstrerror(Berror));
                EXFAIL_OUT(ret);
            }    

            if (EXFAIL == tpcall("TESTSV", (char *)*pp_ub, 0L, (char **)pp_ub, &rsplen,0))
            {
                NDRX_LOG(log_error, "TESTERROR: TESTSV failed: %s", tpstrerror(tperrno));
                EXFAIL_OUT(ret);
            }
            
            /* enqueue message... */
            
            /* enqueue the data buffer */
            memset(&qc, 0, sizeof(qc));
            if (EXSUCCEED!=tpenqueue("MYSPACE", "STATQ", &qc, (char *)*pp_ub, 
                    0L, 0L))
            {
                NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                        tpstrerror(tperrno), qc.diagnostic, qc.diagmsg);
                EXFAIL_OUT(ret);
            }
        }
        
        if (0==i || 2==i)
        {
            if (2==i)
            {
                sleep(20);
                
                if (EXSUCCEED==tpcommit(0L))
                {
                    NDRX_LOG(log_error, "TESTERROR: tpcommit must fail but succeed!");
                    EXFAIL_OUT(ret);
                }
                
                if (TPEABORT!=tperrno)
                {
                    NDRX_LOG(log_error, "TESTERROR: transaction must be aborted but: %s!",
                            tpstrerror(tperrno));
                    EXFAIL_OUT(ret);
                }
                
            }
            else
            {
                if (EXSUCCEED!=tpabort(0L))
                {
                    NDRX_LOG(log_error, "TESTERROR: Failed to abort: %s", 
                            tpstrerror(tperrno));
                    EXFAIL_OUT(ret);
                }
            }
            
            /* Check the counts, must be 0 */
            if (0!=(ret=(int)sql_count()))
            {
                NDRX_LOG(log_error, "TESTERROR: Invalid count expected 0 got: %d", ret);
                EXFAIL_OUT(ret);
            }
            
            /* dequeue shall fail... / no msg */
            
            memset(&qc, 0, sizeof(qc));

            if (EXSUCCEED==tpdequeue("MYSPACE", "STATQ", &qc, (char **)pp_ub, 
                    &len, TPNOTRAN))
            {
                /*
                NDRX_LOG(log_error, "TESTERROR: tpdequeue() failed %s diag: %d:%s", 
                        tpstrerror(tperrno), qc.diagnostic, qc.diagmsg);
                 * */
                NDRX_LOG(log_error, "TESTERROR: Queue must be empty but it is not!");
                EXFAIL_OUT(ret);
            }
            
            if (TPEDIAGNOSTIC!=tperrno)
            {
                NDRX_LOG(log_error, "TESTERROR: Expected TPEDIAGNOSTIC, but got %d", tperrno);
                EXFAIL_OUT(ret);
            }
            
            if (QMENOMSG!=qc.diagnostic)
            {
                NDRX_LOG(log_error, "TESTERROR: Expected QMENOMSG, but got %d", 
                        qc.diagnostic);
                EXFAIL_OUT(ret);
            }
        }
        else
        {
            if (EXSUCCEED!=tpcommit(0L))
            {
                NDRX_LOG(log_error, "TESTERROR: Failed to commit: %s", 
                        tpstrerror(tperrno));
                EXFAIL_OUT(ret);
            }
            
            /* Check the counts, must be 0 */
            if (100!=(ret=(int)sql_count()))
            {
                NDRX_LOG(log_error, "TESTERROR: Invalid count expected 100 got: %d", 
                        ret);
                EXFAIL_OUT(ret);
            }
            ret = EXSUCCEED;
            
            for (j=0; j<100; j++)
            {
                /* there must be 100 msgs in queue */

                memset(&qc, 0, sizeof(qc));

                if (EXSUCCEED!=tpdequeue("MYSPACE", "STATQ", &qc, (char **)pp_ub, 
                        &len, TPNOTRAN))
                {
                    NDRX_LOG(log_error, "TESTERROR: tpdequeue() failed at %d %s diag: %d:%s", 
                            i, tpstrerror(tperrno), qc.diagnostic, qc.diagmsg);
                    EXFAIL_OUT(ret);
                }

                /* test the buffer */

                if (EXSUCCEED!=Bget(*pp_ub, T_LONG_FLD, 0, (char *)&tmp, 0L))
                {
                    NDRX_LOG(log_error, "TESTERROR: Failed to get T_LONG_FLD at %d: %s",
                            i, Bstrerror(Berror));
                    EXFAIL_OUT(ret);
                }
                
                if (tmp!=j)
                {
                    NDRX_LOG(log_error, "TESTERROR: Invalid msg from Q: expected %ld got %ld",
                            j, tmp);
                    EXFAIL_OUT(ret);
                }
            }
        } /* if i=1 (this is case for commit) */
        
        
    } /* for test case */
out:
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
