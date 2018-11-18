/**
 *
 * @file test_bdel.c
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


void load_fdel_test_data_1(UBFH *p_ub)
{
    short s = 102;
    long l = 10212312;
    char c = 'a';
    float f = 127.001;
    double d = 12312312.1112;
    char carr[] = "CARRAY TEST";
    BFLDLEN len = strlen(carr);

    assert_equal(Bchg(p_ub, T_SHORT_FLD, 0, (char *)&s, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_LONG_FLD, 0, (char *)&l, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_CHAR_FLD, 0, (char *)&c, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_FLOAT_FLD, 0, (char *)&f, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_DOUBLE_FLD, 0, (char *)&d, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_STRING_FLD, 0, (char *)"TEST STR VAL", 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_CARRAY_FLD, 0, (char *)carr, len), EXSUCCEED);

    /* Make second copy of field data (another for not equal test)*/
    s = 212;
    l = 212;
    c = 'b';
    f = 12127;
    d = 1231232.1;
    carr[0] = 'X';
    assert_equal(Bchg(p_ub, T_SHORT_2_FLD, 0, (char *)&s, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_LONG_2_FLD, 0, (char *)&l, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_CHAR_2_FLD, 0, (char *)&c, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_FLOAT_2_FLD, 0, (char *)&f, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_DOUBLE_2_FLD, 0, (char *)&d, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_STRING_2_FLD, 0, (char *)"XTEST STR VAL", 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_CARRAY_2_FLD, 0, (char *)carr, len), EXSUCCEED);
}

Ensure(test_fdel_simple)
{
    char fb[1500];
    UBFH *p_ub = (UBFH *)fb;
    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);
    /* Load test data for fdel test */
    load_fdel_test_data_1(p_ub);
    set_up_dummy_data(p_ub);
    
    assert_equal(Bdel(p_ub, T_SHORT_FLD, 0), EXSUCCEED);
    assert_false(Bpres(p_ub, T_SHORT_FLD, 0));

    assert_equal(Bdel(p_ub, T_LONG_FLD, 0), EXSUCCEED);
    assert_false(Bpres(p_ub, T_LONG_FLD, 0));

    assert_equal(Bdel(p_ub, T_CHAR_FLD, 0), EXSUCCEED);
    assert_false(Bpres(p_ub, T_CHAR_FLD, 0));

    assert_equal(Bdel(p_ub, T_FLOAT_FLD, 0), EXSUCCEED);
    assert_false(Bpres(p_ub, T_FLOAT_FLD, 0));

    assert_equal(Bdel(p_ub, T_DOUBLE_FLD, 0), EXSUCCEED);
    assert_false(Bpres(p_ub, T_DOUBLE_FLD, 0));

    assert_equal(Bdel(p_ub, T_STRING_FLD, 0), EXSUCCEED);
    assert_false(Bpres(p_ub, T_STRING_FLD, 0));

    assert_equal(Bdel(p_ub, T_CARRAY_FLD, 0), EXSUCCEED);
    assert_false(Bpres(p_ub, T_CARRAY_FLD, 0));

    do_dummy_data_test(p_ub);
}

TestSuite *ubf_fdel_tests(void)
{
    TestSuite *suite = create_test_suite();

    add_test(suite, test_fdel_simple);
    
    return suite;
}

/* vim: set ts=4 sw=4 et smartindent: */
