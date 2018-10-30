/**
 *
 * @file test_bproj.c
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
#include "ndebug.h"
#include <fdatatype.h>

void test_proj_data_1(UBFH *p_ub)
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
}

void load_proj_test_data(UBFH *p_ub)
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

/**
 * This simply reads all field and adds them to another buffer, then do compare
 */
Ensure(test_proj_simple)
{
    char fb[1400];
    UBFH *p_ub = (UBFH *)fb;
    char data_buf[100];
    BFLDLEN  len;

    BFLDID proj[] = {
        T_SHORT_FLD,
        T_LONG_FLD,
        T_CHAR_FLD,
        T_FLOAT_FLD,
        T_DOUBLE_FLD,
        T_STRING_FLD,
        T_CARRAY_FLD,
        BBADFLDID
    };
    /* Empty list - delete all */
    BFLDID proj_empty[] = {
        BBADFLDID
    };

    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);

    load_proj_test_data(p_ub);

    /* Projection delete */
    assert_equal(Bproj(p_ub, proj), EXSUCCEED);

    assert_equal(Bpres(p_ub, T_SHORT_FLD, 0), EXTRUE);
    assert_equal(Bpres(p_ub, T_LONG_FLD, 0), EXTRUE);
    assert_equal(Bpres(p_ub, T_CHAR_FLD, 0), EXTRUE);
    assert_equal(Bpres(p_ub, T_FLOAT_FLD, 0), EXTRUE);
    assert_equal(Bpres(p_ub, T_DOUBLE_FLD, 0), EXTRUE);
    assert_equal(Bpres(p_ub, T_STRING_FLD, 0), EXTRUE);
    assert_equal(Bpres(p_ub, T_CARRAY_FLD, 0), EXTRUE);

    assert_equal(Bpres(p_ub, T_SHORT_2_FLD, 0), EXFALSE);
    assert_equal(Bpres(p_ub, T_LONG_2_FLD, 0), EXFALSE);
    assert_equal(Bpres(p_ub, T_CHAR_2_FLD, 0), EXFALSE);
    assert_equal(Bpres(p_ub, T_FLOAT_2_FLD, 0), EXFALSE);
    assert_equal(Bpres(p_ub, T_DOUBLE_2_FLD, 0), EXFALSE);
    assert_equal(Bpres(p_ub, T_STRING_2_FLD, 0), EXFALSE);
    assert_equal(Bpres(p_ub, T_CARRAY_2_FLD, 0), EXFALSE);

    test_proj_data_1(p_ub);

    /* Now test complete delete */

    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);
    load_proj_test_data(p_ub);
    assert_equal(Bproj(p_ub, NULL), EXSUCCEED);

    assert_equal(Bpres(p_ub, T_SHORT_FLD, 0), EXFALSE);
    assert_equal(Bpres(p_ub, T_LONG_FLD, 0), EXFALSE);
    assert_equal(Bpres(p_ub, T_CHAR_FLD, 0), EXFALSE);
    assert_equal(Bpres(p_ub, T_FLOAT_FLD, 0), EXFALSE);
    assert_equal(Bpres(p_ub, T_DOUBLE_FLD, 0), EXFALSE);
    assert_equal(Bpres(p_ub, T_STRING_FLD, 0), EXFALSE);
    assert_equal(Bpres(p_ub, T_CARRAY_FLD, 0), EXFALSE);

    assert_equal(Bpres(p_ub, T_SHORT_2_FLD, 0), EXFALSE);
    assert_equal(Bpres(p_ub, T_LONG_2_FLD, 0), EXFALSE);
    assert_equal(Bpres(p_ub, T_CHAR_2_FLD, 0), EXFALSE);
    assert_equal(Bpres(p_ub, T_FLOAT_2_FLD, 0), EXFALSE);
    assert_equal(Bpres(p_ub, T_DOUBLE_2_FLD, 0), EXFALSE);
    assert_equal(Bpres(p_ub, T_STRING_2_FLD, 0), EXFALSE);
    assert_equal(Bpres(p_ub, T_CARRAY_2_FLD, 0), EXFALSE);

    /* Test the case when projection list is empty. */
    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);
    load_proj_test_data(p_ub);
    assert_equal(Bproj(p_ub, proj_empty), EXSUCCEED);
    assert_equal(Bpres(p_ub, T_SHORT_FLD, 0), EXFALSE);
    assert_equal(Bpres(p_ub, T_LONG_FLD, 0), EXFALSE);
    assert_equal(Bpres(p_ub, T_CHAR_FLD, 0), EXFALSE);
    assert_equal(Bpres(p_ub, T_FLOAT_FLD, 0), EXFALSE);
    assert_equal(Bpres(p_ub, T_DOUBLE_FLD, 0), EXFALSE);
    assert_equal(Bpres(p_ub, T_STRING_FLD, 0), EXFALSE);
    assert_equal(Bpres(p_ub, T_CARRAY_FLD, 0), EXFALSE);

    assert_equal(Bpres(p_ub, T_SHORT_2_FLD, 0), EXFALSE);
    assert_equal(Bpres(p_ub, T_LONG_2_FLD, 0), EXFALSE);
    assert_equal(Bpres(p_ub, T_CHAR_2_FLD, 0), EXFALSE);
    assert_equal(Bpres(p_ub, T_FLOAT_2_FLD, 0), EXFALSE);
    assert_equal(Bpres(p_ub, T_DOUBLE_2_FLD, 0), EXFALSE);
    assert_equal(Bpres(p_ub, T_STRING_2_FLD, 0), EXFALSE);
    assert_equal(Bpres(p_ub, T_CARRAY_2_FLD, 0), EXFALSE);


}

