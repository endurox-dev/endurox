/**
 * @brief Linear hash testing
 *
 * @file test_nstd_lh.c
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
#include <ndebug.h>
#include <exbase64.h>
#include <nstdutil.h>
#include <fcntl.h>
#include "test.fd.h"
#include "ubfunit1.h"
#include "xatmi.h"

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

#define TEST_INDEX(MEM, IDX) ((ndrx_lh_test_t*)(((char*)MEM)+(int)(sizeof(ndrx_lh_test_t)*IDX)))

#define TEST_MEM_MAX    50 /**< how much we plan to store?             */
#define TEST_RUN_MAX    200 /**< how much to loop over                */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/**
 * Shared memory entry for service
 */
typedef struct ndrx_lh_test ndrx_lh_test_t;
struct ndrx_lh_test
{
    int key;                            /**< Index key                  */
    int some_val;                       /**< Store some value           */
    short flags;                        /**< See NDRX_SVQ_MAP_STAT_*    */
};

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Generate hash key for value..
 * @param conf hash config
 * @param key_get key data
 * @param key_len key len (not used)
 * @return slot number within the array
 */
exprivate int lhtest_key_hash(ndrx_lh_config_t *conf, void *key_get, size_t key_len)
{
    int *p_key = (int *)key_get;
    return *p_key % conf->elmmax;
}

/**
 * Get debug
 * @param conf hash config
 * @param key_get key data
 * @param key_len not used
 * @param dbg_out output debug string
 * @param dbg_len debug len
 */
exprivate void lhtest_key_debug(ndrx_lh_config_t *conf, void *key_get, 
        size_t key_len, char *dbg_out, size_t dbg_len)
{
    int *p_key = (int *)key_get;
    snprintf(dbg_out, dbg_len, "%d", *p_key);
}

/**
 * CPM shared mem key debug value
 * @param conf hash config
 * @param idx index number
 * @param dbg_out debug buffer
 * @param dbg_len debug len
 */
exprivate void lhtest_val_debug(ndrx_lh_config_t *conf, int idx, char *dbg_out, 
        size_t dbg_len)
{
    int val = TEST_INDEX(*conf->memptr, idx)->some_val;
    
    snprintf(dbg_out, dbg_len, "%d", val);
}

/**
 * Compare the keys
 * @param conf hash config
 * @param key_get key
 * @param key_len key len (not used)
 * @param idx index at which to compare
 * @return 0 = equals, others not.
 */
exprivate int lhtest_compare(ndrx_lh_config_t *conf, void *key_get, 
        size_t key_len, int idx)
{
    int *key = (int *)key_get;
    
    if (TEST_INDEX((*conf->memptr), idx)->key==*key)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

/**
 * Load some keys with one free, the first one
 * @param mem memory to init
 * @param mark_free position to leave un-inited -> free
 * @param mark_was_used position to mark 
 */
exprivate void init_one_free(ndrx_lh_test_t *mem, int mark_free)
{
    int i;
    
    memset(mem, 0, sizeof(ndrx_lh_test_t)*TEST_MEM_MAX);
    
    for (i=0; i<TEST_MEM_MAX; i++)
    {
        if (i!=mark_free)
        {
            mem[i].some_val=i+5;
            mem[i].key=i;
            mem[i].flags=NDRX_LH_FLAG_ISUSED|NDRX_LH_FLAG_WASUSED;
        }
    }
}

/**
 * Check that every free cell we can use in the hash
 */
Ensure(test_nstd_lh_have_space)
{
    int i;
    ndrx_lh_test_t test_mem[TEST_MEM_MAX];
    void *p = test_mem;
    ndrx_lh_config_t conf;
    
    int pos;
    int have_value;
    int found;
    
    conf.elmmax = TEST_MEM_MAX;
    conf.elmsz = sizeof(ndrx_lh_test_t);
    conf.flags_offset = EXOFFSET(ndrx_lh_test_t, flags);
    conf.memptr = (void **)&p;
    conf.p_key_hash=&lhtest_key_hash;
    conf.p_key_debug=&lhtest_key_debug;
    conf.p_val_debug=&lhtest_val_debug;
    conf.p_compare=&lhtest_compare;

    for (i=TEST_MEM_MAX; i<TEST_RUN_MAX; i++)
    {
        init_one_free(test_mem, i % 25);
        
        /* lookup, there should be one position for install */
        found=ndrx_lh_position_get(&conf, &i, sizeof(i), O_CREAT, &pos, 
                &have_value, "lhtest");
        assert_equal(found, EXTRUE);
        
        /* try to install */
        test_mem[pos].key=i;
        test_mem[pos].some_val=i+6;
        test_mem[pos].flags=NDRX_LH_FLAG_ISUSED|NDRX_LH_FLAG_WASUSED;
        
        /* try to install again */
        found=ndrx_lh_position_get(&conf, &i, sizeof(i), O_CREAT, &pos, 
                &have_value, "lhtest");
        /* position found: */
        assert_equal(found, EXTRUE);
        
        /* search the value... */
        found=ndrx_lh_position_get(&conf, &i, sizeof(i), 0, &pos, 
                &have_value, "lhtest");
        assert_equal(found, EXTRUE);
        
        /* verify the value */
        assert_equal(test_mem[pos].some_val, i+6);
        
        /* uninstall same position and try to-reuse */
        test_mem[pos].flags=NDRX_LH_FLAG_WASUSED;
        found=ndrx_lh_position_get(&conf, &i, sizeof(i), O_CREAT, &pos, 
                &have_value, "lhtest");
        assert_equal(found, EXTRUE);
        
        
        
    }
}

/**
 * Standard library tests
 * @return
 */
TestSuite *ubf_nstd_lh(void)
{
    TestSuite *suite = create_test_suite();

    add_test(suite, test_nstd_lh_have_space);
    
    return suite;
}
/* vim: set ts=4 sw=4 et smartindent: */
