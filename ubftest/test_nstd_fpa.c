/**
 * @brief Feedback Pool Allocator tests
 *
 * @file test_nstd_fpa.c
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

#include <stdio.h>
#include <stdlib.h>
#include <cgreen/cgreen.h>
#include <ubf.h>
#include <ndrstandard.h>
#include <string.h>
#include <errno.h>
#include <ndebug.h>
#include <exbase64.h>
#include <nstdutil.h>
#include "test.fd.h"
#include "ubfunit1.h"
#include "xatmi.h"
#include "nstopwatch.h"
#include "fpalloc.h"

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Check dynamic blocks
 */
Ensure(test_nstd_fpa_dyn)
{
    char *ptr[0];
    char *ptr2[0];
    ndrx_fpablock_t *hdr[0];
    
    ptr[0] = ndrx_fpmalloc(NDRX_FPA_0_SIZE, 0);
    hdr[0] = (ndrx_fpablock_t *)(ptr[0] - sizeof(ndrx_fpablock_t));
    assert_not_equal(ptr[0], NULL);
    /* 0 block */
    assert_equal(hdr[0]->poolno, 0);
    ndrx_fpfree(ptr[0]);
    
    /* alloc again, block shall be re-used */
    ptr2[0] = ndrx_fpmalloc(NDRX_FPA_0_SIZE, 0);
    assert_equal(ptr2[0], ptr[0]);
    
    /* check the boundries -> if any problem addr sanitizer will check */
    memset(ptr2[0], 0, NDRX_FPA_0_SIZE);
    
    
    /* add to stack.. -> free up ? */
    ndrx_fpfree(ptr2[0]);
    
    /* check pool 1 size */
    ptr[0] = ndrx_fpmalloc(NDRX_FPA_0_SIZE+1, 0);
    hdr[0] = (ndrx_fpablock_t *)(ptr[0] - sizeof(ndrx_fpablock_t));
    assert_not_equal(ptr[0], NULL);
    assert_equal(hdr[0]->poolno, 1); /* next pool */
    ndrx_fpfree(ptr[0]);
    
    /* check pool 4 */
    ptr[0] = ndrx_fpmalloc(NDRX_FPA_4_SIZE, 0);
    hdr[0] = (ndrx_fpablock_t *)(ptr[0] - sizeof(ndrx_fpablock_t));
    assert_not_equal(ptr[0], NULL);
    assert_equal(hdr[0]->poolno, 4);
    ndrx_fpfree(ptr[0]);
    
    /* alloc sysbuf... */
    ptr[0] = ndrx_fpmalloc(NDRX_MSGSIZEMAX, NDRX_FPSYSBUF);
    hdr[0] = (ndrx_fpablock_t *)(ptr[0] - sizeof(ndrx_fpablock_t));
    assert_not_equal(ptr[0], NULL);
    assert_equal(hdr[0]->poolno, NDRX_FPA_SYSBUF_POOLNO);
    ndrx_fpfree(ptr[0]);
    
    /* get arbsize... */
    ptr[0] = ndrx_fpmalloc(NDRX_FPA_SIZE_MAX+1, 0);
    hdr[0] = (ndrx_fpablock_t *)(ptr[0] - sizeof(ndrx_fpablock_t));
    assert_not_equal(ptr[0], NULL);
    assert_equal(hdr[0]->flags, NDRX_FPABRSIZE);
    assert_equal(hdr[0]->poolno, EXFAIL);
    ndrx_fpfree(ptr[0]);
    
    /* deinit the pool */
    ndrx_fpuninit();
    
}

/**
 * Pool limits tests
 */