/**
 * This test projcpy function
 */
Ensure(test_projcpy)
{
    char fb_src[600];
    UBFH *p_ub_src = (UBFH *)fb_src;
    char fb_src2[2400];
    UBFH *p_ub_src2 = (UBFH *)fb_src2;
    
    char fb_dst[600];
    UBFH *p_ub_dst = (UBFH *)fb_dst;
    
    UBF_header_t *hsrc = (UBF_header_t *)p_ub_src;
    UBF_header_t *hdst = (UBF_header_t *)p_ub_dst;

    BFLDID proj[] = {
        T_SHORT_FLD,
        T_LONG_FLD,
        T_CHAR_FLD,
        T_FLOAT_FLD,
        T_DOUBLE_FLD,
        T_STRING_FLD,
        T_CARRAY_FLD,
        BBADFLDID
    };
    BFLDID proj_all[] = {
        T_SHORT_2_FLD,
        T_SHORT_2_FLD,
        T_LONG_2_FLD,
        T_CHAR_2_FLD,
        T_FLOAT_2_FLD,
        T_DOUBLE_2_FLD,
        T_STRING_2_FLD,
        T_CARRAY_2_FLD,
        T_SHORT_FLD,
        T_LONG_FLD,
        T_CHAR_FLD,
        T_FLOAT_FLD,
        T_DOUBLE_FLD,
        T_STRING_FLD,
        T_CARRAY_FLD,
        BBADFLDID
    };

    assert_equal(Binit(p_ub_src, sizeof(fb_src)), EXSUCCEED);
    assert_equal(Binit(p_ub_src2, sizeof(fb_src2)), EXSUCCEED);
    assert_equal(Binit(p_ub_dst, sizeof(fb_dst)), EXSUCCEED);

    load_proj_test_data(p_ub_src);
    /* Projcpy */
    assert_equal(Bprojcpy(p_ub_dst, p_ub_src, proj), EXSUCCEED);
    /* Projection */
    assert_equal(Bproj(p_ub_src, proj), EXSUCCEED);
    /* Compare the buffer to one what we get from proj */
    assert_equal(memcmp(p_ub_dst, p_ub_src, Bused(p_ub_dst)), EXSUCCEED);
    
    /* OK now test that all data have been copied! */
    load_proj_test_data(p_ub_src);
    assert_equal(Bprojcpy(p_ub_dst, p_ub_src, proj_all), EXSUCCEED);
    
    UBF_DUMP(log_debug, "Source buffer", p_ub_src, sizeof(fb_dst));
    
    UBF_LOG(log_debug, "hsrc dump short, 0: %ld", 0);
    UBF_LOG(log_debug, "hsrc dump long, 1: %ld", hsrc->cache_long_off);
    UBF_LOG(log_debug, "hsrc dump char, 2: %ld", hsrc->cache_char_off);
    UBF_LOG(log_debug, "hsrc dump float, 3: %ld", hsrc->cache_float_off);
    UBF_LOG(log_debug, "hsrc dump double, 4: %ld", hsrc->cache_double_off);
    UBF_LOG(log_debug, "hsrc dump string, 5: %ld", hsrc->cache_string_off);
    UBF_LOG(log_debug, "hsrc dump carray, 6: %ld", hsrc->cache_carray_off);
        
        
    assert_equal(memcmp(p_ub_dst, p_ub_src, Bused(p_ub_dst)), EXSUCCEED);
    UBF_DUMP(log_debug, "Dest buffer", p_ub_dst, sizeof(fb_dst));
    
        
    UBF_LOG(log_debug, "hdst dump short, 0: %ld", 0);
    UBF_LOG(log_debug, "hdst dump long, 1: %ld", hdst->cache_long_off);
    UBF_LOG(log_debug, "hdst dump char, 2: %ld", hdst->cache_char_off);
    UBF_LOG(log_debug, "hdst dump float, 3: %ld", hdst->cache_float_off);
    UBF_LOG(log_debug, "hdst dump double, 4: %ld", hdst->cache_double_off);
    UBF_LOG(log_debug, "hdst dump string, 5: %ld", hdst->cache_string_off);
    UBF_LOG(log_debug, "hdst dump carray, 6: %ld", hdst->cache_carray_off);
    

    /* Now test the spacing. We will put big string in src buffer,
     * but dest buffer will be shorter, so we must get error, that we do not have
     * a space!
     */
    load_proj_test_data(p_ub_src2);
    assert_equal(Bchg(p_ub_src2, T_STRING_FLD, 0, BIG_TEST_STRING, 0), EXSUCCEED);
    assert_equal(Bprojcpy(p_ub_dst, p_ub_src2, proj_all), EXFAIL);
    assert_equal(Berror, BNOSPACE);

}

