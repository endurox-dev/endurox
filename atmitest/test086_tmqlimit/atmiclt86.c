/**
 * @brief TMQ test client /limits
 *
 * @file atmiclt86.c
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
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
exprivate int basic_qfull(int maxmsg);
exprivate int basic_tmsrvrestart(int maxmsg);
exprivate int basic_diskfull(int maxmsg);
exprivate int basic_commit_shut(int maxmsg);
exprivate int basic_loadprep(int maxmsg);
exprivate int basic_tmsrvdiskerr(int maxmsg);
exprivate int basic_badmsg(int maxmsg);
exprivate int basic_commit_crash(int maxmsg);
exprivate int basic_deqwriteerr(int maxmsg);
int main(int argc, char** argv)
{
    int ret = EXSUCCEED;
    
    if (argc<=1)
    {
        NDRX_LOG(log_error, "usage: %s <test_case: qfull>", argv[0]);
        return EXFAIL;
    }
    NDRX_LOG(log_error, "\n\n\n\n\n !!!!!!!!!!!!!! TEST CASE %s !!!!!!!! \n\n\n\n\n\n", argv[1]);
    
    if (EXSUCCEED!=tpopen())
    {
        EXFAIL_OUT(ret);
    }
    
    if (0==strcmp(argv[1], "qfull"))
    {
        return basic_qfull(1200);
    }
    else if (0==strcmp(argv[1], "tmsrvrestart"))
    {
        return basic_tmsrvrestart(1200);
    }
    else if (0==strcmp(argv[1], "commit_shut"))
    {
        return basic_commit_shut(1200);
    }
    else if (0==strcmp(argv[1], "loadprep"))
    {
        return basic_loadprep(1200);
    }
    else if (0==strcmp(argv[1], "diskfull"))
    {
        return basic_diskfull(10);
    }
    else if (0==strcmp(argv[1], "tmsrvdiskerr"))
    {
        return basic_tmsrvdiskerr(100);
    }
    else if (0==strcmp(argv[1], "badmsg"))
    {
        return basic_badmsg(100);
    }
    else if (0==strcmp(argv[1], "commit_crash"))
    {
        return basic_commit_crash(100);
    }
    else if (0==strcmp(argv[1], "deqwriteerr"))
    {
        return basic_deqwriteerr(100);
    }
    else
    {
        NDRX_LOG(log_error, "Invalid test case!");
        return EXFAIL;
    }
    
out:

    tpclose();

    return ret;   
}

/**
 * Verify that we can process number of messages
 * @param maxmsg max messages to be ok
 */
