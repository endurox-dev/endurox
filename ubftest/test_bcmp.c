/**
 * @brief Test Buffer compare routines (Bcmp and Bsubset)
 *
 * @file test_bcmp.c
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

exprivate char *M_some_ptr1 = "HELLOptr";
exprivate char *M_some_ptr2 = "HELLOptr";

/**
 * Basic preparation before the test
 */
Ensure(cmp_basic_setup1)
{
    /* shared load */
    load_field_table(); 
}


/**
 * Load data1
 */
exprivate void load1(UBFH *p_ub, BFLDOCC occ)
{
    short s = 88;
    long l = -1021;
    char c = 'c';
    float f = 17.31;
    double d = 12312.1111;
    char carr[] = "CARRAY1 TEST STRING DATA";
    char tmp[1024];
    UBFH *p_tmp = (UBFH *)tmp;
    struct UBTESTVIEW2 v2;
    BVIEWFLD vf2;
    
    BFLDLEN len = strlen(carr);
    
    assert_equal(Bchg(p_ub, T_SHORT_FLD, occ, (char *)&s, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_LONG_FLD, occ, (char *)&l, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_CHAR_FLD, occ, (char *)&c, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_FLOAT_FLD, occ, (char *)&f, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_DOUBLE_FLD, occ, (char *)&d, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_STRING_FLD, occ, (char *)"TEST STR VAL", 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_CARRAY_FLD, occ, (char *)carr, len), EXSUCCEED);

    /* Load equal UBF occ 0*/
    assert_equal(Binit(p_tmp, sizeof(tmp)), EXSUCCEED);
    assert_equal(Bchg(p_tmp, T_STRING_10_FLD, 0, "HELO", 0L), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_UBF_FLD, occ, (char *)p_tmp, len), EXSUCCEED);
    /* Load equal PTR occ 0 */
    assert_equal(Bchg(p_ub, T_PTR_FLD, occ, (char *)&M_some_ptr1, len), EXSUCCEED);
    /* Load equal VIEW occ 0*/
    memset(&v2, 0, sizeof(v2));
    v2.tlong1=1000;
    vf2.data=(char *)&v2;
    vf2.vflags=0;
    NDRX_STRCPY_SAFE(vf2.vname, "UBTESTVIEW2");
    assert_equal(Bchg(p_ub, T_VIEW_FLD, occ, (char *)&vf2, len), EXSUCCEED);
    
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
    char tmp[1024];
    UBFH *p_tmp = (UBFH *)tmp;
    struct UBTESTVIEW2 v2;
    BVIEWFLD vf2;
    
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
    assert_equal(Bchg(p_ub, T_STRING_FLD, 0, (char *)"XEST STR VAL", 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_CARRAY_FLD, 0, (char *)carr, len), EXSUCCEED);
    
    
    /* Load equal UBF occ 0*/
    assert_equal(Binit(p_tmp, sizeof(tmp)), EXSUCCEED);
    assert_equal(Bchg(p_tmp, T_STRING_10_FLD, 0, "EHLO", 0L), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_UBF_FLD, 0, (char *)p_tmp, len), EXSUCCEED);
    /* Load equal PTR occ 0 */
    assert_equal(Bchg(p_ub, T_PTR_FLD, 0, (char *)&M_some_ptr2, len), EXSUCCEED);
    
    /* Load equal VIEW occ 0*/
    memset(&v2, 0, sizeof(v2));
    v2.tlong1=2000;
    vf2.data=(char *)&v2;
    vf2.vflags=0;
    NDRX_STRCPY_SAFE(vf2.vname, "UBTESTVIEW2");
    assert_equal(Bchg(p_ub, T_VIEW_FLD, 0, (char *)&vf2, len), EXSUCCEED);
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
    
    load1(p_ub, 0);
    load1(p_ub_2, 0);
    
    assert_equal(Bcmp(p_ub, p_ub_2), 0);
    
    
    assert_equal(Binit(p_ub_2, sizeof(fb_2)), EXSUCCEED);
    load1(p_ub_2, 1);
    
    /* ID of first is greater (due to missing occurrences) than second buffer */
    assert_equal(Bcmp(p_ub, p_ub_2), 1);
    
    /* now reverse check */
    assert_equal(Bcmp(p_ub_2, p_ub), -1);
    
    /* ok have some value differences */
    
    
    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);
    assert_equal(Binit(p_ub_2, sizeof(fb_2)), EXSUCCEED);
    
    load3(p_ub);
    load1(p_ub_2, 0);
    
    /* load3 short is bigger than load1 */
    assert_equal(Bcmp(p_ub, p_ub_2), 1);
    
    /* reverse */
    assert_equal(Bcmp(p_ub_2, p_ub), -1);
    
    
    /* remove some fields from buffer, to make it look shorter... (last carray) */
    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);
    assert_equal(Binit(p_ub_2, sizeof(fb_2)), EXSUCCEED);
    
    load1(p_ub, 0);
    load1(p_ub_2, 0);
    
    
    assert_equal(Bdel(p_ub, T_VIEW_FLD, 0), EXSUCCEED);
    
    /* buf2 have more fields */
    assert_equal(Bcmp(p_ub, p_ub_2), -1);
    
    /* reverse */
    assert_equal(Bcmp(p_ub_2, p_ub), 1);
    
    
    /* lets check some other data types too, so that we run through the new
     * comparator functions
     */
    
    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);
    assert_equal(Binit(p_ub_2, sizeof(fb_2)), EXSUCCEED);
    
    load1(p_ub, 0);
    load3(p_ub_2);

    
    /* short we already tested, now start with long */
    
    assert_equal(Bdel(p_ub, T_SHORT_FLD, 0), EXSUCCEED);
    assert_equal(Bdel(p_ub_2, T_SHORT_FLD, 0), EXSUCCEED);
    
    /* -1021 < 212 */
    assert_equal(Bcmp(p_ub, p_ub_2), -1);
    assert_equal(Bcmp(p_ub_2, p_ub), 1);
    
    
    /* char test... */
    assert_equal(Bdel(p_ub, T_LONG_FLD, 0), EXSUCCEED);
    assert_equal(Bdel(p_ub_2, T_LONG_FLD, 0), EXSUCCEED);
    
    assert_equal(Bcmp(p_ub, p_ub_2), 1);
    assert_equal(Bcmp(p_ub_2, p_ub), -1);
    
    
    /* float test... */
    assert_equal(Bdel(p_ub, T_CHAR_FLD, 0), EXSUCCEED);
    assert_equal(Bdel(p_ub_2, T_CHAR_FLD, 0), EXSUCCEED);
    
    assert_equal(Bcmp(p_ub, p_ub_2), -1);
    assert_equal(Berror, 0);
    assert_equal(Bcmp(p_ub_2, p_ub), 1);
    
    /* double test... */
    assert_equal(Bdel(p_ub, T_FLOAT_FLD, 0), EXSUCCEED);
    assert_equal(Bdel(p_ub_2, T_FLOAT_FLD, 0), EXSUCCEED);
    
    assert_equal(Bcmp(p_ub, p_ub_2), -1);
    assert_equal(Bcmp(p_ub_2, p_ub), 1);
    
    /* string test... */
    assert_equal(Bdel(p_ub, T_DOUBLE_FLD, 0), EXSUCCEED);
    assert_equal(Bdel(p_ub_2, T_DOUBLE_FLD, 0), EXSUCCEED);
    
    assert_equal(Bcmp(p_ub, p_ub_2), -1);
    assert_equal(Bcmp(p_ub_2, p_ub), 1);
    
    /* carray test... */
    assert_equal(Bdel(p_ub, T_STRING_FLD, 0), EXSUCCEED);
    assert_equal(Bdel(p_ub_2, T_STRING_FLD, 0), EXSUCCEED);
    
    assert_equal(Bcmp(p_ub, p_ub_2), -1);
    assert_equal(Bcmp(p_ub_2, p_ub), 1);
    
    
    /* check the length of carray */
    assert_equal(CBchg(p_ub_2, T_CARRAY_FLD, 0, "HELLO", 0, BFLD_STRING), EXSUCCEED);
    
    assert_equal(Bcmp(p_ub, p_ub_2), 1);
    assert_equal(Bcmp(p_ub_2, p_ub), -1);
    
    /* test errors */
    
    assert_equal(Bcmp(NULL, p_ub_2), -2);
    assert_equal(Berror, BEINVAL);
    
    assert_equal(Bcmp(p_ub, NULL), -2);
    assert_equal(Berror, BEINVAL);
    
    
    memset(fb, 0, sizeof(fb));
    
    assert_equal(Bcmp(p_ub, p_ub_2), -2);
    assert_equal(Berror, BNOTFLD);
    
    assert_equal(Bcmp(p_ub_2, p_ub), -2);
    assert_equal(Berror, BNOTFLD);
    
    
}

