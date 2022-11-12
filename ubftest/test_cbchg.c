/**
 *
 * @file test_cbchg.c
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
#include <fml.h>
#include <fml32.h>
#include <ndrstandard.h>
#include <string.h>
#include "test.fd.h"
#include "ubfunit1.h"

/**
 * Original field is BFLD_SHORT => T_SHORT_FLD
 */
Ensure(test_Bchg_short_org)
{
    char buf[2048];
    UBFH *p_ub = (UBFH *)buf;
    short test_val;
    short short_val = 123;
    long long_val = 102;
    char char_val = '7';
    float float_val=11;
    double double_val=1443;
    char string[129]="31255";
    char carray[128]="26411";
    char *ptr=(char *)(ndrx_longptr_t)11;
    
    /* init */
    assert_equal(Binit(p_ub, sizeof(buf)), EXSUCCEED);
    set_up_dummy_data(p_ub);

    /* short-to-short */
    assert_equal(CBchg(p_ub, T_SHORT_FLD, 0, (char *)&short_val, 0, BFLD_SHORT), EXSUCCEED);
    assert_equal(Bget(p_ub, T_SHORT_FLD, 0, (char *)&test_val, 0), EXSUCCEED);
    assert_equal(test_val, 123);
    do_dummy_data_test(p_ub);
    /* long-to-short */
    assert_equal(CBchg(p_ub, T_SHORT_FLD, 0, (char *)&long_val, 0, BFLD_LONG), EXSUCCEED);
    assert_equal(Bget(p_ub, T_SHORT_FLD, 0, (char *)&test_val, 0), EXSUCCEED);
    assert_equal(test_val, 102);
    do_dummy_data_test(p_ub);
    /* char-to-short */
    assert_equal(CBchg(p_ub, T_SHORT_FLD, 0, (char *)&char_val, 0, BFLD_CHAR), EXSUCCEED);
    assert_equal(Bget(p_ub, T_SHORT_FLD, 0, (char *)&test_val, 0), EXSUCCEED);
    assert_equal(test_val, 55); /* 55 - ascii code for '7' */
    do_dummy_data_test(p_ub);
    /* float-to-short */
    assert_equal(CBchg(p_ub, T_SHORT_FLD, 0, (char *)&float_val, 0, BFLD_FLOAT), EXSUCCEED);
    assert_equal(Bget(p_ub, T_SHORT_FLD, 0, (char *)&test_val, 0), EXSUCCEED);
    assert_equal(test_val, 11);
    do_dummy_data_test(p_ub);
    /* double-to-short */
    assert_equal(CBchg(p_ub, T_SHORT_FLD, 0, (char *)&double_val, 0, BFLD_DOUBLE), EXSUCCEED);
    assert_equal(Bget(p_ub, T_SHORT_FLD, 0, (char *)&test_val, 0), EXSUCCEED);
    assert_equal(test_val, 1443);
    do_dummy_data_test(p_ub);
    /* string-to-short */
    assert_equal(CBchg(p_ub, T_SHORT_FLD, 0, (char *)string, 0, BFLD_STRING), EXSUCCEED);
    assert_equal(Bget(p_ub, T_SHORT_FLD, 0, (char *)&test_val, 0), EXSUCCEED);
    assert_equal(test_val, 31255);
    do_dummy_data_test(p_ub);
    /* carray-to-short */
    assert_equal(CBchg(p_ub, T_SHORT_FLD, 0, (char *)carray, strlen(carray), BFLD_CARRAY), EXSUCCEED);
    assert_equal(Bget(p_ub, T_SHORT_FLD, 0, (char *)&test_val, 0), EXSUCCEED);
    assert_equal(test_val, 26411);
    do_dummy_data_test(p_ub);
    
    /* ptr-to-short */
    assert_equal(CBchg(p_ub, T_SHORT_FLD, 0, (char *)&ptr, 0L, BFLD_PTR), EXSUCCEED);
    assert_equal(Bget(p_ub, T_SHORT_FLD, 0, (char *)&test_val, 0), EXSUCCEED);
    assert_equal(test_val, 11);
    do_dummy_data_test(p_ub);
    
    /* ubf-to-short */
    assert_equal(CBchg(p_ub, T_SHORT_FLD, 0, (char *)&ptr, 0L, BFLD_UBF), EXFAIL);
    assert_equal(Berror, BEBADOP);
    do_dummy_data_test(p_ub);
    
    /* view-to-short */
    assert_equal(CBchg(p_ub, T_SHORT_FLD, 0, (char *)ptr, 0L, BFLD_VIEW), EXFAIL);
    assert_equal(Berror, BEBADOP);
    do_dummy_data_test(p_ub);
    
}

/**
 * Original field is BFLD_LONG => T_LONG_FLD
 */
