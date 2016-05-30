/* 
**
** @file test_cbchg.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, ATR Baltic, SIA. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or ATR Baltic's license for commercial use.
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
** A commercial use license is available from ATR Baltic, SIA
** contact@atrbaltic.com
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
Ensure(test_Bchg_short_org)
{
    char buf[640];
    UBFH *p_ub = (UBFH *)buf;
    short test_val;
    short short_val = 123;
    long long_val = 102;
    char char_val = '7';
    float float_val=11;
    double double_val=1443;
    char string[129]="31255";
    char carray[128]="26411";

    /* init */
    assert_equal(Binit(p_ub, sizeof(buf)), SUCCEED);
    set_up_dummy_data(p_ub);

    /* short-to-short */
    assert_equal(CBchg(p_ub, T_SHORT_FLD, 0, (char *)&short_val, 0, BFLD_SHORT), SUCCEED);
    assert_equal(Bget(p_ub, T_SHORT_FLD, 0, (char *)&test_val, 0), SUCCEED);
    assert_equal(test_val, 123);
    do_dummy_data_test(p_ub);
    /* long-to-short */
    assert_equal(CBchg(p_ub, T_SHORT_FLD, 0, (char *)&long_val, 0, BFLD_LONG), SUCCEED);
    assert_equal(Bget(p_ub, T_SHORT_FLD, 0, (char *)&test_val, 0), SUCCEED);
    assert_equal(test_val, 102);
    do_dummy_data_test(p_ub);
    /* char-to-short */
    assert_equal(CBchg(p_ub, T_SHORT_FLD, 0, (char *)&char_val, 0, BFLD_CHAR), SUCCEED);
    assert_equal(Bget(p_ub, T_SHORT_FLD, 0, (char *)&test_val, 0), SUCCEED);
    assert_equal(test_val, 55); /* 55 - ascii code for '7' */
    do_dummy_data_test(p_ub);
    /* float-to-short */
    assert_equal(CBchg(p_ub, T_SHORT_FLD, 0, (char *)&float_val, 0, BFLD_FLOAT), SUCCEED);
    assert_equal(Bget(p_ub, T_SHORT_FLD, 0, (char *)&test_val, 0), SUCCEED);
    assert_equal(test_val, 11);
    do_dummy_data_test(p_ub);
    /* double-to-short */
    assert_equal(CBchg(p_ub, T_SHORT_FLD, 0, (char *)&double_val, 0, BFLD_DOUBLE), SUCCEED);
    assert_equal(Bget(p_ub, T_SHORT_FLD, 0, (char *)&test_val, 0), SUCCEED);
    assert_equal(test_val, 1443);
    do_dummy_data_test(p_ub);
    /* string-to-short */
    assert_equal(CBchg(p_ub, T_SHORT_FLD, 0, (char *)string, 0, BFLD_STRING), SUCCEED);
    assert_equal(Bget(p_ub, T_SHORT_FLD, 0, (char *)&test_val, 0), SUCCEED);
    assert_equal(test_val, 31255);
    do_dummy_data_test(p_ub);
    /* carray-to-short */
    assert_equal(CBchg(p_ub, T_SHORT_FLD, 0, (char *)carray, strlen(carray), BFLD_CARRAY), SUCCEED);
    assert_equal(Bget(p_ub, T_SHORT_FLD, 0, (char *)&test_val, 0), SUCCEED);
    assert_equal(test_val, 26411);
    do_dummy_data_test(p_ub);
}

/**
 * Original field is BFLD_LONG => T_LONG_FLD
 */