/**
 * This simply reads all field and adds them to another buffer, then do compare
 */
Ensure(test_Bsubset)
{
    char fb[1024];
    char fb_2[2048];
    UBFH *p_ub = (UBFH *)fb;
    UBFH *p_ub_2 = (UBFH *)fb_2;
    
    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);
    assert_equal(Binit(p_ub_2, sizeof(fb_2)), EXSUCCEED);
    
    load1(p_ub, 0);
    load1(p_ub_2, 0);
    
    assert_equal(Bsubset(p_ub, p_ub_2), EXTRUE);
    
    
    assert_equal(Binit(p_ub_2, sizeof(fb_2)), EXSUCCEED);
    load1(p_ub_2, 1);
    
    /* ID of first is greater (due to missing occurrences) than second buffer */
    assert_equal(Bsubset(p_ub, p_ub_2), EXFALSE);
    
    /* now reverse check */
    assert_equal(Bsubset(p_ub_2, p_ub), EXFALSE);
    
    
    /* have a real subset */
    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);
    assert_equal(Binit(p_ub_2, sizeof(fb_2)), EXSUCCEED);
    
    load1(p_ub, 0);
    load1(p_ub_2, 0);
    
    assert_equal(Bdel(p_ub_2, T_CHAR_FLD, 0), EXSUCCEED);
    assert_equal(Bdel(p_ub_2, T_FLOAT_FLD, 0), EXSUCCEED);
    
    
    assert_equal(Bsubset(p_ub, p_ub_2), EXTRUE);
    assert_equal(Bsubset(p_ub_2, p_ub), EXFALSE);
    assert_equal(Berror, 0);
    
    /* test some errors */
    
    assert_equal(Bsubset(NULL, p_ub_2), EXFAIL);
    assert_equal(Berror, BEINVAL);
    
    assert_equal(Bsubset(p_ub, NULL), EXFAIL);
    assert_equal(Berror, BEINVAL);
    
    memset(fb, 0, sizeof(fb));
    
    assert_equal(Bsubset(p_ub, p_ub_2), EXFAIL);
    assert_equal(Berror, BNOTFLD);
    
    assert_equal(Bsubset(p_ub_2, p_ub), EXFAIL);
    assert_equal(Berror, BNOTFLD);
    
    
}

