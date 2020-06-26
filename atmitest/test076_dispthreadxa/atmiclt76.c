/**
 * @brief Dispatch thread xa on cluster, null switch, buildtools - client
 *
 * @file atmiclt76.c
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
#include <exassert.h>
#include "test76.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
int M_term = EXFALSE;
volatile int M_has_failure = EXFALSE;
/*---------------------------Prototypes---------------------------------*/

/* this function is run by the second thread */
void *run_case(void *x_void_ptr)
{
    int ret = EXSUCCEED;
    long rsplen;
    char *svcnm = (char *)x_void_ptr;
    int tx_mode = EXFALSE;
    int fail_mode = EXFALSE;
    char res1[64];
    char res2[64];
    if (0==strncmp(svcnm, "TX", 2))
    {
        tx_mode=EXTRUE;
    }
    
    if (0==strcmp(svcnm, "TXFAIL"))
    {
        fail_mode=EXTRUE;
    }
    
    if (tx_mode)
    {
        NDRX_ASSERT_TP_OUT((EXSUCCEED==tpopen()), "Failed to tpopen()");
    }
    
    while (!M_term)
    {
        UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, 1024);
        
        /* start new tran */
        if (tx_mode)
        {
            NDRX_ASSERT_TP_OUT((EXSUCCEED==tpbegin(60, 0)), "Tran shall start OK");
        }
        
        if (EXFAIL == tpcall(svcnm, (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0))
        {
            NDRX_LOG(log_error, "%s failed: %s", svcnm, tpstrerror(tperrno));
            ret=EXFAIL;
            goto out;
        }
        
        /* check the service chain */
        NDRX_ASSERT_UBF_OUT((EXSUCCEED==Bget(p_ub, T_STRING_FLD, 0, res1, NULL)), 
                "Res 1 missing");
        NDRX_ASSERT_UBF_OUT((EXSUCCEED==Bget(p_ub, T_STRING_FLD, 1, res2, NULL)), 
                "Res 2 missing");
        
        /* check the results */
        
        if (!tx_mode)
        {
            NDRX_ASSERT_VAL_OUT((0==strcmp(res1, "NOTX")), "Got [%s]", res1);
            NDRX_ASSERT_VAL_OUT((0==strcmp(res2, "NOTX")), "Got [%s]", res2);
        }
        else
        {
            NDRX_ASSERT_VAL_OUT((0==strcmp(res1, "INTX")), "Got [%s]", res1);
            NDRX_ASSERT_VAL_OUT((0==strcmp(res2, "INTX")), "Got [%s]", res2);
        }
        
        /* try to commit... */
        if (tx_mode)
        {
            if (fail_mode)
            {
                /* commit shall fail */
                NDRX_ASSERT_TP_OUT((EXFAIL==tpcommit(0)), "Commit shall fail");
                NDRX_ASSERT_TP_OUT((TPEABORT==tperrno), "Expected abort");
            }
            else
            {
                /* commit shall OK */
                NDRX_ASSERT_TP_OUT((EXSUCCEED==tpcommit(0)), "Commit shall be OK");
            }
        }
        
        tpfree((char *)p_ub);
        
    }
    
out:
    
    if (tx_mode)
    {
        if (EXSUCCEED!=tpclose())
        {
            NDRX_LOG(log_error, "TESTERROR: tpclose() failed: %s", 
                    tpstrerror(tperrno));
        }
    }

    tpterm();
    
    fprintf(stderr, "Exit with %d\n", ret);
    
    if (EXSUCCEED!=ret)
    {
        M_has_failure=EXTRUE;
    }
    
    return NULL;
}

/**
 * Run 3x threads.
 * Each will call the appropriate chain
 */
int main(int argc, char** argv)
{
#define THREADS     3
    int ret=EXSUCCEED;
    pthread_t thrd[THREADS];
    int i;
    char *svcnms[] = {"TXOK", "TXFAIL", "NOTX"};
    ndrx_stopwatch_t w;
    /* open 3x threads, run for 10 min... */
    
    for (i=0; i<THREADS; i++)
    {
        if(EXSUCCEED!=pthread_create(&thrd[i], NULL, run_case, svcnms[i]))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to create %s thread", svcnms[i]);
            EXFAIL_OUT(ret);
        }
    }
    
    ndrx_stopwatch_reset(&w);
    
    /* run for period... */
    while(!M_has_failure && ndrx_stopwatch_get_delta_sec(&w) < 180)
    {
        sleep(1);
    }
    
    /* mark for shutdown */
    M_term = EXTRUE;
    
    /* wait for join */
    
    for (i=0; i<THREADS; i++)
    {
        void *retptr;
        pthread_join(thrd[i], &retptr);
    }
    
    /* check result */
    
    if (M_has_failure)
    {
        NDRX_LOG(log_error, "TESTERROR! Failure is set");
        ret=EXFAIL;
    }
    
out:
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
