/* 
** TMQ test client.
**
** @file atmiclt3.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, ATR Baltic, SIA. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or ATR Baltic's license for commercial use.
** -----------------------------------------------------------------------------
** GPL license:
** 
** This program is free software; you can redistribute it and/or modify it under
** the terms of the GNU General Public License as published by the Free Software
** Foundation; either version 2 of the License, or (at your option) any later
** version.
**
** This program is distributed in the hope that it will be useful, but WITHOUT ANY
** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
** PARTICULAR PURPOSE. See the GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License along with
** this program; if not, write to the Free Software Foundation, Inc., 59 Temple
** Place, Suite 330, Boston, MA 02111-1307 USA
**
** -----------------------------------------------------------------------------
** A commercial use license is available from ATR Baltic, SIA
** contact@atrbaltic.com
** -----------------------------------------------------------------------------
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>
#include <test.fd.h>
#include <ndrstandard.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
int basic_q_test(void);
int enq_q_test(void);
int deq_q_test(int do_commit);
int deqempty_q_test(void);
int basic_q_msgid_test(void);
int basic_q_corid_test(void);
int basic_autoq_ok(void);
int basic_rndfail(void);


int main(int argc, char** argv)
{
    int ret = SUCCEED;
    
    if (argc<=1)
    {
        NDRX_LOG(log_error, "usage: %s <test_case: basic|enq|deqa|deqc|deqe>", argv[0]);
        return FAIL;
    }
    NDRX_LOG(log_error, "\n\n\n\n\n !!!!!!!!!!!!!! TEST CASE %s !!!!!!!! \n\n\n\n\n\n", argv[1]);
    
    if (SUCCEED!=tpopen())
    {
        FAIL_OUT(ret);
    }
    
    if (0==strcmp(argv[1], "basic"))
    {
        return basic_q_test();
    }
    else if (0==strcmp(argv[1], "enq"))
    {
        return enq_q_test();
    }
    else if (0==strcmp(argv[1], "deqa"))
    {
        return deq_q_test(FALSE);
    }
    else if (0==strcmp(argv[1], "deqc"))
    {
        return deq_q_test(TRUE);
    }
    else if (0==strcmp(argv[1], "deqe"))
    {
        return deqempty_q_test();
    }
    else if (0==strcmp(argv[1], "msgid"))
    {
        return basic_q_msgid_test();
    }
    else if (0==strcmp(argv[1], "corid"))
    {
        return basic_q_corid_test();
    }
    else if (0==strcmp(argv[1], "autoqok"))
    {
        return basic_autoq_ok();
    }
    else if (0==strcmp(argv[1], "autodeadq"))
    {
        return basic_autoq_deadq();
    }
    else if (0==strcmp(argv[1], "rndfail"))
    {
        return basic_rndfail();
    }
    else
    {
        NDRX_LOG(log_error, "Invalid test case!");
        return FAIL;
    }
    
out:

    tpclose();

    return ret;   
}

/**
 * Do the test call to the server
 */
int basic_q_test(void)
{

    int ret = SUCCEED;
    TPQCTL qc;
    int i;

    /* Initial test... */
    for (i=0; i<1000; i++)
    {
        char *buf = tpalloc("CARRAY", "", 1);
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
        if (NULL==buf)
        {
            NDRX_LOG(log_error, "TESTERROR: tpalloc() failed %s", 
                    tpstrerror(tperrno));
            FAIL_OUT(ret);
        }

        /* enqueue the data buffer */
        memset(&qc, 0, sizeof(qc));
        if (SUCCEED!=tpenqueue("MYSPACE", "TEST1", &qc, testbuf_ref, 
                len, TPNOTRAN))
        {
            NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                    tpstrerror(tperrno), qc.diagnostic, qc.diagmsg);
            FAIL_OUT(ret);
        }

        /* dequeue the data buffer + allocate the output buf. */

        memset(&qc, 0, sizeof(qc));

        len = 10;
        if (SUCCEED!=tpdequeue("MYSPACE", "TEST1", &qc, &buf, 
                &len, TPNOTRAN))
        {
            NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                    tpstrerror(tperrno), qc.diagnostic, qc.diagmsg);
            FAIL_OUT(ret);
        }

        /* compare - should be equal */
        if (0!=memcmp(testbuf_ref, buf, len))
        {
            NDRX_LOG(log_error, "TESTERROR: Buffers not equal!");
            NDRX_DUMP(log_error, "original buffer", testbuf_ref, sizeof(testbuf_ref));
            NDRX_DUMP(log_error, "got form q", buf, len);
            FAIL_OUT(ret);
        }

        tpfree(buf);
        tpfree(testbuf_ref);
    }
    
    if (SUCCEED!=tpterm())
    {
        NDRX_LOG(log_error, "tpterm failed with: %s", tpstrerror(tperrno));
        ret=FAIL;
        goto out;
    }
    
