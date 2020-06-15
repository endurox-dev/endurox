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
#include <nstd_int.h>
#include <fpalloc.h>

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
    char *ptr[1];
    char *ptr2[1];
    ndrx_fpablock_t *hdr[1];
    
    unsetenv(CONF_NDRX_FPAOPTS);

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
Ensure(test_nstd_fpa_limits)
{
    char *ptr[NDRX_FPA_4_DNUM+1];
    char *ptr2[NDRX_FPA_4_DNUM+1];
    int i;
    ndrx_fpapool_t stats;
    
    unsetenv(CONF_NDRX_FPAOPTS);

    /* load the bmin blocks */
    for (i=0; i<NDRX_FPA_4_DNUM; i++)
    {
        ptr2[i] = ptr[i] = ndrx_fpmalloc(NDRX_FPA_4_SIZE, 0);
        
        /* use the memory. */
        memset(ptr[i], i, NDRX_FPA_4_SIZE);
        
        /* check that we alloc'd something */
        assert_not_equal(ptr[i], NULL);
    }

    /* free up the pool, in reverse order, so that latest goes to the stack.. */
    for (i=NDRX_FPA_4_DNUM-1; i>=0; i--)
    {
        ndrx_fpfree(ptr[i]);
    }
    
    /* get pool stats, shall be NDRX_FPA_BNUM allocations */
    ndrx_fpstats(4, &stats);
    
    assert_equal(stats.cur_blocks, NDRX_FPA_4_DNUM);
    
    /* alloc again, pointer shall match */
    
    for (i=0; i<NDRX_FPA_4_DNUM; i++)
    {
        ptr2[i] = ndrx_fpmalloc(NDRX_FPA_4_SIZE, 0);
        
        /* check that we alloc'd something */
        assert_not_equal(ptr2[i], NULL);
        
        /* blocks are the same as on first alloc... */
        assert_equal(ptr[i][0], i);
        
        /* ptrs match ... */
        assert_equal(ptr2[i], ptr[i]);
    }
    
    /* extra alloc over the pool limits */
    ptr2[NDRX_FPA_4_DNUM] = ndrx_fpmalloc(NDRX_FPA_4_SIZE, 0);
    assert_not_equal(ptr2[NDRX_FPA_4_DNUM], NULL);
    
    /* free stuff up... check the blocks ... */
    for (i=0; i<NDRX_FPA_4_DNUM; i++)
    {
        ndrx_fpstats(4, &stats);
        assert_equal(stats.cur_blocks, i);
        ndrx_fpfree(ptr2[i]);
    }
    
    ndrx_fpstats(4, &stats);
    assert_equal(stats.cur_blocks, NDRX_FPA_4_DNUM);
    
    /* free up that one extra... should have the same max blocks.. */
    ndrx_fpfree(ptr2[NDRX_FPA_4_DNUM]);
    ndrx_fpstats(4, &stats);
    assert_equal(stats.cur_blocks, NDRX_FPA_4_DNUM);
    
    /* deinit the pool */
    ndrx_fpuninit();
}


#define CHK_LOOPS   5
/**
 * Dynamic work with some blocks
 */
