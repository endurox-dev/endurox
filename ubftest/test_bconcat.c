/* 
**
** @file test_bconcat.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, Mavimax, Ltd. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or Mavimax's license for commercial use.
** -----------------------------------------------------------------------------
** GPL license:
** 
** This program is free software; you can redistribute it and/or modify it under
** the terms of the GNU General Public License as published by the Free Software
** Foundation; either version 2 of the License, or (at your option) any later
** version.
**
** This program is distributed in the hope that it will be useful, but WITHOUT ANY
** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
** PARTICULAR PURPOSE. See the GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License along with
** this program; if not, write to the Free Software Foundation, Inc., 59 Temple
** Place, Suite 330, Boston, MA 02111-1307 USA
**
** -----------------------------------------------------------------------------
** A commercial use license is available from Mavimax, Ltd
** contact@mavimax.com
** -----------------------------------------------------------------------------
*/

#include <stdio.h>
#include <stdlib.h>
#include <cgreen/cgreen.h>
#include <ubf.h>
#include <ndrstandard.h>
#include <string.h>
#include "test.fd.h"
#include "ubfunit1.h"

void load_concat_test_data(UBFH *p_ub)
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
    s = 8;
    l = -21;
    c = '.';
    f = 1.31;
    d = 1231.1111;
    carr[0] = 'Y';
    len = strlen(carr);

    assert_equal(Bchg(p_ub, T_SHORT_FLD, 1, (char *)&s, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_LONG_FLD, 1, (char *)&l, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_CHAR_FLD, 1, (char *)&c, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_FLOAT_FLD, 1, (char *)&f, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_DOUBLE_FLD, 1, (char *)&d, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_STRING_FLD, 1, (char *)"TEST STRING ARRAY2", 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_CARRAY_FLD, 1, (char *)carr, len), EXSUCCEED);
#if 0
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
#endif
}

void test_concat_data_1(UBFH *p_ub)
{
    short s;
    long l;
    char c;
    float f;
    double d;
    char buf[100];
    BFLDLEN len;
    char carr[] = "CARRAY1 TEST STRING DATA";

    /* OCC 0 */
    assert_equal(Bget(p_ub, T_SHORT_FLD, 0, (char *)&s, 0), EXSUCCEED);
    assert_equal(s, 88);
    assert_equal(Bget(p_ub, T_LONG_FLD, 0, (char *)&l, 0), EXSUCCEED);
    assert_equal(l, -1021);
    assert_equal(Bget(p_ub, T_CHAR_FLD, 0, (char *)&c, 0), EXSUCCEED);
    assert_equal(c, 'c');
    assert_equal(Bget(p_ub, T_FLOAT_FLD, 0, (char *)&f, 0), EXSUCCEED);
    assert_double_equal(f, 17.31);
    assert_equal(Bget(p_ub, T_DOUBLE_FLD, 0, (char *)&d, 0), EXSUCCEED);
    assert_double_equal(d, 12312.1111);
    assert_equal(Bget(p_ub, T_STRING_FLD, 0, (char *)buf, 0), EXSUCCEED);
    assert_string_equal(buf, "TEST STR VAL");

    len = sizeof(buf);
    assert_equal(Bget(p_ub, T_CARRAY_FLD, 0, (char *)buf, &len), EXSUCCEED);
    assert_equal(strncmp(buf, carr, strlen(carr)), 0);

    /* OCC 1 */
    assert_equal(Bget(p_ub, T_SHORT_FLD, 1, (char *)&s, 0), EXSUCCEED);
    assert_equal(s, 8);
    assert_equal(Bget(p_ub, T_LONG_FLD, 1, (char *)&l, 0), EXSUCCEED);
    assert_equal(l, -21);
    assert_equal(Bget(p_ub, T_CHAR_FLD, 1, (char *)&c, 0), EXSUCCEED);
    assert_equal(c, '.');
    assert_equal(Bget(p_ub, T_FLOAT_FLD, 1, (char *)&f, 0), EXSUCCEED);
    assert_double_equal(f, 1.31);
    assert_equal(Bget(p_ub, T_DOUBLE_FLD, 1, (char *)&d, 0), EXSUCCEED);
    assert_double_equal(d, 1231.1111);
    assert_equal(Bget(p_ub, T_STRING_FLD, 1, (char *)buf, 0), EXSUCCEED);
    assert_string_equal(buf, "TEST STRING ARRAY2");

    len = sizeof(buf);
    carr[0] = 'Y';
    assert_equal(Bget(p_ub, T_CARRAY_FLD, 1, (char *)buf, &len), EXSUCCEED);
    assert_equal(strncmp(buf, carr, strlen(carr)), 0);
#if 0
    /* Test FLD2 */
    assert_equal(Bget(p_ub, T_SHORT_2_FLD, 0, (char *)&s, 0), EXSUCCEED);
    assert_equal(s, 212);
    assert_equal(Bget(p_ub, T_LONG_2_FLD, 0, (char *)&l, 0), EXSUCCEED);
    assert_equal(l, 212);
    assert_equal(Bget(p_ub, T_CHAR_2_FLD, 0, (char *)&c, 0), EXSUCCEED);
    assert_equal(c, 'b');
    assert_equal(Bget(p_ub, T_FLOAT_2_FLD, 0, (char *)&f, 0), EXSUCCEED);
    assert_double_equal(f, 12127);
    assert_equal(Bget(p_ub, T_DOUBLE_2_FLD, 0, (char *)&d, 0), EXSUCCEED);
    assert_double_equal(d, 1231232.1);
    assert_equal(Bget(p_ub, T_STRING_2_FLD, 0, (char *)buf, 0), EXSUCCEED);
    assert_string_equal(buf, "XTEST STR VAL");

    len = sizeof(buf);
    carr[0] = 'X';
    assert_equal(Bget(p_ub, T_CARRAY_2_FLD, 8, (char *)buf, &len), EXSUCCEED);
    assert_equal(strncmp(buf, carr, strlen(carr)), 0);
#endif
}

