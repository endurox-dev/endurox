/**
 * @brief Cluster Identifier tests
 *
 * @file test_nstd_cid.c
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

#include <stdio.h>
#include <stdlib.h>
#include <cgreen/cgreen.h>
#include <ndrstandard.h>
#include <exhash.h>
#include <nstdutil.h>
#include <thlock.h>
#include <ndebug.h>
#include <sys/time.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

typedef struct
{
    exuuid_t cid;
    EX_hash_handle hh;         /* makes this structure hashable */    
} cid_hash_t;


/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
exprivate cid_hash_t *M_hash = NULL;
exprivate MUTEX_LOCKDECL(M_lock);
/*---------------------------Prototypes---------------------------------*/

/**
 * Return true if found
 */
exprivate cid_hash_t * do_lookup(exuuid_t cid)
{
    cid_hash_t *ent=NULL;
    
    MUTEX_LOCK_V(M_lock);
    
    EXHASH_FIND(hh, M_hash, cid, sizeof(exuuid_t), ent);

    
    MUTEX_UNLOCK_V(M_lock);
    
    return ent;
}

/**
 * Add entry
 * @param 0
 */
exprivate void do_add(exuuid_t cid)
{
    cid_hash_t *add=NDRX_MALLOC(sizeof(cid_hash_t));
    if (NULL==add)
    {
        abort();
    }
    memcpy(add->cid, cid, sizeof(exuuid_t));
    MUTEX_LOCK_V(M_lock);
    EXHASH_ADD(hh, M_hash, cid, sizeof(exuuid_t), add);
    MUTEX_UNLOCK_V(M_lock);
}

/**
 * Dynamic work with some blocks
 */
static void * thread_start(void *arg)
{
    int i, j;
    exuuid_t cid;
            
    /* for 1 M CIDs... ever 1x sec.. */
    for (i=0; i<2; i++)
    {
        for (j=0; j<100000; j++)
        {
            ndrx_cid_generate(129, cid);
            
            /* lookup */
            if (NULL!=do_lookup(cid))
            {
                fail_test("Geneate 0 exists!");
                break;
            }
            /* add */
            do_add(cid);
        }
        sleep(1);
    }

    return NULL;
}

/**
 * Check that 0 is unique accorss the threads
 */
Ensure(test_nstd_cid_unq)
{
    pthread_t th1;
    pthread_t th2;
    pthread_t th3;
    pthread_t th4;
 
    int ret;
    
    ret=pthread_create(&th1, NULL, thread_start, NULL);
    assert_equal(ret, EXSUCCEED);
    
    ret=pthread_create(&th2, NULL, thread_start, NULL);
    assert_equal(ret, EXSUCCEED);
    
    ret=pthread_create(&th3, NULL, thread_start, NULL);
    assert_equal(ret, EXSUCCEED);
    
    ret=pthread_create(&th4, NULL, thread_start, NULL);
    assert_equal(ret, EXSUCCEED);
    
    pthread_join(th1, NULL);
    pthread_join(th2, NULL);
    pthread_join(th3, NULL);
    pthread_join(th4, NULL);
}

/**
 * Analyze the CID format, shall match the desired one.
 */
Ensure(test_nstd_cid_fmt)
{
    exuuid_t cid, cid2;
    unsigned seq1, seq2;
    pid_t pid;
    struct timeval tv, tv2, tvt;
    int i, got_diff;

    memset(cid, 0, sizeof(exuuid_t));
    memset(cid2, 0, sizeof(exuuid_t));
  
    ndrx_cid_generate(129, cid);
    sleep(2);
    ndrx_cid_generate(4, cid2);
    
    /* node id: */
    assert_equal((unsigned char)129, cid[0]);
    assert_equal((unsigned char)4, cid2[0]);
    
    pid=getpid();
    
    assert_equal( (pid>>24 & 0xff), cid[1]);
    assert_equal( (pid>>16 & 0xff), cid[2]);
    assert_equal( (pid>>8 & 0xff), cid[3]);
    assert_equal( (pid & 0xff), cid[4]);
        
/*
    seq1 |= cid[5];
    seq1<<=8;
*/
    seq1 |= cid[6];
    seq1<<=8;
    seq1 |= cid[7];
    seq1<<=8;
    seq1 |= cid[8];
    
/*
    seq2 |= cid2[5];
    seq2<<=8;
*/
    seq2 |= cid2[6];
    seq2<<=8;
    seq2 |= cid2[7];
    seq2<<=8;
    seq2 |= cid2[8];
    
    /* sequence counters: */
    assert_not_equal(seq1, seq2);
    
    if ((seq1+1)!=seq2 )
    {
        fail_test("Sequence not incremented!");
    }
    
    gettimeofday(&tvt, 0);
    
    memset(&tv, 0, sizeof(tv));
    memset(&tv2, 0, sizeof(tv2));
    
    tv.tv_sec |= (cid[9] & 0x01);
    tv.tv_sec<<=8;
    tv.tv_sec |= cid[10];
    tv.tv_sec<<=8;
    tv.tv_sec |= cid[11];
    tv.tv_sec<<=8;
    tv.tv_sec |= cid[12];
    tv.tv_sec<<=8;
    tv.tv_sec |= cid[13];
    
    /* strip off leading usec: */
    tv2.tv_sec |= (cid2[9] & 0x01);
    tv2.tv_sec<<=8;
    tv2.tv_sec |= cid2[10];
    tv2.tv_sec<<=8;
    tv2.tv_sec |= cid2[11];
    tv2.tv_sec<<=8;
    tv2.tv_sec |= cid2[12];
    tv2.tv_sec<<=8;
    tv2.tv_sec |= cid2[13];
    
    assert_not_equal(tv.tv_sec, tv2.tv_sec);
    
    if (tv2.tv_sec - tv.tv_sec > 10)
    {
        fail_test("Time difference too large");
    }
    
    if (labs(tv.tv_sec - tvt.tv_sec) > 10)
    {
        UBF_LOG(log_error, "Time difference too large (from clock): %ld vs %ld",
                (long)tv.tv_sec, (long)tvt.tv_sec);
        fail_test("Time difference too large (from clock)");
    }
    
    got_diff=EXFALSE;
    for (i=0; i<1000; i++)
    {
        ndrx_cid_generate(129, cid);
        ndrx_cid_generate(4, cid2);
        
        if (0!=memcmp(cid+14, cid2+14, 2))
        {
            got_diff=EXTRUE;
        }
    }
    
    assert_equal(got_diff, EXTRUE);
    
}

/**
 * Standard library tests
 * @return
 */
TestSuite *ubf_nstd_cid(void)
{
    TestSuite *suite = create_test_suite();

    add_test(suite, test_nstd_cid_unq);
    add_test(suite, test_nstd_cid_fmt);
    
    return suite;
}
/* vim: set ts=4 sw=4 et smartindent: */