out:
    return ret;
}

/**
 * Enqueue stuff to multiple queues with multiple messages
 * In The same order we shall get the dequeued messages.
 * @return 
 */
int enq_q_test(void)
{
    int ret = SUCCEED;
    TPQCTL qc;
    int i;
    
    UBFH *buf = (UBFH *)tpalloc("UBF", "", 8192);
    
    if (SUCCEED!=tpbegin(90, 0))
    {
        NDRX_LOG(log_error, "TESTERROR! Failed to start transaction!");
        FAIL_OUT(ret);
    } 
    
    /* run into one single global tx, ok?  */
    /* Initial test... */
    for (i=1; i<=300; i++)
    {
        /* enqueue the data buffer */
        if (SUCCEED!=Badd(buf, T_STRING_FLD, "TEST HELLO", 0L))
        {
            NDRX_LOG(log_error, "TESTERROR: failed to set T_STRING_FLD");
            FAIL_OUT(ret);
        }
        
        /* Have a number in FB! */
        memset(&qc, 0, sizeof(qc));
        
        if (SUCCEED!=tpenqueue("MYSPACE", "TESTA", &qc, (char *)buf, 0, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                    tpstrerror(tperrno), qc.diagnostic, qc.diagmsg);
            FAIL_OUT(ret);
        }
        
        memset(&qc, 0, sizeof(qc));
        
        if (SUCCEED!=tpenqueue("MYSPACE", "TESTB", &qc, (char *)buf, 0, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                    tpstrerror(tperrno), qc.diagnostic, qc.diagmsg);
            FAIL_OUT(ret);
        }
        
        memset(&qc, 0, sizeof(qc));
        
        if (SUCCEED!=tpenqueue("MYSPACE", "TESTC", &qc, (char *)buf, 0, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                    tpstrerror(tperrno), qc.diagnostic, qc.diagmsg);
            FAIL_OUT(ret);
        }
        
    }
    tpfree((char *)buf);
    
out:

    if (SUCCEED==ret && SUCCEED!=tpcommit(0))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to commit!");
        ret=FAIL;
    }
    else 
    {
        tpabort(0);
    }

    return ret;
}


/**
 * Enqueue stuff to multiple queues with multiple messages
 * In The same order we shall get the dequeued messages.
 * @return 
 */
