/**
 * @brief General purpose *get* testing file
 *
 * @file test_get.c
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
#include <string.h>
#include "test.fd.h"
#include "ubfunit1.h"

extern void load_find_test_data(UBFH *p_ub);

void load_get_test_data(UBFH *p_ub)
{
    short s = 88;
    long l = -1021;
    char c = 'c';
    float f = 17.31;
    double d = 12312.1111;
    char carr[] = "CARRAY1 TEST STRING DATA";
    BFLDLEN len = strlen(carr);

    assert_equal(Bchg(p_ub, T_SHORT_FLD, 0, (char *)&s, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_LONG_FLD, 0, (char *)&l, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_CHAR_FLD, 0, (char *)&c, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_FLOAT_FLD, 0, (char *)&f, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_DOUBLE_FLD, 0, (char *)&d, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_STRING_FLD, 0, (char *)"TEST STR VAL", 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_CARRAY_FLD, 0, (char *)carr, len), EXSUCCEED);
    
    gen_load_ptr(p_ub, 0, 0, 0);
    gen_load_ubf(p_ub, 0, 0, 0);
    gen_load_view(p_ub, 0, 0, 0);

    /* Make second copy of field data (another for not equal test)*/
    s = 88;
    l = -1021;
    c = '.';
    f = 17.31;
    d = 12312.1111;
    carr[0] = 'Y';
    len = strlen(carr);

    assert_equal(Bchg(p_ub, T_SHORT_FLD, 1, (char *)&s, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_LONG_FLD, 1, (char *)&l, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_CHAR_FLD, 1, (char *)&c, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_FLOAT_FLD, 1, (char *)&f, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_DOUBLE_FLD, 1, (char *)&d, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_STRING_FLD, 1, (char *)"TEST STRING ARRAY2", 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_CARRAY_FLD, 1, (char *)carr, len), EXSUCCEED);

    gen_load_ptr(p_ub, 1, 1, 0);    
    gen_load_ubf(p_ub, 1, 1, 0);
    gen_load_view(p_ub, 1, 1, 0);


    l = 888;
    assert_equal(Bchg(p_ub, T_LONG_FLD, 4, (char *)&l, 0), EXSUCCEED);

    s = 212;
    l = 212;
    c = 'b';
    f = 12127;
    d = 1231232.1;
    carr[0] = 'X';
    assert_equal(Bchg(p_ub, T_SHORT_2_FLD, 0, (char *)&s, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_LONG_2_FLD, 0, (char *)&l, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_CHAR_2_FLD, 0, (char *)&c, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_FLOAT_2_FLD, 0, (char *)&f, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_DOUBLE_2_FLD, 0, (char *)&d, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_STRING_2_FLD, 0, (char *)"XTEST STR VAL", 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_CARRAY_2_FLD, 0, (char *)carr, len), EXSUCCEED);
    
    gen_load_ptr(p_ub, 0, 2, 1);    
    gen_load_ubf(p_ub, 0, 2, 1);
    gen_load_view(p_ub, 0, 2, 1);
    
}

/**
 * Test data used for Bfindlast
 * @param p_ub
 */
void load_get_test_data_2(UBFH *p_ub)
{
    short s = 88;
    long l = -1021;
    char c = 'c';
    float f = 17.31;
    double d = 12312.1111;
    char carr[] = "CARRAY1 TEST STRING DATA";
    BFLDLEN len = strlen(carr);

    assert_equal(Bchg(p_ub, T_SHORT_FLD, 0, (char *)&s, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_LONG_FLD, 1, (char *)&l, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_CHAR_FLD, 2, (char *)&c, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_FLOAT_FLD, 3, (char *)&f, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_DOUBLE_FLD, 4, (char *)&d, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_STRING_FLD, 5, (char *)"TEST STR VAL", 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_CARRAY_FLD, 6, (char *)carr, len), EXSUCCEED);
    
    gen_load_ptr(p_ub, 7, 2, 0);    
    gen_load_ubf(p_ub, 8, 2, 0);
    gen_load_view(p_ub, 9, 2, 0);

}

/**
 * This simply reads all field and adds them to another buffer, then do compare
 */
