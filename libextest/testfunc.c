/**
 * @brief This implements shared test functions (shared between UBF and ATMI
 *  series tests)
 *
 * @file testfunc.c
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

/*---------------------------Includes-----------------------------------*/
#include <ndrx_config.h>

#include <stdio.h>
#include <stdlib.h>
#include <ndrstandard.h>
#include <ndebug.h>
#include <userlog.h>
#include <extest.h>
#include <cgreen/cgreen.h>
#include <test.fd.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Load the view test data
 * @param v
 */
expublic void extest_init_UBTESTVIEW1(struct UBTESTVIEW1 *v)
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
 * Test UBF data
 * @param p_ub UBF buffer to validate the data
 * @param flags see EXTEST_PROC_PTR, EXTEST_PROC_UBF, EXTEST_PROC_VIEW bitmask of
 */
void extest_ubf_do_dummy_data_test(UBFH *p_ub, long flags)
{
    char buf[128];
    int len;
    ndrx_longptr_t ptr;
    BVIEWFLD vf;
    struct UBTESTVIEW2 v;
    char tmp[1024];
    long l;
    UBFH* p_ub_tmp=(UBFH *)tmp;
    
    assert_equal(CBget(p_ub, T_STRING_9_FLD, 0, buf, 0, BFLD_STRING), EXSUCCEED);
    assert_string_equal(buf, "01");
    assert_equal(CBget(p_ub, T_STRING_9_FLD, 1, buf, 0, BFLD_STRING), EXSUCCEED);
    assert_string_equal(buf, "20");
    assert_equal(CBget(p_ub, T_STRING_9_FLD, 2, buf, 0, BFLD_STRING), EXSUCCEED);
    assert_string_equal(buf, "31");

    assert_equal(CBget(p_ub, T_DOUBLE_3_FLD, 0, buf, 0, BFLD_STRING), EXSUCCEED);
    assert_equal(strncmp(buf, "1.11", 4), 0);
    assert_equal(CBget(p_ub, T_DOUBLE_3_FLD, 1,buf, 0, BFLD_STRING), EXSUCCEED);
    assert_equal(strncmp(buf, "2.41231", 4), 0);

    assert_equal(CBget(p_ub, T_STRING_4_FLD, 0, buf, 0, BFLD_STRING), EXSUCCEED);
    assert_string_equal(buf, "HELLO WORLD");
    assert_equal(CBget(p_ub, T_STRING_8_FLD, 0, buf, 0, BFLD_STRING), EXSUCCEED);
    assert_string_equal(buf, "HELLO WORLD1");
    assert_equal(CBget(p_ub, T_STRING_5_FLD, 0, buf, 0, BFLD_STRING), EXSUCCEED);
    assert_string_equal(buf, "HELLO WORLD2");
    assert_equal(CBget(p_ub, T_STRING_7_FLD, 0, buf, 0, BFLD_STRING), EXSUCCEED);
    assert_string_equal(buf, "HELLO WORLD3");
    assert_equal(CBget(p_ub, T_STRING_6_FLD, 0, buf, 0, BFLD_STRING), EXSUCCEED);
    assert_string_equal(buf, "HELLO WORLD4");
    assert_equal(CBget(p_ub, T_STRING_10_FLD, 0, buf, 0, BFLD_STRING), EXSUCCEED);
    assert_string_equal(buf, "HELLO WORLD5");
    assert_equal(CBget(p_ub, T_STRING_4_FLD, 1, buf, 0, BFLD_STRING), EXSUCCEED);
    assert_string_equal(buf, "HELLO WORLD6");
    assert_equal(CBget(p_ub, T_STRING_3_FLD, 0, buf, 0, BFLD_STRING), EXSUCCEED);
    assert_string_equal(buf, "HELLO WORLD7");
    /* Another carray test */
    len = sizeof(buf);
    assert_equal(CBget(p_ub, T_CARRAY_2_FLD, 1, buf, 0, BFLD_STRING), EXSUCCEED);
    assert_string_equal(buf, "TEST CARRAY");
    
    /*  Test some PTR, VIEW and UBF */
    if (flags & EXTEST_PROC_PTR)
    {
        assert_equal(CBget(p_ub, T_PTR_3_FLD, 0, (char *)&ptr, 0L, BFLD_LONG), EXSUCCEED);
        assert_equal(ptr, 199);
        assert_equal(CBget(p_ub, T_PTR_3_FLD, 1, (char *)&ptr, 0L, BFLD_LONG), EXSUCCEED);
        assert_equal(ptr, 299);
    }
    
    if (flags & EXTEST_PROC_VIEW)
    {
        vf.data = (char *)&v;

        assert_equal(Bget(p_ub, T_VIEW_3_FLD, 0, (char *)&vf, 0L), EXSUCCEED);

        assert_string_equal(vf.vname, "UBTESTVIEW2");
        assert_equal(vf.vflags, 0);

        assert_equal(v.tshort1, 100);
        assert_equal(v.tlong1, 200);
        assert_equal(v.tchar1, 'G');
        assert_equal(v.tfloat1, 400);
        assert_equal(v.tdouble1, 500);
        assert_string_equal(v.tstring1, "6XX");
        assert_string_equal(v.tcarray1, "7XX");

        assert_equal(Bget(p_ub, T_VIEW_3_FLD, 4, (char *)&vf, 0L), EXSUCCEED);
        assert_equal(v.tchar1, 'Z');
        assert_string_equal(v.tstring1, "6YY");
        assert_string_equal(v.tcarray1, "7YY");
    }
    
    if (flags & EXTEST_PROC_UBF)
    {
        assert_equal(Bget(p_ub, T_UBF_3_FLD, 2, (char *)p_ub_tmp, 0L), EXSUCCEED);
        assert_equal(Bget(p_ub_tmp, T_STRING_9_FLD, 3, (char *)buf, 0L), EXSUCCEED);
        assert_string_equal(buf, "HELLO WORLD UB");


        assert_equal(Bget(p_ub, T_UBF_3_FLD, 3, (char *)p_ub_tmp, 0L), EXSUCCEED);
        assert_equal(Bget(p_ub_tmp, T_STRING_7_FLD, 2, (char *)buf, 0L), EXSUCCEED);
        assert_string_equal(buf, "ANOTHER UB");    
        assert_equal(Bget(p_ub_tmp, T_STRING_9_FLD, 3, (char *)buf, 0L), EXSUCCEED);
        assert_string_equal(buf, "HELLO WORLD UB");

        assert_equal(Bget(p_ub, T_LONG_3_FLD, 3, (char *)&l, 0L), EXSUCCEED);
        assert_equal(l, 889991);
    }
    
}