int deq_q_test(int do_commit)
{
    int ret = SUCCEED;
    TPQCTL qc;
    int i, j;
    long len;
    UBFH *buf = NULL;
    if (SUCCEED!=tpbegin(90, 0))
    {
        NDRX_LOG(log_error, "TESTERROR! Failed to start transaction!");
        FAIL_OUT(ret);
    } 
    
    /* run into one single global tx, ok?  */
    /* Initial test... */
    for (i=1; i<=300; i++)
    {
        /* Have a number in FB! */
        memset(&qc, 0, sizeof(qc));
        
        buf = (UBFH *)tpalloc("UBF", "", 100);
        if (SUCCEED!=tpdequeue("MYSPACE", "TESTA", &qc, (char **)&buf, &len, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: tpdequeue() failed %s diag: %d:%s", 
                    tpstrerror(tperrno), qc.diagnostic, qc.diagmsg);
            FAIL_OUT(ret);
        }
        
        ndrx_debug_dump_UBF(log_debug, "TESTA rcv buf", buf);
        
        if (i!=Boccur(buf, T_STRING_FLD))
        {
            NDRX_LOG(log_error, "TESTERROR: invalid count for TESTA %d vs %d", 
                    i, Boccur(buf, T_STRING_FLD));
            FAIL_OUT(ret);
        }
        tpfree((char *)buf);
        
        memset(&qc, 0, sizeof(qc));
        
        buf = (UBFH *)tpalloc("UBF", "", 100);
        
        if (SUCCEED!=tpdequeue("MYSPACE", "TESTB", &qc, (char **)&buf, &len, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: tpdequeue() failed %s diag: %d:%s", 
                    tpstrerror(tperrno), qc.diagnostic, qc.diagmsg);
            FAIL_OUT(ret);
        }
        
        ndrx_debug_dump_UBF(log_debug, "TESTB rcv buf", buf);
        
        if (i!=Boccur(buf, T_STRING_FLD))
        {
            NDRX_LOG(log_error, "TESTERROR: invalid count for TESTB %d vs %d", 
                    i, Boccur(buf, T_STRING_FLD));
            FAIL_OUT(ret);
        }
        
        tpfree((char *)buf);
        
        /* Ad some peek tests... for FIFO */
        for (j=0; j<2; j++)
        {
            memset(&qc, 0, sizeof(qc));
            
            if (0==j)
            {
                qc.flags|=TPQPEEK;
            }

            buf = (UBFH *)tpalloc("UBF", "", 100);

            if (SUCCEED!=tpdequeue("MYSPACE", "TESTC", &qc, (char **)&buf, &len, 0))
            {
                NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                        tpstrerror(tperrno), qc.diagnostic, qc.diagmsg);
                FAIL_OUT(ret);
            }

            ndrx_debug_dump_UBF(log_debug, "TESTC rcv buf", buf);

            if (i!=Boccur(buf, T_STRING_FLD))
            {
                NDRX_LOG(log_error, "TESTERROR: invalid count for TESTC %d vs %d", 
                        i, Boccur(buf, T_STRING_FLD));
                FAIL_OUT(ret);
            }

            tpfree((char *)buf);
        }
        
    }

    
out:

    if (do_commit)
    {
        if (SUCCEED!=tpcommit(0))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to commit!");
            ret=FAIL;
        }
    }
    else
    {
        tpabort(0);
    }

    return ret;
}


/**
 * Ensure that queues are empty.
 * @return 
 */
