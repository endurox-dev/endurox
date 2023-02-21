/**
 *
 * @file test_badd.c
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
#include <fml.h>
#include <fml32.h>
#include <ndrstandard.h>
#include <string.h>
#include "test.fd.h"
#include "ubfunit1.h"
#include "ndebug.h"


#define DEFAULT_BUFFER  1024
exprivate UBFH *M_p_ub1 = NULL;
exprivate UBFH *M_p_ub2 = NULL;
exprivate UBFH *M_p_ub3 = NULL;

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
        fprintf(stderr, "Binit failed 1!\n");
    }
    
    M_p_ub2 = malloc(DEFAULT_BUFFER*3);
    memset(M_p_ub2, 255, DEFAULT_BUFFER*3);
    if (EXFAIL==Binit(M_p_ub2, DEFAULT_BUFFER*3))
    {
        fprintf(stderr, "Binit failed 2!\n");
    }
    
    M_p_ub3 = malloc(DEFAULT_BUFFER);
    memset(M_p_ub3, 255, DEFAULT_BUFFER);
    if (EXFAIL==Binit(M_p_ub3, DEFAULT_BUFFER))
    {
        fprintf(stderr, "Binit failed 3!\n");
    }

    /* shared load */
    load_field_table();
}

void basic_teardown1(void)
{
    /*printf("basic_teardown\n");*/
    free (M_p_ub1);
    free (M_p_ub2);
    free (M_p_ub3);
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
 * UBF sub-buffer tests + ptr & view test
 * This will fill up sme recursive data to M_p_ub2
 */
Ensure(test_Badd_ubf)
{
    char tmp_buf[128];
    long l;
    struct UBTESTVIEW1 v;
    BVIEWFLD vf;
    int i;
    
    memset(&v, 18, sizeof(v));
    assert_equal(
            Badd(M_p_ub2, T_STRING_4_FLD, "HELLO4", 0),
            EXSUCCEED);
    
    assert_equal(
            Badd(M_p_ub3, T_STRING_2_FLD, "HELLO WORLD", 0),
            EXSUCCEED);
    
    assert_equal(
            Badd(M_p_ub3, T_STRING_2_FLD, "HELLO WORLD 2", 0),
            EXSUCCEED);
    
    /* Add empty */
    assert_equal(
            Badd(M_p_ub1, T_UBF_2_FLD, (char *)M_p_ub2, 0),
            EXSUCCEED);
   
    assert_equal(
            Badd(M_p_ub1, T_UBF_2_FLD, (char *)M_p_ub2, 0),
            EXSUCCEED);
    
    assert_equal(
            Badd(M_p_ub1, T_UBF_3_FLD, (char *)M_p_ub3, 0),
            EXSUCCEED);
   
    /* Add some data, 3x levels */
    assert_equal(
            Badd(M_p_ub2, T_UBF_FLD, (char *)M_p_ub1, 0),
            EXSUCCEED);
    
    /* Add field before UBF (ptr)  */
    assert_equal(
            CBadd(M_p_ub2, T_CARRAY_2_FLD, "0x55", 0, BFLD_STRING),
            EXSUCCEED);
    
    assert_equal(
            CBadd(M_p_ub2, T_PTR_FLD, "0x55", 0, BFLD_STRING),
            EXSUCCEED);
    
    NDRX_STRCPY_SAFE(v.tcarray5, "TEST");
    
    /* Load some view data... */
    vf.data=(char *)&v;
    vf.vflags=0;
    NDRX_STRCPY_SAFE(vf.vname, "UBTESTVIEW1");
    
    assert_equal(
            Badd(M_p_ub2, T_VIEW_FLD, (char *)&vf, 0),
            EXSUCCEED);
    
    NDRX_STRCPY_SAFE(v.tcarray5, "SOME");
    
    assert_equal(
            Badd(M_p_ub2, T_VIEW_FLD, (char *)&vf, 0),
            EXSUCCEED);
    
    /* add buffer full... */
    while (EXSUCCEED==Badd(M_p_ub2, T_VIEW_FLD, (char *)&vf, 0)){}
    assert_equal(Berror, BNOSPACE);
    
    /* test field presence... */
    UBF_LOG(log_debug, "First");
    assert_equal(Bpresr(M_p_ub2, (int []){T_UBF_FLD, 0, T_UBF_2_FLD, 0, BBADFLDOCC}), EXTRUE);
    
    assert_equal(Berror, EXSUCCEED);
    
    UBF_LOG(log_debug, "2nd");
    
    assert_equal(
            Bpresr(M_p_ub2, (int []){T_UBF_FLD, 0, T_UBF_2_FLD, 1, BBADFLDOCC}),
            EXTRUE);
    assert_equal(Berror, EXSUCCEED);
            
    assert_equal(
            Bpresr(M_p_ub2, (int []){T_UBF_FLD, 0, T_UBF_2_FLD, 2, BBADFLDOCC}),
            EXFALSE);
    
    assert_equal(Berror, EXSUCCEED);
            
    assert_equal(
            Bpresr(M_p_ub2, (int []){T_UBF_FLD, 0, T_UBF_3_FLD, 0, BBADFLDOCC}),
            EXTRUE);
    assert_equal(Berror, EXSUCCEED);
                        
            
    /* Load some values to-sub buffer */
    assert_equal(Bgetrv(M_p_ub2, tmp_buf, 0, 
            T_UBF_FLD, 0, T_UBF_2_FLD, 0, T_STRING_4_FLD, 0, BBADFLDOCC), EXSUCCEED);
    
    assert_string_equal(tmp_buf, "HELLO4");
    
    assert_equal(CBget(M_p_ub2, T_PTR_FLD, 0, (char *)&l, 0, BFLD_LONG), EXSUCCEED);
    assert_equal(l, 0x55);
    
    /* Read view... & check values - occ 0*/
    memset(&v, 0, sizeof(v));
    vf.vname[0]=EXEOS;
    vf.data=(char *)&v;
    
    assert_equal(Bget(M_p_ub2, T_VIEW_FLD, 0, (char *)&vf, 0), EXSUCCEED);
    assert_string_equal(vf.vname, "UBTESTVIEW1");
    assert_string_equal(v.tcarray5, "TEST");
    
    /* occ 1 */
    memset(&v, 0, sizeof(v));
    vf.vname[0]=EXEOS;
    vf.data=(char *)&v;
    assert_equal(Bget(M_p_ub2, T_VIEW_FLD, 1, (char *)&vf, 0), EXSUCCEED);
    assert_string_equal(vf.vname, "UBTESTVIEW1");
    assert_string_equal(v.tcarray5, "SOME");
    
    
    /* test buffer full of FBs... */
    assert_equal(Binit(M_p_ub2, Bsizeof(M_p_ub2)), EXSUCCEED);
    
    i=0;
    while (EXSUCCEED==Badd(M_p_ub2, T_UBF_3_FLD, (char *)M_p_ub3, 0)){i++;}
    assert_equal(Berror, BNOSPACE);
    assert_not_equal(i, 0);
    
    
    /* test buffer full of ptrs */
    assert_equal(Binit(M_p_ub2, Bsizeof(M_p_ub2)), EXSUCCEED);
    
    i=0;
    while (EXSUCCEED==Badd(M_p_ub2, T_PTR_FLD, (char *)M_p_ub3, 0)){i++;}
    assert_equal(Berror, BNOSPACE);
    assert_not_equal(i, 0);
    
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
 * Test invalid sequence of the fields, add string by fastadd, then then short
 */
Ensure(test_Baddfast3)
{
    Bfld_loc_info_t state;
    char buf1[56000];
    short s;
    UBFH *p_ub1 = (UBFH *)buf1;
    
    
    memset(buf1, 0, sizeof(buf1));
    memset(&state, 0, sizeof(state));
    assert_equal(Binit(p_ub1, sizeof(buf1)), EXSUCCEED);
    
    
    assert_equal(Baddfast(p_ub1, T_STRING_FLD, "DUM1", 0, &state), EXSUCCEED);
    assert_equal(Baddfast(p_ub1, T_STRING_FLD, "DUM2", 0, &state), EXSUCCEED);
    
    /* so what happens? */
    s=991;
    assert_equal(Baddfast(p_ub1, T_SHORT_FLD, (char *)&s, 0, &state), EXFAIL);
    assert_equal(Berror, BEINVAL);
    
}

Ensure(test_CBaddfast)
{
    Bfld_loc_info_t state;
    char buf1[56000];
    short s;
    UBFH *p_ub1 = (UBFH *)buf1;
    char *tree = NULL;
    
    memset(buf1, 0, sizeof(buf1));
    memset(&state, 0, sizeof(state));
    assert_equal(Binit(p_ub1, sizeof(buf1)), EXSUCCEED);
    
    
    assert_equal(CBaddfast(p_ub1, T_SHORT_FLD, "1", 0, BFLD_STRING, &state), EXSUCCEED);
    assert_equal(CBaddfast(p_ub1, T_SHORT_FLD, "2", 0, BFLD_STRING, &state), EXSUCCEED);
    
    /* continues next: */
    assert_equal(CBaddfast(p_ub1, T_LONG_FLD, "3", 0, BFLD_STRING, &state), EXSUCCEED);
    assert_equal(CBaddfast(p_ub1, T_LONG_FLD, "4", 0, BFLD_STRING, &state), EXSUCCEED);
    
    s=5;
    assert_equal(CBaddfast(p_ub1, T_STRING_FLD, (char *)&s, 0, BFLD_SHORT, &state), EXSUCCEED);
    s=6;
    assert_equal(CBaddfast(p_ub1, T_STRING_FLD, (char *)&s, 0, BFLD_SHORT, &state), EXSUCCEED);
    
    assert_equal(CBaddfast(p_ub1, T_CARRAY_FLD, "HELLO", 0, BFLD_STRING, &state), EXSUCCEED);
    assert_equal(CBaddfast(p_ub1, T_CARRAY_FLD, "WORLD", 0, BFLD_STRING, &state), EXSUCCEED);
    
    /* test some errors */
    assert_equal(CBaddfast(p_ub1, T_UBF_FLD, "1", 0, BFLD_UBF, &state), EXFAIL);
    assert_equal(Berror, BEBADOP);
    assert_equal(CBaddfast(p_ub1, T_UBF_FLD, "2", 0, BFLD_UBF, &state), EXFAIL);
    assert_equal(Berror, BEBADOP);
    
    /* validate the buffer */
    tree=Bboolco ("T_SHORT_FLD[0]==1 && T_SHORT_FLD[1]==2 && T_LONG_FLD[0]==3 && T_LONG_FLD[1]==4 &&"
                "T_STRING_FLD[0]=='5' && T_STRING_FLD[1]=='6' && T_CARRAY_FLD[0]=='HELLO' &&"
                " T_CARRAY_FLD[1]=='WORLD'");
    assert_not_equal(tree, NULL);
    assert_equal(Bboolev(p_ub1, tree), EXTRUE);
    Btreefree(tree);
}



/**
 * Test FML wrapper
 * i.e. FLD_PTR logic is different for FML.
 */
Ensure(test_Fadd)
{
    char buf1[56000];
    char buf_ptr1[100];
    char buf_ptr2[100];
    char *ptr_get = NULL;
    long l;
    BFLDLEN len;
    UBFH *p_ub1 = (UBFH *)buf1;
    
    memset(buf1, 0, sizeof(buf1));
    assert_equal(Binit(p_ub1, sizeof(buf1)), EXSUCCEED);
    
    /* Load ptr */
    assert_equal(Fadd(p_ub1, T_PTR_FLD, buf_ptr1, 0), EXSUCCEED);
    assert_equal(Fadd32(p_ub1, T_PTR_FLD, buf_ptr2, 0), EXSUCCEED);

    l=999;
    assert_equal(Fadd(p_ub1, T_LONG_FLD, (char *)&l, 0), EXSUCCEED);
    l=888;
    assert_equal(Fadd(p_ub1, T_LONG_FLD, (char *)&l, 0), EXSUCCEED);
    
    assert_equal(Fadd(p_ub1, T_STRING_FLD, "HELLO 1", 0), EXSUCCEED);
    assert_equal(Fadd32(p_ub1, T_STRING_FLD, "HELLO 2", 0), EXSUCCEED);
    
    assert_equal(Fadd(p_ub1, T_CARRAY_FLD, "ABCD", 1), EXSUCCEED);
    assert_equal(Fadd32(p_ub1, T_CARRAY_FLD, "CDE", 1), EXSUCCEED);
    
    /* Validate the data.., ptr */
    assert_equal(Bget(p_ub1, T_PTR_FLD, 0, (char *)&ptr_get, 0), EXSUCCEED);
    assert_equal(ptr_get, buf_ptr1);
    
    assert_equal(Bget(p_ub1, T_PTR_FLD, 1, (char *)&ptr_get, 0), EXSUCCEED);
    assert_equal(ptr_get, buf_ptr2);
    
    /* validate string */
    assert_equal(Bget(p_ub1, T_STRING_FLD, 0, buf_ptr1, 0), EXSUCCEED);
    assert_string_equal(buf_ptr1, "HELLO 1");
    
    assert_equal(Bget(p_ub1, T_STRING_FLD, 1, buf_ptr1, 0), EXSUCCEED);
    assert_string_equal(buf_ptr1, "HELLO 2");
    
    assert_equal(CBget(p_ub1, T_CARRAY_FLD, 0, buf_ptr1, 0, BFLD_STRING), EXSUCCEED);
    assert_string_equal(buf_ptr1, "A");
    
    assert_equal(CBget(p_ub1, T_CARRAY_FLD, 1, buf_ptr1, 0, BFLD_STRING), EXSUCCEED);
    assert_string_equal(buf_ptr1, "C");
    
    /* validate long */
    
    assert_equal(Bget(p_ub1, T_LONG_FLD, 0, (char *)&l, 0), EXSUCCEED);
    assert_equal(l, 999);
    
    assert_equal(Bget(p_ub1, T_LONG_FLD, 1, (char *)&l, 0), EXSUCCEED);
    assert_equal(l, 888);
        
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
    add_test(suite, test_Baddfast3);
    add_test(suite, test_CBaddfast);
    
    add_test(suite, test_Badd_ubf);
    add_test(suite, test_Fadd);

    return suite;
}
/* vim: set ts=4 sw=4 et smartindent: */