/**
 * Test Bdelall function
 */
Ensure(test_Bdelall)
{
    char fb[1024];
    char buf[64];
    UBFH *p_ub = (UBFH *)fb;
    int len;
    BFLDID bfldid;
    BFLDOCC occ;
    int flds = 0;
    
    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);
    load_proj_test_data(p_ub);
    set_up_dummy_data(p_ub);

    assert_equal(Badd(p_ub, T_STRING_2_FLD, "TEST 2 data", 0), EXSUCCEED);
    /* Delete all string occurrances... */
    assert_equal(Bdelall(p_ub, T_STRING_2_FLD), EXSUCCEED);
    assert_equal(Bpres(p_ub, T_STRING_2_FLD, 0), EXFALSE);
    assert_equal(Bpres(p_ub, T_STRING_FLD, 0), EXTRUE);
    do_dummy_data_test(p_ub);

    /*----------------------------------------------------------*/
    /* Now do another test, as we are going to delete last element of the array */
    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);
    load_proj_test_data(p_ub);
    assert_equal(Bdelall(p_ub, T_CARRAY_2_FLD), EXSUCCEED);

    /* validate the post previous field is present */
    len = sizeof(buf);
    assert_equal(Bget(p_ub, T_CARRAY_FLD, 1, (char *)buf, &len), EXSUCCEED);
    assert_equal(strncmp(buf, "YARRAY1 TEST STRING DATA", len), 0);
    /*----------------------------------------------------------*/
    /* Check if we delete nothing, then this is error condition */
    assert_equal(Bdelall(p_ub, T_CARRAY_2_FLD), EXFAIL);
    assert_equal(Berror, BNOTPRES);
    
    
    /* Bug #148 */
    
    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);
    
    assert_equal(CBadd(p_ub, T_CHAR_FLD, "2", 0, BFLD_STRING), EXSUCCEED);
    assert_equal(CBadd(p_ub, T_STRING_FLD, "631419304311", 0, BFLD_STRING), EXSUCCEED);
    assert_equal(CBchg(p_ub, T_CHAR_FLD, 0, "Y", 0L, BFLD_STRING), EXSUCCEED);
    
    Bfprint(p_ub, stderr);
    
    assert_equal(Bdelall(p_ub, T_STRING_FLD), EXSUCCEED);
    
    NDRX_LOG(log_debug, "After T_STRING_FLD delete...");
    Bfprint(p_ub, stderr);
    
    assert_equal(CBchg(p_ub, T_CHAR_FLD, 0, "X", 0L, BFLD_STRING), EXSUCCEED);
    
    NDRX_LOG(log_debug, "After changed T_CHAR_FLD => occ: [%d]", 
            Boccur(p_ub, T_CHAR_FLD));
    
    Bfprint(p_ub, stderr);
    
    bfldid = BFIRSTFLDID;
    
    /* Only 1 field must be here! */
    while(1==Bnext(p_ub, &bfldid, &occ, NULL, NULL))
    {
        flds++;
    }
    
    assert_equal(flds, 1);
}
/**
 * Delete all items from 
 */