int deqempty_q_test(void)
{
    int ret = SUCCEED;
    TPQCTL qc;
    long len;
    UBFH *buf = NULL;
    
    if (SUCCEED!=tpbegin(90, 0))
    {
        NDRX_LOG(log_error, "TESTERROR! Failed to start transaction!");
        FAIL_OUT(ret);
    } 
    
    buf = (UBFH *)tpalloc("UBF", "", 100);
    if (SUCCEED==tpdequeue("MYSPACE", "TESTA", &qc, (char **)&buf, &len, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: TESTA not empty!");
        FAIL_OUT(ret);
    }

    memset(&qc, 0, sizeof(qc));

    if (SUCCEED==tpdequeue("MYSPACE", "TESTB", &qc, (char **)&buf, &len, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: TESTB not empty!");
        FAIL_OUT(ret);
    }


    memset(&qc, 0, sizeof(qc));

    if (SUCCEED==tpdequeue("MYSPACE", "TESTC", &qc, (char **)buf, &len, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: TESTC not empty!");
        FAIL_OUT(ret);
    }

out:

    tpabort(0);

    return ret;
}


/**
 * Test message get by msgid
 */
int basic_q_msgid_test(void)
{

    int ret = SUCCEED;
    TPQCTL qc1, qc2;
    int i, j;

    /* Initial test... */
    for (i=0; i<1000; i++)
    {
        char *buf = tpalloc("CARRAY", "", 1);
        char *testbuf_ref = tpalloc("CARRAY", "", 1);
        long len=1;
        
        /* alloc output buffer */
        if (NULL==buf || NULL==testbuf_ref)
        {
            NDRX_LOG(log_error, "TESTERROR: tpalloc() failed %s", 
                    tpstrerror(tperrno));
            FAIL_OUT(ret);
        }

        testbuf_ref[0]=101;
        
        /* enqueue the data buffer */
        memset(&qc1, 0, sizeof(qc1));
        if (SUCCEED!=tpenqueue("MYSPACE", "TEST1", &qc1, testbuf_ref, 
                len, TPNOTRAN))
        {
            NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                    tpstrerror(tperrno), qc1.diagnostic, qc1.diagmsg);
            FAIL_OUT(ret);
        }
        
        testbuf_ref[0]=102;
        
        /* enqueue the data buffer */
        memset(&qc2, 0, sizeof(qc2));
        if (SUCCEED!=tpenqueue("MYSPACE", "TEST1", &qc2, testbuf_ref, 
                len, TPNOTRAN))
        {
            NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                    tpstrerror(tperrno), qc2.diagnostic, qc2.diagmsg);
            FAIL_OUT(ret);
        }

        /* dequeue the data buffer + allocate the output buf. */
        /* Have some test with peek... */
        for (j=0; j<2; j++)
        {
            len = 1;
            buf[0] = 0;
            
            if (0 == j)
            {
                qc2.flags|=(TPQGETBYMSGID | TPQPEEK);
            }
            else
            {
                /* Already reset to 0 by first dequeue */
                qc2.flags|=TPQGETBYMSGID;
            }
            
            NDRX_LOG(log_info, "Calling with flags: %ld", qc2.flags);
            if (SUCCEED!=tpdequeue("MYSPACE", "TEST1", &qc2, &buf, 
                    &len, TPNOTRAN))
            {
                NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                        tpstrerror(tperrno), qc1.diagnostic, qc1.diagmsg);
                FAIL_OUT(ret);
            }

            if (102!=buf[0])
            {
                NDRX_LOG(log_error, "TESTERROR: Got %d expected 102", buf[0]);
                FAIL_OUT(ret);

            }
        }
        
        for (j=0; j<2; j++)
        {
            len = 1;
            buf[0] = 0;
            
            if (0 == j)
            {
                qc1.flags|=(TPQGETBYMSGID | TPQPEEK);
            }
            else
            {
                /* Already reset to 0 by first dequeue */
                qc1.flags |= TPQGETBYMSGID;
            }
            if (SUCCEED!=tpdequeue("MYSPACE", "TEST1", &qc1, &buf, 
                    &len, TPNOTRAN))
            {
                NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                        tpstrerror(tperrno), qc1.diagnostic, qc1.diagmsg);
                FAIL_OUT(ret);
            }

            if (101!=buf[0])
            {
                NDRX_LOG(log_error, "TESTERROR: Got %d expected 101", buf[0]);
                FAIL_OUT(ret);
            }
        }
        
        tpfree(buf);
        tpfree(testbuf_ref);
    }
    
    if (SUCCEED!=tpterm())
    {
        NDRX_LOG(log_error, "tpterm failed with: %s", tpstrerror(tperrno));
        ret=FAIL;
        goto out;
    }
    
out:
    return ret;
}

/**
 * Test message get by corid
 */