/**
 * Compare ptrs values
 */
Ensure(test_Bcmp_ptr)
{
    char fb[1024];
    char fb_2[2048];
    long ptr1;
    long ptr2;
    UBFH *p_ub = (UBFH *)fb;
    UBFH *p_ub_2 = (UBFH *)fb_2;
    
    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);
    assert_equal(Binit(p_ub_2, sizeof(fb_2)), EXSUCCEED);
    
    ptr1=1;
    ptr2=2;
    
    assert_equal(CBchg(p_ub, T_PTR_FLD, 0, (char *)&ptr1, 0L, BFLD_LONG), EXSUCCEED);
    assert_equal(CBchg(p_ub_2, T_PTR_FLD, 0, (char *)&ptr2, 0L, BFLD_LONG), EXSUCCEED);
    
    assert_equal(Bcmp(p_ub, p_ub_2), -1);
    assert_equal(Bcmp(p_ub_2, p_ub), 1);
    assert_equal(Bcmp(p_ub, p_ub), 0);
    
}

/**
 * Compare view in UBF values
 */
Ensure(test_Bcmp_view)
{
    char fb[2048];
    char fb_2[2048];
    struct UBTESTVIEW1 v1;
    BVIEWFLD vf1;
    
    struct UBTESTVIEW1 v2;
    BVIEWFLD vf2;
    
    UBFH *p_ub = (UBFH *)fb;
    UBFH *p_ub_2 = (UBFH *)fb_2;
    
    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);
    assert_equal(Binit(p_ub_2, sizeof(fb_2)), EXSUCCEED);
    
    memset(&v1, 0, sizeof(v1));
    memset(&v2, 0, sizeof(v2));
    assert_equal(Bvsinit((char *)&v1, "UBTESTVIEW1"), EXSUCCEED);
    assert_equal(Bvsinit((char *)&v2, "UBTESTVIEW1"), EXSUCCEED);
    
    vf1.data=(char *)&v1;
    vf1.vflags=0;
    NDRX_STRCPY_SAFE(vf1.vname, "UBTESTVIEW1");
    assert_equal(Bchg(p_ub, T_VIEW_3_FLD, 0, (char *)&vf1, 0L), EXSUCCEED);
    
    vf2.data=(char *)&v2;
    vf2.vflags=0;
    NDRX_STRCPY_SAFE(vf2.vname, "UBTESTVIEW1");
    assert_equal(Bchg(p_ub_2, T_VIEW_3_FLD, 0, (char *)&vf2, 0L), EXSUCCEED);
    
    /* must match */
    assert_equal(Bcmp(p_ub, p_ub_2), 0);
    
    /* change some view values... in the view, lets say null value 
     * as count is 0, this shall not be checked and still we shall match...
     */
    NDRX_STRCPY_SAFE(v1.tcarray3[4], "HELLO");
    assert_equal(Bchg(p_ub, T_VIEW_3_FLD, 0, (char *)&vf1, 0L), EXSUCCEED);
    assert_equal(Bcmp(p_ub, p_ub_2), 0);
    
    /* no change some non counted field... */
    NDRX_STRCPY_SAFE(v1.tcarray1, "AAAA");
    NDRX_STRCPY_SAFE(v2.tcarray1, "BBBB");
    
    assert_equal(Bchg(p_ub, T_VIEW_3_FLD, 0, (char *)&vf1, 0L), EXSUCCEED);
    assert_equal(Bchg(p_ub_2, T_VIEW_3_FLD, 0, (char *)&vf2, 0L), EXSUCCEED);
    
    assert_equal(Bcmp(p_ub, p_ub_2), -1);
    assert_equal(Bcmp(p_ub_2, p_ub), 1);
    
}

