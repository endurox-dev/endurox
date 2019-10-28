/**
 * @brief Basic test client
 *
 * @file atmiclt15.c
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

#include <unistd.h>     /* Symbolic Constants */
#include <sys/types.h>  /* Primitive System Data Types */ 
#include <errno.h>      /* Errors */
#include <stdio.h>      /* Input/Output */
#include <stdlib.h>     /* General Utilities */
#include <pthread.h>    /* POSIX Threads */

#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>
#include <test.fd.h>
#include <ndrstandard.h>
#include <nstopwatch.h>
#include <nstdutil.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/* #define TEST_STEP       8 */

#define TEST_MIN    1
#define TEST_MAX    56
#define TEST_STEP   4
    
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

double M_test_array[5][TEST_MAX];

/*
 * Thread function for doing ATMI tests...
 */
void do_thread_work ( void *ptr )
{

    UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, 9216);
    long rsplen;
    int i, j;
    int ret=EXSUCCEED;
    double d;
    double cps;
    double dv = 55.66;
    char buf[1024];
    char test_buf_carray[68192];
    char test_buf_small[1024];
    ndrx_stopwatch_t timer;
    int call_num = MAX_ASYNC_CALLS *8;
    Badd(p_ub, T_STRING_FLD, "THIS IS TEST FIELD 1", 0);
    Badd(p_ub, T_STRING_FLD, "THIS IS TEST FIELD 2", 0);
    Badd(p_ub, T_STRING_FLD, "THIS IS TEST FIELD 3", 0);

    
    /***************************************************************************
     * Documentation benchmark 
     **************************************************************************/
    if (NULL!=ptr)
    {
	    int first = EXTRUE;

        for (j=TEST_MIN; j<TEST_MAX; j+=TEST_STEP)
        {
            int callsz = j*1024;
            p_ub = (UBFH *)tprealloc ((char *)p_ub, callsz+500);

            if (EXSUCCEED!=Bchg(p_ub, T_CARRAY_FLD, 0, test_buf_carray, callsz))
            {
                NDRX_LOG(log_error, "TESTERROR: Failed to set T_CARRAY_FLD to %d", callsz);
                ret=EXFAIL;
                goto out;
            }
            
warmed_up:
            ndrx_stopwatch_reset(&timer);
            
            /* Do the loop call! */
            for (i=0; i<call_num; i++) /* Test the cd loop */
            {
                /*
                * Test the case when some data should be returned
                */
                if (EXFAIL==tpcall("ECHO", (char *)p_ub, 0L, (char **)&p_ub, &rsplen, TPNOTIME))
                {
                    NDRX_LOG(log_error, "TESTERROR: ECHO failed: %s", tpstrerror(tperrno));
                    ret=EXFAIL;
                    goto out;
                }
            }

            if (first)
            {
                first = EXFALSE;
                goto warmed_up;
            }

            d = (double)(sizeof(test_buf_carray)*(call_num))/(double)((double)ndrx_stopwatch_get_delta(&timer)/1000.0f);

            cps = (double)(call_num)/((double)ndrx_stopwatch_get_delta(&timer)/1000.0f);

            printf("%dKB Performance: %d bytes in %ld (sec) = %lf bytes/sec = %lf bytes/MB sec, calls/sec = %lf\n", 
                    callsz,
                    (int)(sizeof(test_buf_carray)*(call_num)), 
                    (long)ndrx_stopwatch_get_delta_sec(&timer),  
                    d,
                    (d/1024)/1024, 
                    cps);
            
            fflush(stdout);
            /*
            if (SUCCEED!=ndrx_bench_write_stats((double)j, cps))
            {
                NDRX_LOG(log_always, "Failed to write stats!");
                FAIL_OUT(ret);
            }
            */
            M_test_array[(long)ptr][j] = cps;
        }
        
        return; /* terminate here! */
    }
    /***************************************************************************
     * Documentation benchmark, end
     **************************************************************************/
    
    for (i=0; i<50; i++)
    {

        /* dv+=1; */
        
        if (EXFAIL == tpcall("TESTSV", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0))
        {
            NDRX_LOG(log_error, "TESTERROR: TESTSV failed: %s", tpstrerror(tperrno));
            ret=EXFAIL;
            goto out;
        }

        /* Verify the data */
        if (EXFAIL==Bget(p_ub, T_DOUBLE_FLD, i, (char *)&d, 0))
        {
            NDRX_LOG(log_debug, "TESTERROR: Failed to get T_DOUBLE_FLD[%d]", i);
            ret=EXFAIL;
            goto out;
        }

        if (fabs(dv - d) > 0.00001)
        {
            NDRX_LOG(log_debug, "%lf!=%lf =>  FAIL", dv, d);
            ret=EXFAIL;
            goto out;
        }
        
        /* print the output 
        Bfprint(p_ub, stderr);
        */
    }

    NDRX_LOG(log_debug, "Do second call to timeout-server");
    if (EXFAIL == tpcall("TIMEOUTSV", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0))
    {
        NDRX_LOG(log_error, "TIMEOUTSV failed: %s", tpstrerror(tperrno));
        if (TPETIME!=tperrno)
        {
            NDRX_LOG(log_debug, "TESTERROR: No timeout err from TIMEOUTSV!!!");
            ret=EXFAIL;
            goto out;
        }
    }
    else
    {
        NDRX_LOG(log_debug, "TESTERROR: No timeout from TIMEOUTSV!!!");
        ret=EXFAIL;
        goto out;
    }

    /* Do Another test, but now we will wait! */
    NDRX_LOG(log_debug, "Do third call to timeout-server, TPNOTIME");
    if (EXFAIL == tpcall("TIMEOUTSV", (char *)p_ub, 0L, (char **)&p_ub, &rsplen, TPNOTIME))
    {
        NDRX_LOG(log_error, "TESTERROR: TESTSV failed: %s", tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }

    /*
     * We should be able to call serices with out passing buffer to call.
     */
    if (EXFAIL==tpcall("NULLSV", NULL, 0L, (char **)&p_ub, &rsplen, TPNOTIME))
    {
        NDRX_LOG(log_error, "TESTERROR: NULLSV failed: %s", tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }

    /* Support #386 p_ub now have become NULL, thus allocate it again  */
        
    p_ub = (UBFH *)tpalloc("UBF", NULL, 156000);

    if (NULL==p_ub)
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to alloc ubf: %s", 
                tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }

    for (i=0; i<MAX_ASYNC_CALLS+2000; i++) /* Test the cd loop */
    {
        /*
        * Test the case when some data should be returned
        */
        if (EXFAIL==tpcall("RETSOMEDATA", NULL, 0L, (char **)&p_ub, &rsplen, TPNOTIME))
        {
            NDRX_LOG(log_error, "TESTERROR: RETSOMEDATA failed: %s", tpstrerror(tperrno));
            ret=EXFAIL;
            goto out;
        }

        if (EXFAIL==Bget(p_ub, T_STRING_2_FLD, 0, buf, NULL))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to get T_STRING_2_FLD: %s", Bstrerror(Berror));
            ret=EXFAIL;
            goto out;
        }
    }
    
    if (0!=strcmp(buf, "RESPONSE DATA 1"))
    {
        NDRX_LOG(log_error, "TESTERROR: Invalid response data in T_STRING_2_FLD, got [%s]", buf);
        ret=EXFAIL;
        goto out;
    }
    /**************************************************************************/
    /* 1KB benchmark*/
    p_ub = (UBFH *)tprealloc ((char *)p_ub, 1128);

    if (EXSUCCEED!=Bchg(p_ub, T_CARRAY_FLD, 0, test_buf_small, sizeof(test_buf_small)))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to set T_CARRAY_FLD");
        ret=EXFAIL;
        goto out;
    }
        
    ndrx_stopwatch_reset(&timer);
    
    /* Do the loop call! */
    for (i=0; i<call_num; i++) /* Test the cd loop */
    {
        /*
        * Test the case when some data should be returned
        */
        if (EXFAIL==tpcall("ECHO", (char *)p_ub, 0L, (char **)&p_ub, &rsplen, TPNOTIME))
        {
            NDRX_LOG(log_error, "TESTERROR: ECHO failed: %s", tpstrerror(tperrno));
            ret=EXFAIL;
            goto out;
        }
    }
    
    d = (double)(sizeof(test_buf_small)*(call_num))/(double)ndrx_stopwatch_get_delta_sec(&timer);
    
    cps = (double)(call_num)/(double)ndrx_stopwatch_get_delta_sec(&timer);
    
    printf("1KB Performance: %d bytes in %ld (sec) = %lf bytes/sec = %lf bytes/MB sec, calls/sec = %lf\n", 
            (int)(sizeof(test_buf_small)*(call_num)), 
            (long)ndrx_stopwatch_get_delta_sec(&timer),  
            d,
            (d/1024)/1024, 
            cps);

    /**************************************************************************/
    /* 8KB benchmark*/
    
    p_ub = (UBFH *)tprealloc ((char *)p_ub, 9216);

    if (EXSUCCEED!=Bchg(p_ub, T_CARRAY_FLD, 0, test_buf_carray, sizeof(test_buf_carray)))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to set T_CARRAY_FLD");
        ret=EXFAIL;
        goto out;
    }
    
    ndrx_stopwatch_reset(&timer);
    
    /* Do the loop call! */
    for (i=0; i<call_num; i++) /* Test the cd loop */
    {
        /*
        * Test the case when some data should be returned
        */
        if (EXFAIL==tpcall("ECHO", (char *)p_ub, 0L, (char **)&p_ub, &rsplen, TPNOTIME))
        {
            NDRX_LOG(log_error, "TESTERROR: ECHO failed: %s", tpstrerror(tperrno));
            ret=EXFAIL;
            goto out;
        }
    }
    
    d = (double)(sizeof(test_buf_carray)*(call_num))/(double)ndrx_stopwatch_get_delta_sec(&timer);
    
    cps = (double)(call_num)/(double)ndrx_stopwatch_get_delta_sec(&timer);
    
    printf("8KB Performance: %d bytes in %ld (sec) = %lf bytes/sec = %lf bytes/MB sec, calls/sec = %lf\n", 
            (int)(sizeof(test_buf_carray)*(call_num)), 
            (long)ndrx_stopwatch_get_delta_sec(&timer),  
            d,
            (d/1024)/1024, 
            cps);
