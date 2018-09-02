/**
 *
 * @file test_badd.c
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



#define DEFAULT_BUFFER  1024
UBFH *M_p_ub1 = NULL;

/**
 * Basic preparation before the test
 */
Ensure(basic_setup1)
{
    /*printf("basic_setup\n"); */
    M_p_ub1 = malloc(DEFAULT_BUFFER);
    memset(M_p_ub1, 255, DEFAULT_BUFFER);
    if (EXFAIL==Binit(M_p_ub1, DEFAULT_BUFFER))
    {
        fprintf(stderr, "Binit failed!\n");
    }

    setenv("FLDTBLDIR", "./ubftab", 1);
    setenv("FIELDTBLS", "Exfields,test.fd", 1);
}

void basic_teardown1(void)
{
    /*printf("basic_teardown\n");*/
    free (M_p_ub1);
}

/**
 * Basic test for Badd & Bget
 */
Ensure(test_Badd_str)
{
    char pan1[32] = "THIS IS 1";
    char pan2[32] = "THIS IS 2";
    char pan3[32] = "THIS IS 3";
    char pan4[32] = "THIS IS 4";

    double amttxn1=10021.123;
    double amttxn2=20021.123;
    double amttxn3=321.123;
    double amttxn4=11.123;
    double amttxn5=33.123;
    double tmp_amttxn=0;
    char too_large[2048];

    char tmp_buf[32+8]; /* 8 for alignment */
    int org_used = Bused(M_p_ub1);

    assert_equal(
            Badd(M_p_ub1, T_STRING_FLD, pan1, 0),
            EXSUCCEED);

    assert_equal(
            Badd(M_p_ub1, T_STRING_FLD, pan2, 0),
            EXSUCCEED);

    assert_equal(
            Badd(M_p_ub1, T_STRING_FLD, pan3, 0),
            EXSUCCEED);

    assert_equal(
            Badd(M_p_ub1, T_STRING_FLD, pan4, 0),
            EXSUCCEED);

    /* Get the data from buffer */
    assert_equal(
            Bget(M_p_ub1, T_STRING_FLD, 0, tmp_buf, 0),
            EXSUCCEED);
    assert_string_equal(pan1, tmp_buf);

    assert_equal(
            Bget(M_p_ub1, T_STRING_FLD, 1, tmp_buf, 0),
            EXSUCCEED);
    assert_string_equal(pan2, tmp_buf);

    assert_equal(
            Bget(M_p_ub1, T_STRING_FLD, 2, tmp_buf, 0),
            EXSUCCEED);
    assert_string_equal(pan3, tmp_buf);

    assert_equal(
            Bget(M_p_ub1, T_STRING_FLD, 3, tmp_buf, 0),
            EXSUCCEED);
    assert_string_equal(pan4, tmp_buf);

    /* Now add something other, some double value? */
    assert_equal(
            Badd(M_p_ub1, T_DOUBLE_4_FLD, (char *)&amttxn1, 0),
            EXSUCCEED);
    assert_equal(
            Badd(M_p_ub1, T_DOUBLE_4_FLD, (char *)&amttxn2, 0),
            EXSUCCEED);
    assert_equal(
            Badd(M_p_ub1, T_DOUBLE_4_FLD, (char *)&amttxn3, 0),
            EXSUCCEED);
    assert_equal(
            Badd(M_p_ub1, T_DOUBLE_4_FLD, (char *)&amttxn4, 0),
            EXSUCCEED);
    /* Do not have space available in buffer  */
    assert_equal(
            Badd(M_p_ub1, T_CARRAY_FLD, (char *)too_large, sizeof(too_large)),
            EXFAIL);
    assert_equal(Berror, BNOSPACE);
    /* Compare the values from buffer */
    assert_equal(
        Bget(M_p_ub1, T_DOUBLE_4_FLD, 0, (char *)&tmp_amttxn, 0),
        EXSUCCEED);
    assert_double_equal(tmp_amttxn, 10021.123);

    assert_equal(
            Bget(M_p_ub1, T_DOUBLE_4_FLD, 1, (char *)&tmp_amttxn, 0),
            EXSUCCEED);
    assert_double_equal(tmp_amttxn, 20021.123);

    assert_equal(
            Bget(M_p_ub1, T_DOUBLE_4_FLD, 2, (char *)&tmp_amttxn, 0),
            EXSUCCEED);
    assert_double_equal(tmp_amttxn, 321.123);
    assert_equal(
            Bget(M_p_ub1, T_DOUBLE_4_FLD, 3, (char *)&tmp_amttxn, 0),
            EXSUCCEED);
    assert_double_equal(tmp_amttxn, 11.123);
    /* Do not have space in buffer! 
    assert_equal(
            Bget(M_p_ub1, T_DOUBLE_4_FLD, 4, (char *)&tmp_amttxn, 0),
            EXFAIL);

     Summ the stuff up (+4 - EOS symbols in buffer)*/
#if 0
    assert_equal(org_used+strlen(pan1)+strlen(pan2)+strlen(pan3)+strlen(pan4)+4+
                sizeof(amttxn1)+sizeof(amttxn2)+sizeof(amttxn3)+sizeof(amttxn4)+
                8*sizeof(BFLDID)+8/*align*/, Bused(M_p_ub1));
#endif
}