/**
 * Compare sub-ubf buffers.
 */
Ensure(test_Bcmp_ubf)
{
    char fb[1024];
    char fb_2[2048];
    char fb_tmp[2048];
    
    
    UBFH *p_ub = (UBFH *)fb;
    UBFH *p_ub_2 = (UBFH *)fb_2;
    UBFH *p_ub_tmp = (UBFH *)fb_tmp;
    
    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);
    assert_equal(Binit(p_ub_2, sizeof(fb_2)), EXSUCCEED);
    assert_equal(Binit(p_ub_tmp, sizeof(fb_tmp)), EXSUCCEED);
    
    /* setup the temp buffer.. */
    
    load1(p_ub_tmp, 0);
    
    /* match test: */
    assert_equal(Bchg(p_ub, T_UBF_3_FLD, 0, (char *)p_ub_tmp, 0L), EXSUCCEED);
    assert_equal(Bchg(p_ub_2, T_UBF_3_FLD, 0, (char *)p_ub_tmp, 0L), EXSUCCEED);
    
    assert_equal(Bcmp(p_ub_2, p_ub), 0);
    
    /* change something in sub-ubf... */
    assert_equal(Binit(p_ub_tmp, sizeof(fb_tmp)), EXSUCCEED);
    load3(p_ub_tmp);
    assert_equal(Bchg(p_ub_2, T_UBF_3_FLD, 0, (char *)p_ub_tmp, 0L), EXSUCCEED);
    
    assert_equal(Bcmp(p_ub_2, p_ub), 1);
    
}

