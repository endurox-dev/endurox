/**
 *
 * @file test_mem.c
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
    /* will check with valgrind - do we have memory leaks or not */
    
    for (i=0; i<10; i++)
    {
        p_ub=Balloc(20, 30);
        assert_not_equal(p_ub, NULL);
        /* Put some data into memory so that we can test */
        set_up_dummy_data(p_ub);
        do_dummy_data_test(p_ub);
        assert_equal(Bfree(p_ub), EXSUCCEED);

    }
}

/**
 * Basic test for reallocation
 */
Ensure(test_Brealloc)
{
    UBFH *p_ub = NULL;

    p_ub=Balloc(1, 30);
    assert_not_equal(p_ub, NULL);

    assert_equal(Badd(p_ub, T_STRING_FLD, BIG_TEST_STRING, 0), EXFAIL);
    assert_equal(Berror, BNOSPACE);

    /* Now reallocate, space should be bigger! */
    p_ub=Brealloc(p_ub, 1, strlen(BIG_TEST_STRING)+1+2/* align */);
    assert_not_equal(p_ub, NULL);
    assert_equal(Badd(p_ub, T_STRING_FLD, BIG_TEST_STRING, 0), EXSUCCEED);
    
    /* should not allow to reallocate to 0! */
    assert_equal(Brealloc(p_ub, 1, 0), NULL);
    assert_equal(Berror, BEINVAL);

    /* should be bigger than existing. 4 is sizeof bfld, first in stuct */
    assert_equal(Brealloc(p_ub, 1, strlen(BIG_TEST_STRING)-4), NULL);
    assert_equal(Berror, BEINVAL);

    assert_equal(EXSUCCEED, Bfree(p_ub));

}

TestSuite *ubf_mem_tests(void)
{
    TestSuite *suite = create_test_suite();

    add_test(suite, test_Balloc_Bfree);
    add_test(suite, test_Brealloc);

    return suite;
}

/* vim: set ts=4 sw=4 et smartindent: */