Ensure(test_cbgetalloc)
{
    char fb[2048];
    UBFH *p_ub = (UBFH *)fb;
    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);
    load_find_test_data(p_ub);
    short *s1,*s2,*s3,*s4,*s5,*s6,*s7,*s8,*s9,*s10;
    long *l1,*l2,*l3,*l4,*l5,*l6,*l7,*l8,*l9,*l10;
    char *c1,*c2,*c3,*c4,*c5,*c6,*c7,*c8,*c9,*c10;
    float *f1,*f2,*f3,*f4,*f5,*f6,*f7,*f8,*f9,*f10;
    double *d1,*d2,*d3,*d4,*d5,*d6,*d7,*d8,*d9,*d10;
    char *str1,*str2,*str3,*str4,*str5,*str6,*str7,*str8,*str9,*str10;
    char *carr1,*carr2,*carr3,*carr4,*carr5,*carr6,*carr7,*carr8,*carr9,*carr10;
    ndrx_longptr_t *p1,*p2,*p3,*p4,*p5,*p6,*p7,*p8,*p9,*p10;
    
    /* not supported modes: */
    UBFH *u1;
    BVIEWFLD *v1;
    BFLDLEN len=0;

    /* Test as short */
    assert_not_equal((s1=(short *)CBgetalloc(p_ub, T_SHORT_FLD, 0, BFLD_SHORT, 0)), NULL);
    assert_not_equal((s2=(short *)CBgetalloc(p_ub, T_LONG_FLD, 0, BFLD_SHORT, 0)), NULL);
    assert_not_equal((s3=(short *)CBgetalloc(p_ub, T_CHAR_FLD, 0, BFLD_SHORT, 0)), NULL);
    assert_not_equal((s4=(short *)CBgetalloc(p_ub, T_FLOAT_FLD, 0, BFLD_SHORT, 0)), NULL);
    assert_not_equal((s5=(short *)CBgetalloc(p_ub, T_DOUBLE_FLD, 0, BFLD_SHORT, 0)), NULL);
    assert_not_equal((s6=(short *)CBgetalloc(p_ub, T_STRING_FLD, 0, BFLD_SHORT, 0)), NULL);
    assert_not_equal((s7=(short *)CBgetalloc(p_ub, T_CARRAY_FLD, 0, BFLD_SHORT, 0)), NULL);
    assert_not_equal((s8=(short *)CBgetalloc(p_ub, T_PTR_FLD, 0, BFLD_SHORT, 0)), NULL);
    assert_equal((s9=(short *)CBgetalloc(p_ub, T_UBF_FLD, 0, BFLD_SHORT, 0)), NULL);
    assert_equal(Berror, BEBADOP);
    assert_equal((s10=(short *)CBgetalloc(p_ub, T_VIEW_FLD, 0, BFLD_SHORT, 0)), NULL);
    assert_equal(Berror, BEBADOP);
    
    assert_equal(*s1, 88);
    assert_equal(*s2, -1021);
    assert_equal(*s3, 99);
    assert_equal(*s4, 17);
    assert_equal(*s5, 12312);
    assert_equal(*s6, 0);
    assert_equal(*s7, 0);
    assert_equal(*s8, 9000);

    /* Free up memory */
    free(s1);free(s2);free(s3);free(s4);free(s5);free(s6);free(s7);free(s8);

    /* Test as long */
    assert_not_equal((l1=(long *)CBgetalloc(p_ub, T_SHORT_FLD, 0, BFLD_LONG, 0)), NULL);
    assert_not_equal((l2=(long *)CBgetalloc(p_ub, T_LONG_FLD, 0, BFLD_LONG, 0)), NULL);
    assert_not_equal((l3=(long *)CBgetalloc(p_ub, T_CHAR_FLD, 0, BFLD_LONG, 0)), NULL);
    assert_not_equal((l4=(long *)CBgetalloc(p_ub, T_FLOAT_FLD, 0, BFLD_LONG, 0)), NULL);
    assert_not_equal((l5=(long *)CBgetalloc(p_ub, T_DOUBLE_FLD, 0, BFLD_LONG, 0)), NULL);
    assert_not_equal((l6=(long *)CBgetalloc(p_ub, T_STRING_FLD, 0, BFLD_LONG, 0)), NULL);
    assert_not_equal((l7=(long *)CBgetalloc(p_ub, T_CARRAY_FLD, 0, BFLD_LONG, 0)), NULL);

    assert_not_equal((l8=(long *)CBgetalloc(p_ub, T_PTR_FLD, 0, BFLD_LONG, 0)), NULL);
    assert_equal((l9=(long *)CBgetalloc(p_ub, T_UBF_FLD, 0, BFLD_LONG, 0)), NULL);
    assert_equal(Berror, BEBADOP);
    assert_equal((l10=(long *)CBgetalloc(p_ub, T_VIEW_FLD, 0, BFLD_LONG, 0)), NULL);
    assert_equal(Berror, BEBADOP);
    
    assert_equal(*l1, 88);
    assert_equal(*l2, -1021);
    assert_equal(*l3, 99);
    assert_equal(*l4, 17);
    assert_equal(*l5, 12312);
    assert_equal(*l6, 0);
    assert_equal(*l7, 0);
    assert_equal(*l8, 9000);

    free(l1);free(l2);free(l3);free(l4);free(l5);free(l6);free(l7);free(l8);

    /* Test as char */
    assert_not_equal((c1=(char *)CBgetalloc(p_ub, T_SHORT_FLD, 0, BFLD_CHAR, 0)), NULL);
    assert_not_equal((c2=(char *)CBgetalloc(p_ub, T_LONG_FLD, 0, BFLD_CHAR, 0)), NULL);
    assert_not_equal((c3=(char *)CBgetalloc(p_ub, T_CHAR_FLD, 0, BFLD_CHAR, 0)), NULL);
    assert_not_equal((c4=(char *)CBgetalloc(p_ub, T_FLOAT_FLD, 0, BFLD_CHAR, 0)), NULL);
    assert_not_equal((c5=(char *)CBgetalloc(p_ub, T_DOUBLE_FLD, 0, BFLD_CHAR, 0)), NULL);
    assert_not_equal((c6=(char *)CBgetalloc(p_ub, T_STRING_FLD, 0, BFLD_CHAR, 0)), NULL);
    assert_not_equal((c7=(char *)CBgetalloc(p_ub, T_CARRAY_FLD, 0, BFLD_CHAR, 0)), NULL);

    assert_not_equal((c8=(char *)CBgetalloc(p_ub, T_PTR_FLD, 0, BFLD_CHAR, 0)), NULL);
    assert_equal((c9=(char *)CBgetalloc(p_ub, T_UBF_FLD, 0, BFLD_CHAR, 0)), NULL);
    assert_equal(Berror, BEBADOP);
    assert_equal((c10=(char *)CBgetalloc(p_ub, T_VIEW_FLD, 0, BFLD_CHAR, 0)), NULL);
    assert_equal(Berror, BEBADOP);

    assert_equal(*c1, 'X');
    assert_equal(*c2, 3); /* may be incorrect due to data size*/
    assert_equal(*c3, 'c');
    assert_equal(*c4, 17);
    assert_equal(*c5, 24); /* May be incorrect dute to data size*/
    assert_equal(*c6, 'T');
    assert_equal(*c7, 'C');
    assert_equal(*c8, 40); /* mod 256 */

    free(c1);free(c2);free(c3);free(c4);free(c5);free(c6);free(c7);free(c8);

    /* Test as float */
    assert_not_equal((f1=(float *)CBgetalloc(p_ub, T_SHORT_FLD, 0, BFLD_FLOAT, 0)), NULL);
    assert_not_equal((f2=(float *)CBgetalloc(p_ub, T_LONG_FLD, 0, BFLD_FLOAT, 0)), NULL);
    assert_not_equal((f3=(float *)CBgetalloc(p_ub, T_CHAR_FLD, 0, BFLD_FLOAT, 0)), NULL);
    assert_not_equal((f4=(float *)CBgetalloc(p_ub, T_FLOAT_FLD, 0, BFLD_FLOAT, 0)), NULL);
    assert_not_equal((f5=(float *)CBgetalloc(p_ub, T_DOUBLE_FLD, 0, BFLD_FLOAT, 0)), NULL);
    assert_not_equal((f6=(float *)CBgetalloc(p_ub, T_STRING_FLD, 0, BFLD_FLOAT, 0)), NULL);
    assert_not_equal((f7=(float *)CBgetalloc(p_ub, T_CARRAY_FLD, 0, BFLD_FLOAT, 0)), NULL);

    assert_not_equal((f8=(float *)CBgetalloc(p_ub, T_PTR_FLD, 0, BFLD_FLOAT, 0)), NULL);
    assert_equal((f9=(float *)CBgetalloc(p_ub, T_UBF_FLD, 0, BFLD_FLOAT, 0)), NULL);
    assert_equal(Berror, BEBADOP);
    assert_equal((f10=(float *)CBgetalloc(p_ub, T_VIEW_FLD, 0, BFLD_FLOAT, 0)), NULL);
    assert_equal(Berror, BEBADOP);
    
    assert_double_equal(*f1, 88);
    assert_double_equal(*f2, -1021);
    assert_double_equal(*f3, 99);
    assert_double_equal(*f4, 17.31);
    assert_double_equal(*f5, 12312.1111);
    assert_double_equal(*f6, 0);
    assert_double_equal(*f7, 0);
    assert_double_equal(*f8, 9000);

    free(f1);free(f2);free(f3);free(f4);free(f5);free(f6);free(f7);free(f8);

    assert_not_equal((d1=(double *)CBgetalloc(p_ub, T_SHORT_FLD, 0, BFLD_DOUBLE, 0)), NULL);
    assert_not_equal((d2=(double *)CBgetalloc(p_ub, T_LONG_FLD, 0, BFLD_DOUBLE, 0)), NULL);
    assert_not_equal((d3=(double *)CBgetalloc(p_ub, T_CHAR_FLD, 0, BFLD_DOUBLE, 0)), NULL);
    assert_not_equal((d4=(double *)CBgetalloc(p_ub, T_FLOAT_FLD, 0, BFLD_DOUBLE, 0)), NULL);
    assert_not_equal((d5=(double *)CBgetalloc(p_ub, T_DOUBLE_FLD, 0, BFLD_DOUBLE, 0)), NULL);
    assert_not_equal((d6=(double *)CBgetalloc(p_ub, T_STRING_FLD, 0, BFLD_DOUBLE, 0)), NULL);
    assert_not_equal((d7=(double *)CBgetalloc(p_ub, T_CARRAY_FLD, 0, BFLD_DOUBLE, 0)), NULL);

    assert_not_equal((d8=(double *)CBgetalloc(p_ub, T_PTR_FLD, 0, BFLD_DOUBLE, 0)), NULL);
    assert_equal((d9=(double *)CBgetalloc(p_ub, T_UBF_FLD, 0, BFLD_DOUBLE, 0)), NULL);
    assert_equal(Berror, BEBADOP);
    assert_equal((d10=(double *)CBgetalloc(p_ub, T_VIEW_FLD, 0, BFLD_DOUBLE, 0)), NULL);
    assert_equal(Berror, BEBADOP);
    
    /* Test as double */
    assert_double_equal(*d1, 88);
    assert_double_equal(*d2, -1021);
    assert_double_equal(*d3, 99);
    assert_double_equal(*d4, 17.31);
    assert_double_equal(*d5, 12312.1111);
    assert_double_equal(*d6, 0);
    assert_double_equal(*d7, 0);
    assert_double_equal(*d8, 9000);

    free(d1);free(d2);free(d3);free(d4);free(d5);free(d6);free(d7);free(d8);

    /* Test as string */
    assert_not_equal((str1=CBgetalloc(p_ub, T_SHORT_FLD, 0, BFLD_STRING, 0)), NULL);
    assert_not_equal((str2=CBgetalloc(p_ub, T_LONG_FLD, 0, BFLD_STRING, 0)), NULL);
    assert_not_equal((str3=CBgetalloc(p_ub, T_CHAR_FLD, 0, BFLD_STRING, 0)), NULL);
    assert_not_equal((str4=CBgetalloc(p_ub, T_FLOAT_FLD, 0, BFLD_STRING, 0)), NULL);
    assert_not_equal((str5=CBgetalloc(p_ub, T_DOUBLE_FLD, 0, BFLD_STRING, 0)), NULL);
    assert_not_equal((str6=CBgetalloc(p_ub, T_STRING_FLD, 0, BFLD_STRING, 0)), NULL);
    assert_not_equal((str7=CBgetalloc(p_ub, T_CARRAY_FLD, 0, BFLD_STRING, 0)), NULL);
    
    assert_not_equal((str8=CBgetalloc(p_ub, T_PTR_FLD, 0, BFLD_STRING, 0)), NULL);
    assert_equal((str9=CBgetalloc(p_ub, T_UBF_FLD, 0, BFLD_STRING, 0)), NULL);
    assert_equal(Berror, BEBADOP);
    assert_equal((str10=CBgetalloc(p_ub, T_VIEW_FLD, 0, BFLD_STRING, 0)), NULL);
    assert_equal(Berror, BEBADOP);

    assert_string_equal(str1, "88");
    assert_string_equal(str2, "-1021");
    assert_string_equal(str3, "c");
    assert_equal(strncmp(str4, "17.3", 4), 0);
    assert_equal(strncmp(str5, "12312.1111", 10), 0);
    assert_string_equal(str6, "TEST STR VAL");
    assert_string_equal(str7, "CARRAY1 TEST STRING DATA");
    assert_string_equal(str8, "0x2328");

    free(str1);free(str2);free(str3);free(str4);free(str5);free(str6);free(str7);free(str8);

    /* Test as carray */
    assert_not_equal((carr1=CBgetalloc(p_ub, T_SHORT_FLD, 0, BFLD_CARRAY, 0)), NULL);
    assert_not_equal((carr2=CBgetalloc(p_ub, T_LONG_FLD, 0, BFLD_CARRAY, 0)), NULL);
    assert_not_equal((carr3=CBgetalloc(p_ub, T_CHAR_FLD, 0, BFLD_CARRAY, 0)), NULL);
    assert_not_equal((carr4=CBgetalloc(p_ub, T_FLOAT_FLD, 0, BFLD_CARRAY, 0)), NULL);
    assert_not_equal((carr5=CBgetalloc(p_ub, T_DOUBLE_FLD, 0, BFLD_CARRAY, 0)), NULL);
    assert_not_equal((carr6=CBgetalloc(p_ub, T_STRING_FLD, 0, BFLD_CARRAY, 0)), NULL);
    assert_not_equal((carr7=CBgetalloc(p_ub, T_CARRAY_FLD, 0, BFLD_CARRAY, 0)), NULL);
    
    assert_not_equal((carr8=CBgetalloc(p_ub, T_PTR_FLD, 0, BFLD_CARRAY, 0)), NULL);
    assert_equal((carr9=CBgetalloc(p_ub, T_UBF_FLD, 0, BFLD_CARRAY, 0)), NULL);
    assert_equal(Berror, BEBADOP);
    assert_equal((carr10=CBgetalloc(p_ub, T_VIEW_FLD, 0, BFLD_CARRAY, 0)), NULL);
    assert_equal(Berror, BEBADOP);

    assert_equal(strncmp(carr1, "88", 2), 0);
    assert_equal(strncmp(carr2, "-1021", 5), 0);
    assert_equal(strncmp(carr3, "c", 1), 0);
    assert_equal(strncmp(carr4, "17.3", 4), 0);
    assert_equal(strncmp(carr5, "12312.1111", 10), 0);
    assert_equal(strncmp(carr6, "TEST STR VAL", 12), 0);
    assert_equal(strncmp(carr7, "CARRAY1 TEST STRING DATA", 24), 0);
    assert_equal(strncmp(carr8, "0x2328", 6), 0);

    free(carr1);free(carr2);free(carr3);free(carr4);free(carr5);free(carr6);free(carr7);free(carr8);
    
    /* Check ptr */
    
    assert_not_equal((p1=(ndrx_longptr_t *)CBgetalloc(p_ub, T_SHORT_FLD, 0, BFLD_PTR, 0)), NULL);
    assert_not_equal((p2=(ndrx_longptr_t *)CBgetalloc(p_ub, T_LONG_FLD, 0, BFLD_PTR, 0)), NULL);
    assert_not_equal((p3=(ndrx_longptr_t *)CBgetalloc(p_ub, T_CHAR_FLD, 0, BFLD_PTR, 0)), NULL);
    assert_not_equal((p4=(ndrx_longptr_t *)CBgetalloc(p_ub, T_FLOAT_FLD, 0, BFLD_PTR, 0)), NULL);
    assert_not_equal((p5=(ndrx_longptr_t *)CBgetalloc(p_ub, T_DOUBLE_FLD, 0, BFLD_PTR, 0)), NULL);
    
    /* these will not parse: */
    assert_not_equal((p6=(ndrx_longptr_t *)CBgetalloc(p_ub, T_STRING_FLD, 0, BFLD_PTR, 0)), NULL);
    assert_not_equal((p7=(ndrx_longptr_t *)CBgetalloc(p_ub, T_CARRAY_FLD, 0, BFLD_PTR, 0)), NULL);

    assert_not_equal((p8=(ndrx_longptr_t *)CBgetalloc(p_ub, T_PTR_FLD, 0, BFLD_PTR, 0)), NULL);
    assert_equal((p9=(ndrx_longptr_t *)CBgetalloc(p_ub, T_UBF_FLD, 0, BFLD_PTR, 0)), NULL);
    assert_equal(Berror, BEBADOP);
    assert_equal((p10=(ndrx_longptr_t *)CBgetalloc(p_ub, T_VIEW_FLD, 0, BFLD_PTR, 0)), NULL);
    assert_equal(Berror, BEBADOP);
    
    assert_equal(*p1, 88);
    assert_equal(*p2, -1021);
    assert_equal(*p3, 99);
    assert_equal(*p4, 17);
    assert_equal(*p5, 12312);
    assert_equal(*p6, 0);
    assert_equal(*p7, 0);
    assert_equal(*p8, 9000);

    free(p1);free(p2);free(p3);free(p4);free(p5);free(p6);free(p7);free(p8);
    
    /* Check ubf */
    assert_equal((u1=(UBFH *)CBgetalloc(p_ub, T_UBF_FLD, 0, BFLD_UBF, 0)), NULL);
    assert_equal(Berror, BEBADOP);
    
    /* Check view */
    assert_equal((v1=(BVIEWFLD *)CBgetalloc(p_ub, T_VIEW_FLD, 0, BFLD_VIEW, 0)), NULL);
    assert_equal(Berror, BEBADOP);

    /* Now test the thing that we have different pointers for each of the data type
     * also fields will cross match their types.
     */
    assert_not_equal((s1=(short *)CBgetalloc(p_ub, T_FLOAT_FLD, 0, BFLD_SHORT, 0)), NULL);
    assert_not_equal((l1=(long *)CBgetalloc(p_ub, T_DOUBLE_FLD, 0, BFLD_LONG, 0)), NULL);
    assert_not_equal((c1=(char *)CBgetalloc(p_ub, T_FLOAT_FLD, 0, BFLD_CHAR, 0)), NULL);
    assert_not_equal((f1=(float *)CBgetalloc(p_ub, T_DOUBLE_FLD, 0, BFLD_FLOAT, 0)), NULL);
    assert_not_equal((d1=(double *)CBgetalloc(p_ub, T_FLOAT_FLD, 0, BFLD_DOUBLE, 0)), NULL);
    assert_not_equal((str1=CBgetalloc(p_ub, T_CARRAY_FLD, 0, BFLD_STRING, 0)), NULL);
    len=1024; /* allocate extra buffer also! */
    assert_not_equal((carr1=CBgetalloc(p_ub, T_STRING_FLD, 0, BFLD_CARRAY, &len)), NULL);

    /* Now compare the values */
    assert_equal(*s1, 17);
    assert_equal(*l1, 12312);
    assert_equal(*c1, 17);
    assert_double_equal(*f1, 12312.1111);
    assert_double_equal(*d1, 17.31);
    assert_string_equal(str1, "CARRAY1 TEST STRING DATA");
    assert_equal(len, 12);
    assert_equal(strncmp(carr1, "TEST STR VAL", len), 0);

    /* test that allocated buffer is using extra size! */
    strcpy(carr1, BIG_TEST_STRING);
    assert_equal(strcmp(carr1, BIG_TEST_STRING), 0);

    free(s1);free(l1);free(c1);free(f1);free(d1);free(str1);free(carr1);

    /* Now test the error case, when field is not found? */
    len = 0;
    assert_equal(CBgetalloc(p_ub, T_FLOAT_FLD, 10, BFLD_SHORT, &len), NULL);
    assert_equal(Berror, BNOTPRES);

    /* try to get data from other occurrance */
    assert_double_equal(*(d1=(double *)CBgetalloc(p_ub, T_LONG_FLD, 4, BFLD_DOUBLE, 0)), 888);

    /* Now play with big buffer data */
    assert_equal(Bchg(p_ub, T_CARRAY_2_FLD, 4, BIG_TEST_STRING, strlen(BIG_TEST_STRING)),EXSUCCEED);
    /* now match the string */
    assert_string_equal((str1=CBgetalloc(p_ub, T_CARRAY_2_FLD, 4, BFLD_STRING, 0)), BIG_TEST_STRING);
    free(d1);free(str1);
}