Ensure(test_Bchg_long_org)
{
    char buf[640];
    UBFH *p_ub = (UBFH *)buf;
    long test_val;
    short short_val = 123;
    long long_val = 102;
    char char_val = '7';
    float float_val=11123341;
    double double_val=14431234;
    char string[129]="3125511";
    char carray[128]="2641133";

    /* init */
    assert_equal(Binit(p_ub, sizeof(buf)), SUCCEED);
    set_up_dummy_data(p_ub);

    /* short-to-long */
    assert_equal(CBchg(p_ub, T_LONG_FLD, 0, (char *)&short_val, 0, BFLD_SHORT), SUCCEED);
    assert_equal(Bget(p_ub, T_LONG_FLD, 0, (char *)&test_val, 0), SUCCEED);
    assert_equal(test_val, 123);
    do_dummy_data_test(p_ub);
    /* long-to-long */
    assert_equal(CBchg(p_ub, T_LONG_FLD, 0, (char *)&long_val, 0, BFLD_LONG), SUCCEED);
    assert_equal(Bget(p_ub, T_LONG_FLD, 0, (char *)&test_val, 0), SUCCEED);
    assert_equal(test_val, 102);
    do_dummy_data_test(p_ub);
    /* char-to-long */
    assert_equal(CBchg(p_ub, T_LONG_FLD, 0, (char *)&char_val, 0, BFLD_CHAR), SUCCEED);
    assert_equal(Bget(p_ub, T_LONG_FLD, 0, (char *)&test_val, 0), SUCCEED);
    assert_equal(test_val, 55); /* 55 - ascii code for '7' */
    do_dummy_data_test(p_ub);
    /* float-to-long */
    assert_equal(CBchg(p_ub, T_LONG_FLD, 0, (char *)&float_val, 0, BFLD_FLOAT), SUCCEED);
    assert_equal(Bget(p_ub, T_LONG_FLD, 0, (char *)&test_val, 0), SUCCEED);
    assert_equal(test_val, 11123341);
    do_dummy_data_test(p_ub);
    /* double-to-long */
    assert_equal(CBchg(p_ub, T_LONG_FLD, 0, (char *)&double_val, 0, BFLD_DOUBLE), SUCCEED);
    assert_equal(Bget(p_ub, T_LONG_FLD, 0, (char *)&test_val, 0), SUCCEED);
    assert_equal(test_val, 14431234);
    do_dummy_data_test(p_ub);
    /* string-to-long */
    assert_equal(CBchg(p_ub, T_LONG_FLD, 0, (char *)string, 0, BFLD_STRING), SUCCEED);
    assert_equal(Bget(p_ub, T_LONG_FLD, 0, (char *)&test_val, 0), SUCCEED);
    assert_equal(test_val, 3125511);
    do_dummy_data_test(p_ub);
    /* carray-to-long */
    assert_equal(CBchg(p_ub, T_LONG_FLD, 0, (char *)carray, strlen(carray), BFLD_CARRAY), SUCCEED);
    assert_equal(Bget(p_ub, T_LONG_FLD, 0, (char *)&test_val, 0), SUCCEED);
    assert_equal(test_val, 2641133);
    do_dummy_data_test(p_ub);
}

/**
 * Original field is BFLD_CHAR => T_CHAR_FLD
 */
