/**
 *
 * @file test_readwrite.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2023, Mavimax, Ltd. All Rights Reserved.
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
#include "test.fd.h"
#include "ubfunit1.h"
#include <ndebug.h>
#include <ubfutil.h>


/**
 * Test - Test buffer read/write
 */
Ensure(test_readwrite)
{
    char fb[2048];
    UBFH *p_ub = (UBFH *)fb;
    char fb2[2048];
    UBFH *p_ub2 = (UBFH *)fb2;

    /* needs to set padding to some known value */
    memset(fb, 7, sizeof(fb));
    memset(fb2, 7, sizeof(fb2));

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
    char fb[2048];
    UBFH *p_ub = (UBFH *)fb;
    char fb2[128];
    UBFH *p_ub2 = (UBFH *)fb2;

    memset(fb, 7, sizeof(fb));
    memset(fb2, 7, sizeof(fb2));

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
    char fb[2048];
    UBFH *p_ub = (UBFH *)fb;
    char fb2[128];
    UBFH *p_ub2 = (UBFH *)fb2;

    memset(fb, 7, sizeof(fb));
    memset(fb2, 7, sizeof(fb2));

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
exprivate char M_temp_space[2048];
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
    char fb[2048];
    UBFH *p_ub = (UBFH *)fb;

    memset(fb, 1, sizeof(fb));
    
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

/**
 * Test that BFLD_PTR is stripped in recursive way
 */
Ensure(test_readwrite_no_ptr)
{
    char fb[2048];
    UBFH *p_ub = (UBFH *)fb;
    
    char fb2[2048];
    UBFH *p_ub2 = (UBFH *)fb2;
    
    char fb3[2048];
    UBFH *p_ub3 = (UBFH *)fb3;
    
    char fb4[2048];
    UBFH *p_ub4 = (UBFH *)fb4;

    BFLDLEN len;
    unsetenv("NDRX_APIFLAGS");

    memset(fb, 0, sizeof(fb));
    memset(fb2, 0, sizeof(fb2));
    memset(fb3, 0, sizeof(fb3));
    memset(fb4, 0, sizeof(fb4));

    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);
    assert_equal(Binit(p_ub2, sizeof(fb2)), EXSUCCEED);
    
    assert_equal(Binit(p_ub3, sizeof(fb3)), EXSUCCEED);
    assert_equal(Binit(p_ub4, sizeof(fb4)), EXSUCCEED);
    
    assert_equal(Bchg(p_ub4, T_STRING_9_FLD, 0, "HELLO1", 0), EXSUCCEED);
    assert_equal(Bchg(p_ub4, T_STRING_10_FLD, 0, "HELLO2", 0), EXSUCCEED);
    assert_equal(CBchg(p_ub4, T_PTR_FLD, 3, "999", 0, BFLD_STRING), EXSUCCEED);
    
    assert_equal(Bchg(p_ub3, T_UBF_FLD, 2, (char *)p_ub4, 0), EXSUCCEED);
    assert_equal(CBchg(p_ub3, T_PTR_FLD, 1, "888", 0, BFLD_STRING), EXSUCCEED);
    assert_equal(Bchg(p_ub3, T_STRING_8_FLD, 2, "HELLO3", 0), EXSUCCEED);
    
    assert_equal(Bchg(p_ub, T_UBF_FLD, 4, (char *)p_ub3, 0), EXSUCCEED);
    assert_equal(CBchg(p_ub, T_PTR_FLD, 5, "1231", 0, BFLD_STRING), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_STRING_7_FLD, 2, "HELLO4", 0), EXSUCCEED);
    
    open_test_temp("wb");

    assert_equal(Bwrite(p_ub, M_test_temp_file), EXSUCCEED);
    close_test_temp();

    /* Now read the stuff in second buffer */
    open_test_temp_for_read("rb");
    assert_equal(Bread(p_ub2, M_test_temp_file), EXSUCCEED);

    close_test_temp();
    remove_test_temp();

    /* Now compare the buffers 
    
    Bprint(p_ub);
    printf("**************\n");
    Bprint(p_ub2);
    */
    
    assert_not_equal(memcmp(p_ub, p_ub2, Bused(p_ub)), 0);
    
    /* buffer 1 */
    assert_string_equal(Bfindrv (p_ub2, &len, 
            T_STRING_7_FLD, 2, BBADFLDOCC), "HELLO4");
    assert_equal(Bfindrv (p_ub2, &len,
            T_PTR_FLD, 0, BBADFLDOCC), NULL);
    assert_equal(Berror, BNOTPRES);
    
    assert_string_equal(Bfindrv (p_ub2, &len, 
            T_UBF_FLD, 4, T_STRING_8_FLD, 2, BBADFLDOCC), "HELLO3");
    assert_equal(Bfindrv (p_ub2, &len,
            T_UBF_FLD, 4, T_PTR_FLD, 0, BBADFLDOCC), NULL);
    assert_equal(Berror, BNOTPRES);
    
    
    assert_string_equal(Bfindrv (p_ub2, &len, 
            T_UBF_FLD, 4, T_UBF_FLD, 2, T_STRING_10_FLD, 0, BBADFLDOCC), "HELLO2");
    
    assert_string_equal(Bfindrv (p_ub2, &len, 
            T_UBF_FLD, 4, T_UBF_FLD, 2, T_STRING_9_FLD, 0, BBADFLDOCC), "HELLO1");
    
    assert_equal(Bfindrv (p_ub2, &len,
            T_UBF_FLD, 4, T_UBF_FLD, 2, T_PTR_FLD, 0, BBADFLDOCC), NULL);
    assert_equal(Berror, BNOTPRES);
    
}

TestSuite *ubf_readwrite_tests(void)
{
    TestSuite *suite = create_test_suite();

    add_test(suite, test_readwrite);
    add_test(suite, test_readwrite_err_space);
    add_test(suite, test_readwrite_invalid_descr);
    add_test(suite, test_readwrite_callbacked);
    
    /* Test PTR stripping... unsetenv("NDRX_APIFLAGS"); */
    add_test(suite, test_readwrite_no_ptr);
    
    return suite;
}

/* vim: set ts=4 sw=4 et smartindent: */
