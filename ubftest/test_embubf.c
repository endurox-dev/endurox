/**
 * Perform tests on embedded UBF buffer
 * 
 * @file test_embubf.c
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
#include <ndebug.h>
#include <string.h>
#include "test.fd.h"
#include "ubfunit1.h"

/**
 * Perform tests for buffer add
 */
Ensure(test_ubf_Badd)
{
    char buf1[56000];
    UBFH *p_ub1 = (UBFH *)buf1;
    
    char buf2[56000];
    UBFH *p_ub2 = (UBFH *)buf2;
    
    char buf3[56000];
    UBFH *p_ub3 = (UBFH *)buf3;

    char tmp_str[1024];
    
    memset(buf1, 0, sizeof(buf1));
    memset(buf2, 0, sizeof(buf2));
    memset(buf3, 0, sizeof(buf3));
    
    assert_equal(Binit(p_ub1, sizeof(buf1)), EXSUCCEED);
    assert_equal(Binit(p_ub2, sizeof(buf2)), EXSUCCEED);
    assert_equal(Binit(p_ub3, sizeof(buf3)), EXSUCCEED);
    
    
    assert_equal(Badd(p_ub1, T_STRING_2_FLD, "HELLO_PARENT", 0), EXSUCCEED);
    assert_equal(Badd(p_ub2, T_STRING_3_FLD, "HELLO_CHILD", 0), EXSUCCEED);
    assert_equal(Badd(p_ub2, T_STRING_3_FLD, "HELLO_CHILD 2", 0), EXSUCCEED);
    assert_equal(Badd(p_ub2, T_STRING_3_FLD, "HELLO_CHILD 3", 0), EXSUCCEED);
    assert_equal(Badd(p_ub2, T_STRING_3_FLD, "HELLO_CHILD 4", 0), EXSUCCEED);
    assert_equal(Badd(p_ub1, T_STRING_4_FLD, "HELLO_PARENT 2", 0), EXSUCCEED);

    assert_equal(Badd(p_ub1, T_UBF_FLD, (char *)p_ub2, 0), EXSUCCEED);
    assert_equal(Badd(p_ub1, T_UBF_FLD, (char *)p_ub2, 0), EXSUCCEED);
    
    
    assert_equal(Badd(p_ub3, T_UBF_2_FLD, (char *)p_ub1, 0), EXSUCCEED);
    
    
    UBF_DUMP(log_error, "Buffer Dump", buf1, Bused(p_ub1));
        
    memset(buf2, 0, sizeof(buf2));

    assert_equal(Bget(p_ub1, T_UBF_FLD, 0, (char *)p_ub2, 0), EXSUCCEED);
    assert_equal(Bget(p_ub2, T_STRING_3_FLD, 0, tmp_str, 0), EXSUCCEED);
    assert_string_equal(tmp_str, "HELLO_CHILD");
    
    Bprint(p_ub3);
    
    tmp_str[0]=EXEOS;
    assert_equal(RBget (p_ub3, (int []){ T_UBF_2_FLD, 0, T_UBF_FLD, 1, T_STRING_3_FLD, 1, -1 }, tmp_str, NULL), EXSUCCEED);
    assert_string_equal(tmp_str, "HELLO_CHILD 2");
    
}

/**
 * Common suite entry
 * @return
 */
TestSuite *ubf_embubf_tests(void)
{
    TestSuite *suite = create_test_suite();

    add_test(suite, test_ubf_Badd);
    
    return suite;
}
/* vim: set ts=4 sw=4 et smartindent: */
