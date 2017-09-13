/* 
** View access unit tests - set field value
**
** @file vaccset.c
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
exprivate void basic_setup(void)
{
    
}

exprivate void basic_teardown(void)
{
    
}

/**
 * Test Bvchg as short
 */
Ensure(test_Bvchg_short)
{
    struct MYVIEW1 v;
    short s;
    BFLDOCC maxocc;
    BFLDOCC realocc;
    long dim_size;
    
    memset(&v, 0, sizeof(v));
            
    s = 15556;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tshort1", 0, (char *)&s, 0L, BFLD_SHORT), 
            EXSUCCEED);
    assert_equal(s, v.tshort1);
    
    /* Invalid value, index out of bounds.. */
    s = 0;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tshort1", 1, (char *)&s, 0L, BFLD_SHORT), 
            EXFAIL);
    assert_equal(Berror, BEINVAL);
    
    s = 9999;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tshort2", 0, (char *)&s, 0L, BFLD_SHORT), 
            EXSUCCEED);
    assert_equal(v.tshort2[0], 9999);
    
    s = 8888;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tshort2", 1, (char *)&s, 0L, BFLD_SHORT), 
            EXSUCCEED);
    assert_equal(v.tshort2[1], 8888);
    
    /* occs 0 */
    assert_equal(Bvoccur((char *)&v, "MYVIEW1", "tshort3", &maxocc, &realocc, &dim_size, NULL), 0);
    assert_equal(maxocc, 3);
    assert_equal(realocc, 0);
            
    s = 7777;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tshort3", 0, (char *)&s, 0L, BFLD_SHORT), 
            EXSUCCEED);
    assert_equal(v.tshort3[0], 7777);
    
    /* TEST: Occurrences 1 */
    assert_equal(Bvoccur((char *)&v, "MYVIEW1", "tshort3", &maxocc, &realocc, &dim_size, NULL), 1);
    assert_equal(maxocc, 3);
    assert_equal(realocc, 1);
    
    s = -7777;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tshort3", 1, (char *)&s, 0L, BFLD_SHORT), 
            EXSUCCEED);
    assert_equal(v.tshort3[1], -7777);
    
    
    /* TEST: Occurrences 2 */
    assert_equal(Bvoccur((char *)&v, "MYVIEW1", "tshort3", &maxocc, &realocc, &dim_size, NULL), 2);
    assert_equal(maxocc, 3);
    assert_equal(realocc, 2);
    
    
    s = -7777;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tshort3", 2, (char *)&s, 0L, BFLD_SHORT), 
            EXSUCCEED);
    assert_equal(v.tshort3[2], -7777);
    
    /* TEST: Occurrences 2 */
    assert_equal(Bvoccur((char *)&v, "MYVIEW1", "tshort3", &maxocc, &realocc, &dim_size, NULL), 3);
    assert_equal(maxocc, 3);
    assert_equal(realocc, 3);
    
    assert_equal(v.C_tshort3, 3);
    
    /* test first two empty occs... */
    
    s = 0;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tshort3", 0, (char *)&s, 0L, BFLD_SHORT), 
            EXSUCCEED);
    assert_equal(v.tshort3[0], 0);
    
    s = 0;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tshort3", 1, (char *)&s, 0L, BFLD_SHORT), 
            EXSUCCEED);
    assert_equal(v.tshort3[1], 0);
    
    assert_equal(Bvoccur((char *)&v, "MYVIEW1", "tshort3", &maxocc, &realocc, &dim_size, NULL), 3);
    assert_equal(maxocc, 3);
    assert_equal(realocc, 3);
    
    s = 0;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tshort3", 2, (char *)&s, 0L, BFLD_SHORT), 
            EXSUCCEED);
    assert_equal(v.tshort3[2], 0);
    
    assert_equal(Bvoccur((char *)&v, "MYVIEW1", "tshort3", &maxocc, &realocc, &dim_size, NULL), 3);
    assert_equal(maxocc, 3);
    assert_equal(realocc, 0);
    
    s = -10;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tshort4", 0, (char *)&s, 0L, BFLD_SHORT), 
            EXSUCCEED);
    assert_equal(v.tshort4, -10);
    
    
    s = 255;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tlong1", 0, (char *)&s, 0L, BFLD_SHORT), 
            EXSUCCEED);
    assert_equal(v.tlong1, 255);
    
    s = -100;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tint3", 0, (char *)&s, 0L, BFLD_SHORT), 
            EXSUCCEED);
    assert_equal(v.tint3, -100);
    
    /* non existent field: */
    s = 0;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "xint3", 0, (char *)&s, 0L, BFLD_SHORT), 
            EXFAIL);
    assert_equal(Berror, BNOCNAME);
    
    /* Test field NULL */
    s = -1;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tint4", 0, (char *)&s, 0L, BFLD_SHORT), 
            EXSUCCEED);
    assert_equal(v.tint4[0], -1);
    
    s = -1;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tint4", 1, (char *)&s, 0L, BFLD_SHORT), 
            EXSUCCEED);
    assert_equal(v.tint4[1], -1);
    
    assert_equal(Bvoccur((char *)&v, "MYVIEW1", "tint4", &maxocc, &realocc, &dim_size, NULL), 2);
    assert_equal(maxocc, 2);
    assert_equal(realocc, 0);
    
    /* Test the ascii value to char */
    s = 65;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tchar1", 0, (char *)&s, 0L, 
            BFLD_SHORT), 
            EXSUCCEED);
    assert_equal(v.tchar1, 65);
    
    s = 'D';
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tchar3", 1, (char *)&s, 0L, 
            BFLD_SHORT), 
            EXSUCCEED);
    assert_equal(v.tchar3[1], 'D');
    

    s = 66;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tfloat1", 1, (char *)&s, 0L, 
            BFLD_SHORT), 
            EXSUCCEED);
    assert_double_equal(v.tfloat1[1], 66);
    
    
    s = -999;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tdouble2", 0, (char *)&s, 0L, 
            BFLD_SHORT), 
            EXSUCCEED);
    assert_double_equal(v.tdouble2,-999);
    
    
    s = 125;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tstring0", 0, (char *)&s, 0L, 
            BFLD_SHORT), 
            EXSUCCEED);
    assert_string_equal(v.tstring0[0], "125");
    
    
    s = 6500;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tstring2", 1, (char *)&s, 0L, 
            BFLD_SHORT), 
            EXSUCCEED);
    assert_string_equal(v.tstring2[1], "6500");
    assert_equal(v.L_tstring2[1], 5); /* bytes copied... */
    
    s=521;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tcarray2", 0, (char *)&s, 0L, 
            BFLD_SHORT), 
            EXSUCCEED);
    v.tcarray2[3] = EXEOS;
    assert_string_equal(v.tcarray2, "521");
    assert_equal(v.L_tcarray2, 3); /* bytes copied */
    
}

