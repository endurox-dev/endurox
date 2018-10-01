/**
 *
 * @file test_readwrite.c
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

/**
 * Place where to put the read/write data
 */
exprivate char M_temp_space[1024];
exprivate int M_cur_offset;

/**
 * Write function to buffer
 * @param buffer buffer received
 * @param bufsz buffer to write
 * @param dataptr1 extra ptr must be 239
 * @return  number of bytes written
 */

exprivate long test_writef(char *buffer, long bufsz, void *dataptr1)
{
    memcpy(M_temp_space+M_cur_offset, buffer, bufsz);
    
    M_cur_offset+=(int)bufsz;
    
    assert_equal(dataptr1, (char *)675);
    
    return bufsz;
}

/**
 * Read data from buffer
 * @param buffer buffer to read from
 * @param bufsz data size to fill in the buffer / requested
 * @param dataptr1 optional ptr must be 675
 * @return 
 */
exprivate long test_readf(char *buffer, long bufsz, void *dataptr1)
{
    memcpy(buffer, M_temp_space+M_cur_offset, bufsz);
    
    M_cur_offset+=(int)bufsz;
    
    assert_equal(dataptr1, (char *)239);
    
    return bufsz;
}

/**
 * Perform testing of Brwritecb() and Breadcb()
 */
Ensure(test_readwrite_callbacked)
{
    char fb[1024];
    UBFH *p_ub = (UBFH *)fb;
    
    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);

    /* Load test stuff */
    set_up_dummy_data(p_ub);
    
    M_cur_offset = 0;
    assert_equal(Bwritecb(p_ub, test_writef, (char *)675), EXSUCCEED);
    
    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);
    
    M_cur_offset = 0;
    assert_equal(Breadcb(p_ub, test_readf, (char *)239), EXSUCCEED);
    
    do_dummy_data_test(p_ub);
}

TestSuite *ubf_readwrite_tests(void)
{
    TestSuite *suite = create_test_suite();

    add_test(suite, test_readwrite);
    add_test(suite, test_readwrite_err_space);
    add_test(suite, test_readwrite_invalid_descr);
    add_test(suite, test_readwrite_callbacked);

    return suite;
}

/* vim: set ts=4 sw=4 et smartindent: */
