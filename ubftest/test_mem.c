/**
 *
 * @file test_mem.c
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
#include "test.fd.h"
#include "ubfunit1.h"

/**
 * Test Balloc
 * @return
 */
Ensure(test_Balloc_Bfree)
{
    UBFH *p_ub = NULL;
    int i;
    long j;
    char block[1230];
    /* will check with valgrind - do we have memory leaks or not */
    
    for (i=0; i<10; i++)
    {
        p_ub=Balloc(20, sizeof(block)*20);
        assert_not_equal(p_ub, NULL);
        
        for (j=0; j<20; j++)
        {
            assert_equal(Badd(p_ub, T_CARRAY_FLD, block, sizeof(block)), EXSUCCEED);
        }
        
        /* this one shall fail as no space ...*/
        /* lets leave some few for alignment 
         * if payload is big enough, no add data needed
        for (j=0; j<10; j++)
        {
            Badd(p_ub, T_CARRAY_FLD, block, sizeof(block));
        }
        */
        
        assert_equal(Badd(p_ub, T_CARRAY_FLD, block, sizeof(block)), EXFAIL);
        assert_equal(Berror, BNOSPACE);
        
        /* Feature #393
         * ensure that we got a correct size estimate with working buffer 
         */
        assert_equal(Bneeded(20, sizeof(block)*20), Bsizeof(p_ub));
        
        assert_equal(Bfree(p_ub), EXSUCCEED);
    }
}

/**
 * Basic test for reallocation
 */
Ensure(test_Brealloc)
{
    UBFH *p_ub = NULL;
    int i;
    long j;
    int loop;
    char block[1230];
    /* will check with valgrind - do we have memory leaks or not */
    
    for (i=0; i<10; i++)
    {
        p_ub=Balloc(20, sizeof(block)*20);
        assert_not_equal(p_ub, NULL);
        
        for (j=0; j<20; j++)
        {
            assert_equal(Badd(p_ub, T_CARRAY_FLD, block, sizeof(block)), EXSUCCEED);
        }
        
        /* this one shall fail as no space ...*/
        /* lets leave some few for alignment 
        for (j=0; j<10; j++)
        {
            Badd(p_ub, T_CARRAY_FLD, block, sizeof(block));
        }
        */
        
        assert_equal(Badd(p_ub, T_CARRAY_FLD, block, sizeof(block)), EXFAIL);
        assert_equal(Berror, BNOSPACE);
        
        /* now realloc... to 40 and 40 shall fill in */
        p_ub=Balloc(40, sizeof(block)*40);
        assert_not_equal(p_ub, NULL);
        
        loop = 40 - Bnum(p_ub);
        for (j=0; j<loop; j++)
        {
            Badd(p_ub, T_CARRAY_FLD, block, sizeof(block));
        }
        
        /* this one shall fail as no space ...*/
        /* lets leave some few for alignment 
        for (j=0; j<10; j++)
        {
            Badd(p_ub, T_CARRAY_FLD, block, sizeof(block));
        }
        */
        
        assert_equal(Badd(p_ub, T_CARRAY_FLD, block, sizeof(block)), EXFAIL);
        assert_equal(Berror, BNOSPACE);
        
        assert_equal(Bfree(p_ub), EXSUCCEED);

    }

}

TestSuite *ubf_mem_tests(void)
{
    TestSuite *suite = create_test_suite();

    add_test(suite, test_Balloc_Bfree);
    add_test(suite, test_Brealloc);

    return suite;
}

/* vim: set ts=4 sw=4 et smartindent: */
