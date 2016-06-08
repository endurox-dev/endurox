/* 
** Basic test client
**
** @file atmiclt1.c
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

#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>
#include <test.fd.h>
#include <ndrstandard.h>
#include <ntimer.h>
#include <fcntl.h>
#include <unistd.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define _BENCHONLY

#define SYNC_FILE "sync.log"
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Sync test start...
 */
int do_sync (void)
{
    while (FAIL == access( SYNC_FILE, F_OK ) )
    {
        /* sleep for 0.01 sec */
        usleep(10000);
    }
    
    return SUCCEED;
}

/*
 * Do the test call to the server
 */
int main(int argc, char** argv) {

    UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, 9216);
    long rsplen;
    int i;
    int ret=SUCCEED;
    double d;
    double cps;
    double dv = 55.66;
    char bench_mode[16]=":0:1:8:";
    char buf[1024];
    char test_buf_carray[8192];
    char test_buf_small[1024];
    ndrx_timer_t timer;
    int call_num = MAX_ASYNC_CALLS *4;
    Badd(p_ub, T_STRING_FLD, "THIS IS TEST FIELD 1", 0);
    Badd(p_ub, T_STRING_FLD, "THIS IS TEST FIELD 2", 0);
    Badd(p_ub, T_STRING_FLD, "THIS IS TEST FIELD 3", 0);

    /* Do synchronization if needed...  */
    if (argc>1 && (argv[1][0]=='s' || argv[1][0]=='S'))
    {
        printf("syncing...\n");
        do_sync();
    }
    
    
    printf("syncing ok...\n");
    fflush(stdout);
    
    if (argc>2)
    {
        strcpy(bench_mode, argv[2]);
    }
    
    
    printf("test mode [%s]\n", bench_mode);
    fflush(stdout);
    
    if (strstr(bench_mode, ":0:"))
    {
        for (i=0; i<50; i++)
        {
            d=i;
            if (FAIL==Bchg(p_ub, T_DOUBLE_FLD, i, (char *)&d, 0))
            {
                NDRX_LOG(log_debug, "Failed to get T_DOUBLE_FLD[%d]", i);
                ret=FAIL;
                goto out;
            }

            if (FAIL == tpcall("TESTSV", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0))
            {
                NDRX_LOG(log_error, "TESTSV failed: %s", tpstrerror(tperrno));
                ret=FAIL;
                goto out;
            }

            /* Verify the data */
            if (FAIL==Bget(p_ub, T_DOUBLE_FLD, i, (char *)&d, 0))
            {
                NDRX_LOG(log_debug, "Failed to get T_DOUBLE_FLD[%d]", i);
                ret=FAIL;
                goto out;
            }

            if (fabs(i - d) > 0.00001)
            {
                NDRX_LOG(log_debug, "%lf!=%lf =>  FAIL", dv, d);
                ret=FAIL;
                goto out;
            }

            /* print the output */
            Bfprint(p_ub, stderr);
        }

        NDRX_LOG(log_debug, "Do second call to timeout-server");
        if (FAIL == tpcall("TIMEOUTSV", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0))
        {
            NDRX_LOG(log_error, "TIMEOUTSV failed: %s", tpstrerror(tperrno));
            if (TPETIME!=tperrno)
            {
                NDRX_LOG(log_debug, "No timeout err from TIMEOUTSV!!!");
                ret=FAIL;
                goto out;
            }
        }
        else
        {
            NDRX_LOG(log_debug, "No timeout from TIMEOUTSV!!!");
            ret=FAIL;
            goto out;
        }

        /* Do Another test, but now we will wait! */
        NDRX_LOG(log_debug, "Do third call to timeout-server, TPNOTIME");
        if (FAIL == tpcall("TIMEOUTSV", (char *)p_ub, 0L, (char **)&p_ub, 
                &rsplen, TPNOTIME))
        {
            NDRX_LOG(log_error, "TESTSV failed: %s", tpstrerror(tperrno));
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
                NDRX_LOG(log_error, "TESTERROR: RETSOMEDATA failed: %s", 
                        tpstrerror(tperrno));
                ret=FAIL;
                goto out;
            }

            if (FAIL==Bget(p_ub, T_STRING_2_FLD, 0, buf, NULL))
            {
                NDRX_LOG(log_error, "TESTERROR: Failed to get T_STRING_2_FLD: %s", 
                        Bstrerror(Berror));
                ret=FAIL;
                goto out;
            }
        }

        if (0!=strcmp(buf, "RESPONSE DATA 1"))
        {
            NDRX_LOG(log_error, "TESTERROR: Invalid response data in "
                    "T_STRING_2_FLD, got [%s]", buf);
            ret=FAIL;
            goto out;
        }
    }
    /**************************************************************************/
    /* 1KB benchmark*/
    if (strstr(bench_mode, ":1:"))
    {
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

        printf("1KB Performance: %d bytes in %ld (sec) = %lf bytes/sec = %lf bytes/MB sec, "
                "calls/sec = %lf\n", 
                (int)(sizeof(test_buf_small)*(call_num)), 
                (long)ndrx_timer_get_delta_sec(&timer),  
                d,
                (d/1024)/1024, 
                cps);
        fflush(stdout);
    }
    /**************************************************************************/
    /* 8KB benchmark*/
    
    if (strstr(bench_mode, ":8:"))
    {
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
        fflush(stdout);
    }
out:
    tpterm();

    if (argc>1 && argv[1][0]=='S')
    {
        printf("Test finished waiting for kill..");
        fflush(stdout);
        sleep(9999999);
    }

    return ret;
}