exprivate int basic_qfull(int maxmsg)
{
    int ret = EXSUCCEED;
    TPQCTL qc;
    int i;
    
    NDRX_LOG(log_error, "case qfull");
    if (EXSUCCEED!=tpbegin(9999, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to begin");
        EXFAIL_OUT(ret);
    }
    
    /* Initial test... */
    for (i=0; i<maxmsg; i++)
    {
        char *testbuf_ref = tpalloc("CARRAY", "", 10);
        long len=10;

        testbuf_ref[0]=0;
        testbuf_ref[1]=1;
        testbuf_ref[2]=2;
        testbuf_ref[3]=3;
        testbuf_ref[4]=4;
        testbuf_ref[5]=5;
        testbuf_ref[6]=6;
        testbuf_ref[7]=7;
        testbuf_ref[8]=8;
        testbuf_ref[9]=9;

        /* alloc output buffer */
        if (NULL==testbuf_ref)
        {
            NDRX_LOG(log_error, "TESTERROR: tpalloc() failed %s", 
                    tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }

        /* enqueue the data buffer */
        memset(&qc, 0, sizeof(qc));
        if (EXSUCCEED!=tpenqueue("MYSPACE", "TEST1", &qc, testbuf_ref, 
                len, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                    tpstrerror(tperrno), qc.diagnostic, qc.diagmsg);
            EXFAIL_OUT(ret);
        }

        tpfree(testbuf_ref);
    }
    
    /* restart tmqueue.... 
     * it shall be able to commit OK
     */
    if (EXSUCCEED!=system("xadmin stop -s tmqueue"))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to stop tmqueue");
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=system("xadmin start -s tmqueue"))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to start tmqueue");
        EXFAIL_OUT(ret);
    }
    
    /* also.. here all message shall be locked */
    for (i=0; i<1; i++)
    {
        long len=0;
        char *buf;
        buf = tpalloc("CARRAY", "", 100);
        memset(&qc, 0, sizeof(qc));

        /* This shall be updated so that we do not need to use TPNOABORT  */
        if (EXSUCCEED==tpdequeue("MYSPACE", "TEST1", &qc, (char **)&buf, &len, TPNOABORT))
        {
            NDRX_LOG(log_error, "TESTERROR: TEST1 dequeued, even already in progress!");
            EXFAIL_OUT(ret);
        }
        
        tpfree(buf);
    }
    
    if (EXSUCCEED!=tpcommit(0))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to commit");
        EXFAIL_OUT(ret);
    }
    
    /* check that number of messages are available... */
    NDRX_LOG(log_error, "About to dequeue %d messages", maxmsg);
    
    if (EXSUCCEED!=tpbegin(9999, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to begin");
        EXFAIL_OUT(ret);
    }
    
    /* all messages must be available */
    for (i=0; i<maxmsg; i++)
    {
        long len=0;
        char *buf;
        buf = tpalloc("CARRAY", "", 100);
        memset(&qc, 0, sizeof(qc));

        if (EXSUCCEED!=tpdequeue("MYSPACE", "TEST1", &qc, (char **)&buf, &len, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: TEST1 failed dequeue!");
            EXFAIL_OUT(ret);
        }
        
        tpfree(buf);
    }
    
    /* restart tmqueue.... */
    if (EXSUCCEED!=system("xadmin stop -s tmqueue"))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to stop tmqueue");
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=system("xadmin start -s tmqueue"))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to start tmqueue");
        EXFAIL_OUT(ret);
    }
    
    /* dequeue must fail as we have already dequeued the messages */
    /* try just 1 msg. */
    for (i=0; i<1; i++)
    {
        long len=0;
        char *buf;
        buf = tpalloc("CARRAY", "", 100);
        memset(&qc, 0, sizeof(qc));

        /* This shall be updated so that we do not need to use TPNOABORT  */
        if (EXSUCCEED==tpdequeue("MYSPACE", "TEST1", &qc, (char **)&buf, &len, TPNOABORT))
        {
            NDRX_LOG(log_error, "TESTERROR: TEST1 dequeued, even already in progress!");
            EXFAIL_OUT(ret);
        }
        
        tpfree(buf);
    }
    
    if (EXSUCCEED!=tpcommit(0))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to commit");
        EXFAIL_OUT(ret);
    }
    
out:
    
    if (EXSUCCEED!=tpterm())
    {
        NDRX_LOG(log_error, "tpterm failed with: %s", tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }

    return ret;
}

/**
 * Verify that we can handle tmsrv restarts in the middle of transaction
 * @param maxmsg max messages to be ok
 */