/**
 * Test Bvchg as long
 */
Ensure(test_Bvchg_long)
{
    struct MYVIEW1 v;
    long l;
    
    memset(&v, 0, sizeof(v));
    
    l = 15556;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tshort1", 0, (char *)&l, 0L, BFLD_LONG), 
            EXSUCCEED);
    assert_equal(v.tshort1, 15556);
    
    /* Invalid value, index out of bounds.. */
    l = 0;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tshort1", 1, (char *)&l, 0L, BFLD_LONG), 
            EXFAIL);
    assert_equal(Berror, BEINVAL);
    
    l = 9999;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tshort2", 0, (char *)&l, 0L, BFLD_LONG), 
            EXSUCCEED);
    assert_equal(v.tshort2[0], 9999);
    
    l = 8888;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tshort2", 1, (char *)&l, 0L, BFLD_LONG), 
            EXSUCCEED);
    assert_equal(v.tshort2[1], 8888);
    
    l = 7777;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tshort3", 0, (char *)&l, 0L, BFLD_LONG), 
            EXSUCCEED);
    assert_equal(v.tshort3[0], 7777);
    
    
    l = 33333333;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tlong1", 0, (char *)&l, 0L, BFLD_LONG), 
            EXSUCCEED);
    assert_equal(v.tlong1, 33333333);
    
    l = -100;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tint3", 0, (char *)&l, 0L, BFLD_LONG), 
            EXSUCCEED);
    l = 1010101;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tint4", 0, (char *)&l, 0L, BFLD_LONG), 
            EXSUCCEED);
    assert_equal(v.tint3, -100);
    assert_equal(v.tint4[0], 1010101);
    
    l = 989898;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tint4", 1, (char *)&l, 0L, BFLD_LONG), 
            EXSUCCEED);
    assert_equal(v.tint4[1], 989898);
    
    /* non existent field: */
    l = 0;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "xint3", 0, (char *)&l, 0L, BFLD_LONG), 
            EXFAIL);
    assert_equal(Berror, BNOCNAME);
    
    
    /* Test the ascii value to char */
    l = 65;
    
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tchar1", 0, (char *)&l, 0L, 
            BFLD_LONG), 
            EXSUCCEED);
    assert_equal(v.tchar1, 65);
    
    l = 67;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tchar2", 2, (char *)&l, 0L, 
            BFLD_LONG), 
            EXSUCCEED);
    l = 66;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tchar2", 1, (char *)&l, 0L, 
            BFLD_LONG), 
            EXSUCCEED);
    
    assert_equal(v.tchar2[2], 67);
    assert_equal(v.tchar2[1], 66);
    
    
    l = 66;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tfloat1", 1, (char *)&l, 0L, 
            BFLD_LONG), 
            EXSUCCEED);
    assert_double_equal(v.tfloat1[1], 66);
    
    l = 65;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tfloat1", 1, (char *)&l, 0L, 
            BFLD_LONG), 
            EXSUCCEED);
    assert_double_equal(v.tfloat1[1], 65);
   
    
    l = 11111;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tdouble1", 1, (char *)&l, 0L, 
            BFLD_LONG), 
            EXSUCCEED);
    assert_double_equal(v.tdouble1[1],11111);
    
    
    l = 125;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tstring0", 1, (char *)&l, 0L, 
            BFLD_LONG), 
            EXSUCCEED);
    assert_string_equal(v.tstring0[1], "125");
    
    
    l = 521;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tcarray2", 0, (char *)&l, 0L, 
            BFLD_LONG), 
            EXSUCCEED);
    
    v.tcarray2[3] = EXEOS;
    assert_string_equal(v.tcarray2, "521");
    
    
    l = 123456;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tcarray4", 0, (char *)&l, 0L, 
            BFLD_LONG), 
            EXFAIL);
    
    assert_equal(Berror, BNOSPACE);
    
    
}

