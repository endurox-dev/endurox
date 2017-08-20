/* 
** View unit tests.
**
** @file viewunit1.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, Mavimax, Ltd. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or Mavimax's license for commercial use.
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
** A commercial use license is available from Mavimax, Ltd
** contact@mavimax.com
** -----------------------------------------------------------------------------
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cgreen/cgreen.h>
#include <ubf.h>
#include <ndrstandard.h>
#include <string.h>
#include "test.fd.h"
#include "ubfunit1.h"
#include "ndebug.h"
#include <fdatatype.h>

#include "test040.h"

/**
 * Basic preparation before the test
 */
void basic_setup(void)
{
    
}

void basic_teardown(void)
{
    
}


/**
 * Test NULL column
 */
Ensure(test_Bvnull)
{
    struct MYVIEW1 v;
    int i;
    char *spec_symbols = "\n\t\f\\\'\"\vHELLOWORLD\0";
    memset(&v, 0, sizeof(v));
    
    /***************************** SHORT TESTS *******************************/
    /* These fields shall not be NULL as NULL defined */
    assert_equal(Bvnull((char *)&v, "tshort2", 0, "MYVIEW1"), EXFALSE);
    assert_equal(Bvnull((char *)&v, "tshort2", 1, "MYVIEW1"), EXFALSE);
    
    /* Set to NULL */
    v.tshort2[1]=2001;
    assert_equal(Bvnull((char *)&v, "tshort2", 0, "MYVIEW1"), EXFALSE);
    assert_equal(Bvnull((char *)&v, "tshort2", 1, "MYVIEW1"), EXTRUE);
    
    /*  Set all to NULL */
    assert_equal(Bvselinit((char *)&v,"tshort2", "MYVIEW1"), EXSUCCEED);
    assert_equal(Bvnull((char *)&v, "tshort2", 0, "MYVIEW1"), EXTRUE);
    assert_equal(Bvnull((char *)&v, "tshort2", 1, "MYVIEW1"), EXTRUE);
    
    
    assert_equal(Bvnull((char *)&v, "tshort3", 0, "MYVIEW1"), EXTRUE);
    assert_equal(Bvnull((char *)&v, "tshort3", 1, "MYVIEW1"), EXTRUE);
    assert_equal(Bvnull((char *)&v, "tshort3", 2, "MYVIEW1"), EXTRUE);
    
    v.tshort3[0] = 1;
    v.tshort3[2] = 2;
    
    assert_equal(Bvnull((char *)&v, "tshort3", 0, "MYVIEW1"), EXFALSE);
    assert_equal(Bvnull((char *)&v, "tshort3", 1, "MYVIEW1"), EXTRUE);
    assert_equal(Bvnull((char *)&v, "tshort3", 2, "MYVIEW1"), EXFALSE);
    
    /* this is NONE, field set to 0, but as NONE is there it is not NULL */
    assert_equal(Bvnull((char *)&v, "tshort4", 0, "MYVIEW1"), EXFALSE);
    
    /***************************** LONG TESTS *******************************/
    
    assert_equal(Bvnull((char *)&v, "tlong1", 0, "MYVIEW1"), EXTRUE);
    
    /***************************** INT TESTS ********************************/
    
    assert_equal(Bvnull((char *)&v, "tint2", 0, "MYVIEW1"), EXTRUE);
    assert_equal(Bvnull((char *)&v, "tint2", 1, "MYVIEW1"), EXTRUE);
    assert_equal(Bvnull((char *)&v, "tint3", 0, "MYVIEW1"), EXFALSE);
    assert_equal(Bvselinit((char *)&v,"tint3", "MYVIEW1"), EXSUCCEED);
    assert_equal(Bvnull((char *)&v, "tint3", 0, "MYVIEW1"), EXTRUE);
    
    assert_equal(v.tint3, -1);
    
    assert_equal(Bvnull((char *)&v, "tint4", 0, "MYVIEW1"), EXFALSE);
    assert_equal(Bvnull((char *)&v, "tint4", 1, "MYVIEW1"), EXFALSE);
    
    assert_equal(v.tint4[0], 0);
    assert_equal(v.tint4[1], 0);
    
    assert_equal(Bvselinit((char *)&v,"tint4", "MYVIEW1"), EXSUCCEED);
    
    assert_equal(Bvnull((char *)&v, "tint4", 0, "MYVIEW1"), EXTRUE);
    assert_equal(Bvnull((char *)&v, "tint4", 1, "MYVIEW1"), EXTRUE);
    
    /***************************** CHAR TESTS ********************************/
    
    UBF_LOG(log_debug, "tchar1=%x", v.tchar1);
    assert_equal(Bvnull((char *)&v, "tchar1", 0, "MYVIEW1"), EXFALSE);
    
    v.tchar1 = '\n';
    assert_equal(Bvnull((char *)&v, "tchar1", 0, "MYVIEW1"), EXTRUE);
    v.tchar1 = 0;
    assert_equal(Bvnull((char *)&v, "tchar1", 0, "MYVIEW1"), EXFALSE);
    assert_equal(Bvselinit((char *)&v,"tchar1", "MYVIEW1"), EXSUCCEED);
    
    UBF_LOG(log_debug, "tchar1=%x", v.tchar1);
    assert_equal(Bvnull((char *)&v, "tchar1", 0, "MYVIEW1"), EXTRUE);
    
    
    /* Here default is 'A' */
    for (i=0;i<5;i++)
    {
        assert_equal(Bvnull((char *)&v, "tchar2", i, "MYVIEW1"), EXFALSE);
    }
    
    assert_equal(Bvselinit((char *)&v,"tchar2", "MYVIEW1"), EXSUCCEED);
    
    for (i=0;i<5;i++)
    {
        assert_equal(Bvnull((char *)&v, "tchar2", i, "MYVIEW1"), EXTRUE);
    }
    
    for (i=0;i<5;i++)
    {
        assert_equal(v.tchar2[i], 'A');
    }
    
    /* Here default is zero byte */
    
    for (i=0;i<2;i++)
    {
        assert_equal(v.tchar3[i], 0);
    }
    
    /***************************** FLOAT TESTS *******************************/
    for (i=0;i<4;i++)
    {
        assert_equal(Bvnull((char *)&v, "tfloat1", 0, "MYVIEW1"), EXFALSE);
    }
    
    assert_equal(Bvselinit((char *)&v,"tfloat1", "MYVIEW1"), EXSUCCEED);
    
    for (i=0;i<4;i++)
    {
        assert_equal(Bvnull((char *)&v, "tfloat1", i, "MYVIEW1"), EXTRUE);
    }
    
    for (i=0;i<4;i++)
    {
        assert_equal(v.tfloat1[i], 1.1);
    }
    
    /* occ: 0..3*/
    assert_equal(Bvnull((char *)&v, "tfloat1", 4, "MYVIEW1"), EXFAIL);
    assert_equal(Berror, BEINVAL);
    
    /* Default set by memset... */
    for (i=0;i<2;i++)
    {
        assert_equal(Bvnull((char *)&v, "tfloat2", i, "MYVIEW1"), EXTRUE);
        assert_equal(Berror, 0);
    }
    
    assert_equal(Bvnull((char *)&v, "tfloat3", 0, "MYVIEW1"), EXFALSE);
    assert_equal(v.tfloat3, 0);
    
    assert_equal(Bvselinit((char *)&v,"tfloat3", "MYVIEW1"), EXSUCCEED);
    assert_equal(v.tfloat3, 9999.99);
    assert_equal(Bvnull((char *)&v, "tfloat3", 0, "MYVIEW1"), EXTRUE);
    
    
    /***************************** DOUBLE TESTS *******************************/
    for (i=0;i<2;i++)
    {
        assert_equal(Bvnull((char *)&v, "tdouble1", i, "MYVIEW1"), EXFALSE);
    }
    
    assert_equal(Bvselinit((char *)&v,"tdouble1", "MYVIEW1"), EXSUCCEED);
    
    for (i=0;i<2;i++)
    {
        assert_equal(Bvnull((char *)&v, "tdouble1", i, "MYVIEW1"), EXTRUE);
        assert_equal(v.tdouble1[i], 55555.99);
    }
    
    assert_equal(Bvnull((char *)&v, "tdouble2", 0, "MYVIEW1"), EXFALSE);
    assert_equal(Bvselinit((char *)&v,"tdouble2", "MYVIEW1"), EXSUCCEED);
    assert_equal(Bvnull((char *)&v, "tdouble2", 0, "MYVIEW1"), EXTRUE);
    assert_equal(v.tdouble2, -999.123);
    
    /***************************** STRING TESTS *******************************/
    /* Fill the string 1 in advance.. */
    assert_equal(Bvselinit((char *)&v,"tstring1", "MYVIEW1"), EXSUCCEED);
    
    /* Test filler: */
    for (i=0;i<3;i++)
    {
        UBF_LOG(log_debug, "tstring1=[%s]", v.tstring1[i]);
        
        assert_equal(Bvnull((char *)&v, "tstring1", i, "MYVIEW1"), EXTRUE);
    }
    
    /* Test special symbols... */
    for (i=0;i<3;i++)
    {
        assert_equal(Bvnull((char *)&v, "tstring0", i, "MYVIEW1"), EXFALSE);
    }
    
    assert_equal(Bvselinit((char *)&v,"tstring0", "MYVIEW1"), EXSUCCEED);
    
    UBF_DUMP(log_debug, "Special symbols test...", spec_symbols, strlen(spec_symbols));
    
    for (i=0;i<3;i++)
    {
        UBF_LOG(log_debug, "tstring0=[%s]", v.tstring0[i]);
        
        UBF_DUMP(log_debug, "testing0", v.tstring0[i], strlen(v.tstring0[i]));
        
        UBF_DUMP_DIFF(log_debug, "diff", spec_symbols, v.tstring0[i], strlen(spec_symbols));
        
        assert_equal(Bvnull((char *)&v, "tstring0", i, "MYVIEW1"), EXTRUE);
    }
    
    assert_string_equal(v.tstring0[0], spec_symbols);
    
    /* Continue with filler... */
    
    for (i=0;i<3;i++)
    {
        assert_string_equal(v.tstring1[i], "HELLO WORLDBBBBBBBB");
    }
    
    for (i=0;i<3;i++)
    {
        assert_equal(Bvnull((char *)&v, "tstring2", i, "MYVIEW1"), EXTRUE);
    }
    
    for (i=0;i<4;i++)
    {
        assert_equal(Bvnull((char *)&v, "tstring3", i, "MYVIEW1"), EXFALSE);
    }
    
    assert_equal(Bvselinit((char *)&v,"tstring3", "MYVIEW1"), EXSUCCEED);
    
    for (i=0;i<4;i++)
    {
        assert_equal(Bvnull((char *)&v, "tstring3", i, "MYVIEW1"), EXTRUE);
    }
    
    for (i=0;i<4;i++)
    {
        assert_string_equal(v.tstring3[i], "TESTEST");
    }
    
    assert_equal(Bvnull((char *)&v, "tstring4", 0, "MYVIEW1"), EXFALSE);
    assert_equal(Bvselinit((char *)&v,"tstring4", "MYVIEW1"), EXSUCCEED);
    assert_equal(Bvnull((char *)&v, "tstring4", 0, "MYVIEW1"), EXTRUE);
    assert_string_equal(v.tstring4, "HELLO TESTTTTT");
    
    
    /***************************** CARRAY TESTS *******************************/
    
    assert_equal(Bvnull((char *)&v, "tcarray1", 0, "MYVIEW1"), EXFALSE);
    assert_equal(Bvselinit((char *)&v,"tcarray1", "MYVIEW1"), EXSUCCEED);
    assert_equal(Bvnull((char *)&v, "tcarray1", 0, "MYVIEW1"), EXTRUE);
    assert_equal(memcmp(v.tcarray1, "\0\n\t\f\\\'\"\vHELLOWORLD", 18), 0);
    
    assert_equal(Bvnull((char *)&v, "tcarray2", 0, "MYVIEW1"), EXFALSE);
    assert_equal(Bvselinit((char *)&v,"tcarray2", "MYVIEW1"), EXSUCCEED);
    assert_equal(Bvnull((char *)&v, "tcarray2", 0, "MYVIEW1"), EXTRUE);
    assert_equal(memcmp(v.tcarray2, "\0\n\t\f\\\'\"\vHELLOWORL\n\n\n\n\n\n\n\n'", 25), 0);
    
    for (i=0; i<10; i++)
    {
        assert_equal(v.tcarray3[i][0], 0);
    }
    
    assert_equal(Bvselinit((char *)&v, "tcarray3", "MYVIEW1"), EXSUCCEED);
    
    for (i=0; i<10; i++)
    {
        assert_equal(memcmp(v.tcarray3[i], "\0\\\nABC\t\f\'\vHELLOOOOOOOOOOOOOOOOOOO", 30), 0);
    }
    
    assert_equal(Bvselinit((char *)&v, "tcarray4", "MYVIEW1"), EXSUCCEED);
    assert_equal(Bvnull((char *)&v, "tcarray4", 0, "MYVIEW1"), EXTRUE);
    assert_equal(memcmp(v.tcarray4, "ABC", 3), 0);
    
    
    assert_equal(Bvselinit((char *)&v, "tcarray5", "MYVIEW1"), EXSUCCEED);
    assert_equal(Bvnull((char *)&v, "tcarray5", 0, "MYVIEW1"), EXTRUE);
    
    assert_equal(memcmp(v.tcarray5, "\0\0\0\0\0", 5), 0);
    
}