int basic_q_corid_test(void)
{

    int ret = SUCCEED;
    TPQCTL qc1, qc2;
    int i, j;

    /* Initial test... */
    for (i=0; i<1000; i++)
    {
        char *buf = tpalloc("CARRAY", "", 1);
        char *testbuf_ref = tpalloc("CARRAY", "", 1);
        long len=1;
        
        /* alloc output buffer */
        if (NULL==buf || NULL==testbuf_ref)
        {
            NDRX_LOG(log_error, "TESTERROR: tpalloc() failed %s", 
                    tpstrerror(tperrno));
            FAIL_OUT(ret);
        }

        testbuf_ref[0]=101;
        
        /* enqueue the data buffer */
        memset(&qc1, 0, sizeof(qc1));
        qc1.corrid[0] = 1;
        qc1.corrid[1] = 2;
        qc1.flags|=TPQCORRID;
        if (SUCCEED!=tpenqueue("MYSPACE", "TEST1", &qc1, testbuf_ref, 
                len, TPNOTRAN))
        {
            NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                    tpstrerror(tperrno), qc1.diagnostic, qc1.diagmsg);
            FAIL_OUT(ret);
        }
        
        testbuf_ref[0]=102;
        
        /* enqueue the data buffer */
        memset(&qc2, 0, sizeof(qc2));
        
        qc2.corrid[0] = 3;
        qc2.corrid[1] = 4;
        qc2.flags|=TPQCORRID;
        
        if (SUCCEED!=tpenqueue("MYSPACE", "TEST1", &qc2, testbuf_ref, 
                len, TPNOTRAN))
        {
            NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                    tpstrerror(tperrno), qc2.diagnostic, qc2.diagmsg);
            FAIL_OUT(ret);
        }

        /* dequeue the data buffer + allocate the output buf. */
        /* Have some test with peek... */
        for (j=0; j<2; j++)
        {
            len = 1;
            buf[0] = 0;
            
            memset(&qc2, 0, sizeof(qc2));
        
            qc2.corrid[0] = 3;
            qc2.corrid[1] = 4;
            
            if (0 == j)
            {
                qc2.flags|=(TPQGETBYCORRID | TPQPEEK);
            }
            else
            {
                /* Already reset to 0 by first dequeue */
                qc2.flags|=TPQGETBYCORRID;
            }
            
            NDRX_LOG(log_info, "Calling with flags: %ld", qc2.flags);
            if (SUCCEED!=tpdequeue("MYSPACE", "TEST1", &qc2, &buf, 
                    &len, TPNOTRAN))
            {
                NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                        tpstrerror(tperrno), qc1.diagnostic, qc1.diagmsg);
                FAIL_OUT(ret);
            }

            if (102!=buf[0])
            {
                NDRX_LOG(log_error, "TESTERROR: Got %d expected 102", buf[0]);
                FAIL_OUT(ret);

            }
        }
        
        for (j=0; j<2; j++)
        {
            len = 1;
            buf[0] = 0;
            
            memset(&qc1, 0, sizeof(qc1));
        
            qc1.corrid[0] = 1;
            qc1.corrid[1] = 2;
            
            if (0 == j)
            {
                qc1.flags|=(TPQGETBYCORRID | TPQPEEK);
            }
            else
            {
                /* Already reset to 0 by first dequeue */
                qc1.flags |= TPQGETBYCORRID;
            }
            
            if (SUCCEED!=tpdequeue("MYSPACE", "TEST1", &qc1, &buf, 
                    &len, TPNOTRAN))
            {
                NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                        tpstrerror(tperrno), qc1.diagnostic, qc1.diagmsg);
                FAIL_OUT(ret);
            }

            if (101!=buf[0])
            {
                NDRX_LOG(log_error, "TESTERROR: Got %d expected 101", buf[0]);
                FAIL_OUT(ret);
            }
        }
        
        tpfree(buf);
        tpfree(testbuf_ref);
    }
    
    if (SUCCEED!=tpterm())
    {
        NDRX_LOG(log_error, "tpterm failed with: %s", tpstrerror(tperrno));
        ret=FAIL;
        goto out;
    }
    
out:
    return ret;
}


/**
 * Sending to OK q
 */
