/* 
** View access unit
**
** @file vaccget.c
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
 * Test Bvget as short
 */
Ensure(test_Bvget_short)
{
    struct MYVIEW1 v;
    short s;
    
    init_MYVIEW1(&v);
    
    s = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort1", 0, (char *)&s, 0L, BFLD_SHORT, 0L), 
            EXSUCCEED);
    assert_equal(s, 15556);
    
    /* Invalid value, index out of bounds.. */
    s = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort1", 1, (char *)&s, 0L, BFLD_SHORT, 0L), 
            EXFAIL);
    assert_equal(Berror, BEINVAL);
    
    s = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort2", 0, (char *)&s, 0L, BFLD_SHORT, 0L), 
            EXSUCCEED);
    assert_equal(s, 9999);
    
    s = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort2", 1, (char *)&s, 0L, BFLD_SHORT, 0L), 
            EXSUCCEED);
    assert_equal(s, 8888);
    
    s = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort3", 0, (char *)&s, 0L, BFLD_SHORT, 0L), 
            EXSUCCEED);
    assert_equal(s, 7777);
    
    s = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort3", 1, (char *)&s, 0L, BFLD_SHORT, 0L), 
            EXSUCCEED);
    assert_equal(s, -7777);
    
    s = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort3", 2, (char *)&s, 0L, BFLD_SHORT, 0L), 
            EXSUCCEED);
    assert_equal(s, -7777);
    
    s = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort3", 2, (char *)&s, 0L, 
            BFLD_SHORT, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BNOTPRES);
    
    s = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort4", 0, (char *)&s, 0L, BFLD_SHORT, 0L), 
            EXSUCCEED);
    assert_equal(s, -10);
    
    
    s = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tlong1", 0, (char *)&s, 0L, BFLD_SHORT, 0L), 
            EXSUCCEED);
    assert_not_equal(s, 0);
    
    s = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint3", 0, (char *)&s, 0L, BFLD_SHORT, 0L), 
            EXSUCCEED);
    assert_equal(s, -100);
    
    /* non existent field: */
    s = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "xint3", 0, (char *)&s, 0L, BFLD_SHORT, 0L), 
            EXFAIL);
    assert_equal(Berror, BNOCNAME);
    
    /* Test field NULL */
    v.tint4[0] = -1;
    v.tint4[1] = -1;
    
    s = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint4", 0, (char *)&s, 0L, BFLD_SHORT, 0L), 
            EXSUCCEED);
    assert_equal(s, -1);
    
    s = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint4", 1, (char *)&s, 0L, BFLD_SHORT, 0L), 
            EXSUCCEED);
    assert_equal(s, -1);
    
    
    /* not pres because of NULL value */
    s = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint4", 0, (char *)&s, 0L, 
            BFLD_SHORT, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BNOTPRES);
    
    s = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint4", 1, (char *)&s, 0L, 
            BFLD_SHORT, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BNOTPRES);
    
    
    /* Test the ascii value to char */
    s = 0;
    NDRX_LOG(log_debug, "tchar1=%c", v.tchar1);
    
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar1", 0, (char *)&s, 0L, 
            BFLD_SHORT, BVACCESS_NOTNULL), 
            EXSUCCEED);
    assert_equal(s, 65);
    
    
    s = 0;
    NDRX_LOG(log_debug, "tchar2=%c", v.tchar1);
    
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar2", 0, (char *)&s, 0L, 
            BFLD_SHORT, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BNOTPRES);
    
    s = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar2", 1, (char *)&s, 0L, 
            BFLD_SHORT, BVACCESS_NOTNULL), 
            EXSUCCEED);
    assert_equal(s, 66);
    
    s = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar2", 2, (char *)&s, 0L, 
            BFLD_SHORT, BVACCESS_NOTNULL), 
            EXSUCCEED);
    assert_equal(s, 67);
    
    s = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar2", 3, (char *)&s, 0L, 
            BFLD_SHORT, BVACCESS_NOTNULL), 
            EXSUCCEED);
    assert_equal(s, '\n');
    
    s = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar2", 4, (char *)&s, 0L, 
            BFLD_SHORT, BVACCESS_NOTNULL), 
            EXSUCCEED);
    assert_equal(s, '\t');
    
    s = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar2", 5, (char *)&s, 0L, 
            BFLD_SHORT, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BEINVAL);
    
    s = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar3", 0, (char *)&s, 0L, 
            BFLD_SHORT, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BNOTPRES);
    
    s = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar3", 1, (char *)&s, 0L, 
            BFLD_SHORT, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BNOTPRES);
    
    s = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar3", 0, (char *)&s, 0L, 
            BFLD_SHORT, 0L), 
            EXSUCCEED);
    assert_equal(s, 'C');
    
    s = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar3", 1, (char *)&s, 0L, 
            BFLD_SHORT, 0L), 
            EXSUCCEED);
    assert_equal(s, 'D');
    

    v.tfloat1[1] = 66.0f;
    
    s = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tfloat1", 1, (char *)&s, 0L, 
            BFLD_SHORT, 0L), 
            EXSUCCEED);
    assert_equal(s, 66);
    
    
    v.tfloat1[1] = 66.0f;
    
    s = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tfloat1", 1, (char *)&s, 0L, 
            BFLD_SHORT, 0L), 
            EXSUCCEED);
    assert_equal(s, 66);
    
    
    s = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tdouble2", 0, (char *)&s, 0L, 
            BFLD_SHORT, 0L), 
            EXSUCCEED);
    assert_equal(s,-999);
    
    
    NDRX_STRCPY_SAFE(v.tstring0[0], "125");
    
    s = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tstring0", 0, (char *)&s, 0L, 
            BFLD_SHORT, 0L), 
            EXSUCCEED);
    assert_equal(s,125);
    
    
    v.L_tcarray2 = 4; /* includes EOS.. */
    NDRX_STRCPY_SAFE(v.tcarray2, "521");
    
    
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tcarray2", 0, (char *)&s, 0L, 
            BFLD_SHORT, 0L), 
            EXSUCCEED);
    assert_equal(s,521);
    
}

/**
 * Test Bvget as long
 */