Ensure(test_Bchg_long_org)
{
    char buf[2048];
    UBFH *p_ub = (UBFH *)buf;
    long test_val;
    short short_val = 123;
    long long_val = 102;
    char char_val = '7';
    float float_val=11123341;
    double double_val=14431234;
    char string[129]="3125511";
    char carray[128]="2641133";
    char *ptr=(char *)(ndrx_longptr_t)11;
    
    /* init */
    assert_equal(Binit(p_ub, sizeof(buf)), EXSUCCEED);
    set_up_dummy_data(p_ub);

    /* short-to-long */
    assert_equal(CBchg(p_ub, T_LONG_FLD, 0, (char *)&short_val, 0, BFLD_SHORT), EXSUCCEED);
    assert_equal(Bget(p_ub, T_LONG_FLD, 0, (char *)&test_val, 0), EXSUCCEED);
    assert_equal(test_val, 123);
    do_dummy_data_test(p_ub);
    /* long-to-long */
    assert_equal(CBchg(p_ub, T_LONG_FLD, 0, (char *)&long_val, 0, BFLD_LONG), EXSUCCEED);
    assert_equal(Bget(p_ub, T_LONG_FLD, 0, (char *)&test_val, 0), EXSUCCEED);
    assert_equal(test_val, 102);
    do_dummy_data_test(p_ub);
    /* char-to-long */
    assert_equal(CBchg(p_ub, T_LONG_FLD, 0, (char *)&char_val, 0, BFLD_CHAR), EXSUCCEED);
    assert_equal(Bget(p_ub, T_LONG_FLD, 0, (char *)&test_val, 0), EXSUCCEED);
    assert_equal(test_val, 55); /* 55 - ascii code for '7' */
    do_dummy_data_test(p_ub);
    /* float-to-long */
    assert_equal(CBchg(p_ub, T_LONG_FLD, 0, (char *)&float_val, 0, BFLD_FLOAT), EXSUCCEED);
    assert_equal(Bget(p_ub, T_LONG_FLD, 0, (char *)&test_val, 0), EXSUCCEED);
    assert_equal(test_val, 11123341);
    do_dummy_data_test(p_ub);
    /* double-to-long */
    assert_equal(CBchg(p_ub, T_LONG_FLD, 0, (char *)&double_val, 0, BFLD_DOUBLE), EXSUCCEED);
    assert_equal(Bget(p_ub, T_LONG_FLD, 0, (char *)&test_val, 0), EXSUCCEED);
    assert_equal(test_val, 14431234);
    do_dummy_data_test(p_ub);
    /* string-to-long */
    assert_equal(CBchg(p_ub, T_LONG_FLD, 0, (char *)string, 0, BFLD_STRING), EXSUCCEED);
    assert_equal(Bget(p_ub, T_LONG_FLD, 0, (char *)&test_val, 0), EXSUCCEED);
    assert_equal(test_val, 3125511);
    do_dummy_data_test(p_ub);
    /* carray-to-long */
    assert_equal(CBchg(p_ub, T_LONG_FLD, 0, (char *)carray, strlen(carray), BFLD_CARRAY), EXSUCCEED);
    assert_equal(Bget(p_ub, T_LONG_FLD, 0, (char *)&test_val, 0), EXSUCCEED);
    assert_equal(test_val, 2641133);
    do_dummy_data_test(p_ub);
    
    /* ptr-to-long */
    assert_equal(CBchg(p_ub, T_LONG_FLD, 0, (char *)&ptr, 0L, BFLD_PTR), EXSUCCEED);
    assert_equal(Bget(p_ub, T_LONG_FLD, 0, (char *)&test_val, 0), EXSUCCEED);
    assert_equal(test_val, 11);
    do_dummy_data_test(p_ub);
    
    /* ubf-to-long */
    assert_equal(CBchg(p_ub, T_LONG_FLD, 0, (char *)&ptr, 0L, BFLD_UBF), EXFAIL);
    assert_equal(Berror, BEBADOP);
    do_dummy_data_test(p_ub);
    
    /* view-to-long */
    assert_equal(CBchg(p_ub, T_LONG_FLD, 0, (char *)ptr, 0L, BFLD_VIEW), EXFAIL);
    assert_equal(Berror, BEBADOP);
    do_dummy_data_test(p_ub);
}

/**
 * Original field is BFLD_CHAR => T_CHAR_FLD
 */
Ensure(test_Bchg_char_org)
{
    char buf[2048];
    UBFH *p_ub = (UBFH *)buf;
    char test_val;
    short short_val = 1;
    long long_val = 2;
    char char_val = '7';
    float float_val=3;
    double double_val=4;
    char string[129]="5";
    char carray[128]="6";
    char *ptr=(char *)(ndrx_longptr_t)65;
    
    /* init */
    assert_equal(Binit(p_ub, sizeof(buf)), EXSUCCEED);
    set_up_dummy_data(p_ub);

    /* short-to-char */
    assert_equal(CBchg(p_ub, T_CHAR_FLD, 0, (char *)&short_val, 0, BFLD_SHORT), EXSUCCEED);
    assert_equal(Bget(p_ub, T_CHAR_FLD, 0, (char *)&test_val, 0), EXSUCCEED);
    assert_equal(test_val, 1);
    do_dummy_data_test(p_ub);
    /* long-to-char */
    assert_equal(CBchg(p_ub, T_CHAR_FLD, 0, (char *)&long_val, 0, BFLD_LONG), EXSUCCEED);
    assert_equal(Bget(p_ub, T_CHAR_FLD, 0, (char *)&test_val, 0), EXSUCCEED);
    assert_equal(test_val, 2);
    do_dummy_data_test(p_ub);
    /* char-to-long */
    assert_equal(CBchg(p_ub, T_CHAR_FLD, 0, (char *)&char_val, 0, BFLD_CHAR), EXSUCCEED);
    assert_equal(Bget(p_ub, T_CHAR_FLD, 0, (char *)&test_val, 0), EXSUCCEED);
    assert_equal(test_val, 55); /* 55 - ascii code for '7' */
    do_dummy_data_test(p_ub);
    /* float-to-char */
    assert_equal(CBchg(p_ub, T_CHAR_FLD, 0, (char *)&float_val, 0, BFLD_FLOAT), EXSUCCEED);
    assert_equal(Bget(p_ub, T_CHAR_FLD, 0, (char *)&test_val, 0), EXSUCCEED);
    assert_equal(test_val, 3);
    do_dummy_data_test(p_ub);
    /* double-to-char */
    assert_equal(CBchg(p_ub, T_CHAR_FLD, 0, (char *)&double_val, 0, BFLD_DOUBLE), EXSUCCEED);
    assert_equal(Bget(p_ub, T_CHAR_FLD, 0, (char *)&test_val, 0), EXSUCCEED);
    assert_equal(test_val, 4);
    do_dummy_data_test(p_ub);
    /* string-to-char */
    assert_equal(CBchg(p_ub, T_CHAR_FLD, 0, (char *)string, 0, BFLD_STRING), EXSUCCEED);
    assert_equal(Bget(p_ub, T_CHAR_FLD, 0, (char *)&test_val, 0), EXSUCCEED);
    assert_equal(test_val, 53);
    do_dummy_data_test(p_ub);
    /* carray-to-char */
    assert_equal(CBchg(p_ub, T_CHAR_FLD, 0, (char *)carray, strlen(carray), BFLD_CARRAY), EXSUCCEED);
    assert_equal(Bget(p_ub, T_CHAR_FLD, 0, (char *)&test_val, 0), EXSUCCEED);
    assert_equal(test_val, 54);
    do_dummy_data_test(p_ub);
    
    /* ptr-to-char */
    assert_equal(CBchg(p_ub, T_CHAR_FLD, 0, (char *)&ptr, 0L, BFLD_PTR), EXSUCCEED);
    assert_equal(Bget(p_ub, T_CHAR_FLD, 0, (char *)&test_val, 0), EXSUCCEED);
    assert_equal(test_val, 'A');
    do_dummy_data_test(p_ub);
    
    /* ubf-to-char */
    assert_equal(CBchg(p_ub, T_CHAR_FLD, 0, (char *)&ptr, 0L, BFLD_UBF), EXFAIL);
    assert_equal(Berror, BEBADOP);
    do_dummy_data_test(p_ub);
    
    /* view-to-char */
    assert_equal(CBchg(p_ub, T_CHAR_FLD, 0, (char *)ptr, 0L, BFLD_VIEW), EXFAIL);
    assert_equal(Berror, BEBADOP);
    do_dummy_data_test(p_ub);
}

