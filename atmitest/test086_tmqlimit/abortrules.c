/**
 * @brief Test abort rules
 *
 * @file abortrules.c
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
#include <unistd.h>

#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>
#include <test.fd.h>
#include <ndrstandard.h>
#include <ubfutil.h>
#include <nstopwatch.h>
#include <nstdutil.h>
#include <exassert.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Perform checks on abort rules
 * i.e. when transaction is marked for abort, and when not
 * @param maxmsg
 * @return 
 */
expublic int basic_abort_rules(int maxmsg)
{
    int ret = EXSUCCEED;
    TPQCTL qc;
    int i;
    long olen;
    char *buf=NULL;
    
    NDRX_LOG(log_error, "case basic_abort_rules");
    
    /* no abort: */
    NDRX_LOG(log_error, "Test tpcall + TPENOENT");
    NDRX_ASSERT_TP_OUT(EXSUCCEED==tpbegin(60, 0), "failed to start tran");
    NDRX_ASSERT_TP_OUT( 
                (EXFAIL==tpcall("NO_SUCH_SERVICE", NULL, 0, &buf, &olen, 0) 
                    && TPENOENT==tperrno), "NOENT call failed");
    NDRX_ASSERT_TP_OUT(EXSUCCEED==tpcommit(0), "failed to commit");
    
    /* no abort: */
    NDRX_LOG(log_error, "Test tpcall + TPEINVAL");
    NDRX_ASSERT_TP_OUT(EXSUCCEED==tpbegin(60, 0), "failed to start tran");
    NDRX_ASSERT_TP_OUT( 
                (EXFAIL==tpcall("NO_SUCH_SERVICE", NULL, 0, NULL, NULL, 0) 
                    && TPEINVAL==tperrno), "NOENT call failed");
    NDRX_ASSERT_TP_OUT(EXSUCCEED==tpcommit(0), "failed to commit");
    
    /* standard abort: */
    NDRX_LOG(log_error, "Test @TM-1 junk msg");
    NDRX_ASSERT_TP_OUT(EXSUCCEED==tpbegin(60, 0), "failed to start tran");
    NDRX_ASSERT_TP_OUT( 
                (EXFAIL==tpcall("@TM-1", NULL, 0, &buf, &olen, 0) 
                    && TPESVCFAIL==tperrno), "NOENT call failed");
    NDRX_ASSERT_TP_OUT( (EXSUCCEED!=tpcommit(0) && TPEABORT==tperrno), "commit + TPEABORT failed");
    
    /* QMENOMSG -> no abort tpdeq */
    NDRX_ASSERT_TP_OUT(EXSUCCEED==tpbegin(60, 0), "failed to start tran");
    memset(&qc, 0, sizeof(qc));
    NDRX_ASSERT_TP_OUT(
            (   EXFAIL==tpdequeue("MYSPACE", "TEST1", &qc, (char **)&buf, &olen, 0) 
                && TPEDIAGNOSTIC==tperrno 
                && QMENOMSG==qc.diagnostic
            ),
            "Failed to dequeue: %ld", qc.diagnostic
            );
    NDRX_ASSERT_TP_OUT(EXSUCCEED==tpcommit(0), "failed to commit");
    
    /* QMEBADQUEUE -> no abort tpdeq */
    NDRX_ASSERT_TP_OUT(EXSUCCEED==tpbegin(60, 0), "failed to start tran");
    memset(&qc, 0, sizeof(qc));
    NDRX_ASSERT_TP_OUT(
            (   EXFAIL==tpdequeue("MYSPACE", "NO_SUCH_QUEUE", &qc, (char **)&buf, &olen, 0) 
                && TPEDIAGNOSTIC==tperrno 
                && QMEBADQUEUE==qc.diagnostic
            ),
            "Failed to dequeue: %ld", qc.diagnostic
            );
    NDRX_ASSERT_TP_OUT(EXSUCCEED==tpcommit(0), "failed to commit");
    
    /* TPEINVAL -> no abort tpdeq */
    NDRX_ASSERT_TP_OUT(EXSUCCEED==tpbegin(60, 0), "failed to start tran");
    memset(&qc, 0, sizeof(qc));
    NDRX_ASSERT_TP_OUT(
            (   EXFAIL==tpdequeue(NULL, NULL, &qc, (char **)&buf, &olen, 0) 
                && TPEINVAL==tperrno 
            ),
            "Failed to dequeue: %ld", qc.diagnostic
            );
    NDRX_ASSERT_TP_OUT(EXSUCCEED==tpcommit(0), "failed to commit");
    
    /* QMEBADQUEUE -> no abort tpenq */
    NDRX_ASSERT_TP_OUT(EXSUCCEED==tpbegin(60, 0), "failed to start tran");
    memset(&qc, 0, sizeof(qc));
    NDRX_ASSERT_TP_OUT(
            (   EXFAIL==tpenqueue("MYSPACE", "NO_SUCH_QUEUE", &qc, NULL, 0, 0) 
                && TPEDIAGNOSTIC==tperrno 
                && QMEBADQUEUE==qc.diagnostic
            ),
            "Failed to enqueue: %ld", qc.diagnostic
            );
    NDRX_ASSERT_TP_OUT(EXSUCCEED==tpcommit(0), "failed to commit");
    
    /* TPEINVAL -> no abort tpenq */
    NDRX_ASSERT_TP_OUT(EXSUCCEED==tpbegin(60, 0), "failed to start tran");
    memset(&qc, 0, sizeof(qc));
    NDRX_ASSERT_TP_OUT(
            (   EXFAIL==tpenqueue(NULL, NULL, &qc, NULL, 0, 0) 
                && TPEINVAL==tperrno 
            ),
            "Failed to enqueue: %ld", qc.diagnostic
            );
    NDRX_ASSERT_TP_OUT(EXSUCCEED==tpcommit(0), "failed to commit");
    
    
    /* enqueue Norm message, for deq test */
    
    NDRX_ASSERT_TP_OUT(EXSUCCEED==tpbegin(60, 0), "failed to start tran");
    memset(&qc, 0, sizeof(qc));
    NDRX_ASSERT_TP_OUT(
            (EXSUCCEED==tpenqueue("MYSPACE", "TEST1", &qc, NULL, 0, 0)),
            "Failed to enqueue: %ld", qc.diagnostic
            );
    NDRX_ASSERT_TP_OUT(EXSUCCEED==tpcommit(0), "failed to commit");
    
    /* enable write error */
    if (EXSUCCEED!=system("xadmin lcf qwriterr -A 1 -a"))
    {
        NDRX_LOG(log_error, "TESTERROR: xadmin lcf qwriterr -A 1 -a failed");
        EXFAIL_OUT(ret);
    }
    
    /* TPEOS -> deq abort (disk) */
    NDRX_ASSERT_TP_OUT(EXSUCCEED==tpbegin(60, 0), "failed to start tran");
    memset(&qc, 0, sizeof(qc));
    NDRX_ASSERT_TP_OUT(
            (   EXFAIL==tpdequeue("MYSPACE", "TEST1", &qc, (char **)&buf, &olen, 0) 
                && TPEDIAGNOSTIC==tperrno 
                && QMEOS==qc.diagnostic
            ),
            "Dequeue shall fail with QMEOS, but got: %ld", qc.diagnostic
            );
    NDRX_ASSERT_TP_OUT( (EXSUCCEED!=tpcommit(0) && TPEABORT==tperrno), "commit must fail");
    
    
    /* QMESYSTEM -> enq abort (disk) */
    NDRX_ASSERT_TP_OUT(EXSUCCEED==tpbegin(60, 0), "failed to start tran");
    memset(&qc, 0, sizeof(qc));
    NDRX_ASSERT_TP_OUT(
            (   EXFAIL==tpenqueue("MYSPACE", "TEST1", &qc, NULL, 0, 0) 
                && TPEDIAGNOSTIC==tperrno 
                && QMEOS==qc.diagnostic
            ),
            "Enqueue shall fail with QMEOS, but got: %ld", qc.diagnostic
            );
    NDRX_ASSERT_TP_OUT( (EXSUCCEED!=tpcommit(0) && TPEABORT==tperrno), "commit must fail");
    
    
    /* unset error... */
    if (EXSUCCEED!=system("xadmin lcf qwriterr -A 0 -a"))
    {
            NDRX_LOG(log_error, "TESTERROR: xadmin lcf qwriterr -A 0 -a failed");
            EXFAIL_OUT(ret);
    }
    
    /* fetch 1 msg */
    NDRX_ASSERT_TP_OUT(EXSUCCEED==tpbegin(60, 0), "failed to start tran");
    memset(&qc, 0, sizeof(qc));
    NDRX_ASSERT_TP_OUT(
            (   EXSUCCEED==tpdequeue("MYSPACE", "TEST1", &qc, (char **)&buf, &olen, 0) 
            ),
            "Dequeue must not fail but got: %ld", qc.diagnostic
            );
    NDRX_ASSERT_TP_OUT( (EXSUCCEED==tpcommit(0)), "commit must not fail");
    
out:

    if (EXSUCCEED!=tpterm())
    {
        NDRX_LOG(log_error, "tpterm failed with: %s", tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
