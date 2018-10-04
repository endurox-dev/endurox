/**
 *
 * @file test_macro.c
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
 * Performs basic macro function testing.
 */
Ensure(test_macros)
{
    char fb[1024];
    UBFH *p_ub = (UBFH *)fb;
    char test_buf[64];
    char *p;
    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);

    assert_equal(Badds (p_ub, T_SHORT_FLD, "321"), EXSUCCEED);
    assert_equal(Bchgs (p_ub, T_LONG_FLD, 10, "88889"), EXSUCCEED);
    assert_equal(Bchgs (p_ub, T_CARRAY_FLD, 5, "THIS IS TEST VALUE"), EXSUCCEED);
    
    assert_equal(Bgets (p_ub, T_LONG_FLD, 10, test_buf), EXSUCCEED);
    assert_string_equal(test_buf, "88889");
    assert_equal(Bgets (p_ub, T_SHORT_FLD, 0, test_buf), EXSUCCEED);
    assert_string_equal(test_buf, "321");

    /* Get with allocate */
    assert_string_equal((p=Bgetsa (p_ub, T_CARRAY_FLD, 5, 0)), "THIS IS TEST VALUE");
    assert_not_equal(p, NULL);
    free(p);
    
    assert_string_equal(Bfinds (p_ub, T_CARRAY_FLD, 5), "THIS IS TEST VALUE");
    
}

TestSuite *ubf_macro_tests(void)
{
    TestSuite *suite = create_test_suite();

    add_test(suite, test_macros);
    
    return suite;
}
/* vim: set ts=4 sw=4 et smartindent: */
