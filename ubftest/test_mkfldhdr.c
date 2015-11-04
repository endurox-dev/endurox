/* 
**
** @file test_mkfldhdr.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, ATR Baltic, SIA. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or ATR Baltic's license for commercial use.
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
** A commercial use license is available from ATR Baltic, SIA
** contact@atrbaltic.com
** -----------------------------------------------------------------------------
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
void test_mkfldhdr()
{
    assert_equal(system("./test_mkfldhdr_cmd.sh"), SUCCEED);
    assert_equal(system("./test_mkfldhdr_env.sh"), SUCCEED);
    assert_not_equal(system("./test_mkfldhdr_err_output.sh"), SUCCEED);
    assert_not_equal(system("./test_mkfldhdr_no_FLDTBLDIR.sh"), SUCCEED);
    assert_not_equal(system("./test_mkfldhdr_no_FIELDTBLS.sh"), SUCCEED);
    assert_not_equal(system("./test_mkfldhdr_syntax_err.sh"), SUCCEED);

}

TestSuite *ubf_mkfldhdr_tests(void)
{
    TestSuite *suite = create_test_suite();

    add_test(suite, test_mkfldhdr);

    return suite;
}


