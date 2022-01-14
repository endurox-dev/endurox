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
    exuuid_t uuid;
    
    EX_hash_handle hh;         /* makes this structure hashable */    
} uuid_hash_t;

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
exprivate uuid_hash_t *M_hash = NULL;
exprivate MUTEX_LOCKDECL(M_lock);
/*---------------------------Prototypes---------------------------------*/

/**
 * Return true if found
 */
exprivate uuid_hash_t * do_lookup(exuuid_t uuid)
{
    uuid_hash_t *ent=NULL;
    MUTEX_LOCK_V(M_lock);
    EXHASH_FIND(hh, M_hash, uuid, sizeof(exuuid_t), ent);    
    MUTEX_UNLOCK_V(M_lock);
    
    return ent;
}

/**
 * Add entry
 * @param uuid
 */
exprivate void do_add(exuuid_t uuid)
{
    uuid_hash_t *add=NDRX_MALLOC(sizeof(uuid_hash_t));
    if (NULL==add)
    {
        abort();
    }
    memcpy(add->uuid, uuid, sizeof(exuuid_t));
    MUTEX_LOCK_V(M_lock);
    EXHASH_ADD(hh, M_hash, uuid, sizeof(exuuid_t), add);
    MUTEX_UNLOCK_V(M_lock);
}

/**
 * Dynamic work with some blocks
 */
static void * thread_start(void *arg)
{
    int i, j;
    exuuid_t uuid;
            
    /* for 1 M UUIDs... ever 1x sec.. */
    for (i=0; i<2; i++)
    {
        for (j=0; j<1000000; j++)
        {
            ndrx_uuid_generate(129, uuid);
            
            /* lookup */
            if (NULL!=do_lookup(uuid))
            {
                fail_test("Geneate uuid exists!");
                break;
            }
            /* add */
            do_add(uuid);
        }
        sleep(1);
    }
}

/**
 * Check that uuid is unique accorss the threads
 */
Ensure(test_nstd_uuid_unq)
{
    pthread_t th1;
    pthread_t th2;
    pthread_t th3;
    pthread_t th4;
 
    int ret;
    
    ndrx_uuid_init();
    
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
 * Analyze the UUID format, shall match the desired one.
 */
Ensure(test_nstd_uuid_fmt)
{
    exuuid_t uuid, uuid2;
    unsigned seq1, seq2;
    pid_t pid;
    struct timeval tv, tv2, tvt;

    ndrx_uuid_init();
    
    memset(uuid, 0, sizeof(exuuid_t));
    memset(uuid2, 0, sizeof(exuuid_t));
    
    ndrx_uuid_generate(129, uuid);
    sleep(2);
    ndrx_uuid_generate(4, uuid2);
    
    /* node id: */
    assert_equal((unsigned char)129, uuid[0]);
    assert_equal((unsigned char)4, uuid2[0]);
    
    pid=getpid();
    
    assert_equal( (pid>>24 & 0xff), uuid[1]);
    assert_equal( (pid>>16 & 0xff), uuid[2]);
    assert_equal( (pid>>8 & 0xff), uuid[3]);
    assert_equal( (pid & 0xff), uuid[4]);
        
    seq1 |= uuid[5];
    seq1<<=8;
    seq1 |= uuid[6];
    seq1<<=8;
    seq1 |= uuid[7];
    seq1<<=8;
    seq1 |= uuid[8];
    
    seq2 |= uuid2[5];
    seq2<<=8;
    seq2 |= uuid2[6];
    seq2<<=8;
    seq2 |= uuid2[7];
    seq2<<=8;
    seq2 |= uuid2[8];
    
    /* sequence counters: */
    assert_not_equal(seq1, seq2);
    
    if ((seq1+1)!=seq2 )
    {
        fail_test("Sequence not incremented!");
    }
    
    gettimeofday(&tvt, 0);
    
    memset(&tv, 0, sizeof(tv));
    memset(&tv2, 0, sizeof(tv2));
    
    tv.tv_sec |= uuid[9];
    tv.tv_sec<<=8;
    tv.tv_sec |= uuid[10];
    tv.tv_sec<<=8;
    tv.tv_sec |= uuid[11];
    tv.tv_sec<<=8;
    tv.tv_sec |= uuid[12];
    tv.tv_sec<<=8;
    tv.tv_sec |= uuid[13];
    
    tv2.tv_sec |= uuid2[9];
    tv2.tv_sec<<=8;
    tv2.tv_sec |= uuid2[10];
    tv2.tv_sec<<=8;
    tv2.tv_sec |= uuid2[11];
    tv2.tv_sec<<=8;
    tv2.tv_sec |= uuid2[12];
    tv2.tv_sec<<=8;
    tv2.tv_sec |= uuid2[13];
    
    assert_not_equal(tv.tv_sec, tv2.tv_sec);
    
    if (tv2.tv_sec - tv.tv_sec > 10)
    {
        fail_test("Time difference too large");
    }
    
    if (abs(tv.tv_sec - tvt.tv_sec) > 10)
    {
        UBF_LOG(log_error, "Time difference too large (from clock): %ld vs %ld",
                (long)tv.tv_sec, (long)tvt.tv_sec);
        fail_test("Time difference too large (from clock)");
    }
    
    /* time: 
    assert_not_equal(memcmp(uuid+9, uuid2+9, 5), 0);
    */
    /* random... might be random, might not... 
    assert_not_equal(memcmp(uuid+14, uuid2+14, 2), 0);
     * */
}

/**
 * Standard library tests
 * @return
 */
TestSuite *ubf_nstd_uuid(void)
{
    TestSuite *suite = create_test_suite();

    add_test(suite, test_nstd_uuid_unq);
    add_test(suite, test_nstd_uuid_fmt);
    
    return suite;
}
/* vim: set ts=4 sw=4 et smartindent: */