Ensure(test_Bvget_long)
{
    struct MYVIEW1 v;
    long l;
    
    init_MYVIEW1(&v);
    
    l = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort1", 0, (char *)&l, 0L, BFLD_LONG, 0L), 
            EXSUCCEED);
    assert_equal(l, 15556);
    
    /* Invalid value, index out of bounds.. */
    l = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort1", 1, (char *)&l, 0L, BFLD_LONG, 0L), 
            EXFAIL);
    assert_equal(Berror, BEINVAL);
    
    l = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort2", 0, (char *)&l, 0L, BFLD_LONG, 0L), 
            EXSUCCEED);
    assert_equal(l, 9999);
    
    l = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort2", 1, (char *)&l, 0L, BFLD_LONG, 0L), 
            EXSUCCEED);
    assert_equal(l, 8888);
    
    l = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort3", 0, (char *)&l, 0L, BFLD_LONG, 0L), 
            EXSUCCEED);
    assert_equal(l, 7777);
    
    l = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort3", 1, (char *)&l, 0L, BFLD_LONG, 0L), 
            EXSUCCEED);
    assert_equal(l, -7777);
    
    l = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort3", 2, (char *)&l, 0L, BFLD_LONG, 0L), 
            EXSUCCEED);
    assert_equal(l, -7777);
    
    l = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort3", 2, (char *)&l, 0L, 
            BFLD_LONG, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BNOTPRES);
    
    l = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort4", 0, (char *)&l, 0L, BFLD_LONG, 0L), 
            EXSUCCEED);
    assert_equal(l, -10);
    
    
    l = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tlong1", 0, (char *)&l, 0L, BFLD_LONG, 0L), 
            EXSUCCEED);
    assert_equal(l, 33333333);
    
    l = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint3", 0, (char *)&l, 0L, BFLD_LONG, 0L), 
            EXSUCCEED);
    assert_equal(l, -100);
    
    l = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint4", 0, (char *)&l, 0L, BFLD_LONG, 0L), 
            EXSUCCEED);
    assert_equal(l, 1010101);
    
    l = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint4", 1, (char *)&l, 0L, BFLD_LONG, 0L), 
            EXSUCCEED);
    assert_equal(l, 989898);
    
    /* non existent field: */
    l = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "xint3", 0, (char *)&l, 0L, BFLD_LONG, 0L), 
            EXFAIL);
    assert_equal(Berror, BNOCNAME);
    
    /* Test field NULL */
    v.tint4[0] = -1;
    v.tint4[1] = -1;
    
    l = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint4", 0, (char *)&l, 0L, BFLD_LONG, 0L), 
            EXSUCCEED);
    assert_equal(l, -1);
    
    l = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint4", 1, (char *)&l, 0L, BFLD_LONG, 0L), 
            EXSUCCEED);
    assert_equal(l, -1);
    
    
    /* not pres because of NULL value */
    l = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint4", 0, (char *)&l, 0L, 
            BFLD_LONG, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BNOTPRES);
    
    l = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint4", 1, (char *)&l, 0L, 
            BFLD_LONG, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BNOTPRES);
    
    
    /* Test the ascii value to char */
    l = 0;
    NDRX_LOG(log_debug, "tchar1=%c", v.tchar1);
    
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar1", 0, (char *)&l, 0L, 
            BFLD_LONG, BVACCESS_NOTNULL), 
            EXSUCCEED);
    assert_equal(l, 65);
    
    
    l = 0;
    NDRX_LOG(log_debug, "tchar2=%c", v.tchar1);
    
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar2", 0, (char *)&l, 0L, 
            BFLD_LONG, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BNOTPRES);
    
    l = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar2", 1, (char *)&l, 0L, 
            BFLD_LONG, BVACCESS_NOTNULL), 
            EXSUCCEED);
    assert_equal(l, 66);
    
    l = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar2", 2, (char *)&l, 0L, 
            BFLD_LONG, BVACCESS_NOTNULL), 
            EXSUCCEED);
    assert_equal(l, 67);
    
    l = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar2", 3, (char *)&l, 0L, 
            BFLD_LONG, BVACCESS_NOTNULL), 
            EXSUCCEED);
    assert_equal(l, '\n');
    
    l = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar2", 4, (char *)&l, 0L, 
            BFLD_LONG, BVACCESS_NOTNULL), 
            EXSUCCEED);
    assert_equal(l, '\t');
    
    l = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar2", 5, (char *)&l, 0L, 
            BFLD_LONG, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BEINVAL);
    
    l = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar3", 0, (char *)&l, 0L, 
            BFLD_LONG, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BNOTPRES);
    
    l = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar3", 1, (char *)&l, 0L, 
            BFLD_LONG, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BNOTPRES);
    
    l = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar3", 0, (char *)&l, 0L, 
            BFLD_LONG, 0L), 
            EXSUCCEED);
    assert_equal(l, 'C');
    
    l = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar3", 1, (char *)&l, 0L, 
            BFLD_LONG, 0L), 
            EXSUCCEED);
    assert_equal(l, 'D');
    

    v.tfloat1[1] = 66.0f;
    
    l = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tfloat1", 1, (char *)&l, 0L, 
            BFLD_LONG, 0L), 
            EXSUCCEED);
    assert_equal(l, 66);
    
    
    l = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tfloat2", 0, (char *)&l, 0L, 
            BFLD_LONG, 0L), 
            EXSUCCEED);
    assert_equal(l, 100000);
   
    
    l = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tdouble1", 1, (char *)&l, 0L, 
            BFLD_LONG, 0L), 
            EXSUCCEED);
    assert_equal(l,11111);
    
    l = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tdouble2", 0, (char *)&l, 0L, 
            BFLD_LONG, 0L), 
            EXSUCCEED);
    assert_equal(l,-999);
    
    
    NDRX_STRCPY_SAFE(v.tstring0[0], "125");
    
    l = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tstring0", 0, (char *)&l, 0L, 
            BFLD_LONG, 0L), 
            EXSUCCEED);
    assert_equal(l,125);
    
    
    v.L_tcarray2 = 4; /* includes EOS.. */
    NDRX_STRCPY_SAFE(v.tcarray2, "521");
    
    
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tcarray2", 0, (char *)&l, 0L, 
            BFLD_LONG, 0L), 
            EXSUCCEED);
    assert_equal(l,521);
    
}

/**
 * Test Bvget as char
 */