/**
 * Original field is BFLD_FLOAT => T_FLOAT_FLD
 */
Ensure(test_Bchg_float_org)
{
    char buf[2048];
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
    char *ptr=(char *)(ndrx_longptr_t)65;
    
    /* init */
    assert_equal(Binit(p_ub, sizeof(buf)), EXSUCCEED);
    set_up_dummy_data(p_ub);

    /* short-to-float */
    len=sizeof(test_val);
    assert_equal(CBchg(p_ub, T_FLOAT_FLD, 0, (char *)&short_val, 0, BFLD_SHORT), EXSUCCEED);
    assert_equal(Bget(p_ub, T_FLOAT_FLD, 0, (char *)&test_val, 0), EXSUCCEED);
    assert_double_equal(test_val, 12321);
    do_dummy_data_test(p_ub);
    assert_equal(sizeof(test_val), len);
    /* long-to-float */
    assert_equal(CBchg(p_ub, T_FLOAT_FLD, 0, (char *)&long_val, 0, BFLD_LONG), EXSUCCEED);
    assert_equal(Bget(p_ub, T_FLOAT_FLD, 0, (char *)&test_val, 0), EXSUCCEED);
    assert_double_equal(test_val, 10223112);
    do_dummy_data_test(p_ub);
    /* char-to-float */
    len=sizeof(test_val);
    assert_equal(CBchg(p_ub, T_FLOAT_FLD, 0, (char *)&char_val, 0, BFLD_CHAR), EXSUCCEED);
    assert_equal(Bget(p_ub, T_FLOAT_FLD, 0, (char *)&test_val, &len), EXSUCCEED);
    assert_double_equal(test_val, 55); /* 55 - ascii code for '7' */
    do_dummy_data_test(p_ub);
    assert_equal(len, sizeof(test_val));
    len=sizeof(test_val)-1;
    /* validate the error output! */
    assert_equal(Bget(p_ub, T_FLOAT_FLD, 0, (char *)&test_val, &len), EXFAIL);
    assert_equal(Berror, BNOSPACE);
    /* Check the case when buffer is too short! */
    /* float-to-float */
    assert_equal(CBchg(p_ub, T_FLOAT_FLD, 0, (char *)&float_val, 0, BFLD_FLOAT), EXSUCCEED);
    assert_equal(Bget(p_ub, T_FLOAT_FLD, 0, (char *)&test_val, 0), EXSUCCEED);
    assert_double_equal(test_val, 111233.41);
    do_dummy_data_test(p_ub);
    /* double-to-float */
    len=sizeof(test_val);
    assert_equal(CBchg(p_ub, T_FLOAT_FLD, 0, (char *)&double_val, 0, BFLD_DOUBLE), EXSUCCEED);
    assert_equal(Bget(p_ub, T_FLOAT_FLD, 0, (char *)&test_val, &len), EXSUCCEED);
    assert_double_equal(test_val, 144.31234);
    do_dummy_data_test(p_ub);
    assert_equal(len, sizeof(test_val));
    /* string-to-float */
    assert_equal(CBchg(p_ub, T_FLOAT_FLD, 0, (char *)string, 0, BFLD_STRING), EXSUCCEED);
    assert_equal(Bget(p_ub, T_FLOAT_FLD, 0, (char *)&test_val, 0), EXSUCCEED);
    assert_double_equal(test_val, 3125.511);
    do_dummy_data_test(p_ub);
    /* carray-to-float */
    len=sizeof(test_val);
    assert_equal(CBchg(p_ub, T_FLOAT_FLD, 0, (char *)carray, strlen(carray), BFLD_CARRAY), EXSUCCEED);
    assert_equal(Bget(p_ub, T_FLOAT_FLD, 0, (char *)&test_val, &len), EXSUCCEED);
    assert_double_equal(test_val, -264.1133f);
    do_dummy_data_test(p_ub);
    assert_equal(len, sizeof(test_val));
    
    /* ptr-to-float */
    assert_equal(CBchg(p_ub, T_FLOAT_FLD, 0, (char *)&ptr, 0L, BFLD_PTR), EXSUCCEED);
    assert_equal(Bget(p_ub, T_FLOAT_FLD, 0, (char *)&test_val, 0), EXSUCCEED);
    assert_double_equal(test_val, 65.0f);
    do_dummy_data_test(p_ub);
    
    /* ubf-to-float */
    assert_equal(CBchg(p_ub, T_FLOAT_FLD, 0, (char *)&ptr, 0L, BFLD_UBF), EXFAIL);
    assert_equal(Berror, BEBADOP);
    do_dummy_data_test(p_ub);
    
    /* view-to-float */
    assert_equal(CBchg(p_ub, T_FLOAT_FLD, 0, (char *)ptr, 0L, BFLD_VIEW), EXFAIL);
    assert_equal(Berror, BEBADOP);
    do_dummy_data_test(p_ub);
}

/**
 * Original field is BFLD_DOUBLE => T_DOUBLE_FLD
 */