#define BGETALLOC_TEST_17      16384

/**
 * Test Bgetalloc
 */
Ensure(test_bgetalloc)
{
    char fb[BGETALLOC_TEST_17+1024];
    UBFH *p_ub = (UBFH *)fb;
    short *s1;
    long *l1;
    char *c1;
    float *f1;
    double *d1;
    char *str1;
    char *carr1;
    char test17[BGETALLOC_TEST_17]; /* have some random memory data. */
    BFLDLEN len=0;
    ndrx_longptr_t *p1;
    BVIEWFLD *v1;
    UBFH *u1;
    
    randomize_test_data(test17, sizeof(test17));

    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);
    load_get_test_data(p_ub);

    /* Test as short */
    assert_not_equal((s1=(short *)Bgetalloc(p_ub, T_SHORT_FLD, 0, 0)), NULL);
    assert_equal(*s1, 88);
    /* Free up memory */
    free(s1);

    /* Test as long */
    assert_not_equal((l1=(long *)Bgetalloc(p_ub, T_LONG_FLD, 0, 0)), NULL);
    assert_equal(*l1, -1021);

    free(l1);

     /* Test as char */
    assert_not_equal((c1=(char *)Bgetalloc(p_ub, T_CHAR_FLD, 0, 0)), NULL);
    assert_equal(*c1, 'c');

    free(c1);

    /* Test as float */
    assert_not_equal((f1=(float *)Bgetalloc(p_ub, T_FLOAT_FLD, 0, 0)), NULL);

    assert_double_equal(*f1, 17.31);

    free(f1);

    /* Test as double */
    assert_not_equal((d1=(double *)Bgetalloc(p_ub, T_DOUBLE_FLD, 0, 0)), NULL);

    assert_double_equal(*d1, 12312.1111);

    free(d1);

    /* Test as string */
    assert_not_equal((str1=Bgetalloc(p_ub, T_STRING_FLD, 0, 0)), NULL);
    assert_string_equal(str1, "TEST STR VAL");
    free(str1);

    /* Test as carray */
    len = 1000;
    assert_not_equal((carr1=Bgetalloc(p_ub, T_CARRAY_FLD, 0, &len)), NULL);
    assert_equal(len, 24);
    assert_equal(strncmp(carr1, "CARRAY1 TEST STRING DATA", 24), 0);

    strcpy(carr1, BIG_TEST_STRING);
    assert_equal(strcmp(carr1, BIG_TEST_STRING), 0);

    free(carr1);
    
    /* test as ptr */
    len=100;
    assert_not_equal((p1=(ndrx_longptr_t *)Bgetalloc(p_ub, T_PTR_FLD, 0, &len)), NULL);
    gen_test_ptr_val_dbg(__FILE__, __LINE__, *p1,  0, &len);
    free(p1);
    
    /* test as ubf */
    len=100;
    assert_not_equal((u1=(UBFH *)Bgetalloc(p_ub, T_UBF_FLD, 0, &len)), NULL);
    gen_test_ubf_val_dbg(__FILE__, __LINE__, u1,  0, &len);
    
    /* check memory access beyond the data size */
    memset((char *)u1+len, 255, 100);
    free(u1);
    
    /* test as view */
    len=100;
    assert_not_equal((v1=(BVIEWFLD *)Bgetalloc(p_ub, T_VIEW_FLD, 0, &len)), NULL);
    gen_test_view_val_dbg(__FILE__, __LINE__, v1,  0, &len);
    memset((char *)v1+len, 255, 100);
    free(v1);

    /* Test the case when data is not found! */
    assert_equal((str1=Bgetalloc(p_ub, T_STRING_FLD, 20, 0)), NULL);
    assert_equal(Berror, BNOTPRES);
    assert_equal((str1=Bgetalloc(p_ub, T_STRING_FLD, 21, 0)), NULL);
    assert_equal(Berror, BNOTPRES);
    assert_equal((str1=Bgetalloc(p_ub, T_STRING_FLD, 22, 0)), NULL);
    assert_equal(Berror, BNOTPRES);
    assert_equal((str1=Bgetalloc(p_ub, T_STRING_FLD, 23, 0)), NULL);
    assert_equal(Berror, BNOTPRES);
    
    /* Test case for http://www.endurox.org/issues/17 */
    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);
    len = BGETALLOC_TEST_17;
    assert_equal(Bchg(p_ub, T_CARRAY_FLD, 0, test17, len), EXSUCCEED);
    assert_not_equal((carr1=Bgetalloc(p_ub, T_CARRAY_FLD, 0, &len)), NULL);
    assert_equal(len, BGETALLOC_TEST_17);
    assert_equal(memcmp(test17, carr1, BGETALLOC_TEST_17), 0);
    free(carr1);
    
}