Ensure(test_Bvget_char)
{
    struct MYVIEW1 v;
    
    char c;
    short s;
    long l;
    
    init_MYVIEW1(&v);
    
    c = 0;
    v.tshort1 = 65;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort1", 0, (char *)&c, 0L, BFLD_CHAR, 0L), 
            EXSUCCEED);
    assert_equal(c, 'A');
    
    /* Invalid value, index out of bounds.. */
    c = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort1", 1, (char *)&c, 0L, BFLD_CHAR, 0L), 
            EXFAIL);
    assert_equal(Berror, BEINVAL);
    
    
    v.tshort2[0] = 'a';
    c = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort2", 0, (char *)&c, 0L, BFLD_CHAR, 0L), 
            EXSUCCEED);
    assert_equal(c, 'a');
   
    
    v.tlong1 = 98;
    c = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tlong1", 0, (char *)&c, 0L, BFLD_CHAR, 0L), 
            EXSUCCEED);
    assert_equal(c, 'b');
    
    
    v.tint3 = 90;
    c = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint3", 0, (char *)&c, 0L, BFLD_CHAR, 0L), 
            EXSUCCEED);
    assert_equal(c, 'Z');
    
    
    c = 0;
    v.tint4[1] = 89;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint4", 1, (char *)&c, 0L, BFLD_CHAR, 0L), 
            EXSUCCEED);
    assert_equal(c, 'Y');
    
    /* non existent field: */
    c = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "xint3", 0, (char *)&c, 0L, BFLD_CHAR, 0L), 
            EXFAIL);
    assert_equal(Berror, BNOCNAME);
    
    /* Test field NULL */
    v.tint4[0] = 104;
    v.tint4[1] = 105;
    
    c = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint4", 0, (char *)&c, 0L, BFLD_CHAR, 0L), 
            EXSUCCEED);
    assert_equal(c, 'h');
    
    c = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint4", 1, (char *)&c, 0L, BFLD_CHAR, 0L), 
            EXSUCCEED);
    assert_equal(c, 'i');
    
    
    /* not pres because of NULL value */
    c = 0;
    v.tint4[0]=-1;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint4", 0, (char *)&c, 0L, 
            BFLD_CHAR, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BNOTPRES);
    
    c = 0;
    v.tint4[1]=-1;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint4", 1, (char *)&c, 0L, 
            BFLD_CHAR, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BNOTPRES);
    
    
    /* Test the ascii value to char */
    c = 0;
    NDRX_LOG(log_debug, "tchar1=%c", v.tchar1);
    
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar1", 0, (char *)&c, 0L, 
            BFLD_CHAR, BVACCESS_NOTNULL), 
            EXSUCCEED);
    assert_equal(c, 65);
    
    
    c = 0;
    NDRX_LOG(log_debug, "tchar2=%c", v.tchar1);
    
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar2", 0, (char *)&c, 0L, 
            BFLD_CHAR, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BNOTPRES);
    
    c = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar2", 1, (char *)&c, 0L, 
            BFLD_CHAR, BVACCESS_NOTNULL), 
            EXSUCCEED);
    assert_equal(c, 66);
    
    c = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar2", 2, (char *)&c, 0L, 
            BFLD_CHAR, BVACCESS_NOTNULL), 
            EXSUCCEED);
    assert_equal(c, 67);
    
    c = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar2", 3, (char *)&c, 0L, 
            BFLD_CHAR, BVACCESS_NOTNULL), 
            EXSUCCEED);
    assert_equal(c, '\n');
    
    c = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar2", 4, (char *)&c, 0L, 
            BFLD_CHAR, BVACCESS_NOTNULL), 
            EXSUCCEED);
    assert_equal(c, '\t');
    
    c = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar2", 5, (char *)&c, 0L, 
            BFLD_CHAR, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BEINVAL);
    
    c = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar3", 0, (char *)&c, 0L, 
            BFLD_CHAR, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BNOTPRES);
    
    c = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar3", 1, (char *)&c, 0L, 
            BFLD_CHAR, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BNOTPRES);
    
    c = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar3", 0, (char *)&c, 0L, 
            BFLD_CHAR, 0L), 
            EXSUCCEED);
    assert_equal(c, 'C');
    
    c = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar3", 1, (char *)&c, 0L, 
            BFLD_CHAR, 0L), 
            EXSUCCEED);
    assert_equal(c, 'D');
    

    v.tfloat1[1] = 66.0f;
    
    c = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tfloat1", 1, (char *)&c, 0L, 
            BFLD_CHAR, 0L), 
            EXSUCCEED);
    assert_equal(c, 66);
    
    
    c = 0;
    v.tdouble1[1]='D';
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tdouble1", 1, (char *)&c, 0L, 
            BFLD_CHAR, 0L), 
            EXSUCCEED);
    assert_equal(c,'D');
    
    
    NDRX_STRCPY_SAFE(v.tstring0[0], "X");
    
    c = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tstring0", 0, (char *)&c, 0L, 
            BFLD_CHAR, 0L), 
            EXSUCCEED);
    assert_equal(c,'X');
    
    
    v.L_tcarray2 = 0; /* includes EOS.. */
    NDRX_STRCPY_SAFE(v.tcarray2, "Y");
    
    
    /* Does not use EOS, just take first symbol.. */
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tcarray2", 0, (char *)&c, 0L, 
            BFLD_CHAR, 0L), 
            EXSUCCEED);
    assert_equal(c,'Y');
    
}

/**
 * Test Bvget as float
 */