Ensure(test_Bchg_double_org)
{
    char buf[2048];
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
    char *ptr=(char *)(ndrx_longptr_t)65;
    
    /* init */
    assert_equal(Binit(p_ub, sizeof(buf)), EXSUCCEED);
    set_up_dummy_data(p_ub);

    /* short-to-float */
    assert_equal(CBchg(p_ub, T_DOUBLE_FLD, 0, (char *)&short_val, 0, BFLD_SHORT), EXSUCCEED);
    assert_equal(Bget(p_ub, T_DOUBLE_FLD, 0, (char *)&test_val, 0), EXSUCCEED);
    assert_double_equal(test_val, 22321);
    do_dummy_data_test(p_ub);
    /* long-to-float */
    len=sizeof(test_val);
    assert_equal(CBchg(p_ub, T_DOUBLE_FLD, 0, (char *)&long_val, 0, BFLD_LONG), EXSUCCEED);
    assert_equal(Bget(p_ub, T_DOUBLE_FLD, 0, (char *)&test_val, &len), EXSUCCEED);
    assert_double_equal(test_val, 20223112);
    do_dummy_data_test(p_ub);
    assert_equal(sizeof(test_val), len);

    /* char-to-float */
    assert_equal(CBchg(p_ub, T_DOUBLE_FLD, 0, (char *)&char_val, 0, BFLD_CHAR), EXSUCCEED);
    assert_equal(Bget(p_ub, T_DOUBLE_FLD, 0, (char *)&test_val, 0), EXSUCCEED);
    assert_double_equal(test_val, 52); /* 55 - ascii code for '7' */
    do_dummy_data_test(p_ub);
    /* float-to-float */
    len=sizeof(test_val);
    assert_equal(CBchg(p_ub, T_DOUBLE_FLD, 0, (char *)&float_val, 0, BFLD_FLOAT), EXSUCCEED);
    assert_equal(Bget(p_ub, T_DOUBLE_FLD, 0, (char *)&test_val, &len), EXSUCCEED);
    assert_double_equal(test_val, 211233.41);
    do_dummy_data_test(p_ub);
    assert_equal(sizeof(test_val), len);
    /* double-to-float */
    assert_equal(CBchg(p_ub, T_DOUBLE_FLD, 0, (char *)&double_val, 0, BFLD_DOUBLE), EXSUCCEED);
    assert_equal(Bget(p_ub, T_DOUBLE_FLD, 0, (char *)&test_val, 0), EXSUCCEED);
    assert_double_equal(test_val, 244.31234);
    do_dummy_data_test(p_ub);
    /* string-to-double */
    len=sizeof(test_val);
    assert_equal(CBchg(p_ub, T_DOUBLE_FLD, 0, (char *)string, 0, BFLD_STRING), EXSUCCEED);
    assert_equal(Bget(p_ub, T_DOUBLE_FLD, 0, (char *)&test_val, &len), EXSUCCEED);
    assert_double_equal(test_val, 2125.511);
    do_dummy_data_test(p_ub);
    assert_equal(sizeof(test_val), len);

    /* carray-to-float */
    assert_equal(CBchg(p_ub, T_DOUBLE_FLD, 0, (char *)carray, strlen(carray), BFLD_CARRAY), EXSUCCEED);
    assert_equal(Bget(p_ub, T_DOUBLE_FLD, 0, (char *)&test_val, 0), EXSUCCEED);
    assert_double_equal(test_val, -2264.1133);
    do_dummy_data_test(p_ub);
    
    /* ptr-to-double */
    assert_equal(CBchg(p_ub, T_DOUBLE_FLD, 0, (char *)&ptr, 0L, BFLD_PTR), EXSUCCEED);
    assert_equal(Bget(p_ub, T_DOUBLE_FLD, 0, (char *)&test_val, 0), EXSUCCEED);
    assert_double_equal(test_val, 65.0f);
    do_dummy_data_test(p_ub);
    
    /* ubf-to-double */
    assert_equal(CBchg(p_ub, T_DOUBLE_FLD, 0, (char *)&ptr, 0L, BFLD_UBF), EXFAIL);
    assert_equal(Berror, BEBADOP);
    do_dummy_data_test(p_ub);
    
    /* view-to-double */
    assert_equal(CBchg(p_ub, T_DOUBLE_FLD, 0, (char *)ptr, 0L, BFLD_VIEW), EXFAIL);
    assert_equal(Berror, BEBADOP);
    do_dummy_data_test(p_ub);
}

/**
 * Original field is BFLD_STRING => T_STRING_FLD
 */