/**
 * Test Bgetlast
 */
Ensure(test_bgetlast)
{
    char fb[2048];
    UBFH *p_ub = (UBFH *)fb;
    short s1;
    long l1;
    char c1;
    float f1;
    double d1;
    char str1[64];
    char carr1[64];
    BFLDLEN len;
    BFLDOCC occ;
    ndrx_longptr_t p1;
    struct UBTESTVIEW2 vdata;
    BVIEWFLD v1;
    char tmp[1024];
    UBFH *u1=(UBFH *)tmp;

    assert_equal(Binit(u1, sizeof(tmp)), EXSUCCEED);
    
    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);
    load_get_test_data_2(p_ub);

    /* Test as short */
    occ=-1;
    assert_not_equal(Bgetlast(p_ub, T_SHORT_FLD, &occ, (char *)&s1, 0), EXFAIL);
    assert_equal(s1, 88);
    assert_equal(occ, 0);

    /* Test as long */
    occ=-1;
    assert_not_equal(Bgetlast(p_ub, T_LONG_FLD, &occ, (char *)&l1, 0), EXFAIL);
    assert_equal(l1, -1021);
    assert_equal(occ, 1);

     /* Test as char */
    occ=-1;
    assert_not_equal(Bgetlast(p_ub, T_CHAR_FLD, &occ, (char *)&c1, 0), EXFAIL);
    assert_equal(c1, 'c');
    assert_equal(occ, 2);

    /* Test as float */
    occ=-1;
    assert_not_equal(Bgetlast(p_ub, T_FLOAT_FLD, &occ, (char *)&f1, 0), EXFAIL);
    assert_double_equal(f1, 17.31);
    assert_equal(occ, 3);

    /* Test as double */
    occ=-1;
    assert_not_equal(Bgetlast(p_ub, T_DOUBLE_FLD, &occ, (char *)&d1, 0), EXFAIL);
    assert_double_equal(d1, 12312.1111);
    assert_equal(occ, 4);

    /* Test as string */
    occ=-1;
    assert_not_equal(Bgetlast(p_ub, T_STRING_FLD, &occ, str1, 0), EXFAIL);
    assert_string_equal(str1, "TEST STR VAL");
    assert_equal(occ, 5);

    /* Test as carray */
    len = sizeof(carr1);
    occ=-1;
    assert_not_equal(Bgetlast(p_ub, T_CARRAY_FLD, &occ, carr1, &len), EXFAIL);
    assert_equal(len, 24);
    assert_equal(strncmp(carr1, "CARRAY1 TEST STRING DATA", 24), 0);
    assert_equal(occ, 6);
    
    
    /* test as ptr */
    occ=-1;
    assert_not_equal(Bgetlast(p_ub, T_PTR_FLD, &occ, (char *)&p1, &len), EXFAIL);
    assert_equal(occ, 7);
    gen_test_ptr_val_dbg(__FILE__, __LINE__, p1, 2, &len);
    
    /* test as ubf */
    occ=-1;
    len=sizeof(tmp);
    assert_not_equal(Bgetlast(p_ub, T_UBF_FLD, &occ, (char *)u1, &len), EXFAIL);
    assert_equal(occ, 8);
    gen_test_ubf_val_dbg(__FILE__, __LINE__, u1,  2, &len);
    
    /* test as view */
    v1.data=(char *)&vdata;
    occ=-1;
    len=sizeof(vdata);
    assert_not_equal(Bgetlast(p_ub, T_VIEW_FLD, &occ, (char *)&v1, &len), EXFAIL);
    assert_equal(occ, 9);
    gen_test_view_val_dbg(__FILE__, __LINE__, &v1,  2, &len);
    
    /* Test the case when data is not found! */
    occ=-1;
    assert_equal(Bgetlast(p_ub, T_STRING_2_FLD, &occ, str1, 0), EXFAIL);
    assert_equal(Berror, BNOTPRES);
    assert_equal(occ, -1);
}