Ensure(test_Bchg_char_org)
{
    char buf[640];
    UBFH *p_ub = (UBFH *)buf;
    char test_val;
    short short_val = 1;
    long long_val = 2;
    char char_val = '7';
    float float_val=3;
    double double_val=4;
    char string[129]="5";
    char carray[128]="6";

    /* init */
    assert_equal(Binit(p_ub, sizeof(buf)), SUCCEED);
    set_up_dummy_data(p_ub);

    /* short-to-char */
    assert_equal(CBchg(p_ub, T_CHAR_FLD, 0, (char *)&short_val, 0, BFLD_SHORT), SUCCEED);
    assert_equal(Bget(p_ub, T_CHAR_FLD, 0, (char *)&test_val, 0), SUCCEED);
    assert_equal(test_val, 1);
    do_dummy_data_test(p_ub);
    /* long-to-char */
    assert_equal(CBchg(p_ub, T_CHAR_FLD, 0, (char *)&long_val, 0, BFLD_LONG), SUCCEED);
    assert_equal(Bget(p_ub, T_CHAR_FLD, 0, (char *)&test_val, 0), SUCCEED);
    assert_equal(test_val, 2);
    do_dummy_data_test(p_ub);
    /* char-to-long */
    assert_equal(CBchg(p_ub, T_CHAR_FLD, 0, (char *)&char_val, 0, BFLD_CHAR), SUCCEED);
    assert_equal(Bget(p_ub, T_CHAR_FLD, 0, (char *)&test_val, 0), SUCCEED);
    assert_equal(test_val, 55); /* 55 - ascii code for '7' */
    do_dummy_data_test(p_ub);
    /* float-to-char */
    assert_equal(CBchg(p_ub, T_CHAR_FLD, 0, (char *)&float_val, 0, BFLD_FLOAT), SUCCEED);
    assert_equal(Bget(p_ub, T_CHAR_FLD, 0, (char *)&test_val, 0), SUCCEED);
    assert_equal(test_val, 3);
    do_dummy_data_test(p_ub);
    /* double-to-char */
    assert_equal(CBchg(p_ub, T_CHAR_FLD, 0, (char *)&double_val, 0, BFLD_DOUBLE), SUCCEED);
    assert_equal(Bget(p_ub, T_CHAR_FLD, 0, (char *)&test_val, 0), SUCCEED);
    assert_equal(test_val, 4);
    do_dummy_data_test(p_ub);
    /* string-to-char */
    assert_equal(CBchg(p_ub, T_CHAR_FLD, 0, (char *)string, 0, BFLD_STRING), SUCCEED);
    assert_equal(Bget(p_ub, T_CHAR_FLD, 0, (char *)&test_val, 0), SUCCEED);
    assert_equal(test_val, 53);
    do_dummy_data_test(p_ub);
    /* carray-to-char */
    assert_equal(CBchg(p_ub, T_CHAR_FLD, 0, (char *)carray, strlen(carray), BFLD_CARRAY), SUCCEED);
    assert_equal(Bget(p_ub, T_CHAR_FLD, 0, (char *)&test_val, 0), SUCCEED);
    assert_equal(test_val, 54);
    do_dummy_data_test(p_ub);
}

/**
 * Original field is BFLD_FLOAT => T_FLOAT_FLD
 */
Ensure(test_Bchg_float_org)
{
    char buf[640];
    UBFH *p_ub = (UBFH *)buf;
    float test_val;
    short short_val = 12321;
    long long_val = 10223112;
    char char_val = '7';
    float float_val=111233.41;
    double double_val=144.31234;
    char string[129]="3125.511";
    char carray[128]="-264.1133";
    BFLDLEN len; /* Testing len only for few, because thos other should work fine w/out them */

    /* init */
    assert_equal(Binit(p_ub, sizeof(buf)), SUCCEED);
    set_up_dummy_data(p_ub);

    /* short-to-float */
    len=sizeof(test_val);
    assert_equal(CBchg(p_ub, T_FLOAT_FLD, 0, (char *)&short_val, 0, BFLD_SHORT), SUCCEED);
    assert_equal(Bget(p_ub, T_FLOAT_FLD, 0, (char *)&test_val, 0), SUCCEED);
    assert_double_equal(test_val, 12321);
    do_dummy_data_test(p_ub);
    assert_equal(sizeof(test_val), len);
    /* long-to-float */
    assert_equal(CBchg(p_ub, T_FLOAT_FLD, 0, (char *)&long_val, 0, BFLD_LONG), SUCCEED);
    assert_equal(Bget(p_ub, T_FLOAT_FLD, 0, (char *)&test_val, 0), SUCCEED);
    assert_double_equal(test_val, 10223112);
    do_dummy_data_test(p_ub);
    /* char-to-float */
    len=sizeof(test_val);
    assert_equal(CBchg(p_ub, T_FLOAT_FLD, 0, (char *)&char_val, 0, BFLD_CHAR), SUCCEED);
    assert_equal(Bget(p_ub, T_FLOAT_FLD, 0, (char *)&test_val, &len), SUCCEED);
    assert_double_equal(test_val, 55); /* 55 - ascii code for '7' */
    do_dummy_data_test(p_ub);
    assert_equal(len, sizeof(test_val));
    len=sizeof(test_val)-1;
    /* validate the error output! */
    assert_equal(Bget(p_ub, T_FLOAT_FLD, 0, (char *)&test_val, &len), FAIL);
    assert_equal(Berror, BNOSPACE);
    /* Check the case when buffer is too short! */
    /* float-to-float */
    assert_equal(CBchg(p_ub, T_FLOAT_FLD, 0, (char *)&float_val, 0, BFLD_FLOAT), SUCCEED);
    assert_equal(Bget(p_ub, T_FLOAT_FLD, 0, (char *)&test_val, 0), SUCCEED);
    assert_double_equal(test_val, 111233.41);
    do_dummy_data_test(p_ub);
    /* double-to-float */
    len=sizeof(test_val);
    assert_equal(CBchg(p_ub, T_FLOAT_FLD, 0, (char *)&double_val, 0, BFLD_DOUBLE), SUCCEED);
    assert_equal(Bget(p_ub, T_FLOAT_FLD, 0, (char *)&test_val, &len), SUCCEED);
    assert_double_equal(test_val, 144.31234);
    do_dummy_data_test(p_ub);
    assert_equal(len, sizeof(test_val));
    /* string-to-float */
    assert_equal(CBchg(p_ub, T_FLOAT_FLD, 0, (char *)string, 0, BFLD_STRING), SUCCEED);
    assert_equal(Bget(p_ub, T_FLOAT_FLD, 0, (char *)&test_val, 0), SUCCEED);
    assert_double_equal(test_val, 3125.511);
    do_dummy_data_test(p_ub);
    /* carray-to-float */
    len=sizeof(test_val);
    assert_equal(CBchg(p_ub, T_FLOAT_FLD, 0, (char *)carray, strlen(carray), BFLD_CARRAY), SUCCEED);
    assert_equal(Bget(p_ub, T_FLOAT_FLD, 0, (char *)&test_val, &len), SUCCEED);
    assert_double_equal(test_val, -264.1133f);
    do_dummy_data_test(p_ub);
    assert_equal(len, sizeof(test_val));
}