Ensure(test_Bvget_float)
{
    struct MYVIEW1 v;
    float f;
    
    init_MYVIEW1(&v);
    
    f = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort1", 0, (char *)&f, 0L, BFLD_FLOAT, 0L), 
            EXSUCCEED);
    assert_double_equal(f, 15556);
    
    /* Invalid value, index out of bounds.. */
    f = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort1", 1, (char *)&f, 0L, BFLD_FLOAT, 0L), 
            EXFAIL);
    assert_double_equal(Berror, BEINVAL);
    
    f = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort2", 0, (char *)&f, 0L, BFLD_FLOAT, 0L), 
            EXSUCCEED);
    assert_double_equal(f, 9999);
    
    f = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort2", 1, (char *)&f, 0L, BFLD_FLOAT, 0L), 
            EXSUCCEED);
    assert_double_equal(f, 8888);
    
    f = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort3", 0, (char *)&f, 0L, BFLD_FLOAT, 0L), 
            EXSUCCEED);
    assert_double_equal(f, 7777);
    
    f = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort3", 1, (char *)&f, 0L, BFLD_FLOAT, 0L), 
            EXSUCCEED);
    assert_equal(f, -7777);
    
    f = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort3", 2, (char *)&f, 0L, BFLD_FLOAT, 0L), 
            EXSUCCEED);
    assert_double_equal(f, -7777);
    
    f = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort3", 2, (char *)&f, 0L, 
            BFLD_FLOAT, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_double_equal(Berror, BNOTPRES);
    
    f = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort4", 0, (char *)&f, 0L, BFLD_FLOAT, 0L), 
            EXSUCCEED);
    assert_double_equal(f, -10);
    
    
    f = 0;
    v.tlong1 = 333333;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tlong1", 0, (char *)&f, 0L, BFLD_FLOAT, 0L), 
            EXSUCCEED);
    assert_double_equal(f, 333333);
    
    f = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint3", 0, (char *)&f, 0L, BFLD_FLOAT, 0L), 
            EXSUCCEED);
    assert_double_equal(f, -100);
    
    f = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint4", 0, (char *)&f, 0L, BFLD_FLOAT, 0L), 
            EXSUCCEED);
    assert_double_equal(f, 1010101);
    
    f = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint4", 1, (char *)&f, 0L, BFLD_FLOAT, 0L), 
            EXSUCCEED);
    assert_double_equal(f, 989898);
    
    /* non existent field: */
    f = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "xint3", 0, (char *)&f, 0L, BFLD_FLOAT, 0L), 
            EXFAIL);
    assert_equal(Berror, BNOCNAME);
    
    /* Test field NULL */
    v.tint4[0] = -1;
    v.tint4[1] = -1;
    
    f = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint4", 0, (char *)&f, 0L, BFLD_FLOAT, 0L), 
            EXSUCCEED);
    assert_double_equal(f, -1);
    
    f = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint4", 1, (char *)&f, 0L, BFLD_FLOAT, 0L), 
            EXSUCCEED);
    assert_double_equal(f, -1);
    
    
    /* not pres because of NULL value */
    f = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint4", 0, (char *)&f, 0L, 
            BFLD_FLOAT, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BNOTPRES);
    
    f = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint4", 1, (char *)&f, 0L, 
            BFLD_FLOAT, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BNOTPRES);
    
    
    /* Test the ascii value to char */
    f = 0;
    NDRX_LOG(log_debug, "tchar1=%c", v.tchar1);
    
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar1", 0, (char *)&f, 0L, 
            BFLD_FLOAT, BVACCESS_NOTNULL), 
            EXSUCCEED);
    assert_double_equal(f, 65);

    
    f = 0;
    NDRX_LOG(log_debug, "tchar2=%c", v.tchar1);
    
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar2", 0, (char *)&f, 0L, 
            BFLD_FLOAT, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BNOTPRES);
    
    f = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar2", 1, (char *)&f, 0L, 
            BFLD_FLOAT, BVACCESS_NOTNULL), 
            EXSUCCEED);
    assert_double_equal(f, 66);
    
    f = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar2", 2, (char *)&f, 0L, 
            BFLD_FLOAT, BVACCESS_NOTNULL), 
            EXSUCCEED);
    assert_double_equal(f, 67);
    
    f = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar2", 3, (char *)&f, 0L, 
            BFLD_FLOAT, BVACCESS_NOTNULL), 
            EXSUCCEED);
    assert_double_equal(f, '\n');
    
    f = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar2", 4, (char *)&f, 0L, 
            BFLD_FLOAT, BVACCESS_NOTNULL), 
            EXSUCCEED);
    assert_double_equal(f, '\t');
    
    f = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar2", 5, (char *)&f, 0L, 
            BFLD_FLOAT, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BEINVAL);
    
    f = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar3", 0, (char *)&f, 0L, 
            BFLD_FLOAT, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BNOTPRES);
    
    f = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar3", 1, (char *)&f, 0L, 
            BFLD_FLOAT, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BNOTPRES);
    
    f = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar3", 0, (char *)&f, 0L, 
            BFLD_FLOAT, 0L), 
            EXSUCCEED);
    assert_double_equal(f, 'C');
    
    f = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar3", 1, (char *)&f, 0L, 
            BFLD_FLOAT, 0L), 
            EXSUCCEED);
    assert_double_equal(f, 'D');
    

    v.tfloat1[1] = 66.0f;
    
    f = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tfloat1", 1, (char *)&f, 0L, 
            BFLD_FLOAT, 0L), 
            EXSUCCEED);
    assert_double_equal(f, 66);
    
    
    f = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tfloat2", 0, (char *)&f, 0L, 
            BFLD_FLOAT, 0L), 
            EXSUCCEED);
    assert_double_equal(f, 100000.1);
   
    
    f = 0;
    v.tdouble1[1] = 11.44;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tdouble1", 1, (char *)&f, 0L, 
            BFLD_FLOAT, 0L), 
            EXSUCCEED);
    assert_double_equal(f,11.44);
    
    f = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tdouble2", 0, (char *)&f, 0L, 
            BFLD_FLOAT, 0L), 
            EXSUCCEED);
    assert_double_equal(f,-999.123);
    
    
    NDRX_STRCPY_SAFE(v.tstring0[0], "125.77");
    
    f = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tstring0", 0, (char *)&f, 0L, 
            BFLD_FLOAT, 0L), 
            EXSUCCEED);
    assert_double_equal(f,125.77);
    
    
    v.L_tcarray2 = 6; /* includes EOS.. */
    NDRX_STRCPY_SAFE(v.tcarray2, "521.5");
        
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tcarray2", 0, (char *)&f, 0L, 
            BFLD_FLOAT, 0L), 
            EXSUCCEED);
    assert_double_equal(f,521.5);
    
}

/**
 * Test Bvget as double
 */
