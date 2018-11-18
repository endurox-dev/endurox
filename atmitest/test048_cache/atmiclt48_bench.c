/**
 * @brief Basic test client
 *
 * @file atmiclt48_bench.c
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

#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>
#include <test.fd.h>
#include <ndrstandard.h>
#include <nstopwatch.h>
#include <fcntl.h>
#include <unistd.h>

#include <nstdutil.h>
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

    UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, 156000);
    long rsplen;
    int i, j;
    int ret=EXSUCCEED;
    double d;
    double cps;
    char bench_mode[16]=":0:1:8:";
    char buf[1024];
    char test_buf_carray[56*1024];
    char test_buf_small[1024];
    ndrx_stopwatch_t timer;
    int call_num = MAX_ASYNC_CALLS *10;
    short callnum_s;
    int bench_call_num;
    short warm_up=0;
    int first = EXTRUE;
    /* Load cache definitions... */
    tpinit(NULL);
    
    /*start with 1 byte, then with 1 kb, then +4 kb up till 56... */
    for (j=0; j<56; j=(j==0?j=1:j+4))
    {
        int callsz = j*1024;
        if (0==j)
        {
            callsz = 1; /* send 1 byte.. */
        }

warmed_up:
        if (EXSUCCEED!=Bchg(p_ub, T_SHORT_FLD, 0, (char *)&warm_up, 0L))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to set T_SHORT_FLD to %d: %s",
                    callsz, Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }

        if (EXSUCCEED!=Bchg(p_ub, T_CARRAY_FLD, 0, test_buf_carray, callsz))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to set T_CARRAY_FLD to %d: %s",
                    callsz, Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }

        if (j<10)
        {
            bench_call_num = call_num*2;
        }
        else
        {
            bench_call_num = call_num;
        }

        ndrx_stopwatch_reset(&timer);

        /* Do the loop call! */
        for (i=0; i<bench_call_num; i++) /* Test the cd loop */
        {
            /*
            * Test the case when some data should be returned
            */
            if (EXFAIL==tpcall("BENCH48", (char *)p_ub, 0L, (char **)&p_ub, 
                    &rsplen, TPNOTIME))
            {
                NDRX_LOG(log_error, "TESTERROR: BENCH48 failed: %s", tpstrerror(tperrno));
                ret=EXFAIL;
                goto out;
            }
        }

        d = (double)(sizeof(test_buf_carray)*(call_num))/(double)((double)
                ndrx_stopwatch_get_delta(&timer)/1000.0f);

        cps = (double)(bench_call_num)/((double)ndrx_stopwatch_get_delta(&timer)/1000.0f);

        printf("%dKB Performance: %d bytes in %ld (sec) = %lf bytes/sec = %lf "
                "bytes/MB sec, calls/sec = %lf\n", 
                callsz,
                (int)(sizeof(test_buf_carray)*(call_num)), 
                (long)ndrx_stopwatch_get_delta_sec(&timer),  
                d,
                (d/1024)/1024, 
                cps);

        fflush(stdout);

        warm_up++;

        if (first)
        {
            first = EXFALSE;
            goto warmed_up;
        }


        /* first loop goes for warmup... */

        if (EXSUCCEED!=ndrx_bench_write_stats((double)j, cps))
        {
            NDRX_LOG(log_always, "Failed to write stats!");
            EXFAIL_OUT(ret);
        }

        
    }
    
out:
    tpterm();

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