/**
 * Test UBF subset function at second level
 */
Ensure(test_Bsubset_ubf)
{
    char fb[2048];
    char fb_2[2048];
    char fb_tmp[2048];
    char fb_tmp2[2048];
    
    UBFH *p_ub = (UBFH *)fb;
    UBFH *p_ub_2 = (UBFH *)fb_2;
    UBFH *p_ub_tmp = (UBFH *)fb_tmp;
    UBFH *p_ub_tmp2 = (UBFH *)fb_tmp2;
    
    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);
    assert_equal(Binit(p_ub_2, sizeof(fb_2)), EXSUCCEED);
    assert_equal(Binit(p_ub_tmp, sizeof(fb_tmp)), EXSUCCEED);
    assert_equal(Binit(p_ub_tmp2, sizeof(fb_tmp2)), EXSUCCEED);
    
    /* setup the temp buffer.. */
    
    load1(p_ub_tmp, 0);
    load1(p_ub_tmp, 1);
    load1(p_ub_tmp, 2);
    
    /* that path is  T_UBF_FLD.T_UBF_2_FLD.<fields> */
    
    assert_equal(Bchg(p_ub_tmp2, T_UBF_2_FLD, 0, (char *)p_ub_tmp, 0L), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_UBF_FLD, 0, (char *)p_ub_tmp2, 0L), EXSUCCEED);
    
    /* for p_ub_2 there is two levels less */
    assert_equal(Binit(p_ub_tmp, sizeof(fb_tmp)), EXSUCCEED);
    assert_equal(Binit(p_ub_tmp2, sizeof(fb_tmp2)), EXSUCCEED);
    
    load1(p_ub_tmp, 0);
    load1(p_ub_tmp, 1);
    
    assert_equal(Bchg(p_ub_tmp2, T_UBF_2_FLD, 0, (char *)p_ub_tmp, 0L), EXSUCCEED);
    assert_equal(Bchg(p_ub_2, T_UBF_FLD, 0, (char *)p_ub_tmp2, 0L), EXSUCCEED);
    
    
    UBF_LOG(log_debug, "****Starting Bsubset ****");
    assert_equal(Bsubset(p_ub, p_ub_2), EXTRUE);
    UBF_LOG(log_debug, "****ENDING Bsubset ****");
    
    fprintf(stderr, "p_ub:\n");
    Bfprint(p_ub, stderr);
    
    fprintf(stderr, "p_ub2:\n");
    Bfprint(p_ub_2, stderr);
    
    assert_equal(Bsubset(p_ub_2, p_ub), EXFALSE);
    
}

TestSuite *ubf_bcmp_tests(void)
{
    TestSuite *suite = create_test_suite();

    set_setup(suite, cmp_basic_setup1);
    add_test(suite, test_Bcmp);
    add_test(suite, test_Bcmp_ptr);
    add_test(suite, test_Bcmp_view);   
    add_test(suite, test_Bcmp_ubf);
    
    add_test(suite, test_Bsubset);
    add_test(suite, test_Bsubset_ubf);
    
    return suite;
}
/* vim: set ts=4 sw=4 et smartindent: */
