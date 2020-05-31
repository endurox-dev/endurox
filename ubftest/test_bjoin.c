/**
 * @brief Test for Source buffer join functionality to Destination buffer (Bjoin)
 *
 * @file test_bjoin.c
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
#include "ndebug.h"
#include <ubf_int.h>

void load_test_data_bjoin_dst(UBFH *p_ub)
{
    short s = 88;
    long l = -1021;
    char c = 'c';
    float f = 17.31;
    double d = 12312.1111;
    char carr[] = "CARRAY1 TEST STRING DATA";
    BFLDLEN len = strlen(carr);

    assert_equal(Bchg(p_ub, T_SHORT_FLD, 0, (char *)&s, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_LONG_FLD, 0, (char *)&l, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_CHAR_FLD, 0, (char *)&c, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_FLOAT_FLD, 0, (char *)&f, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_DOUBLE_FLD, 0, (char *)&d, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_STRING_FLD, 0, (char *)"TEST STR VAL", 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_CARRAY_FLD, 0, (char *)carr, len), EXSUCCEED);

    /* Make second copy of field data (another for not equal test)*/
    s = 88;
    l = -1021;
    c = '.';
    f = 17.31;
    d = 12312.1111;
    carr[0] = 'Y';
    len = strlen(carr);

    assert_equal(Bchg(p_ub, T_SHORT_FLD, 1, (char *)&s, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_CHAR_FLD, 1, (char *)&c, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_FLOAT_FLD, 1, (char *)&f, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_DOUBLE_FLD, 1, (char *)&d, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_STRING_FLD, 1, (char *)"TEST STRING 2", 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_CARRAY_FLD, 1, (char *)carr, len), EXSUCCEED);

    /* Make second copy of field data (another for not equal test)*/
    l = -2323;
    c = 'Q';
    f = 12.31;
    d = 6547.1111;

    assert_equal(Bchg(p_ub, T_CHAR_FLD, 2, (char *)&c, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_FLOAT_FLD, 2, (char *)&f, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_DOUBLE_FLD, 2, (char *)&d, 0), EXSUCCEED);
}

void load_test_data_bjoin_src(UBFH *p_ub)
{
    short s = 222;
    long l = 23456789;
    char c = 'z';
    float f = 9.9;
    double d = 987654.9876;

    assert_equal(Bchg(p_ub, T_SHORT_FLD, 0, (char *)&s, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_LONG_FLD, 0, (char *)&l, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_CHAR_FLD, 0, (char *)&c, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_FLOAT_FLD, 0, (char *)&f, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_DOUBLE_FLD, 0, (char *)&d, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_STRING_FLD, 0, (char *)"TEST STR VAL JOIN", 0), EXSUCCEED);

    /* Make second copy of field data (another for not equal test)*/
    s = 33;
    l = -2048;
    c = 'X';
    f = 8.8;
    d = 12312.1111;

    assert_equal(Bchg(p_ub, T_SHORT_FLD, 1, (char *)&s, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_LONG_FLD, 1, (char *)&l, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_CHAR_FLD, 1, (char *)&c, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_FLOAT_FLD, 1, (char *)&f, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_DOUBLE_FLD, 1, (char *)&d, 0), EXSUCCEED);

    /* Make second copy of field data (another for not equal test)*/
    c = 'Q';
    f = 7.7;

    assert_equal(Bchg(p_ub, T_CHAR_FLD, 2, (char *)&c, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_FLOAT_FLD, 2, (char *)&f, 0), EXSUCCEED);

    /* Make second copy of field data (another for not equal test)*/
    c = 'Q';
    f = 6.6;

    assert_equal(Bchg(p_ub, T_CHAR_FLD, 3, (char *)&c, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_FLOAT_FLD, 3, (char *)&f, 0), EXSUCCEED);
}

Ensure(test_bjoin_simple)
{
    char fb_src[1400];
    char fb_dst[1400];
    UBFH *p_ub_src = (UBFH *)fb_src;
    UBFH *p_ub_dst = (UBFH *)fb_dst;
    
    short s;
    long l;
    char c;
    float f;
    double d;
    char buf[100];

    UBF_header_t *hsrc = (UBF_header_t *)p_ub_src;
    UBF_header_t *hdst = (UBF_header_t *)p_ub_dst;

    Binit(p_ub_src, sizeof(fb_src));
    Binit(p_ub_dst, sizeof(fb_dst));

    load_test_data_bjoin_src(p_ub_src);
    load_test_data_bjoin_dst(p_ub_dst);

    significant_figures_for_assert_double_are(FLOAT_RESOLUTION);

    assert_equal(Bjoin(p_ub_dst, p_ub_src),EXSUCCEED);

    UBF_LOG(log_debug, "Field num in src buffer after Bjoin [%d]", Bnum(p_ub_src));
    UBF_LOG(log_debug, "Field num in dst buffer after Bjoin [%d]", Bnum(p_ub_dst));

    assert_equal(Bget(p_ub_dst, T_SHORT_FLD, 0, (char *)&s, 0), EXSUCCEED);
    assert_equal(s,222);
    assert_equal(Bget(p_ub_dst, T_LONG_FLD, 0, (char *)&l, 0), EXSUCCEED);
    assert_equal(l,23456789);
    assert_equal(Bget(p_ub_dst, T_CHAR_FLD, 0, (char *)&c, 0), EXSUCCEED);
    assert_equal(c,'z');
    assert_equal(Bget(p_ub_dst, T_FLOAT_FLD, 0, (char *)&f, 0), EXSUCCEED);
    assert_double_equal(f,9.9);
    assert_equal(Bget(p_ub_dst, T_DOUBLE_FLD, 0, (char *)&d, 0), EXSUCCEED);
    assert_double_equal(d,987654.9876);
    assert_equal(Bget(p_ub_dst, T_STRING_FLD, 0, (char *)buf, 0), EXSUCCEED);
    assert_string_equal(buf,"TEST STR VAL JOIN");
    assert_equal(Bpres(p_ub_dst, T_CARRAY_FLD, 0), EXFALSE);

    assert_equal(Bget(p_ub_dst, T_SHORT_FLD, 1, (char *)&s, 0), EXSUCCEED);
    assert_equal(s,33);
    assert_equal(Bpres(p_ub_dst, T_LONG_FLD, 1), EXFALSE);
    assert_equal(Bget(p_ub_dst, T_CHAR_FLD, 1, (char *)&c, 0), EXSUCCEED);
    assert_equal(c,'X');
    assert_equal(Bget(p_ub_dst, T_FLOAT_FLD, 1, (char *)&f, 0), EXSUCCEED);
    assert_double_equal(f,8.8);
    assert_equal(Bget(p_ub_dst, T_DOUBLE_FLD, 1, (char *)&d, 0), EXSUCCEED);
    assert_double_equal(d,12312.1111);
    assert_equal(Bpres(p_ub_dst, T_STRING_FLD, 1), EXFALSE);

    assert_equal(Bget(p_ub_dst, T_CHAR_FLD, 2, (char *)&c, 0), EXSUCCEED);
    assert_equal(c,'Q');
    assert_equal(Bget(p_ub_dst, T_FLOAT_FLD, 2, (char *)&f, 0), EXSUCCEED);
    assert_double_equal(f,7.7);
    assert_equal(Bpres(p_ub_dst, T_DOUBLE_FLD, 2), EXFALSE);

}

TestSuite *ubf_bjoin_tests(void)
{
    TestSuite *suite = create_test_suite();

    add_test(suite, test_bjoin_simple);

    return suite;
}

/* vim: set ts=4 sw=4 et smartindent: */