/**
 * Test of FAST add test
 * Single field in buffer
 */
Ensure(test_Baddfast1)
{
    Bfld_loc_info_t state;
    int i;
    char str[16];
    char buf1[56000];
    UBFH *p_ub1 = (UBFH *)buf1;
    
    char buf2[56000];
    UBFH *p_ub2 = (UBFH *)buf2;

    
    memset(buf1, 0, sizeof(buf1));
    memset(buf2, 0, sizeof(buf2));
    
    assert_equal(Binit(p_ub1, sizeof(buf1)), EXSUCCEED);
    assert_equal(Binit(p_ub2, sizeof(buf2)), EXSUCCEED);
    
    memset(&state, 0, sizeof(state));
    
    for (i=0; i<100; i++)
    {
	snprintf(str, sizeof(str), "%i", i);
	assert_equal(Baddfast(p_ub1, T_STRING_FLD, str, 0, &state), EXSUCCEED);
        assert_equal(Badd(p_ub2, T_STRING_FLD, str, 0), EXSUCCEED);
    }
    
    /* the buffer shall be equal */
    
    assert_equal(memcmp(buf1, buf2, sizeof(buf1)), 0);
    
}

/**
 * Test of FAST add test
 * Some field exists before and after 
 */
Ensure(test_Baddfast2)
{
    Bfld_loc_info_t state;
    int i;
    char str[16];
    char buf1[56000];
    UBFH *p_ub1 = (UBFH *)buf1;
    
    char buf2[56000];
    UBFH *p_ub2 = (UBFH *)buf2;

    
    memset(buf1, 0, sizeof(buf1));
    memset(buf2, 0, sizeof(buf2));
    
    assert_equal(Binit(p_ub1, sizeof(buf1)), EXSUCCEED);
    assert_equal(Binit(p_ub2, sizeof(buf2)), EXSUCCEED);
    
    
    
    assert_equal(Badd(p_ub1, T_STRING_2_FLD, "HELLO", 0), EXSUCCEED);
    assert_equal(Badd(p_ub2, T_STRING_2_FLD, "HELLO", 0), EXSUCCEED);
    
    assert_equal(CBadd(p_ub1, T_SHORT_FLD, "14", 0, BFLD_STRING), EXSUCCEED);
    assert_equal(CBadd(p_ub2, T_SHORT_FLD, "14", 0, BFLD_STRING), EXSUCCEED);
    
    assert_equal(CBadd(p_ub1, T_CARRAY_FLD, "WORLD", 0, BFLD_STRING), EXSUCCEED);
    assert_equal(CBadd(p_ub2, T_CARRAY_FLD, "WORLD", 0, BFLD_STRING), EXSUCCEED);
    
    /* now perform fast add */
    memset(&state, 0, sizeof(state));
    
    for (i=0; i<100; i++)
    {
	snprintf(str, sizeof(str), "%i", i);
	assert_equal(Baddfast(p_ub1, T_STRING_FLD, str, 0, &state), EXSUCCEED);
        assert_equal(Badd(p_ub2, T_STRING_FLD, str, 0), EXSUCCEED);
    }
    
    /* the buffer shall be equal */
    
    assert_equal(memcmp(buf1, buf2, sizeof(buf1)), 0);
    
    
    /* should fail if state is NULL */
    
    assert_equal(Baddfast(p_ub1, T_STRING_FLD, str, 0, NULL), EXFAIL);
    assert_equal(Berror, BEINVAL);
    
}

/**
 * Common suite entry
 * @return
 */
TestSuite *ubf_Badd_tests(void)
{
    TestSuite *suite = create_test_suite();

    set_setup(suite, basic_setup1);
    set_teardown(suite, basic_teardown1);

    add_test(suite, test_Badd_str);
    add_test(suite, test_Baddfast1);
    add_test(suite, test_Baddfast2);

    return suite;
}
/* vim: set ts=4 sw=4 et smartindent: */