/**
 * Test Test cached fields
 */
Ensure(test_cached_flds)
{
    char fb[1024];
    UBFH *p_ub = (UBFH *)fb;
    short s1;
    long l1;

    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);

    s1 = 144;
    
    assert_equal(Bchg(p_ub, T_SHORT_FLD, 0, (char *)&s1, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_SHORT_FLD, 1, (char *)&s1, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_SHORT_FLD, 2, (char *)&s1, 0), EXSUCCEED);
    
    assert_equal(Boccur(p_ub, T_SHORT_FLD), 3);
    assert_equal(Bdel(p_ub, T_SHORT_FLD, 2), EXSUCCEED);
    
    assert_equal(Boccur(p_ub, T_SHORT_FLD), 2);
    assert_equal(Bdel(p_ub, T_SHORT_FLD, 1), EXSUCCEED);
    
    assert_equal(Boccur(p_ub, T_SHORT_FLD), 1);
    assert_equal(Bdel(p_ub, T_SHORT_FLD, 0), EXSUCCEED);
    
    
    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);

    l1 = 144;
    
    assert_equal(Bchg(p_ub, T_LONG_FLD, 0, (char *)&l1, 0), EXSUCCEED);
    assert_equal(Bdel(p_ub, T_LONG_FLD, 0), EXSUCCEED);
    assert_equal(Boccur(p_ub, T_LONG_FLD), 0);
        
}

TestSuite *ubf_get_tests(void)
{
    TestSuite *suite = create_test_suite();

    add_test(suite, test_cbgetalloc);
    add_test(suite, test_bgetalloc);
    add_test(suite, test_bgetlast);
    
    add_test(suite, test_cached_flds);

    return suite;
}
/* vim: set ts=4 sw=4 et smartindent: */