Ensure(test_Bchg_string_org)
{
    char buf[3072];
    UBFH *p_ub = (UBFH *)buf;
    char test_val[641];
    short short_val = 22321;
    long long_val = 20223112;
    char char_val = '4';
    float float_val=211233.41;
    double double_val=244.31234;
    double double_val2=1223234232342324323423423423423423434342343454353453243245343453243452343453243243425343453425523423423423423234234234234234234234234234234234234234234234234243234234324343.65;
    double double_tmp;
    char string[514+1]="2125.511";
    char carray[514];
    int i;
    BFLDLEN len;
    /* variables for big buffer test */
    char string_big[1024];
    int str_len_tmp;
    int big_loop;
    char *ptr=(char *)(ndrx_longptr_t)65;
    
    assert_equal(Binit(p_ub, sizeof(buf)), EXSUCCEED);
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

    assert_equal(CBchg(p_ub, T_STRING_FLD, 0, (char *)&short_val, 0, BFLD_SHORT), EXSUCCEED);
    Bprint(p_ub);
    assert_equal(Bget(p_ub, T_STRING_FLD, 0, (char *)test_val, &len), EXSUCCEED);
    assert_string_equal(test_val, "22321");
    assert_equal(len, 6);

    do_dummy_data_test(p_ub);
    /* long-to-string */
    assert_equal(CBchg(p_ub, T_STRING_FLD, 0, (char *)&long_val, 0, BFLD_LONG), EXSUCCEED);
    assert_equal(Bget(p_ub, T_STRING_FLD, 0, (char *)test_val, 0), EXSUCCEED);
    assert_string_equal(test_val, "20223112");
    do_dummy_data_test(p_ub);
    /* char-to-string */
    len=sizeof(test_val);
    assert_equal(CBchg(p_ub, T_STRING_FLD, 0, (char *)&char_val, 0, BFLD_CHAR), EXSUCCEED);
    assert_equal(Bget(p_ub, T_STRING_FLD, 0, (char *)test_val, &len), EXSUCCEED);
    assert_string_equal(test_val, "4"); /* 55 - ascii code for '7' */
    assert_equal(len, 2);
    do_dummy_data_test(p_ub);
    /* float-to-string (test on Tuxedo) */
    assert_equal(CBchg(p_ub, T_STRING_FLD, 0, (char *)&float_val, 0, BFLD_FLOAT), EXSUCCEED);
    assert_equal(Bget(p_ub, T_STRING_FLD, 0, (char *)test_val, 0), EXSUCCEED);
    assert_equal(strncmp(test_val, "211233.41", 8),0);
    do_dummy_data_test(p_ub);
    /* double-to-string (test un Tuxedo?) */
    len=sizeof(test_val);
    assert_equal(CBchg(p_ub, T_STRING_FLD, 0, (char *)&double_val, 0, BFLD_DOUBLE), EXSUCCEED);
    assert_equal(Bget(p_ub, T_STRING_FLD, 0, (char *)test_val, &len), EXSUCCEED);
    assert_equal(strncmp(test_val, "244.31234",9),0);
    assert_true(len>9);
    do_dummy_data_test(p_ub);
    assert_equal(CBchg(p_ub, T_STRING_FLD, 1, (char *)&double_val2, 0, BFLD_DOUBLE), EXSUCCEED);
    assert_equal(CBget(p_ub, T_STRING_FLD, 1, (char *)&double_tmp, 0, BFLD_DOUBLE), EXSUCCEED);
    assert_double_equal(double_val2, double_tmp);
    Bdel(p_ub, T_STRING_FLD, 1);

    /* string-to-string */
    assert_equal(CBchg(p_ub, T_STRING_FLD, 0, (char *)string, 0, BFLD_STRING), EXSUCCEED);
    assert_equal(Bget(p_ub, T_STRING_FLD, 0, (char *)test_val, 0), EXSUCCEED);
    assert_string_equal(test_val, string);
    do_dummy_data_test(p_ub);
    /* carray-to-string */
    len=sizeof(test_val);
    assert_equal(CBchg(p_ub, T_STRING_FLD, 0, (char *)carray, sizeof(carray), BFLD_CARRAY), EXSUCCEED);
    assert_equal(Bget(p_ub, T_STRING_FLD, 0, (char *)test_val, &len), EXSUCCEED);
    assert_equal(strncmp(test_val, carray, sizeof(carray)), 0);
    assert_equal(len, sizeof(carray)+1);
    do_dummy_data_test(p_ub);

    /* Special test for handling large strings & carrays */
    for (big_loop=0; big_loop<10;big_loop++)
    {
        str_len_tmp=strlen(BIG_TEST_STRING);
        assert_equal(CBchg(p_ub, T_STRING_FLD, 0, (char *)BIG_TEST_STRING, str_len_tmp, BFLD_CARRAY), EXSUCCEED);
        len = sizeof(string_big);
        assert_equal(Bget(p_ub, T_STRING_FLD, 0, (char *)string_big, &len), EXSUCCEED);
        assert_equal(len, str_len_tmp+1);
        assert_equal(strncmp(string_big, BIG_TEST_STRING, str_len_tmp), 0);
    }
    
    /* ptr-to-string */
    assert_equal(CBchg(p_ub, T_STRING_FLD, 0, (char *)&ptr, 0L, BFLD_PTR), EXSUCCEED);
    assert_equal(Bget(p_ub, T_STRING_FLD, 0, (char *)&test_val, 0), EXSUCCEED);
    assert_string_equal(test_val, "0x41");
    do_dummy_data_test(p_ub);
    
    /* ubf-to-string */
    assert_equal(CBchg(p_ub, T_STRING_FLD, 0, (char *)&ptr, 0L, BFLD_UBF), EXFAIL);
    assert_equal(Berror, BEBADOP);
    do_dummy_data_test(p_ub);
    
    /* view-to-string */
    assert_equal(CBchg(p_ub, T_STRING_FLD, 0, (char *)ptr, 0L, BFLD_VIEW), EXFAIL);
    assert_equal(Berror, BEBADOP);
    do_dummy_data_test(p_ub);

}

/**
 * Original field is BFLD_CARRAY => T_CARRAY_FLD
 */
