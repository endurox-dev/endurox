/* 
**
** @file test_badd.c
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



#define DEFAULT_BUFFER  128+8
UBFH *M_p_ub1 = NULL;

/**
 * Basic preparation before the test
 */
Ensure(basic_setup1)
{
    /*printf("basic_setup\n"); */
    M_p_ub1 = malloc(DEFAULT_BUFFER);
    memset(M_p_ub1, 255, DEFAULT_BUFFER);
    if (FAIL==Binit(M_p_ub1, DEFAULT_BUFFER))
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

    char tmp_buf[32+8]; /* 8 for alignment */
    int org_used = Bused(M_p_ub1);

    assert_equal(
            Badd(M_p_ub1, T_STRING_FLD, pan1, 0),
            SUCCEED);

    assert_equal(
            Badd(M_p_ub1, T_STRING_FLD, pan2, 0),
            SUCCEED);

    assert_equal(
            Badd(M_p_ub1, T_STRING_FLD, pan3, 0),
            SUCCEED);

    assert_equal(
            Badd(M_p_ub1, T_STRING_FLD, pan4, 0),
            SUCCEED);

    /* Get the data from buffer */
    assert_equal(
            Bget(M_p_ub1, T_STRING_FLD, 0, tmp_buf, 0),
            SUCCEED);
    assert_string_equal(pan1, tmp_buf);

    assert_equal(
            Bget(M_p_ub1, T_STRING_FLD, 1, tmp_buf, 0),
            SUCCEED);
    assert_string_equal(pan2, tmp_buf);

    assert_equal(
            Bget(M_p_ub1, T_STRING_FLD, 2, tmp_buf, 0),
            SUCCEED);
    assert_string_equal(pan3, tmp_buf);

    assert_equal(
            Bget(M_p_ub1, T_STRING_FLD, 3, tmp_buf, 0),
            SUCCEED);
    assert_string_equal(pan4, tmp_buf);

    /* Now add something other, some double value? */
    assert_equal(
            Badd(M_p_ub1, T_DOUBLE_4_FLD, (char *)&amttxn1, 0),
            SUCCEED);
    assert_equal(
            Badd(M_p_ub1, T_DOUBLE_4_FLD, (char *)&amttxn2, 0),
            SUCCEED);
    assert_equal(
            Badd(M_p_ub1, T_DOUBLE_4_FLD, (char *)&amttxn3, 0),
            SUCCEED);
    assert_equal(
            Badd(M_p_ub1, T_DOUBLE_4_FLD, (char *)&amttxn4, 0),
            SUCCEED);
    /* Do not have space availabel in buffer */
    assert_equal(
            Badd(M_p_ub1, T_DOUBLE_4_FLD, (char *)&amttxn5, 0),
            FAIL);
    /* Compare the values from buffer */
    assert_equal(
        Bget(M_p_ub1, T_DOUBLE_4_FLD, 0, (char *)&tmp_amttxn, 0),
        SUCCEED);
    assert_double_equal(tmp_amttxn, 10021.123);

    assert_equal(
            Bget(M_p_ub1, T_DOUBLE_4_FLD, 1, (char *)&tmp_amttxn, 0),
            SUCCEED);
    assert_double_equal(tmp_amttxn, 20021.123);

    assert_equal(
            Bget(M_p_ub1, T_DOUBLE_4_FLD, 2, (char *)&tmp_amttxn, 0),
            SUCCEED);
    assert_double_equal(tmp_amttxn, 321.123);
    assert_equal(
            Bget(M_p_ub1, T_DOUBLE_4_FLD, 3, (char *)&tmp_amttxn, 0),
            SUCCEED);
    assert_double_equal(tmp_amttxn, 11.123);
    /* Do not have space in buffer! */
    assert_equal(
            Bget(M_p_ub1, T_DOUBLE_4_FLD, 4, (char *)&tmp_amttxn, 0),
            FAIL);

    /* Summ the stuff up (+4 - EOS symbols in buffer)*/
    assert_equal(org_used+strlen(pan1)+strlen(pan2)+strlen(pan3)+strlen(pan4)+4+
                sizeof(amttxn1)+sizeof(amttxn2)+sizeof(amttxn3)+sizeof(amttxn4)+
                8*sizeof(BFLDID)+8/*align*/, Bused(M_p_ub1));
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

    return suite;
}
