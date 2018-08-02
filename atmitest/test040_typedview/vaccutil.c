/* 
 * @brief View access unit test, utility funcs
 *
 * @file vaccutil.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2018, Mavimax, Ltd. All Rights Reserved.
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
#include <unistd.h>
#include <cgreen/cgreen.h>
#include <ubf.h>
#include <ndrstandard.h>
#include <string.h>
#include "test.fd.h"
#include "ubfunit1.h"
#include "ndebug.h"
#include <fdatatype.h>

#include "test040.h"

/**
 * Basic preparation before the test
 */
exprivate void basic_setup(void)
{
    
}

exprivate void basic_teardown(void)
{
    
}

/**
 * Test Bvget as int
 */
Ensure(test_Bvsizeof)
{
    struct MYVIEW1 v;
    struct MYVIEW3 v3;
    
    assert_equal(Bvsizeof("MYVIEW1"), sizeof(v));
    assert_equal(Bvsizeof("MYVIEW3"), sizeof(v3));
}


/**
 * Test Bvoccur
 */
Ensure(test_Bvoccur)
{
    struct MYVIEW1 v;
    BFLDOCC maxocc;
    BFLDOCC realocc;
    long dim_size;
    int fldtype;
    
    init_MYVIEW1(&v);
    
    assert_equal(Bvoccur((char *)&v, "MYVIEW1", "tshort1", &maxocc, &realocc, &dim_size, &fldtype), 1);
    assert_equal(maxocc, 1);
    assert_equal(realocc, 1);
    assert_equal(dim_size, sizeof(short));
    assert_equal(fldtype, BFLD_SHORT);
    
    assert_equal(Bvoccur((char *)&v, "MYVIEW1", "tshort2", &maxocc, &realocc, &dim_size, NULL), 2);
    assert_equal(maxocc, 2);
    assert_equal(realocc, 2);
    
    assert_equal(Bvoccur((char *)&v, "MYVIEW1", "tshort3", &maxocc, &realocc, &dim_size, NULL), 2);
    assert_equal(maxocc, 3);
    assert_equal(realocc, 2);
    
    
    assert_equal(Bvoccur((char *)&v, "MYVIEW1", "tshort4", NULL, NULL, NULL, NULL), 1);
    
    v.tint2[1] = 0;
    assert_equal(Bvoccur((char *)&v, "MYVIEW1", "tint2", &maxocc, &realocc, &dim_size, &fldtype), 2);
    assert_equal(maxocc, 2);
    assert_equal(realocc, 1); /* due to last element is NULL */
    assert_equal(dim_size, sizeof(int));
    assert_equal(fldtype, BFLD_INT);
    
    assert_equal(Bvoccur((char *)&v, "MYVIEW1", "tchar2", &maxocc, &realocc, &dim_size, &fldtype), 5);
    assert_equal(maxocc, 5);
    assert_equal(realocc, 5);
    assert_equal(fldtype, BFLD_CHAR);
    
    
    v.tchar2[4]='A';
    v.tchar2[3]='A';
    
    assert_equal(Bvoccur((char *)&v, "MYVIEW1", "tchar2", &maxocc, &realocc, &dim_size, NULL), 5);
    assert_equal(maxocc, 5);
    assert_equal(realocc, 3);
    
    
    assert_equal(Bvoccur((char *)&v, "MYVIEW1", "tchar3", &maxocc, &realocc, &dim_size, NULL), 0);
    assert_equal(maxocc, 2);
    assert_equal(realocc, 0); /* because count is set to 0 */
    
    
    assert_equal(Bvoccur((char *)&v, "MYVIEW1", "tfloat1", &maxocc, &realocc, &dim_size, &fldtype), 4);
    assert_equal(maxocc, 4);
    assert_equal(realocc, 4);
    assert_equal(dim_size, sizeof(float));
    assert_equal(fldtype, BFLD_FLOAT);
    
    assert_equal(Bvoccur((char *)&v, "MYVIEW1", "tstring0", &maxocc, &realocc, &dim_size, &fldtype), 3);
    assert_equal(maxocc, 3);
    assert_equal(realocc, 3);
    assert_equal(dim_size, 18);
    assert_equal(fldtype, BFLD_STRING);
    
    assert_equal(Bvoccur((char *)&v, "MYVIEW1", "tstring2", &maxocc, &realocc, &dim_size, &fldtype), 2);
    assert_equal(maxocc, 3);
    assert_equal(realocc, 2);
    assert_equal(fldtype, BFLD_STRING);
    
}


/**
 * Test Bvsetoccur
 */
