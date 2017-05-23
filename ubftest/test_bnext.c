/* 
**
** @file test_bnext.c
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


void load_fdel_test_data(UBFH *p_ub)
{
    short s = 88;
    long l = -1021;
    char c = 'c';
    float f = 17.31;
    double d = 12312.1111;
    char carr[] = "CARRAY1 TEST STRING DATA";
    BFLDLEN len = strlen(carr);

    assert_equal(Bchg(p_ub, T_SHORT_FLD, 0, (char *)&s, 0), SUCCEED);
    assert_equal(Bchg(p_ub, T_LONG_FLD, 0, (char *)&l, 0), SUCCEED);
    assert_equal(Bchg(p_ub, T_CHAR_FLD, 0, (char *)&c, 0), SUCCEED);
    assert_equal(Bchg(p_ub, T_FLOAT_FLD, 0, (char *)&f, 0), SUCCEED);
    assert_equal(Bchg(p_ub, T_DOUBLE_FLD, 0, (char *)&d, 0), SUCCEED);
    assert_equal(Bchg(p_ub, T_STRING_FLD, 0, (char *)"TEST STR VAL", 0), SUCCEED);
    assert_equal(Bchg(p_ub, T_CARRAY_FLD, 0, (char *)carr, len), SUCCEED);

    /* Make second copy of field data (another for not equal test)*/
    s = 88;
    l = -1021;
    c = '.';
    f = 17.31;
    d = 12312.1111;
    carr[0] = 'Y';
    len = strlen(carr);

    assert_equal(Bchg(p_ub, T_SHORT_FLD, 1, (char *)&s, 0), SUCCEED);
    assert_equal(Bchg(p_ub, T_LONG_FLD, 1, (char *)&l, 0), SUCCEED);
    assert_equal(Bchg(p_ub, T_CHAR_FLD, 1, (char *)&c, 0), SUCCEED);
    assert_equal(Bchg(p_ub, T_FLOAT_FLD, 1, (char *)&f, 0), SUCCEED);
    assert_equal(Bchg(p_ub, T_DOUBLE_FLD, 1, (char *)&d, 0), SUCCEED);
    assert_equal(Bchg(p_ub, T_STRING_FLD, 1, (char *)"TEST STRING ARRAY2", 0), SUCCEED);
    assert_equal(Bchg(p_ub, T_CARRAY_FLD, 1, (char *)carr, len), SUCCEED);

    s = 212;
    l = 212;
    c = 'b';
    f = 12127;
    d = 1231232.1;
    carr[0] = 'X';
    assert_equal(Bchg(p_ub, T_SHORT_2_FLD, 0, (char *)&s, 0), SUCCEED);
    assert_equal(Bchg(p_ub, T_LONG_2_FLD, 0, (char *)&l, 0), SUCCEED);
    assert_equal(Bchg(p_ub, T_CHAR_2_FLD, 0, (char *)&c, 0), SUCCEED);
    assert_equal(Bchg(p_ub, T_FLOAT_2_FLD, 0, (char *)&f, 0), SUCCEED);
    assert_equal(Bchg(p_ub, T_DOUBLE_2_FLD, 0, (char *)&d, 0), SUCCEED);
    assert_equal(Bchg(p_ub, T_STRING_2_FLD, 0, (char *)"XTEST STR VAL", 0), SUCCEED);
    assert_equal(Bchg(p_ub, T_CARRAY_2_FLD, 0, (char *)carr, len), SUCCEED);
}

/**
 * This simply reads all field and adds them to another buffer, then do compare
 */
Ensure(test_fnext_simple)
{
    char fb[400];
    char fb_2[400];
    UBFH *p_ub = (UBFH *)fb;
    UBFH *p_ub_2 = (UBFH *)fb;
    BFLDID bfldid;
    BFLDOCC occ;
    char data_buf[100];
    BFLDLEN  len;
    int fldcount=0;

    assert_equal(Binit(p_ub, sizeof(fb)), SUCCEED);
    assert_equal(Binit(p_ub_2, sizeof(fb_2)), SUCCEED);
    load_fdel_test_data(p_ub);
    
    len = sizeof(data_buf);
    bfldid = BFIRSTFLDID;
    while(1==Bnext(p_ub, &bfldid, &occ, data_buf, &len))
    {
        /* Put the data inside of the other buffer... */
        assert_equal(Bchg(p_ub_2, bfldid, occ, data_buf, len), SUCCEED);
        /* Got the value? */
        len = sizeof(data_buf);
    }
    /* Now do the mem compare (this is not the best way to test,
     * but will be OK for now */
    assert_equal(memcmp(p_ub, p_ub_2, sizeof(fb)),0);

    /* Now test the count because buffer is NULL */
    bfldid = BFIRSTFLDID;
    while(1==Bnext(p_ub, &bfldid, &occ, NULL, NULL))
    {
        fldcount++;
    }

    assert_equal(fldcount, 21);
}

/**
 * This simply reads all field and adds them to another buffer, then do compare
 */
Ensure(test_fnext_chk_errors)
{
    char fb[400];
    UBFH *p_ub = (UBFH *)fb;
    BFLDID bfldid;
    BFLDOCC occ;
    char data_buf[100];
    BFLDLEN  len;

    assert_equal(Binit(p_ub, sizeof(fb)), SUCCEED);
    load_fdel_test_data(p_ub);

    bfldid = BFIRSTFLDID;
    assert_equal(Bnext(p_ub, NULL, &occ, data_buf, &len), FAIL);
    assert_equal(Berror, BEINVAL);
    assert_equal(Bnext(p_ub, &bfldid, NULL, data_buf, &len), FAIL);
    assert_equal(Berror, BEINVAL);
    
    bfldid = BFIRSTFLDID;
    len = sizeof(data_buf);
    assert_equal(Bnext(p_ub, &bfldid, &occ, data_buf, &len), 1);
    /* Now try again, but change the buffer */
    assert_equal(Bdel(p_ub, T_SHORT_2_FLD, 0), SUCCEED);

    /* Buffer have been changed */
    assert_equal(Bnext(p_ub, &bfldid, &occ, data_buf, &len), FAIL);
    assert_equal(Berror, BEINVAL);

    /* Now try again, but p_ub is NULL */
    bfldid = BFIRSTFLDID;
    assert_equal(Bnext(NULL, &bfldid, &occ, data_buf, &len), FAIL);
    assert_equal(Berror, BNOTFLD);

    
}

TestSuite *ubf_fnext_tests(void)
{
    TestSuite *suite = create_test_suite();

    add_test(suite, test_fnext_simple);
    add_test(suite, test_fnext_chk_errors);

    return suite;
}
