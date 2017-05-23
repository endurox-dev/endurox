/* 
**
** @file test_cbget.c
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
 * Original field is BFLD_SHORT => T_SHORT_FLD
 */
Ensure(test_CBget_short_org)
{
    char buf[640];
    UBFH *p_ub = (UBFH *)buf;
    short test_val=4564;
    int len = 0;
    short small_val=44;
    /* test buffers */
    short test_short = 0;
    long test_long = 0;
    char test_char = 0;
    float test_float = 0.0;
    double test_double = 0.0;
    char test_string[128];
    char test_carray[128];

    /* init */
    assert_equal(Binit(p_ub, sizeof(buf)), SUCCEED);
    len = sizeof(test_val);
    assert_equal(Badd(p_ub, T_SHORT_FLD, (char *)&test_val, len), SUCCEED);

    /* Now test all types */
    /* short out */
    len=sizeof(test_short);
    assert_equal(CBget(p_ub, T_SHORT_FLD, 0, (char *)&test_short, &len, BFLD_SHORT), SUCCEED);
    assert_equal(test_short, 4564);
    assert_equal(len, sizeof(test_short));
    /* long out */
    len=sizeof(test_long);
    assert_equal(CBget(p_ub, T_SHORT_FLD, 0, (char *)&test_long, &len, BFLD_LONG), SUCCEED);
    assert_equal(test_long, 4564);
    assert_equal(len, sizeof(test_long));
    /* char out */
    assert_equal(Bchg(p_ub, T_SHORT_FLD, 0, (char *)&small_val, 0), SUCCEED);
    assert_equal(CBget(p_ub, T_SHORT_FLD, 0, (char *)&test_char, 0, BFLD_CHAR), SUCCEED);
    assert_equal(test_char, ',');
    assert_equal(Bchg(p_ub, T_SHORT_FLD, 0, (char *)&test_val, 0), SUCCEED); /* restore back */
    /* test float */
    len=sizeof(test_float);
    assert_equal(CBget(p_ub, T_SHORT_FLD, 0, (char *)&test_float, &len, BFLD_FLOAT), SUCCEED);
    assert_equal(test_float, 4564);
    assert_equal(len, sizeof(test_float));
    /* test double */
    assert_equal(CBget(p_ub, T_SHORT_FLD, 0, (char *)&test_double, 0, BFLD_DOUBLE), SUCCEED);
    assert_equal(test_float, 4564);
    /* test string */
    len=sizeof(test_string);
    assert_equal(CBget(p_ub, T_SHORT_FLD, 0, (char *)test_string, &len, BFLD_STRING), SUCCEED);
    assert_string_equal(test_string, "4564");
    assert_equal(len, 5);
    /* test carray */
    assert_equal(CBget(p_ub, T_SHORT_FLD, 0, (char *)test_carray, 0, BFLD_CARRAY), SUCCEED);
    assert_equal(strncmp(test_carray, "4564", 4), 0);
}

/**
 * Original field is BFLD_LONG => T_LONG_FLD
 */
Ensure(test_CBget_long_org)
{
    char buf[640];
    UBFH *p_ub = (UBFH *)buf;
    long test_val=456412;
    int len = 0;
    long small_val=44;
    /* test buffers */
    short test_short = 0;
    long test_long = 0;
    char test_char = 0;
    float test_float = 0.0;
    double test_double = 0.0;
    char test_string[128];
    char test_carray[128];

    /* init */
    assert_equal(Binit(p_ub, sizeof(buf)), SUCCEED);
    len = sizeof(test_val);
    assert_equal(Badd(p_ub, T_LONG_FLD, (char *)&small_val, len), SUCCEED);

    /* Now test all types */
    /* short out */
    len=sizeof(test_short);
    assert_equal(CBget(p_ub, T_LONG_FLD, 0, (char *)&test_short, &len, BFLD_SHORT), SUCCEED);
    assert_equal(test_short, 44);
    assert_equal(len, sizeof(test_short));
    /* char out */
    len=sizeof(test_char);
    assert_equal(CBget(p_ub, T_LONG_FLD, 0, (char *)&test_char, &len, BFLD_CHAR), SUCCEED);
    assert_equal(test_char, ',');
    assert_equal(Bchg(p_ub, T_LONG_FLD, 0, (char *)&test_val, 0), SUCCEED); /* restore back */
    assert_equal(len, sizeof(test_char));

    /* long out */
    len=sizeof(test_long);
    assert_equal(CBget(p_ub, T_LONG_FLD, 0, (char *)&test_long, &len, BFLD_LONG), SUCCEED);
    assert_equal(test_long, 456412);
    assert_equal(len, sizeof(test_long));
    /* test float */
    len=sizeof(test_float);
    assert_equal(CBget(p_ub, T_LONG_FLD, 0, (char *)&test_float, &len, BFLD_FLOAT), SUCCEED);
    assert_equal(test_float, 456412);
    assert_equal(len, sizeof(test_float));
    /* test double */
    len=sizeof(test_double);
    assert_equal(CBget(p_ub, T_LONG_FLD, 0, (char *)&test_double, &len, BFLD_DOUBLE), SUCCEED);
    assert_equal(test_float, 456412);
    assert_equal(len, sizeof(test_double));
    /* test string */
    len=sizeof(test_string);
    assert_equal(CBget(p_ub, T_LONG_FLD, 0, (char *)test_string, &len, BFLD_STRING), SUCCEED);
    assert_string_equal(test_string, "456412");
    assert_equal(len, 7);
    /* test carray */
    len=sizeof(test_string);
    assert_equal(CBget(p_ub, T_LONG_FLD, 0, (char *)test_carray, &len, BFLD_CARRAY), SUCCEED);
    assert_equal(strncmp(test_carray, "456412", 6), 0);
    assert_equal(len, 6);
}