Ensure(test_Bvsinit)
{
    struct MYVIEW2 v;
   
    memset(&v, 0, sizeof(v));
    assert_equal(Bvsinit((char *)&v, "MYVIEW2"), EXSUCCEED);
    
    assert_equal(Bvnull((char *)&v, "tshort1", 0, "MYVIEW2"), EXTRUE);
    assert_equal(Bvnull((char *)&v, "tlong1", 0, "MYVIEW2"), EXTRUE);
    assert_equal(Bvnull((char *)&v, "tchar1", 0, "MYVIEW2"), EXTRUE);
    assert_equal(Bvnull((char *)&v, "tfloat1", 0, "MYVIEW2"), EXTRUE);
    assert_equal(Bvnull((char *)&v, "tdouble1", 0, "MYVIEW2"), EXTRUE);
    assert_equal(Bvnull((char *)&v, "tstring1", 0, "MYVIEW2"), EXTRUE);
    assert_equal(Bvnull((char *)&v, "tcarray1", 0, "MYVIEW2"), EXTRUE); 
}

Ensure(test_Bvrefresh)
{
    Bvrefresh();
}

/**
 * Install structure to UBF
 */
Ensure(test_Bvstof)
{
    struct MYVIEW1 v;
    char buf[2048];
    BFLDID bfldid;
    BFLDOCC occ;
    int flds_got;
    UBFH *p_ub = (UBFH *)buf;
    char tmp[128];
    double dtemp;
    BFLDLEN len;
    int i, j;
    /***************************** TEST EMPTY STRUCT **************************/
    assert_equal(Binit(p_ub, sizeof(buf)), EXSUCCEED);
    
    /* Set to NULL */
    assert_equal(Bvsinit((char *)&v, "MYVIEW1"), EXSUCCEED);
    
    /* transfer to UBF.. */
    assert_equal(Bvstof(p_ub, (char *)&v, BUPDATE, "MYVIEW1"), EXSUCCEED);
    
    /* not fields must be transfered. */
    bfldid = BFIRSTFLDID;
    flds_got = 0;
    while(1==Bnext(p_ub, &bfldid, &occ, NULL, NULL))
    {
        flds_got++;
    }

    assert_equal(flds_got, 0);
    
    
    /***************************** TEST FILLED UBF ****************************/
    
    /* Load test value... */
    init_MYVIEW1(&v);
    
    assert_equal(Bvstof(p_ub, (char *)&v, BUPDATE, "MYVIEW1"), EXSUCCEED);
    
    bfldid = BFIRSTFLDID;
    while(1==Bnext(p_ub, &bfldid, &occ, NULL, NULL))
    {
        flds_got++;
    }

    assert_not_equal(flds_got, 0);
    
    /* Test values... */
    
    /*
     * short
     */
    TEST_AS_STRING(T_SHORT_FLD, 0, "15556");
    
    TEST_AS_STRING(T_SHORT_2_FLD, 0, "9999");
    TEST_AS_STRING(T_SHORT_2_FLD, 1, "8888");
    
    
    TEST_AS_STRING(T_SHORT_3_FLD, 0, "7777");
    TEST_AS_STRING(T_SHORT_3_FLD, 1, "-7777");
    assert_equal(Bpres(p_ub, T_SHORT_3_FLD, 2),  EXFALSE);
    
    /*
     * long
     */
    TEST_AS_STRING(T_LONG_FLD, 0, "33333333");
    
    /*
     * Int
     */
    TEST_AS_STRING(T_LONG_2_FLD, 0, "54545");
    TEST_AS_STRING(T_LONG_2_FLD, 1, "23232");
    
    /*
     * Char
     */
    TEST_AS_STRING(T_CHAR_FLD, 0, "A");
    assert_equal(Bpres(p_ub, T_CHAR_2_FLD, 0),  EXFALSE);
    assert_equal(Bpres(p_ub, T_CHAR_3_FLD, 0),  EXFALSE);
    
    /*
     * Float tests
     */
    TEST_AS_DOUBLE(T_FLOAT_FLD, 0, -0.11);
    TEST_AS_DOUBLE(T_FLOAT_FLD, 1, -0.22);
    TEST_AS_DOUBLE(T_FLOAT_FLD, 2, 0.33);
    TEST_AS_DOUBLE(T_FLOAT_FLD, 3, 0.44);
    assert_equal(Bpres(p_ub, T_FLOAT_2_FLD, 0),  EXFALSE);
    
    /*
     * Double tests
     */
    TEST_AS_DOUBLE(T_DOUBLE_FLD, 0, 99999.111111);
    TEST_AS_DOUBLE(T_DOUBLE_FLD, 1, 11111.999999);
    assert_equal(Bpres(p_ub, T_DOUBLE_FLD, 2),  EXFALSE);
    
    /* Not transfered due to default value NULL: */
    assert_equal(Bpres(p_ub, T_DOUBLE_2_FLD, 0),  EXFALSE);
    
    /*
     * String
     */
    TEST_AS_STRING(T_STRING_FLD, 0, "Pack my box");
    TEST_AS_STRING(T_STRING_FLD, 1, "BOX MY PACK");
    TEST_AS_STRING(T_STRING_FLD, 2, "\nEnduro/X\n");
    assert_equal(Bpres(p_ub, T_STRING_FLD, 3),  EXFALSE);
    
    TEST_AS_STRING(T_STRING_2_FLD, 0, "CCCCAAAADDDD");
    assert_equal(v.L_tstring2[0], 13);
    
    TEST_AS_STRING(T_STRING_2_FLD, 1, "EEEFFFGGG");
    assert_equal(v.L_tstring2[1], 10);
    
    assert_equal(Bpres(p_ub, T_STRING_2_FLD, 2),  EXFALSE);
    
    
    TEST_AS_STRING(T_STRING_3_FLD, 0, "LLLLLL");
    TEST_AS_STRING(T_STRING_3_FLD, 1, "MMMMMM");
    TEST_AS_STRING(T_STRING_3_FLD, 2, "");
    TEST_AS_STRING(T_STRING_3_FLD, 3, "NNNNNN");
    
    assert_equal(Bpres(p_ub, T_STRING_3_FLD, 4),  EXFALSE);
    
    /*
     * Carray test
     */
    GET_CARRAY_DOUBLE_TEST_LEN(T_CARRAY_FLD, 0, 30)
    for (i=0; i<30; i++)
    {
        assert_equal(v.tcarray1[i], i);
    }
    
    GET_CARRAY_DOUBLE_TEST_LEN(T_CARRAY_FLD, 0, 30)
    for (i=0; i<30; i++)
    {
        assert_equal(tmp[i], i);
    }
    
    GET_CARRAY_DOUBLE_TEST_LEN(T_CARRAY_2_FLD, 0, 5)
    for (i=0; i<5; i++)
    {
        assert_equal(tmp[i], i+1);
    }
    
    for (j=0; j<9; j++)
    {
        UBF_LOG(log_debug, "Testing j=%d", j);
        GET_CARRAY_DOUBLE_TEST_LEN(T_CARRAY_3_FLD, j, j+1)
                
        for (i=0; i<j; i++)
        {
            assert_equal(tmp[i], i+2);
        }
    }
    assert_equal(Bpres(p_ub, T_CARRAY_3_FLD, 9),  EXFALSE);
    
}
/**
 * Transfer UBF buffer to C struct..
 * We will use previously initialised buffer by Bvstof
 */
