/**
 *
 * @file test_genbuf.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or Mavimax's license for commercial use.
 * -----------------------------------------------------------------------------
 * GPL license:
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
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
#include "fdatatype.h"

/**
 * Bcpy tests.
 */
Ensure(test_Bcpy)
{
    char buf1[550];
    char buf2[480];
    char data_buf[480];

    BFLDLEN len;
    UBFH *p_ub_src = (UBFH *)buf1;
    UBFH *p_ub_dst = (UBFH *)buf2;

    assert_equal(Binit(p_ub_src, sizeof(buf1)), EXSUCCEED);
    assert_equal(Binit(p_ub_dst, sizeof(buf2)), EXSUCCEED);

    /* Having no data it should copy OK */
    assert_equal(Bcpy(p_ub_dst, p_ub_src), EXSUCCEED);
    /* Set up some 300 bytes */
    len = sizeof(buf2);
    assert_equal(CBchg(p_ub_src, T_CARRAY_FLD, 0, data_buf, sizeof(data_buf), BFLD_CARRAY), EXSUCCEED);

    /* Now copy should fail, because of no space in buffer! */
    assert_equal_with_message(Bcpy(p_ub_dst, p_ub_src), EXFAIL, "Dest have no space!");
}

/**
 * Test user type conversation function
 * which dynamically allocates the buffer.
 */