Ensure(test_Bvget_double)
{
    struct MYVIEW1 v;
    double d;
    
    init_MYVIEW1(&v);
    
    d = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort1", 0, (char *)&d, 0L, BFLD_DOUBLE, 0L), 
            EXSUCCEED);
    assert_double_equal(d, 15556);
    
    /* Invalid value, index out of bounds.. */
    d = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort1", 1, (char *)&d, 0L, BFLD_DOUBLE, 0L), 
            EXFAIL);
    assert_double_equal(Berror, BEINVAL);
    
    d = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort2", 0, (char *)&d, 0L, BFLD_DOUBLE, 0L), 
            EXSUCCEED);
    assert_double_equal(d, 9999);
    
    d = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort2", 1, (char *)&d, 0L, BFLD_DOUBLE, 0L), 
            EXSUCCEED);
    assert_double_equal(d, 8888);
    
    d = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort3", 0, (char *)&d, 0L, BFLD_DOUBLE, 0L), 
            EXSUCCEED);
    assert_double_equal(d, 7777);
    
    d = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort3", 1, (char *)&d, 0L, BFLD_DOUBLE, 0L), 
            EXSUCCEED);
    assert_equal(d, -7777);
    
    d = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort3", 2, (char *)&d, 0L, BFLD_DOUBLE, 0L), 
            EXSUCCEED);
    assert_double_equal(d, -7777);
    
    d = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort3", 2, (char *)&d, 0L, 
            BFLD_DOUBLE, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_double_equal(Berror, BNOTPRES);
    
    d = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort4", 0, (char *)&d, 0L, BFLD_DOUBLE, 0L), 
            EXSUCCEED);
    assert_double_equal(d, -10);
    
    
    d = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tlong1", 0, (char *)&d, 0L, BFLD_DOUBLE, 0L), 
            EXSUCCEED);
    assert_double_equal(d, 33333333);
    
    d = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint3", 0, (char *)&d, 0L, BFLD_DOUBLE, 0L), 
            EXSUCCEED);
    assert_double_equal(d, -100);
    
    d = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint4", 0, (char *)&d, 0L, BFLD_DOUBLE, 0L), 
            EXSUCCEED);
    assert_double_equal(d, 1010101);
    
    d = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint4", 1, (char *)&d, 0L, BFLD_DOUBLE, 0L), 
            EXSUCCEED);
    assert_double_equal(d, 989898);
    
    /* non existent field: */
    d = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "xint3", 0, (char *)&d, 0L, BFLD_DOUBLE, 0L), 
            EXFAIL);
    assert_equal(Berror, BNOCNAME);
    
    /* Test field NULL */
    v.tint4[0] = -1;
    v.tint4[1] = -1;
    
    d = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint4", 0, (char *)&d, 0L, BFLD_DOUBLE, 0L), 
            EXSUCCEED);
    assert_double_equal(d, -1);
    
    d = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint4", 1, (char *)&d, 0L, BFLD_DOUBLE, 0L), 
            EXSUCCEED);
    assert_double_equal(d, -1);
    
    
    /* not pres because of NULL value */
    d = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint4", 0, (char *)&d, 0L, 
            BFLD_DOUBLE, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BNOTPRES);
    
    d = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint4", 1, (char *)&d, 0L, 
            BFLD_DOUBLE, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BNOTPRES);
    
    
    /* Test the ascii value to char */
    d = 0;
    NDRX_LOG(log_debug, "tchar1=%c", v.tchar1);
    
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar1", 0, (char *)&d, 0L, 
            BFLD_DOUBLE, BVACCESS_NOTNULL), 
            EXSUCCEED);
    assert_double_equal(d, 65);

    
    d = 0;
    NDRX_LOG(log_debug, "tchar2=%c", v.tchar1);
    
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar2", 0, (char *)&d, 0L, 
            BFLD_DOUBLE, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BNOTPRES);
    
    d = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar2", 1, (char *)&d, 0L, 
            BFLD_DOUBLE, BVACCESS_NOTNULL), 
            EXSUCCEED);
    assert_double_equal(d, 66);
    
    d = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar2", 2, (char *)&d, 0L, 
            BFLD_DOUBLE, BVACCESS_NOTNULL), 
            EXSUCCEED);
    assert_double_equal(d, 67);
    
    d = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar2", 3, (char *)&d, 0L, 
            BFLD_DOUBLE, BVACCESS_NOTNULL), 
            EXSUCCEED);
    assert_double_equal(d, '\n');
    
    d = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar2", 4, (char *)&d, 0L, 
            BFLD_DOUBLE, BVACCESS_NOTNULL), 
            EXSUCCEED);
    assert_double_equal(d, '\t');
    
    d = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar2", 5, (char *)&d, 0L, 
            BFLD_DOUBLE, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BEINVAL);
    
    d = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar3", 0, (char *)&d, 0L, 
            BFLD_DOUBLE, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BNOTPRES);
    
    d = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar3", 1, (char *)&d, 0L, 
            BFLD_DOUBLE, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BNOTPRES);
    
    d = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar3", 0, (char *)&d, 0L, 
            BFLD_DOUBLE, 0L), 
            EXSUCCEED);
    assert_double_equal(d, 'C');
    
    d = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar3", 1, (char *)&d, 0L, 
            BFLD_DOUBLE, 0L), 
            EXSUCCEED);
    assert_double_equal(d, 'D');
    

    v.tfloat1[1] = 66.0f;
    
    d = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tfloat1", 1, (char *)&d, 0L, 
            BFLD_DOUBLE, 0L), 
            EXSUCCEED);
    assert_double_equal(d, 66);
    
    
    d = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tfloat2", 0, (char *)&d, 0L, 
            BFLD_DOUBLE, 0L), 
            EXSUCCEED);
    assert_double_equal(d, 100000.1);
   
    
    d = 0;
    v.tdouble1[1] = 11.44;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tdouble1", 1, (char *)&d, 0L, 
            BFLD_DOUBLE, 0L), 
            EXSUCCEED);
    assert_double_equal(d,11.44);
    
    d = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tdouble2", 0, (char *)&d, 0L, 
            BFLD_DOUBLE, 0L), 
            EXSUCCEED);
    assert_double_equal(d,-999.123);
    
    
    NDRX_STRCPY_SAFE(v.tstring0[0], "125.77");
    
    d = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tstring0", 0, (char *)&d, 0L, 
            BFLD_DOUBLE, 0L), 
            EXSUCCEED);
    assert_double_equal(d,125.77);
    
    
    v.L_tcarray2 = 6; /* includes EOS.. */
    NDRX_STRCPY_SAFE(v.tcarray2, "521.5");
        
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tcarray2", 0, (char *)&d, 0L, 
            BFLD_DOUBLE, 0L), 
            EXSUCCEED);
    assert_double_equal(d,521.5);
    
}

/**
 * Test Bvget as string
 */