exprivate int basic_tmsrvrestart(int maxmsg)
{
    int ret = EXSUCCEED;
    TPQCTL qc;
    int i;
    
    NDRX_LOG(log_error, "case basic_tmsrvrestart");
    if (EXSUCCEED!=tpbegin(9999, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to begin");
        EXFAIL_OUT(ret);
    }
    
    /* Initial test... */
    for (i=0; i<maxmsg; i++)
    {
        char *testbuf_ref = tpalloc("CARRAY", "", 10);
        long len=10;

        testbuf_ref[0]=0;
        testbuf_ref[1]=1;
        testbuf_ref[2]=2;
        testbuf_ref[3]=3;
        testbuf_ref[4]=4;
        testbuf_ref[5]=5;
        testbuf_ref[6]=6;
        testbuf_ref[7]=7;
        testbuf_ref[8]=8;
        testbuf_ref[9]=9;

        /* alloc output buffer */
        if (NULL==testbuf_ref)
        {
            NDRX_LOG(log_error, "TESTERROR: tpalloc() failed %s", 
                    tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }

        /* enqueue the data buffer */
        memset(&qc, 0, sizeof(qc));
        if (EXSUCCEED!=tpenqueue("MYSPACE", "TEST1", &qc, testbuf_ref, 
                len, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                    tpstrerror(tperrno), qc.diagnostic, qc.diagmsg);
            EXFAIL_OUT(ret);
        }

        tpfree(testbuf_ref);
    }
    
    /* restart tmsrv.... 
     * it shall be able to commit OK
     */
    if (EXSUCCEED!=system("xadmin stop -s tmsrv"))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to stop tmsrv");
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=system("xadmin start -s tmsrv"))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to start tmsrv");
        EXFAIL_OUT(ret);
    }
    
    /* also.. here all message shall be locked */
    for (i=0; i<1; i++)
    {
        long len=0;
        char *buf;
        buf = tpalloc("CARRAY", "", 100);
        memset(&qc, 0, sizeof(qc));

        /* This shall be updated so that we do not need to use TPNOABORT  */
        if (EXSUCCEED==tpdequeue("MYSPACE", "TEST1", &qc, (char **)&buf, &len, TPNOABORT))
        {
            NDRX_LOG(log_error, "TESTERROR: TEST1 dequeued, even already in progress!");
            EXFAIL_OUT(ret);
        }
        
        tpfree(buf);
    }
    
    if (EXSUCCEED!=tpcommit(0))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to commit");
        EXFAIL_OUT(ret);
    }
    
    /* check that number of messages are available... */
    NDRX_LOG(log_error, "About to dequeue %d messages", maxmsg);
    
    if (EXSUCCEED!=tpbegin(9999, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to begin");
        EXFAIL_OUT(ret);
    }
    
    /* all messages must be available */
    for (i=0; i<maxmsg; i++)
    {
        long len=0;
        char *buf;
        buf = tpalloc("CARRAY", "", 100);
        memset(&qc, 0, sizeof(qc));

        if (EXSUCCEED!=tpdequeue("MYSPACE", "TEST1", &qc, (char **)&buf, &len, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: TEST1 failed dequeue!");
            EXFAIL_OUT(ret);
        }
        
        tpfree(buf);
    }
    
    /* restart tmqueue.... */
    if (EXSUCCEED!=system("xadmin stop -s tmqueue"))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to stop tmqueue");
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=system("xadmin start -s tmqueue"))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to start tmqueue");
        EXFAIL_OUT(ret);
    }
    
    /* dequeue must fail as we have already dequeued the messages */
    /* try just 1 msg. */
    for (i=0; i<1; i++)
    {
        long len=0;
        char *buf;
        buf = tpalloc("CARRAY", "", 100);
        memset(&qc, 0, sizeof(qc));

        /* This shall be updated so that we do not need to use TPNOABORT  */
        if (EXSUCCEED==tpdequeue("MYSPACE", "TEST1", &qc, (char **)&buf, &len, TPNOABORT))
        {
            NDRX_LOG(log_error, "TESTERROR: TEST1 dequeued, even already in progress!");
            EXFAIL_OUT(ret);
        }
        
        tpfree(buf);
    }
    
    if (EXSUCCEED!=tpcommit(0))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to commit");
        EXFAIL_OUT(ret);
    }
    
out:
    
    if (EXSUCCEED!=tpterm())
    {
        NDRX_LOG(log_error, "tpterm failed with: %s", tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }

    return ret;
}


/**
 * Try commit, queue is shutdown
 * It shall abort with retries... boot tmqueue back, no messages shall be available
 * @param maxmsg max messages to be ok
 */