Ensure(test_Bvftos)
{
    struct MYVIEW1 v;
    char buf[2048];
    BFLDID bfldid;
    BFLDOCC occ;
    int flds_got;
    UBFH *p_ub = (UBFH *)buf;
    char tmp[128];
    double dtemp;
    BFLDLEN len;
    int i, j;
    /***************************** TEST EMPTY STRUCT **************************/
    assert_equal(Binit(p_ub, sizeof(buf)), EXSUCCEED);
    
    /* Load test value... */
    init_MYVIEW1(&v);
    
    /* transfer to UBF.. */
    assert_equal(Bvstof(p_ub, (char *)&v, BCONCAT, "MYVIEW1"), EXSUCCEED);
    
    memset(&v, 0, sizeof(v));
    
    /* First will go as NULL */
    assert_equal(CBchg(p_ub, T_CHAR_2_FLD, 0, "A", 0L, BFLD_STRING), EXSUCCEED);
    assert_equal(CBchg(p_ub, T_CHAR_2_FLD, 1, "B", 0L, BFLD_STRING), EXSUCCEED);
    assert_equal(CBchg(p_ub, T_CHAR_2_FLD, 2, "C", 0L, BFLD_STRING), EXSUCCEED);
    assert_equal(CBchg(p_ub, T_CHAR_2_FLD, 3, "D", 0L, BFLD_STRING), EXSUCCEED);
    assert_equal(CBchg(p_ub, T_CHAR_2_FLD, 4, "E", 0L, BFLD_STRING), EXSUCCEED);
    
    assert_equal(CBchg(p_ub, T_CHAR_3_FLD, 0, "X", 0L, BFLD_STRING), EXSUCCEED);
    assert_equal(CBchg(p_ub, T_CHAR_3_FLD, 0, "Y", 0L, BFLD_STRING), EXSUCCEED);
    
    assert_equal(CBchg(p_ub, T_FLOAT_2_FLD, 0, "44", 0L, BFLD_STRING), EXSUCCEED);
    
    assert_equal(Bvftos(p_ub, (char *)&v, "MYVIEW1"), EXSUCCEED);
    
    /* now in v either we have value from buffer of NULL for the field. */
    
    /*
     * test short
     */
    assert_equal(v.tshort1, 15556);
    
    assert_equal(v.C_tshort2, 2);
    assert_equal(v.tshort2[0], 9999);
    assert_equal(v.tshort2[1], 8888);
    
    assert_equal(v.C_tshort3, 2);
    assert_equal(v.tshort3[0], 7777);
    assert_equal(v.tshort3[1], -7777);
    
    UBF_LOG(log_debug, "tshort4=%hd", v.tshort4);
    
    /* NULL set to NONE */
    assert_equal(Bvnull((char *)&v, "tshort4", 0, "MYVIEW1"), EXFALSE);
    
    /*
     * Long tests
     */
    assert_equal(v.tlong1, 33333333);
    
    /*
     * Int tests
     */
    assert_equal(v.tint2[0], 54545);
    assert_equal(v.tint2[1], 23232);
    
    assert_equal(Bvnull((char *)&v, "tint3", 0, "MYVIEW1"), EXTRUE);
    assert_equal(Bvnull((char *)&v, "tint4", 0, "MYVIEW1"), EXTRUE);
    
    /*
     * Char tests
     */
    
    /* Field is F, no copy back to struct */
    assert_equal(Bvnull((char *)&v, "tchar1", 0, "MYVIEW1"), EXTRUE);
    
    for (i=0; i<5; i++)
    {
        assert_equal(v.tchar2[i], 'A'+i);
    
    }
    
    assert_equal(Bvnull((char *)&v, "tchar2", 0, "MYVIEW1"), EXTRUE);
    
    for (i=0; i<2; i++)
    {
        assert_equal(Bvnull((char *)&v, "tchar3", i, "MYVIEW1"), EXTRUE);
    }
    
    /*
     * Float tests
     */
    assert_double_equal(v.tfloat1[0], -0.11);
    assert_double_equal(v.tfloat1[1], -0.22);
    assert_double_equal(v.tfloat1[2], 0.33);
    assert_double_equal(v.tfloat1[3], 0.44);
    
    assert_double_equal(v.tfloat2[0], 44);
    
    assert_equal(Bvnull((char *)&v, "tfloat2", 1, "MYVIEW1"), EXTRUE);
    
    assert_equal(Bvnull((char *)&v, "tfloat3", 0, "MYVIEW1"), EXTRUE);
    
    
    /*
     * Double tests
     */
    assert_double_equal(v.tdouble1[0], 99999.111111);
    assert_double_equal(v.tdouble1[1], 11111.999999);
    assert_equal(Bvnull((char *)&v, "tdouble2", 0, "MYVIEW1"), EXTRUE);
    
    
    /*
     * String tests
     */
    assert_equal(Bvnull((char *)&v, "tstring0", 0, "MYVIEW1"), EXTRUE);
    
    assert_string_equal(v.tstring1[0], "Pack my box");
    assert_string_equal(v.tstring1[1], "BOX MY PACK");
    assert_string_equal(v.tstring1[2], "\nEnduro/X\n");
    
    assert_equal(Bvnull((char *)&v, "tstring2", 0, "MYVIEW1"), EXTRUE);
    assert_equal(Bvnull((char *)&v, "tstring2", 1, "MYVIEW1"), EXTRUE);
    assert_equal(Bvnull((char *)&v, "tstring2", 2, "MYVIEW1"), EXTRUE);
    
    
    assert_equal(v.L_tstring3[0], 7);
    assert_string_equal(v.tstring3[0], "LLLLLL");
    assert_equal(v.L_tstring3[1], 7);
    assert_string_equal(v.tstring3[1], "MMMMMM");
    
    assert_equal(v.L_tstring3[2], 1); /* EOS */
    assert_string_equal(v.tstring3[2], "");
    
    assert_equal(v.L_tstring3[3], 7); /* EOS */
    assert_string_equal(v.tstring3[3], "NNNNNN");
    
    
    assert_equal(Bvnull((char *)&v, "tstring4", 0, "MYVIEW1"), EXTRUE);
    
    /*
     * Carray tests
     */
    
    for (i=0; i<30; i++)
    {
        assert_equal(v.tcarray1[i], i);
    }
    
    assert_equal(v.L_tcarray2, 5);
    
    for (i=0; i<5; i++)
    {
        assert_equal(v.tcarray2[i], i+1);
    }
    
    assert_equal(v.C_tcarray3, 9);
            
    for (j=0; j<9; j++)
    {
        assert_equal(v.L_tcarray3[j], j+1);
        
        for (i=0; i<j+1; i++)
        {
            assert_equal(v.tcarray3[j][i], i+2);
        }
    }
    
    assert_equal(Bvnull((char *)&v, "tcarray4", 0, "MYVIEW1"), EXTRUE);
    
    assert_equal(Bvnull((char *)&v, "tcarray5", 0, "MYVIEW1"), EXTRUE);
}

/**
 * Very basic tests of the framework
 * @return
 */
TestSuite *view_tests() {
    TestSuite *suite = create_test_suite();
    
    set_setup(suite, basic_setup);
    set_teardown(suite, basic_teardown);

    /* init view test */
    add_test(suite, test_Bvnull);
    add_test(suite, test_Bvsinit);
    add_test(suite, test_Bvrefresh);
    add_test(suite, test_Bvstof);
    add_test(suite, test_Bvftos);
    
    return suite;
}

/*
 * Main test entry.
 */
int main(int argc, char** argv)
{    
    TestSuite *suite = create_test_suite();

    add_suite(suite, view_tests());
    

    if (argc > 1) {
        return run_single_test(suite,argv[1],create_text_reporter());
    }

    return run_test_suite(suite, create_text_reporter());
    
}
