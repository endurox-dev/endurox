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
exprivate int basic_commit_shut(int maxmsg);
exprivate int basic_loadprep(int maxmsg);
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
    else if (0==strcmp(argv[1], "commit_shut"))
    {
        return basic_commit_shut(1200);
    }
    else if (0==strcmp(argv[1], "loadprep"))
    {
        return basic_loadprep(1200);
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
 * Try commit, queue is shutdown
 * It shall abort with retries... boot tmqueue back, no messages shall be available
 * @param maxmsg max messages to be ok
 */
exprivate int basic_commit_shut(int maxmsg)
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
 * Load prepared messages (also tmq scans this twice, thus check that no problem)
 * @param maxmsg
 * @return 
 */
exprivate int basic_loadprep(int maxmsg)
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
/* vim: set ts=4 sw=4 et smartindent: */