Ensure(test_Bchg_carray_org)
{
    char buf[3072];
    UBFH *p_ub = (UBFH *)buf;
    char test_val[514];
    short short_val = 22321;
    long long_val = 20223112;
    char char_val = '4';
    float float_val=211233.41;
    double double_val=244.31234;
    double double_val2=1223234232342324323423423423423423434342343454353453243245343453243452343453243243425343453425523423423423423234234234234234234234234234234234234234234234234243234234324343.65;
    char string[514+1]="2125.511";
    char carray[514];
    int i;
    int len;
    /* Big buffer test... */
    char carray_big[1024];
    int str_len_tmp;
    int big_loop;
    
    /* working in LP64 mode */
    char *ptr=(char *)(ndrx_longptr_t)0xff;
    
    assert_equal(Binit(p_ub, sizeof(buf)), EXSUCCEED);
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
    assert_equal(CBchg(p_ub, T_CARRAY_FLD, 0, (char *)&short_val, 0, BFLD_SHORT), EXSUCCEED);
    Bprint(p_ub);
    len=sizeof(carray);
    assert_equal(Bget(p_ub, T_CARRAY_FLD, 0, (char *)test_val, &len), EXSUCCEED);
    assert_equal(strncmp(test_val,  "22321", 5), 0);
    assert_equal(len, 5);
    do_dummy_data_test(p_ub);
    
    /* Bug #495 - no length passed */
    assert_equal(Bget(p_ub, T_CARRAY_FLD, 0, (char *)test_val, NULL), EXSUCCEED);
    assert_equal(strncmp(test_val,  "22321", 5), 0);
    
    /* long-to-carray */
    assert_equal(CBchg(p_ub, T_CARRAY_FLD, 0, (char *)&long_val, 0, BFLD_LONG), EXSUCCEED);
    len=sizeof(carray);
    assert_equal(Bget(p_ub, T_CARRAY_FLD, 0, (char *)test_val, &len), EXSUCCEED);
    assert_equal(strncmp(test_val,  "20223112", 8), 0);
    assert_equal(len, 8);
    do_dummy_data_test(p_ub);
    /* char-to-carray */
    assert_equal(CBchg(p_ub, T_CARRAY_FLD, 0, (char *)&char_val, 0, BFLD_CHAR), EXSUCCEED);
    len=sizeof(carray);
    assert_equal(Bget(p_ub, T_CARRAY_FLD, 0, (char *)test_val, &len), EXSUCCEED);
    assert_equal(strncmp(test_val,  "4", 1), 0); /* 55 - ascii code for '7' */
    assert_equal(len, 1);
    do_dummy_data_test(p_ub);
    /* float-to-carray (test on Tuxedo) */
    assert_equal(CBchg(p_ub, T_CARRAY_FLD, 0, (char *)&float_val, 0, BFLD_FLOAT), EXSUCCEED);
    len=sizeof(carray);
    assert_equal(Bget(p_ub, T_CARRAY_FLD, 0, (char *)test_val, &len), EXSUCCEED);
    assert_equal(strncmp(test_val, "211233.41", 8),0);
    assert_true(len>=8);
    do_dummy_data_test(p_ub);
    /* double-to-carray (test un Tuxedo?) */
    assert_equal(CBchg(p_ub, T_CARRAY_FLD, 0, (char *)&double_val, 0, BFLD_DOUBLE), EXSUCCEED);
    len=sizeof(carray);
    assert_equal(Bget(p_ub, T_CARRAY_FLD, 0, (char *)test_val, &len), EXSUCCEED);
    assert_equal(strncmp(test_val, "244.31234",9),0);
    assert_true(len>=9);
    do_dummy_data_test(p_ub);

    assert_equal(CBchg(p_ub, T_CARRAY_FLD, 1, (char *)&double_val2, 0, BFLD_DOUBLE), EXSUCCEED);
    Bdel(p_ub, T_CARRAY_FLD, 1);

    /* string-to-carray */
    assert_equal(CBchg(p_ub, T_CARRAY_FLD, 0, (char *)string, 0, BFLD_STRING), EXSUCCEED);
    len=sizeof(carray);
    assert_equal(Bget(p_ub, T_CARRAY_FLD, 0, (char *)test_val, &len), EXSUCCEED);
    assert_equal(strncmp(test_val, string, strlen(string)),0);

    do_dummy_data_test(p_ub);
    /* carray-to-carray */
    assert_equal(CBchg(p_ub, T_CARRAY_FLD, 0, (char *)carray, sizeof(carray), BFLD_CARRAY), EXSUCCEED);
    len=sizeof(carray);
    assert_equal(Bget(p_ub, T_CARRAY_FLD, 0, (char *)test_val, &len), EXSUCCEED);
    assert_equal(strncmp(test_val, carray, sizeof(carray)), 0);

    /* test empty carray */
    assert_equal(CBchg(p_ub, T_CARRAY_FLD, 0, (char *)carray, 0, BFLD_CARRAY), EXSUCCEED);
    len=sizeof(carray);
    assert_equal(Bget(p_ub, T_CARRAY_FLD, 0, (char *)test_val, &len), EXSUCCEED);
    assert_equal(len, 0);

    do_dummy_data_test(p_ub);

    /* Special test for hadling large strings & carrays */
    str_len_tmp=strlen(BIG_TEST_STRING);

    for (big_loop=0; big_loop<10;big_loop++)
    {
        assert_equal(CBchg(p_ub, T_CARRAY_FLD, 0, (char *)BIG_TEST_STRING, str_len_tmp, BFLD_CARRAY), EXSUCCEED);
        len = sizeof(carray_big);
        assert_equal(Bget(p_ub, T_CARRAY_FLD, 0, (char *)carray_big, &len), EXSUCCEED);
        assert_equal(len, str_len_tmp);
        assert_equal(strncmp(carray_big, BIG_TEST_STRING, str_len_tmp), 0);
    }
    
    /* ptr-to-carray */
    assert_equal(CBchg(p_ub, T_CARRAY_FLD, 0, (char *)&ptr, 0L, BFLD_PTR), EXSUCCEED);
    assert_equal(Bget(p_ub, T_CARRAY_FLD, 0, (char *)&test_val, 0), EXSUCCEED);
    assert_equal(strncmp(test_val, "0xff", 4), 0);
    do_dummy_data_test(p_ub);
    
    /* ubf-to-carray */
    assert_equal(CBchg(p_ub, T_CARRAY_FLD, 0, (char *)&ptr, 0L, BFLD_UBF), EXFAIL);
    assert_equal(Berror, BEBADOP);
    do_dummy_data_test(p_ub);
    
    /* view-to-carray */
    assert_equal(CBchg(p_ub, T_CARRAY_FLD, 0, (char *)ptr, 0L, BFLD_VIEW), EXFAIL);
    assert_equal(Berror, BEBADOP);
    do_dummy_data_test(p_ub);
}

Ensure(test_Bchg_simple)
{
    char fb[1024];
    char buf[1024]="This is test...";
    int len = sizeof(fb);
    
    UBFH *p_ub = (UBFH *)fb;
    assert_equal(Binit(p_ub, len), EXSUCCEED);
    assert_equal(CBchg(p_ub, T_CARRAY_FLD, 0, buf, len-84, BFLD_CARRAY), EXSUCCEED);
}

/**
 * Note all over the system we assume that sizeof(long) == sizeof(ptr)
 * In case if will port to Windows someday, then this needs to be corrected
 * i.e. in that case we need to use long long LLP64
 */