Ensure(test_nstd_fpalimits)
{
    char *ptr[NDRX_FPA_BMAX+1];
    char *ptr2[NDRX_FPA_BMAX+1];
    int i;
    ndrx_fpapool_t stats;
    
    /* load the bmin blocks */
    for (i=0; i<NDRX_FPA_BMAX; i++)
    {
        ptr2[i] = ptr[i] = ndrx_fpmalloc(NDRX_FPA_6_SIZE, 0);
        
        /* use the memory. */
        memset(ptr[i], i, NDRX_FPA_6_SIZE);
        
        /* check that we alloc'd something */
        assert_not_equal(ptr[i], NULL);
    }

    /* free up the pool, in reverse order, so that latest goes to the stack.. */
    for (i=NDRX_FPA_BMAX-1; i>=0; i--)
    {
        ndrx_fpfree(ptr[i]);
    }
    
    /* get pool stats, shall be NDRX_FPA_BMAX allocations */
    ndrx_fpstats(6, &stats);
    
    assert_equal(stats.blocks, NDRX_FPA_BMAX);
    
    /* alloc again, pointer shall match */
    
    for (i=0; i<NDRX_FPA_BMAX; i++)
    {
        ptr2[i] = ndrx_fpmalloc(NDRX_FPA_6_SIZE, 0);
        
        /* check that we alloc'd something */
        assert_not_equal(ptr2[i], NULL);
        
        /* blocks are the same as on first alloc... */
        assert_equal(ptr[i][0], i);
        
        /* ptrs match ... */
        assert_equal(ptr2[i], ptr[i]);
    }
    
    /* extra alloc over the pool limits */
    ptr2[NDRX_FPA_BMAX] = ndrx_fpmalloc(NDRX_FPA_6_SIZE, 0);
    assert_not_equal(ptr2[NDRX_FPA_BMAX], NULL);
    
    /* free stuff up... check the blocks ... */
    for (i=0; i<NDRX_FPA_BMAX; i++)
    {
        ndrx_fpstats(6, &stats);
        assert_equal(stats.blocks, i);
        ndrx_fpfree(ptr2[i]);
    }
    
    ndrx_fpstats(6, &stats);
    assert_equal(stats.blocks, NDRX_FPA_BMAX);
    
    /* free up that one extra... should have the same max blocks.. */
    ndrx_fpfree(ptr2[NDRX_FPA_BMAX]);
    ndrx_fpstats(6, &stats);
    assert_equal(stats.blocks, NDRX_FPA_BMAX);
    
    /* deinit the pool */
    ndrx_fpuninit();
}

/**
 * Check the feedback function.
 * - If we swing around bmin limit, the pool shall stay at the given size over the bmin
 * - If we swing around over the bmin limit, the pool size shall reduce
 */
Ensure(test_nstd_feeback_free)
{
    char *ptr[NDRX_FPA_BMAX+1];
    int i;
    ndrx_fpapool_t stats;
    
    /* build up the pool */
    for (i=0; i<NDRX_FPA_BMIN+2; i++)
    {
        ptr[i] = ndrx_fpmalloc(NDRX_FPA_5_SIZE, 0);
    }
    
    /* load blocks in the pool */
    for (i=0; i<NDRX_FPA_BMIN+2; i++)
    {
        ndrx_fpfree(ptr[i]);
    }
    
    /* check the hits and blocks */
    ndrx_fpstats(5, &stats);
    
    assert_equal(stats.blocks, NDRX_FPA_BMIN+2);
    assert_equal(stats.cur_hits, 0);
    
    /* run the hits times */
    for (i=0; i<stats.max_hits; i++)
    {
        ptr[0] = ndrx_fpmalloc(NDRX_FPA_5_SIZE, 0);
        ndrx_fpfree(ptr[0]);
    }
    
    ndrx_fpstats(5, &stats);
    assert_equal(stats.blocks, NDRX_FPA_BMIN+2);
    assert_equal(stats.cur_hits, stats.max_hits);
    
    /* next alloc pair shall remove one element as feedback reached */
    ptr[0] = ndrx_fpmalloc(NDRX_FPA_5_SIZE, 0);
    ndrx_fpfree(ptr[0]);
    
    ndrx_fpstats(5, &stats);
    assert_equal(stats.blocks, NDRX_FPA_BMIN+1);
    assert_equal(stats.cur_hits, 0);
    
    /* run loop again... */
    for (i=0; i<stats.max_hits; i++)
    {
        ptr[0] = ndrx_fpmalloc(NDRX_FPA_5_SIZE, 0);
        ndrx_fpfree(ptr[0]);
    }
    
    ndrx_fpstats(5, &stats);
    assert_equal(stats.blocks, NDRX_FPA_BMIN+1);
    assert_equal(stats.cur_hits, stats.max_hits);
    
    /* go over the hits ... */
    ptr[0] = ndrx_fpmalloc(NDRX_FPA_5_SIZE, 0);
    ndrx_fpfree(ptr[0]);
    
    ndrx_fpstats(5, &stats);
    assert_equal(stats.blocks, NDRX_FPA_BMIN);
    assert_equal(stats.cur_hits, 0);
    
    /* now we work in min area -> stay the same */
    for (i=0; i<stats.max_hits*2; i++)
    {
        ptr[0] = ndrx_fpmalloc(NDRX_FPA_5_SIZE, 0);
        ndrx_fpfree(ptr[0]);
    }
    
    ndrx_fpstats(5, &stats);
    assert_equal(stats.blocks, NDRX_FPA_BMIN);
    assert_equal(stats.cur_hits, 0);
    
    /* deinit the pool */
    ndrx_fpuninit();
    
}