int basic_autoq_ok(void)
{
    int ret = SUCCEED;
    TPQCTL qc1;
    long len = 0;
    char *p;
    int i;
    char strbuf[128];
    
    
    for (i=0; i<100; i++)
    {
        UBFH *buf = (UBFH *)tpalloc("UBF", "", 1024);
        if (NULL==buf)
        {
            NDRX_LOG(log_error, "TESTERROR: tpalloc() failed %s", 
                    tpstrerror(tperrno));
            FAIL_OUT(ret);
        }

        sprintf(strbuf, "HELLO FROM SENDER");
        
        if (SUCCEED!=Bchg(buf, T_STRING_2_FLD, 0, strbuf, 0L))
        {
            NDRX_LOG(log_error, "TESTERROR: failed to set T_STRING_2_FLD %s", 
                    Bstrerror(Berror));
            FAIL_OUT(ret);
        }

        /* enqueue the data buffer */
        memset(&qc1, 0, sizeof(qc1));

        qc1.flags|=TPQREPLYQ;

        strcpy(qc1.replyqueue, "REPLYQ");

        if (SUCCEED!=tpenqueue("MYSPACE", "OKQ1", &qc1, (char *)buf, 0, TPNOTRAN))
        {
            NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                    tpstrerror(tperrno), qc1.diagnostic, qc1.diagmsg);
            FAIL_OUT(ret);
        }
        tpfree((char *)buf);
    }
    sleep(60); /* should be enough */
    
    for (i=0; i<100; i++)
    {
        UBFH *buf2 = (UBFH *)tpalloc("UBF", "", 1024);
        memset(&qc1, 0, sizeof(qc1));

        NDRX_LOG(log_warn, "LOOP: %d", i);
        
        if (SUCCEED!=tpdequeue("MYSPACE", "REPLYQ", &qc1, (char **)&buf2, 
                &len, TPNOTRAN))
        {
            NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                    tpstrerror(tperrno), qc1.diagnostic, qc1.diagmsg);
            FAIL_OUT(ret);
        }

        /* Verify that we have fields in place... */
        if (NULL==(p = Bfind(buf2, T_STRING_2_FLD, 0, 0L)))
        {
            NDRX_LOG(log_error, "TESTERROR: failed to get T_STRING_2_FLD %s", 
                    Bstrerror(Berror));
            FAIL_OUT(ret);
        }

        sprintf(strbuf, "HELLO FROM SENDER");
        
        if (0!=strcmp(p, strbuf))
        {
            NDRX_LOG(log_error, "TESTERROR: Invalid value [%s]", p);
            FAIL_OUT(ret);
        }

        /* Verify that we have fields in place... */
        if (NULL==(p = Bfind(buf2, T_STRING_FLD, 0, 0L)))
        {
            NDRX_LOG(log_error, "TESTERROR: failed to get T_STRING_FLD %s", 
                    Bstrerror(Berror));
            FAIL_OUT(ret);
        }

        if (0!=strcmp(p, "OK"))
        {
            NDRX_LOG(log_error, "TESTERROR: Invalid value [%s]", p);
            FAIL_OUT(ret);
        }
        tpfree((char *)buf2);
    }

    if (SUCCEED!=tpterm())
    {
        NDRX_LOG(log_error, "tpterm failed with: %s", tpstrerror(tperrno));
        ret=FAIL;
        goto out;
    }
    
out:
    return ret;
}

/**
 * Test deadq
 */
int basic_autoq_deadq(void)
{
    int ret = SUCCEED;
    TPQCTL qc1;
    long len = 0;
    char *p;
    int i;
    char strbuf[128];
    
    
    for (i=0; i<100; i++)
    {
        UBFH *buf = (UBFH *)tpalloc("UBF", "", 1024);
        if (NULL==buf)
        {
            NDRX_LOG(log_error, "TESTERROR: tpalloc() failed %s", 
                    tpstrerror(tperrno));
            FAIL_OUT(ret);
        }

        sprintf(strbuf, "HELLO FROM SENDER");
        
        if (SUCCEED!=Bchg(buf, T_STRING_2_FLD, 0, strbuf, 0L))
        {
            NDRX_LOG(log_error, "TESTERROR: failed to set T_STRING_2_FLD %s", 
                    Bstrerror(Berror));
            FAIL_OUT(ret);
        }

        /* enqueue the data buffer */
        memset(&qc1, 0, sizeof(qc1));

        qc1.flags|=TPQFAILUREQ;

        strcpy(qc1.failurequeue, "DEADQ");

        if (SUCCEED!=tpenqueue("MYSPACE", "BADQ1", &qc1, (char *)buf, 0, TPNOTRAN))
        {
            NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                    tpstrerror(tperrno), qc1.diagnostic, qc1.diagmsg);
            FAIL_OUT(ret);
        }
        tpfree((char *)buf);
    }
    sleep(60); /* should be enough */
    
    for (i=0; i<100; i++)
    {
        UBFH *buf2 = (UBFH *)tpalloc("UBF", "", 1024);
        memset(&qc1, 0, sizeof(qc1));

        NDRX_LOG(log_warn, "LOOP: %d", i);
        
        if (SUCCEED!=tpdequeue("MYSPACE", "DEADQ", &qc1, (char **)&buf2, 
                &len, TPNOTRAN))
        {
            NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                    tpstrerror(tperrno), qc1.diagnostic, qc1.diagmsg);
            FAIL_OUT(ret);
        }

        /* Verify that we have fields in place... */
        if (NULL==(p = Bfind(buf2, T_STRING_2_FLD, 0, 0L)))
        {
            NDRX_LOG(log_error, "TESTERROR: failed to get T_STRING_2_FLD %s", 
                    Bstrerror(Berror));
            FAIL_OUT(ret);
        }

        sprintf(strbuf, "HELLO FROM SENDER");
        
        if (0!=strcmp(p, strbuf))
        {
            NDRX_LOG(log_error, "TESTERROR: Invalid value [%s]", p);
            FAIL_OUT(ret);
        }

        tpfree((char *)buf2);
    }

    if (SUCCEED!=tpterm())
    {
        NDRX_LOG(log_error, "tpterm failed with: %s", tpstrerror(tperrno));
        ret=FAIL;
        goto out;
    }
    