/**
 * Original field is BFLD_DOUBLE => T_DOUBLE_FLD
 */
Ensure(test_Bchg_double_org)
{
    char buf[640];
    UBFH *p_ub = (UBFH *)buf;
    double test_val;
    short short_val = 22321;
    long long_val = 20223112;
    char char_val = '4';
    float float_val=211233.41;
    double double_val=244.31234;
    char string[129]="2125.511";
    char carray[128]="-2264.1133";
    BFLDLEN len; /* Testing len only for few, because thos other should work fine w/out them */
    
    /* init */
    assert_equal(Binit(p_ub, sizeof(buf)), SUCCEED);
    set_up_dummy_data(p_ub);

    /* short-to-float */
    assert_equal(CBchg(p_ub, T_DOUBLE_FLD, 0, (char *)&short_val, 0, BFLD_SHORT), SUCCEED);
    assert_equal(Bget(p_ub, T_DOUBLE_FLD, 0, (char *)&test_val, 0), SUCCEED);
    assert_double_equal(test_val, 22321);
    do_dummy_data_test(p_ub);
    /* long-to-float */
    len=sizeof(test_val);
    assert_equal(CBchg(p_ub, T_DOUBLE_FLD, 0, (char *)&long_val, 0, BFLD_LONG), SUCCEED);
    assert_equal(Bget(p_ub, T_DOUBLE_FLD, 0, (char *)&test_val, &len), SUCCEED);
    assert_double_equal(test_val, 20223112);
    do_dummy_data_test(p_ub);
    assert_equal(sizeof(test_val), len);

    /* char-to-float */
    assert_equal(CBchg(p_ub, T_DOUBLE_FLD, 0, (char *)&char_val, 0, BFLD_CHAR), SUCCEED);
    assert_equal(Bget(p_ub, T_DOUBLE_FLD, 0, (char *)&test_val, 0), SUCCEED);
    assert_double_equal(test_val, 52); /* 55 - ascii code for '7' */
    do_dummy_data_test(p_ub);
    /* float-to-float */
    len=sizeof(test_val);
    assert_equal(CBchg(p_ub, T_DOUBLE_FLD, 0, (char *)&float_val, 0, BFLD_FLOAT), SUCCEED);
    assert_equal(Bget(p_ub, T_DOUBLE_FLD, 0, (char *)&test_val, &len), SUCCEED);
    assert_double_equal(test_val, 211233.41);
    do_dummy_data_test(p_ub);
    assert_equal(sizeof(test_val), len);
    /* double-to-float */
    assert_equal(CBchg(p_ub, T_DOUBLE_FLD, 0, (char *)&double_val, 0, BFLD_DOUBLE), SUCCEED);
    assert_equal(Bget(p_ub, T_DOUBLE_FLD, 0, (char *)&test_val, 0), SUCCEED);
    assert_double_equal(test_val, 244.31234);
    do_dummy_data_test(p_ub);
    /* string-to-float */
    len=sizeof(test_val);
    assert_equal(CBchg(p_ub, T_DOUBLE_FLD, 0, (char *)string, 0, BFLD_STRING), SUCCEED);
    assert_equal(Bget(p_ub, T_DOUBLE_FLD, 0, (char *)&test_val, &len), SUCCEED);
    assert_double_equal(test_val, 2125.511);
    do_dummy_data_test(p_ub);
    assert_equal(sizeof(test_val), len);

    /* carray-to-float */
    assert_equal(CBchg(p_ub, T_DOUBLE_FLD, 0, (char *)carray, strlen(carray), BFLD_CARRAY), SUCCEED);
    assert_equal(Bget(p_ub, T_DOUBLE_FLD, 0, (char *)&test_val, 0), SUCCEED);
    assert_double_equal(test_val, -2264.1133);
    do_dummy_data_test(p_ub);
}

