/* 
** View unit tests.
**
** @file viewunit1.c
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

#include "t40.h"

/**
 * Basic preparation before the test
 */
void basic_setup(void)
{
    
}

void basic_teardown(void)
{
    
}


/**
 * Test NULL column
 */
Ensure(test_Bvnull)
{
    struct MYVIEW1 v;
    
    memset(&v, 0, sizeof(v));
    
    /***************************** SHORT TESTS *******************************/
    /* These fields shall not be NULL as NULL defined */
    assert_equal(Bvnull((char *)&v, "tshort2", 0, "MYVIEW1"), EXFALSE);
    assert_equal(Bvnull((char *)&v, "tshort2", 1, "MYVIEW1"), EXFALSE);
    
    /* Set to NULL */
    v.tshort2[1]=2001;
    assert_equal(Bvnull((char *)&v, "tshort2", 0, "MYVIEW1"), EXFALSE);
    assert_equal(Bvnull((char *)&v, "tshort2", 1, "MYVIEW1"), EXTRUE);
    
    /*  Set all to NULL */
    assert_equal(Bvselinit((char *)&v,"tshort2", "MYVIEW1"), EXSUCCEED);
    assert_equal(Bvnull((char *)&v, "tshort2", 0, "MYVIEW1"), EXTRUE);
    assert_equal(Bvnull((char *)&v, "tshort2", 1, "MYVIEW1"), EXTRUE);
    
    
    assert_equal(Bvnull((char *)&v, "tshort3", 0, "MYVIEW1"), EXTRUE);
    assert_equal(Bvnull((char *)&v, "tshort3", 1, "MYVIEW1"), EXTRUE);
    assert_equal(Bvnull((char *)&v, "tshort3", 2, "MYVIEW1"), EXTRUE);
    
    v.tshort3[0] = 1;
    v.tshort3[2] = 2;
    
    assert_equal(Bvnull((char *)&v, "tshort3", 0, "MYVIEW1"), EXFALSE);
    assert_equal(Bvnull((char *)&v, "tshort3", 1, "MYVIEW1"), EXTRUE);
    assert_equal(Bvnull((char *)&v, "tshort3", 2, "MYVIEW1"), EXFALSE);
    
    /* this is NONE, field set to 0, but as NONE is there it is not NULL */
    assert_equal(Bvnull((char *)&v, "tshort4", 0, "MYVIEW1"), EXFALSE);
    
    /***************************** LONG TESTS *******************************/
    
    assert_equal(Bvnull((char *)&v, "tlong1", 0, "MYVIEW1"), EXTRUE);
    
    /***************************** INT TESTS ********************************/
    
    assert_equal(Bvnull((char *)&v, "tint2", 0, "MYVIEW1"), EXTRUE);
    assert_equal(Bvnull((char *)&v, "tint2", 1, "MYVIEW1"), EXTRUE);
    assert_equal(Bvnull((char *)&v, "tint3", 0, "MYVIEW1"), EXFALSE);
    assert_equal(Bvselinit((char *)&v,"tint3", "MYVIEW1"), EXSUCCEED);
    assert_equal(Bvnull((char *)&v, "tint3", 0, "MYVIEW1"), EXTRUE);
    
    assert_equal(Bvnull((char *)&v, "tint4", 0, "MYVIEW1"), EXFALSE);
    assert_equal(Bvnull((char *)&v, "tint4", 1, "MYVIEW1"), EXFALSE);
    
    assert_equal(v.tint4[0], 0);
    assert_equal(v.tint4[1], 0);
    
    assert_equal(Bvselinit((char *)&v,"tint4", "MYVIEW1"), EXSUCCEED);
    
    assert_equal(Bvnull((char *)&v, "tint4", 0, "MYVIEW1"), EXTRUE);
    assert_equal(Bvnull((char *)&v, "tint4", 1, "MYVIEW1"), EXTRUE);
    
    /***************************** CHAR TESTS ********************************/
    
    assert_equal(Bvnull((char *)&v, "tchar1", 0, "MYVIEW1"), EXTRUE);
    v.tchar1 = 0x01;
    assert_equal(Bvnull((char *)&v, "tchar1", 0, "MYVIEW1"), EXFALSE);
    
}

/* TODO: Unit test for default values... */

/**
 * Very basic tests of the framework
 * @return
 */
TestSuite *view_tests() {
    TestSuite *suite = create_test_suite();
    
    set_setup(suite, basic_setup);
    set_teardown(suite, basic_teardown);

    

    /*  UBF init test */
    add_test(suite, test_Bvnull);
    
    return suite;
}

/*
 * Main test entry.
 */
int main(int argc, char** argv)
{    
    TestSuite *suite = create_test_suite();

    add_suite(suite, view_tests());
    

    if (argc > 1) {
        return run_single_test(suite,argv[1],create_text_reporter());
    }

    return run_test_suite(suite, create_text_reporter());
    
}