/**
 * Original field is BFLD_CHAR => T_CHAR_FLD
 */
Ensure(test_CBget_char_org)
{
    char buf[640];
    UBFH *p_ub = (UBFH *)buf;
    long test_val=456412;
    int len = 0;
    long small_val=0;
    /* test buffers */
    short test_short = 0;
    long test_long = 0;
    char test_char = 'r';
    float test_float = 0.0;
    double test_double = 0.0;
    char test_string[128];
    char test_carray[128];

    /* init */
    assert_equal(Binit(p_ub, sizeof(buf)), SUCCEED);
    len = sizeof(test_val);
    assert_equal(Badd(p_ub, T_CHAR_FLD, (char *)&test_char, len), SUCCEED);

    /* Now test all types */
    /* short out */
    len=sizeof(test_short);
    assert_equal(CBget(p_ub, T_CHAR_FLD, 0, (char *)&test_short, &len, BFLD_SHORT), SUCCEED);
    assert_equal(test_short, 114);
    assert_equal(len, sizeof(test_short));

    /* long out */
    len=sizeof(test_long);
    assert_equal(CBget(p_ub, T_CHAR_FLD, 0, (char *)&test_long, &len, BFLD_LONG), SUCCEED);
    assert_equal(test_long, 114);
    assert_equal(len, sizeof(test_long));

    /* char out */
    len=sizeof(test_char);
    assert_equal(CBget(p_ub, T_CHAR_FLD, 0, (char *)&test_char, &len, BFLD_CHAR), SUCCEED);
    assert_equal(test_char, 'r');
    assert_equal(len, sizeof(test_char));

    /* test float */
    len=sizeof(test_float);
    assert_equal(CBget(p_ub, T_CHAR_FLD, 0, (char *)&test_float, &len, BFLD_FLOAT), SUCCEED);
    assert_equal(test_float, 114);
    assert_equal(len, sizeof(test_float));

    /* test double */
    len=sizeof(test_double);
    assert_equal(CBget(p_ub, T_CHAR_FLD, 0, (char *)&test_double, &len, BFLD_DOUBLE), SUCCEED);
    assert_equal(test_float, 114);
    assert_equal(len, sizeof(test_double));

    /* test string */
    len=sizeof(test_string);
    assert_equal(CBget(p_ub, T_CHAR_FLD, 0, (char *)test_string, &len, BFLD_STRING), SUCCEED);
    assert_string_equal(test_string, "r");
    assert_equal(len, 2);

    /* test carray */
    len=sizeof(test_string);
    assert_equal(CBget(p_ub, T_CHAR_FLD, 0, (char *)test_carray, &len, BFLD_CARRAY), SUCCEED);
    assert_equal(strncmp(test_carray, "r", 1), 0);
    assert_equal(len, 1);   
}

/**
 * Original field is BFLD_FLOAT => T_FLOAT_FLD
 */
