/**
 * Perform tests on embedded UBF buffer
 *  Testing strategy:
 *  - check error handling for each api
 *  - check reading: some fixed len, string, UBF
 * 
 * @file test_embubf.c
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
#include <ndebug.h>
#include <string.h>
#include "test.fd.h"
#include "ubfunit1.h"


/**
 * Preparation before the test
 */
Ensure(embubf_basic_setup1)
{
    /* set view env... */
    setenv("VIEWDIR", "./", 1);
    setenv("VIEWFILES", "test_view.V", 1);
}


/**
 * Load the view test data
 * @param v
 */
expublic void init_UBTESTVIEW1(struct UBTESTVIEW1 *v)
{
    int i, j;
        
    v->tshort1=15556;	/* null=2000 */

    v->C_tshort2=2;
    v->tshort2[0]=9999;	/* null=2001 */
    v->tshort2[1]=8888;	/* null=2001 */

    v->C_tshort3 = 2;
    v->tshort3[0]=7777;	/* null=2001 */
    v->tshort3[1]=-7777;	/* null=2001 */
    v->tshort3[2]=-7777;	/* null=2001 */

    v->tshort4=-10;	/* null=NONE */

    v->tlong1=33333333;	/* null=0 */

    v->tint2[0]=54545;	/* null=0 */
    v->tint2[1]=23232;	/* null=0 */
    v->tint3=-100;
    v->tint4[0]=1010101;	/* null=-1 */
    v->tint4[1]=989898;	/* null=-1 */

    v->tchar1='A';	/* null="\n" */

    v->C_tchar2=5;
    v->tchar2[0]='A';	/* null="A" */
    v->tchar2[1]='B';
    v->tchar2[2]='C';
    v->tchar2[3]='\n';
    v->tchar2[4]='\t';

    v->C_tchar3=0;
    v->tchar3[0]='C';	/* null="A" */
    v->tchar3[1]='D';	/* null="A" */

    v->tfloat1[0]=-0.11;	/* null=1.1 */
    v->tfloat1[1]=-0.22;	/* null=1.1 */
    v->tfloat1[2]=0.33;	/* null=1.1 */
    v->tfloat1[3]=0.44;	/* null=1.1 */

    v->tfloat2[0]=100000.1;	/* null=1.1 */
    v->tfloat2[1]=200000.2;	/* null=1.1 */

    v->tfloat3=333333.111;	/* null=9999.99 */

    v->tdouble1[0]=99999.111111;	/* null=55555.99 */
    v->tdouble1[1]=11111.999999;	/* null=55555.99 */
    v->tdouble2=-999.123;	/* null=-999.123 */

    NDRX_STRCPY_SAFE(v->tstring0[0], "HELLO Enduro/X");	/* null="\n\t\f\\\'\"\vHELLOWORLD" */
    NDRX_STRCPY_SAFE(v->tstring0[1], "");	/* null="\n\t\f\\\'\"\vHELLOWORLD" */
    NDRX_STRCPY_SAFE(v->tstring0[2], "\nABC\n");	/* null="\n\t\f\\\'\"\vHELLOWORLD" */

    NDRX_STRCPY_SAFE(v->tstring1[0], "Pack my box");	/* null="HELLO WORLDB" */
    NDRX_STRCPY_SAFE(v->tstring1[1], "BOX MY PACK");	/* null="HELLO WORLDB" */
    NDRX_STRCPY_SAFE(v->tstring1[2], "\nEnduro/X\n");	/* null="HELLO WORLDB" */

    /* Test the L length indicator, must be set to number of bytes transfered */
    v->C_tstring2=2;

    NDRX_STRCPY_SAFE(v->tstring2[0], "CCCCAAAADDDD");
    NDRX_STRCPY_SAFE(v->tstring2[1], "EEEFFFGGG");
    NDRX_STRCPY_SAFE(v->tstring2[2], "IIIIJJJJKKK");

    v->C_tstring3=4;

    NDRX_STRCPY_SAFE(v->tstring3[0], "LLLLLL");	/* null="TESTEST" */
    NDRX_STRCPY_SAFE(v->tstring3[1], "MMMMMM");	/* null="TESTEST" */
    NDRX_STRCPY_SAFE(v->tstring3[2], "");	/* null="TESTEST" */
    NDRX_STRCPY_SAFE(v->tstring3[3], "NNNNNN");	/* null="TESTEST" */

    NDRX_STRCPY_SAFE(v->tstring4, "Some string");	/* null="HELLO TEST" */
    
    NDRX_STRCPY_SAFE(v->tstring5, "MEGA TEST");

    for (i=0; i<30; i++)
    {
        v->tcarray1[i]=i;
    }

    v->L_tcarray2 = 5;
    for (i=0; i<25; i++)
    {
        v->tcarray2[i]=i+1;
    }
    
    v->C_tcarray3 = 9;
            
    for (j=0; j<9; j++)
    {
        v->L_tcarray3[j]=j+1;
        
        for (i=0; i<16+j; i++)
        {
            v->tcarray3[j][i]=i+2;
        }
    }
    
    for (i=0; i<5; i++)
    {
        v->tcarray4[i]=i+3;
    }
    
    for (i=0; i<5; i++)
    {
        v->tcarray5[i]=i+4;
    }
    
}


