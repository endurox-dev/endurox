/**
 *
 * @file test_mkfldhdr.c
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
#include "test.fd.h"
#include "ubfunit1.h"

/**
 * Calls scripts for checing mkfldhdr. Return code says
 * was OK or not OK.
 */
Ensure(test_mkfldhdr)
{
    assert_equal(system("./test_mkfldhdr_cmd.sh"), EXSUCCEED);
    assert_equal(system("./test_mkfldhdr_env.sh"), EXSUCCEED);
    assert_not_equal(system("./test_mkfldhdr_err_output.sh"), EXSUCCEED);
    assert_not_equal(system("./test_mkfldhdr_no_FLDTBLDIR.sh"), EXSUCCEED);
    assert_not_equal(system("./test_mkfldhdr_no_FIELDTBLS.sh"), EXSUCCEED);
    assert_not_equal(system("./test_mkfldhdr_syntax_err.sh"), EXSUCCEED);

}

TestSuite *ubf_mkfldhdr_tests(void)
{
    TestSuite *suite = create_test_suite();

    add_test(suite, test_mkfldhdr);

    return suite;
}