exprivate int basic_commit_shut(int maxmsg)
{
    int ret = EXSUCCEED;
    TPQCTL qc;
    int i;
    
    NDRX_LOG(log_error, "case basic_commit_shut");
    if (EXSUCCEED!=tpbegin(9999, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to begin");
        EXFAIL_OUT(ret);
    }
    
    /* Initial test... */
    for (i=0; i<maxmsg; i++)
    {
        char *testbuf_ref = tpalloc("CARRAY", "", 10);
        long len=10;

        testbuf_ref[0]=0;
        testbuf_ref[1]=1;
        testbuf_ref[2]=2;
        testbuf_ref[3]=3;
        testbuf_ref[4]=4;
        testbuf_ref[5]=5;
        testbuf_ref[6]=6;
        testbuf_ref[7]=7;
        testbuf_ref[8]=8;
        testbuf_ref[9]=9;

        /* alloc output buffer */
        if (NULL==testbuf_ref)
        {
            NDRX_LOG(log_error, "TESTERROR: tpalloc() failed %s", 
                    tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }

        /* enqueue the data buffer */
        memset(&qc, 0, sizeof(qc));
        if (EXSUCCEED!=tpenqueue("MYSPACE", "TEST1", &qc, testbuf_ref, 
                len, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                    tpstrerror(tperrno), qc.diagnostic, qc.diagmsg);
            EXFAIL_OUT(ret);
        }

        tpfree(testbuf_ref);
    }
    
    /* restart tmqueue.... 
     * it shall be able to commit OK
     */
    if (EXSUCCEED!=system("xadmin stop -s tmqueue"))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to stop tmqueue");
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED==tpcommit(0))
    {
        NDRX_LOG(log_error, "TESTERROR: commit must fail");
        EXFAIL_OUT(ret);
    }
    
    /* we shall get abort error... */
    if (TPEHAZARD!=tperrno)
    {
        NDRX_LOG(log_error, "TESTERROR: invalid error, expected TPEHAZARD got %d",
                tperrno);
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=system("xadmin start -s tmqueue"))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to start tmqueue");
        EXFAIL_OUT(ret);
    }
    
    /* let tmsrv to complete stuff in the background... */
    NDRX_LOG(log_debug, "Waiting for message completion...");
    sleep(30);
    
    /* all messages must be available */
    for (i=0; i<maxmsg; i++)
    {
        long len=0;
        char *buf;
        buf = tpalloc("CARRAY", "", 100);
        memset(&qc, 0, sizeof(qc));

        if (EXSUCCEED!=tpdequeue("MYSPACE", "TEST1", &qc, (char **)&buf, &len, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: TEST1 failed dequeue!");
            EXFAIL_OUT(ret);
        }
        
        tpfree(buf);
    }
    
out:
    
    if (EXSUCCEED!=tpterm())
    {
        NDRX_LOG(log_error, "tpterm failed with: %s", tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }

    return ret;
}

/**
 * Dequeue write error (disk full on command file) must be reported
 * @param maxmsg max messages to be ok
 */
exprivate int basic_deqwriteerr(int maxmsg)
{
    int ret = EXSUCCEED;
    TPQCTL qc;
    int i;
    
    NDRX_LOG(log_error, "case basic_deqwriteerr");
    if (EXSUCCEED!=tpbegin(9999, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to begin");
        EXFAIL_OUT(ret);
    }
    
    /* Initial test... */
    for (i=0; i<maxmsg; i++)
    {
        char *testbuf_ref = tpalloc("CARRAY", "", 10);
        long len=10;

        testbuf_ref[0]=0;
        testbuf_ref[1]=1;
        testbuf_ref[2]=2;
        testbuf_ref[3]=3;
        testbuf_ref[4]=4;
        testbuf_ref[5]=5;
        testbuf_ref[6]=6;
        testbuf_ref[7]=7;
        testbuf_ref[8]=8;
        testbuf_ref[9]=9;

        /* alloc output buffer */
        if (NULL==testbuf_ref)
        {
            NDRX_LOG(log_error, "TESTERROR: tpalloc() failed %s", 
                    tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }

        /* enqueue the data buffer */
        memset(&qc, 0, sizeof(qc));
        if (EXSUCCEED!=tpenqueue("MYSPACE", "TEST1", &qc, testbuf_ref, 
                len, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                    tpstrerror(tperrno), qc.diagnostic, qc.diagmsg);
            EXFAIL_OUT(ret);
        }

        tpfree(testbuf_ref);
    }
    
    if (EXSUCCEED!=tpcommit(0))
    {
        NDRX_LOG(log_error, "TESTERROR: commit failed: %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=system("xadmin lcf qwriterr -A 1 -a"))
    {
        NDRX_LOG(log_error, "TESTERROR: xadmin lcf qwriterr -A 1 -a failed");
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=tpbegin(9999, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to begin");
        EXFAIL_OUT(ret);
    }
    
    /* all messages must be available */
    for (i=0; i<1; i++)
    {
        long len=0;
        char *buf;
        buf = tpalloc("CARRAY", "", 100);
        memset(&qc, 0, sizeof(qc));

        if (EXSUCCEED==tpdequeue("MYSPACE", "TEST1", &qc, (char **)&buf, &len, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: TEST1 dequeue must fail!!");
            EXFAIL_OUT(ret);
        }
        
        if (tperrno!=TPEDIAGNOSTIC)
        {
            NDRX_LOG(log_error, "TESTERROR: TPEDIAGNOSTIC expected, got: %d!", tperrno);
            EXFAIL_OUT(ret);
        }
        
        if (QMEOS!=qc.diagnostic)
        {
            NDRX_LOG(log_error, "TESTERROR: QMEOS expected, got: %d!", qc.diagnostic);
            EXFAIL_OUT(ret);
        }   
        tpfree(buf);

    }
    
    /* terminate the transaction */
    tpabort(0);
    
    
     if (EXSUCCEED!=system("xadmin lcf qwriterr -A 0 -a"))
    {
        NDRX_LOG(log_error, "TESTERROR: xadmin lcf qwriterr -A 0 -a failed");
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=tpbegin(9999, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to begin");
        EXFAIL_OUT(ret);
    }
    
    /* all messages must be available */
    for (i=0; i<maxmsg; i++)
    {
        long len=0;
        char *buf;
        buf = tpalloc("CARRAY", "", 100);
        memset(&qc, 0, sizeof(qc));

        if (EXSUCCEED!=tpdequeue("MYSPACE", "TEST1", &qc, (char **)&buf, &len, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: TEST1 dequeue failed!: %s %ld", 
                    tpstrerror(tperrno), qc.diagnostic);
            EXFAIL_OUT(ret);
        }
        
        tpfree(buf);
        
    }
    
    if (EXSUCCEED!=tpcommit(0))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to tpcommit");
        EXFAIL_OUT(ret);
    }
    
out:
    
    if (EXSUCCEED!=tpterm())
    {
        NDRX_LOG(log_error, "tpterm failed with: %s", tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }

    return ret;
}

/**
 * Load prepared messages (also tmq scans this twice, thus check that no problem)
 * @param maxmsg
 * @return 
 */
exprivate int basic_loadprep(int maxmsg)
{
    int ret = EXSUCCEED;
    TPQCTL qc;
    int i;
    
    NDRX_LOG(log_error, "case basic_loadprep");
    if (EXSUCCEED!=tpbegin(9999, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to begin");
        EXFAIL_OUT(ret);
    }
    
    /* Initial test... */
    for (i=0; i<maxmsg; i++)
    {
        char *testbuf_ref = tpalloc("CARRAY", "", 10);
        long len=10;

        testbuf_ref[0]=0;
        testbuf_ref[1]=1;
        testbuf_ref[2]=2;
        testbuf_ref[3]=3;
        testbuf_ref[4]=4;
        testbuf_ref[5]=5;
        testbuf_ref[6]=6;
        testbuf_ref[7]=7;
        testbuf_ref[8]=8;
        testbuf_ref[9]=9;

        /* alloc output buffer */
        if (NULL==testbuf_ref)
        {
            NDRX_LOG(log_error, "TESTERROR: tpalloc() failed %s", 
                    tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }

        /* enqueue the data buffer */
        memset(&qc, 0, sizeof(qc));
        if (EXSUCCEED!=tpenqueue("MYSPACE", "TEST1", &qc, testbuf_ref, 
                len, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                    tpstrerror(tperrno), qc.diagnostic, qc.diagmsg);
            EXFAIL_OUT(ret);
        }

        tpfree(testbuf_ref);
    }
    
    /* restart tmqueue.... 
     * it shall be able to commit OK
     */
    if (EXSUCCEED!=system("xadmin stop -s tmqueue"))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to stop tmqueue");
        EXFAIL_OUT(ret);
    }
    
    /* move files from active to prep... 
     * we will not commit, but tmqueue startup shall not fail.
     */
    
    if (EXSUCCEED!=system("mv QSPACE1/active/* QSPACE1/prepared/"))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to move transaction files...");
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=system("xadmin start -s tmqueue"))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to start tmqueue");
        EXFAIL_OUT(ret);
    }
    
out:
    
    if (EXSUCCEED!=tpterm())
    {
        NDRX_LOG(log_error, "tpterm failed with: %s", tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }

    return ret;
}

/**
 * Check the case when disk is full, shall fail to enqueue
 * @param maxmsg max messages to be ok
 */
exprivate int basic_diskfull(int maxmsg)
{
    int ret = EXSUCCEED;
    TPQCTL qc;
    int i;
    
    NDRX_LOG(log_error, "case basic_diskfull");
    
    if (EXSUCCEED!=tpbegin(9999, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to begin");
        EXFAIL_OUT(ret);
    }
    
    /* Set disk failure on for all processes...
     */
    if (EXSUCCEED!=system("xadmin lcf qwriterr -A 1 -a"))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to enable write failure");
        EXFAIL_OUT(ret);
    }
    
    /* Initial test... */
    for (i=0; i<maxmsg; i++)
    {
        char *testbuf_ref = tpalloc("CARRAY", "", 10);
        long len=10;

        testbuf_ref[0]=0;
        testbuf_ref[1]=1;
        testbuf_ref[2]=2;
        testbuf_ref[3]=3;
        testbuf_ref[4]=4;
        testbuf_ref[5]=5;
        testbuf_ref[6]=6;
        testbuf_ref[7]=7;
        testbuf_ref[8]=8;
        testbuf_ref[9]=9;

        /* alloc output buffer */
        if (NULL==testbuf_ref)
        {
            NDRX_LOG(log_error, "TESTERROR: tpalloc() failed %s", 
                    tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }

        /* enqueue the data buffer */
        memset(&qc, 0, sizeof(qc));
        
        if (EXSUCCEED==tpenqueue("MYSPACE", "TEST1", &qc, testbuf_ref, 
                len, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: tpenqueue() must fail - disk full");
            EXFAIL_OUT(ret);
        }
        
        NDRX_LOG(log_error, "tpenqueue() failed %s diag: %d:%s", 
                    tpstrerror(tperrno), qc.diagnostic, qc.diagmsg);
        
        if (TPEDIAGNOSTIC!=tperrno || QMESYSTEM!=qc.diagnostic)
        {
            NDRX_LOG(log_error, "TESTERROR: expected tperrno==TPEDIAGNOSTIC got %d and qc.diagnostic==QMESYSTEM got %d",
                    tperrno, qc.diagnostic);
            EXFAIL_OUT(ret);
        }
        
        tpfree(testbuf_ref);
    }
    
    if (EXSUCCEED==tpcommit(0))
    {
        NDRX_LOG(log_error, "TESTERROR: it shall fail to commit, as transactions ar marked for abort!");
        EXFAIL_OUT(ret);
    }
    
    if (TPEABORT!=tperrno)
    {
        NDRX_LOG(log_error, "TESTERROR: Expected TPEABORT got %d", tperrno);
        EXFAIL_OUT(ret);
    }
    
    /* reset write error back to norm. */
    if (EXSUCCEED!=system("xadmin lcf qwriterr -A 0 -a"))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to enable write failure");
        EXFAIL_OUT(ret);
    }
    
    /* restart tmqueue.... no message shall be available as no files
     * are saved due to write error
     */
    if (EXSUCCEED!=system("xadmin stop -s tmqueue; xadmin start -s tmqueue"))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to stop tmqueue");
        EXFAIL_OUT(ret);
    }
    
    for (i=0; i<1; i++)
    {
        long len=0;
        char *buf;
        buf = tpalloc("CARRAY", "", 100);
        memset(&qc, 0, sizeof(qc));

        /* This shall be updated so that we do not need to use TPNOABORT  */
        if (EXSUCCEED==tpdequeue("MYSPACE", "TEST1", &qc, (char **)&buf, &len, TPNOABORT))
        {
            NDRX_LOG(log_error, "TESTERROR: TEST1 dequeued, even already in progress!");
            EXFAIL_OUT(ret);
        }
        
        tpfree(buf);
    }
    
out:
    
    if (EXSUCCEED!=tpterm())
    {
        NDRX_LOG(log_error, "tpterm failed with: %s", tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }

    return ret;
}

/**
 * tmsrv fails to log transaction - disk full  / abort
 * @param maxmsg
 * @return 
 */
exprivate int basic_tmsrvdiskerr(int maxmsg)
{
    int ret = EXSUCCEED;
    TPQCTL qc;
    int i;
    
    NDRX_LOG(log_error, "case basic_tmsrvdiskerr");
    
    if (EXSUCCEED!=tpbegin(9999, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to begin");
        EXFAIL_OUT(ret);
    }
    
    /* Initial test... */
    for (i=0; i<maxmsg; i++)
    {
        char *testbuf_ref = tpalloc("CARRAY", "", 10);
        long len=10;
        
        if (i==1)
        {
            /* Set disk failure for tmsrv - now tmqueue is joined...
             */
            if (EXSUCCEED!=system("xadmin lcf twriterr -A 1 -a"))
            {
                NDRX_LOG(log_error, "TESTERROR: failed to enable write failure");
                EXFAIL_OUT(ret);
            }
        }

        testbuf_ref[0]=0;
        testbuf_ref[1]=1;
        testbuf_ref[2]=2;
        testbuf_ref[3]=3;
        testbuf_ref[4]=4;
        testbuf_ref[5]=5;
        testbuf_ref[6]=6;
        testbuf_ref[7]=7;
        testbuf_ref[8]=8;
        testbuf_ref[9]=9;

        /* alloc output buffer */
        if (NULL==testbuf_ref)
        {
            NDRX_LOG(log_error, "TESTERROR: tpalloc() failed %s", 
                    tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
        
        /* enqueue the data buffer */
        memset(&qc, 0, sizeof(qc));
        
        if (EXSUCCEED!=tpenqueue("MYSPACE", "TEST1", &qc, testbuf_ref, 
                len, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                    tpstrerror(tperrno), qc.diagnostic, qc.diagmsg);
            EXFAIL_OUT(ret);
        }

        tpfree(testbuf_ref);
    }
    
    /* commit shall fail, as failed to log stuff
     * we shall get abort error
     *  */
    if (EXSUCCEED==tpcommit(0))
    {
        NDRX_LOG(log_error, "TESTERROR: it shall fail to commit, as transactions ar marked for abort!");
        EXFAIL_OUT(ret);
    }
    
    /* stuff shall be rolled back... */
    if (TPEABORT!=tperrno)
    {
        NDRX_LOG(log_error, "TESTERROR: Expected TPEABORT got %d", tperrno);
        EXFAIL_OUT(ret);
    }
    
    /* reset write error back to norm. */
    if (EXSUCCEED!=system("xadmin lcf twriterr -A 0 -a"))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to enable write failure");
        EXFAIL_OUT(ret);
    }
    
    /* no messages in queue, as rolled back.. */
    for (i=0; i<1; i++)
    {
        long len=0;
        char *buf;
        buf = tpalloc("CARRAY", "", 100);
        memset(&qc, 0, sizeof(qc));

        /* This shall be updated so that we do not need to use TPNOABORT  */
        if (EXSUCCEED==tpdequeue("MYSPACE", "TEST1", &qc, (char **)&buf, &len, TPNOABORT))
        {
            NDRX_LOG(log_error, "TESTERROR: TEST1 dequeued, even already in progress!");
            EXFAIL_OUT(ret);
        }
        
        tpfree(buf);
    }
    
out:
    
    if (EXSUCCEED!=tpterm())
    {
        NDRX_LOG(log_error, "tpterm failed with: %s", tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }

    return ret;
}

/**
 * Skip bad messages on the disk
 * @param maxmsg max messages to be ok
 */
exprivate int basic_badmsg(int maxmsg)
{
    int ret = EXSUCCEED;
    TPQCTL qc;
    int i;
    
    NDRX_LOG(log_error, "case basic_badmsg");
    if (EXSUCCEED!=tpbegin(9999, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to begin");
        EXFAIL_OUT(ret);
    }
    
    /* Initial test... */
    for (i=0; i<maxmsg; i++)
    {
        char *testbuf_ref = tpalloc("CARRAY", "", 10);
        long len=10;

        testbuf_ref[0]=0;
        testbuf_ref[1]=1;
        testbuf_ref[2]=2;
        testbuf_ref[3]=3;
        testbuf_ref[4]=4;
        testbuf_ref[5]=5;
        testbuf_ref[6]=6;
        testbuf_ref[7]=7;
        testbuf_ref[8]=8;
        testbuf_ref[9]=9;

        /* alloc output buffer */
        if (NULL==testbuf_ref)
        {
            NDRX_LOG(log_error, "TESTERROR: tpalloc() failed %s", 
                    tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }

        /* enqueue the data buffer */
        memset(&qc, 0, sizeof(qc));
        if (EXSUCCEED!=tpenqueue("MYSPACE", "TEST1", &qc, testbuf_ref, 
                len, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                    tpstrerror(tperrno), qc.diagnostic, qc.diagmsg);
            EXFAIL_OUT(ret);
        }

        tpfree(testbuf_ref);
    }
    
    /* create a bad file */
    if (EXSUCCEED!=system("touch QSPACE1/active/some_bad_message_file"))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to create bad message file...");
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=tpcommit(0))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to commit got: %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=system("xadmin sreload -s tmqueue"))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to retart tmqueue");
        EXFAIL_OUT(ret);
    }
    
    /* all messages must be available - bad file ignored...*/
    for (i=0; i<maxmsg; i++)
    {
        long len=0;
        char *buf;
        buf = tpalloc("CARRAY", "", 100);
        memset(&qc, 0, sizeof(qc));

        if (EXSUCCEED!=tpdequeue("MYSPACE", "TEST1", &qc, (char **)&buf, &len, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: TEST1 failed dequeue!");
            EXFAIL_OUT(ret);
        }
        tpfree(buf);
    }
    
out:
    
    if (EXSUCCEED!=tpterm())
    {
        NDRX_LOG(log_error, "tpterm failed with: %s", tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }

    /* create a bad file */
    if (system("rm QSPACE1/active/some_bad_message_file"))
    {
        /* avoid warning... */
    }
    
    return ret;
}


/**
 * Simulate commit crash & recovery
 * We enqueue data.
 * Commit fails (change stage to committing, thus we perform automatic
 * rollback)
 * @param maxmsg max messages to be ok
 */
exprivate int basic_commit_crash(int maxmsg)
{
    int ret = EXSUCCEED;
    TPQCTL qc;
    int i;
    
    NDRX_LOG(log_error, "case basic_commit_crash");
    if (EXSUCCEED!=tpbegin(9999, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to begin");
        EXFAIL_OUT(ret);
    }
    
    /* Initial test... */
    for (i=0; i<maxmsg; i++)
    {
        char *testbuf_ref = tpalloc("CARRAY", "", 10);
        long len=10;

        testbuf_ref[0]=0;
        testbuf_ref[1]=1;
        testbuf_ref[2]=2;
        testbuf_ref[3]=3;
        testbuf_ref[4]=4;
        testbuf_ref[5]=5;
        testbuf_ref[6]=6;
        testbuf_ref[7]=7;
        testbuf_ref[8]=8;
        testbuf_ref[9]=9;

        /* alloc output buffer */
        if (NULL==testbuf_ref)
        {
            NDRX_LOG(log_error, "TESTERROR: tpalloc() failed %s", 
                    tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }

        /* enqueue the data buffer */
        memset(&qc, 0, sizeof(qc));
        if (EXSUCCEED!=tpenqueue("MYSPACE", "TEST1", &qc, testbuf_ref, 
                len, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                    tpstrerror(tperrno), qc.diagnostic, qc.diagmsg);
            EXFAIL_OUT(ret);
        }

        tpfree(testbuf_ref);
    }
    
    if (EXSUCCEED!=tpcommit(0))
    {
        NDRX_LOG(log_error, "TESTERROR: commit failed: %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    /* start to dequeue... */
    if (EXSUCCEED!=tpbegin(9999, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to begin");
        EXFAIL_OUT(ret);
    }
    
    /* all messages must be available */
    for (i=0; i<maxmsg; i++)
    {
        long len=0;
        char *buf;
        buf = tpalloc("CARRAY", "", 100);
        memset(&qc, 0, sizeof(qc));

        if (EXSUCCEED!=tpdequeue("MYSPACE", "TEST1", &qc, (char **)&buf, &len, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: TEST1 failed dequeue!");
            EXFAIL_OUT(ret);
        }
        
        tpfree(buf);
    }
    
    /* 
     * Set crash point
     */
    if (EXSUCCEED!=system("xadmin lcf tcrash -A 50 -a"))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to enable crash");
        EXFAIL_OUT(ret);
    }
    
    /* set timeout time  
     * commit will fail...
     * and all records will be rolled back (assuming in 30 sec)
     */
    tptoutset(30);
    
    if (EXSUCCEED==tpcommit(0))
    {
        NDRX_LOG(log_error, "TESTERROR: commit must fail");
        EXFAIL_OUT(ret);
    }
    
    if (tperrno!=TPETIME)
    {
        NDRX_LOG(log_error, "TESTERROR: expected TPETIME got %d", tperrno);
        EXFAIL_OUT(ret);
    }
    
    /* set timeout back... */
    tptoutset(90);
    
    if (EXSUCCEED!=system("xadmin lcf tcrash -A 0 -a"))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to disable crash");
        EXFAIL_OUT(ret);
    }
    
    /* all messages must be available (rolled back after crash recovery) */
    for (i=0; i<maxmsg; i++)
    {
        long len=0;
        char *buf;
        buf = tpalloc("CARRAY", "", 100);
        memset(&qc, 0, sizeof(qc));

        if (EXSUCCEED!=tpdequeue("MYSPACE", "TEST1", &qc, (char **)&buf, &len, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: TEST1 failed dequeue!");
            EXFAIL_OUT(ret);
        }
        
        tpfree(buf);
    }
    
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