/**
 * Perform tests for buffer add
 */
expublic void load_recursive_data(UBFH *p_ub)
{
    char buf1[56000];
    UBFH *p_ub1 = (UBFH *)buf1;
    
    char buf2[56000];
    UBFH *p_ub2 = (UBFH *)buf2;
    
    char buf4[56000];
    UBFH *p_ub4 = (UBFH *)buf4;

    struct UBTESTVIEW1 v;
    BVIEWFLD vf;
    
    char tmp_str[1024];
    
    memset(buf1, 0, sizeof(buf1));
    memset(buf2, 0, sizeof(buf2));
    memset(buf4, 0, sizeof(buf4));
    
    assert_equal(Binit(p_ub1, sizeof(buf1)), EXSUCCEED);
    assert_equal(Binit(p_ub2, sizeof(buf2)), EXSUCCEED);
    assert_equal(Binit(p_ub4, sizeof(buf4)), EXSUCCEED);
    
    assert_equal(Badd(p_ub2, T_STRING_3_FLD, "HELLO_CHILD", 0), EXSUCCEED);
    assert_equal(Badd(p_ub2, T_STRING_3_FLD, "HELLO_CHILD 2", 0), EXSUCCEED);
    assert_equal(Badd(p_ub2, T_STRING_3_FLD, "HELLO_CHILD 3", 0), EXSUCCEED);
    assert_equal(Badd(p_ub2, T_STRING_3_FLD, "HELLO_CHILD 4", 0), EXSUCCEED);
    
    assert_equal(Badd(p_ub1, T_STRING_2_FLD, "HELLO_PARENT", 0), EXSUCCEED);
    assert_equal(Badd(p_ub1, T_STRING_4_FLD, "HELLO_PARENT 2", 0), EXSUCCEED);

    /* Load view data: */    
    init_UBTESTVIEW1(&v);
    vf.data=(char *)&v;
    vf.vflags=0;
    NDRX_STRCPY_SAFE(vf.vname, "UBTESTVIEW1");
    
    assert_equal(Bchg(p_ub2, T_VIEW_FLD, 1, (char *)&vf, 0), EXSUCCEED);
    
    /* load add fields to lev 2 */
    set_up_dummy_data(p_ub4);
    assert_equal(Badd(p_ub2, T_UBF_2_FLD, (void *)p_ub4, 0), EXSUCCEED);
    
    assert_equal(Badd(p_ub1, T_UBF_FLD, (char *)p_ub2, 0), EXSUCCEED);
    assert_equal(Badd(p_ub1, T_UBF_FLD, (char *)p_ub4, 0), EXSUCCEED);
    
    /* load standard base to lev 3*/
    assert_equal(Badd(p_ub, T_UBF_FLD, (char *)p_ub4, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_UBF_2_FLD, 1, (char *)p_ub1, 0), EXSUCCEED);
    set_up_dummy_data(p_ub);
    
    
    Bprint(p_ub);
    
#if 0
    
    UBF_DUMP(log_error, "Buffer Dump", buf1, Bused(p_ub1));
        
    memset(buf2, 0, sizeof(buf2));

    assert_equal(Bget(p_ub1, T_UBF_FLD, 0, (char *)p_ub2, 0), EXSUCCEED);
    assert_equal(Bget(p_ub2, T_STRING_3_FLD, 0, tmp_str, 0), EXSUCCEED);
    assert_string_equal(tmp_str, "HELLO_CHILD");
    
    
    Bfprint(p_ub4, stderr);
    
    tmp_str[0]=EXEOS;
    assert_equal(RBget (p_ub4, (int []){ T_UBF_2_FLD, 0, T_UBF_FLD, 1, T_STRING_3_FLD, 1, BBADFLDOCC}, 
            tmp_str, NULL), EXSUCCEED);
    assert_string_equal(tmp_str, "HELLO_CHILD 2");
    
#endif
    
}

