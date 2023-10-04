/**
 * @brief TMQ test client.
 *
 * @file atmiclt28.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2023, Mavimax, Ltd. All Rights Reserved.
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
exprivate int basic_q_test(void);
exprivate int basic_bench_q_test();
exprivate int enq_q_test(char *q1, char *q2, char *q3);
exprivate int deq_q_test(int do_commit, int lifo, char *q1, char *q2, char *q3);
exprivate int deqempty_q_test(void);
exprivate int basic_q_msgid_test(void);
exprivate int basic_q_corfifo_test(void);
exprivate int basic_q_corlifo_test(void);
exprivate int basic_q_corauto_test(void);
exprivate int basic_q_deqdefault_test(void);
exprivate int basic_q_cortran_test(void);
exprivate int basic_autoq_ok(void);
exprivate int basic_autoqnr_ok(void);

exprivate int basic_rndfail(void);
exprivate int basic_enqcarray(void);
exprivate int basic_autoq_deadq(void);
exprivate int noabort_q_test(void);

exprivate int basic_q_fut_fifo_test(void);
exprivate int basic_q_fut_lifo_test(void);
exprivate int basic_q_fut_fifo_lifo_auto_test(void);

int main(int argc, char** argv)
{
    int ret = EXSUCCEED;
    
    if (argc<=1)
    {
        NDRX_LOG(log_error, "usage: %s <test_case: basic|enq|deqa|deqc|deqe>", argv[0]);
        return EXFAIL;
    }
    NDRX_LOG(log_error, "\n\n\n\n\n !!!!!!!!!!!!!! TEST CASE %s !!!!!!!! \n\n\n\n\n\n", argv[1]);
    
    if (EXSUCCEED!=tpopen())
    {
        EXFAIL_OUT(ret);
    }
    
    if (0==strcmp(argv[1], "basic"))
    {
        return basic_q_test();
    }
    else if (0==strcmp(argv[1], "basicbench"))
    {
        return basic_bench_q_test();
    }
    else if (0==strcmp(argv[1], "enq"))
    {
        return enq_q_test("TESTA", "TESTB", "TESTC");
    }
    else if (0==strcmp(argv[1], "lenq"))
    {
        return enq_q_test("LTESTA", "LTESTB", "LTESTC");
    }
    else if (0==strcmp(argv[1], "deqa"))
    {
        return deq_q_test(EXFALSE, EXFALSE, "TESTA", "TESTB", "TESTC");
    }
    else if (0==strcmp(argv[1], "deqc"))
    {
        return deq_q_test(EXTRUE, EXFALSE, "TESTA", "TESTB", "TESTC");
    }
    else if (0==strcmp(argv[1], "ldeqa"))
    {
        return deq_q_test(EXFALSE, EXTRUE, "LTESTA", "LTESTB", "LTESTC");
    }
    else if (0==strcmp(argv[1], "ldeqc"))
    {
        return deq_q_test(EXTRUE, EXTRUE, "LTESTA", "LTESTB", "LTESTC");
    }
    else if (0==strcmp(argv[1], "deqe"))
    {
        return deqempty_q_test();
    }
    else if (0==strcmp(argv[1], "msgid"))
    {
        return basic_q_msgid_test();
    }
    else if (0==strcmp(argv[1], "corfifo"))
    {
        return basic_q_corfifo_test();
    }
    else if (0==strcmp(argv[1], "corlifo"))
    {
        return basic_q_corlifo_test();
    }
    else if (0==strcmp(argv[1], "corauto"))
    {
        return basic_q_corauto_test();
    }
    else if (0==strcmp(argv[1], "deqdefault"))
    {
        return basic_q_deqdefault_test();
    }
    else if (0==strcmp(argv[1], "cortran"))
    {
        return basic_q_cortran_test();
    }
    else if (0==strcmp(argv[1], "autoqok"))
    {
        return basic_autoq_ok();
    }
    else if (0==strcmp(argv[1], "autoqnr"))
    {
        return basic_autoqnr_ok();
    }
    else if (0==strcmp(argv[1], "autodeadq"))
    {
        return basic_autoq_deadq();
    }
    else if (0==strcmp(argv[1], "rndfail"))
    {
        return basic_rndfail();
    }
    else if (0==strcmp(argv[1], "carr"))
    {
        return basic_enqcarray();
    }
    else if (0==strcmp(argv[1], "noabort"))
    {
        return noabort_q_test();
    }
    else if (0==strcmp(argv[1], "futfifotrans"))
    {
        return basic_q_fut_fifo_test();
    }
    else if (0==strcmp(argv[1], "futlifotrans"))
    {
        return basic_q_fut_lifo_test();
    }
    else if (0==strcmp(argv[1], "futlifofifoauto"))
    {
        return basic_q_fut_fifo_lifo_auto_test();
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
 * Do the test call to the server, benchmark for docs
 */
exprivate int basic_bench_q_test(void)
{

    int ret = EXSUCCEED;
    TPQCTL qc;
    int i, j;
    ndrx_stopwatch_t timer;
    int call_num = 5000;
    int callsz;
    int first= EXTRUE;
    double cps;
    
    /*start with 1 byte, then with 1 kb, then +4 kb up till 56... */
    for (j=1; j<56; j+=4)
    {
        callsz = j*1024;

        char *buf = tpalloc("CARRAY", "", callsz);
        char *testbuf_ref = tpalloc("CARRAY", "", callsz);
        
warmed_up:
        ndrx_stopwatch_reset(&timer);
        /* Do the loop call! */
        for (i=0; i<call_num; i++) /* Test the cd loop */
        {
            long len=callsz;

            /* enqueue the data buffer */
            memset(&qc, 0, sizeof(qc));
            if (EXSUCCEED!=tpenqueue("MYSPACE", "TEST1", &qc, testbuf_ref, 
                    len, TPNOTRAN))
            {
                NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                        tpstrerror(tperrno), qc.diagnostic, qc.diagmsg);
                EXFAIL_OUT(ret);
            }

            /* dequeue the data buffer + allocate the output buf. */

            memset(&qc, 0, sizeof(qc));

            len = 10;
            if (EXSUCCEED!=tpdequeue("MYSPACE", "TEST1", &qc, &buf, 
                    &len, TPNOTRAN))
            {
                NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                        tpstrerror(tperrno), qc.diagnostic, qc.diagmsg);
                EXFAIL_OUT(ret);
            }

            /* compare - should be equal */
            if (0!=memcmp(testbuf_ref, buf, len))
            {
                NDRX_LOG(log_error, "TESTERROR: Buffers not equal!");
                NDRX_DUMP(log_error, "original buffer", testbuf_ref, sizeof(testbuf_ref));
                NDRX_DUMP(log_error, "got form q", buf, len);
                EXFAIL_OUT(ret);
            }
        }

        /*do the warmup... */
        if (first)
        {
            first = EXFALSE;
            goto warmed_up;
        }

        cps = (double)(call_num)/(double)((double)ndrx_stopwatch_get_delta(&timer)/1000.0f);

        fflush(stdout);

        if (EXSUCCEED!=ndrx_bench_write_stats((double)j, cps))
        {
            NDRX_LOG(log_always, "Failed to write stats!");
            EXFAIL_OUT(ret);
        }
        tpfree(buf);
        tpfree(testbuf_ref);
    }

    if (EXSUCCEED!=tpterm())
    {
        NDRX_LOG(log_error, "tpterm failed with: %s", tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }
    
out:
    return ret;
}

/**
 * Do the test call to the server
 */