/**
 * Test Bvchg as char
 */
Ensure(test_Bvchg_char)
{
    struct MYVIEW1 v;
    init_MYVIEW1(&v);
    char c;
    short s;
    long l;
   
    c = 65;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tshort1", 0, (char *)&c, 0L, BFLD_CHAR), 
            EXSUCCEED);
    assert_equal(v.tshort1, 'A');
    
    /* Invalid value, index out of bounds.. */
    c = 0;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tshort1", 1, (char *)&c, 0L, BFLD_CHAR), 
            EXFAIL);
    assert_equal(Berror, BEINVAL);
    
    
    c = 'a';
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tshort2", 0, (char *)&c, 0L, BFLD_CHAR), 
            EXSUCCEED);
    assert_equal(v.tshort2[0], 'a');
   
    c = 'b';
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tlong1", 0, (char *)&c, 0L, BFLD_CHAR), 
            EXSUCCEED);
    assert_equal(v.tlong1, 'b');
    
    
    c = 90;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tint3", 0, (char *)&c, 0L, BFLD_CHAR), 
            EXSUCCEED);
    assert_equal(v.tint3, 'Z');
    
    
    c = 89;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tint4", 1, (char *)&c, 0L, BFLD_CHAR), 
            EXSUCCEED);
    assert_equal(v.tint4[1], 'Y');
    
    /* non existent field: */
    c = 0;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "xint3", 0, (char *)&c, 0L, BFLD_CHAR), 
            EXFAIL);
    assert_equal(Berror, BNOCNAME);
    
    
    c = 'i';
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tint4", 1, (char *)&c, 0L, BFLD_CHAR), 
            EXSUCCEED);
    assert_equal(v.tint4[1], 'i');
    
    c = 'h';
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tint4", 0, (char *)&c, 0L, BFLD_CHAR), 
            EXSUCCEED);
    assert_equal(v.tint4[0], 'h');
    
    /* Test the ascii value to char */
    c =  65;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tchar1", 0, (char *)&c, 0L, 
            BFLD_CHAR), 
            EXSUCCEED);
    assert_equal(v.tchar1, 65);
    

    c = 66;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tfloat1", 1, (char *)&c, 0L, 
            BFLD_CHAR), 
            EXSUCCEED);
    assert_equal(v.tfloat1[1], 66);
    
    c = 'D';
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tdouble1", 1, (char *)&c, 0L, 
            BFLD_CHAR), 
            EXSUCCEED);
    assert_equal(v.tdouble1[1],'D');
    

    c = 'X';
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tstring0", 0, (char *)&c, 0L, 
            BFLD_CHAR), 
            EXSUCCEED);
    
    assert_string_equal(v.tstring0[0],"X");
    
    c = 'Y';
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tstring2", 2, (char *)&c, 0L, 
            BFLD_CHAR), 
            EXSUCCEED);
    
    assert_string_equal(v.tstring2[2],"Y");
    assert_equal(v.L_tstring2[2], 2); /* two bytes copied. */
    
    /* Does not use EOS, just take first symbol.. */
    c='M';
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tcarray2", 0, (char *)&c, 0L, 
            BFLD_CHAR), 
            EXSUCCEED);
    assert_equal(v.tcarray2[0], 'M');
    assert_equal(v.L_tcarray2, 1);
    
}