/**
 * We do some dynamic work alloc pages bellow and above the bmin boundry
 * Thus pool size shall stay the same
 */
Ensure(test_nstd_feeback_stay)
{
    char *ptr[NDRX_FPA_BMAX+1];
    int i, j;
    ndrx_fpapool_t stats;
    
    /* build up the pool */
    for (i=0; i<NDRX_FPA_BMIN+5; i++)
    {
        ptr[i] = ndrx_fpmalloc(NDRX_FPA_5_SIZE, 0);
    }
    
    /* load blocks in the pool */
    for (i=0; i<NDRX_FPA_BMIN+5; i++)
    {
        ndrx_fpfree(ptr[i]);
    }
    
    /* check the hits and blocks */
    ndrx_fpstats(5, &stats);
    assert_equal(stats.blocks, NDRX_FPA_BMIN+5);
    assert_equal(stats.cur_hits, 0);
    
    for (j=0; j<stats.max_hits*2; j++)
    {
        /* alloc 7 */
        for (i=0; i<7; i++)
        {
            ptr[i] = ndrx_fpmalloc(NDRX_FPA_5_SIZE, 0);
        }

        /* free 7 */
        for (i=0; i<7; i++)
        {
            ndrx_fpfree(ptr[i]);
        }
    }
    
    /* size shall stay in dynamic range.. */
    ndrx_fpstats(5, &stats);
    assert_equal(stats.blocks, NDRX_FPA_BMIN+5);
    assert_equal(stats.cur_hits, 0);
}

#define CHK_LOOPS   5
/**
 * Dynamic work with some blocks
 */
static void * thread_start(void *arg)
{
    int i, j;
    char *ptr[NDRX_FPA_BMAX+1];
    ndrx_fpapool_t stats;
    
    ndrx_fpstats(NDRX_FPA_SYSBUF_POOLNO, &stats);
    
    for (j=0; j<stats.max_hits*10; j++)
    {
        
        for (i=0; i<CHK_LOOPS; i++)
        {
            ptr[i] = ndrx_fpmalloc(NDRX_MSGSIZEMAX, NDRX_FPSYSBUF);
        }

        for (i=0; i<CHK_LOOPS; i++)
        {
            ndrx_fpfree(ptr[i]);
        }
    }
    
    return NULL;
}
 
/**
 * Init the pool. Run two threads.
 * The pool size shall stay in the boundries of alloc'd size
 * due to negative feedback (went bellow bmin)
 */
Ensure(test_nstd_feeback_stay_threaded)
{
    char *ptr[NDRX_FPA_BMAX+1];
    int i;
    ndrx_fpapool_t stats;
    pthread_t th1;
    pthread_t th2;
    int ret;
    
    /* build up the pool */
    for (i=0; i<NDRX_FPA_BMIN+(CHK_LOOPS-1); i++)
    {
        ptr[i] = ndrx_fpmalloc(NDRX_MSGSIZEMAX, NDRX_FPSYSBUF);
    }
    
    /* load blocks in the pool */
    for (i=0; i<NDRX_FPA_BMIN+(CHK_LOOPS-1); i++)
    {
        ndrx_fpfree(ptr[i]);
    }
    
    ret=pthread_create(&th1, NULL, thread_start, NULL);
    assert_equal(ret, EXSUCCEED);
    
    ret=pthread_create(&th2, NULL, thread_start, NULL);
    assert_equal(ret, EXSUCCEED);
    
    pthread_join(th1, NULL);
    pthread_join(th2, NULL);
    
    ndrx_fpstats(NDRX_FPA_SYSBUF_POOLNO, &stats);
    assert_equal(stats.blocks, NDRX_FPA_BMIN+(CHK_LOOPS-1));
 
    /* deinit the pool */
    ndrx_fpuninit();
    
}

/**
 * Load some config...
 */
