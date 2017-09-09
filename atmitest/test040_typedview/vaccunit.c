/* 
** View access unit
**
** @file vaccunit.c
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
 * Test Bvget as short
 */
Ensure(test_Bvget_short)
{
    struct MYVIEW1 v;
    init_MYVIEW1(&v);
    short s;
    
    s = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort1", 0, (char *)&s, 0L, BFLD_SHORT, 0L), 
            EXSUCCEED);
    assert_equal(s, 15556);
    
    /* Invalid value, index out of bounds.. */
    s = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort1", 1, (char *)&s, 0L, BFLD_SHORT, 0L), 
            EXFAIL);
    assert_equal(Berror, BEINVAL);
    
    s = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort2", 0, (char *)&s, 0L, BFLD_SHORT, 0L), 
            EXSUCCEED);
    assert_equal(s, 9999);
    
    s = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort2", 1, (char *)&s, 0L, BFLD_SHORT, 0L), 
            EXSUCCEED);
    assert_equal(s, 8888);
    
    s = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort3", 0, (char *)&s, 0L, BFLD_SHORT, 0L), 
            EXSUCCEED);
    assert_equal(s, 7777);
    
    s = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort3", 1, (char *)&s, 0L, BFLD_SHORT, 0L), 
            EXSUCCEED);
    assert_equal(s, -7777);
    
    s = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort3", 2, (char *)&s, 0L, BFLD_SHORT, 0L), 
            EXSUCCEED);
    assert_equal(s, -7777);
    
    s = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort3", 2, (char *)&s, 0L, 
            BFLD_SHORT, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BNOTPRES);
    
    s = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort4", 0, (char *)&s, 0L, BFLD_SHORT, 0L), 
            EXSUCCEED);
    assert_equal(s, -10);
    
    
    s = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tlong1", 0, (char *)&s, 0L, BFLD_SHORT, 0L), 
            EXSUCCEED);
    assert_not_equal(s, 0);
    
    s = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint3", 0, (char *)&s, 0L, BFLD_SHORT, 0L), 
            EXSUCCEED);
    assert_equal(s, -100);
    
    /* non existent field: */
    s = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "xint3", 0, (char *)&s, 0L, BFLD_SHORT, 0L), 
            EXFAIL);
    assert_equal(Berror, BNOCNAME);
    
    /* Test field NULL */
    v.tint4[0] = -1;
    v.tint4[1] = -1;
    
    s = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint4", 0, (char *)&s, 0L, BFLD_SHORT, 0L), 
            EXSUCCEED);
    assert_equal(s, -1);
    
    s = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint4", 1, (char *)&s, 0L, BFLD_SHORT, 0L), 
            EXSUCCEED);
    assert_equal(s, -1);
    
    
    /* not pres because of NULL value */
    s = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint4", 0, (char *)&s, 0L, 
            BFLD_SHORT, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BNOTPRES);
    
    s = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint4", 1, (char *)&s, 0L, 
            BFLD_SHORT, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BNOTPRES);
    
    
    /* Test the ascii value to char */
    s = 0;
    NDRX_LOG(log_debug, "tchar1=%c", v.tchar1);
    
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar1", 0, (char *)&s, 0L, 
            BFLD_SHORT, /* TODO: BVACCESS_NOTNULL */ 0), 
            EXSUCCEED);
    assert_equal(s, 65);
    
}

/**
 * Very basic tests of the framework
 * @return
 */
TestSuite *vacc_tests(void) {
    TestSuite *suite = create_test_suite();
    
    set_setup(suite, basic_setup);
    set_teardown(suite, basic_teardown);

    /* init view test */
    add_test(suite, test_Bvget_short);
    
    return suite;
}