/**
 * Load alternate data, which after
 * @param p_ub
 */
void load_concat_test_data_2(UBFH *p_ub)
{
    short s = 881;
    long l = -10211;
    char c = '1';
    float f = 117.31;
    double d = 112312.1111;
    char carr[] = "CARRAY1 TEST STRING DATA1 --- SOME --- LONG - STUFF";
    BFLDLEN len = strlen(carr);

    assert_equal(Bchg(p_ub, T_SHORT_FLD, 0, (char *)&s, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_LONG_FLD, 0, (char *)&l, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_CHAR_FLD, 0, (char *)&c, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_FLOAT_FLD, 0, (char *)&f, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_DOUBLE_FLD, 0, (char *)&d, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_STRING_FLD, 0, (char *)"TEST STR VAL1M THIS IS LOGN STRING", 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_CARRAY_FLD, 0, (char *)carr, len), EXSUCCEED);

    /* Make second copy of field data (another for not equal test)*/
    s = 81;
    l = -211;
    c = '.';
    f = 11.31;
    d = 11231.1111;
    carr[0] = '2';
    len = strlen(carr);

    assert_equal(Bchg(p_ub, T_SHORT_FLD, 1, (char *)&s, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_LONG_FLD, 1, (char *)&l, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_CHAR_FLD, 1, (char *)&c, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_FLOAT_FLD, 1, (char *)&f, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_DOUBLE_FLD, 1, (char *)&d, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_STRING_FLD, 1, (char *)"3EST STRING ARRAY2 THIS IS EVEN MORE LONGER", 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_CARRAY_FLD, 1, (char *)carr, len), EXSUCCEED);

    /* This part we will keep the same */
    s = 2121;
    l = 2121;
    c = 'c';
    f = 1227;
    d = 1232.1;
    carr[0] = 'G';
    assert_equal(Bchg(p_ub, T_SHORT_2_FLD, 2, (char *)&s, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_LONG_2_FLD, 2, (char *)&l, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_CHAR_2_FLD, 2, (char *)&c, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_FLOAT_2_FLD, 2, (char *)&f, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_DOUBLE_2_FLD, 2, (char *)&d, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_STRING_2_FLD, 2, (char *)"XTEST STR VAL", 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_CARRAY_2_FLD, 2, (char *)carr, len), EXSUCCEED);
}