/**
 * Test Bvchg as float
 */
Ensure(test_Bvchg_float)
{
    struct MYVIEW1 v;
    init_MYVIEW1(&v);
    float f;
    
    f = 1555;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tshort1", 0, (char *)&f, 0L, BFLD_FLOAT), 
            EXSUCCEED);
    assert_equal(v.tshort1, 1555);
    
    /* Invalid value, index out of bounds.. */
    f = 0;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tshort1", 1, (char *)&f, 0L, BFLD_FLOAT), 
            EXFAIL);
    assert_double_equal(Berror, BEINVAL);
    
    
    f = -8888;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tshort3", 2, (char *)&f, 0L, BFLD_FLOAT), 
            EXSUCCEED);
    f = -7777;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tshort3", 1, (char *)&f, 0L, BFLD_FLOAT), 
            EXSUCCEED);
    
    assert_equal(v.tshort3[2], -8888);
    assert_equal(v.tshort3[1], -7777);
    
    f = 333333;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tlong1", 0, (char *)&f, 0L, BFLD_FLOAT), 
            EXSUCCEED);
    assert_equal(v.tlong1, 333333);
    
    f = -100;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tint3", 0, (char *)&f, 0L, BFLD_FLOAT), 
            EXSUCCEED);
    assert_equal(v.tint3, -100);
    
    f = 1010101;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tint4", 0, (char *)&f, 0L, BFLD_FLOAT), 
            EXSUCCEED);
    assert_equal(v.tint4[0], 1010101);
    
    f = 989898;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tint4", 1, (char *)&f, 0L, BFLD_FLOAT), 
            EXSUCCEED);
    assert_equal(v.tint4[1], 989898);
        
    
    /* Test the ascii value to char */
    f = 65;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tchar1", 0, (char *)&f, 0L, 
            BFLD_FLOAT), 
            EXSUCCEED);
    assert_equal(v.tchar1, 65);

    
    f = 66;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tfloat1", 1, (char *)&f, 0L, 
            BFLD_FLOAT), 
            EXSUCCEED);
    assert_double_equal(v.tfloat1[1], 66);
    
    
    f = 11.44;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tdouble1", 1, (char *)&f, 0L, 
            BFLD_FLOAT), 
            EXSUCCEED);
    assert_double_equal(v.tdouble1[1],11.44);
    
    
    f = 55;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tstring0", 0, (char *)&f, 0L, 
            BFLD_FLOAT), 
            EXSUCCEED);
    assert_string_equal(v.tstring0[0],"55.00000");
    
    f=44;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tcarray3", 3, (char *)&f, 0L, 
            BFLD_FLOAT), 
            EXSUCCEED);
    v.tcarray3[3][8] = EXEOS;
    assert_string_equal(v.tcarray3[3], "44.00000");
    
}

/**
 * Test Bvchg as float
 */