Ensure(test_Bdelete)
{
    char fb[1024];
    UBFH *p_ub = (UBFH *)fb;
    char fb2[1024];
    UBFH *p_ub2 = (UBFH *)fb2;

    BFLDID delete_fld2[] = {
        T_SHORT_2_FLD,
        T_SHORT_2_FLD,
        T_LONG_2_FLD,
        T_CHAR_2_FLD,
        T_FLOAT_2_FLD,
        T_DOUBLE_2_FLD,
        T_STRING_2_FLD,
        T_CARRAY_2_FLD,
        BBADFLDID
    };

    BFLDID delete_all[] = {
        T_SHORT_FLD,
        T_LONG_FLD,
        T_CHAR_FLD,
        T_FLOAT_FLD,
        T_DOUBLE_FLD,
        T_STRING_FLD,
        T_CARRAY_FLD,
        T_SHORT_2_FLD,
        T_SHORT_2_FLD,
        T_LONG_2_FLD,
        T_CHAR_2_FLD,
        T_FLOAT_2_FLD,
        T_DOUBLE_2_FLD,
        T_STRING_2_FLD,
        T_CARRAY_2_FLD,
        BBADFLDID
    };


    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);
    load_proj_test_data(p_ub);
    assert_equal(Bdelete(p_ub, delete_fld2), EXSUCCEED);
    test_proj_data_1(p_ub);
    /*----------------------------------------------------------*/
    /* Now check the error, that nothing have been deleted */
    assert_equal(Bdelete(p_ub, delete_fld2), EXFAIL);
    assert_equal(Berror, BNOTPRES);
    /*----------------------------------------------------------*/
    /* Ok now we are about to delete all items */
    assert_equal(Binit(p_ub2, sizeof(fb2)), EXSUCCEED);
    assert_equal(Bdelete(p_ub, delete_all), EXSUCCEED);
            
    assert_equal(memcmp(p_ub, p_ub2, Bused(p_ub)), EXSUCCEED);
    
}

TestSuite *ubf_fproj_tests(void)
{
    TestSuite *suite = create_test_suite();

    add_test(suite, test_proj_simple);
    add_test(suite, test_projcpy);
    add_test(suite, test_Bdelall);
    add_test(suite, test_Bdelete);
    
    return suite;
}

/* vim: set ts=4 sw=4 et smartindent: */