out:
    tpterm();
 /*   return ret; */
}


/*
 * Do the test call to the server
 */
int main(int argc, char** argv) 
{
    int j;
    pthread_t thread1, thread2, thread3, thread4, thread5;  /* thread variables */
    void *arg1 = NULL;
    void *arg2 = NULL;
    void *arg3 = NULL;
    void *arg4 = NULL;
    void *arg5 = NULL;
    pthread_attr_t pthread_custom_attr;
    pthread_attr_init(&pthread_custom_attr);
    
    memset(M_test_array, 0, sizeof(M_test_array));
    
    if (argc>1)
    {
        arg1 = (void *)0;
        arg2 = (void *)1;
        arg3 = (void *)2;
        arg4 = (void *)3;
        arg5 = (void *)4;
    }
    
    ndrx_platf_stack_set(&pthread_custom_attr);

    /* create threads 1 and 2 */    
    pthread_create (&thread1, &pthread_custom_attr, (void *) &do_thread_work, arg1);
    pthread_create (&thread2, &pthread_custom_attr, (void *) &do_thread_work, arg2);
    /*sleep(1);  Have some async works... WHY? */
    pthread_create (&thread3, &pthread_custom_attr, (void *) &do_thread_work, arg3);
    pthread_create (&thread4, &pthread_custom_attr, (void *) &do_thread_work, arg4);
    pthread_create (&thread5, &pthread_custom_attr, (void *) &do_thread_work, arg5);

    /* Main block now waits for both threads to terminate, before it exits
       If main block exits, both threads exit, even if the threads have not
       finished their work */ 
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    pthread_join(thread3, NULL);
    pthread_join(thread4, NULL);
    pthread_join(thread5, NULL);
    
    /* Benchmark stuff... */
    if (argc>1)
    {
        double sum;
        int i;
        /* plot the results... */
        for (j=TEST_MIN; j<TEST_MAX; j+=TEST_STEP)
        {
            sum=0;
            for (i=0; i<5; i++)
            {
                sum+=M_test_array[i][j];
            }
            if (EXSUCCEED!=ndrx_bench_write_stats((double)j, sum))
            {
                NDRX_LOG(log_always, "Failed to write stats!");
                exit(EXFAIL);
            }
        }
    }

    exit(0);
}

/* vim: set ts=4 sw=4 et smartindent: */