exprivate int basic_q_test(void)
{

    int ret = EXSUCCEED;
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
            EXFAIL_OUT(ret);
        }

        /* enqueue the data buffer */
        memset(&qc, 0, sizeof(qc));
        if (EXSUCCEED!=tpenqueue("MYSPACE", "TEST1", &qc, testbuf_ref, 
                len, TPNOTRAN))
        {
            NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                    tpstrerror(tperrno), qc.diagnostic, qc.diagmsg);
            EXFAIL_OUT(ret);
        }

        /* dequeue the data buffer + allocate the output buf. */

        memset(&qc, 0, sizeof(qc));

        len = 10;
        if (EXSUCCEED!=tpdequeue("MYSPACE", "TEST1", &qc, &buf, 
                &len, TPNOTRAN))
        {
            NDRX_LOG(log_error, "TESTERROR: tpdequeue() failed %s diag: %d:%s", 
                    tpstrerror(tperrno), qc.diagnostic, qc.diagmsg);
            EXFAIL_OUT(ret);
        }

        /* compare - should be equal */
        if (0!=memcmp(testbuf_ref, buf, len))
        {
            NDRX_LOG(log_error, "TESTERROR: Buffers not equal!");
            NDRX_DUMP(log_error, "original buffer", testbuf_ref, sizeof(testbuf_ref));
            NDRX_DUMP(log_error, "got form q", buf, len);
            EXFAIL_OUT(ret);
        }

        tpfree(buf);
        tpfree(testbuf_ref);
    }
    
    if (EXSUCCEED!=tpterm())
    {
        NDRX_LOG(log_error, "tpterm failed with: %s", tpstrerror(tperrno));
        ret=EXFAIL;
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
exprivate int enq_q_test(char *q1, char *q2, char *q3)
{
    int ret = EXSUCCEED;
    TPQCTL qc;
    int i;
    
    UBFH *buf = (UBFH *)tpalloc("UBF", "", 8192);
    
    if (EXSUCCEED!=tpbegin(180, 0))
    {
        NDRX_LOG(log_error, "TESTERROR! Failed to start transaction!");
        EXFAIL_OUT(ret);
    } 
    
    /* run into one single global tx, ok?  */
    /* Initial test... */
    for (i=1; i<=300; i++)
    {
        /* enqueue the data buffer */
        if (EXSUCCEED!=Badd(buf, T_STRING_FLD, "TEST HELLO", 0L))
        {
            NDRX_LOG(log_error, "TESTERROR: failed to set T_STRING_FLD");
            EXFAIL_OUT(ret);
        }
        
        /* Have a number in FB! */
        memset(&qc, 0, sizeof(qc));
        
        if (EXSUCCEED!=tpenqueue("MYSPACE", q1, &qc, (char *)buf, 0, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                    tpstrerror(tperrno), qc.diagnostic, qc.diagmsg);
            EXFAIL_OUT(ret);
        }
        
        memset(&qc, 0, sizeof(qc));
        
        if (EXSUCCEED!=tpenqueue("MYSPACE", q2, &qc, (char *)buf, 0, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                    tpstrerror(tperrno), qc.diagnostic, qc.diagmsg);
            EXFAIL_OUT(ret);
        }
        
        memset(&qc, 0, sizeof(qc));
        
        if (EXSUCCEED!=tpenqueue("MYSPACE", q3, &qc, (char *)buf, 0, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                    tpstrerror(tperrno), qc.diagnostic, qc.diagmsg);
            EXFAIL_OUT(ret);
        }
        
    }
    tpfree((char *)buf);
    
out:

    if (EXSUCCEED==ret && EXSUCCEED!=tpcommit(0))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to commit!");
        ret=EXFAIL;
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
exprivate int deq_q_test(int do_commit, int lifo, char *q1, char *q2, char *q3)
{
    int ret = EXSUCCEED;
    TPQCTL qc;
    int i, j;
    long len;
    UBFH *buf = NULL;
    if (EXSUCCEED!=tpbegin(180, 0))
    {
        NDRX_LOG(log_error, "TESTERROR! Failed to start transaction!");
        EXFAIL_OUT(ret);
    } 
    
    if (lifo)
    {
        i = 300;
    }
    else
    {
        i = 1;
    }
    
    /* run into one single global tx, ok?  */
    /* Initial test... */
    for (; (lifo?i>=1:i<=300); (lifo?i--:i++))
    {
        /* Have a number in FB! */
        memset(&qc, 0, sizeof(qc));
        
        buf = (UBFH *)tpalloc("UBF", "", 100);
        if (EXSUCCEED!=tpdequeue("MYSPACE", q1, &qc, (char **)&buf, &len, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: tpdequeue() %d failed %s diag: %d:%s", 
                    i, tpstrerror(tperrno), qc.diagnostic, qc.diagmsg);
            EXFAIL_OUT(ret);
        }
        
        ndrx_debug_dump_UBF(log_debug, "TESTA rcv buf", buf);
        
        if (i!=Boccur(buf, T_STRING_FLD))
        {
            NDRX_LOG(log_error, "TESTERROR: invalid count for TESTA %d vs %d", 
                    i, Boccur(buf, T_STRING_FLD));
            EXFAIL_OUT(ret);
        }
        tpfree((char *)buf);
        
        memset(&qc, 0, sizeof(qc));
        
        buf = (UBFH *)tpalloc("UBF", "", 100);
        
        if (EXSUCCEED!=tpdequeue("MYSPACE", q2, &qc, (char **)&buf, &len, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: tpdequeue() failed %s diag: %d:%s", 
                    tpstrerror(tperrno), qc.diagnostic, qc.diagmsg);
            EXFAIL_OUT(ret);
        }
        
        ndrx_debug_dump_UBF(log_debug, "TESTB rcv buf", buf);
        
        if (i!=Boccur(buf, T_STRING_FLD))
        {
            NDRX_LOG(log_error, "TESTERROR: invalid count for TESTB %d vs %d", 
                    i, Boccur(buf, T_STRING_FLD));
            EXFAIL_OUT(ret);
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

            if (EXSUCCEED!=tpdequeue("MYSPACE", q3, &qc, (char **)&buf, &len, 0))
            {
                NDRX_LOG(log_error, "TESTERROR: tpdequeue() failed %s diag: %d:%s i=%d j=%d", 
                        tpstrerror(tperrno), qc.diagnostic, qc.diagmsg, i, j);
                EXFAIL_OUT(ret);
            }

            ndrx_debug_dump_UBF(log_debug, "TESTC rcv buf", buf);

            if (i!=Boccur(buf, T_STRING_FLD))
            {
                NDRX_LOG(log_error, "TESTERROR: invalid count for TESTC %d vs %d", 
                        i, Boccur(buf, T_STRING_FLD));
                EXFAIL_OUT(ret);
            }

            tpfree((char *)buf);
        }
        
    }

    
out:

    if (do_commit)
    {
        if (EXSUCCEED!=tpcommit(0))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to commit!");
            ret=EXFAIL;
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
exprivate int deqempty_q_test(void)
{
    int ret = EXSUCCEED;
    TPQCTL qc;
    long len;
    UBFH *buf = NULL;
    
    if (EXSUCCEED!=tpbegin(90, 0))
    {
        NDRX_LOG(log_error, "TESTERROR! Failed to start transaction!");
        EXFAIL_OUT(ret);
    } 
    
    buf = (UBFH *)tpalloc("UBF", "", 100);
    memset(&qc, 0, sizeof(qc));
    
    if (EXSUCCEED==tpdequeue("MYSPACE", "TESTA", &qc, (char **)&buf, &len, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: TESTA not empty!");
        EXFAIL_OUT(ret);
    }

    memset(&qc, 0, sizeof(qc));

    if (EXSUCCEED==tpdequeue("MYSPACE", "TESTB", &qc, (char **)&buf, &len, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: TESTB not empty!");
        EXFAIL_OUT(ret);
    }

    memset(&qc, 0, sizeof(qc));

    if (EXSUCCEED==tpdequeue("MYSPACE", "TESTC", &qc, (char **)buf, &len, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: TESTC not empty!");
        EXFAIL_OUT(ret);
    }

out:

    tpabort(0);

    return ret;
}

/**
 * Ensure that queues are empty (no tran abort..)
 * @return 
 */
exprivate int noabort_q_test(void)
{
    int ret = EXSUCCEED;
    TPQCTL qc;
    long len;
    UBFH *buf = (UBFH *)tpalloc("UBF", "", 1024);
    int i;
    
    if (NULL==buf)
    {
        NDRX_LOG(log_error, "TESTERROR: failed to allocate buffer: %s", 
                tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    for (i=0; i<5000; i++)
    {
        /**********************************************************************/
        NDRX_LOG(log_warn, "No abort on empty Q");
        /**********************************************************************/
        if (EXSUCCEED!=tpbegin(90, 0))
        {
            NDRX_LOG(log_error, "TESTERROR! Failed to start transaction!");
            EXFAIL_OUT(ret);
        } 

        memset(&qc, 0, sizeof(qc));

        if (EXSUCCEED==tpdequeue("MYSPACE", "TESTA", &qc, (char **)&buf, &len, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: TESTA not empty!");
            EXFAIL_OUT(ret);
        }

        if (EXSUCCEED!=tpcommit(0L))
        {
            NDRX_LOG(log_error, "TESTERROR! Transaction MUST BE committed but fail: %s",
                    tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }

    }
out:
    if (NULL!=buf)
    {
        tpfree((char *)buf);
    }
     
    return ret;
}

/**
 * Test message get by msgid
 */
exprivate int basic_q_msgid_test(void)
{

    int ret = EXSUCCEED;
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
            EXFAIL_OUT(ret);
        }

        testbuf_ref[0]=101;
        
        /* enqueue the data buffer */
        memset(&qc1, 0, sizeof(qc1));
        if (EXSUCCEED!=tpenqueue("MYSPACE", "TEST1", &qc1, testbuf_ref, 
                len, TPNOTRAN))
        {
            NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                    tpstrerror(tperrno), qc1.diagnostic, qc1.diagmsg);
            EXFAIL_OUT(ret);
        }
        
        testbuf_ref[0]=102;
        
        /* enqueue the data buffer */
        memset(&qc2, 0, sizeof(qc2));
        if (EXSUCCEED!=tpenqueue("MYSPACE", "TEST1", &qc2, testbuf_ref, 
                len, TPNOTRAN))
        {
            NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                    tpstrerror(tperrno), qc2.diagnostic, qc2.diagmsg);
            EXFAIL_OUT(ret);
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
            if (EXSUCCEED!=tpdequeue("MYSPACE", "TEST1", &qc2, &buf, 
                    &len, TPNOTRAN))
            {
                NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s i=%d j=%d", 
                        tpstrerror(tperrno), qc1.diagnostic, qc1.diagmsg, i, j);
                EXFAIL_OUT(ret);
            }

            if (102!=buf[0])
            {
                NDRX_LOG(log_error, "TESTERROR: Got %d expected 102", buf[0]);
                EXFAIL_OUT(ret);

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
            if (EXSUCCEED!=tpdequeue("MYSPACE", "TEST1", &qc1, &buf, 
                    &len, TPNOTRAN))
            {
                NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s i=%d j=%d", 
                        tpstrerror(tperrno), qc1.diagnostic, qc1.diagmsg, i, j);
                EXFAIL_OUT(ret);
            }

            if (101!=buf[0])
            {
                NDRX_LOG(log_error, "TESTERROR: Got %d expected 101", buf[0]);
                EXFAIL_OUT(ret);
            }
        }
        
        tpfree(buf);
        tpfree(testbuf_ref);
    }
    
    if (EXSUCCEED!=tpterm())
    {
        NDRX_LOG(log_error, "tpterm failed with: %s", tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }
    
out:
    return ret;
}

/**
 * Test message get by corid
 * - Add 2x queues: CORFIFO, CORLIFO
 * - Load normal messages
 * - Load correlated messages
 * - read correlated messages, ensure getting QMENOMSG when done - fifo
 * - read correlated + QMENOMSG - lifo
 * -- Re-do above after load correlated msgs + tmq restart (load sort valiate)
 * - read normal messages, all shall be in place.
 */
exprivate int basic_q_corfifo_test(void)
{

    int ret = EXSUCCEED;
    TPQCTL qc1;
    int test;
    char c,cor;
    long len;
    char *buf = tpalloc("CARRAY", "", 3);
        
    if (NULL==buf)
    {
        NDRX_LOG(log_error, "TESTERROR: failed to malloc 3 bytes: %s",
                tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    /* enqueue messages to fifo q / cor & non cor */
    for (test=0; test<2; test++)
    {
        /* load correlated msgs... */
        for (cor=2; cor<5; cor++)
        {
            /* load correlated msgs.. */
            for (c=5; c<126; c++)
            {
                buf[0]=1;
                buf[1]=c;
                buf[2]=cor;
                
                memset(&qc1, 0, sizeof(qc1));
                qc1.flags|=TPQCORRID;
                qc1.corrid[0]=cor;
                
                if (EXSUCCEED!=tpenqueue("MYSPACE", "CORFIFO", &qc1, buf, 3, 0))
                {
                    NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                            tpstrerror(tperrno), qc1.diagnostic, qc1.diagmsg);
                    EXFAIL_OUT(ret);
                }
                
            }
        }

        /* load non correlated msgs.. */
        for (c=0; c<126; c++)
        {
            buf[0]=0;
            buf[1]=c;
            buf[2]=0;

            memset(&qc1, 0, sizeof(qc1));
            if (EXSUCCEED!=tpenqueue("MYSPACE", "CORFIFO", &qc1, buf, 3, 0))
            {
                NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                        tpstrerror(tperrno), qc1.diagnostic, qc1.diagmsg);
                EXFAIL_OUT(ret);
            }
        }

        /* restart tmqueue to get  */
        if (1==test)
        {
            if (EXSUCCEED!=system("xadmin restart tmqueue"))
            {
                NDRX_LOG(log_error, "TESTERROR: failed to restart tmqueue", 
                        strerror(errno));
                EXFAIL_OUT(ret);
            }
        }

        /* Now fetch first message, shall be corelated */
        memset(&qc1, 0, sizeof(qc1));
        len=3;
        if (EXSUCCEED!=tpdequeue("MYSPACE", "CORFIFO", &qc1, &buf, &len, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: tpdequeue() failed %s diag: %d:%s", 
                    tpstrerror(tperrno), qc1.diagnostic, qc1.diagmsg);
            EXFAIL_OUT(ret);
        }

        /* it shall be msg with cor */
        NDRX_ASSERT_VAL_OUT((buf[0]==1 && buf[1]==5 && buf[2]==2 && len==3 && (qc1.flags & TPQCORRID)), 
                "Invalid buffer %d %d %d %ld %lx", 
                (int)buf[0], (int)buf[1], (int)buf[2], len, qc1.flags);

        /* correlator must be set */
        NDRX_ASSERT_VAL_OUT(qc1.corrid[0]==2, 
                "Invalid cor %d", 
                (int)qc1.corrid[0]);

        /* Download all by cor... + last one is QMENOMSG */
        for (cor=2; cor<5; cor++)
        {
            /* load correlated msgs.. 
             * Also c=5/cor=2 is already fetched
             */
            for (c=5; c<127; c++)
            {
                if (5==c && 2==cor)
                {
                    continue;
                }
                
                memset(&qc1, 0, sizeof(qc1));
                len=3;
                /* get by corid */
                qc1.flags|=TPQGETBYCORRID;
                qc1.corrid[0]=cor;
                if (EXSUCCEED!=tpdequeue("MYSPACE", "CORFIFO", &qc1, &buf, &len, 0))
                {
                    if (!(c==126 && TPEDIAGNOSTIC==tperrno && QMENOMSG==qc1.diagnostic))
                    {
                        NDRX_LOG(log_error, "TESTERROR: tpdequeue() failed %s diag: %d:%s", 
                                tpstrerror(tperrno), qc1.diagnostic, qc1.diagmsg);
                        EXFAIL_OUT(ret);
                    }
                    else
                    {
                        continue;
                    }
                }
                
                /* validate the msg */
                NDRX_ASSERT_VAL_OUT((buf[0]==1 && buf[1]==c && buf[2]==cor && len==3 && (qc1.flags & TPQCORRID)), 
                        "Invalid buffer %d (exp %d) %d (exp %d) %d (exp %d) %ld %lx", 
                        (int)buf[0], 1, (int)buf[1], (int)c, (int)buf[2], (int)cor, len, qc1.flags);
                
                /* correlator must be set */
                NDRX_ASSERT_VAL_OUT(qc1.corrid[0]==cor, 
                        "Invalid cor %d (exp %d)", 
                        (int)qc1.corrid[0], (int)cor);
            }
        }
        
        /* Download normal msgs... */
        for (c=0; c<127; c++)
        {
            memset(&qc1, 0, sizeof(qc1));
            len=3;
            
            if (EXSUCCEED!=tpdequeue("MYSPACE", "CORFIFO", &qc1, &buf, &len, 0))
            {
                /* validate that we fetch all and terminate with EOS... */
                if (!(c==126 && TPEDIAGNOSTIC==tperrno && QMENOMSG==qc1.diagnostic))
                {
                    NDRX_LOG(log_error, "TESTERROR: tpdequeue() failed %s diag: %d:%s", 
                            tpstrerror(tperrno), qc1.diagnostic, qc1.diagmsg);
                    EXFAIL_OUT(ret);
                }
                else
                {
                    continue;
                }
            }
            
            /* validate the normal msg... */
            NDRX_ASSERT_VAL_OUT((buf[0]==0 && buf[1]==c && buf[2]==0 && len==3), 
                    "Invalid buffer %d (exp %d) %d (exp %d) %d (exp %d) %ld", 
                    (int)buf[0], 0, (int)buf[1], (int)c, (int)buf[2], 0, len);

            /* correlator must be set */
            NDRX_ASSERT_VAL_OUT(qc1.corrid[0]==0 && !(qc1.flags & TPQCORRID), 
                    "Invalid cor %d (exp %d) %lx", 
                    (int)qc1.corrid[0], 0, qc1.flags);
        }
    }

    
    
out:

    /* finish it off */
    if (NULL!=buf)
    {
        tpfree(buf);
    }

    if (EXSUCCEED!=tpterm())
    {
        NDRX_LOG(log_error, "tpterm failed with: %s", tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }

    return ret;
}

/**
 * Test that transactino marking is working on dequeue
 * @return EXSUCCEED/EXFAIL
 */
exprivate int basic_q_cortran_test(void)
{
    int ret = EXSUCCEED;
    TPQCTL qc1;
    long len;
    char *buf = tpalloc("CARRAY", "", 3);
    
    if (NULL==buf)
    {
        NDRX_LOG(log_error, "TESTERROR: failed to malloc 3 bytes: %s",
                tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    buf[0]=1;
    buf[1]=2;
    buf[2]=3;

    memset(&qc1, 0, sizeof(qc1));
    qc1.flags|=TPQCORRID;
    qc1.corrid[0]=3;

    if (EXSUCCEED!=tpenqueue("MYSPACE", "CORFIFO", &qc1, buf, 3, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                tpstrerror(tperrno), qc1.diagnostic, qc1.diagmsg);
        EXFAIL_OUT(ret);
    }
    
    NDRX_ASSERT_TP_OUT(EXSUCCEED==tpbegin(60, 0), "Failed to start tran...");
    
    memset(&qc1, 0, sizeof(qc1));
    len=3;
    /* get by corid */
    qc1.flags|=TPQGETBYCORRID;
    qc1.corrid[0]=3;
    if (EXSUCCEED!=tpdequeue("MYSPACE", "CORFIFO", &qc1, &buf, &len, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: tpdequeue() failed %s diag: %d:%s", 
                tpstrerror(tperrno), qc1.diagnostic, qc1.diagmsg);
        EXFAIL_OUT(ret);
    }
    
    memset(&qc1, 0, sizeof(qc1));
    len=3;
    /* get by corid */
    qc1.flags|=TPQGETBYCORRID;
    qc1.corrid[0]=3;
    if (EXSUCCEED==tpdequeue("MYSPACE", "CORFIFO", &qc1, &buf, &len, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: tpdequeue() must fail but succeed!");
        EXFAIL_OUT(ret);
    }
    
    NDRX_ASSERT_VAL_OUT(TPEDIAGNOSTIC==tperrno && QMENOMSG==qc1.diagnostic, 
            "Expected QMENOMSG got %d %ld", tperrno, qc1.diagnostic);

    NDRX_ASSERT_TP_OUT(EXSUCCEED==tpcommit(0), "Failed to commit...");
    
    /* Expect no msg: */
    memset(&qc1, 0, sizeof(qc1));
    len=3;
    if (EXSUCCEED==tpdequeue("MYSPACE", "CORFIFO", &qc1, &buf, &len, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: tpdequeue() must fail but succeed!");
        EXFAIL_OUT(ret);
    }
    
    NDRX_ASSERT_VAL_OUT(TPEDIAGNOSTIC==tperrno && QMENOMSG==qc1.diagnostic, 
            "Expected QMENOMSG got %d %ld", tperrno, qc1.diagnostic);
    
out:

    /* finish it off */
    if (NULL!=buf)
    {
        tpfree(buf);
    }

    if (EXSUCCEED!=tpterm())
    {
        NDRX_LOG(log_error, "tpterm failed with: %s", tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }

    return ret;
}


/**
 * Process messages in lifo order
 * @return 
 */
exprivate int basic_q_corlifo_test(void)
{

    int ret = EXSUCCEED;
    TPQCTL qc1;
    int test;
    signed char c,cor;
    long len;
    char *buf = tpalloc("CARRAY", "", 3);
        
    if (NULL==buf)
    {
        NDRX_LOG(log_error, "TESTERROR: failed to malloc 3 bytes: %s",
                tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    /* enqueue messages to fifo q / cor & non cor */
    for (test=0; test<2; test++)
    {
        
        /* load non correlated msgs.. */
        for (c=1; c<126; c++)
        {
            buf[0]=0;
            buf[1]=c;
            buf[2]=0;

            memset(&qc1, 0, sizeof(qc1));
            if (EXSUCCEED!=tpenqueue("MYSPACE", "CORLIFO", &qc1, buf, 3, 0))
            {
                NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                        tpstrerror(tperrno), qc1.diagnostic, qc1.diagmsg);
                EXFAIL_OUT(ret);
            }
        }
        
        /* load correlated msgs... */
        for (cor=2; cor<5; cor++)
        {
            /* load correlated msgs.. */
            for (c=5; c<126; c++)
            {
                buf[0]=1;
                buf[1]=c;
                buf[2]=cor;
                
                memset(&qc1, 0, sizeof(qc1));
                qc1.flags|=TPQCORRID;
                qc1.corrid[0]=cor;
                
                if (EXSUCCEED!=tpenqueue("MYSPACE", "CORLIFO", &qc1, buf, 3, 0))
                {
                    NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                            tpstrerror(tperrno), qc1.diagnostic, qc1.diagmsg);
                    EXFAIL_OUT(ret);
                }
                
            }
        }

        /* restart tmqueue to get  */
        if (1==test)
        {
            if (EXSUCCEED!=system("xadmin restart tmqueue"))
            {
                NDRX_LOG(log_error, "TESTERROR: failed to restart tmqueue", 
                        strerror(errno));
                EXFAIL_OUT(ret);
            }
        }

        /* Now fetch first message, shall be corelated */
        memset(&qc1, 0, sizeof(qc1));
        len=3;
        if (EXSUCCEED!=tpdequeue("MYSPACE", "CORLIFO", &qc1, &buf, &len, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: tpdequeue() failed %s diag: %d:%s", 
                    tpstrerror(tperrno), qc1.diagnostic, qc1.diagmsg);
            EXFAIL_OUT(ret);
        }

        /* it shall be msg with cor */
        NDRX_ASSERT_VAL_OUT((buf[0]==1 && buf[1]==125 && buf[2]==4 && len==3 && (qc1.flags & TPQCORRID)), 
                "Invalid buffer %d %d %d %ld %lx", 
                (int)buf[0], (int)buf[1], (int)buf[2], len, qc1.flags);

        /* correlator must be set */
        NDRX_ASSERT_VAL_OUT(qc1.corrid[0]==4, 
                "Invalid cor %d", 
                (int)qc1.corrid[0]);

        /* Download all by cor... + last one is QMENOMSG */
        for (cor=2; cor<5; cor++)
        {
            /* load correlated msgs.. 
             * Also c=125/cor=4 is already fetched
             */
            for (c=125; c>=4; c--)
            {
                if (125==c && 4==cor)
                {
                    continue;
                }
                
                memset(&qc1, 0, sizeof(qc1));
                len=3;
                /* get by corid */
                qc1.flags|=TPQGETBYCORRID;
                qc1.corrid[0]=cor;
                if (EXSUCCEED!=tpdequeue("MYSPACE", "CORLIFO", &qc1, &buf, &len, 0))
                {
                    if (!(c==4 && TPEDIAGNOSTIC==tperrno && QMENOMSG==qc1.diagnostic))
                    {
                        NDRX_LOG(log_error, "TESTERROR: tpdequeue() failed %s diag: %d:%s", 
                                tpstrerror(tperrno), qc1.diagnostic, qc1.diagmsg);
                        EXFAIL_OUT(ret);
                    }
                    else
                    {
                        continue;
                    }
                }
                
                /* validate the msg */
                NDRX_ASSERT_VAL_OUT((buf[0]==1 && buf[1]==c && buf[2]==cor && len==3 && (qc1.flags & TPQCORRID)), 
                        "Invalid buffer %d (exp %d) %d (exp %d) %d (exp %d) %ld %lx", 
                        (int)buf[0], 1, (int)buf[1], (int)c, (int)buf[2], (int)cor, len, qc1.flags);
                
                /* correlator must be set */
                NDRX_ASSERT_VAL_OUT(qc1.corrid[0]==cor, 
                        "Invalid cor %d (exp %d)", 
                        (int)qc1.corrid[0], (int)cor);
            }
        }
        
        /* Download normal msgs... */
        for (c=125; c>=0; c--)
        {
            memset(&qc1, 0, sizeof(qc1));
            len=3;
            
            if (EXSUCCEED!=tpdequeue("MYSPACE", "CORLIFO", &qc1, &buf, &len, 0))
            {
                /* validate that we fetch all and terminate with EOS... */
                if (!(0==c && TPEDIAGNOSTIC==tperrno && QMENOMSG==qc1.diagnostic))
                {
                    NDRX_LOG(log_error, "TESTERROR: tpdequeue() failed on %d - %s diag: %d:%s", 
                            (int)c, tpstrerror(tperrno), qc1.diagnostic, qc1.diagmsg);
                    EXFAIL_OUT(ret);
                }
                else
                {
                    continue;
                }
            }
            
            /* validate the normal msg... */
            NDRX_ASSERT_VAL_OUT((buf[0]==0 && buf[1]==c && buf[2]==0 && len==3), 
                    "Invalid buffer %d (exp %d) %d (exp %d) %d (exp %d) %ld", 
                    (int)buf[0], 0, (int)buf[1], (int)c, (int)buf[2], 0, len);

            /* correlator must be set */
            NDRX_ASSERT_VAL_OUT(qc1.corrid[0]==0 && !(qc1.flags & TPQCORRID), 
                    "Invalid cor %d (exp %d) %lx", 
                    (int)qc1.corrid[0], 0, qc1.flags);
        }
    }

    
out:

    /* finish it off */
    if (NULL!=buf)
    {
        tpfree(buf);
    }

    if (EXSUCCEED!=tpterm())
    {
        NDRX_LOG(log_error, "tpterm failed with: %s", tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }

    return ret;
}

/**
 * When dequeuing from default Q (if have defaults)
 *  return QMENOMSG instead of QMEINVAL
 * @return 
 */
exprivate int basic_q_deqdefault_test(void)
{
    int ret = EXSUCCEED;
    TPQCTL qc1;
    
    long len;
    char *buf = tpalloc("CARRAY", "", 3);
    
    if (NULL==buf)
    {
        NDRX_LOG(log_error, "TESTERROR: failed to malloc 3 bytes: %s",
                tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    memset(&qc1, 0, sizeof(qc1));
    len=3;
    
    ret=tpdequeue("MYSPACE", "NO_SUCH_Q", &qc1, &buf, &len, 0);
    
    if (!(EXFAIL==ret && TPEDIAGNOSTIC==tperrno && QMENOMSG==qc1.diagnostic))
    {
        NDRX_LOG(log_error, "TESTERROR: tpdequeue() failed %s diag: %d:%s", 
                tpstrerror(tperrno), qc1.diagnostic, qc1.diagmsg);
        EXFAIL_OUT(ret);
    }
    
    ret = EXSUCCEED;
    
out:

    if (NULL!=buf)
    {
        tpfree(buf);
    }

    tpterm();

    return ret;
}

/**
 * Keeps the corrid when forward enqueues to errorq
 * @return EXSUCCEED/EXFAIL
 */
exprivate int basic_q_corauto_test(void)
{
    int ret = EXSUCCEED;
    TPQCTL qc1;
    char c,cor, i;
    long len;
    char *buf = tpalloc("CARRAY", "", 3);
        
    if (NULL==buf)
    {
        NDRX_LOG(log_error, "TESTERROR: failed to malloc 3 bytes: %s",
                tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    /* load correlated msgs... */
    for (cor=2; cor<50; cor++)
    {
        for (i=0; i<2; i++)
        {
            buf[0]=1;
            buf[1]=i;
            buf[2]=cor;

            memset(&qc1, 0, sizeof(qc1));
            qc1.flags|=TPQCORRID;
            qc1.corrid[0]=cor;

            if (EXSUCCEED!=tpenqueue("MYSPACE", "CORAUTO", &qc1, buf, 3, 0))
            {
                NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                        tpstrerror(tperrno), qc1.diagnostic, qc1.diagmsg);
                EXFAIL_OUT(ret);
            }
        }

    }
    
    sleep(30);
    
    /* Download all by cor... + last one is QMENOMSG */
    for (cor=2; cor<50; cor++)
    {            
        for (i=0; i<2; i++)
        {
            memset(&qc1, 0, sizeof(qc1));
            len=3;
            /* get by corid */
            qc1.flags|=TPQGETBYCORRID;
            qc1.corrid[0]=cor;
            if (EXSUCCEED!=tpdequeue("MYSPACE", "CORERR", &qc1, &buf, &len, 0))
            {
                NDRX_LOG(log_error, "TESTERROR: tpdequeue() failed %s diag: %d:%s", 
                        tpstrerror(tperrno), qc1.diagnostic, qc1.diagmsg);
                EXFAIL_OUT(ret);
            }

            /* validate the msg */
            NDRX_ASSERT_VAL_OUT((buf[0]==1 && (buf[1]==0||buf[1]==1) 
                        && buf[2]==cor && len==3 && (qc1.flags & TPQCORRID)), 
                    "Invalid buffer %d (exp %d) %d (exp 0|1) %d (exp %d) %ld %lx", 
                    (int)buf[0], 1, (int)buf[1], (int)buf[2], (int)cor, len, qc1.flags);
            
            /* correlator must be set */
            NDRX_ASSERT_VAL_OUT(qc1.corrid[0]==cor, 
                    "Invalid cor %d (exp %d)", 
                    (int)qc1.corrid[0], (int)cor);
        }
    }
    
    /* Main q shall be empty..*/
    
    ret=tpdequeue("MYSPACE", "CORAUTO", &qc1, &buf, &len, 0);
    
    if (!(EXFAIL==ret && TPEDIAGNOSTIC==tperrno && QMENOMSG==qc1.diagnostic))
    {
        NDRX_LOG(log_error, "TESTERROR: tpdequeue() failed %s diag: %d:%s", 
                tpstrerror(tperrno), qc1.diagnostic, qc1.diagmsg);
        EXFAIL_OUT(ret);
    }
    
    ret = EXSUCCEED;
    
out:

    /* finish it off */
    if (NULL!=buf)
    {
        tpfree(buf);
    }

    if (EXSUCCEED!=tpterm())
    {
        NDRX_LOG(log_error, "tpterm failed with: %s", tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }

    return ret;
}

/**
 * Sending to OK q.
 * So answers of the forward and put in the reply queue.
 * we wait for it to be filled up.
 */
exprivate int basic_autoq_ok(void)
{
    int ret = EXSUCCEED;
    TPQCTL qc1;
    long len = 0;
    char *p;
    int i;
    char strbuf[128];
    char *buf3=NULL;
    
    for (i=0; i<100; i++)
    {
        UBFH *buf = (UBFH *)tpalloc("UBF", "", 1024);
        if (NULL==buf)
        {
            NDRX_LOG(log_error, "TESTERROR: tpalloc() failed %s", 
                    tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }

        sprintf(strbuf, "HELLO FROM SENDER");
        
        if (EXSUCCEED!=Bchg(buf, T_STRING_2_FLD, 0, strbuf, 0L))
        {
            NDRX_LOG(log_error, "TESTERROR: failed to set T_STRING_2_FLD %s", 
                    Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }

        /* enqueue the data buffer */
        memset(&qc1, 0, sizeof(qc1));

        qc1.flags|=TPQREPLYQ;

        strcpy(qc1.replyqueue, "REPLYQ");

        if (EXSUCCEED!=tpenqueue("MYSPACE", "OKQ1", &qc1, (char *)buf, 0, TPNOTRAN))
        {
            NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                    tpstrerror(tperrno), qc1.diagnostic, qc1.diagmsg);
            EXFAIL_OUT(ret);
        }
        tpfree((char *)buf);
    }
    
    sleep(30); /* should be enough */
    
    for (i=0; i<100; i++)
    {
        UBFH *buf2 = (UBFH *)tpalloc("UBF", "", 1024);
        memset(&qc1, 0, sizeof(qc1));

        NDRX_LOG(log_warn, "LOOP: %d", i);
        
        if (EXSUCCEED!=tpdequeue("MYSPACE", "REPLYQ", &qc1, (char **)&buf2, 
                &len, TPNOTRAN))
        {
            NDRX_LOG(log_error, "TESTERROR: tpdequeue() failed %s diag: %d:%s", 
                    tpstrerror(tperrno), qc1.diagnostic, qc1.diagmsg);
            EXFAIL_OUT(ret);
        }

        /* Verify that we have fields in place... */
        if (NULL==(p = Bfind(buf2, T_STRING_2_FLD, 0, 0L)))
        {
            NDRX_LOG(log_error, "TESTERROR: failed to get T_STRING_2_FLD %s", 
                    Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }

        sprintf(strbuf, "HELLO FROM SENDER");
        
        if (0!=strcmp(p, strbuf))
        {
            NDRX_LOG(log_error, "TESTERROR: Invalid value [%s]", p);
            EXFAIL_OUT(ret);
        }

        /* Verify that we have fields in place... */
        if (NULL==(p = Bfind(buf2, T_STRING_FLD, 0, 0L)))
        {
            NDRX_LOG(log_error, "TESTERROR: failed to get T_STRING_FLD %s", 
                    Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }

        if (0!=strcmp(p, "OK"))
        {
            NDRX_LOG(log_error, "TESTERROR: Invalid value [%s]", p);
            EXFAIL_OUT(ret);
        }
        tpfree((char *)buf2);
    }
    
    /* Check that OKQ1 is empty.... even after restart of tmqueue */
    if (EXSUCCEED!=system("xadmin restart tmqueue"))
    {
        NDRX_LOG(log_error, "Failed to restart tmqueue: %s", strerror(errno));
        EXFAIL_OUT(ret);
    }
    
    /* no messages available, even after restart... */
    memset(&qc1, 0, sizeof(qc1));
    len=0;
    if (EXSUCCEED==tpdequeue("MYSPACE", "OKQ1", &qc1, (char **)&buf3, 
            &len, TPNOTRAN))
    {
        NDRX_LOG(log_error, "TESTERROR: tpdequeue() must fail but was OK!");
        EXFAIL_OUT(ret);
    }

    if (EXSUCCEED!=tpterm())
    {
        NDRX_LOG(log_error, "tpterm failed with: %s", tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }
    
out:
    return ret;
}


/**
 * Sending to OK q.
 * So answers of the forward and put in the reply queue.
 * we wait for it to be filled up.
 * ---
 * Do not use reply q.
 */
exprivate int basic_autoqnr_ok(void)
{
    int ret = EXSUCCEED;
    TPQCTL qc1;
    long len = 0;
    char *p;
    int i;
    char strbuf[128];
    char *buf3=NULL;
    
    for (i=0; i<100; i++)
    {
        UBFH *buf = (UBFH *)tpalloc("UBF", "", 1024);
        if (NULL==buf)
        {
            NDRX_LOG(log_error, "TESTERROR: tpalloc() failed %s", 
                    tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }

        sprintf(strbuf, "HELLO FROM SENDER");
        
        if (EXSUCCEED!=Bchg(buf, T_STRING_2_FLD, 0, strbuf, 0L))
        {
            NDRX_LOG(log_error, "TESTERROR: failed to set T_STRING_2_FLD %s", 
                    Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }

        /* enqueue the data buffer */
        memset(&qc1, 0, sizeof(qc1));

        if (EXSUCCEED!=tpenqueue("MYSPACE", "OKQ1", &qc1, (char *)buf, 0, TPNOTRAN))
        {
            NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                    tpstrerror(tperrno), qc1.diagnostic, qc1.diagmsg);
            EXFAIL_OUT(ret);
        }
        tpfree((char *)buf);
    }
    
    sleep(30); /* should be enough */
    
    /* Check that OKQ1 is empty.... even after restart of tmqueue */
    if (EXSUCCEED!=system("xadmin restart tmqueue"))
    {
        NDRX_LOG(log_error, "Failed to restart tmqueue: %s", strerror(errno));
        EXFAIL_OUT(ret);
    }
    
    /* no messages available, even after restart... */
    memset(&qc1, 0, sizeof(qc1));
    len=0;
    if (EXSUCCEED==tpdequeue("MYSPACE", "OKQ1", &qc1, (char **)&buf3, 
            &len, TPNOTRAN))
    {
        NDRX_LOG(log_error, "TESTERROR: tpdequeue() must fail but was OK!");
        EXFAIL_OUT(ret);
    }

    if (EXSUCCEED!=tpterm())
    {
        NDRX_LOG(log_error, "tpterm failed with: %s", tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }
    
out:
    return ret;
}

/**
 * Test deadq
 */
exprivate int basic_autoq_deadq(void)
{
    int ret = EXSUCCEED;
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
            EXFAIL_OUT(ret);
        }

        sprintf(strbuf, "HELLO FROM SENDER");
        
        if (EXSUCCEED!=Bchg(buf, T_STRING_2_FLD, 0, strbuf, 0L))
        {
            NDRX_LOG(log_error, "TESTERROR: failed to set T_STRING_2_FLD %s", 
                    Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }

        /* enqueue the data buffer */
        memset(&qc1, 0, sizeof(qc1));

        qc1.flags|=TPQFAILUREQ;

        strcpy(qc1.failurequeue, "DEADQ");

        if (EXSUCCEED!=tpenqueue("MYSPACE", "BADQ1", &qc1, (char *)buf, 0, TPNOTRAN))
        {
            NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                    tpstrerror(tperrno), qc1.diagnostic, qc1.diagmsg);
            EXFAIL_OUT(ret);
        }
        tpfree((char *)buf);
    }
    sleep(60); /* should be enough */
    
    for (i=0; i<100; i++)
    {
        UBFH *buf2 = (UBFH *)tpalloc("UBF", "", 1024);
        memset(&qc1, 0, sizeof(qc1));

        NDRX_LOG(log_warn, "LOOP: %d", i);
        
        if (EXSUCCEED!=tpdequeue("MYSPACE", "DEADQ", &qc1, (char **)&buf2, 
                &len, TPNOTRAN))
        {
            NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                    tpstrerror(tperrno), qc1.diagnostic, qc1.diagmsg);
            EXFAIL_OUT(ret);
        }

        /* Verify that we have fields in place... */
        if (NULL==(p = Bfind(buf2, T_STRING_2_FLD, 0, 0L)))
        {
            NDRX_LOG(log_error, "TESTERROR: failed to get T_STRING_2_FLD %s", 
                    Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }

        sprintf(strbuf, "HELLO FROM SENDER");
        
        if (0!=strcmp(p, strbuf))
        {
            NDRX_LOG(log_error, "TESTERROR: Invalid value [%s]", p);
            EXFAIL_OUT(ret);
        }

        tpfree((char *)buf2);
    }

    if (EXSUCCEED!=tpterm())
    {
        NDRX_LOG(log_error, "tpterm failed with: %s", tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }
    
out:
    return ret;
}

/**
 * Sending to OK q
 */
exprivate int basic_rndfail(void)
{
    int ret = EXSUCCEED;
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
            EXFAIL_OUT(ret);
        }

        sprintf(strbuf, "HELLO FROM SENDER");
        
        if (EXSUCCEED!=Bchg(buf, T_STRING_2_FLD, 0, strbuf, 0L))
        {
            NDRX_LOG(log_error, "TESTERROR: failed to set T_STRING_2_FLD %s", 
                    Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }

        /* enqueue the data buffer */
        memset(&qc1, 0, sizeof(qc1));

        qc1.flags|=TPQREPLYQ;

        strcpy(qc1.replyqueue, "REPLYQ");

        if (EXSUCCEED!=tpenqueue("MYSPACE", "RFQ", &qc1, (char *)buf, 0, TPNOTRAN))
        {
            NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                    tpstrerror(tperrno), qc1.diagnostic, qc1.diagmsg);
            EXFAIL_OUT(ret);
        }
        tpfree((char *)buf);
    }
    sleep(30); /* should be enough */
    
    for (i=0; i<100; i++)
    {
        UBFH *buf2 = (UBFH *)tpalloc("UBF", "", 1024);
        memset(&qc1, 0, sizeof(qc1));

        NDRX_LOG(log_warn, "LOOP: %d", i);
        
        if (EXSUCCEED!=tpdequeue("MYSPACE", "REPLYQ", &qc1, (char **)&buf2, 
                &len, TPNOTRAN))
        {
            NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                    tpstrerror(tperrno), qc1.diagnostic, qc1.diagmsg);
            EXFAIL_OUT(ret);
        }

        /* Verify that we have fields in place... */
        if (NULL==(p = Bfind(buf2, T_STRING_2_FLD, 0, 0L)))
        {
            NDRX_LOG(log_error, "TESTERROR: failed to get T_STRING_2_FLD %s", 
                    Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }

        sprintf(strbuf, "HELLO FROM SENDER");
        
        if (0!=strcmp(p, strbuf))
        {
            NDRX_LOG(log_error, "TESTERROR: Invalid value [%s]", p);
            EXFAIL_OUT(ret);
        }

        /* Verify that we have fields in place... */
        if (NULL==(p = Bfind(buf2, T_STRING_FLD, 0, 0L)))
        {
            NDRX_LOG(log_error, "TESTERROR: failed to get T_STRING_FLD %s", 
                    Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }

        if (0!=strcmp(p, "OK"))
        {
            NDRX_LOG(log_error, "TESTERROR: Invalid value [%s]", p);
            EXFAIL_OUT(ret);
        }
        tpfree((char *)buf2);
    }

    if (EXSUCCEED!=tpterm())
    {
        NDRX_LOG(log_error, "tpterm failed with: %s", tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }
    
out:
    return ret;
}


/**
 * Add some binary data to Q
 */
    exprivate int basic_enqcarray(void)
{
    int ret = EXSUCCEED;
    TPQCTL qc1;
    long len = 0;
    char *buf = tpalloc("CARRAY", "", 8);
    
    if (NULL==buf)
    {
        NDRX_LOG(log_error, "TESTERROR: tpalloc() failed %s", 
                tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    buf[0] = 0;
    buf[1] = 1;
    buf[2] = 2;
    buf[3] = 3;
    buf[4] = 4;
    buf[5] = 5;
    buf[6] = 6;
    buf[7] = 7;
    
    len = 8;
    
    /* enqueue the data buffer */
    memset(&qc1, 0, sizeof(qc1));
    
    qc1.flags|=TPQREPLYQ;
    strcpy(qc1.replyqueue, "TESTREPLY");
    
    qc1.flags|=TPQFAILUREQ;
    strcpy(qc1.failurequeue, "TESTFAIL");

    if (EXSUCCEED!=tpenqueue("MYSPACE", "BINQ", &qc1, buf, len, TPNOTRAN))
    {
        NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                tpstrerror(tperrno), qc1.diagnostic, qc1.diagmsg);
        EXFAIL_OUT(ret);
    }
    
    tpfree((char *)buf);

    if (EXSUCCEED!=tpterm())
    {
        NDRX_LOG(log_error, "tpterm failed with: %s", tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }
    
out:
    return ret;
}

/**
 * Test FIFO message get from future queue
 * - Load normal messages
 * - Load messages with future reltive time 
 * - Fetch messages normal messages 
 * - Fetch messages with future relative time
 */
exprivate int basic_q_fut_fifo_test(void)
{
    int ret = EXSUCCEED;
    TPQCTL qc1;
    long len=0;
    int test; /* 0 - relative time, 1 - absolute time*/
    long i, l;
    long max_msgs = 200;

    for (test = 0; test < 2; test++)
    {
        for (i=0; i<max_msgs; i++)
        {
            UBFH *buf = (UBFH *)tpalloc("UBF", "", 1024);
            if (NULL==buf)
            {
                NDRX_LOG(log_error, "TESTERROR: tpalloc() failed %s", 
                        tpstrerror(tperrno));
                EXFAIL_OUT(ret);
            }

            if (EXSUCCEED != Bchg(buf, T_LONG_FLD, 0, (char *)&i, 0L))
            {
                NDRX_LOG(log_error, "TESTERROR: failed to set T_LONG_FLD %s", 
                        Bstrerror(Berror));
                EXFAIL_OUT(ret);
            }

            /* enqueue the data buffer */
            memset(&qc1, 0, sizeof(qc1));

            /* even to future queue */
            if (0==i%2)
            {
                if (0==test)
                {
                    qc1.flags|=TPQTIME_REL;
                    qc1.deq_time = 10+(i%9);
                }
                else
                {
                    qc1.flags|=TPQTIME_ABS;
                    qc1.deq_time = time(NULL)+10;
                }
            }

            if (EXSUCCEED!=tpenqueue("MYSPACE", "FUT_FIFO", &qc1, (char *)buf, 0, 0))
            {
                NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                        tpstrerror(tperrno), qc1.diagnostic, qc1.diagmsg);
                EXFAIL_OUT(ret);
            }
            tpfree((char *)buf);
        }

        NDRX_LOG(log_debug, "dequeue normal msg!");
        /* dequeue normal msg */
        for (i=0; i<max_msgs; i++)
        {
            /* even is in future queue */
            if (0==i%2)
            {
                continue;
            }

            UBFH *buf2 = (UBFH *)tpalloc("UBF", "", 1024);
            memset(&qc1, 0, sizeof(qc1));

            if (EXSUCCEED!=tpdequeue("MYSPACE", "FUT_FIFO", &qc1, (char **)&buf2, &len, 0))
            {
                NDRX_LOG(log_error, "TESTERROR: tpdequeue() failed %s diag: %d:%s", 
                        tpstrerror(tperrno), qc1.diagnostic, qc1.diagmsg);
                EXFAIL_OUT(ret);
            }

            ndrx_debug_dump_UBF(log_debug, "normal msg buf", buf2);

            /* Verify that we have fields in place... */
            if (EXSUCCEED!=Bget(buf2, T_LONG_FLD, 0, (char *)&l, 0L))
            {
                NDRX_LOG(log_error, "TESTERROR: failed to get T_LONG_FLD %s", 
                        Bstrerror(Berror));
                EXFAIL_OUT(ret);
            }
            if (l != i )
            {
                NDRX_LOG(log_error, "TESTERROR: Invalid value [%d] exp [%d]", l, i);
                EXFAIL_OUT(ret);
            }

            tpfree((char *)buf2);
        }

        UBFH *buf = (UBFH *)tpalloc("UBF", "", 1024);
        if (NULL==buf)
        {
            NDRX_LOG(log_error, "TESTERROR: tpalloc() failed %s", 
                    tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }

        memset(&qc1, 0, sizeof(qc1));
        if (EXSUCCEED==tpdequeue("MYSPACE", "FUT_LIFO", &qc1, (char **)&buf, &len, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: tpdequeue() must fail but succeed!");
            EXFAIL_OUT(ret);
        }
    
        NDRX_ASSERT_VAL_OUT(TPEDIAGNOSTIC==tperrno && QMENOMSG==qc1.diagnostic, 
            "Expected QMENOMSG got %d %ld", tperrno, qc1.diagnostic);


        sleep(30); /* should be enough */
        NDRX_LOG(log_debug, "dequeue future msg!");

        /* dequeue future msg */
        for (i=0; i<max_msgs; i++)
        {
            /* odd is in normal queue */
            if (0!=i%2)
            {
                continue;
            }

            UBFH *buf3 = (UBFH *)tpalloc("UBF", "", 1024);
            memset(&qc1, 0, sizeof(qc1));

            if (EXSUCCEED!=tpdequeue("MYSPACE", "FUT_FIFO", &qc1, (char **)&buf3, &len, 0))
            {
                NDRX_LOG(log_error, "TESTERROR: tpdequeue() failed %s diag: %d:%s", 
                        tpstrerror(tperrno), qc1.diagnostic, qc1.diagmsg);
                EXFAIL_OUT(ret);
            }

            ndrx_debug_dump_UBF(log_debug, "fut Q msg buf", buf3);

            /* Verify that we have fields in place... */
            if (EXSUCCEED!=Bget(buf3, T_LONG_FLD, 0, (char *)&l, 0L))
            {
                NDRX_LOG(log_error, "TESTERROR: failed to get T_LONG_FLD %s", 
                        Bstrerror(Berror));
                EXFAIL_OUT(ret);
            }
            if (l != i )
            {
                NDRX_LOG(log_error, "TESTERROR: Invalid value [%d] exp [%d]", l, i);
                EXFAIL_OUT(ret);
            }

            tpfree((char *)buf3);
        }
    }


out:
    return ret;
}

/**
 * Test LIFO message get from future queue
 * - Load normal messages
 * - Load messages with future reltive time 
 * - Fetch messages normal messages 
 * - Fetch messages with future relative time
 */
exprivate int basic_q_fut_lifo_test(void)
{
    int ret = EXSUCCEED;
    TPQCTL qc1;
    long len=0;
    int test; /* 0 - relative time, 1 - absolute time*/
    long i, l;
    long max_msgs = 200;

    for (test = 0; test < 2; test++)
    {
        for (i=0; i<max_msgs; i++)
        {
            UBFH *buf = (UBFH *)tpalloc("UBF", "", 1024);
            if (NULL==buf)
            {
                NDRX_LOG(log_error, "TESTERROR: tpalloc() failed %s", 
                        tpstrerror(tperrno));
                EXFAIL_OUT(ret);
            }

            if (EXSUCCEED != Bchg(buf, T_LONG_FLD, 0, (char *)&i, 0L))
            {
                NDRX_LOG(log_error, "TESTERROR: failed to set T_LONG_FLD %s", 
                        Bstrerror(Berror));
                EXFAIL_OUT(ret);
            }

            /* enqueue the data buffer */
            memset(&qc1, 0, sizeof(qc1));

            /* even to future queue */
            if (0==i%2)
            {
                if (0==test)
                {
                    qc1.flags|=TPQTIME_REL;
                    qc1.deq_time = 10+(i%9);
                }
                else
                {
                    qc1.flags|=TPQTIME_ABS;
                    qc1.deq_time = time(NULL)+10;
                }
            }

            if (EXSUCCEED!=tpenqueue("MYSPACE", "FUT_LIFO", &qc1, (char *)buf, 0, 0))
            {
                NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                        tpstrerror(tperrno), qc1.diagnostic, qc1.diagmsg);
                EXFAIL_OUT(ret);
            }
            tpfree((char *)buf);
        }

        NDRX_LOG(log_debug, "dequeue normal msg from LIFO!");
        /* dequeue normal msg */
        for (i=max_msgs-1; i>=0; i--)
        {
            /* even is in future queue */
            if (0==i%2)
            {
                continue;
            }

            UBFH *buf2 = (UBFH *)tpalloc("UBF", "", 1024);
            memset(&qc1, 0, sizeof(qc1));

            if (EXSUCCEED!=tpdequeue("MYSPACE", "FUT_LIFO", &qc1, (char **)&buf2, &len, 0))
            {
                NDRX_LOG(log_error, "TESTERROR: tpdequeue() failed %s diag: %d:%s", 
                        tpstrerror(tperrno), qc1.diagnostic, qc1.diagmsg);
                EXFAIL_OUT(ret);
            }

            ndrx_debug_dump_UBF(log_debug, "normal msg buf", buf2);

            /* Verify that we have fields in place... */
            if (EXSUCCEED!=Bget(buf2, T_LONG_FLD, 0, (char *)&l, 0L))
            {
                NDRX_LOG(log_error, "TESTERROR: failed to get T_LONG_FLD %s", 
                        Bstrerror(Berror));
                EXFAIL_OUT(ret);
            }
            if (l != i )
            {
                NDRX_LOG(log_error, "TESTERROR: Invalid value [%d] exp [%d]", l, i);
                EXFAIL_OUT(ret);
            }

            tpfree((char *)buf2);
        }

        UBFH *buf = (UBFH *)tpalloc("UBF", "", 1024);
        if (NULL==buf)
        {
            NDRX_LOG(log_error, "TESTERROR: tpalloc() failed %s", 
                    tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }

        memset(&qc1, 0, sizeof(qc1));
        if (EXSUCCEED==tpdequeue("MYSPACE", "FUT_LIFO", &qc1, (char **)&buf, &len, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: tpdequeue() must fail but succeed!");
            EXFAIL_OUT(ret);
        }
    
        NDRX_ASSERT_VAL_OUT(TPEDIAGNOSTIC==tperrno && QMENOMSG==qc1.diagnostic, 
            "Expected QMENOMSG got %d %ld", tperrno, qc1.diagnostic);

        sleep(30); /* should be enough */
        NDRX_LOG(log_debug, "dequeue future msg from LIFO !");

        /* dequeue future msg */
        for (i=max_msgs-1; i>=0; i--)
        {
            /* odd is in normal queue */
            if (0!=i%2)
            {
                continue;
            }

            UBFH *buf3 = (UBFH *)tpalloc("UBF", "", 1024);
            memset(&qc1, 0, sizeof(qc1));

            if (EXSUCCEED!=tpdequeue("MYSPACE", "FUT_LIFO", &qc1, (char **)&buf3, &len, 0))
            {
                NDRX_LOG(log_error, "TESTERROR: tpdequeue() failed %s diag: %d:%s", 
                        tpstrerror(tperrno), qc1.diagnostic, qc1.diagmsg);
                EXFAIL_OUT(ret);
            }

            ndrx_debug_dump_UBF(log_debug, "fut Q msg buf", buf3);

            /* Verify that we have fields in place... */
            if (EXSUCCEED!=Bget(buf3, T_LONG_FLD, 0, (char *)&l, 0L))
            {
                NDRX_LOG(log_error, "TESTERROR: failed to get T_LONG_FLD %s", 
                        Bstrerror(Berror));
                EXFAIL_OUT(ret);
            }
            if (l != i )
            {
                NDRX_LOG(log_error, "TESTERROR: Invalid value [%d] exp [%d]", l, i);
                EXFAIL_OUT(ret);
            }

            tpfree((char *)buf3);
        }
    }


out:
    return ret;
}

/******************************************************************************/
/* FIFO/LIFO FUTURE AUTOQ tests                                               */
/******************************************************************************/
#define TEST_FUT_FIFO   0
#define TEST_FUT_LIFO   1

/** is this lifo or fifo test? */
volatile int M_fut_test_mode=0;

/** 0 marks the start... */
volatile int M_fut_test_counter=0;

/** number of messages processed */
volatile int M_fut_nr_proc=0;

#define TEST_FUT_STAGE_CUR      0 /**< expect current messages */
#define TEST_FUT_STAGE_FUT      1 /**< epxect future messages  */
#define TEST_FUT_MAX        200

volatile int M_fut_test_stage;

/**
 * Process callbacks
 */
void notification_callback (char *data, long len, long flags)
{
    UBFH *buf = (UBFH *)data;
    long l;
    char qname[64];

    /* read the counter */

    if (EXSUCCEED!=Bget(buf, T_LONG_FLD, 0, (char *)&l, 0L))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to get T_LONG_FLD %s", 
                Bstrerror(Berror));
    }

    if (EXSUCCEED!=Bget(buf, T_STRING_FLD, 0, qname, 0L))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to get T_STRING_FLD %s", 
                Bstrerror(Berror));
    }

    /* determine it's sequence */
    if (TEST_FUT_FIFO==M_fut_test_mode)
    {
        /* what's expected: */
        M_fut_test_counter+=2;
        if (TEST_FUT_STAGE_CUR==M_fut_test_stage)
        {
            /* odd from current Q */
            if (l%2!=1)
            {
                NDRX_LOG(log_error, "TESTERROR: FIFO/CUR Expected odd counter "
                    "rcved [%d] (M_fut_nr_proc=%d qname=%s)", l, M_fut_nr_proc, qname);
            }

            /* read form current Q */
            if (l!=M_fut_test_counter)
            {
                NDRX_LOG(log_error, "TESTERROR: FIFO/CUR Expected counter [%d] got [%d] "
                    "(M_fut_nr_proc=%d qname=%s)", 
                        M_fut_test_counter, l, M_fut_nr_proc, qname);
            }

            /* check the proper increment */
            M_fut_nr_proc++;

            NDRX_LOG(log_error, "FIFO/CUR M_fut_nr_proc=%d l=%ld qname=%s",
                M_fut_nr_proc, l, qname);
        }
        else
        {
            /* even form future */
            if (l%2!=0)
            {
                NDRX_LOG(log_error, "TESTERROR: FIFO/FUT Expected rcvd "
                    "cntr [%d] to be even, but got odd "
                    "(M_fut_nr_proc=%d qname=%s)", l, M_fut_nr_proc, qname);
            }

            /* read form current Q */
            if (l!=M_fut_test_counter)
            {
                NDRX_LOG(log_error, "TESTERROR: FIFO/FUT Expected counter [%d] got [%d] "
                    "(M_fut_nr_proc=%d qname=%s)", 
                        M_fut_test_counter+2, l, M_fut_nr_proc, qname);
            }

            /* check the proper increment */
            M_fut_nr_proc++;

            NDRX_LOG(log_error, "FIFO/FUT M_fut_nr_proc=%d l=%ld qname=%s",
                M_fut_nr_proc, l, qname);
        }
    }
    else if (TEST_FUT_LIFO==M_fut_test_mode)
    {
        /* what's expected: */
        M_fut_test_counter-=2;
        if (TEST_FUT_STAGE_CUR==M_fut_test_stage)
        {
            /* it be odd from current Q */
            if (l%2!=1)
            {
                NDRX_LOG(log_error, "TESTERROR: LIFO/CUR Expected odd counter "
                    "rcved [%d] (M_fut_nr_proc=%d qname=%s)", l, M_fut_nr_proc, qname);
            }

            /* check the current counter */
            if (l!=M_fut_test_counter)
            {
                NDRX_LOG(log_error, "TESTERROR: LIFO/CUR Expected counter [%d] got [%d] "
                    "(M_fut_nr_proc=%d qname=%s)", 
                        M_fut_test_counter, l, M_fut_nr_proc, qname);
            }

            /* check the proper increment */
            M_fut_nr_proc++;

            NDRX_LOG(log_error, "LIFO/CUR M_fut_nr_proc=%d l=%ld qname=%s", 
                M_fut_nr_proc, l, qname);
        }
        else
        {
            /* it be odd from current Q */
            if (l%2!=0)
            {
                NDRX_LOG(log_error, "TESTERROR: LIFO/FUT Expected rcvd "
                    "cntr [%d] to be even, but got odd "
                    "(M_fut_nr_proc=%d qname=5s)", l, M_fut_nr_proc, qname);
            }

            /*  check the message nr */
            if (l!=M_fut_test_counter)
            {
                NDRX_LOG(log_error, "TESTERROR: LIFO/FUT Expected counter [%d] got [%d] "
                    "(M_fut_nr_proc=%d qname=%s)", 
                        M_fut_test_counter+2, l, M_fut_nr_proc, qname);
            }

            /* check the proper increment */
            M_fut_nr_proc++;
            NDRX_LOG(log_error, "LIFO/CUR M_fut_nr_proc=%d l=%ld qname=%s", 
                M_fut_nr_proc, l, qname);
        }
    }
}

/**
 * Test auto fifo-lifo q processing
 */
exprivate int basic_q_fut_fifo_lifo_auto_test(void)
{
    int ret = EXSUCCEED;
    TPQCTL qc1;
    long len=0;
    long i, l, q;
    long max_msgs = TEST_FUT_MAX;
    char *qnames[] = {"FUT_FIFO_AUTO", "FUT_LIFO_AUTO"};
    UBFH *buf = (UBFH *)tpalloc("UBF", "", 1024);
    ndrx_stopwatch_t w;
    time_t tt;

    tpsetunsol(notification_callback);

    for (q=0;q<2; q++)
    {
        /* capture the deq time here, as 
         * when the test execute, the deq time grows too
         */
        tt=time(NULL);
        /* mark the test mode for the callback... */
        M_fut_test_mode=q;
        M_fut_nr_proc=0;    /* start from scratch... */
        M_fut_test_stage=TEST_FUT_STAGE_CUR;

        /* for fifo & lifo first part is the same
         * as dequeue happens in the same order as enqueue
         */
        if (TEST_FUT_FIFO==q)
        {
            M_fut_test_counter=-1;
        }
        else
        {
            M_fut_test_counter=(TEST_FUT_MAX+1);
        }

        for (i=0; i<max_msgs; i++)
        {
            /* for LIFO only future messages,
             * as current onse are dequeued in real-time
             * and thus we got almost the same fifo order
             */
            if (TEST_FUT_LIFO==q && 1==i%2)
            {
                continue;
            }

            if (EXSUCCEED != Bchg(buf, T_LONG_FLD, 0, (char *)&i, 0L))
            {
                NDRX_LOG(log_error, "TESTERROR: failed to set T_LONG_FLD %s", 
                        Bstrerror(Berror));
                EXFAIL_OUT(ret);
            }

            if (EXSUCCEED != Bchg(buf, T_STRING_FLD, 0, qnames[q], 0L))
            {
                NDRX_LOG(log_error, "TESTERROR: failed to set T_STRING_FLD %s", 
                        Bstrerror(Berror));
                EXFAIL_OUT(ret);
            }

            /* enqueue the data buffer */
            memset(&qc1, 0, sizeof(qc1));

            /* even to future queue */
            if (0==i%2)
            {
               qc1.flags|=TPQTIME_ABS;

               if (TEST_FUT_FIFO==q)
               {
                    if (i>=TEST_FUT_MAX/2)
                    {
                        /* second halve goes first */
                        qc1.deq_time = tt+20;
                    }
                    else
                    {
                        /* first halve as second... */
                        qc1.deq_time = tt+35;
                    }
               }
               else if (TEST_FUT_LIFO==q)
               {
                    /* first havle goes first for lifo */

                    if (i>=TEST_FUT_MAX/2)
                    {
                        /* second halve goes first */
                        qc1.deq_time = tt+35;
                    }
                    else
                    {
                        /* first halve as second... */
                        qc1.deq_time = tt+20;
                    }
               }
            }
            /* else: odd messages are normal ones... without time */

            if (EXSUCCEED!=tpenqueue("MYSPACE", qnames[q], &qc1, (char *)buf, 0, 0))
            {
                NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                        tpstrerror(tperrno), qc1.diagnostic, qc1.diagmsg);
                EXFAIL_OUT(ret);
            }
        }

        /* dequeue cur, only for fifo */
        if (TEST_FUT_FIFO==q)
        {
            ndrx_stopwatch_reset(&w);
            do
            {
                /* download all msgs... */
                while (tpchkunsol()>0){}
                sleep(1);
            } while (ndrx_stopwatch_get_delta_sec(&w) <=10);
            
            if (M_fut_nr_proc!=TEST_FUT_MAX/2)
            {
                NDRX_LOG(log_error, "TESTERROR: Expected %d messages, got %d, qname=[%s] (NORMAL)", 
                    TEST_FUT_MAX/2, M_fut_nr_proc, qnames[q]);
                EXFAIL_OUT(ret);
            }

            NDRX_LOG(log_error, "Expected %d messages, got %d, qname=[%s] (NORMAL) - OK", 
                    TEST_FUT_MAX/2, M_fut_nr_proc, qnames[q]);
        }
        else
        {
            sleep(10);
        }

        /***********************************************************************/
        /* first part... is >=100 */
        /***********************************************************************/

        /* prepare for future tests... */
        M_fut_nr_proc=0;
        M_fut_test_stage=TEST_FUT_STAGE_FUT;

        if (TEST_FUT_FIFO==q)
        {
            /* for even messages */
            /* M_fut_test_counter=-2; */
            M_fut_test_counter=98;
        }
        else
        {
            /* M_fut_test_counter=(TEST_FUT_MAX); */
            M_fut_test_counter=(TEST_FUT_MAX)/2;
        }
        
        /* first part of download shall be >100, and only then <100
         * as deq time for second part if shorter
         */
        ndrx_stopwatch_reset(&w);
        do
        {
            /* download all msgs... */
            while (tpchkunsol()>0){}
            sleep(1);
        } while (ndrx_stopwatch_get_delta_sec(&w) <=15);
        
        if (M_fut_nr_proc!=TEST_FUT_MAX/2/2)
        {
            NDRX_LOG(log_error, "TESTERROR: Expected %d messages, got %d qname=[%s] (FUT, 1st part)", 
                TEST_FUT_MAX/2, M_fut_nr_proc, qnames[q]);
            EXFAIL_OUT(ret);
        }
        NDRX_LOG(log_error, "Expected %d messages, got %d, qname=[%s] (FUT, 1st part) - OK", 
                TEST_FUT_MAX/2/2, M_fut_nr_proc, qnames[q]);

        /***********************************************************************/
        /* second part... */
        /***********************************************************************/
        M_fut_nr_proc=0;
        M_fut_test_stage=TEST_FUT_STAGE_FUT;

        if (TEST_FUT_FIFO==q)
        {
            /* for even messages */
            /* M_fut_test_counter=-2; */
            M_fut_test_counter=-2;
        }
        else
        {
            M_fut_test_counter=(TEST_FUT_MAX);
        }
        
        /* 
         * second part is Ids bellow 200
         */
        ndrx_stopwatch_reset(&w);
        do
        {
            /* download all msgs... */
            while (tpchkunsol()>0){}
            sleep(1);
        } while (ndrx_stopwatch_get_delta_sec(&w) <=25);
        
        if (M_fut_nr_proc!=TEST_FUT_MAX/2/2)
        {
            NDRX_LOG(log_error, "TESTERROR: Expected %d messages, got %d qname=[%s] (FUT, 2nd part)", 
                TEST_FUT_MAX/2, M_fut_nr_proc, qnames[q]);
            EXFAIL_OUT(ret);
        }
        NDRX_LOG(log_error, "Expected %d messages, got %d, qname=[%s] (FUT, 2nd part) - OK", 
                TEST_FUT_MAX/2/2, M_fut_nr_proc, qnames[q]);

    }
out:
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */

