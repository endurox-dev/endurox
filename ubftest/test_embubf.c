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
    
    
    /*
     Bprint(p_ub); 
     */
    
    /*

T_LONG_3_FLD	0
T_LONG_3_FLD	0
T_LONG_3_FLD	0
T_LONG_3_FLD	889991
T_DOUBLE_3_FLD	1.110000
T_DOUBLE_3_FLD	2.412310
T_STRING_10_FLD	HELLO WORLD5
T_STRING_3_FLD	HELLO WORLD7
T_STRING_4_FLD	HELLO WORLD
T_STRING_4_FLD	HELLO WORLD6
T_STRING_5_FLD	HELLO WORLD2
T_STRING_6_FLD	HELLO WORLD4
T_STRING_7_FLD	HELLO WORLD3
T_STRING_8_FLD	HELLO WORLD1
T_STRING_9_FLD	01
T_STRING_9_FLD	20
T_STRING_9_FLD	31
T_CARRAY_2_FLD	
T_CARRAY_2_FLD	TEST CARRAY
T_PTR_3_FLD	0xc7
T_PTR_3_FLD	0x12b
T_UBF_FLD	
	T_LONG_3_FLD	0
	T_LONG_3_FLD	0
	T_LONG_3_FLD	0
	T_LONG_3_FLD	889991
	T_DOUBLE_3_FLD	1.110000
	T_DOUBLE_3_FLD	2.412310
	T_STRING_10_FLD	HELLO WORLD5
	T_STRING_3_FLD	HELLO WORLD7
	T_STRING_4_FLD	HELLO WORLD
	T_STRING_4_FLD	HELLO WORLD6
	T_STRING_5_FLD	HELLO WORLD2
	T_STRING_6_FLD	HELLO WORLD4
	T_STRING_7_FLD	HELLO WORLD3
	T_STRING_8_FLD	HELLO WORLD1
	T_STRING_9_FLD	01
	T_STRING_9_FLD	20
	T_STRING_9_FLD	31
	T_CARRAY_2_FLD	
	T_CARRAY_2_FLD	TEST CARRAY
	T_PTR_3_FLD	0xc7
	T_PTR_3_FLD	0x12b
	T_UBF_3_FLD	
	T_UBF_3_FLD	
	T_UBF_3_FLD	
		T_STRING_9_FLD	
		T_STRING_9_FLD	
		T_STRING_9_FLD	
		T_STRING_9_FLD	HELLO WORLD UB
	T_UBF_3_FLD	
		T_STRING_7_FLD	
		T_STRING_7_FLD	
		T_STRING_7_FLD	ANOTHER UB
		T_STRING_9_FLD	
		T_STRING_9_FLD	
		T_STRING_9_FLD	
		T_STRING_9_FLD	HELLO WORLD UB
	T_VIEW_3_FLD	UBTESTVIEW2
		tshort1	100
		tlong1	200
		tchar1	G
		tfloat1	400.00000
		tdouble1	500.000000
		tstring1	6XX
		tcarray1	7XX\00\00\00\00\00\00\00
	T_VIEW_3_FLD	
	T_VIEW_3_FLD	
	T_VIEW_3_FLD	
	T_VIEW_3_FLD	UBTESTVIEW2
		tshort1	100
		tlong1	200
		tchar1	Z
		tfloat1	400.00000
		tdouble1	500.000000
		tstring1	6YY
		tcarray1	7YY\00\00\00\00\00\00\00
T_UBF_2_FLD	
T_UBF_2_FLD	
	T_STRING_2_FLD	HELLO_PARENT
	T_STRING_4_FLD	HELLO_PARENT 2
	T_UBF_FLD	
		T_STRING_3_FLD	HELLO_CHILD
		T_STRING_3_FLD	HELLO_CHILD 2
		T_STRING_3_FLD	HELLO_CHILD 3
		T_STRING_3_FLD	HELLO_CHILD 4
		T_UBF_2_FLD	
			T_LONG_3_FLD	0
			T_LONG_3_FLD	0
			T_LONG_3_FLD	0
			T_LONG_3_FLD	889991
			T_DOUBLE_3_FLD	1.110000
			T_DOUBLE_3_FLD	2.412310
			T_STRING_10_FLD	HELLO WORLD5
			T_STRING_3_FLD	HELLO WORLD7
			T_STRING_4_FLD	HELLO WORLD
			T_STRING_4_FLD	HELLO WORLD6
			T_STRING_5_FLD	HELLO WORLD2
			T_STRING_6_FLD	HELLO WORLD4
			T_STRING_7_FLD	HELLO WORLD3
			T_STRING_8_FLD	HELLO WORLD1
			T_STRING_9_FLD	01
			T_STRING_9_FLD	20
			T_STRING_9_FLD	31
			T_CARRAY_2_FLD	
			T_CARRAY_2_FLD	TEST CARRAY
			T_PTR_3_FLD	0xc7
			T_PTR_3_FLD	0x12b
			T_UBF_3_FLD	
			T_UBF_3_FLD	
			T_UBF_3_FLD	
				T_STRING_9_FLD	
				T_STRING_9_FLD	
				T_STRING_9_FLD	
				T_STRING_9_FLD	HELLO WORLD UB
			T_UBF_3_FLD	
				T_STRING_7_FLD	
				T_STRING_7_FLD	
				T_STRING_7_FLD	ANOTHER UB
				T_STRING_9_FLD	
				T_STRING_9_FLD	
				T_STRING_9_FLD	
				T_STRING_9_FLD	HELLO WORLD UB
			T_VIEW_3_FLD	UBTESTVIEW2
				tshort1	100
				tlong1	200
				tchar1	G
				tfloat1	400.00000
				tdouble1	500.000000
				tstring1	6XX
				tcarray1	7XX\00\00\00\00\00\00\00
			T_VIEW_3_FLD	
			T_VIEW_3_FLD	
			T_VIEW_3_FLD	
			T_VIEW_3_FLD	UBTESTVIEW2
				tshort1	100
				tlong1	200
				tchar1	Z
				tfloat1	400.00000
				tdouble1	500.000000
				tstring1	6YY
				tcarray1	7YY\00\00\00\00\00\00\00
		T_VIEW_FLD	
		T_VIEW_FLD	UBTESTVIEW1
			tshort1	15556
			tshort2	9999
			tshort2	8888
			tshort3	7777
			tshort3	-7777
			tshort4	-10
			tlong1	33333333
			tint2	54545
			tint2	23232
			tint3	-100
			tint4	1010101
			tint4	989898
			tchar1	A
			tchar2	A
			tchar2	B
			tchar2	C
			tchar2	

			tchar2		
			tfloat1	-0.11000
			tfloat1	-0.22000
			tfloat1	0.33000
			tfloat1	0.44000
			tfloat2	100000.10156
			tfloat2	200000.20312
			tfloat3	333333.12500
			tdouble1	99999.111111
			tdouble1	11111.999999
			tdouble2	-999.123000
			tstring0	HELLO Enduro/X
			tstring0	
			tstring0	\0aABC\0a
			tstring1	Pack my box
			tstring1	BOX MY PACK
			tstring1	\0aEnduro/X\0a
			tstring2	CCCCAAAADDDD
			tstring2	EEEFFFGGG
			tstring3	LLLLLL
			tstring3	MMMMMM
			tstring3	
			tstring3	NNNNNN
			tstring4	Some string
			tstring5	MEGA TEST
			tcarray1	\00\01\02\03\04\05\06\07\08\09\0a\0b\0c\0d\0e\0f\10\11\12\13\14\15\16\17\18\19\1a\1b\1c\1d
			tcarray2	\01\02\03\04\05
			tcarray3	\02
			tcarray3	\02\03
			tcarray3	\02\03\04
			tcarray3	\02\03\04\05
			tcarray3	\02\03\04\05\06
			tcarray3	\02\03\04\05\06\07
			tcarray3	\02\03\04\05\06\07\08
			tcarray3	\02\03\04\05\06\07\08\09
			tcarray3	\02\03\04\05\06\07\08\09\0a
			tcarray4	\03\04\05\06\07
			tcarray5	\04\05\06\07\08
	T_UBF_FLD	
		T_LONG_3_FLD	0
		T_LONG_3_FLD	0
		T_LONG_3_FLD	0
		T_LONG_3_FLD	889991
		T_DOUBLE_3_FLD	1.110000
		T_DOUBLE_3_FLD	2.412310
		T_STRING_10_FLD	HELLO WORLD5
		T_STRING_3_FLD	HELLO WORLD7
		T_STRING_4_FLD	HELLO WORLD
		T_STRING_4_FLD	HELLO WORLD6
		T_STRING_5_FLD	HELLO WORLD2
		T_STRING_6_FLD	HELLO WORLD4
		T_STRING_7_FLD	HELLO WORLD3
		T_STRING_8_FLD	HELLO WORLD1
		T_STRING_9_FLD	01
		T_STRING_9_FLD	20
		T_STRING_9_FLD	31
		T_CARRAY_2_FLD	
		T_CARRAY_2_FLD	TEST CARRAY
		T_PTR_3_FLD	0xc7
		T_PTR_3_FLD	0x12b
		T_UBF_3_FLD	
		T_UBF_3_FLD	
		T_UBF_3_FLD	
			T_STRING_9_FLD	
			T_STRING_9_FLD	
			T_STRING_9_FLD	
			T_STRING_9_FLD	HELLO WORLD UB
		T_UBF_3_FLD	
			T_STRING_7_FLD	
			T_STRING_7_FLD	
			T_STRING_7_FLD	ANOTHER UB
			T_STRING_9_FLD	
			T_STRING_9_FLD	
			T_STRING_9_FLD	
			T_STRING_9_FLD	HELLO WORLD UB
		T_VIEW_3_FLD	UBTESTVIEW2
			tshort1	100
			tlong1	200
			tchar1	G
			tfloat1	400.00000
			tdouble1	500.000000
			tstring1	6XX
			tcarray1	7XX\00\00\00\00\00\00\00
		T_VIEW_3_FLD	
		T_VIEW_3_FLD	
		T_VIEW_3_FLD	
		T_VIEW_3_FLD	UBTESTVIEW2
			tshort1	100
			tlong1	200
			tchar1	Z
			tfloat1	400.00000
			tdouble1	500.000000
			tstring1	6YY
			tcarray1	7YY\00\00\00\00\00\00\00
T_UBF_3_FLD	
T_UBF_3_FLD	
T_UBF_3_FLD	
	T_STRING_9_FLD	
	T_STRING_9_FLD	
	T_STRING_9_FLD	
	T_STRING_9_FLD	HELLO WORLD UB
T_UBF_3_FLD	
	T_STRING_7_FLD	
	T_STRING_7_FLD	
	T_STRING_7_FLD	ANOTHER UB
	T_STRING_9_FLD	
	T_STRING_9_FLD	
	T_STRING_9_FLD	
	T_STRING_9_FLD	HELLO WORLD UB
T_VIEW_3_FLD	UBTESTVIEW2
	tshort1	100
	tlong1	200
	tchar1	G
	tfloat1	400.00000
	tdouble1	500.000000
	tstring1	6XX
	tcarray1	7XX\00\00\00\00\00\00\00
T_VIEW_3_FLD	
T_VIEW_3_FLD	
T_VIEW_3_FLD	
T_VIEW_3_FLD	UBTESTVIEW2
	tshort1	100
	tlong1	200
	tchar1	Z
	tfloat1	400.00000
	tdouble1	500.000000
	tstring1	6YY
	tcarray1	7YY\00\00\00\00\00\00\00
     
     */
    
}

Ensure(test_Bgetr)
{
    char buf[56000];
    char buf_tmp[56000];
    UBFH *p_ub = (UBFH *)buf;
    char tmp[1024];
    BFLDLEN len;
    long l;
    UBFH *p_ub_tmp = (UBFH *)buf_tmp;
    BVIEWFLD vf;
    struct UBTESTVIEW2 v;
    
    assert_equal(Binit(p_ub, sizeof(buf)), EXSUCCEED);
    load_recursive_data(p_ub);
    
    /* check error path: */
    
    assert_equal(Bgetr (p_ub, (int []){BBADFLDOCC}, 
            tmp, &len), EXFAIL);
    assert_equal(Berror, BBADFLD);

    assert_equal(Bgetr (p_ub, (int []){ T_UBF_2_FLD, BBADFLDOCC}, 
            tmp, &len), EXFAIL);
    assert_equal(Berror, BBADFLD);
    
    assert_equal(Bgetrv (p_ub, tmp, &len, T_UBF_2_FLD, BBADFLDOCC), EXFAIL);
    assert_equal(Berror, BBADFLD);
    
    /* check invalid subfield (i.e subfield of non UBF..) */
    assert_equal(Bgetr (p_ub, (int []){ T_STRING_8_FLD, 0, T_STRING_10_FLD, 0, BBADFLDOCC}, 
            tmp, &len), EXFAIL);
    assert_equal(Berror, BEBADOP);
    
    assert_equal(Bgetrv (p_ub, tmp, &len, 
            T_STRING_8_FLD, 0, T_STRING_10_FLD, 0, BBADFLDOCC), EXFAIL);
    assert_equal(Berror, BEBADOP);
    
    /* check field not found */
    
    assert_equal(Bgetrv (p_ub, tmp, &len, 
            T_UBF_2_FLD, 1, T_UBF_FLD, 4, T_STRING_3_FLD, 1, BBADFLDOCC), EXFAIL);
    assert_equal(Berror, BNOTPRES);
    
    /* check no space.. */
    
    /* check standard version */
    len=sizeof(tmp);
    assert_equal(Bgetr (p_ub, (int []){ T_UBF_2_FLD, 1, T_UBF_FLD, 0, T_STRING_3_FLD, 1, BBADFLDOCC}, 
            tmp, &len), EXSUCCEED);
    assert_string_equal(tmp, "HELLO_CHILD 2");
    assert_equal(len, 14);
    
    /* Check the v version */
    len=sizeof(tmp);
    tmp[0]=EXEOS;
    
    assert_equal(Bgetrv (p_ub, tmp, &len, 
            T_UBF_2_FLD, 1, T_UBF_FLD, 0, T_STRING_3_FLD, 1, BBADFLDOCC), EXSUCCEED);
    assert_string_equal(tmp, "HELLO_CHILD 2");
    assert_equal(len, 14);

    /* read fixed len for level 0*/
    
    assert_equal(Bgetr (p_ub, (int []){ T_LONG_3_FLD, 3, BBADFLDOCC}, (char *)&l, &len), EXSUCCEED);
    assert_equal(l, 889991);
    
    l=0;
    assert_equal(Bgetrv (p_ub, (char *)&l, &len, T_LONG_3_FLD, 3, BBADFLDOCC), EXSUCCEED);
    assert_equal(l, 889991);
    
    
    /* Read the UBF... and validate dummy data */
    
    /* check no space.. */
    len=1;
    assert_equal(Bgetr (p_ub, (int []){ T_UBF_2_FLD,1,T_UBF_FLD,0,T_UBF_2_FLD,0,BBADFLDOCC}, 
            (char *)p_ub_tmp, &len), EXFAIL);
    assert_equal(Berror, BNOSPACE);
    
    len=sizeof(buf_tmp);
    assert_equal(Bgetr (p_ub, (int []){ T_UBF_2_FLD,1,T_UBF_FLD,0,T_UBF_2_FLD,0,BBADFLDOCC}, 
            (char *)p_ub_tmp, &len), EXSUCCEED);
    do_dummy_data_test(p_ub_tmp);
    
    vf.data=(char *)&v;
    len=sizeof(v);
    assert_equal(Bgetrv (p_ub, (char *)&vf, &len, 
            T_UBF_2_FLD, 1, T_UBF_FLD, 0, T_UBF_2_FLD, 0, T_VIEW_3_FLD, 4, BBADFLDOCC), EXSUCCEED);
    assert_equal(v.tlong1, 200);
    
}

Ensure(test_CBgetr)
{
    char buf[56000];
    char buf_tmp[56000];
    long l=0;
    BFLDLEN len;
    UBFH *p_ub = (UBFH *)buf;
    
    assert_equal(Binit(p_ub, sizeof(buf)), EXSUCCEED);
    load_recursive_data(p_ub);
    
    /* convert ok */
    assert_equal(CBgetr (p_ub, (int []){T_UBF_FLD,0,T_STRING_9_FLD,1,BBADFLDOCC}, 
            (char *)&l, NULL, BFLD_LONG), EXSUCCEED);
    assert_equal(l, 20);
    
    /* check no space */
    len=1;
    assert_equal(CBgetrv (p_ub, (char *)buf_tmp, &len, BFLD_STRING, 
            T_UBF_FLD,0,T_UBF_3_FLD,2,T_STRING_9_FLD,3,BBADFLDOCC), EXFAIL);
    assert_equal(Berror, BNOSPACE);
    
    /* check ok */
    len=sizeof(buf_tmp);
    assert_equal(CBgetrv (p_ub, (char *)buf_tmp, &len, BFLD_STRING, 
            T_UBF_FLD,0,T_UBF_3_FLD,2,T_STRING_9_FLD,3,BBADFLDOCC), EXSUCCEED);
    assert_string_equal(buf_tmp, "HELLO WORLD UB");
    
}

Ensure(test_Bfindr)
{
    char buf[56000];
    char *ptr;
    BVIEWFLD *vf;
    struct UBTESTVIEW2 *v;
    BFLDLEN len;
    UBFH *p_ub = (UBFH *)buf;
    
    assert_equal(Binit(p_ub, sizeof(buf)), EXSUCCEED);
    load_recursive_data(p_ub);
    
    /* find value OK with len */
    ptr=Bfindrv(p_ub, &len, T_UBF_2_FLD,1,T_UBF_FLD,0,T_UBF_2_FLD,0,T_VIEW_3_FLD,4,BBADFLDOCC);
    
    assert_not_equal(ptr, NULL);
    vf=(BVIEWFLD *)ptr;
    v=(struct UBTESTVIEW2 *)vf->data;
    
    assert_string_equal(vf->vname, "UBTESTVIEW2");
    assert_equal(len, sizeof(struct UBTESTVIEW2));
    /* check view values... */
    assert_string_equal(v->tstring1, "6YY");
    
    /* check the error handling.. */
    ptr=Bfindrv(p_ub, &len, T_UBF_2_FLD,1,T_UBF_FLD,0,T_UBF_2_FLD,0,T_VIEW_3_FLD,10,BBADFLDOCC);
    assert_equal(ptr, NULL);
    assert_equal(Berror, BNOTPRES);
    
}

Ensure(test_CBfindr)
{
    char buf[56000];
    char *p;
    BFLDLEN len;
    long *lv;
    UBFH *p_ub = (UBFH *)buf;
    
    assert_equal(Binit(p_ub, sizeof(buf)), EXSUCCEED);
    load_recursive_data(p_ub);
    
    p=CBfindrv(p_ub, &len, BFLD_LONG, T_UBF_FLD,0,T_STRING_9_FLD,2,BBADFLDOCC);
    assert_not_equal(p, NULL);
    lv = (long *)p;
    assert_equal(*lv, 31);
    assert_equal(len, sizeof(long));
    
    /* check error */
    p=CBfindrv(p_ub, &len, BFLD_LONG, T_UBF_FLD,0,T_STRING_9_FLD,1000,BBADFLDOCC);
    assert_equal(p, NULL);
    assert_equal(Berror, BNOTPRES);
}

Ensure(test_CBvgetr)
{
    char buf[56000];
    char buf_tmp[56000];
    BFLDLEN len;
    UBFH *p_ub = (UBFH *)buf;
    
    assert_equal(Binit(p_ub, sizeof(buf)), EXSUCCEED);
    load_recursive_data(p_ub);
    
    /* get view field from recursive buffer */
    len=sizeof(buf_tmp);
    assert_equal(CBvgetrv(p_ub, "tstring0", 2, buf_tmp, &len, BFLD_STRING, 0,
            T_UBF_2_FLD,1,T_UBF_FLD,0,T_VIEW_FLD,1,BBADFLDOCC), EXSUCCEED);
    assert_string_equal(buf_tmp, "\nABC\n");
    
    /* check the error */
    len=0;
    assert_equal(CBvgetrv(p_ub, "tstring0", 2, buf_tmp, &len, BFLD_STRING, 0,
            T_UBF_2_FLD,1,T_UBF_FLD,0,T_VIEW_FLD,1,BBADFLDOCC), EXFAIL);
    assert_equal(Berror, BNOSPACE);
    
    /* check type cast */
    len=sizeof(buf_tmp);
    assert_equal(CBvgetrv(p_ub, "tint2", 1, buf_tmp, &len, BFLD_STRING, 0,
            T_UBF_2_FLD,1,T_UBF_FLD,0,T_VIEW_FLD,1,BBADFLDOCC), EXSUCCEED);
    assert_string_equal(buf_tmp, "23232");
    
}


Ensure(test_Bvnullr)
{
    char buf[56000];
    char buf_tmp[56000];
    UBFH *p_ub = (UBFH *)buf;
    
    assert_equal(Binit(p_ub, sizeof(buf)), EXSUCCEED);
    load_recursive_data(p_ub);
    
    /* check null field... */
    assert_equal(Bvnullrv(p_ub, "tchar2", 0, T_UBF_2_FLD,1,T_UBF_FLD,0,T_VIEW_FLD,1,BBADFLDOCC), EXTRUE);
    assert_equal(Bvnullrv(p_ub, "tchar2", 1, T_UBF_2_FLD,1,T_UBF_FLD,0,T_VIEW_FLD,1,BBADFLDOCC), EXFALSE);
    assert_equal(Bvnullrv(p_ub, "tcarray3", 9, T_UBF_2_FLD,1,T_UBF_FLD,0,T_VIEW_FLD,1,BBADFLDOCC), EXTRUE);
    
    /* check errors */
    assert_equal(Bvnullrv(p_ub, "tchar2", 10, T_UBF_2_FLD,1,T_UBF_FLD,0,T_VIEW_FLD,1,BBADFLDOCC), EXFAIL);
    assert_equal(Berror, BEINVAL);

    /* check unknown view ? */
    assert_equal(Bvnullrv(p_ub, "tchar2", 10, T_UBF_2_FLD,1,T_UBF_FLD,0,T_VIEW_FLD,0,BBADFLDOCC), EXFAIL);
    assert_equal(Berror, BEINVAL);
}

/**
 * Common suite entry
 * @return
 */
TestSuite *ubf_embubf_tests(void)
{
    TestSuite *suite = create_test_suite();

    std_basic_setup();
    
    add_test(suite, test_Bgetr);
    
    add_test(suite, test_CBgetr);
    add_test(suite, test_Bfindr);
    add_test(suite, test_CBfindr);
    add_test(suite, test_CBvgetr);
    add_test(suite, test_Bvnullr);    
    return suite;
}
/* vim: set ts=4 sw=4 et smartindent: */