Ensure(test_CBget_float_org)
{
    char buf[640];
    UBFH *p_ub = (UBFH *)buf;
    long test_val=0;
    int len = 0;
    /* test buffers */
    short test_short = 0;
    long test_long = 0;
    unsigned char test_char = 0;
    float test_float = 16822.5;
    double test_double = 0.0;
    char test_string[128];
    char test_carray[128];

    /* init */
    assert_equal(Binit(p_ub, sizeof(buf)), SUCCEED);
    len = sizeof(test_float);
    assert_equal(Badd(p_ub, T_FLOAT_FLD, (char *)&test_float, len), SUCCEED);

    /* Now test all types */
    /* short out */
    len=sizeof(test_short);
    assert_equal(CBget(p_ub, T_FLOAT_FLD, 0, (char *)&test_short, &len, BFLD_SHORT), SUCCEED);
    assert_equal(test_short, 16822);
    assert_equal(len, sizeof(test_short));

    /* long out */
    len=sizeof(test_long);
    assert_equal(CBget(p_ub, T_FLOAT_FLD, 0, (char *)&test_long, &len, BFLD_LONG), SUCCEED);
    assert_equal(test_long, 16822);
    assert_equal(len, sizeof(test_long));

    /* char out */

    len = sizeof(test_float);
    test_float = 168.5;
    assert_equal(Bchg(p_ub, T_FLOAT_FLD, 0, (char *)&test_float, len), SUCCEED);
    len=sizeof(test_char);
    assert_equal(CBget(p_ub, T_FLOAT_FLD, 0, (char *)&test_char, &len, BFLD_CHAR), SUCCEED);
    assert_equal(test_char, 168);
    assert_equal(len, sizeof(test_char));

    /* test float */
    /* continue as normal */
    len = sizeof(test_float);
    test_float = 16822.5;
    assert_equal(Bchg(p_ub, T_FLOAT_FLD, 0, (char *)&test_float, len), SUCCEED);

    len=sizeof(test_float);
    assert_equal(CBget(p_ub, T_FLOAT_FLD, 0, (char *)&test_float, &len, BFLD_FLOAT), SUCCEED);
    assert_equal(test_float, 16822.5);
    assert_equal(len, sizeof(test_float));

    /* test double */
    len=sizeof(test_double);
    assert_equal(CBget(p_ub, T_FLOAT_FLD, 0, (char *)&test_double, &len, BFLD_DOUBLE), SUCCEED);
    assert_equal(test_float, 16822.5);
    assert_equal(len, sizeof(test_double));

    /* test string */
    len=sizeof(test_string);
    assert_equal(CBget(p_ub, T_FLOAT_FLD, 0, (char *)test_string, &len, BFLD_STRING), SUCCEED);
    assert_string_equal(test_string, "16822.50000");
    assert_equal(len, 12);

    /* test carray */
    len=sizeof(test_string);
    assert_equal(CBget(p_ub, T_FLOAT_FLD, 0, (char *)test_carray, &len, BFLD_CARRAY), SUCCEED);
    assert_equal(strncmp(test_carray, "16822.50000", 10), 0);
    assert_equal(len, 11);
}

/**
 * Original field is BFLD_DOUBLE => T_DOUBLE_FLD
 */
Ensure(test_CBget_double_org)
{
    char buf[640];
    UBFH *p_ub = (UBFH *)buf;
    int len = 0;
    /* test buffers */
    short test_short = 0;
    long test_long = 0;
    unsigned char test_char = 0;
    float test_float = 0.0;
    double test_double = 16822.5;
    char test_string[128];
    char test_carray[128];

    /* init */
    assert_equal(Binit(p_ub, sizeof(buf)), SUCCEED);
    len = sizeof(test_double);
    assert_equal(Badd(p_ub, T_DOUBLE_FLD, (char *)&test_double, len), SUCCEED);

    /* Now test all types */
    /* short out */
    len=sizeof(test_short);
    assert_equal(CBget(p_ub, T_DOUBLE_FLD, 0, (char *)&test_short, &len, BFLD_SHORT), SUCCEED);
    assert_equal(test_short, 16822);
    assert_equal(len, sizeof(test_short));

    /* long out */
    len=sizeof(test_long);
    assert_equal(CBget(p_ub, T_DOUBLE_FLD, 0, (char *)&test_long, &len, BFLD_LONG), SUCCEED);
    assert_equal(test_long, 16822);
    assert_equal(len, sizeof(test_long));

    /* char out */
    len = sizeof(test_double);
    test_double = 168.5;
    assert_equal(Bchg(p_ub, T_DOUBLE_FLD, 0, (char *)&test_double, len), SUCCEED);
    
    len=sizeof(test_char);
    assert_equal(CBget(p_ub, T_DOUBLE_FLD, 0, (char *)&test_char, &len, BFLD_CHAR), SUCCEED);
    assert_equal(test_char, 168);
    assert_equal(len, sizeof(test_char));

    /* test float */
    /* continue as normal */
    test_double = 16822.5;
    len = sizeof(test_double);
    assert_equal(Bchg(p_ub, T_DOUBLE_FLD, 0, (char *)&test_double, len), SUCCEED);

    len=sizeof(test_float);
    assert_equal(CBget(p_ub, T_DOUBLE_FLD, 0, (char *)&test_float, &len, BFLD_FLOAT), SUCCEED);
    assert_equal(test_float, 16822.5);
    assert_equal(len, sizeof(test_float));

    /* test double */
    len=sizeof(test_double);
    assert_equal(CBget(p_ub, T_DOUBLE_FLD, 0, (char *)&test_double, &len, BFLD_DOUBLE), SUCCEED);
    assert_equal(test_float, 16822.5);
    assert_equal(len, sizeof(test_double));

    /* test string */
    len=sizeof(test_string);
    assert_equal(CBget(p_ub, T_DOUBLE_FLD, 0, (char *)test_string, &len, BFLD_STRING), SUCCEED);
    assert_string_equal(test_string, "16822.500000");
    assert_equal(len, 13);

    /* test carray */
    len=sizeof(test_string);
    assert_equal(CBget(p_ub, T_DOUBLE_FLD, 0, (char *)test_carray, &len, BFLD_CARRAY), SUCCEED);
    assert_equal(strncmp(test_carray, "16822.500000", 10), 0);
    assert_equal(len, 12);
}
/**
 * Original field is BFLD_STRING => T_STRING_FLD
 */