Ensure(test_Bvchg_double)
{
    struct MYVIEW1 v;
    init_MYVIEW1(&v);
    double d;
    
    d = 1555;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tshort1", 0, (char *)&d, 0L, BFLD_DOUBLE), 
            EXSUCCEED);
    assert_equal(v.tshort1, 1555);
    
    /* Invalid value, index out of bounds.. */
    d = 0;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tshort1", 1, (char *)&d, 0L, BFLD_DOUBLE), 
            EXFAIL);
    assert_double_equal(Berror, BEINVAL);
    
    
    d = -8888;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tshort3", 2, (char *)&d, 0L, BFLD_DOUBLE), 
            EXSUCCEED);
    d = -7777;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tshort3", 1, (char *)&d, 0L, BFLD_DOUBLE), 
            EXSUCCEED);
    
    assert_equal(v.tshort3[2], -8888);
    assert_equal(v.tshort3[1], -7777);
    
    d = 333333;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tlong1", 0, (char *)&d, 0L, BFLD_DOUBLE), 
            EXSUCCEED);
    assert_equal(v.tlong1, 333333);
    
    d = -100;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tint3", 0, (char *)&d, 0L, BFLD_DOUBLE), 
            EXSUCCEED);
    assert_equal(v.tint3, -100);
    
    d = 1010101;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tint4", 0, (char *)&d, 0L, BFLD_DOUBLE), 
            EXSUCCEED);
    assert_equal(v.tint4[0], 1010101);
    
    d = 989898;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tint4", 1, (char *)&d, 0L, BFLD_DOUBLE), 
            EXSUCCEED);
    assert_equal(v.tint4[1], 989898);
        
    
    /* Test the ascii value to char */
    d = 65;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tchar1", 0, (char *)&d, 0L, 
            BFLD_DOUBLE), 
            EXSUCCEED);
    assert_equal(v.tchar1, 65);

    
    d = 66;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tfloat1", 1, (char *)&d, 0L, 
            BFLD_DOUBLE), 
            EXSUCCEED);
    assert_double_equal(v.tfloat1[1], 66);
    
    
    d = 11.44;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tdouble1", 1, (char *)&d, 0L, 
            BFLD_DOUBLE), 
            EXSUCCEED);
    assert_double_equal(v.tdouble1[1],11.44);
    
    
    d = 55;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tstring0", 0, (char *)&d, 0L, 
            BFLD_DOUBLE), 
            EXSUCCEED);
    assert_string_equal(v.tstring0[0],"55.000000");
    
    d=44;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tcarray3", 3, (char *)&d, 0L, 
            BFLD_DOUBLE), 
            EXSUCCEED);
    v.tcarray3[3][9] = EXEOS;
    assert_string_equal(v.tcarray3[3], "44.000000");
    
}

/**
 * Test Bvchg as string
 */
Ensure(test_Bvchg_string)
{
    struct MYVIEW1 v;
    init_MYVIEW1(&v);
    
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tshort1", 0, "32000", 0L, BFLD_STRING), 
            EXSUCCEED);
    assert_equal(v.tshort1, 32000);

    /* Invalid value, index out of bounds.. */
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tshort1", 1, "66", 0L, BFLD_STRING), 
            EXFAIL);
    assert_equal(Berror, BEINVAL);
    
    
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tshort2", 1, "-32000", 0L, BFLD_STRING), 
            EXSUCCEED);
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tshort3", 0, "55", 0L, BFLD_STRING), 
            EXSUCCEED);
    
    assert_equal(v.tshort2[1],-32000);
    assert_equal(v.tshort3[0], 55);
    
    
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tlong1", 0, "33333333", 0L, BFLD_STRING), 
            EXSUCCEED);
    assert_equal(v.tlong1, 33333333);
    
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tint3", 0, "-100", 0L, BFLD_STRING), 
            EXSUCCEED);
    assert_equal(v.tint3, -100);
    
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tint4", 0, "1010101", 0L, BFLD_STRING), 
            EXSUCCEED);
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tint4", 1, "989898", 0L, BFLD_STRING), 
            EXSUCCEED);
    
    assert_equal(v.tint4[0], 1010101);
    assert_equal(v.tint4[1], 989898);
    
    /* non existent field: */
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "xint3", 0, "", 0L, BFLD_STRING), 
            EXFAIL);
    assert_equal(Berror, BNOCNAME);
    
    /* Test field NULL */
    
    /* Test the ascii value to char */

    NDRX_LOG(log_debug, "tchar1=%c", v.tchar1);
    
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tchar1", 0, "A", 0L, 
            BFLD_STRING), 
            EXSUCCEED);
    assert_equal(v.tchar1, 'A');

    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tchar2", 4, "\t", 0L, 
            BFLD_STRING), 
            EXSUCCEED);
    assert_equal(v.tchar2[4], '\t');
    
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tfloat1", 1, "66.5", 0L, 
            BFLD_STRING), 
            EXSUCCEED);
    assert_double_equal(v.tfloat1[1], 66.5);
    
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tdouble1", 1, "11.440000", 0L, 
            BFLD_STRING), 
            EXSUCCEED);
    assert_double_equal(v.tdouble1[1], 11.44);
    
  
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tstring0", 1, "HELLO WOLRD", 0L, 
            BFLD_STRING), 
            EXSUCCEED);
    assert_string_equal(v.tstring0[1], "HELLO WOLRD");
    
    
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tstring3", 1, 
            "HELLO WOLRDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD", 0L, 
            BFLD_STRING), 
            EXFAIL);
    
    assert_equal(Berror, BNOSPACE);
        
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tcarray3", 3, "HELLO1", 6, 
            BFLD_STRING), 
            EXSUCCEED);
    
    v.tcarray3[3][6] = EXEOS;
    assert_string_equal(v.tcarray3[3], "HELLO1");
    assert_equal(v.L_tcarray3[3], 6);
        
}

