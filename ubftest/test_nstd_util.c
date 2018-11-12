/**
 * @brief Standard utility library tests
 *
 * @file test_nstd_util.c
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
#include <ndebug.h>
#include <exbase64.h>
#include <nstdutil.h>
#include "test.fd.h"
#include "ubfunit1.h"
#include "xatmi.h"

/**
 * Enduro/X version of strsep...
 */
Ensure(test_nstd_strsep)
{
    char test_str[] = "HELLO//WORLD";
    char *p;
    
    p = test_str;
    
    assert_string_equal(ndrx_strsep(&p, "/"), "HELLO");
    
    assert_string_equal(ndrx_strsep(&p, "/"), "");
    assert_string_equal(ndrx_strsep(&p, "/"), "WORLD");
    assert_true(NULL==ndrx_strsep(&p, "/"));
    
}

/**
 * Standard library tests
 * @return
 */
TestSuite *ubf_nstd_util(void)
{
    TestSuite *suite = create_test_suite();

    add_test(suite, test_nstd_strsep);
            
    return suite;
}
/* vim: set ts=4 sw=4 et smartindent: */