Ensure(test_CBget_string_org)
{
    char buf[2048];
    UBFH *p_ub = (UBFH *)buf;
    int len = 0;
    /* test buffers */
    short test_short = 0;
    long test_long = 0;
    unsigned char test_char = 0;
    float test_float = 0.0;
    double test_double = 16822.5;
    char test_string[1024];
    char test_carray[1024];

    /* init */
    assert_equal(Binit(p_ub, sizeof(buf)), SUCCEED);
    /*  len 1 - should be ok, because it is ignored!*/
    assert_equal(Badd(p_ub, T_STRING_FLD, "16822.5000000000000", 1), SUCCEED);

    /* Now test all types */
    /* short out */
    len=sizeof(test_short);
    assert_equal(CBget(p_ub, T_STRING_FLD, 0, (char *)&test_short, &len, BFLD_SHORT), SUCCEED);
    assert_equal(test_short, 16822);
    assert_equal(len, sizeof(test_short));

    /* long out */
    len=sizeof(test_long);
    assert_equal(CBget(p_ub, T_STRING_FLD, 0, (char *)&test_long, &len, BFLD_LONG), SUCCEED);
    assert_equal(test_long, 16822);
    assert_equal(len, sizeof(test_long));

    /* char out */
    len=sizeof(test_char);
    assert_equal(CBget(p_ub, T_STRING_FLD, 0, (char *)&test_char, &len, BFLD_CHAR), SUCCEED);
    assert_equal(test_char, '1');
    assert_equal(len, sizeof(test_char));

    /* test float */
    len=sizeof(test_float);
    assert_equal(CBget(p_ub, T_STRING_FLD, 0, (char *)&test_float, &len, BFLD_FLOAT), SUCCEED);
    assert_equal(test_float, 16822.5);
    assert_equal(len, sizeof(test_float));

    /* test double */
    len=sizeof(test_double);
    assert_equal(CBget(p_ub, T_STRING_FLD, 0, (char *)&test_double, &len, BFLD_DOUBLE), SUCCEED);
    assert_equal(test_float, 16822.5);
    assert_equal(len, sizeof(test_double));

    /* test string */
    len=sizeof(test_string);
    assert_equal(CBget(p_ub, T_STRING_FLD, 0, (char *)test_string, &len, BFLD_STRING), SUCCEED);
    assert_string_equal(test_string, "16822.5000000000000");
    assert_equal(len, 20);

    /* test carray */
    len=sizeof(test_string);
    assert_equal(CBget(p_ub, T_STRING_FLD, 0, (char *)test_carray, &len, BFLD_CARRAY), SUCCEED);
    assert_equal(strncmp(test_carray, "16822.5000000000000", 17), 0);
    assert_equal(len, 19);
    
    /* Special test for hadling strings & carrays */
    assert_equal(Bchg(p_ub, T_STRING_FLD, 0, BIG_TEST_STRING, 0), SUCCEED);
    len=sizeof(test_carray);
    assert_equal(CBget(p_ub, T_STRING_FLD, 0, (char *)test_carray, &len, BFLD_CARRAY), SUCCEED);
    assert_equal(strncmp(test_carray, BIG_TEST_STRING, strlen(BIG_TEST_STRING)), 0);
    assert_equal(len, strlen(BIG_TEST_STRING));
    
}