/**
 * Test Bvchg as carray
 */
Ensure(test_Bvchg_carray)
{
    struct MYVIEW1 v;
    init_MYVIEW1(&v);
    
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tshort1", 0, "32000", 5, BFLD_CARRAY), 
            EXSUCCEED);
    assert_equal(v.tshort1, 32000);

    /* Invalid value, index out of bounds.. */
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tshort1", 1, "66", 2, BFLD_STRING), 
            EXFAIL);
    assert_equal(Berror, BEINVAL);
    
    
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tshort2", 1, "-32000", 6, BFLD_STRING), 
            EXSUCCEED);
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tshort3", 0, "55", 2, BFLD_STRING), 
            EXSUCCEED);
    
    assert_equal(v.tshort2[1],-32000);
    assert_equal(v.tshort3[0], 55);
    
    
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tlong1", 0, "33333333", 8, BFLD_STRING), 
            EXSUCCEED);
    assert_equal(v.tlong1, 33333333);
    
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tint3", 0, "-100", 4, BFLD_STRING), 
            EXSUCCEED);
    assert_equal(v.tint3, -100);
    
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tint4", 0, "1010101", 7, BFLD_STRING), 
            EXSUCCEED);
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tint4", 1, "989898", 6, BFLD_STRING), 
            EXSUCCEED);
    
    assert_equal(v.tint4[0], 1010101);
    assert_equal(v.tint4[1], 989898);
    
    /* non existent field: */
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "xint3", 0, "", 0L, BFLD_STRING), 
            EXFAIL);
    assert_equal(Berror, BNOCNAME);
    
    /* Test field NULL */
    
    /* Test the ascii value to char */

    NDRX_LOG(log_debug, "tchar1=%c", v.tchar1);
    
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tchar1", 0, "A", 1, 
            BFLD_STRING), 
            EXSUCCEED);
    assert_equal(v.tchar1, 'A');

    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tchar2", 4, "\t", 1, 
            BFLD_STRING), 
            EXSUCCEED);
    assert_equal(v.tchar2[4], '\t');
    
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tfloat1", 1, "66.5", 4, 
            BFLD_STRING), 
            EXSUCCEED);
    assert_double_equal(v.tfloat1[1], 66.5);
    
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tdouble1", 1, "11.440000", 9, 
            BFLD_STRING), 
            EXSUCCEED);
    assert_double_equal(v.tdouble1[1], 11.44);
    
  
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tstring0", 1, "HELLO WOLRD", 11, 
            BFLD_STRING), 
            EXSUCCEED);
    assert_string_equal(v.tstring0[1], "HELLO WOLRD");
    
    
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tstring3", 3, 
            "HELLO WOLRDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD", 75, 
            BFLD_STRING), 
            EXFAIL);
    
    assert_equal(Berror, BNOSPACE);
        
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tcarray3", 3, "HELLO1", 6, 
            BFLD_STRING), 
            EXSUCCEED);
    
    v.tcarray3[3][6] = EXEOS;
    assert_string_equal(v.tcarray3[3], "HELLO1");
    assert_equal(v.L_tcarray3[3], 6);
        
}


/**
 * Test Bvchg as int
 */