Ensure(test_btypcvt)
{
    BFLDLEN len;
    short s=-11;
    long l=111;
    char c='3';
    float f=1.33341;
    double d=5547;
    char *p;

    /* Convert to short validation */
    assert_not_equal((p=Btypcvt(&len, BFLD_SHORT, (char *)&s, BFLD_SHORT, 0)), NULL);
    assert_equal(*((short *)p), -11);
    free(p);
    assert_not_equal((p=Btypcvt(&len, BFLD_SHORT, (char *)&l, BFLD_LONG, 0)), NULL);
    assert_equal(*((short *)p), 111);
    free(p);
    assert_not_equal((p=Btypcvt(&len, BFLD_SHORT, (char *)&c, BFLD_CHAR, 0)), NULL);
    assert_equal(*((short *)p), 51);
    free(p);
    assert_not_equal((p=Btypcvt(&len, BFLD_SHORT, (char *)&f, BFLD_FLOAT, 0)), NULL);
    assert_equal(*((short *)p), 1);
    free(p);
    assert_not_equal((p=Btypcvt(&len, BFLD_SHORT, (char *)&d, BFLD_DOUBLE, 0)), NULL);
    assert_equal(*((short *)p), 5547);
    free(p);
    assert_not_equal((p=Btypcvt(&len, BFLD_SHORT, (char *)"12701", BFLD_STRING, 0)), NULL);
    assert_equal(*((short *)p), 12701);
    assert_equal(len, 2);
    free(p);
    assert_not_equal((p=Btypcvt(&len, BFLD_SHORT, (char *)"12801", BFLD_CARRAY, 5)), NULL);
    assert_equal(*((short *)p), 12801);
    free(p);

    /* Convert to long validation */
    assert_not_equal((p=Btypcvt(&len, BFLD_LONG, (char *)&s, BFLD_SHORT, 0)), NULL);
    assert_equal(*((long *)p), -11);
    free(p);
    assert_not_equal((p=Btypcvt(&len, BFLD_LONG, (char *)&l, BFLD_LONG, 0)), NULL);
    assert_equal(*((long *)p), 111);
    free(p);
    assert_not_equal((p=Btypcvt(&len, BFLD_LONG, (char *)&c, BFLD_CHAR, 0)), NULL);
    assert_equal(*((long *)p), 51);
    free(p);
    assert_not_equal((p=Btypcvt(&len, BFLD_LONG, (char *)&f, BFLD_FLOAT, 0)), NULL);
    assert_equal(*((long *)p), 1);
    free(p);
    assert_not_equal((p=Btypcvt(&len, BFLD_LONG, (char *)&d, BFLD_DOUBLE, 0)), NULL);
    assert_equal(*((long *)p), 5547);
    free(p);
    assert_not_equal((p=Btypcvt(&len, BFLD_LONG, (char *)"12701123", BFLD_STRING, 0)), NULL);
    assert_equal(*((long *)p), 12701123);
    /* fix for 32bit system */
    if (sizeof(long)==8)
    	assert_equal(len, 8);
    else
        assert_equal(len, 4);

    free(p);
    assert_not_equal((p=Btypcvt(&len, BFLD_LONG, (char *)"12801123", BFLD_CARRAY, 8)), NULL);
    assert_equal(*((long *)p), 12801123);
    free(p);

    /* Convert to char validation */
    s=111;
    assert_not_equal((p=Btypcvt(&len, BFLD_CHAR, (char *)&s, BFLD_SHORT, 0)), NULL);
    assert_equal(*((char *)p), 'o');
    free(p);
    s=-11;
    
    assert_not_equal((p=Btypcvt(&len, BFLD_CHAR, (char *)&l, BFLD_LONG, 0)), NULL);
    assert_equal(*((char *)p), 'o');
    free(p);
    assert_not_equal((p=Btypcvt(&len, BFLD_CHAR, (char *)&c, BFLD_CHAR, 0)), NULL);
    assert_equal(*((char *)p), '3');
    free(p);
    assert_not_equal((p=Btypcvt(&len, BFLD_CHAR, (char *)&f, BFLD_FLOAT, 0)), NULL);
    assert_equal(*((char *)p), 1);
    free(p);
    d=47;
    assert_not_equal((p=Btypcvt(&len, BFLD_CHAR, (char *)&d, BFLD_DOUBLE, 0)), NULL);
    assert_equal(*((char *)p), '/');
    d=5547;
    free(p);
    assert_not_equal((p=Btypcvt(&len, BFLD_CHAR, (char *)"abc", BFLD_STRING, 0)), NULL);
    assert_equal(*((char *)p), 'a');
    assert_equal(len, 1);
    free(p);
    assert_not_equal((p=Btypcvt(&len, BFLD_CHAR, (char *)"cde", BFLD_CARRAY, 8)), NULL);
    assert_equal(*((char *)p), 'c');
    free(p);


    /* Convert to float validation */
    assert_not_equal((p=Btypcvt(&len, BFLD_FLOAT, (char *)&s, BFLD_SHORT, 0)), NULL);
    assert_double_equal(*((float *)p), -11);
    free(p);
    assert_not_equal((p=Btypcvt(&len, BFLD_FLOAT, (char *)&l, BFLD_LONG, 0)), NULL);
    assert_double_equal(*((float *)p), 111);
    free(p);
    assert_not_equal((p=Btypcvt(&len, BFLD_FLOAT, (char *)&c, BFLD_CHAR, 0)), NULL);
    assert_double_equal(*((float *)p), 51);
    free(p);
    assert_not_equal((p=Btypcvt(&len, BFLD_FLOAT, (char *)&f, BFLD_FLOAT, 0)), NULL);
    assert_double_equal(*((float *)p), 1.33341);
    free(p);
    assert_not_equal((p=Btypcvt(&len, BFLD_FLOAT, (char *)&d, BFLD_DOUBLE, 0)), NULL);
    assert_double_equal(*((float *)p), 5547);
    free(p);
    assert_not_equal((p=Btypcvt(&len, BFLD_FLOAT, (char *)"12701.1", BFLD_STRING, 0)), NULL);
    assert_double_equal(*((float *)p), 12701.1);
    assert_equal(len, 4);
    free(p);
    assert_not_equal((p=Btypcvt(&len, BFLD_FLOAT, (char *)"128.1", BFLD_CARRAY, 5)), NULL);
    assert_double_equal(*((float *)p), 128.1);
    free(p);


    /* Convert to double validation */
    assert_not_equal((p=Btypcvt(&len, BFLD_DOUBLE, (char *)&s, BFLD_SHORT, 0)), NULL);
    assert_double_equal(*((double *)p), -11);
    free(p);
    assert_not_equal((p=Btypcvt(&len, BFLD_DOUBLE, (char *)&l, BFLD_LONG, 0)), NULL);
    assert_double_equal(*((double *)p), 111);
    free(p);
    assert_not_equal((p=Btypcvt(&len, BFLD_DOUBLE, (char *)&c, BFLD_CHAR, 0)), NULL);
    assert_double_equal(*((double *)p), 51);
    free(p);
    assert_not_equal((p=Btypcvt(&len, BFLD_DOUBLE, (char *)&f, BFLD_FLOAT, 0)), NULL);
    assert_double_equal(*((double *)p), 1.33341);
    free(p);
    assert_not_equal((p=Btypcvt(&len, BFLD_DOUBLE, (char *)&d, BFLD_DOUBLE, 0)), NULL);
    assert_double_equal(*((double *)p), 5547);
    free(p);
    assert_not_equal((p=Btypcvt(&len, BFLD_DOUBLE, (char *)"12701.1", BFLD_STRING, 0)), NULL);
    assert_double_equal(*((double *)p), 12701.1);
    assert_equal(len, 8);
    free(p);
    assert_not_equal((p=Btypcvt(&len, BFLD_DOUBLE, (char *)"128.1", BFLD_CARRAY, 5)), NULL);
    assert_double_equal(*((double *)p), 128.1);
    free(p);


    /* Convert to string validation */
    assert_string_equal((p=Btypcvt(&len, BFLD_STRING, (char *)&s, BFLD_SHORT, 0)), "-11");
    free(p);
    assert_string_equal((p=Btypcvt(&len, BFLD_STRING, (char *)&l, BFLD_LONG, 0)), "111");
    free(p);
    assert_string_equal((p=Btypcvt(&len, BFLD_STRING, (char *)&c, BFLD_CHAR, 0)), "3");
    free(p);
    assert_string_equal((p=Btypcvt(&len, BFLD_STRING, (char *)&f, BFLD_FLOAT, 0)), "1.33341");
    free(p);
    assert_string_equal((p=Btypcvt(&len, BFLD_STRING, (char *)&d, BFLD_DOUBLE, 0)), "5547.000000");
    free(p);
    assert_string_equal((p=Btypcvt(&len, BFLD_STRING, (char *)"TEST STRING", BFLD_STRING, 0)), "TEST STRING");
    free(p);
    assert_string_equal((p=Btypcvt(&len, BFLD_STRING, (char *)"TEST CARRAY", BFLD_CARRAY, 11)), "TEST CARRAY");
    free(p);
    
    /* Convert to string validation */
    assert_not_equal((p=Btypcvt(&len, BFLD_CARRAY, (char *)&s, BFLD_SHORT, 0)), NULL);
    assert_equal(strncmp(p, "-11", 3), 0);
    free(p);
    assert_not_equal((p=Btypcvt(&len, BFLD_CARRAY, (char *)&l, BFLD_LONG, 0)), NULL);
    assert_equal(strncmp(p, "111", 3), 0);
    free(p);
    assert_not_equal((p=Btypcvt(&len, BFLD_CARRAY, (char *)&c, BFLD_CHAR, 0)), NULL);
    assert_equal(strncmp(p, "3", 1), 0);
    free(p);
    assert_not_equal((p=Btypcvt(&len, BFLD_CARRAY, (char *)&f, BFLD_FLOAT, 0)), NULL);
    assert_equal(strncmp(p, "1.33341", 7), 0);
    free(p);
    assert_not_equal((p=Btypcvt(&len, BFLD_CARRAY, (char *)&d, BFLD_DOUBLE, 0)), NULL);
    assert_equal(strncmp(p, "5547.000000", 11), 0);
    free(p);
    assert_not_equal((p=Btypcvt(&len, BFLD_CARRAY, (char *)"TEST STRING", BFLD_STRING, 0)), NULL);
    assert_equal(strncmp(p, "TEST STRING", 11), 0);
    free(p);
    assert_not_equal((p=Btypcvt(&len, BFLD_CARRAY, (char *)"TEST CARRAY", BFLD_CARRAY, 11)), NULL);
    assert_equal(strncmp(p, "TEST CARRAY", 11), 0);
    assert_equal(len, 11);
    free(p);

    /* test invalid types */
    assert_equal((p=Btypcvt(&len, 11, (char *)"TEST CARRAY", BFLD_CARRAY, 11)), NULL);
    assert_equal(Berror, BTYPERR);
    assert_equal((p=Btypcvt(&len, BFLD_CARRAY, (char *)"TEST CARRAY", 12, 11)), NULL);
    assert_equal(Berror, BTYPERR);
    assert_equal((p=Btypcvt(&len, BFLD_CARRAY, NULL, 12, 11)), NULL);
    assert_equal(Berror, BEINVAL);
}

/**
 * Test data structures by it self
 */
Ensure(ubf_test_struct)
{
    /* test data structures */
    assert_equal(sizeof(UBF_header_t), 48);
}

/**
 * Common suite entry
 * @return
 */
TestSuite *ubf_genbuf_tests(void)
{
    TestSuite *suite = create_test_suite();
/*
    setup_(suite, basic_setup1);
    teardown_(suite, basic_teardown1);
*/
    add_test(suite, test_Bcpy);
    add_test(suite, test_btypcvt);
    add_test(suite, ubf_test_struct);

    return suite;
}