/**
 * Load test data
 * @param p_ub UBF buffer where to store the data
 * @param flags see EXTEST_PROC_PTR, EXTEST_PROC_UBF, EXTEST_PROC_VIEW bitmask of
 */
void extest_ubf_set_up_dummy_data(UBFH *p_ub, long flags)
{
    char buf[128];
    int len;
    long ptr;
    BVIEWFLD vf;
    struct UBTESTVIEW2 v;
    char tmp[1024];
    long l;
    UBFH* p_ub_tmp=(UBFH *)tmp;
    
    assert_equal(CBadd(p_ub, T_STRING_9_FLD, "01", 0, BFLD_STRING), EXSUCCEED);
    
    assert_equal(CBget(p_ub, T_STRING_9_FLD, 0, buf, 0, BFLD_STRING), EXSUCCEED);
    assert_string_equal(buf, "01");
    
    assert_equal(CBadd(p_ub, T_STRING_9_FLD, "20", 0, BFLD_STRING), EXSUCCEED);
    
    assert_equal(CBget(p_ub, T_STRING_9_FLD, 1, buf, 0, BFLD_STRING), EXSUCCEED);
    assert_string_equal(buf, "20");
    
    assert_equal(CBadd(p_ub, T_STRING_9_FLD, "31", 0, BFLD_STRING), EXSUCCEED);
    
    assert_equal(CBget(p_ub, T_STRING_9_FLD, 2, buf, 0, BFLD_STRING), EXSUCCEED);
    assert_string_equal(buf, "31");
    

    assert_equal(CBadd(p_ub, T_DOUBLE_3_FLD, "1.11", 0, BFLD_STRING), EXSUCCEED);
    
    assert_equal(CBget(p_ub, T_DOUBLE_3_FLD, 0, buf, 0, BFLD_STRING), EXSUCCEED);
    assert_equal(strncmp(buf, "1.11", 4), 0);
    
    assert_equal(CBadd(p_ub, T_DOUBLE_3_FLD, "2.41231", 0, BFLD_STRING), EXSUCCEED);
    
    assert_equal(CBget(p_ub, T_DOUBLE_3_FLD, 1,buf, 0, BFLD_STRING), EXSUCCEED);
    assert_equal(strncmp(buf, "2.41231", 4), 0);
    

    assert_equal(CBadd(p_ub, T_STRING_4_FLD, "HELLO WORLD", 0, BFLD_STRING), EXSUCCEED);
    
    assert_equal(CBget(p_ub, T_STRING_4_FLD, 0, buf, 0, BFLD_STRING), EXSUCCEED);
    assert_string_equal(buf, "HELLO WORLD");
    
    
    assert_equal(CBadd(p_ub, T_STRING_8_FLD, "HELLO WORLD1", 0, BFLD_STRING), EXSUCCEED);
    
    assert_equal(CBget(p_ub, T_STRING_8_FLD, 0, buf, 0, BFLD_STRING), EXSUCCEED);
    assert_string_equal(buf, "HELLO WORLD1");
    
    assert_equal(CBadd(p_ub, T_STRING_5_FLD, "HELLO WORLD2", 0, BFLD_STRING), EXSUCCEED);
    
    assert_string_equal(buf, "HELLO WORLD1");
    assert_equal(CBget(p_ub, T_STRING_5_FLD, 0, buf, 0, BFLD_STRING), EXSUCCEED);
    
    assert_equal(CBadd(p_ub, T_STRING_7_FLD, "HELLO WORLD3", 0, BFLD_STRING), EXSUCCEED);
    
    assert_equal(CBget(p_ub, T_STRING_7_FLD, 0, buf, 0, BFLD_STRING), EXSUCCEED);
    assert_string_equal(buf, "HELLO WORLD3");
    
    assert_equal(CBadd(p_ub, T_STRING_6_FLD, "HELLO WORLD4", 0, BFLD_STRING), EXSUCCEED);
    
    assert_equal(CBget(p_ub, T_STRING_6_FLD, 0, buf, 0, BFLD_STRING), EXSUCCEED);
    assert_string_equal(buf, "HELLO WORLD4");
    
    assert_equal(CBadd(p_ub, T_STRING_10_FLD, "HELLO WORLD5", 0, BFLD_STRING), EXSUCCEED);
    assert_equal(CBget(p_ub, T_STRING_10_FLD, 0, buf, 0, BFLD_STRING), EXSUCCEED);
    assert_string_equal(buf, "HELLO WORLD5");
    
    
    assert_equal(CBadd(p_ub, T_STRING_4_FLD, "HELLO WORLD6", 0, BFLD_STRING), EXSUCCEED);
    
    assert_equal(CBget(p_ub, T_STRING_4_FLD, 1, buf, 0, BFLD_STRING), EXSUCCEED);
    assert_string_equal(buf, "HELLO WORLD6");
    
    assert_equal(CBadd(p_ub, T_STRING_3_FLD, "HELLO WORLD7", 0, BFLD_STRING), EXSUCCEED);
    
    assert_equal(CBget(p_ub, T_STRING_3_FLD, 0, buf, 0, BFLD_STRING), EXSUCCEED);
    assert_string_equal(buf, "HELLO WORLD7");
    
    assert_equal(CBchg(p_ub, T_CARRAY_2_FLD, 1, "TEST CARRAY", 0, BFLD_STRING), EXSUCCEED);
    
    /* Another carray test */
    len = sizeof(buf);
    assert_equal(CBget(p_ub, T_CARRAY_2_FLD, 1, buf, 0, BFLD_STRING), EXSUCCEED);
    assert_string_equal(buf, "TEST CARRAY");
    
    /* Load some PTR, VIEW and UBF */
    if (flags & EXTEST_PROC_PTR)
    {
        ptr=199;
        assert_equal(CBchg(p_ub, T_PTR_3_FLD, 0, (char *)&ptr, 0L, BFLD_LONG), EXSUCCEED);
        ptr=299;
        assert_equal(CBchg(p_ub, T_PTR_3_FLD, 1, (char *)&ptr, 0L, BFLD_LONG), EXSUCCEED);
    }
    
    if (flags & EXTEST_PROC_VIEW)
    {
        NDRX_STRCPY_SAFE(vf.vname, "UBTESTVIEW2");
        vf.vflags=0;
        vf.data = (char *)&v;

        v.tshort1=100;
        v.tlong1=200;
        v.tchar1='G';
        v.tfloat1=400;
        v.tdouble1=500;
        NDRX_STRCPY_SAFE(v.tstring1, "6XX");
        NDRX_STRCPY_SAFE(v.tcarray1, "7XX");

        assert_equal(Bchg(p_ub, T_VIEW_3_FLD, 0, (char *)&vf, 0L), EXSUCCEED);

        NDRX_STRCPY_SAFE(v.tstring1, "6YY");
        NDRX_STRCPY_SAFE(v.tcarray1, "7YY");
        v.tchar1='Z';
        assert_equal(Bchg(p_ub, T_VIEW_3_FLD, 4, (char *)&vf, 0L), EXSUCCEED);
    }
    
    if (flags & EXTEST_PROC_UBF)
    {
        /* And some ubf... */
        assert_equal(Binit(p_ub_tmp, sizeof(tmp)), EXSUCCEED);
        assert_equal(Bchg(p_ub_tmp, T_STRING_9_FLD, 3, "HELLO WORLD UB", 0L), EXSUCCEED);
        assert_equal(Bchg(p_ub, T_UBF_3_FLD, 2, (char *)p_ub_tmp, 0L), EXSUCCEED);

        assert_equal(Bchg(p_ub_tmp, T_STRING_7_FLD, 2, "ANOTHER UB", 0L), EXSUCCEED);
        assert_equal(Bchg(p_ub, T_UBF_3_FLD, 3, (char *)p_ub_tmp, 0L), EXSUCCEED);

        l=889991;
        assert_equal(Bchg(p_ub, T_LONG_3_FLD, 3, (char *)&l, 0L), EXSUCCEED);
    }
    
}

/* vim: set ts=4 sw=4 et smartindent: */
