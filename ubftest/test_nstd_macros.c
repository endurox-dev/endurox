/**
 * @brief Standard library tests - macros
 *
 * @file test_nstd_macros.c
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
/* vim: set ts=4 sw=4 et smartindent: */