Ensure(test_Bvchg_int)
{
    struct MYVIEW1 v;
    int i;
    
    memset(&v, 0, sizeof(v));
    
    i = 15556;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tshort1", 0, (char *)&i, 0L, BFLD_INT), 
            EXSUCCEED);
    assert_equal(v.tshort1, 15556);
    
    /* Invalid value, index out of bounds.. */
    i = 0;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tshort1", 1, (char *)&i, 0L, BFLD_INT), 
            EXFAIL);
    assert_equal(Berror, BEINVAL);
    
    i = 9999;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tshort2", 0, (char *)&i, 0L, BFLD_INT), 
            EXSUCCEED);
    assert_equal(v.tshort2[0], 9999);
    
    i = 8888;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tshort2", 1, (char *)&i, 0L, BFLD_INT), 
            EXSUCCEED);
    assert_equal(v.tshort2[1], 8888);
    
    i = 7777;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tshort3", 0, (char *)&i, 0L, BFLD_INT), 
            EXSUCCEED);
    assert_equal(v.tshort3[0], 7777);
    
    
    i = 33333333;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tlong1", 0, (char *)&i, 0L, BFLD_INT), 
            EXSUCCEED);
    assert_equal(v.tlong1, 33333333);
    
    i = -100;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tint3", 0, (char *)&i, 0L, BFLD_INT), 
            EXSUCCEED);
    i = 1010101;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tint4", 0, (char *)&i, 0L, BFLD_INT), 
            EXSUCCEED);
    assert_equal(v.tint3, -100);
    assert_equal(v.tint4[0], 1010101);
    
    i = 989898;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tint4", 1, (char *)&i, 0L, BFLD_INT), 
            EXSUCCEED);
    assert_equal(v.tint4[1], 989898);
    
    /* non existent field: */
    i = 0;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "xint3", 0, (char *)&i, 0L, BFLD_INT), 
            EXFAIL);
    assert_equal(Berror, BNOCNAME);
    
    
    /* Test the ascii value to char */
    i = 65;
    
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tchar1", 0, (char *)&i, 0L, 
            BFLD_INT), 
            EXSUCCEED);
    assert_equal(v.tchar1, 65);
    
    i = 67;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tchar2", 2, (char *)&i, 0L, 
            BFLD_INT), 
            EXSUCCEED);
    i = 66;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tchar2", 1, (char *)&i, 0L, 
            BFLD_INT), 
            EXSUCCEED);
    
    assert_equal(v.tchar2[2], 67);
    assert_equal(v.tchar2[1], 66);
    
    
    i = 66;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tfloat1", 1, (char *)&i, 0L, 
            BFLD_INT), 
            EXSUCCEED);
    assert_double_equal(v.tfloat1[1], 66);
    
    i = 65;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tfloat1", 1, (char *)&i, 0L, 
            BFLD_INT), 
            EXSUCCEED);
    assert_double_equal(v.tfloat1[1], 65);
   
    
    i = 11111;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tdouble1", 1, (char *)&i, 0L, 
            BFLD_INT), 
            EXSUCCEED);
    assert_double_equal(v.tdouble1[1],11111);
    
    
    i = 125;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tstring0", 1, (char *)&i, 0L, 
            BFLD_INT), 
            EXSUCCEED);
    assert_string_equal(v.tstring0[1], "125");
    
    
    i = 521;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tcarray2", 0, (char *)&i, 0L, 
            BFLD_INT), 
            EXSUCCEED);
    
    v.tcarray2[3] = EXEOS;
    assert_string_equal(v.tcarray2, "521");
    
    
    i = 123456;
    assert_equal(CBvchg((char *)&v, "MYVIEW1", "tcarray4", 0, (char *)&i, 0L, 
            BFLD_INT), 
            EXFAIL);
    
    assert_equal(Berror, BNOSPACE);
    
    
}


/**
 * Very basic tests of the framework
 * @return
 */
TestSuite *vacc_CBvchg_tests(void) {
    TestSuite *suite = create_test_suite();
    
    set_setup(suite, basic_setup);
    set_teardown(suite, basic_teardown);

    /* init view test */
    add_test(suite, test_Bvchg_short);
    add_test(suite, test_Bvchg_long);
    add_test(suite, test_Bvchg_char);
    add_test(suite, test_Bvchg_float);
    add_test(suite, test_Bvchg_double);
    add_test(suite, test_Bvchg_string);
    add_test(suite, test_Bvchg_carray);
    add_test(suite, test_Bvchg_int);
    
    return suite;
}