/**
 * Original field is BFLD_CARRAY => T_CARRAY_FLD
 */
Ensure(test_CBget_carray_org)
{
    char buf[2048];
    UBFH *p_ub = (UBFH *)buf;
    int len = 0;
    /* test buffers */
    short test_short = 0;
    long test_long = 0;
    unsigned char test_char = 0;
    float test_float = 0.0;
    double test_double = 16822.5;
    char test_string[1024];
    char test_carray[1024];
    char test_data_str [] = "16822.5000000000000";
    /* init */
    assert_equal(Binit(p_ub, sizeof(buf)), SUCCEED);
    len = strlen(test_data_str);
    assert_equal(Badd(p_ub, T_CARRAY_FLD, test_data_str, len), SUCCEED);

    /* Now test all types */
    /* short out */
    len=sizeof(test_short);
    assert_equal(CBget(p_ub, T_CARRAY_FLD, 0, (char *)&test_short, &len, BFLD_SHORT), SUCCEED);
    assert_equal(test_short, 16822);
    assert_equal(len, sizeof(test_short));

    /* long out */
    len=sizeof(test_long);
    assert_equal(CBget(p_ub, T_CARRAY_FLD, 0, (char *)&test_long, &len, BFLD_LONG), SUCCEED);
    assert_equal(test_long, 16822);
    assert_equal(len, sizeof(test_long));

    /* char out */
    len=sizeof(test_char);
    assert_equal(CBget(p_ub, T_CARRAY_FLD, 0, (char *)&test_char, &len, BFLD_CHAR), SUCCEED);
    assert_equal(test_char, '1');
    assert_equal(len, sizeof(test_char));

    /* test float */
    len=sizeof(test_float);
    assert_equal(CBget(p_ub, T_CARRAY_FLD, 0, (char *)&test_float, &len, BFLD_FLOAT), SUCCEED);
    assert_equal(test_float, 16822.5);
    assert_equal(len, sizeof(test_float));

    /* test double */
    len=sizeof(test_double);
    assert_equal(CBget(p_ub, T_CARRAY_FLD, 0, (char *)&test_double, &len, BFLD_DOUBLE), SUCCEED);
    assert_equal(test_float, 16822.5);
    assert_equal(len, sizeof(test_double));

    /* test string */
    len=sizeof(test_string);
    assert_equal(CBget(p_ub, T_CARRAY_FLD, 0, (char *)test_string, &len, BFLD_STRING), SUCCEED);
    assert_string_equal(test_string, test_data_str);
    assert_equal(len, 20);

    /* test carray */
    len=sizeof(test_string);
    assert_equal(CBget(p_ub, T_CARRAY_FLD, 0, (char *)test_carray, &len, BFLD_CARRAY), SUCCEED);
    assert_equal(strncmp(test_carray, test_data_str, 17), 0);
    assert_equal(len, 19);

    /* Special test for hadling strings & carrays */
    len = strlen(BIG_TEST_STRING);
    assert_equal(Bchg(p_ub, T_CARRAY_FLD, 0, BIG_TEST_STRING, len), SUCCEED);
    len=sizeof(test_string);
    memset(test_string, 0xff, sizeof(test_string)); /* For checking EOS terminator */
    assert_equal(CBget(p_ub, T_CARRAY_FLD, 0, (char *)test_string, &len, BFLD_STRING), SUCCEED);
    assert_equal(strncmp(test_string, BIG_TEST_STRING, strlen(BIG_TEST_STRING)), 0);
    assert_equal(len, strlen(BIG_TEST_STRING)+1);
    assert_equal(strlen(test_string), strlen(BIG_TEST_STRING));
}

TestSuite *ubf_cfget_tests(void)
{
    TestSuite *suite = create_test_suite();
    
    add_test(suite, test_CBget_short_org);
    add_test(suite, test_CBget_long_org);
    add_test(suite, test_CBget_char_org);
    add_test(suite, test_CBget_float_org);
    add_test(suite, test_CBget_double_org);
    add_test(suite, test_CBget_string_org);
    add_test(suite, test_CBget_carray_org);
    
    /* string & carray */

    return suite;
}