Ensure(test_Bvsetoccur)
{
    struct MYVIEW1 v;
    BFLDOCC maxocc;
    memset(&v, 0, sizeof(v));
    
    assert_equal(Bvsetoccur((char *)&v, "MYVIEW1", "tshort2", 0), 0);
    assert_equal(v.C_tshort2, 0);
   
    assert_equal(Bvsetoccur((char *)&v, "MYVIEW1", "tshort2", 1), 0);
    assert_equal(v.C_tshort2, 1);
   
    assert_equal(Bvsetoccur((char *)&v, "MYVIEW1", "tshort2", 2), 0);
    assert_equal(v.C_tshort2, 2);
    
    assert_equal(Bvsetoccur((char *)&v, "MYVIEW1", "tshort2", 3), -1);
    assert_equal(Berror, BEINVAL);
    
}


Ensure(test_Bvnext)
{
    struct MYVIEW1 v;
    Bvnext_state_t state;
    char cname[NDRX_VIEW_CNAME_LEN];
    int fldtype=99;
    BFLDOCC maxocc;
    long dim_size;
    
    init_MYVIEW1(&v);
    
    assert_equal(Bvnext (&state, "MYVIEW1", cname, &fldtype, &maxocc, &dim_size), 1);
    assert_string_equal(cname, "tshort1");
    assert_equal(fldtype, 0);
    assert_equal(maxocc, 1);
    assert_equal(dim_size, sizeof(short));
    
    
    assert_equal(Bvnext (&state, NULL, cname, &fldtype, &maxocc, &dim_size), 1);
    assert_string_equal(cname, "tshort2");
    assert_equal(fldtype, BFLD_SHORT);
    
    assert_equal(Bvnext (&state, NULL, cname, &fldtype, &maxocc, &dim_size), 1);
    assert_string_equal(cname, "tshort3");
    assert_equal(fldtype, BFLD_SHORT);
    
    assert_equal(Bvnext (&state, NULL, cname, &fldtype, &maxocc, &dim_size), 1);
    assert_string_equal(cname, "tshort4");
    assert_equal(fldtype, BFLD_SHORT);
    
    assert_equal(Bvnext (&state, NULL, cname, &fldtype, &maxocc, &dim_size), 1);
    assert_string_equal(cname, "tlong1");
    assert_equal(fldtype, BFLD_LONG);
    
    assert_equal(Bvnext (&state, NULL, cname, &fldtype, &maxocc, &dim_size), 1);
    assert_string_equal(cname, "tint2");
    assert_equal(fldtype, BFLD_INT);
    
    assert_equal(Bvnext (&state, NULL, cname, &fldtype, &maxocc, &dim_size), 1);
    assert_string_equal(cname, "tint3");
    assert_equal(fldtype, BFLD_INT);
    
    assert_equal(Bvnext (&state, NULL, cname, &fldtype, &maxocc, &dim_size), 1);
    assert_string_equal(cname, "tint4");
    assert_equal(fldtype, BFLD_INT);
    assert_equal(maxocc, 2);
    assert_equal(dim_size, sizeof(int));
    
    assert_equal(Bvnext (&state, NULL, cname, &fldtype, &maxocc, &dim_size), 1);
    assert_string_equal(cname, "tchar1");
    assert_equal(fldtype, BFLD_CHAR);
    
    
    assert_equal(Bvnext (&state, NULL, cname, &fldtype, &maxocc, &dim_size), 1);
    assert_string_equal(cname, "tchar2");
    assert_equal(fldtype, BFLD_CHAR);
    
    assert_equal(Bvnext (&state, NULL, cname, &fldtype, &maxocc, &dim_size), 1);
    assert_string_equal(cname, "tchar3");
    assert_equal(fldtype, BFLD_CHAR);
    
    assert_equal(Bvnext (&state, NULL, cname, &fldtype, &maxocc, &dim_size), 1);
    assert_string_equal(cname, "tfloat1");
    assert_equal(fldtype, BFLD_FLOAT);
    
    assert_equal(Bvnext (&state, NULL, cname, &fldtype, &maxocc, &dim_size), 1);
    assert_string_equal(cname, "tfloat2");
    assert_equal(fldtype, BFLD_FLOAT);
    
    assert_equal(Bvnext (&state, NULL, cname, &fldtype, &maxocc, &dim_size), 1);
    assert_string_equal(cname, "tfloat3");
    assert_equal(fldtype, BFLD_FLOAT);
    
    assert_equal(Bvnext (&state, NULL, cname, &fldtype, &maxocc, &dim_size), 1);
    assert_string_equal(cname, "tdouble1");
    assert_equal(fldtype, BFLD_DOUBLE);
    
    assert_equal(Bvnext (&state, NULL, cname, &fldtype, &maxocc, &dim_size), 1);
    assert_string_equal(cname, "tdouble2");
    assert_equal(fldtype, BFLD_DOUBLE);
    
    assert_equal(Bvnext (&state, NULL, cname, NULL, NULL, NULL), 1);
    assert_string_equal(cname, "tstring0");
    
    assert_equal(Bvnext (&state, NULL, cname, &fldtype, &maxocc, &dim_size), 1);
    assert_string_equal(cname, "tstring1");
    assert_equal(fldtype, BFLD_STRING);
    
    assert_equal(Bvnext (&state, NULL, cname, &fldtype, &maxocc, &dim_size), 1);
    assert_string_equal(cname, "tstring2");
    assert_equal(fldtype, BFLD_STRING);
    
    assert_equal(Bvnext (&state, NULL, cname, &fldtype, &maxocc, &dim_size), 1);
    assert_string_equal(cname, "tstring3");
    assert_equal(fldtype, BFLD_STRING);
    
    assert_equal(maxocc, 4);
    assert_equal(dim_size, 20);
    
    
    assert_equal(Bvnext (&state, NULL, cname, &fldtype, &maxocc, &dim_size), 1);
    assert_string_equal(cname, "tstring4");
    assert_equal(fldtype, BFLD_STRING);
    
    assert_equal(Bvnext (&state, NULL, cname, &fldtype, &maxocc, &dim_size), 1);
    assert_string_equal(cname, "tstring5");
    assert_equal(fldtype, BFLD_STRING);
    
    assert_equal(Bvnext (&state, NULL, cname, &fldtype, &maxocc, &dim_size), 1);
    assert_string_equal(cname, "tcarray1");
    assert_equal(fldtype, BFLD_CARRAY);
    
    assert_equal(Bvnext (&state, NULL, cname, &fldtype, &maxocc, &dim_size), 1);
    assert_string_equal(cname, "tcarray2");
    assert_equal(fldtype, BFLD_CARRAY);
    
    assert_equal(Bvnext (&state, NULL, cname, &fldtype, &maxocc, &dim_size), 1);
    assert_string_equal(cname, "tcarray3");
    assert_equal(fldtype, BFLD_CARRAY);
    
    assert_equal(Bvnext (&state, NULL, cname, &fldtype, &maxocc, &dim_size), 1);
    assert_string_equal(cname, "tcarray4");
    assert_equal(fldtype, BFLD_CARRAY);
    
    assert_equal(Bvnext (&state, NULL, cname, &fldtype, &maxocc, &dim_size), 1);
    assert_string_equal(cname, "tcarray5");
    assert_equal(fldtype, BFLD_CARRAY);
    
    assert_equal(Bvnext (&state, NULL, cname, &fldtype, &maxocc, &dim_size), 0);
    
    
    /* Test inval param */
    assert_equal(Bvnext (NULL, NULL, cname, &fldtype, &maxocc, &dim_size), -1);
    assert_equal(Berror, BEINVAL);
    
    /* Test inval param */
    state.v = NULL;
    state.vel = NULL;
    assert_equal(Bvnext (&state, NULL, cname, &fldtype, &maxocc, &dim_size), -1);
    assert_equal(Berror, BEINVAL);
    
    /* Test eof */
    state.v = (char *)0x1;
    state.vel = NULL;
    assert_equal(Bvnext (&state, NULL, cname, &fldtype, &maxocc, &dim_size), 0);
    
    
}

Ensure(test_Bvcpy)
{
    struct MYVIEW1 src;
    struct MYVIEW1 dst;
    init_MYVIEW1(&src);
    
    memset(&dst, 0, sizeof(dst));
    assert_equal(Bvcpy ((char *)&dst, (char *)&src, "MYVIEW1"), sizeof(struct MYVIEW1));
    assert_equal(memcmp(&dst, &src, sizeof(struct MYVIEW1)), 0);
    
}
/**
 * Very basic tests of the framework
 * @return
 */
TestSuite *vacc_util_tests(void)
{
    TestSuite *suite = create_test_suite();
    
    set_setup(suite, basic_setup);
    set_teardown(suite, basic_teardown);

    /* init view test */
    add_test(suite, test_Bvsizeof);
    add_test(suite, test_Bvoccur);
    add_test(suite, test_Bvsetoccur);
    add_test(suite, test_Bvnext);
    add_test(suite, test_Bvcpy);
            
    return suite;
}