/**
 * Original field is BFLD_STRING => T_STRING_FLD
 */
Ensure(test_Bchg_string_org)
{
    char buf[1024];
    UBFH *p_ub = (UBFH *)buf;
    char test_val[641];
    short short_val = 22321;
    long long_val = 20223112;
    char char_val = '4';
    float float_val=211233.41;
    double double_val=244.31234;
    char string[514+1]="2125.511";
    char carray[514];
    int i;
    BFLDLEN len;
    /* variables for big buffer test */
    char string_big[1024];
    int str_len_tmp;
    int big_loop;
    
    assert_equal(Binit(p_ub, sizeof(buf)), SUCCEED);
    set_up_dummy_data(p_ub);
    /*
     * Initialize character array to some dummy value
     */
    for (i=0; i<sizeof(carray); i++)
    {
        char c = 48 + (i % 74);
        carray[i] = c;
        string[1] = c-1;
    }
    /* Put trailing EOS */
    string[sizeof(string)-1] = 0;

    len=sizeof(test_val);
    /* short-to-string */

    assert_equal(CBchg(p_ub, T_STRING_FLD, 0, (char *)&short_val, 0, BFLD_SHORT), SUCCEED);
    assert_equal(Bget(p_ub, T_STRING_FLD, 0, (char *)test_val, &len), SUCCEED);
    assert_string_equal(test_val, "22321");
    assert_equal(len, 6);

    do_dummy_data_test(p_ub);
    /* long-to-string */
    assert_equal(CBchg(p_ub, T_STRING_FLD, 0, (char *)&long_val, 0, BFLD_LONG), SUCCEED);
    assert_equal(Bget(p_ub, T_STRING_FLD, 0, (char *)test_val, 0), SUCCEED);
    assert_string_equal(test_val, "20223112");
    do_dummy_data_test(p_ub);
    /* char-to-string */
    len=sizeof(test_val);
    assert_equal(CBchg(p_ub, T_STRING_FLD, 0, (char *)&char_val, 0, BFLD_CHAR), SUCCEED);
    assert_equal(Bget(p_ub, T_STRING_FLD, 0, (char *)test_val, &len), SUCCEED);
    assert_string_equal(test_val, "4"); /* 55 - ascii code for '7' */
    assert_equal(len, 2);
    do_dummy_data_test(p_ub);
    /* float-to-string (test on Tuxedo) */
    assert_equal(CBchg(p_ub, T_STRING_FLD, 0, (char *)&float_val, 0, BFLD_FLOAT), SUCCEED);
    assert_equal(Bget(p_ub, T_STRING_FLD, 0, (char *)test_val, 0), SUCCEED);
    assert_equal(strncmp(test_val, "211233.41", 8),0);
    do_dummy_data_test(p_ub);
    /* double-to-string (test un Tuxedo?) */
    len=sizeof(test_val);
    assert_equal(CBchg(p_ub, T_STRING_FLD, 0, (char *)&double_val, 0, BFLD_DOUBLE), SUCCEED);
    assert_equal(Bget(p_ub, T_STRING_FLD, 0, (char *)test_val, &len), SUCCEED);
    assert_equal(strncmp(test_val, "244.31234",9),0);
    assert_true(len>9);
    do_dummy_data_test(p_ub);
    /* string-to-string */
    assert_equal(CBchg(p_ub, T_STRING_FLD, 0, (char *)string, 0, BFLD_STRING), SUCCEED);
    assert_equal(Bget(p_ub, T_STRING_FLD, 0, (char *)test_val, 0), SUCCEED);
    assert_string_equal(test_val, string);
    do_dummy_data_test(p_ub);
    /* carray-to-string */
    len=sizeof(test_val);
    assert_equal(CBchg(p_ub, T_STRING_FLD, 0, (char *)carray, sizeof(carray), BFLD_CARRAY), SUCCEED);
    assert_equal(Bget(p_ub, T_STRING_FLD, 0, (char *)test_val, &len), SUCCEED);
    assert_equal(strncmp(test_val, carray, sizeof(carray)), 0);
    assert_equal(len, sizeof(carray)+1);
    do_dummy_data_test(p_ub);

    /* Special test for hadling large strings & carrays */
    for (big_loop=0; big_loop<10;big_loop++)
    {
        str_len_tmp=strlen(BIG_TEST_STRING);
        assert_equal(CBchg(p_ub, T_STRING_FLD, 0, (char *)BIG_TEST_STRING, str_len_tmp, BFLD_CARRAY), SUCCEED);
        len = sizeof(string_big);
        assert_equal(Bget(p_ub, T_STRING_FLD, 0, (char *)string_big, &len), SUCCEED);
        assert_equal(len, str_len_tmp+1);
        assert_equal(strncmp(string_big, BIG_TEST_STRING, str_len_tmp), 0);
    }
    
}