Ensure(test_Bvget_string)
{
    struct MYVIEW1 v;
    char s[255];
    BFLDLEN len;
    
    init_MYVIEW1(&v);
    
    s[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort1", 0, (char *)s, 0L, BFLD_STRING, 0L), 
            EXSUCCEED);
    assert_string_equal(s, "15556");
    
    
    s[0] = EXEOS;
    len = 255;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort1", 0, (char *)s, &len, BFLD_STRING, 0L), 
            EXSUCCEED);
    assert_string_equal(s, "15556");
    assert_equal(len, 6); /* + EOS */
    
    /* test buffer no space */    
    s[0] = EXEOS;
    len = 1;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort1", 0, (char *)s, &len, BFLD_STRING, 0L), 
            EXFAIL);
    assert_equal(Berror, BNOSPACE);
    
    
    s[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort1", 0, (char *)s, 0L, BFLD_STRING, 0L), 
            EXSUCCEED);
    assert_string_equal(s, "15556");
    
    /* Invalid value, index out of bounds.. */
    s[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort1", 1, (char *)s, 0L, BFLD_STRING, 0L), 
            EXFAIL);
    assert_equal(Berror, BEINVAL);
    
    s[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort2", 0, (char *)s, 0L, BFLD_STRING, 0L), 
            EXSUCCEED);
    assert_string_equal(s, "9999");
    
    s[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort2", 1, (char *)s, 0L, BFLD_STRING, 0L), 
            EXSUCCEED);
    assert_string_equal(s, "8888");
    
    s[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort3", 0, (char *)s, 0L, BFLD_STRING, 0L), 
            EXSUCCEED);
    assert_string_equal(s, "7777");
    
    s[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort3", 1, (char *)s, 0L, BFLD_STRING, 0L), 
            EXSUCCEED);
    assert_string_equal(s, "-7777");
    
    s[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort3", 2, (char *)s, 0L, BFLD_STRING, 0L), 
            EXSUCCEED);
    assert_string_equal(s, "-7777");
    
    s[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort3", 2, (char *)s, 0L, 
            BFLD_STRING, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BNOTPRES);
    
    s[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort4", 0, (char *)s, 0L, BFLD_STRING, 0L), 
            EXSUCCEED);
    assert_string_equal(s, "-10");
    
    
    s[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tlong1", 0, (char *)s, 0L, BFLD_STRING, 0L), 
            EXSUCCEED);
    assert_string_equal(s, "33333333");
    
    s[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint3", 0, (char *)s, 0L, BFLD_STRING, 0L), 
            EXSUCCEED);
    assert_string_equal(s, "-100");
    
    s[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint4", 0, (char *)s, 0L, BFLD_STRING, 0L), 
            EXSUCCEED);
    assert_string_equal(s, "1010101");
    
    s[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint4", 1, (char *)s, 0L, BFLD_STRING, 0L), 
            EXSUCCEED);
    assert_string_equal(s, "989898");
    
    /* non existent field: */
    s[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "xint3", 0, (char *)s, 0L, BFLD_STRING, 0L), 
            EXFAIL);
    assert_equal(Berror, BNOCNAME);
    
    /* Test field NULL */
    
    v.tint4[0] = -1;
    v.tint4[1] = -1;
    s[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint4", 0, (char *)s, 0L, BFLD_STRING, 0L), 
            EXSUCCEED);
    assert_string_equal(s, "-1");
    
    s[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint4", 1, (char *)s, 0L, BFLD_STRING, 0L), 
            EXSUCCEED);
    assert_string_equal(s, "-1");
    
    /* not pres because of NULL value */
    s[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint4", 0, (char *)s, 0L, 
            BFLD_STRING, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BNOTPRES);
    
    s[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint4", 1, (char *)s, 0L, 
            BFLD_STRING, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BNOTPRES);
    
    
    /* Test the ascii value to char */
    s[0] = EXEOS;
    NDRX_LOG(log_debug, "tchar1=%c", v.tchar1);
    
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar1", 0, (char *)s, 0L, 
            BFLD_STRING, BVACCESS_NOTNULL), 
            EXSUCCEED);
    assert_string_equal(s, "A");

    
    s[0] = EXEOS;
    NDRX_LOG(log_debug, "tchar2=%c", v.tchar1);
    
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar2", 0, (char *)s, 0L, 
            BFLD_STRING, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BNOTPRES);
    
    s[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar2", 1, (char *)s, 0L, 
            BFLD_STRING, BVACCESS_NOTNULL), 
            EXSUCCEED);
    assert_string_equal(s, "B");
    
    s[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar2", 2, (char *)s, 0L, 
            BFLD_STRING, BVACCESS_NOTNULL), 
            EXSUCCEED);
    assert_string_equal(s, "C");
    
    s[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar2", 3, (char *)s, 0L, 
            BFLD_STRING, BVACCESS_NOTNULL), 
            EXSUCCEED);
    assert_string_equal(s, "\n");
    
    s[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar2", 4, (char *)s, 0L, 
            BFLD_STRING, BVACCESS_NOTNULL), 
            EXSUCCEED);
    assert_string_equal(s, "\t");
    
    s[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar2", 5, (char *)s, 0L, 
            BFLD_STRING, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BEINVAL);
    
    s[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar3", 0, (char *)s, 0L, 
            BFLD_STRING, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BNOTPRES);
    
    s[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar3", 1, (char *)s, 0L, 
            BFLD_STRING, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BNOTPRES);
    
    s[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar3", 0, (char *)s, 0L, 
            BFLD_STRING, 0L), 
            EXSUCCEED);
    assert_string_equal(s, "C");
    
    s[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar3", 1, (char *)s, 0L, 
            BFLD_STRING, 0L), 
            EXSUCCEED);
    assert_string_equal(s, "D");
    
    v.tfloat1[1] = 66.0f;
    
    s[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tfloat1", 1, (char *)s, 0L, 
            BFLD_STRING, 0L), 
            EXSUCCEED);
    assert_string_equal(s, "66.00000");
    

    s[0] = EXEOS;
    v.tfloat2[0] = 1.22;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tfloat2", 0, (char *)s, 0L, 
            BFLD_STRING, 0L), 
            EXSUCCEED);
    assert_string_equal(s, "1.22000");
   
    
    s[0] = EXEOS;
    v.tdouble1[1] = 11.44;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tdouble1", 1, (char *)s, 0L, 
            BFLD_STRING, 0L), 
            EXSUCCEED);
    assert_string_equal(s,"11.440000");
    
    s[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tdouble2", 0, (char *)s, 0L, 
            BFLD_STRING, 0L), 
            EXSUCCEED);
    assert_string_equal(s,"-999.123000");
    
    
    NDRX_STRCPY_SAFE(v.tstring0[0], "HELLO WOLRD");
    
    s[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tstring0", 0, (char *)s, 0L, 
            BFLD_STRING, 0L), 
            EXSUCCEED);
    assert_string_equal(s,"HELLO WOLRD");
    
    
    v.L_tcarray2 = 6; /* includes EOS.. */
    NDRX_STRCPY_SAFE(v.tcarray2, "HELLOO");
        
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tcarray2", 0, (char *)s, 0L, 
            BFLD_STRING, 0L), 
            EXSUCCEED);
    assert_string_equal(s,"HELLOO");
    
}

/**
 * Test Bvget as carray
 */