Ensure(test_nstd_fpa_config_memall)
{
    char *ptr;
    int i;
    
    /* use malloc only... */
    setenv(CONF_NDRX_FPAOPTS, "D:M", EXTRUE);
    
    for (i=0;i<10;i++)
    {
        ptr=ndrx_fpmalloc(777, 0);
        assert_not_equal(ptr, NULL);
        ndrx_fpfree(ptr);
    }
    
    /* deinit the pool */
    ndrx_fpuninit();
    
}

/**
 * Configure limits and verify...
 */
Ensure(test_nstd_fpa_config_limits)
{
    char *ptr;
    ndrx_fpapool_t stats;
    
    /* use malloc only... */
    setenv(CONF_NDRX_FPAOPTS, "1:5,2:3:5,S:20:40:100,4:M", EXTRUE);
    
    /* pull in the init... */
    ptr=ndrx_fpmalloc(777, 0);
    assert_not_equal(ptr, NULL);
    ndrx_fpfree(ptr);
    
    /* check the settings applied... */
    ndrx_fpstats(0, &stats);
    assert_equal(stats.bmin, 5);
    assert_equal(stats.bmax, NDRX_FPA_BMAX);
    assert_equal(stats.max_hits, NDRX_FPA_BHITS);
    
    /* check the settings applied... */
    ndrx_fpstats(1, &stats);
    assert_equal(stats.bmin, 3);
    assert_equal(stats.bmax, 5);
    assert_equal(stats.max_hits, NDRX_FPA_BHITS);
    
    /* check the settings applied... */
    ndrx_fpstats(NDRX_FPA_SYSBUF_POOLNO, &stats);
    assert_equal(stats.bmin, 20);
    assert_equal(stats.bmax, 40);
    assert_equal(stats.max_hits, 100);
    assert_equal(stats.flags, NDRX_FPSYSBUF);
    
    ndrx_fpstats(2, &stats);
    assert_equal(stats.flags, NDRX_FPNOPOOL);
    
    
    /* deinit the pool */
    ndrx_fpuninit();
    
}

/**
 * Check invalid config
 */
Ensure(test_nstd_fpa_config_inval)
{
    char *ptr;
    ndrx_fpapool_t stats;
    /* use malloc only... */
    setenv(CONF_NDRX_FPAOPTS, "1:5:3:4:6", EXTRUE);
    /* pull in the init... */
    assert_equal(ndrx_fpmalloc(777, 0), NULL);
    assert_equal(errno, EINVAL);
    
    setenv(CONF_NDRX_FPAOPTS, "X", EXTRUE);
    assert_equal(ndrx_fpmalloc(777, 0), NULL);
    assert_equal(errno, EINVAL);
    
    setenv(CONF_NDRX_FPAOPTS, "1:5, 4:5", EXTRUE);
    assert_equal(ndrx_fpmalloc(777, 0), NULL);
    assert_equal(errno, EINVAL);
    
    setenv(CONF_NDRX_FPAOPTS, "D:M:4", EXTRUE);
    assert_equal(ndrx_fpmalloc(777, 0), NULL);
    assert_equal(errno, EINVAL);
    
    setenv(CONF_NDRX_FPAOPTS, "D:1:2:3", EXTRUE);
    ptr = ndrx_fpmalloc(777, 0);
    assert_not_equal(ptr, NULL);
    ndrx_fpfree(ptr);
    
    ndrx_fpstats(2, &stats);
    assert_equal(stats.bmin, 1);
    assert_equal(stats.bmax, 2);
    assert_equal(stats.max_hits, 3);
    
    /* check the settings applied... */
    ndrx_fpstats(NDRX_FPA_SYSBUF_POOLNO, &stats);
    assert_equal(stats.bmin, 1);
    assert_equal(stats.bmax, 2);
    assert_equal(stats.max_hits, 3);
    assert_equal(stats.flags, NDRX_FPSYSBUF);
    
}

/**
 * Standard library tests
 * @return
 */
TestSuite *ubf_nstd_fpa(void)
{
    TestSuite *suite = create_test_suite();

    add_test(suite, test_nstd_fpa_dyn);
    add_test(suite, test_nstd_fpalimits);
    add_test(suite, test_nstd_feeback_free);
    add_test(suite, test_nstd_feeback_stay);
    add_test(suite, test_nstd_feeback_stay_threaded);
    add_test(suite, test_nstd_fpa_config_memall);
    add_test(suite, test_nstd_fpa_config_limits);
    add_test(suite, test_nstd_fpa_config_inval);
    
    return suite;
}
/* vim: set ts=4 sw=4 et smartindent: */