void test_concat_data_2(UBFH *p_ub)
{
    short s;
    long l;
    char c;
    float f;
    double d;
    char buf[100];
    BFLDLEN len;
    char carr[] = "CARRAY1 TEST STRING DATA1 --- SOME --- LONG - STUFF";

    /* OCC 0 */
    assert_equal(Bget(p_ub, T_SHORT_FLD, 2, (char *)&s, 0), EXSUCCEED);
    assert_equal(s, 881);
    assert_equal(Bget(p_ub, T_LONG_FLD, 2, (char *)&l, 0), EXSUCCEED);
    assert_equal(l, -10211);
    assert_equal(Bget(p_ub, T_CHAR_FLD, 2, (char *)&c, 0), EXSUCCEED);
    assert_equal(c, '1');
    assert_equal(Bget(p_ub, T_FLOAT_FLD, 2, (char *)&f, 0), EXSUCCEED);
    assert_double_equal(f, 117.31);
    assert_equal(Bget(p_ub, T_DOUBLE_FLD, 2, (char *)&d, 0), EXSUCCEED);
    assert_double_equal(d, 112312.1111);
    assert_equal(Bget(p_ub, T_STRING_FLD, 2, (char *)buf, 0), EXSUCCEED);
    assert_string_equal(buf, "TEST STR VAL1M THIS IS LOGN STRING");

    len = sizeof(buf);
    assert_equal(Bget(p_ub, T_CARRAY_FLD, 2, (char *)buf, &len), EXSUCCEED);
    assert_equal(strncmp(buf, carr, strlen(carr)), 0);

    /* OCC 1 */
    assert_equal(Bget(p_ub, T_SHORT_FLD, 3, (char *)&s, 0), EXSUCCEED);
    assert_equal(s, 81);
    assert_equal(Bget(p_ub, T_LONG_FLD, 3, (char *)&l, 0), EXSUCCEED);
    assert_equal(l, -211);
    assert_equal(Bget(p_ub, T_CHAR_FLD, 3, (char *)&c, 0), EXSUCCEED);
    assert_equal(c, '.');
    assert_equal(Bget(p_ub, T_FLOAT_FLD, 3, (char *)&f, 0), EXSUCCEED);
    assert_double_equal(f, 11.31);
    assert_equal(Bget(p_ub, T_DOUBLE_FLD, 3, (char *)&d, 0), EXSUCCEED);
    assert_double_equal(d, 11231.1111);
    assert_equal(Bget(p_ub, T_STRING_FLD, 3, (char *)buf, 0), EXSUCCEED);
    assert_string_equal(buf, "3EST STRING ARRAY2 THIS IS EVEN MORE LONGER");

    len = sizeof(buf);
    carr[0] = '2';
    assert_equal(Bget(p_ub, T_CARRAY_FLD, 3, (char *)buf, &len), EXSUCCEED);
    assert_equal(strncmp(buf, carr, strlen(carr)), 0);

    /* Test FLD2 */
    assert_equal(Bget(p_ub, T_SHORT_2_FLD, 2, (char *)&s, 0), EXSUCCEED);
    assert_equal(s, 2121);
    assert_equal(Bget(p_ub, T_LONG_2_FLD, 2, (char *)&l, 0), EXSUCCEED);
    assert_equal(l, 2121);
    assert_equal(Bget(p_ub, T_CHAR_2_FLD, 2, (char *)&c, 0), EXSUCCEED);
    assert_equal(c, 'c');
    assert_equal(Bget(p_ub, T_FLOAT_2_FLD, 2, (char *)&f, 0), EXSUCCEED);
    assert_double_equal(f, 1227);
    assert_equal(Bget(p_ub, T_DOUBLE_2_FLD, 2, (char *)&d, 0), EXSUCCEED);
    assert_double_equal(d, 1232.1);
    assert_equal(Bget(p_ub, T_STRING_2_FLD, 2, (char *)buf, 0), EXSUCCEED);
    assert_string_equal(buf, "XTEST STR VAL");

    len = sizeof(buf);
    carr[0] = 'G';
    assert_equal(Bget(p_ub, T_CARRAY_2_FLD, 2, (char *)buf, &len), EXSUCCEED);
    assert_equal(strncmp(buf, carr, strlen(carr)), 0);
}


/**
 * This simply reads all field and adds them to another buffer, then do compare
 */
Ensure(test_fconcat)
{
    char fb[9000];
    UBFH *p_ub = (UBFH *)fb;

    char fb2[9000];
    UBFH *p_ub2 = (UBFH *)fb2;

    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);
    assert_equal(Binit(p_ub2, sizeof(fb2)), EXSUCCEED);

    load_concat_test_data(p_ub);
    /* Do the full copy - output should be the same */
    assert_equal(Bconcat(p_ub2, p_ub), EXSUCCEED);
    assert_equal(memcmp(p_ub2, p_ub, Bused(p_ub2)), 0);

    /* Do the update over again - should be the same */
    assert_equal(memcmp(p_ub2, p_ub, Bused(p_ub2)), 0);
    test_concat_data_1(p_ub2);

    /* -------------------------------------------------------- */
    /* test real append */
    assert_equal(Binit(p_ub2, sizeof(fb2)), EXSUCCEED);
    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);

    load_concat_test_data(p_ub);
    load_concat_test_data_2(p_ub2);
    assert_equal(Bconcat(p_ub, p_ub2), EXSUCCEED);

    /* Both tests should be passed. */
    test_concat_data_1(p_ub);
    test_concat_data_2(p_ub);

}

TestSuite *ubf_fconcat_tests(void)
{
    TestSuite *suite = create_test_suite();

    add_test(suite, test_fconcat);

    return suite;
}