Ensure(test_RBget)
{
    char buf[56000];
    UBFH *p_ub = (UBFH *)buf;
    char tmp[1024];
    BFLDLEN len;
    
    assert_equal(Binit(p_ub, sizeof(buf)), EXSUCCEED);
    load_recursive_data(p_ub);
    
    /* check error path: */
    
    assert_equal(RBget (p_ub, (int []){ T_UBF_2_FLD, BBADFLDOCC}, 
            tmp, &len), EXFAIL);
    assert_equal(Berror, BBADFLDID);
    
    assert_equal(RBgetv (p_ub, tmp, &len, T_UBF_2_FLD, BBADFLDOCC), EXFAIL);
    assert_equal(Berror, BBADFLDID);
    
    /* check invalid subfield (i.e subfield of non UBF..) */
    assert_equal(RBget (p_ub, (int []){ T_STRING_8_FLD, 0, T_STRING_10_FLD, 0, BBADFLDOCC}, 
            tmp, &len), EXFAIL);
    assert_equal(Berror, BBADFLDID);
    
    assert_equal(RBgetv (p_ub, tmp, &len, T_STRING_8_FLD, 0, T_STRING_10_FLD, 0, BBADFLDOCC), EXFAIL);
    assert_equal(Berror, BBADFLDID);
    
    /* check standard version */
    len=sizeof(tmp);
    assert_equal(RBget (p_ub, (int []){ T_UBF_2_FLD, 1, T_UBF_FLD, 0, T_STRING_3_FLD, 1, BBADFLDOCC}, 
            tmp, &len), EXSUCCEED);
    assert_string_equal(tmp, "HELLO_CHILD 2");
    assert_equal(len, 14);
    
    /* Check the v version */
    len=sizeof(tmp);
    tmp[0]=EXEOS;
    
    assert_equal(RBgetv (p_ub, tmp, &len, T_UBF_2_FLD, 1, T_UBF_FLD, 0, T_STRING_3_FLD, 1, BBADFLDOCC), EXSUCCEED);
    assert_string_equal(tmp, "HELLO_CHILD 2");
    assert_equal(len, 14);

    /* read fixed len for level 0*/
    
    /* TODO: Read the UBF... and validate dummy data */
    
}

/**
 * Common suite entry
 * @return
 */
TestSuite *ubf_embubf_tests(void)
{
    TestSuite *suite = create_test_suite();

    set_setup(suite, embubf_basic_setup1);
    add_test(suite, test_RBget);
    
    return suite;
}
/* vim: set ts=4 sw=4 et smartindent: */
