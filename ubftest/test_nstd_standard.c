/**
 * @brief Standard header tests
 *
 * @file test_nstd_standard.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
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
#include <ubf.h>
#include <ndrstandard.h>
#include <string.h>
#include <ndebug.h>
#include <exbase64.h>
#include "test.fd.h"
#include "ubfunit1.h"
#include "xatmi.h"

/**
 * Test NDRX_STRCPY_S macro
 */
Ensure(test_nstd_ndrx_strcpy_s)
{
    char tmp[16] = {EXEOS};
    
    NDRX_STRCAT_S(tmp, sizeof(tmp), "HELLO");
    assert_string_equal(tmp, "HELLO");
    
    NDRX_STRCAT_S(tmp, sizeof(tmp), " WORLD");
    assert_string_equal(tmp, "HELLO WORLD");
    
    NDRX_STRCAT_S(tmp, sizeof(tmp), " THIS");
    /* last goes as EOS... */
    assert_string_equal(tmp, "HELLO WORLD THI");
}

/**
 * Test NDRX_ASPRINTF
 */
Ensure(test_nstd_ndrx_asprintf)
{
    char *p = (char *)123;
    long len;
    
    NDRX_ASPRINTF(&p, &len, "Hello %d %s", 1, "world");
    
    assert_not_equal(p, NULL);
    assert_not_equal(p, 123);
    assert_equal(len, 13);
    
    assert_string_equal(p, "Hello 1 world");
    
    NDRX_FREE(p);
    
}

/**
 * Standard header tests
 * @return
 */
TestSuite *ubf_nstd_standard(void)
{
    TestSuite *suite = create_test_suite();

    add_test(suite, test_nstd_ndrx_strcpy_s);
    add_test(suite, test_nstd_ndrx_asprintf);
            
    return suite;
}
/* vim: set ts=4 sw=4 et smartindent: */