Ensure(test_Bchg_ptr_org)
{
    char buf[2048];
    UBFH *p_ub = (UBFH *)buf;
    ndrx_longptr_t test_val;
    short short_val = 123;
    long long_val = 102;
    char char_val = '7';
    float float_val=11123341;
    double double_val=14431234;
    char string[129]="0x3125511";
    char carray[128]="0x2641133";
    char *ptr=(char *)(ndrx_longptr_t)11;
    
    /* init */
    assert_equal(Binit(p_ub, sizeof(buf)), EXSUCCEED);
    set_up_dummy_data(p_ub);

    /* short-to-ptr */
    assert_equal(CBchg(p_ub, T_PTR_FLD, 0, (char *)&short_val, 0, BFLD_SHORT), EXSUCCEED);
    assert_equal(Bget(p_ub, T_PTR_FLD, 0, (char *)&test_val, 0), EXSUCCEED);
    assert_equal(test_val, 123);
    do_dummy_data_test(p_ub);
    /* long-to-ptr */
    assert_equal(CBchg(p_ub, T_PTR_FLD, 0, (char *)&long_val, 0, BFLD_LONG), EXSUCCEED);
    assert_equal(Bget(p_ub, T_PTR_FLD, 0, (char *)&test_val, 0), EXSUCCEED);
    assert_equal(test_val, 102);
    do_dummy_data_test(p_ub);
    /* char-to-ptr */
    assert_equal(CBchg(p_ub, T_PTR_FLD, 0, (char *)&char_val, 0, BFLD_CHAR), EXSUCCEED);
    assert_equal(Bget(p_ub, T_PTR_FLD, 0, (char *)&test_val, 0), EXSUCCEED);
    assert_equal(test_val, 55); /* 55 - ascii code for '7' */
    do_dummy_data_test(p_ub);
    /* float-to-ptr */
    assert_equal(CBchg(p_ub, T_PTR_FLD, 0, (char *)&float_val, 0, BFLD_FLOAT), EXSUCCEED);
    assert_equal(Bget(p_ub, T_PTR_FLD, 0, (char *)&test_val, 0), EXSUCCEED);
    assert_equal(test_val, 11123341);
    do_dummy_data_test(p_ub);
    /* double-to-ptr */
    assert_equal(CBchg(p_ub, T_PTR_FLD, 0, (char *)&double_val, 0, BFLD_DOUBLE), EXSUCCEED);
    assert_equal(Bget(p_ub, T_PTR_FLD, 0, (char *)&test_val, 0), EXSUCCEED);
    assert_equal(test_val, 14431234);
    do_dummy_data_test(p_ub);
    /* string-to-ptr */
    assert_equal(CBchg(p_ub, T_PTR_FLD, 0, (char *)string, 0, BFLD_STRING), EXSUCCEED);
    assert_equal(Bget(p_ub, T_PTR_FLD, 0, (char *)&test_val, 0), EXSUCCEED);
    assert_equal(test_val, 0x3125511);
    do_dummy_data_test(p_ub);
    /* carray-to-ptr */
    assert_equal(CBchg(p_ub, T_PTR_FLD, 0, (char *)carray, strlen(carray), BFLD_CARRAY), EXSUCCEED);
    assert_equal(Bget(p_ub, T_PTR_FLD, 0, (char *)&test_val, 0), EXSUCCEED);
    assert_equal(test_val, 0x2641133);
    do_dummy_data_test(p_ub);
    
    /* ptr-to-ptr */
    assert_equal(CBchg(p_ub, T_PTR_FLD, 0, (char *)&ptr, 0L, BFLD_PTR), EXSUCCEED);
    assert_equal(Bget(p_ub, T_PTR_FLD, 0, (char *)&test_val, 0), EXSUCCEED);
    assert_equal(test_val, 11);
    do_dummy_data_test(p_ub);
    
    /* ubf-to-ptr */
    assert_equal(CBchg(p_ub, T_PTR_FLD, 0, (char *)&ptr, 0L, BFLD_UBF), EXFAIL);
    assert_equal(Berror, BEBADOP);
    do_dummy_data_test(p_ub);
    
    /* view-to-ptr */
    assert_equal(CBchg(p_ub, T_PTR_FLD, 0, (char *)ptr, 0L, BFLD_VIEW), EXFAIL);
    assert_equal(Berror, BEBADOP);
    do_dummy_data_test(p_ub);
}

Ensure(test_Bchg_ubf_org)
{
    char buf[2048];
    UBFH *p_ub = (UBFH *)buf;
    
    char buf_tmp[2048];
    UBFH *p_ub_tmp = (UBFH *)buf_tmp;
    
    /* init */
    assert_equal(Binit(p_ub, sizeof(buf)), EXSUCCEED);
    assert_equal(Binit(p_ub_tmp, sizeof(buf_tmp)), EXSUCCEED);
    
    set_up_dummy_data(p_ub);
    
    /* load some UBF */
    assert_equal(Badd(p_ub_tmp, T_STRING_FLD, "HELLO WORLD", 0), EXSUCCEED);
    assert_equal(Badd(p_ub, T_UBF_FLD, (char *)p_ub_tmp, 0), EXSUCCEED);

    /* short-to-ubf */
    assert_equal(CBchg(p_ub, T_UBF_FLD, 0, (char *)buf_tmp, 0, BFLD_SHORT), EXFAIL);
    assert_equal(Berror, BEBADOP);
    do_dummy_data_test(p_ub);
    
    /* long-to-ubf */
    assert_equal(CBchg(p_ub, T_UBF_FLD, 0, (char *)buf_tmp, 0, BFLD_LONG), EXFAIL);
    assert_equal(Berror, BEBADOP);
    do_dummy_data_test(p_ub);
    
    /* char-to-ubf */
    assert_equal(CBchg(p_ub, T_UBF_FLD, 0, (char *)buf_tmp, 0, BFLD_CHAR), EXFAIL);
    assert_equal(Berror, BEBADOP);
    do_dummy_data_test(p_ub);
    
    /* float-to-ubf */
    assert_equal(CBchg(p_ub, T_UBF_FLD, 0, (char *)buf_tmp, 0, BFLD_FLOAT), EXFAIL);
    assert_equal(Berror, BEBADOP);
    do_dummy_data_test(p_ub);
    
    /* double-to-ubf */
    assert_equal(CBchg(p_ub, T_UBF_FLD, 0, (char *)buf_tmp, 0, BFLD_DOUBLE), EXFAIL);
    assert_equal(Berror, BEBADOP);
    do_dummy_data_test(p_ub);
    
    /* string-to-ubf */
    assert_equal(CBchg(p_ub, T_UBF_FLD, 0, (char *)buf_tmp, 0, BFLD_STRING), EXFAIL);
    assert_equal(Berror, BEBADOP);
    do_dummy_data_test(p_ub);
    
    /* carray-to-ubf */
    assert_equal(CBchg(p_ub, T_UBF_FLD, 0, (char *)buf_tmp, 0, BFLD_CARRAY), EXFAIL);
    assert_equal(Berror, BEBADOP);
    do_dummy_data_test(p_ub);
    
    /* ptr-to-ubf */
    assert_equal(CBchg(p_ub, T_UBF_FLD, 0, (char *)buf_tmp, 0, BFLD_PTR), EXFAIL);
    assert_equal(Berror, BEBADOP);
    do_dummy_data_test(p_ub);
    
    /* view-to-ubf */
    assert_equal(CBchg(p_ub, T_UBF_FLD, 0, (char *)buf_tmp, 0, BFLD_VIEW), EXFAIL);
    assert_equal(Berror, BEBADOP);
    do_dummy_data_test(p_ub);
}