/**
 * Original field is BFLD_CARRAY => T_CARRAY_FLD
 */
Ensure(test_Bchg_carray_org)
{
    char buf[1024];
    UBFH *p_ub = (UBFH *)buf;
    char test_val[514];
    short short_val = 22321;
    long long_val = 20223112;
    char char_val = '4';
    float float_val=211233.41;
    double double_val=244.31234;
    char string[514+1]="2125.511";
    char carray[514];
    int i;
    int len;
    /* Big buffer test... */
    char carray_big[1024];
    int str_len_tmp;
    int big_loop;
    
    assert_equal(Binit(p_ub, sizeof(buf)), SUCCEED);
    set_up_dummy_data(p_ub);
    /*
     * Initialize character array to some dummy value
     */
    for (i=0; i<sizeof(carray); i++)
    {
        char c = 48 + (i % 74);
        carray[i] = c;
        string[1] = c-1;
    }
    /* Put trailing EOS */
    string[sizeof(string)-1] = 0;

    /* short-to-carray */
    assert_equal(CBchg(p_ub, T_CARRAY_FLD, 0, (char *)&short_val, 0, BFLD_SHORT), SUCCEED);
    len=sizeof(carray);
    assert_equal(Bget(p_ub, T_CARRAY_FLD, 0, (char *)test_val, &len), SUCCEED);
    assert_equal(strncmp(test_val,  "22321", 5), 0);
    assert_equal(len, 5);
    do_dummy_data_test(p_ub);
    /* long-to-carray */
    assert_equal(CBchg(p_ub, T_CARRAY_FLD, 0, (char *)&long_val, 0, BFLD_LONG), SUCCEED);
    len=sizeof(carray);
    assert_equal(Bget(p_ub, T_CARRAY_FLD, 0, (char *)test_val, &len), SUCCEED);
    assert_equal(strncmp(test_val,  "20223112", 8), 0);
    assert_equal(len, 8);
    do_dummy_data_test(p_ub);
    /* char-to-carray */
    assert_equal(CBchg(p_ub, T_CARRAY_FLD, 0, (char *)&char_val, 0, BFLD_CHAR), SUCCEED);
    len=sizeof(carray);
    assert_equal(Bget(p_ub, T_CARRAY_FLD, 0, (char *)test_val, &len), SUCCEED);
    assert_equal(strncmp(test_val,  "4", 1), 0); /* 55 - ascii code for '7' */
    assert_equal(len, 1);
    do_dummy_data_test(p_ub);
    /* float-to-carray (test on Tuxedo) */
    assert_equal(CBchg(p_ub, T_CARRAY_FLD, 0, (char *)&float_val, 0, BFLD_FLOAT), SUCCEED);
    len=sizeof(carray);
    assert_equal(Bget(p_ub, T_CARRAY_FLD, 0, (char *)test_val, &len), SUCCEED);
    assert_equal(strncmp(test_val, "211233.41", 8),0);
    assert_true(len>=8);
    do_dummy_data_test(p_ub);
    /* double-to-carray (test un Tuxedo?) */
    assert_equal(CBchg(p_ub, T_CARRAY_FLD, 0, (char *)&double_val, 0, BFLD_DOUBLE), SUCCEED);
    len=sizeof(carray);
    assert_equal(Bget(p_ub, T_CARRAY_FLD, 0, (char *)test_val, &len), SUCCEED);
    assert_equal(strncmp(test_val, "244.31234",9),0);
    assert_true(len>=9);
    do_dummy_data_test(p_ub);
    /* string-to-carray */
    assert_equal(CBchg(p_ub, T_CARRAY_FLD, 0, (char *)string, 0, BFLD_STRING), SUCCEED);
    len=sizeof(carray);
    assert_equal(Bget(p_ub, T_CARRAY_FLD, 0, (char *)test_val, &len), SUCCEED);
    assert_equal(strncmp(test_val, string, strlen(string)),0);

    do_dummy_data_test(p_ub);
    /* carray-to-carray */
    assert_equal(CBchg(p_ub, T_CARRAY_FLD, 0, (char *)carray, sizeof(carray), BFLD_CARRAY), SUCCEED);
    len=sizeof(carray);
    assert_equal(Bget(p_ub, T_CARRAY_FLD, 0, (char *)test_val, &len), SUCCEED);
    assert_equal(strncmp(test_val, carray, sizeof(carray)), 0);
    do_dummy_data_test(p_ub);

    /* Special test for hadling large strings & carrays */
    str_len_tmp=strlen(BIG_TEST_STRING);

    for (big_loop=0; big_loop<10;big_loop++)
    {
        assert_equal(CBchg(p_ub, T_CARRAY_FLD, 0, (char *)BIG_TEST_STRING, str_len_tmp, BFLD_CARRAY), SUCCEED);
        len = sizeof(carray_big);
        assert_equal(Bget(p_ub, T_CARRAY_FLD, 0, (char *)carray_big, &len), SUCCEED);
        assert_equal(len, str_len_tmp);
        assert_equal(strncmp(carray_big, BIG_TEST_STRING, str_len_tmp), 0);
    }
}


Ensure(test_Bchg_simple)
{
    char fb[1024];
    char buf[1024]="This is test...";
    int len = sizeof(buf);
    
    UBFH *p_ub = (UBFH *)fb;
    assert_equal(Binit(p_ub, len), SUCCEED);
    assert_equal(CBchg(p_ub, T_CARRAY_FLD, 0, buf, len-40, BFLD_CARRAY), SUCCEED);
}

TestSuite *ubf_cfchg_tests(void)
{
    TestSuite *suite = create_test_suite();
/*
    setup_(suite, basic_setup1);
    teardown_(suite, basic_teardown1);
*/
    add_test(suite, test_Bchg_simple);
    add_test(suite, test_Bchg_short_org);
    add_test(suite, test_Bchg_long_org);
    add_test(suite, test_Bchg_char_org);
    add_test(suite, test_Bchg_float_org);
    add_test(suite, test_Bchg_double_org);
    add_test(suite, test_Bchg_string_org);
    add_test(suite, test_Bchg_carray_org);
    return suite;
}

