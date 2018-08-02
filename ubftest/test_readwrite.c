/* 
 *
 * @file test_readwrite.c
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
 * Test - Test buffer read/write
 */
Ensure(test_readwrite)
{
    char fb[1024];
    UBFH *p_ub = (UBFH *)fb;
    char fb2[1024];
    UBFH *p_ub2 = (UBFH *)fb2;
    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);
    assert_equal(Binit(p_ub2, sizeof(fb2)), EXSUCCEED);

    /* Load test stuff */
    set_up_dummy_data(p_ub);
    open_test_temp("wb");

    assert_equal(Bwrite(p_ub, M_test_temp_file), EXSUCCEED);
    close_test_temp();

    /* Now read the stuff in second buffer */
    open_test_temp_for_read("rb");
    assert_equal(Bread(p_ub2, M_test_temp_file), EXSUCCEED);

    close_test_temp();
    remove_test_temp();

    /* Now compare the buffers */
    assert_equal(memcmp(p_ub, p_ub2, Bused(p_ub)), 0);
    /* run check on data */
    do_dummy_data_test(p_ub2);
}

/**
 * Dest buffer from read too short
 */
Ensure(test_readwrite_err_space)
{
    char fb[1024];
    UBFH *p_ub = (UBFH *)fb;
    char fb2[128];
    UBFH *p_ub2 = (UBFH *)fb2;
    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);
    assert_equal(Binit(p_ub2, sizeof(fb2)), EXSUCCEED);

    /* Load test stuff */
    set_up_dummy_data(p_ub);
    open_test_temp("wb");

    assert_equal(Bwrite(p_ub, M_test_temp_file), EXSUCCEED);
    close_test_temp();

    /* Now read the stuff in second buffer */
    open_test_temp_for_read("rb");
    assert_equal(Bread(p_ub2, M_test_temp_file), EXFAIL);
    assert_equal(Berror, BNOSPACE);

    close_test_temp();
    remove_test_temp();
}

/**
 * Test unix error on bad file descriptor
 */
Ensure(test_readwrite_invalid_descr)
{
    char fb[1024];
    UBFH *p_ub = (UBFH *)fb;
    char fb2[128];
    UBFH *p_ub2 = (UBFH *)fb2;
    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);
    assert_equal(Binit(p_ub2, sizeof(fb2)), EXSUCCEED);

    /* Load test stuff */
    set_up_dummy_data(p_ub);
    open_test_temp("r");

    assert_equal(Bwrite(p_ub, M_test_temp_file), EXFAIL);
    assert_equal(Berror, BEUNIX);
    close_test_temp();

    /* Now read the stuff in second buffer */
    open_test_temp_for_read("w");
    assert_equal(Bread(p_ub2, M_test_temp_file), EXFAIL);
    assert_equal(Berror, BEUNIX);

    close_test_temp();
    remove_test_temp();
}

TestSuite *ubf_readwrite_tests(void)
{
    TestSuite *suite = create_test_suite();

    add_test(suite, test_readwrite);
    add_test(suite, test_readwrite_err_space);
    add_test(suite, test_readwrite_invalid_descr);

    return suite;
}

