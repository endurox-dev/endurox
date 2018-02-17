/* 
** Test Buffer compare routines (Bcmp and Bsubset) 
**
** @file test_cmp.c
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

/**
 * Load data1
 */
exprivate void load1(UBFH *p_ub)
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

}

/**
 * Load data 2
 */
exprivate void load2(UBFH *p_ub)
{
    short s;
    long l;
    char c;
    float f;
    double d;
    char carr[] = "CARRAY1 TEST STRING DATA";
    BFLDLEN len = strlen(carr);

    /* Make second copy of field data (another for not equal test)*/
    s = 99;
    l = -1021;
    c = '.';
    f = 17.31;
    d = 12312.1111;
    carr[0] = 'Y';
    len = strlen(carr);

    assert_equal(Bchg(p_ub, T_SHORT_FLD, 1, (char *)&s, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_LONG_FLD, 1, (char *)&l, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_CHAR_FLD, 1, (char *)&c, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_FLOAT_FLD, 1, (char *)&f, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_DOUBLE_FLD, 1, (char *)&d, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_STRING_FLD, 1, (char *)"TEST STRING ARRAY2", 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_CARRAY_FLD, 1, (char *)carr, len), EXSUCCEED);
    
}
/**
 * Load test data 3
 */
exprivate void load3(UBFH *p_ub)
{
    short s;
    long l;
    char c;
    float f;
    double d;
    char carr[] = "CARRAY1 TEST STRING DATA";
    BFLDLEN len = strlen(carr);

    s = 212;
    l = 212;
    c = 'b';
    f = 12127;
    d = 1231232.1;
    carr[0] = 'X';
    assert_equal(Bchg(p_ub, T_SHORT_FLD, 0, (char *)&s, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_LONG_FLD, 0, (char *)&l, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_CHAR_FLD, 0, (char *)&c, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_FLOAT_FLD, 0, (char *)&f, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_DOUBLE_FLD, 0, (char *)&d, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_STRING_FLD, 0, (char *)"XTEST STR VAL", 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_CARRAY_FLD, 0, (char *)carr, len), EXSUCCEED);
}


/**
 * This simply reads all field and adds them to another buffer, then do compare
 */
Ensure(test_Bcmp)
{
    char fb[1024];
    char fb_2[2048];
    UBFH *p_ub = (UBFH *)fb;
    UBFH *p_ub_2 = (UBFH *)fb_2;
    
    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);
    assert_equal(Binit(p_ub_2, sizeof(fb_2)), EXSUCCEED);
    
    load1(p_ub);
    load1(p_ub_2);
    
    assert_equal(Bcmp(p_ub, p_ub_2), 0);
    
    
    assert_equal(Binit(p_ub_2, sizeof(fb_2)), EXSUCCEED);
    load2(p_ub_2);
    
    /* ID of first is greater (due to missing occurrences) than second buffer */
    assert_equal(Bcmp(p_ub, p_ub_2), 1);
    
    /* now reverse check */
    assert_equal(Bcmp(p_ub_2, p_ub), -1);
    
    /* ok have some value differences */
    
    
    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);
    assert_equal(Binit(p_ub_2, sizeof(fb_2)), EXSUCCEED);
    
    load3(p_ub);
    load1(p_ub_2);
    
    /* load3 short is bigger than load1 */
    assert_equal(Bcmp(p_ub, p_ub_2), 1);
    
    /* reverse */
    assert_equal(Bcmp(p_ub_2, p_ub), -1);
    
    
    /* remove some fields from buffer, to make it look shorter... (last carray) */
    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);
    assert_equal(Binit(p_ub_2, sizeof(fb_2)), EXSUCCEED);
    
    load1(p_ub);
    load1(p_ub_2);
    
    
    assert_equal(Bdel(p_ub, T_CARRAY_FLD, 0), EXSUCCEED);
    
    /* buf2 have more fields */
    assert_equal(Bcmp(p_ub, p_ub_2), -1);
    
    /* reverse */
    assert_equal(Bcmp(p_ub_2, p_ub), 1);
    
    
    /* lets check some other data types too, so that we run throught the new
     * comparator functions
     * TODO!
     */
    
    
    
    
}
TestSuite *ubf_bcmp_tests(void)
{
    TestSuite *suite = create_test_suite();

    add_test(suite, test_Bcmp);

    return suite;
}