static void * thread_start(void *arg)
{
    int i, j;
    char *ptr[NDRX_FPA_SYSBUF_DNUM+1];
    ndrx_fpapool_t stats;
    
    ndrx_fpstats(NDRX_FPA_SYSBUF_POOLNO, &stats);
    
    for (j=0; j<1000; j++)
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
Ensure(test_nstd_fpa_threaded)
{
    char *ptr[NDRX_FPA_SYSBUF_DNUM];
    int i;
    ndrx_fpapool_t stats;
    pthread_t th1;
    pthread_t th2;
    int ret;
    
    unsetenv(CONF_NDRX_FPAOPTS);

    /* build up the pool */
    for (i=0; i<NDRX_FPA_SYSBUF_DNUM; i++)
    {
        ptr[i] = ndrx_fpmalloc(NDRX_MSGSIZEMAX, NDRX_FPSYSBUF);
    }
    
    /* load blocks in the pool */
    for (i=0; i<NDRX_FPA_SYSBUF_DNUM; i++)
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
    assert_equal(stats.cur_blocks, NDRX_FPA_SYSBUF_DNUM);
 
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
    setenv(CONF_NDRX_FPAOPTS, "256:5,2K:3,S:20,4K:M", EXTRUE);
    
    /* pull in the init... */
    ptr=ndrx_fpmalloc(777, 0);
    assert_not_equal(ptr, NULL);
    ndrx_fpfree(ptr);
    
    /* check the settings applied... */
    ndrx_fpstats(0, &stats);
    assert_equal(stats.num_blocks, 5);
    
    /* check the settings applied... */
    ndrx_fpstats(3, &stats);
    assert_equal(stats.num_blocks, 3);
    
    /* check the settings applied... */
    ndrx_fpstats(NDRX_FPA_SYSBUF_POOLNO, &stats);
    assert_equal(stats.num_blocks, 20);
    
    ndrx_fpstats(4, &stats);
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
    
    setenv(CONF_NDRX_FPAOPTS, "4M:4", EXTRUE);
    assert_equal(ndrx_fpmalloc(777, 0), NULL);
    assert_equal(errno, EINVAL);
    
    setenv(CONF_NDRX_FPAOPTS, "4K:99", EXTRUE);
    assert_not_equal((ptr=ndrx_fpmalloc(777, 0)), NULL);
    ndrx_fpstats(4, &stats);
    assert_equal(stats.num_blocks, 99);
    ndrx_fpfree(ptr);
    
    /* check the settings applied... */
    ndrx_fpstats(NDRX_FPA_SYSBUF_POOLNO, &stats);
    assert_equal(stats.num_blocks, NDRX_FPA_SYSBUF_DNUM);
    assert_equal(stats.flags, NDRX_FPSYSBUF);
    
}

/**
 * Do some realloc tests
 */
Ensure(test_nstd_fpa_realloc)
{
    ndrx_fpapool_t stats;
    ndrx_fpablock_t *hdr;
    char *ptr, *ptr2;
    
    unsetenv(CONF_NDRX_FPAOPTS);

    ptr = ndrx_fpmalloc(NDRX_FPA_0_SIZE-100, 0);
    
    NDRX_STRCPY_SAFE_DST(ptr, "HELLO WORLD", (NDRX_FPA_0_SIZE-100));
    
    hdr = (ndrx_fpablock_t *)(ptr - sizeof(ndrx_fpablock_t));
    assert_equal(hdr->poolno, 0);
    
    /* check that it is still first pool, no pointer change */
    ptr2 = ndrx_fprealloc(ptr, NDRX_FPA_0_SIZE);
    assert_equal(ptr, ptr2);
    hdr = (ndrx_fpablock_t *)(ptr - sizeof(ndrx_fpablock_t));
    assert_equal(hdr->poolno, 0);
    assert_string_equal(ptr, "HELLO WORLD");
    
    
    /* check that it is second poll */
    ptr = ndrx_fprealloc(ptr, NDRX_FPA_1_SIZE);
    hdr = (ndrx_fpablock_t *)(ptr - sizeof(ndrx_fpablock_t));
    assert_equal(hdr->poolno, 1);
    assert_string_equal(ptr, "HELLO WORLD");
    
    /* check that it is free style pool */
    ptr = ndrx_fprealloc(ptr, NDRX_FPA_5_SIZE+100);
    hdr = (ndrx_fpablock_t *)(ptr - sizeof(ndrx_fpablock_t));
    assert_equal(hdr->flags, NDRX_FPABRSIZE);
    assert_string_equal(ptr, "HELLO WORLD");
    
    /* check that it is first pool */
    ptr = ndrx_fprealloc(ptr, 1);
    hdr = (ndrx_fpablock_t *)(ptr - sizeof(ndrx_fpablock_t));
    assert_equal(hdr->poolno, 0);
    /* block stays the same... */
    assert_string_equal(ptr, "HELLO WORLD");
    
    ndrx_fpfree(ptr);
    
    ndrx_fpstats(0, &stats);
    
    assert_equal(stats.cur_blocks, 1);
    
    
}

/**
 * Standard library tests
 * @return
 */
TestSuite *ubf_nstd_fpa(void)
{
    TestSuite *suite = create_test_suite();

    add_test(suite, test_nstd_fpa_dyn);
    add_test(suite, test_nstd_fpa_limits);
    add_test(suite, test_nstd_fpa_threaded);
    add_test(suite, test_nstd_fpa_config_memall);
    add_test(suite, test_nstd_fpa_config_limits);
    add_test(suite, test_nstd_fpa_config_inval);
    add_test(suite, test_nstd_fpa_realloc);
    
    return suite;
}
/* vim: set ts=4 sw=4 et smartindent: */