Ensure(test_Bvget_carray)
{
    struct MYVIEW1 v;
    char c[255];
    BFLDLEN len;
    
    init_MYVIEW1(&v);
    
    c[0] = EXEOS;
    len = sizeof(c);
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort1", 0, (char *)c, &len, BFLD_CARRAY, 0L), 
            EXSUCCEED);
    assert_equal(0==strncmp(c, "15556", 5), 1);
    assert_equal(len, 5);
    
    c[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort1", 0, (char *)c, 0L, BFLD_CARRAY, 0L), 
            EXSUCCEED);
    assert_equal(0==strncmp(c, "15556", 5), 1);
    
    /* test buffer no space */
    c[0] = EXEOS;
    len = 1;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort1", 0, (char *)c, &len, BFLD_CARRAY, 0L), 
            EXFAIL);
    assert_equal(Berror, BNOSPACE);
    
    c[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort1", 0, (char *)c, 0L, BFLD_CARRAY, 0L), 
            EXSUCCEED);
    assert_equal(0==strncmp(c, "15556", 5), 1);
    
    /* Invalid value, index out of bounds.. */
    c[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort1", 1, (char *)c, 0L, BFLD_CARRAY, 0L), 
            EXFAIL);
    assert_equal(Berror, BEINVAL);
    
    c[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort2", 0, (char *)c, 0L, BFLD_CARRAY, 0L), 
            EXSUCCEED);
    c[4] =EXEOS;
    assert_string_equal(c, "9999");
    
    c[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort2", 1, (char *)c, 0L, BFLD_CARRAY, 0L), 
            EXSUCCEED);
    c[4] = EXEOS;
    assert_string_equal(c, "8888");
    
    c[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort3", 0, (char *)c, 0L, BFLD_CARRAY, 0L), 
            EXSUCCEED);
    assert_string_equal(c, "7777");
    
    c[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort3", 1, (char *)c, 0L, BFLD_CARRAY, 0L), 
            EXSUCCEED);
    c[5] = EXEOS;
    assert_string_equal(c, "-7777");
    
    c[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort3", 2, (char *)c, 0L, BFLD_CARRAY, 0L), 
            EXSUCCEED);
    c[5] = EXEOS;
    assert_string_equal(c, "-7777");
    
    c[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort3", 2, (char *)c, 0L, 
            BFLD_CARRAY, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BNOTPRES);
    
    c[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort4", 0, (char *)c, 0L, BFLD_CARRAY, 0L), 
            EXSUCCEED);
    c[3] = EXEOS;
    assert_string_equal(c, "-10");
    
    
    c[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tlong1", 0, (char *)c, 0L, BFLD_CARRAY, 0L), 
            EXSUCCEED);
    c[8] = EXEOS;
    assert_string_equal(c, "33333333");
    
    c[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint3", 0, (char *)c, 0L, BFLD_CARRAY, 0L), 
            EXSUCCEED);
    c[4] = EXEOS;
    assert_string_equal(c, "-100");
    
    c[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint4", 0, (char *)c, 0L, BFLD_CARRAY, 0L), 
            EXSUCCEED);
    c[7] = EXEOS;
    assert_string_equal(c, "1010101");
    
    c[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint4", 1, (char *)c, 0L, BFLD_CARRAY, 0L), 
            EXSUCCEED);
    c[6] = EXEOS;
    assert_string_equal(c, "989898");
    
    /* non existent field: */
    c[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "xint3", 0, (char *)c, 0L, BFLD_CARRAY, 0L), 
            EXFAIL);
    assert_equal(Berror, BNOCNAME);
    
    /* Test field NULL */
    
    v.tint4[0] = -1;
    v.tint4[1] = -1;
    c[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint4", 0, (char *)c, 0L, BFLD_CARRAY, 0L), 
            EXSUCCEED);
    c[2] = EXEOS;
    assert_string_equal(c, "-1");
    
    c[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint4", 1, (char *)c, 0L, BFLD_CARRAY, 0L), 
            EXSUCCEED);
    c[2] = EXEOS;
    assert_string_equal(c, "-1");
    
    /* not pres because of NULL value */
    c[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint4", 0, (char *)c, 0L, 
            BFLD_CARRAY, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BNOTPRES);
    
    c[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint4", 1, (char *)c, 0L, 
            BFLD_CARRAY, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BNOTPRES);
    
    
    /* Test the ascii value to char */
    c[0] = EXEOS;
    NDRX_LOG(log_debug, "tchar1=%c", v.tchar1);
    
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar1", 0, (char *)c, 0L, 
            BFLD_CARRAY, BVACCESS_NOTNULL), 
            EXSUCCEED);
    c[1] = EXEOS;
    assert_string_equal(c, "A");

    
    c[0] = EXEOS;
    NDRX_LOG(log_debug, "tchar2=%c", v.tchar1);
    
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar2", 0, (char *)c, 0L, 
            BFLD_CARRAY, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BNOTPRES);
    
    c[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar2", 1, (char *)c, 0L, 
            BFLD_CARRAY, BVACCESS_NOTNULL), 
            EXSUCCEED);
    c[1] = EXEOS;
    assert_string_equal(c, "B");
    
    c[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar2", 2, (char *)c, 0L, 
            BFLD_CARRAY, BVACCESS_NOTNULL), 
            EXSUCCEED);
    
    c[1] = EXEOS;
    assert_string_equal(c, "C");
    
    c[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar2", 3, (char *)c, 0L, 
            BFLD_CARRAY, BVACCESS_NOTNULL), 
            EXSUCCEED);
    c[1] = EXEOS;
    assert_string_equal(c, "\n");
    
    c[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar2", 4, (char *)c, 0L, 
            BFLD_CARRAY, BVACCESS_NOTNULL), 
            EXSUCCEED);
    c[1] = EXEOS;
    assert_string_equal(c, "\t");
    
    c[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar2", 5, (char *)c, 0L, 
            BFLD_CARRAY, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BEINVAL);
    
    c[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar3", 0, (char *)c, 0L, 
            BFLD_CARRAY, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BNOTPRES);
    
    c[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar3", 1, (char *)c, 0L, 
            BFLD_CARRAY, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BNOTPRES);
    
    c[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar3", 0, (char *)c, 0L, 
            BFLD_CARRAY, 0L), 
            EXSUCCEED);
    c[1] = EXEOS;
    assert_string_equal(c, "C");
    
    c[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar3", 1, (char *)c, 0L, 
            BFLD_CARRAY, 0L), 
            EXSUCCEED);
    c[1] = EXEOS;
    assert_string_equal(c, "D");
    
    v.tfloat1[1] = 66.0f;
    
    c[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tfloat1", 1, (char *)c, 0L, 
            BFLD_CARRAY, 0L), 
            EXSUCCEED);
    c[8] = EXEOS;
    assert_string_equal(c, "66.00000");
    

    c[0] = EXEOS;
    v.tfloat2[0] = 1.22;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tfloat2", 0, (char *)c, 0L, 
            BFLD_CARRAY, 0L), 
            EXSUCCEED);
    c[7] = EXEOS;
    assert_string_equal(c, "1.22000");
   
    
    c[0] = EXEOS;
    v.tdouble1[1] = 11.44;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tdouble1", 1, (char *)c, 0L, 
            BFLD_CARRAY, 0L), 
            EXSUCCEED);
    c[9] = EXEOS;
    assert_string_equal(c, "11.440000");
    
    c[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tdouble2", 0, (char *)c, 0L, 
            BFLD_CARRAY, 0L), 
            EXSUCCEED);
    c[11] = EXEOS;
    assert_string_equal(c,"-999.123000");
    
    
    NDRX_STRCPY_SAFE(v.tstring0[0], "HELLO WOLRD");
    
    c[0] = EXEOS;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tstring0", 0, (char *)c, 0L, 
            BFLD_CARRAY, 0L), 
            EXSUCCEED);
    c[11] =EXEOS;
    assert_string_equal(c, "HELLO WOLRD");
    
    
    v.L_tcarray2 = 6; /* includes EOS.. */
    NDRX_STRCPY_SAFE(v.tcarray2, "HELLOO");
        
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tcarray2", 0, (char *)c, 0L, 
            BFLD_CARRAY, 0L), 
            EXSUCCEED);
    
    c[6] = EXEOS;
    assert_string_equal(c,"HELLOO");
    
}

/**
 * Test Bvget as int
 */
