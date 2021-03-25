/**
 * @brief Atomic add
 *
 * @file test_nstd_atomicadd.c
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
#include <ndebug.h>
#include <exbase64.h>
#include <nstdutil.h>
#include "test.fd.h"
#include "ubfunit1.h"
#include "xatmi.h"
#include "nstopwatch.h"
#include <exatomic.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

/**
 * This shall be last (as macroses are being disabled)
 */
Ensure(test_nstd_atomicadd)
{
    int i=0;
        
    NDRX_ATOMIC_ADD(&i, 1);
    assert_equal(i, 1);
    
#undef EX_HAVE_STDATOMIC
    
    NDRX_ATOMIC_ADD(&i, 1);
    assert_equal(i, 2);
    
#undef EX_HAVE_SYNCFETCHADD
    
    NDRX_ATOMIC_ADD(&i, 1);
    assert_equal(i, 3);
    
}

/**
 * Atom add tests (needs new file due to macroses being disabled)
 * @return
 */
TestSuite *ubf_nstd_atomicadd(void)
{
    TestSuite *suite = create_test_suite();

    add_test(suite, test_nstd_atomicadd);
    
    return suite;
}
/* vim: set ts=4 sw=4 et smartindent: */