Ensure(test_Bchg_view_org)
{
    char buf[2048];
    UBFH *p_ub = (UBFH *)buf;
    char buf_tmp[2048];
    BVIEWFLD vf;
    struct UBTESTVIEW2 v;
    
    NDRX_STRCPY_SAFE(vf.vname, "UBTESTVIEW2");
    vf.vflags=0;
    vf.data=(char *)&v;
    
    /* init */
    assert_equal(Binit(p_ub, sizeof(buf)), EXSUCCEED);
    
    set_up_dummy_data(p_ub);
    
    /* load some UBF */
    assert_equal(Badd(p_ub, T_VIEW_FLD, (char *)&vf, 0), EXSUCCEED);

    /* short-to-view */
    assert_equal(CBchg(p_ub, T_VIEW_FLD, 0, (char *)buf_tmp, 0, BFLD_SHORT), EXFAIL);
    assert_equal(Berror, BEBADOP);
    do_dummy_data_test(p_ub);
    
    /* long-to-view */
    assert_equal(CBchg(p_ub, T_VIEW_FLD, 0, (char *)buf_tmp, 0, BFLD_LONG), EXFAIL);
    assert_equal(Berror, BEBADOP);
    do_dummy_data_test(p_ub);
    
    /* char-to-view */
    assert_equal(CBchg(p_ub, T_VIEW_FLD, 0, (char *)buf_tmp, 0, BFLD_CHAR), EXFAIL);
    assert_equal(Berror, BEBADOP);
    do_dummy_data_test(p_ub);
    
    /* float-to-view */
    assert_equal(CBchg(p_ub, T_VIEW_FLD, 0, (char *)buf_tmp, 0, BFLD_FLOAT), EXFAIL);
    assert_equal(Berror, BEBADOP);
    do_dummy_data_test(p_ub);
    
    /* double-to-view */
    assert_equal(CBchg(p_ub, T_VIEW_FLD, 0, (char *)buf_tmp, 0, BFLD_DOUBLE), EXFAIL);
    assert_equal(Berror, BEBADOP);
    do_dummy_data_test(p_ub);
    
    /* string-to-view */
    assert_equal(CBchg(p_ub, T_VIEW_FLD, 0, (char *)buf_tmp, 0, BFLD_STRING), EXFAIL);
    assert_equal(Berror, BEBADOP);
    do_dummy_data_test(p_ub);
    
    /* carray-to-view */
    assert_equal(CBchg(p_ub, T_VIEW_FLD, 0, (char *)buf_tmp, 0, BFLD_CARRAY), EXFAIL);
    assert_equal(Berror, BEBADOP);
    do_dummy_data_test(p_ub);
    
    /* ptr-to-view */
    assert_equal(CBchg(p_ub, T_VIEW_FLD, 0, (char *)buf_tmp, 0, BFLD_PTR), EXFAIL);
    assert_equal(Berror, BEBADOP);
    do_dummy_data_test(p_ub);
    
    /* ubf-to-view */
    assert_equal(CBchg(p_ub, T_VIEW_FLD, 0, (char *)buf_tmp, 0, BFLD_UBF), EXFAIL);
    assert_equal(Berror, BEBADOP);
    do_dummy_data_test(p_ub);
}

/**
 * Test FML wrapper
 * i.e. FLD_PTR logic is different for FML.
 */
Ensure(test_Fchg)
{
    char buf1[56000];
    char buf_ptr1[100];
    char buf_ptr2[100];
    char *ptr_get = NULL;
    long l;
    BFLDLEN len;
    UBFH *p_ub1 = (UBFH *)buf1;
    
    memset(buf1, 0, sizeof(buf1));
    assert_equal(Binit(p_ub1, sizeof(buf1)), EXSUCCEED);
    
    /* Load ptr */
    assert_equal(Fchg(p_ub1, T_PTR_FLD, 0, buf_ptr1, 0), EXSUCCEED);
    assert_equal(Fchg(p_ub1, T_PTR_FLD, 1, buf_ptr1, 0), EXSUCCEED);
    
    /* test the occ field */
    assert_equal(Fchg32(p_ub1, T_PTR_FLD, 1, buf_ptr2, 0), EXSUCCEED);

    l=999;
    assert_equal(Fchg(p_ub1, T_LONG_FLD, 0, (char *)&l, 0), EXSUCCEED);
    l=888;
    assert_equal(Fchg(p_ub1, T_LONG_FLD, 1, (char *)&l, 0), EXSUCCEED);
    
    assert_equal(Fchg(p_ub1, T_STRING_FLD, 0, "HELLO 1", 0), EXSUCCEED);
    assert_equal(Fchg32(p_ub1, T_STRING_FLD, 1, "HELLO 2", 0), EXSUCCEED);
    
    assert_equal(Fchg(p_ub1, T_CARRAY_FLD, 0, "ABCD", 1), EXSUCCEED);
    assert_equal(Fchg32(p_ub1, T_CARRAY_FLD, 1, "CDE", 1), EXSUCCEED);
    
    /* Validate the data.., ptr */
    assert_equal(Bget(p_ub1, T_PTR_FLD, 0, (char *)&ptr_get, 0), EXSUCCEED);
    assert_equal(ptr_get, buf_ptr1);
    
    assert_equal(Bget(p_ub1, T_PTR_FLD, 1, (char *)&ptr_get, 0), EXSUCCEED);
    assert_equal(ptr_get, buf_ptr2);
    
    /* validate string */
    assert_equal(Bget(p_ub1, T_STRING_FLD, 0, buf_ptr1, 0), EXSUCCEED);
    assert_string_equal(buf_ptr1, "HELLO 1");
    
    assert_equal(Bget(p_ub1, T_STRING_FLD, 1, buf_ptr1, 0), EXSUCCEED);
    assert_string_equal(buf_ptr1, "HELLO 2");
    
    assert_equal(CBget(p_ub1, T_CARRAY_FLD, 0, buf_ptr1, 0, BFLD_STRING), EXSUCCEED);
    assert_string_equal(buf_ptr1, "A");
    
    assert_equal(CBget(p_ub1, T_CARRAY_FLD, 1, buf_ptr1, 0, BFLD_STRING), EXSUCCEED);
    assert_string_equal(buf_ptr1, "C");
    
    /* validate long */
    
    assert_equal(Bget(p_ub1, T_LONG_FLD, 0, (char *)&l, 0), EXSUCCEED);
    assert_equal(l, 999);
    
    assert_equal(Bget(p_ub1, T_LONG_FLD, 1, (char *)&l, 0), EXSUCCEED);
    assert_equal(l, 888);
    
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
    add_test(suite, test_Bchg_ptr_org);
    add_test(suite, test_Bchg_ubf_org);
    add_test(suite, test_Bchg_view_org);
    add_test(suite, test_Fchg);
            
    return suite;
}

/* vim: set ts=4 sw=4 et smartindent: */