Ensure(test_Bvget_int)
{
    struct MYVIEW1 v;
    int i;
    init_MYVIEW1(&v);
    
    i = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort1", 0, (char *)&i, 0L, BFLD_INT, 0L), 
            EXSUCCEED);
    assert_equal(i, 15556);
    
    /* Invalid value, index out of bounds.. */
    i = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort1", 1, (char *)&i, 0L, BFLD_INT, 0L), 
            EXFAIL);
    assert_equal(Berror, BEINVAL);
    
    i = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort2", 0, (char *)&i, 0L, BFLD_INT, 0L), 
            EXSUCCEED);
    assert_equal(i, 9999);
    
    i = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort2", 1, (char *)&i, 0L, BFLD_INT, 0L), 
            EXSUCCEED);
    assert_equal(i, 8888);
    
    i = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort3", 0, (char *)&i, 0L, BFLD_INT, 0L), 
            EXSUCCEED);
    assert_equal(i, 7777);
    
    i = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort3", 1, (char *)&i, 0L, BFLD_INT, 0L), 
            EXSUCCEED);
    assert_equal(i, -7777);
    
    i = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort3", 2, (char *)&i, 0L, BFLD_INT, 0L), 
            EXSUCCEED);
    assert_equal(i, -7777);
    
    i = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort3", 2, (char *)&i, 0L, 
            BFLD_INT, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BNOTPRES);
    
    i = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tshort4", 0, (char *)&i, 0L, BFLD_INT, 0L), 
            EXSUCCEED);
    assert_equal(i, -10);
    
    
    i = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tlong1", 0, (char *)&i, 0L, BFLD_INT, 0L), 
            EXSUCCEED);
    assert_equal(i, 33333333);
    
    i = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint3", 0, (char *)&i, 0L, BFLD_INT, 0L), 
            EXSUCCEED);
    assert_equal(i, -100);
    
    i = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint4", 0, (char *)&i, 0L, BFLD_INT, 0L), 
            EXSUCCEED);
    assert_equal(i, 1010101);
    
    i = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint4", 1, (char *)&i, 0L, BFLD_INT, 0L), 
            EXSUCCEED);
    assert_equal(i, 989898);
    
    /* non existent field: */
    i = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "xint3", 0, (char *)&i, 0L, BFLD_INT, 0L), 
            EXFAIL);
    assert_equal(Berror, BNOCNAME);
    
    /* Test field NULL */
    v.tint4[0] = -1;
    v.tint4[1] = -1;
    
    i = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint4", 0, (char *)&i, 0L, BFLD_INT, 0L), 
            EXSUCCEED);
    assert_equal(i, -1);
    
    i = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint4", 1, (char *)&i, 0L, BFLD_INT, 0L), 
            EXSUCCEED);
    assert_equal(i, -1);
    
    
    /* not pres because of NULL value */
    i = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint4", 0, (char *)&i, 0L, 
            BFLD_INT, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BNOTPRES);
    
    i = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tint4", 1, (char *)&i, 0L, 
            BFLD_INT, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BNOTPRES);
    
    
    /* Test the ascii value to char */
    i = 0;
    NDRX_LOG(log_debug, "tchar1=%c", v.tchar1);
    
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar1", 0, (char *)&i, 0L, 
            BFLD_INT, BVACCESS_NOTNULL), 
            EXSUCCEED);
    assert_equal(i, 65);
    
    
    i = 0;
    NDRX_LOG(log_debug, "tchar2=%c", v.tchar1);
    
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar2", 0, (char *)&i, 0L, 
            BFLD_INT, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BNOTPRES);
    
    i = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar2", 1, (char *)&i, 0L, 
            BFLD_INT, BVACCESS_NOTNULL), 
            EXSUCCEED);
    assert_equal(i, 66);
    
    i = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar2", 2, (char *)&i, 0L, 
            BFLD_INT, BVACCESS_NOTNULL), 
            EXSUCCEED);
    assert_equal(i, 67);
    
    i = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar2", 3, (char *)&i, 0L, 
            BFLD_INT, BVACCESS_NOTNULL), 
            EXSUCCEED);
    assert_equal(i, '\n');
    
    i = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar2", 4, (char *)&i, 0L, 
            BFLD_INT, BVACCESS_NOTNULL), 
            EXSUCCEED);
    assert_equal(i, '\t');
    
    i = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar2", 5, (char *)&i, 0L, 
            BFLD_INT, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BEINVAL);
    
    i = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar3", 0, (char *)&i, 0L, 
            BFLD_INT, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BNOTPRES);
    
    i = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar3", 1, (char *)&i, 0L, 
            BFLD_INT, BVACCESS_NOTNULL), 
            EXFAIL);
    assert_equal(Berror, BNOTPRES);
    
    i = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar3", 0, (char *)&i, 0L, 
            BFLD_INT, 0L), 
            EXSUCCEED);
    assert_equal(i, 'C');
    
    i = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tchar3", 1, (char *)&i, 0L, 
            BFLD_INT, 0L), 
            EXSUCCEED);
    assert_equal(i, 'D');
    

    v.tfloat1[1] = 66.0f;
    
    i = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tfloat1", 1, (char *)&i, 0L, 
            BFLD_INT, 0L), 
            EXSUCCEED);
    assert_equal(i, 66);
    
    
    i = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tfloat2", 0, (char *)&i, 0L, 
            BFLD_INT, 0L), 
            EXSUCCEED);
    assert_equal(i, 100000);
   
    
    i = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tdouble1", 1, (char *)&i, 0L, 
            BFLD_INT, 0L), 
            EXSUCCEED);
    assert_equal(i,11111);
    
    i = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tdouble2", 0, (char *)&i, 0L, 
            BFLD_INT, 0L), 
            EXSUCCEED);
    assert_equal(i,-999);
    
    
    NDRX_STRCPY_SAFE(v.tstring0[0], "125");
    
    i = 0;
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tstring0", 0, (char *)&i, 0L, 
            BFLD_INT, 0L), 
            EXSUCCEED);
    assert_equal(i,125);
    
    
    v.L_tcarray2 = 4; /* includes EOS.. */
    NDRX_STRCPY_SAFE(v.tcarray2, "521");
    
    
    assert_equal(CBvget((char *)&v, "MYVIEW1", "tcarray2", 0, (char *)&i, 0L, 
            BFLD_INT, 0L), 
            EXSUCCEED);
    assert_equal(i,521);
    
}


/**
 * Very basic tests of the framework
 * @return
 */
TestSuite *vacc_CBvget_tests(void) {
    TestSuite *suite = create_test_suite();
    
    set_setup(suite, basic_setup);
    set_teardown(suite, basic_teardown);

    /* init view test */
    add_test(suite, test_Bvget_short);
    add_test(suite, test_Bvget_long);
    add_test(suite, test_Bvget_char);
    add_test(suite, test_Bvget_float);
    add_test(suite, test_Bvget_double);
    add_test(suite, test_Bvget_string);
    add_test(suite, test_Bvget_carray);
    add_test(suite, test_Bvget_int);
            
    return suite;
}
