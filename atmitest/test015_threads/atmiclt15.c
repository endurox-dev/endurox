/* 
** Basic test client
**
** @file atmiclt15.c
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
#include <ntimer.h>
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
    int ret=SUCCEED;
    double d;
    double cps;
    double dv = 55.66;
    char buf[1024];
    char test_buf_carray[8192];
    char test_buf_small[1024];
    ndrx_timer_t timer;
    int call_num = MAX_ASYNC_CALLS *10;
    Badd(p_ub, T_STRING_FLD, "THIS IS TEST FIELD 1", 0);
    Badd(p_ub, T_STRING_FLD, "THIS IS TEST FIELD 2", 0);
    Badd(p_ub, T_STRING_FLD, "THIS IS TEST FIELD 3", 0);

    
    /***************************************************************************
     * Documentation benchmark 
     **************************************************************************/
    if (NULL!=ptr)
    {
        for (j=TEST_MIN; j<TEST_MAX; j+=TEST_STEP)
        {
            int callsz = j*1024;
            p_ub = (UBFH *)tprealloc ((char *)p_ub, callsz+500);

            if (SUCCEED!=Bchg(p_ub, T_CARRAY_FLD, 0, test_buf_carray, callsz))
            {
                NDRX_LOG(log_error, "TESTERROR: Failed to set T_CARRAY_FLD to %d", callsz);
                ret=FAIL;
                goto out;
            }
            
            ndrx_timer_reset(&timer);
            
            /* Do the loop call! */
            for (i=0; i<call_num; i++) /* Test the cd loop */
            {
                /*
                * Test the case when some data should be returned
                */
                if (FAIL==tpcall("ECHO", (char *)p_ub, 0L, (char **)&p_ub, &rsplen, TPNOTIME))
                {
                    NDRX_LOG(log_error, "TESTERROR: ECHO failed: %s", tpstrerror(tperrno));
                    ret=FAIL;
                    goto out;
                }
            }

            d = (double)(sizeof(test_buf_carray)*(call_num))/(double)ndrx_timer_get_delta_sec(&timer);

            cps = (double)(call_num)/(double)ndrx_timer_get_delta_sec(&timer);

            printf("%dKB Performance: %d bytes in %ld (sec) = %lf bytes/sec = %lf bytes/MB sec, calls/sec = %lf\n", 
                    callsz,
                    (int)(sizeof(test_buf_carray)*(call_num)), 
                    (long)ndrx_timer_get_delta_sec(&timer),  
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
        
        if (FAIL == tpcall("TESTSV", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0))
        {
            NDRX_LOG(log_error, "TESTERROR: TESTSV failed: %s", tpstrerror(tperrno));
            ret=FAIL;
            goto out;
        }

        /* Verify the data */
        if (FAIL==Bget(p_ub, T_DOUBLE_FLD, i, (char *)&d, 0))
        {
            NDRX_LOG(log_debug, "TESTERROR: Failed to get T_DOUBLE_FLD[%d]", i);
            ret=FAIL;
            goto out;
        }

        if (fabs(dv - d) > 0.00001)
        {
            NDRX_LOG(log_debug, "%lf!=%lf =>  FAIL", dv, d);
            ret=FAIL;
            goto out;
        }
        
        /* print the output 
        Bfprint(p_ub, stderr);
        */
    }

    NDRX_LOG(log_debug, "Do second call to timeout-server");
    if (FAIL == tpcall("TIMEOUTSV", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0))
    {
        NDRX_LOG(log_error, "TIMEOUTSV failed: %s", tpstrerror(tperrno));
        if (TPETIME!=tperrno)
        {
            NDRX_LOG(log_debug, "TESTERROR: No timeout err from TIMEOUTSV!!!");
            ret=FAIL;
            goto out;
        }
    }
    else
    {
        NDRX_LOG(log_debug, "TESTERROR: No timeout from TIMEOUTSV!!!");
        ret=FAIL;
        goto out;
    }

    /* Do Another test, but now we will wait! */
    NDRX_LOG(log_debug, "Do third call to timeout-server, TPNOTIME");
    if (FAIL == tpcall("TIMEOUTSV", (char *)p_ub, 0L, (char **)&p_ub, &rsplen, TPNOTIME))
    {
        NDRX_LOG(log_error, "TESTERROR: TESTSV failed: %s", tpstrerror(tperrno));
        ret=FAIL;
        goto out;
    }

    /*
     * We should be able to call serices with out passing buffer to call.
     */
    if (FAIL==tpcall("NULLSV", NULL, 0L, (char **)&p_ub, &rsplen, TPNOTIME))
    {
        NDRX_LOG(log_error, "TESTERROR: NULLSV failed: %s", tpstrerror(tperrno));
        ret=FAIL;
        goto out;
    }


    for (i=0; i<MAX_ASYNC_CALLS+2000; i++) /* Test the cd loop */
    {
        /*
        * Test the case when some data should be returned
        */
        if (FAIL==tpcall("RETSOMEDATA", NULL, 0L, (char **)&p_ub, &rsplen, TPNOTIME))
        {
            NDRX_LOG(log_error, "TESTERROR: RETSOMEDATA failed: %s", tpstrerror(tperrno));
            ret=FAIL;
            goto out;
        }

        if (FAIL==Bget(p_ub, T_STRING_2_FLD, 0, buf, NULL))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to get T_STRING_2_FLD: %s", Bstrerror(Berror));
            ret=FAIL;
            goto out;
        }
    }
    
    if (0!=strcmp(buf, "RESPONSE DATA 1"))
    {
        NDRX_LOG(log_error, "TESTERROR: Invalid response data in T_STRING_2_FLD, got [%s]", buf);
        ret=FAIL;
        goto out;
    }
    /**************************************************************************/
    /* 1KB benchmark*/
    p_ub = (UBFH *)tprealloc ((char *)p_ub, 1128);

    if (SUCCEED!=Bchg(p_ub, T_CARRAY_FLD, 0, test_buf_small, sizeof(test_buf_small)))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to set T_CARRAY_FLD");
        ret=FAIL;
        goto out;
    }
        
    ndrx_timer_reset(&timer);
    
    /* Do the loop call! */
    for (i=0; i<call_num; i++) /* Test the cd loop */
    {
        /*
        * Test the case when some data should be returned
        */
        if (FAIL==tpcall("ECHO", NULL, 0L, (char **)&p_ub, &rsplen, TPNOTIME))
        {
            NDRX_LOG(log_error, "TESTERROR: ECHO failed: %s", tpstrerror(tperrno));
            ret=FAIL;
            goto out;
        }
    }
    
    d = (double)(sizeof(test_buf_small)*(call_num))/(double)ndrx_timer_get_delta_sec(&timer);
    
    cps = (double)(call_num)/(double)ndrx_timer_get_delta_sec(&timer);
    
    printf("1KB Performance: %d bytes in %ld (sec) = %lf bytes/sec = %lf bytes/MB sec, calls/sec = %lf\n", 
            (int)(sizeof(test_buf_small)*(call_num)), 
            (long)ndrx_timer_get_delta_sec(&timer),  
            d,
            (d/1024)/1024, 
            cps);

    /**************************************************************************/
    /* 8KB benchmark*/
    
    p_ub = (UBFH *)tprealloc ((char *)p_ub, 9216);

    if (SUCCEED!=Bchg(p_ub, T_CARRAY_FLD, 0, test_buf_carray, sizeof(test_buf_carray)))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to set T_CARRAY_FLD");
        ret=FAIL;
        goto out;
    }
    
    ndrx_timer_reset(&timer);
    
    /* Do the loop call! */
    for (i=0; i<call_num; i++) /* Test the cd loop */
    {
        /*
        * Test the case when some data should be returned
        */
        if (FAIL==tpcall("ECHO", NULL, 0L, (char **)&p_ub, &rsplen, TPNOTIME))
        {
            NDRX_LOG(log_error, "TESTERROR: ECHO failed: %s", tpstrerror(tperrno));
            ret=FAIL;
            goto out;
        }
    }
    
    d = (double)(sizeof(test_buf_carray)*(call_num))/(double)ndrx_timer_get_delta_sec(&timer);
    
    cps = (double)(call_num)/(double)ndrx_timer_get_delta_sec(&timer);
    
    printf("8KB Performance: %d bytes in %ld (sec) = %lf bytes/sec = %lf bytes/MB sec, calls/sec = %lf\n", 
            (int)(sizeof(test_buf_carray)*(call_num)), 
            (long)ndrx_timer_get_delta_sec(&timer),  
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
    
    memset(M_test_array, 0, sizeof(M_test_array));
    
    if (argc>1)
    {
        arg1 = (void *)0;
        arg2 = (void *)1;
        arg3 = (void *)2;
        arg4 = (void *)3;
        arg5 = (void *)4;
    }
    
    /* create threads 1 and 2 */    
    pthread_create (&thread1, NULL, (void *) &do_thread_work, arg1);
    pthread_create (&thread2, NULL, (void *) &do_thread_work, arg2);
    /*sleep(1);  Have some async works... WHY? */
    pthread_create (&thread3, NULL, (void *) &do_thread_work, arg3);
    pthread_create (&thread4, NULL, (void *) &do_thread_work, arg4);
    pthread_create (&thread5, NULL, (void *) &do_thread_work, arg5);

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
            if (SUCCEED!=ndrx_bench_write_stats((double)j, sum))
            {
                NDRX_LOG(log_always, "Failed to write stats!");
                exit(FAIL);
            }
        }
    }

    exit(0);
}

