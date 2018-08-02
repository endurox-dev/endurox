/* 
 * @brief Standard library tests - macros
 *
 * @file test_nstd_macros.c
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
#include <cgreen/cgreen.h>
#include <ndrstandard.h>
#include <string.h>
#include <ndebug.h>
#include <excrypto.h>
#include "test.fd.h"
#include "ubfunit1.h"

/**
 * Test for Enduro/X standard C header macros
 */
Ensure(test_nstd_ndrstandard)
{
    char buf1[9];
    char buf2[5];
    
    
    NDRX_STRCPY_LAST_SAFE(buf1, "ABC", 8);
    assert_string_equal(buf1, "ABC");
    
    NDRX_STRCPY_LAST_SAFE(buf1, "1234567890ABCDEFGHIJK", 8);
    assert_string_equal(buf1, "DEFGHIJK");
    
    
    NDRX_STRCPY_LAST_SAFE(buf2, "ABCD", 8);
    assert_string_equal(buf2, "ABCD");
    
    NDRX_STRCPY_LAST_SAFE(buf2, "1234567890ABCDEFGHIJK", 8);
    
    /* because of safe copy... */
    assert_string_equal(buf2, "DEFG");
    
}

/**
 * C macros testings of standard library
 * @return
 */
TestSuite *test_nstd_macros(void)
{
    TestSuite *suite = create_test_suite();

    /*
    set_setup(suite, basic_setup1);
    set_teardown(suite, basic_teardown1);
    */
    
    add_test(suite, test_nstd_ndrstandard);
            
    return suite;
}