out:
    return ret;
}

/**
 * Sending to OK q
 */
int basic_rndfail(void)
{
    int ret = SUCCEED;
    TPQCTL qc1;
    long len = 0;
    char *p;
    int i;
    char strbuf[128];
    
    
    for (i=0; i<100; i++)
    {
        UBFH *buf = (UBFH *)tpalloc("UBF", "", 1024);
        if (NULL==buf)
        {
            NDRX_LOG(log_error, "TESTERROR: tpalloc() failed %s", 
                    tpstrerror(tperrno));
            FAIL_OUT(ret);
        }

        sprintf(strbuf, "HELLO FROM SENDER");
        
        if (SUCCEED!=Bchg(buf, T_STRING_2_FLD, 0, strbuf, 0L))
        {
            NDRX_LOG(log_error, "TESTERROR: failed to set T_STRING_2_FLD %s", 
                    Bstrerror(Berror));
            FAIL_OUT(ret);
        }

        /* enqueue the data buffer */
        memset(&qc1, 0, sizeof(qc1));

        qc1.flags|=TPQREPLYQ;

        strcpy(qc1.replyqueue, "REPLYQ");

        if (SUCCEED!=tpenqueue("MYSPACE", "RFQ", &qc1, (char *)buf, 0, TPNOTRAN))
        {
            NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                    tpstrerror(tperrno), qc1.diagnostic, qc1.diagmsg);
            FAIL_OUT(ret);
        }
        tpfree((char *)buf);
    }
    sleep(90); /* should be enough */
    
    for (i=0; i<100; i++)
    {
        UBFH *buf2 = (UBFH *)tpalloc("UBF", "", 1024);
        memset(&qc1, 0, sizeof(qc1));

        NDRX_LOG(log_warn, "LOOP: %d", i);
        
        if (SUCCEED!=tpdequeue("MYSPACE", "REPLYQ", &qc1, (char **)&buf2, 
                &len, TPNOTRAN))
        {
            NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                    tpstrerror(tperrno), qc1.diagnostic, qc1.diagmsg);
            FAIL_OUT(ret);
        }

        /* Verify that we have fields in place... */
        if (NULL==(p = Bfind(buf2, T_STRING_2_FLD, 0, 0L)))
        {
            NDRX_LOG(log_error, "TESTERROR: failed to get T_STRING_2_FLD %s", 
                    Bstrerror(Berror));
            FAIL_OUT(ret);
        }

        sprintf(strbuf, "HELLO FROM SENDER");
        
        if (0!=strcmp(p, strbuf))
        {
            NDRX_LOG(log_error, "TESTERROR: Invalid value [%s]", p);
            FAIL_OUT(ret);
        }

        /* Verify that we have fields in place... */
        if (NULL==(p = Bfind(buf2, T_STRING_FLD, 0, 0L)))
        {
            NDRX_LOG(log_error, "TESTERROR: failed to get T_STRING_FLD %s", 
                    Bstrerror(Berror));
            FAIL_OUT(ret);
        }

        if (0!=strcmp(p, "OK"))
        {
            NDRX_LOG(log_error, "TESTERROR: Invalid value [%s]", p);
            FAIL_OUT(ret);
        }
        tpfree((char *)buf2);
    }

    if (SUCCEED!=tpterm())
    {
        NDRX_LOG(log_error, "tpterm failed with: %s", tpstrerror(tperrno));
        ret=FAIL;
        goto out;
    }
    
out:
    return ret;
}
