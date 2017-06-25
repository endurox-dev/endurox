/*
** Test expression compiler & evaluator
**
** @file test_expr.c
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
#include <cgreen/cgreen.h>
#include <string.h>
#include <sys/resource.h>

#include <ubf.h>
#include <ndrstandard.h>
#include "test.fd.h"
#include "ubfunit1.h"
#include <ndebug.h>


void delete_fb_test_data(UBFH *p_ub)
{
    
    assert_equal(Bdel(p_ub, T_SHORT_FLD, 0), EXSUCCEED);
    assert_equal(Bdel(p_ub, T_LONG_FLD, 0), EXSUCCEED);
    assert_equal(Bdel(p_ub, T_CHAR_FLD, 0), EXSUCCEED);
    assert_equal(Bdel(p_ub, T_FLOAT_FLD, 0), EXSUCCEED);
    assert_equal(Bdel(p_ub, T_DOUBLE_FLD, 0), EXSUCCEED);
    assert_equal(Bdel(p_ub, T_STRING_FLD, 0), EXSUCCEED);
    assert_equal(Bdel(p_ub, T_CARRAY_FLD, 0), EXSUCCEED);

}

void load_expr_test_data_1(UBFH *p_ub)
{
    short s = 102;
    long l = 10212312;
    char c = 'a';
    float f = 127.001;
    double d = 12312312.1112;
    char carr[] = "CARRAY TEST";
    BFLDLEN len = strlen(carr);

    assert_equal(Bchg(p_ub, T_SHORT_FLD, 0, (char *)&s, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_LONG_FLD, 0, (char *)&l, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_CHAR_FLD, 0, (char *)&c, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_FLOAT_FLD, 0, (char *)&f, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_DOUBLE_FLD, 0, (char *)&d, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_STRING_FLD, 0, (char *)"TEST STR VAL", 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_CARRAY_FLD, 0, (char *)carr, len), EXSUCCEED);

    /* Make second copy of field data (another for not equal test)*/
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
}

void load_expr_test_data_2(UBFH *p_ub)
{
    short s = -102;
    long l = -10212312;
    char c = 't';
    float f = -127.001;
    double d = -12312312.1112;
    char carr[] = "ZARRAY TEST";
    BFLDLEN len = strlen(carr);

    assert_equal(Bchg(p_ub, T_SHORT_FLD, 0, (char *)&s, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_LONG_FLD, 0, (char *)&l, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_CHAR_FLD, 0, (char *)&c, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_FLOAT_FLD, 0, (char *)&f, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_DOUBLE_FLD, 0, (char *)&d, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_STRING_FLD, 0, (char *)"TEST STR VAL", 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_CARRAY_FLD, 0, (char *)carr, len), EXSUCCEED);

    /* Make second copy of field data (another for not equal test)*/
    s = -212;
    l = -212;
    c = 'c';
    f = -12127;
    d = -1231232.1;
    carr[0] = 'Z';
    assert_equal(Bchg(p_ub, T_SHORT_2_FLD, 0, (char *)&s, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_LONG_2_FLD, 0, (char *)&l, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_CHAR_2_FLD, 0, (char *)&c, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_FLOAT_2_FLD, 0, (char *)&f, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_DOUBLE_2_FLD, 0, (char *)&d, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_STRING_2_FLD, 0, (char *)"XTEST STR VAL", 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_CARRAY_2_FLD, 0, (char *)carr, len), EXSUCCEED);
}

Ensure(test_expr_basic)
{
    char buf[640];
    UBFH *p_ub = (UBFH *)buf;
    char *tree = NULL;
    /*----------------------------------------------------------*/
    /* init */
    assert_equal(Binit(p_ub, sizeof(buf)), EXSUCCEED);
    /* Test OK */
    tree=Bboolco ("2+2*4==10 && 'abc' %% '.bc'");
    assert_not_equal(tree, NULL);
    assert_equal(Bboolev(NULL, tree), EXTRUE);
    Btreefree(tree);
    /*----------------------------------------------------------*/
    /* We have failure somehere! */
    tree=Bboolco ("100+1=='101' || 1==1 || 2==2 || 1 || #");
    B_error("Bboolco");
    assert_equal(tree, NULL);
    assert_equal(Berror, BSYNTAX);
    /* Print the error message */
    Btreefree(tree);
    /*----------------------------------------------------------*/
    {
    char buf[2048];
    strcpy(buf, getenv("FIELDTBLS"));
    unsetenv("FIELDTBLS");
    /* Field table not initialized - ERROR! */
    tree=Bboolco ("100+1=='101' || 1==1 || 2==2 || 1 || T_STRING_2_FLD %% '58.*'");
    /* Print the error message */
    B_error("Bboolco");
    assert_equal(tree, NULL);
    assert_equal(Berror, BFTOPEN);
    Btreefree(tree);
    setenv("FIELDTBLS", buf, 1);
    }
    /*----------------------------------------------------------*/
    /* Do some real test, load the environment vairables */
    load_field_table(); /* set field table environment variable */
    /* Insert data */
    assert_equal(CBadd(p_ub, T_STRING_2_FLD, "0123456789012345", 0, BFLD_STRING), EXSUCCEED);
    assert_equal(CBadd(p_ub, T_CHAR_FLD, "2", 0, BFLD_STRING), EXSUCCEED);
    tree=Bboolco ("T_STRING_2_FLD=='0123456789012345' && T_CHAR_FLD==2");
    assert_not_equal(tree, NULL);
    assert_equal(Bboolev(p_ub, tree), EXTRUE);
    Btreefree(tree);
    /*----------------------------------------------------------*/
    /* Reporduce the error when field is not found. In field tables */
    tree=Bboolco ("T_STRING_2_FLD=='0123456789012345' && T_CHAR_FLD==2 && B_AD_FIELDNAME");
    assert_equal(tree, NULL);
    B_error("Bboolco");
    assert_equal(Berror, BBADNAME);
    Btreefree(tree);
    /*----------------------------------------------------------*/
    /* Test that  '0' is not becoming as 1 in math ops */
    tree=Bboolco ("!'0'+1==2");
    assert_not_equal(tree, NULL);
    assert_equal(Bboolev(p_ub, tree), EXFALSE);
    Btreefree(tree);

}

/**
 * Tests basic logic operations:- &&, ||, ^, !
 */
Ensure(test_expr_basic_logic)
{
    char buf[128];
    UBFH *p_ub = (UBFH *)buf;
    char *tree = NULL;
    double f;
    short s;
    assert_equal(Binit(p_ub, sizeof(buf)), EXSUCCEED);
    load_field_table(); /* set field table environment variable */

    /*----------------------------------------------------------*/
    /* Basic testing of OR */
        f = 127.001;
        assert_equal(Badd(p_ub, T_STRING_FLD, "TEST STR VAL", 0), EXSUCCEED);
        assert_equal(Badd(p_ub, T_DOUBLE_FLD, (char *)&f, 0), EXSUCCEED);
        /* In this case second expression should not be valuated. */
        tree=Bboolco ("T_DOUBLE_FLD==127.001 || T_STRING_FLD=='TEST STR VAL'");
        assert_not_equal(tree, NULL);
        assert_equal(Bboolev(p_ub, tree), EXTRUE);

        /* Now should be true because of second expression  */
        f = 127.002;
        assert_equal(Bchg(p_ub, T_STRING_FLD, 0, "TEST STR VAL", 0), EXSUCCEED);
        assert_equal(Bchg(p_ub, T_DOUBLE_FLD, 0, (char *)&f, 0), EXSUCCEED);
        assert_equal(Bboolev(p_ub, tree), EXTRUE);
        /* should be true */
        f = 127.001;
        assert_equal(Bchg(p_ub, T_STRING_FLD, 0, "XTEST STR VAL", 0), EXSUCCEED);
        assert_equal(Bchg(p_ub, T_DOUBLE_FLD, 0, (char *)&f, 0), EXSUCCEED);
        assert_equal(Bboolev(p_ub, tree), EXTRUE);

        /* Now it should not true */
        f = 127.002;
        assert_equal(Bchg(p_ub, T_STRING_FLD, 0, "XTEST STR VAL", 0), EXSUCCEED);
        assert_equal(Bchg(p_ub, T_DOUBLE_FLD, 0, (char *)&f, 0), EXSUCCEED);
        assert_equal(Bboolev(p_ub, tree), EXFALSE);
        Btreefree(tree);
    /*----------------------------------------------------------*/
    /* Basic AND testing */
        /* this should be true. */
        f = 127.001;
        assert_equal(Bchg(p_ub, T_STRING_FLD, 0, "TEST STR VAL", 0), EXSUCCEED);
        assert_equal(Bchg(p_ub, T_DOUBLE_FLD, 0, (char *)&f, 0), EXSUCCEED);
        tree=Bboolco ("T_DOUBLE_FLD==127.001 && T_STRING_FLD=='TEST STR VAL'");
        assert_not_equal(tree, NULL);
        assert_equal(Bboolev(p_ub, tree), EXTRUE);
        /* this should be false */
        f = 127.002;
        assert_equal(Bchg(p_ub, T_DOUBLE_FLD, 0, (char *)&f, 0), EXSUCCEED);
        assert_equal(Bboolev(p_ub, tree), EXFALSE);
        /* this also should be false */
        f = 127.001;
        assert_equal(Bchg(p_ub, T_DOUBLE_FLD, 0, (char *)&f, 0), EXSUCCEED);
        assert_equal(Bchg(p_ub, T_STRING_FLD, 0, "XTEST STR VAL", 0), EXSUCCEED);
        assert_equal(Bboolev(p_ub, tree), EXFALSE);
        /* Both false - false */
        f = 127.002;
        assert_equal(Bchg(p_ub, T_DOUBLE_FLD, 0, (char *)&f, 0), EXSUCCEED);
        assert_equal(Bchg(p_ub, T_STRING_FLD, 0, "XTEST STR VAL", 0), EXSUCCEED);
        assert_equal(Bboolev(p_ub, tree), EXFALSE);
        Btreefree(tree);
    /*----------------------------------------------------------*/
    /* Basic not (!) testing */
        s = 0; /* In this case if field is present, then value is true! */
        assert_equal(Bchg(p_ub, T_SHORT_FLD, 0, (char *)&f, 0), EXSUCCEED);
        tree=Bboolco ("T_DOUBLE_FLD || !1");
        assert_not_equal(tree, NULL);
        assert_equal(Bboolev(p_ub, tree), EXTRUE);
        /* should be true too */
        s = 1; /* In this case if field is present, then value is true! */
        assert_equal(Bchg(p_ub, T_SHORT_FLD, 0, (char *)&f, 0), EXSUCCEED);
        assert_equal(Bboolev(p_ub, tree), EXTRUE);
        Btreefree(tree);
        /*----------------------------------------------------------*/
        tree=Bboolco ("!1");
        assert_not_equal(tree, NULL);
        assert_equal(Bboolev(p_ub, tree), EXFALSE);
        Btreefree(tree);
        /*----------------------------------------------------------*/
        s = 0;
        assert_equal(Bchg(p_ub, T_SHORT_FLD, 0, (char *)&f, 0), EXSUCCEED);
        tree=Bboolco ("!T_SHORT_FLD || T_SHORT_FLD");
        assert_not_equal(tree, NULL);
        assert_equal(Bboolev(p_ub, tree), EXTRUE);
        Btreefree(tree);
        /*----------------------------------------------------------*/
        tree=Bboolco ("!T_SHORT_FLD || !T_SHORT_FLD");
        assert_not_equal(tree, NULL);
        assert_equal(Bboolev(p_ub, tree), EXFALSE);
        Btreefree(tree);
    /*----------------------------------------------------------*/
     /* Basic not XOR testing */
        tree=Bboolco ("1 ^ 1"); /* false */
        assert_not_equal(tree, NULL);
        assert_equal(Bboolev(p_ub, tree), EXFALSE);
        Btreefree(tree);
        /*----------------------------------------------------------*/
        tree=Bboolco ("0 ^ 1"); /* true */
        assert_not_equal(tree, NULL);
        assert_equal(Bboolev(p_ub, tree), EXTRUE);
        Btreefree(tree);
        /*----------------------------------------------------------*/
        tree=Bboolco ("1 ^ 0"); /* true */
        assert_not_equal(tree, NULL);
        assert_equal(Bboolev(p_ub, tree), EXTRUE);
        Btreefree(tree);
        /*----------------------------------------------------------*/
        tree=Bboolco ("0 ^ 0"); /* true */
        assert_not_equal(tree, NULL);
        assert_equal(Bboolev(p_ub, tree), EXFALSE);
        Btreefree(tree);       
}
/**
 * Tests ==, !=, %%, !%
 */
Ensure(test_expr_basic_equality)
{
    char buf[224];
    UBFH *p_ub = (UBFH *)buf;
    char *tree = NULL;
    assert_equal(Binit(p_ub, sizeof(buf)), EXSUCCEED);
    load_field_table(); /* set field table environment variable */

    /*----------------------------------------------------------*/
    load_expr_test_data_1(p_ub);
    tree=Bboolco ("T_SHORT_FLD==102 && T_LONG_FLD==10212312 && T_CHAR_FLD=='a'&&"
                  "T_FLOAT_FLD-127.001<0.01 && T_FLOAT_2_FLD==12127 && "
                  "T_DOUBLE_FLD==12312312.1112 &&  "
                  "T_STRING_FLD=='TEST STR VAL' && T_CARRAY_FLD=='CARRAY TEST'");
    assert_not_equal(tree, NULL);
    assert_equal(Bboolev(p_ub, tree), EXTRUE);
    load_expr_test_data_2(p_ub);
    assert_equal(Bboolev(p_ub, tree), EXFALSE);
    Btreefree(tree); 
    /*----------------------------------------------------------*/
    load_expr_test_data_1(p_ub);
    tree=Bboolco ("T_SHORT_FLD!=102 || T_LONG_FLD!=10212312 || T_CHAR_FLD!='a'||"
                  "T_FLOAT_FLD-127.001>=0.01 || T_DOUBLE_FLD!=12312312.1112 || "
                  "T_DOUBLE_FLD!=12312312.1112 ||  "
                  "T_STRING_FLD!='TEST STR VAL' ||   T_CARRAY_FLD!='CARRAY TEST'");
    assert_not_equal(tree, NULL);
    assert_equal(Bboolev(p_ub, tree), EXFALSE);
    load_expr_test_data_2(p_ub);
    assert_equal(Bboolev(p_ub, tree), EXTRUE);
    Btreefree(tree);
    /*----------------------------------------------------------*/
        /* Actually here is error, that expression (T_FLOAT_FLD-127.001 %% '0\\.0.*')
         * is done against regex, so it will fail due to syntax error.
         */
        load_expr_test_data_1(p_ub);
        tree=Bboolco ("T_SHORT_FLD %% '1.2' && T_LONG_FLD %% '1021.312' && T_CHAR_FLD %% 'a'&&"
                      "T_FLOAT_FLD-127.001 %% '0\\.0.*' && T_DOUBLE_FLD %% '12312312.1112' &&  "
                      "T_STRING_FLD %% 'TEST STR ...' && T_CARRAY_FLD  %% 'CA..AY TEST'");
        assert_equal(tree, NULL);
        assert_equal(Berror, BSYNTAX);

        tree=Bboolco ("T_SHORT_FLD %% '1.2' && T_LONG_FLD %% '1021.312' && T_CHAR_FLD %% 'a'&&"
                      "T_FLOAT_FLD %% '127.*' && T_DOUBLE_FLD %% '12312312\\.111.*' &&  "
                      "T_STRING_FLD %% 'TEST STR ...' && T_CARRAY_FLD  %% 'CA..AY TEST'");
        assert_not_equal(tree, NULL);
        /* Above stuff should be true. */
        assert_equal(Bboolev(p_ub, tree), EXTRUE);
        load_expr_test_data_2(p_ub);
        /* should be false */
        assert_equal(Bboolev(p_ub, tree), EXFALSE);
        Btreefree(tree);
    /*----------------------------------------------------------*/
        /* Test error reporting when regular expression syntax is failed */
        load_expr_test_data_1(p_ub);
        tree=Bboolco ("T_SHORT_FLD %% '((((())))(()()\\9\12\1\\3\1((('");
        assert_not_equal(tree, NULL);
        assert_not_equal(Berror, BSYNTAX);
        
        assert_equal(Bboolev(p_ub, tree), EXFAIL);
        assert_equal(Berror, BSYNTAX);
        Btreefree(tree);

    /*----------------------------------------------------------*/
       
        tree=Bboolco ("T_SHORT_FLD !% '1.2' || T_LONG_FLD !% '1021.312' || T_CHAR_FLD !% 'a'||"
                      "T_FLOAT_FLD !% '127.*' || T_DOUBLE_FLD !% '12312312\\.111.*' ||  "
                      "T_STRING_FLD !% 'TEST STR ...' || T_CARRAY_FLD  !% 'CA..AY TEST'");
        assert_not_equal(tree, NULL);
        
        /* Above stuff should be false, because we have correct data */
        load_expr_test_data_1(p_ub);
        assert_equal(Bboolev(p_ub, tree), EXFALSE);
        load_expr_test_data_2(p_ub);
        /* should be TRUE, because we have bad data */
        assert_equal(Bboolev(p_ub, tree), EXTRUE);        
        
        Btreefree(tree);
    /*----------------------------------------------------------*/
       
        tree=Bboolco ("(T_STRING_9_FLD %% '79.*') || (T_STRING_10_FLD %% '79.*') || (T_STRING_FLD %% '79.*')");
        assert_not_equal(tree, NULL);
        
        /* Above stuff should be false, because we have correct data */
        load_expr_test_data_1(p_ub);
        assert_equal(Bboolev(p_ub, tree), EXFALSE);
        
        Btreefree(tree);

    /*----------------------------------------------------------*/
        /* test FB-to-FB equality */

        tree=Bboolco ("T_SHORT_FLD==T_SHORT_FLD && T_LONG_FLD==T_LONG_FLD && T_CHAR_FLD==T_CHAR_FLD&&"
                      "T_FLOAT_FLD==T_FLOAT_FLD && T_DOUBLE_FLD==T_DOUBLE_FLD &&  "
                      "T_STRING_FLD==T_STRING_FLD && T_CARRAY_FLD==T_CARRAY_FLD");
        assert_not_equal(tree, NULL);
        /* should be true with no matter of which data */

        load_expr_test_data_1(p_ub);
        assert_equal(Bboolev(p_ub, tree), EXTRUE);
        load_expr_test_data_2(p_ub);
        assert_equal(Bboolev(p_ub, tree), EXTRUE);
        Btreefree(tree);
    /*----------------------------------------------------------*/
        /* test Non existing field compare to 0, should be false */
        tree=Bboolco ("T_STRING_FLD=='0'");
        assert_not_equal(tree, NULL);
        /* should be true with no matter of which data */
        while (Bpres(p_ub, T_STRING_FLD, 0))
        	Bdel(p_ub, T_STRING_FLD, 0);
		
        assert_equal(Bboolev(p_ub, tree), EXFALSE);

        Btreefree(tree);
    /*----------------------------------------------------------*/

}

/**
 * +, -, !, ~
 * Not sure about ~, but it should work simillary.
 */
Ensure(test_expr_basic_unary)
{
    char buf[224];
    UBFH *p_ub = (UBFH *)buf;
    char *tree = NULL;
    assert_equal(Binit(p_ub, sizeof(buf)), EXSUCCEED);
    long test_long;
    load_field_table(); /* set field table environment variable */

    /*----------------------------------------------------------*/
        /* unary - operation test */
        load_expr_test_data_1(p_ub);
        tree=Bboolco ("-T_SHORT_FLD==0-102 && -T_LONG_FLD==0-10212312 && -T_CHAR_FLD==0 &&"
                  "-T_FLOAT_FLD<-126.001 && -T_DOUBLE_FLD==0-12312312.1112 &&  "
                  "-T_STRING_FLD==0 && -T_CARRAY_FLD==0 && -1==0-1 && -1.0==0-1");
        assert_not_equal(tree, NULL);
        assert_equal(Bboolev(p_ub, tree), EXTRUE);
        load_expr_test_data_2(p_ub);
        assert_equal(Bboolev(p_ub, tree), EXFALSE);
        Btreefree(tree);
    /*----------------------------------------------------------*/
        /* unary + operation test */
        load_expr_test_data_1(p_ub);
        tree=Bboolco ("+T_SHORT_FLD==+102 && +T_LONG_FLD==10212312 && +T_CHAR_FLD==0 &&"
                  "+T_FLOAT_FLD>=126.001 && T_FLOAT_2_FLD==12127 && +T_DOUBLE_FLD==+12312312.1112 &&  "
                  "+T_STRING_FLD==0 && +T_CARRAY_FLD==0");
        assert_not_equal(tree, NULL);
        assert_equal(Bboolev(p_ub, tree), EXTRUE);
        load_expr_test_data_2(p_ub);
        assert_equal(Bboolev(p_ub, tree), EXFALSE);
        Btreefree(tree);
    /*----------------------------------------------------------*/
        /* test ! on fields */
        load_expr_test_data_1(p_ub);
        /* Now delete all the stuff from FB? */
        delete_fb_test_data(p_ub);
        tree=Bboolco ("!T_SHORT_FLD && !T_LONG_FLD && !T_CHAR_FLD &&"
                  "!T_FLOAT_FLD && !T_DOUBLE_FLD &&  "
                  "!T_STRING_FLD && !T_CARRAY_FLD && !0 && !(!'0') && !0.0");
        assert_not_equal(tree, NULL);
        assert_equal(Bboolev(p_ub, tree), EXTRUE);
        /* now restore the data - should fail, because data available */
        load_expr_test_data_1(p_ub);
        assert_equal(Bboolev(p_ub, tree), EXFALSE);
        Btreefree(tree);
    /*----------------------------------------------------------*/
        /* test ! on fields */
        load_expr_test_data_1(p_ub);
        tree=Bboolco ("~T_SHORT_FLD==-2 && ~T_LONG_FLD==-2 && ~T_CHAR_FLD==-2 &&"
                  "~T_FLOAT_FLD==-2 && ~T_DOUBLE_FLD==-2 &&  "
                  "~T_STRING_FLD==-2 && ~T_CARRAY_FLD==-2 && ~0==-1 && ~'0'==-2 && ~0.0==-1");
        assert_not_equal(tree, NULL);
        assert_equal(Bboolev(p_ub, tree), EXTRUE);
        /* Test with 0 in LONG field - result should be the same. */
        test_long = 0;
        assert_equal(CBchg(p_ub, T_LONG_FLD, 0, (char *)&test_long, 0, BFLD_LONG), EXSUCCEED);
        assert_equal(Bboolev(p_ub, tree), EXTRUE);
        Btreefree(tree);
}
/**
 * Math operations with: +, -
 */
Ensure(test_expr_basic_additive)
{
    char buf[224];
    UBFH *p_ub = (UBFH *)buf;
    char *tree = NULL;
    assert_equal(Binit(p_ub, sizeof(buf)), EXSUCCEED);
    double d = 2312.2;
    int big_test;
    load_field_table(); /* set field table environment variable */
    /*----------------------------------------------------------*/
        /* test + */

        /* T_SHORT_FLD = 102
         * T_LONG_FLD = 10212312
         * T_CHAR_FLD = 8
         * T_FLOAT_2_FLD = 12127
         * T_DOUBLE_FLD = 2312.2
         * T_STRING_FLD = 144
         * T_CARRAY_FLD= 244
         */
        tree=Bboolco ("100+100==200 && 100.51+100.49==201 && '123'+'123'==246 &&"
                      "T_SHORT_FLD+50==152 && T_SHORT_FLD+50==152.0 && "
                      "T_SHORT_FLD+50.1==152.1 && T_LONG_FLD+1==10212313 &&"
                      "T_LONG_FLD+1.1==10212313+0.1 && T_FLOAT_2_FLD+3==12130 &&"
                      "T_CHAR_FLD+1==9 && T_CHAR_FLD+0.1==8.1 && "
                      "T_CHAR_FLD+'1'==9 && T_CHAR_FLD+'0.1'==8+0.1 &&"
                      "T_CHAR_FLD+'1'=='5'+'4' &&"
                      "T_FLOAT_2_FLD+3.0==12130 && T_FLOAT_2_FLD+'3.0'==12130 && "
                      "T_FLOAT_2_FLD+'3'==12130 && T_FLOAT_2_FLD+'4'==12130.0+1 &&"
                      "T_DOUBLE_FLD+1==2313.2 && T_DOUBLE_FLD+'1'==2313.2 &&"
                      "T_DOUBLE_FLD+1.0=='2313.2'+0.1-0.1 &&"
                      "T_STRING_FLD+1==145 && T_STRING_FLD+0.1==144.1 && "
                      "T_STRING_FLD+'1'==145 && T_STRING_FLD+'0.1'==144+0.1 &&"
                      "T_STRING_FLD+'1'=='100'+'45' &&"
                      "T_CARRAY_FLD+1==245 && T_CARRAY_FLD+0.1==244.1 && "
                      "T_CARRAY_FLD+'1'==245 && T_CARRAY_FLD+'0.1'==244+0.1 &&"
                      "T_CARRAY_FLD+'1'=='200'+'45'");
        assert_not_equal(tree, NULL);
#if UBF_BIG_TEST
        for (big_test=0; big_test<50; big_test++)
        {
#endif
            load_expr_test_data_1(p_ub);
            /* Fix up double */
            assert_equal(Bchg(p_ub, T_DOUBLE_FLD, 0, (char *)&d, 0), EXSUCCEED);
            assert_equal(Bchg(p_ub, T_STRING_FLD, 0, (char *)"144", 0), EXSUCCEED);
            assert_equal(CBchg(p_ub, T_CARRAY_FLD, 0, (char *)"244", 3, BFLD_STRING), EXSUCCEED);
            assert_equal(CBchg(p_ub, T_CHAR_FLD, 0, (char *)"8", 3, BFLD_STRING), EXSUCCEED);
        
            assert_equal(Bboolev(p_ub, tree), EXTRUE);
            load_expr_test_data_2(p_ub);
            assert_equal(Bboolev(p_ub, tree), EXFALSE);
#if UBF_BIG_TEST
        }
#endif
        Btreefree(tree);
   /*----------------------------------------------------------*/
        /* test - */
        load_expr_test_data_1(p_ub);
        /* Fix up double */
        assert_equal(Bchg(p_ub, T_DOUBLE_FLD, 0, (char *)&d, 0), EXSUCCEED);
        assert_equal(Bchg(p_ub, T_STRING_FLD, 0, (char *)"144", 0), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_CARRAY_FLD, 0, (char *)"-244", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_CHAR_FLD, 0, (char *)"8", 3, BFLD_STRING), EXSUCCEED);
        
        /* T_SHORT_FLD = 102
         * T_LONG_FLD = 10212312
         * T_CHAR_FLD = 8
         * T_FLOAT_2_FLD = 12127
         * T_DOUBLE_FLD = 2312.2
         * T_STRING_FLD = 144
         * T_CARRAY_FLD= 244
         */
        tree=Bboolco ("100-100==0 && 100.51-100.49==0.02 && '123'-'126'==-3 && "
                      "50-T_SHORT_FLD==-52 && 50-T_SHORT_FLD==-52.0 && "
                      "T_SHORT_FLD-50.1==51.9 && 1-T_LONG_FLD==-10212311 &&"
                      "-T_LONG_FLD+1.1==-10212310.9 && T_LONG_FLD-1==10212311 &&"
                      "1-T_CHAR_FLD==-7 && T_CHAR_FLD-0.1==7.9 && "
                      "T_CHAR_FLD-'1'==7 && T_CHAR_FLD-'0.1'==8-0.1 &&"
                      "T_CHAR_FLD-'1'=='5'+'5'-3 && T_FLOAT_2_FLD-2==12125 &&"
                      "T_FLOAT_2_FLD-3.0==12124 && -T_FLOAT_2_FLD-'3.0'==-12130 && "
                      "T_FLOAT_2_FLD-'3'==12124 && T_FLOAT_2_FLD-'4'==12123+1-1 &&"
                      "T_DOUBLE_FLD-1==2311.2 && T_DOUBLE_FLD-'1'==2311.2 &&"
                      "T_DOUBLE_FLD-1.2=='2311'+0.1-0.1 &&"
                      "T_STRING_FLD-3==141 && 0.3-T_STRING_FLD==-143.7 && "
                      "T_STRING_FLD-'3'==141 && T_STRING_FLD-'0.3'==143.7 &&"
                      "T_STRING_FLD-'3'=='141'+4-'4' &&"
                      "T_CARRAY_FLD-5==-249 && T_CARRAY_FLD-0.5==-244.5 && "
                      "-T_CARRAY_FLD+'5'==+249 && T_CARRAY_FLD-'0.5'==-244-0.5 &&"
                      "T_CARRAY_FLD-'5'=='-244'-'5'");
        
        assert_equal(Bboolev(p_ub, tree), EXTRUE);
        load_expr_test_data_2(p_ub);
        assert_equal(Bboolev(p_ub, tree), EXFALSE);
        Btreefree(tree);
}

/**
 * Math operations with: *, /, %
 */
Ensure(test_expr_basic_multiplicative)
{
    char buf[224];
    UBFH *p_ub = (UBFH *)buf;
    char *tree = NULL;
    assert_equal(Binit(p_ub, sizeof(buf)), EXSUCCEED);
    double d = 2312.2;
    int big_test;
    load_field_table(); /* set field table environment variable */
    /*----------------------------------------------------------*/
        /* test * */
        /* T_SHORT_FLD = 257
         * T_LONG_FLD = -8941
         * T_CHAR_FLD = 2
         * T_FLOAT_2_FLD = -0.2
         * T_DOUBLE_FLD = 887441
         * T_STRING_FLD = 448.33
         * T_CARRAY_FLD= 55412.11
         */
        assert_equal(CBchg(p_ub, T_SHORT_FLD, 0, (char *)"257", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_LONG_FLD, 0, (char *)"-8941", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_CHAR_FLD, 0, (char *)"2", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_FLOAT_2_FLD, 0, (char *)"-0.2", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_DOUBLE_FLD, 0, (char *)"887441", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_STRING_FLD, 0, (char *)"448.33", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_CARRAY_FLD, 0, (char *)"55412.11", 3, BFLD_STRING), EXSUCCEED);

        tree=Bboolco ("100*100==10000 && 100.51*1.49==149.7599 && '123'*'13'==1599 && -1*-1==1 &&"
                      "1285==T_SHORT_FLD*5 && 1310.7==T_SHORT_FLD*5.1 && "
                      "'1.1'*T_SHORT_FLD==282.7 && T_LONG_FLD*3==-26823 &&"
                      "-T_LONG_FLD*'1.1'==8941*1.1 && T_LONG_FLD*1.1==-8941*1.1 && "
                      "T_FLOAT_2_FLD*23==-4.6 &&"
                      "T_CHAR_FLD*2==4 && T_CHAR_FLD*0.3==0.6 && "
                      "T_CHAR_FLD*'4'==8 && T_CHAR_FLD*'0.3'==2*0.3 &&"
                      "T_CHAR_FLD*'3'==2+'4' &&"
                      "T_FLOAT_2_FLD*3.0==-0.6 && -T_FLOAT_2_FLD*'3.0'==0.6 && "
                      "T_FLOAT_2_FLD*'3'==-0.6 && T_FLOAT_2_FLD*'4'==-0.8 &&"
                      "T_DOUBLE_FLD*3==2662323 && T_DOUBLE_FLD*'2'==1774882 &&"
                      "T_DOUBLE_FLD*1.5=='1331161.5'+0 &&"
                      "T_STRING_FLD*3==1344.99 && T_STRING_FLD*0.3==134.499 && "
                      "T_STRING_FLD*'3'==1344.99 && T_STRING_FLD*'0.7'==313.831 &&"
                      "T_STRING_FLD*'3'==1344 + 0.99 &&"
                      "T_CARRAY_FLD*0.7==38788.477 && T_CARRAY_FLD*0.7==38788.477 && "
                      "T_CARRAY_FLD*'0.7'==38788.477 && 38788.477==T_CARRAY_FLD*'0.7' &&"
                      "T_CARRAY_FLD*'0.7'==('38788.477'/0.7)*0.7 &&"
                      "T_SHORT_FLD %% '257.*'");
        assert_not_equal(tree, NULL);
        assert_equal(Bboolev(p_ub, tree), EXTRUE);
        load_expr_test_data_2(p_ub);
        
        assert_equal(Bboolev(p_ub, tree), EXFALSE);
        Btreefree(tree);
    /*----------------------------------------------------------*/
        /* test / */
        assert_equal(CBchg(p_ub, T_SHORT_FLD, 0, (char *)"252", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_LONG_FLD, 0, (char *)"-2980", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_CHAR_FLD, 0, (char *)"2", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_FLOAT_2_FLD, 0, (char *)"-0.2", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_DOUBLE_FLD, 0, (char *)"42", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_STRING_FLD, 0, (char *)"66", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_CARRAY_FLD, 0, (char *)"80.4", 3, BFLD_STRING), EXSUCCEED);

        tree=Bboolco ("100/100==1 && 100.10/2==50.05 "
                      "&& '122'/'2'==61 "
                      "&& -3/-3==1"
                      "&& T_SHORT_FLD/2==252/2"
                      "&& 63==T_SHORT_FLD/4"
                      "&& '504'/T_SHORT_FLD==2.0 "
                      /* long -2980 */
                      "&& T_LONG_FLD/5==-2980/5"
                      "&& -T_LONG_FLD/-2980==-1"
                      "&& T_LONG_FLD/4.0==-745"
                      /* char: 2 */
                      "&& T_CHAR_FLD/2==1 "
                      "&& T_CHAR_FLD/2==1.0"
                      "&& T_CHAR_FLD/'1'=='2.0'-0"
                      "&& T_CHAR_FLD/'0.2'==10"
                      "&& T_CHAR_FLD/'2'==0+1"
                      /* float -0.2 */
                      "&& T_FLOAT_2_FLD/2.0==-0.1 "
                      "&& '4.0'/-T_FLOAT_2_FLD==20"
                      "&& '0.4'/T_FLOAT_2_FLD==-2 "
                      "&& T_FLOAT_2_FLD/'4'==-0.05"
                      /* double: 42 */
                      "&& T_DOUBLE_FLD/2==21 "
                      "&& T_DOUBLE_FLD/'1'==42"
                      "&& T_DOUBLE_FLD/1.5=='28'+0"
                      /* string: 66 */
                      "&& T_STRING_FLD/3==22 "
                      "&& 220==T_STRING_FLD/0.3"
                      "&& T_STRING_FLD/'3'==22.00"
                      "&& T_STRING_FLD/'0.5'==132"
                      "&& T_STRING_FLD/'3'==21.5 + 0.5"
                      /* carray: 80.4 */
                      "&& T_CARRAY_FLD/0.2==402"
                      "&& T_CARRAY_FLD/0.2==402.00"
                      "&& T_CARRAY_FLD/'2'==40.2"
                      "&& 16.08==T_CARRAY_FLD/'5'"
                      "&& T_CARRAY_FLD/'4'==20.1000"
                );
        assert_not_equal(tree, NULL);
        assert_equal(Bboolev(p_ub, tree), EXTRUE);
        load_expr_test_data_2(p_ub);
        assert_equal(Bboolev(p_ub, tree), EXFALSE);
        Btreefree(tree);

        tree=Bboolco ("100/0==0");
        assert_not_equal(tree, NULL);
        assert_equal(Bboolev(p_ub, tree), EXTRUE);
        Btreefree(tree);

    /*----------------------------------------------------------*/
        /* test % */
        assert_equal(CBchg(p_ub, T_SHORT_FLD, 0, (char *)"252", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_LONG_FLD, 0, (char *)"-2980", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_CHAR_FLD, 0, (char *)"7", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_FLOAT_2_FLD, 0, (char *)"14", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_DOUBLE_FLD, 0, (char *)"42", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_STRING_FLD, 0, (char *)"66", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_CARRAY_FLD, 0, (char *)"80.4", 3, BFLD_STRING), EXSUCCEED);

        tree=Bboolco ("100 % 97==3"
                      "&& 10 % 3==1 "
                      "&& 2.9%'3.0'==2"
                      "&& '8' % '7'==1 "
                      "&& -2 %-3==-2"
                      "&& T_SHORT_FLD%200==52"
                      "&& 52.00==T_SHORT_FLD%200"
                      "&& '504'%T_SHORT_FLD==0"
                      /* long -2980 */
                      "&& T_LONG_FLD %  5 ==-2980%5"
                      "&& T_LONG_FLD % -3000 == -2980"
                      "&& T_LONG_FLD % 1== 0"
                      /* char: 2 */
                      "&& T_CHAR_FLD%2==1 "
                      "&& T_CHAR_FLD%5==2.0"
                      "&& T_CHAR_FLD%'5'=='2.0'-0"
                      "&& T_CHAR_FLD%'0.2'==7" /* special case from original system v8 */
                      "&& T_CHAR_FLD%'2'==0+1"
                      /* float -0.2 */
                      "&& T_FLOAT_2_FLD%2.0==0.0 "
                      "&& '4.0'%-T_FLOAT_2_FLD==4"
                      "&& '0.4'%T_FLOAT_2_FLD==0"
                      "&& T_FLOAT_2_FLD%'4'==2.00"
                      /* double: 42 */
                      "&& T_DOUBLE_FLD%5.5==2"
                      "&& T_DOUBLE_FLD%'1'==0"
                      "&& T_DOUBLE_FLD%1.5=='0'+0"
                      /* string: 66 */
                      "&& T_STRING_FLD%3==0"
                      "&& 66==T_STRING_FLD%0.3"
                      "&& T_STRING_FLD%'3'==0"
                      "&& T_STRING_FLD%'0.5'==66"
                      "&& T_STRING_FLD%'8'==2"
                      /* carray: 80.4 */
                      "&& T_CARRAY_FLD%8==0"
                      "&& T_CARRAY_FLD%4.2==0.00"
                      "&& T_CARRAY_FLD%'7'==3"
                      "&& 8.00==T_CARRAY_FLD%'12'"
                      "&& T_CARRAY_FLD%'100'==80"
                );
        assert_not_equal(tree, NULL);
        assert_equal(Bboolev(p_ub, tree), EXTRUE);
        load_expr_test_data_2(p_ub);
        assert_equal(Bboolev(p_ub, tree), EXFALSE);
        Btreefree(tree);
}

/**
 * Basic floatev support test
 */
Ensure(test_expr_basic_floatev)
{
    char buf[224];
    UBFH *p_ub = (UBFH *)buf;
    char *tree = NULL;
    assert_equal(Binit(p_ub, sizeof(buf)), EXSUCCEED);
    load_field_table(); /* set field table environment variable */
    /*----------------------------------------------------------*/
        /* test % */
        assert_equal(CBchg(p_ub, T_SHORT_FLD, 0, (char *)"252", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_LONG_FLD, 0, (char *)"-2980", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_CHAR_FLD, 0, (char *)"7", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_FLOAT_2_FLD, 0, (char *)"14", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_DOUBLE_FLD, 0, (char *)"42", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_STRING_FLD, 0, (char *)"abc", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_CARRAY_FLD, 0, (char *)"80.4", 3, BFLD_STRING), EXSUCCEED);
        /* Mod */
        tree=Bboolco ("100 % 97");
        assert_not_equal(tree, NULL);
        assert_equal(Bfloatev(p_ub, tree), 3.0);
        Btreefree(tree);
        /* Div */
        tree=Bboolco ("10/3");
        assert_not_equal(tree, NULL);
        assert_true(Bfloatev(p_ub, tree)>1.3);
        Btreefree(tree);
        /* Mul */
        tree=Bboolco ("10*3");
        assert_not_equal(tree, NULL);
        assert_equal(Bfloatev(p_ub, tree), 30);
        Btreefree(tree);
        /* + */
        tree=Bboolco ("0.9+19.1");
        assert_not_equal(tree, NULL);
        assert_equal(Bfloatev(p_ub, tree), 20);
        Btreefree(tree);
        /* - */
        tree=Bboolco ("'9'-'a'"); 
        assert_not_equal(tree, NULL);
        assert_equal(Bfloatev(p_ub, tree), 9);
        Btreefree(tree);

        tree=Bboolco ("'9'-'4'");
        assert_not_equal(tree, NULL);
        assert_equal(Bfloatev(p_ub, tree), 5);
        Btreefree(tree);

        tree=Bboolco ("T_CARRAY_FLD-0.4");
        assert_not_equal(tree, NULL);
        assert_equal(Bfloatev(p_ub, tree), 80);
        Btreefree(tree);
        /* field value */
        tree=Bboolco ("T_CARRAY_FLD");
        assert_not_equal(tree, NULL);
        assert_equal(Bfloatev(p_ub, tree), 80.4);
        Btreefree(tree);
        /* field value, logical */
        tree=Bboolco ("!T_CARRAY_FLD");
        assert_not_equal(tree, NULL);
        assert_equal(Bfloatev(p_ub, tree), 0);
        Btreefree(tree);

        tree=Bboolco ("!(!T_CARRAY_FLD)");
        assert_not_equal(tree, NULL);
        assert_equal(Bfloatev(p_ub, tree), 1);
        Btreefree(tree);

        tree=Bboolco ("T_STRING_FLD");
        assert_not_equal(tree, NULL);
        assert_equal(Bfloatev(p_ub, tree), 0);
        Btreefree(tree);
        
        tree=Bboolco ("T_STRING_FLD=='abc'");
        assert_not_equal(tree, NULL);
        assert_equal(Bfloatev(p_ub, tree), 1);
        Btreefree(tree);

        tree=Bboolco ("T_DOUBLE_FLD");
        assert_not_equal(tree, NULL);
        assert_double_equal(Bfloatev(p_ub, tree), 42);
        Btreefree(tree);

        tree=Bboolco ("T_LONG_FLD");
        assert_not_equal(tree, NULL);
        assert_double_equal(Bfloatev(p_ub, tree), -2980);
        Btreefree(tree);
        
}

/**
 * Relational: < , >, <=, >=, ==, !=
 */
Ensure(test_expr_basic_relational)
{
    char buf[224];
    UBFH *p_ub = (UBFH *)buf;
    char *tree = NULL;
    assert_equal(Binit(p_ub, sizeof(buf)), EXSUCCEED);
    double d = 2312.2;
    int big_test;
    load_field_table(); /* set field table environment variable */
    /*----------------------------------------------------------*/
        /* test < */
        assert_equal(CBchg(p_ub, T_SHORT_FLD, 0, (char *)"252", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_LONG_FLD, 0, (char *)"-2980", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_CHAR_FLD, 0, (char *)"2", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_FLOAT_2_FLD, 0, (char *)"-0.2", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_DOUBLE_FLD, 0, (char *)"42", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_STRING_FLD, 0, (char *)"66", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_CARRAY_FLD, 0, (char *)"80.4", 3, BFLD_STRING), EXSUCCEED);

        tree=Bboolco ("10<100 "
                      "&& 100.10/2<51.05 "
                      "&& '2'<'322' " /* string cmp */
                      "&& -3<3"
                      "&& '1'<'a'"
                      /* short 252 */
                      "&& T_SHORT_FLD<300"
                      "&& T_SHORT_FLD<300.01"
                      "&& T_SHORT_FLD<'252'+1"
                      "&& !(T_SHORT_FLD<50) " /* should be false, but we invert. */
                      /* long -2980 */
                      "&& T_LONG_FLD<'-4000'" /* string comp */
                      "&& 100 < -T_LONG_FLD"
                      "&& T_LONG_FLD<'123'"
                      /* char: 2 */
                      "&& !(T_CHAR_FLD<1) "
                      "&& T_CHAR_FLD<'a'"
                      "&& T_CHAR_FLD<2.5"
                      "&& T_CHAR_FLD<'a'+3"
                      /* float -0.2 */
                      "&& T_FLOAT_2_FLD < 12"
                      "&& -0.2 < -T_FLOAT_2_FLD"
                      "&& !(T_FLOAT_2_FLD < -12.121)"
                      "&& -0.4 < T_FLOAT_2_FLD"
                      /* double: 42 */
                      "&& T_DOUBLE_FLD < 50"
                      "&& 30.1< T_DOUBLE_FLD"
                      "&& T_DOUBLE_FLD < 'abc'" /* string cmp */
                      /* string: 66 */
                      "&& T_STRING_FLD < 77"
                      "&& 40.01 < T_STRING_FLD"
                      "&& T_STRING_FLD < 'abc'"
                      "&& '0667' < T_STRING_FLD" /* string cmp */
                      "&& T_STRING_FLD < 100-40+55.6"
                      /* carray: 80.4 */
                      "&& T_CARRAY_FLD < 100"
                      "&& !(80.45 < T_CARRAY_FLD)"
                      "&& T_CARRAY_FLD < 'abc'"
                      "&& '00008000.40' < T_CARRAY_FLD"
                      "&& T_CARRAY_FLD < 80.4445"
                );
        assert_not_equal(tree, NULL);
        assert_equal(Bboolev(p_ub, tree), EXTRUE);
        load_expr_test_data_2(p_ub);
        assert_equal(Bboolev(p_ub, tree), EXFALSE);
        Btreefree(tree);
        
        /*----------------------------------------------------------*/
        /* test > */
        assert_equal(CBchg(p_ub, T_SHORT_FLD, 0, (char *)"252", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_LONG_FLD, 0, (char *)"-2980", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_CHAR_FLD, 0, (char *)"2", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_FLOAT_2_FLD, 0, (char *)"-0.2", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_DOUBLE_FLD, 0, (char *)"42", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_STRING_FLD, 0, (char *)"66", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_CARRAY_FLD, 0, (char *)"80.4", 3, BFLD_STRING), EXSUCCEED);

        tree=Bboolco ("100>10 "
                      "&& 51.05>100.10/2 "
                      "&& '322'>'2' " /* string cmp */
                      "&& 3>-3"
                      "&& 'a'>'1'"
                      /* short 252 */
                      "&& 300>T_SHORT_FLD"
                      "&& 300.01>T_SHORT_FLD"
                      "&& '252'+1>T_SHORT_FLD"
                      "&& !(50>T_SHORT_FLD) " /* should be false, but we invert. */
                      /* long -2980 */
                      "&& '-4000'>T_LONG_FLD" /* string comp */
                      "&& -T_LONG_FLD>100 "
                      "&& '123'>T_LONG_FLD"
                      /* char: 2 */
                      "&& !(1>T_CHAR_FLD) "
                      "&& 'a'>T_CHAR_FLD"
                      "&& 2.5>T_CHAR_FLD"
                      "&& 'a'+3>T_CHAR_FLD"
                      /* float -0.2 */
                      "&& !(-0.3>T_FLOAT_2_FLD)"
                      "&& -T_FLOAT_2_FLD > -0.2"
                      "&& 12.121 > T_FLOAT_2_FLD"
                      "&& T_FLOAT_2_FLD > -0.4"
                      /* double: 42 */
                      "&& 50 > T_DOUBLE_FLD"
                      "&& !(T_DOUBLE_FLD > 50.1)"
                      "&& 'abc' > T_DOUBLE_FLD" /* string cmp */
                      /* string: 66 */
                      "&& 77 > T_STRING_FLD"
                      "&& !(T_STRING_FLD > 75.01)"
                      "&& 'abc' > T_STRING_FLD"
                      "&& 'T_STRING_FLD > 0667'" /* string cmp */
                      "&& 100-40+55.6 > T_STRING_FLD"
                      /* carray: 80.4 */
                      "&& 100 > T_CARRAY_FLD"
                      "&& !(T_CARRAY_FLD > 80.445)"
                      "&& 'abc' > T_CARRAY_FLD"
                      "&& T_CARRAY_FLD>'00008000.40'"
                      "&& 80.4445>T_CARRAY_FLD"
                );
        assert_not_equal(tree, NULL);
        assert_equal(Bboolev(p_ub, tree), EXTRUE);
        load_expr_test_data_2(p_ub);
        assert_equal(Bboolev(p_ub, tree), EXFALSE);
        Btreefree(tree);


        /*----------------------------------------------------------*/
        /* test <= */
        assert_equal(CBchg(p_ub, T_SHORT_FLD, 0, (char *)"252", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_LONG_FLD, 0, (char *)"-2980", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_CHAR_FLD, 0, (char *)"2", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_FLOAT_2_FLD, 0, (char *)"-0.2", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_DOUBLE_FLD, 0, (char *)"42", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_STRING_FLD, 0, (char *)"66", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_CARRAY_FLD, 0, (char *)"abc", 3, BFLD_STRING), EXSUCCEED);

        tree=Bboolco ("100<=100 "
                      "&& 100.10/2<=51.05 "
                      "&& '321'<='322' " /* string cmp */
                      "&& -3<=3"
                      "&& 'a'<='a'"
                      /* short 252 */
                      "&& T_SHORT_FLD<=252"
                      "&& T_SHORT_FLD<=252.1"
                      "&& T_SHORT_FLD<='251'+1"
                      "&& !(T_SHORT_FLD<=251) " /* should be false, but we invert. */
                      /* long -2980 */
                      "&& T_LONG_FLD<='-2980'" /* string comp */
                      "&& 100 <= -T_LONG_FLD"
                      "&& T_LONG_FLD<='123'"
                      /* char: 2 */
                      "&& !(T_CHAR_FLD<=1) "
                      "&& T_CHAR_FLD<='a'"
                      "&& T_CHAR_FLD<=2.5"
                      "&& T_CHAR_FLD<='a'+2"
                      /* float -0.2 */
                      "&& T_FLOAT_2_FLD <= -0.2"
                      "&& 0.2 <= -T_FLOAT_2_FLD"
                      "&& !(T_FLOAT_2_FLD <= -12.121)"
                      "&& -0.4 <= T_FLOAT_2_FLD"
                      /* double: 42 */
                      "&& T_DOUBLE_FLD <= '42.0000000000000'"
                      "&& !(42.1<= T_DOUBLE_FLD)"
                      "&& T_DOUBLE_FLD <= 'abc'" /* string cmp */
                      /* string: 66 */
                      "&& T_STRING_FLD <= 66"
                      "&& 40.01 <= T_STRING_FLD"
                      "&& !(667 <= T_STRING_FLD)"
                      "&& '0667' <= T_STRING_FLD" /* string cmp */
                      "&& T_STRING_FLD <= 100-40+55.6"
                      /* carray: abc */
                      "&& T_CARRAY_FLD <= 0"
                      "&& !(10 <= T_CARRAY_FLD)"
                      "&& T_CARRAY_FLD <= 'abc'"
                      "&& -0.1 <= T_CARRAY_FLD"
                      "&& T_CARRAY_FLD <= 3"
                );
        assert_not_equal(tree, NULL);
        assert_equal(Bboolev(p_ub, tree), EXTRUE);
        load_expr_test_data_2(p_ub);
        assert_equal(Bboolev(p_ub, tree), EXFALSE);
        Btreefree(tree);
        /*----------------------------------------------------------*/
        /* test >= */
        assert_equal(CBchg(p_ub, T_SHORT_FLD, 0, (char *)"252", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_LONG_FLD, 0, (char *)"-2980", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_CHAR_FLD, 0, (char *)"2", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_FLOAT_2_FLD, 0, (char *)"-0.2", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_DOUBLE_FLD, 0, (char *)"42", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_STRING_FLD, 0, (char *)"66", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_CARRAY_FLD, 0, (char *)"abc", 3, BFLD_STRING), EXSUCCEED);

        tree=Bboolco ("100>=100 "
                      "&& 51.05>=100.10/2 "
                      "&& '322'>='321'" /* string cmp */
                      "&& 3>=-3"
                      "&& 'a'>='a'"
                      /* short 252 */
                      "&& 252>=T_SHORT_FLD"
                      "&& 252.1>=T_SHORT_FLD"
                      "&& '251'+1>=T_SHORT_FLD"
                      "&& !(251>=T_SHORT_FLD) " /* should be false, but we invert. */
                      /* long -2980 */
                      "&& '-2980'>=T_LONG_FLD" /* string comp */
                      "&& -T_LONG_FLD>=100"
                      "&& '123'>=T_LONG_FLD"
                      /* char: 2 */
                      "&& !(1>=T_CHAR_FLD) "
                      "&& 'a'>=T_CHAR_FLD"
                      "&& 2.5>=T_CHAR_FLD"
                      "&& 'a'+2>=T_CHAR_FLD"
                      /* float -0.2 */
                      "&& -0.2>=T_FLOAT_2_FLD"
                      "&& -T_FLOAT_2_FLD>=0.2"
                      "&& !(-12.121>=T_FLOAT_2_FLD)"
                      "&& T_FLOAT_2_FLD>=-0.4"
                      /* double: 42 */
                      "&& '42.0000000000000'>=T_DOUBLE_FLD"
                      "&& !(T_DOUBLE_FLD>=42.1)"
                      "&& 'abc'>=T_DOUBLE_FLD" /* string cmp */
                      /* string: 66 */
                      "&& 66>=T_STRING_FLD"
                      "&& T_STRING_FLD>=40.01"
                      "&& !(T_STRING_FLD>=667)"
                      "&& T_STRING_FLD>='0667'" /* string cmp */
                      "&& 100-40+55.6>=T_STRING_FLD"
                      /* carray: abc */
                      "&& 0>=T_CARRAY_FLD"
                      "&& !(T_CARRAY_FLD>=10)"
                      "&& 'abc'>=T_CARRAY_FLD"
                      "&& T_CARRAY_FLD>=-0.1"
                      "&& 3>=T_CARRAY_FLD"
                );
        assert_not_equal(tree, NULL);
        assert_equal(Bboolev(p_ub, tree), EXTRUE);
        load_expr_test_data_2(p_ub);
        assert_equal(Bboolev(p_ub, tree), EXFALSE);
        Btreefree(tree);
        /*----------------------------------------------------------*/
        /* test == */
        assert_equal(CBchg(p_ub, T_SHORT_FLD, 0, (char *)"12311", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_LONG_FLD, 0, (char *)"558211", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_CHAR_FLD, 0, (char *)"6", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_FLOAT_2_FLD, 0, (char *)"-10", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_DOUBLE_FLD, 0, (char *)"12.5", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_STRING_FLD, 0, (char *)"7721", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_CARRAY_FLD, 0, (char *)"12a", 3, BFLD_STRING), EXSUCCEED);

        tree=Bboolco ("100==100 "
                      "&& !(51.05==100.10/2) "
                      "&& '322'=='322'" /* string cmp */
                      "&& !(3==-3)"
                      "&& 'a'=='a'"
                      "&& 155.12==155.12-0.12+0.12"
                      /* short 12311 */
                      "&& T_SHORT_FLD==T_SHORT_FLD"
                      "&& 12311==T_SHORT_FLD"
                      "&& !(12312==T_SHORT_FLD)"
                      "&& '12311'==T_SHORT_FLD"
                      "&& !(1231==T_SHORT_FLD) "
                      /* long 558211 */
                      "&& '-558211'==-T_LONG_FLD"
                      "&& !(-T_LONG_FLD==100)"
                      "&& 558211==T_LONG_FLD"
                      "&& T_LONG_FLD==T_LONG_FLD"
                      /* char: 6 */
                      "&& T_CHAR_FLD==T_CHAR_FLD"
                      "&& T_CHAR_FLD+T_CHAR_FLD==T_CHAR_FLD*2"
                      "&& !(5==T_CHAR_FLD)"
                      "&& '6'==T_CHAR_FLD"
                      "&& 6.0==T_CHAR_FLD"
                      "&& 6==T_CHAR_FLD"
                      /* float -10 */
                      "&& T_FLOAT_2_FLD==T_FLOAT_2_FLD"
                      "&& -10==T_FLOAT_2_FLD"
                      "&& !(-T_FLOAT_2_FLD==-0.1)"
                      "&& !(-12.121==T_FLOAT_2_FLD)"
                      "&& T_FLOAT_2_FLD=='-10.0000000000000'"
                      /* double: 12.5 */
                      "&& T_DOUBLE_FLD==T_DOUBLE_FLD"
                      "&& '12.5000000000000'==T_DOUBLE_FLD"
                      "&& !(T_DOUBLE_FLD==42.1)"
                      "&& !('abc'==T_DOUBLE_FLD)"
                      /* string: 7721 */
                      "&& T_STRING_FLD==T_STRING_FLD"
                      "&& 7721==T_STRING_FLD"
                      "&& T_STRING_FLD=='7721'"
                      "&& !(T_STRING_FLD==667)"
                      "&& T_STRING_FLD==7721.000"
                      "&& !(0.2+0.2==T_STRING_FLD)"
                      /* carray: 12a */
                      "&& T_CARRAY_FLD==T_CARRAY_FLD"
                      "&& !(T_CARRAY_FLD==10)"
                      "&& '12a'==T_CARRAY_FLD"
                      "&& T_CARRAY_FLD==12.0"
                      "&& 12==T_CARRAY_FLD"
                      "&& 12325.5==T_CARRAY_FLD+T_FLOAT_2_FLD+T_SHORT_FLD+T_DOUBLE_FLD"
                );
        assert_not_equal(tree, NULL);
        assert_equal(Bboolev(p_ub, tree), EXTRUE);
        load_expr_test_data_2(p_ub);
        assert_equal(Bboolev(p_ub, tree), EXFALSE);
        Btreefree(tree);
        

        /* test != */
        assert_equal(CBchg(p_ub, T_SHORT_FLD, 0, (char *)"12311", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_LONG_FLD, 0, (char *)"558211", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_CHAR_FLD, 0, (char *)"6", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_FLOAT_2_FLD, 0, (char *)"-10", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_DOUBLE_FLD, 0, (char *)"12.5", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_STRING_FLD, 0, (char *)"7721", 3, BFLD_STRING), EXSUCCEED);
        assert_equal(CBchg(p_ub, T_CARRAY_FLD, 0, (char *)"12a", 3, BFLD_STRING), EXSUCCEED);

        tree=Bboolco ("!(100!=100) "
                      "&& 51.05!=100.10/2 "
                      "&& !('322'!='322')" /* string cmp */
                      "&& 3!=-3"
                      "&& !('a'!='a')"
                      "&& 155.0!=155.12-0.12+0.12"
                      /* short 12311 */
                      "&& !(T_SHORT_FLD!=T_SHORT_FLD)"
                      "&& !(12311!=T_SHORT_FLD)"
                      "&& 12312!=T_SHORT_FLD"
                      "&& !('12311'!=T_SHORT_FLD)"
                      "&& 1231!=T_SHORT_FLD "
                      /* long 558211 */
                      "&& !('-558211'!=-T_LONG_FLD)"
                      "&& -T_LONG_FLD!=100"
                      "&& !(558211!=T_LONG_FLD)"
                      "&& !(T_LONG_FLD!=T_LONG_FLD)"
                      /* char: 6 */
                      "&& !(T_CHAR_FLD!=T_CHAR_FLD)"
                      "&& !(T_CHAR_FLD+T_CHAR_FLD!=T_CHAR_FLD*2)"
                      "&&  5!=T_CHAR_FLD"
                      "&& !('6'!=T_CHAR_FLD)"
                      "&& !(6.0!=T_CHAR_FLD)"
                      "&& !(6!=T_CHAR_FLD)"
                      /* float -10 */
                      "&& !(T_FLOAT_2_FLD!=T_FLOAT_2_FLD)"
                      "&& !(-10!=T_FLOAT_2_FLD)"
                      "&& -T_FLOAT_2_FLD!=-0.1"
                      "&& -12.121!=T_FLOAT_2_FLD"
                      "&& !(T_FLOAT_2_FLD!='-10.0000000000000')"
                      /* double: 12.5 */
                      "&& !(T_DOUBLE_FLD!=T_DOUBLE_FLD)"
                      "&& !('12.5000000000000'!=T_DOUBLE_FLD)"
                      "&& T_DOUBLE_FLD!=42.1"
                      "&& 'abc'!=T_DOUBLE_FLD"
                      /* string: 7721 */
                      "&& !(T_STRING_FLD!=T_STRING_FLD)"
                      "&& !(7721!=T_STRING_FLD)"
                      "&& !(T_STRING_FLD!='7721')"
                      "&& T_STRING_FLD!=667"
                      "&& !(T_STRING_FLD!=7721.000)"
                      "&& 0.2+0.2!=T_STRING_FLD"
                      /* carray: 12a */
                      "&& !(T_CARRAY_FLD!=T_CARRAY_FLD)"
                      "&& T_CARRAY_FLD!=10"
                      "&& !('12a'!=T_CARRAY_FLD)"
                      "&& !(T_CARRAY_FLD!=12.0)"
                      "&& !(12!=T_CARRAY_FLD)"
                );
        assert_not_equal(tree, NULL);
        assert_equal(Bboolev(p_ub, tree), EXTRUE);
        load_expr_test_data_2(p_ub);
        assert_equal(Bboolev(p_ub, tree), EXFALSE);
        Btreefree(tree);
}

Ensure(test_expr_basic_scopes)
{
    char buf[224];
    UBFH *p_ub = (UBFH *)buf;
    char *tree = NULL;
    assert_equal(Binit(p_ub, sizeof(buf)), EXSUCCEED);
    double d = 2312.2;
    load_field_table(); /* set field table environment variable */
    /*----------------------------------------------------------*/

        tree=Bboolco ("2*(5+6)==22"
                      "&& 190==(10*(5.0+2*(3+4)))");
        assert_not_equal(tree, NULL);
        assert_equal(Bboolev(p_ub, tree), EXTRUE);
        Btreefree(tree);
}
/**
 * Test expressions with manuplating with string field
 */
Ensure(test_expr_short)
{
    char buf[640];
    UBFH *p_ub = (UBFH *)buf;
    char *tree = NULL;
    short short_val;
    assert_equal(Binit(p_ub, sizeof(buf)), EXSUCCEED);
    set_up_dummy_data(p_ub);
    load_field_table(); /* set field table environment variable */

    /*----------------------------------------------------------*/
        /* Compare short and do some math...*/
        short_val = 137;
        assert_equal(Badd(p_ub, T_SHORT_FLD, (char *)&short_val, 0), EXSUCCEED);
        tree=Bboolco ("T_SHORT_FLD==50*2+(30/2)*2+5+2");
        assert_not_equal(tree, NULL);
        assert_equal(Bboolev(p_ub, tree), EXTRUE);
        /*----------------------------------------------------------*/
        short_val = -137;
        assert_equal(Bchg(p_ub, T_SHORT_FLD, 0, (char *)&short_val, 0), EXSUCCEED);
        assert_equal(Bboolev(p_ub, tree), EXFALSE);
        /*----------------------------------------------------------*/
        short_val = 137;
        assert_equal(Bchg(p_ub, T_SHORT_FLD, 0, (char *)&short_val, 0), EXSUCCEED);
        assert_equal(Bboolev(p_ub, tree), EXTRUE);
        /* Free up tree */
        Btreefree(tree);
    /*----------------------------------------------------------*/
        /* Compare field against regular expression */
        short_val = 4112;
        assert_equal(Badd(p_ub, T_SHORT_FLD, (char *)&short_val, 0), EXSUCCEED);
        tree=Bboolco ("T_SHORT_FLD[1] %% '4..2'");
        assert_not_equal(tree, NULL);
        assert_equal(Bboolev(p_ub, tree), EXTRUE);
        /*----------------------------------------------------------*/
        short_val = 4442;
        assert_equal(Bchg(p_ub, T_SHORT_FLD, 1, (char *)&short_val, 0), EXSUCCEED);
        assert_equal(Bboolev(p_ub, tree), EXTRUE);
        /*----------------------------------------------------------*/
        short_val = 1442;
        assert_equal(Bchg(p_ub, T_SHORT_FLD, 1, (char *)&short_val, 0), EXSUCCEED);
        assert_equal(Bboolev(p_ub, tree), EXFALSE);
        
        /* Free up tree */
        Btreefree(tree);
    /*----------------------------------------------------------*/
        /* Compare the field against float & do some math stuff!
         * and do some unarray operation.
         */
        short_val = -4112;
        assert_equal(Bchg(p_ub, T_SHORT_FLD, 5, (char *)&short_val, 0), EXSUCCEED);
        tree=Bboolco ("-T_SHORT_FLD[5] == 4110+0.8+1.2 && 0.0==-1+1");
        assert_not_equal(tree, NULL);
        assert_equal(Bboolev(p_ub, tree), EXTRUE);
        /* Free up tree */
        Btreefree(tree);
    /*----------------------------------------------------------*/
    
}

/**
 * Test tree printing routine
 */
Ensure(test_bboolpr)
{
    char *tree;
    char buf[640];
    UBFH *p_ub = (UBFH *)buf;
    char testbuf[640];
    int read;
    assert_equal(Binit(p_ub, sizeof(buf)), EXSUCCEED);
    /* Simple test... */
    open_test_temp("w");
    assert_not_equal((tree=Bboolco ("2 * ( 4 + 5 ) || 5 && 'abc' %% '..b' && 2/2*4==5")), NULL);
    Bboolpr(tree, M_test_temp_file);
    close_test_temp();
    open_test_temp_for_read("r");
    assert_not_equal((read=fread (testbuf, 1, sizeof(testbuf), M_test_temp_file)),0);
    testbuf[read-1]=EXEOS; /* remove trainline newline */
    close_test_temp();
    remove_test_temp();
    assert_string_equal(testbuf,"((2*(4+5)) || ((5 && ('abc' %% '..b')) && (((2/2)*4) == 5)))");
    
    Btreefree(tree);

    /* test negation */
    
    open_test_temp("w");
    assert_not_equal((tree=Bboolco ("!( 'a'!='b' ) || !( 'c'&&'b' )")), NULL);
    Bboolpr(tree, M_test_temp_file);
    close_test_temp();
    open_test_temp_for_read("r");
    assert_not_equal((read=fread (testbuf, 1, sizeof(testbuf), M_test_temp_file)),0);
    testbuf[read-1]=EXEOS; /* remove trainline newline */
    close_test_temp();
    remove_test_temp();
    assert_string_equal(testbuf, "((!('a' != 'b')) || (!('c' && 'b')))");
    Btreefree(tree);

    /* Some other tests... */
    open_test_temp("w");
    assert_not_equal((tree=Bboolco ("1<1 && 2>1 && 2>=1 && 1<=~2 && 2^1")), NULL);
    Bboolpr(tree, M_test_temp_file);
    close_test_temp();
    open_test_temp_for_read("r");
    assert_not_equal((read=fread (testbuf, 1, sizeof(testbuf), M_test_temp_file)),0);
    testbuf[read-1]=EXEOS; /* remove trainline newline */
    close_test_temp();
    remove_test_temp();
    assert_string_equal(testbuf, "(((((1 < 1) && (2 > 1)) && (2 >= 1)) && (1 <= (~2))) && (2 ^ 1))");
    Btreefree(tree);
}

/* -------------------------------------------------------------------------- */
long callback_function_true(UBFH *p_ub, char *funcname)
{
    assert_string_equal(funcname, "callback_function_true");
    return 1;
}

long callback_function_false(UBFH *p_ub, char *funcname)
{
    assert_string_equal(funcname, "callback_function_false");
    UBF_LOG(log_debug, "callback_function_false() called!");
    return 0;
}

long callback_function_cond(UBFH *p_ub, char *funcname)
{
    long ret;
    short short_val;
    
    assert_string_equal(funcname, "callback_function_cond");
    assert_equal(Bget(p_ub, T_SHORT_FLD, 0, (char *)&short_val, 0L), EXSUCCEED);
    
    if (137 == short_val)
        ret=EXTRUE;
    else
        ret=EXFALSE;
    
    return ret;
}

long callback_function_value(UBFH *p_ub, char *funcname)
{
    assert_string_equal(funcname, "callback_function_value");
    long ret;
    
    ret = 177788;
    
    return ret;
}


/**
 * Test callback functions
 */
Ensure(test_cbfunc)
{
    char buf[640];
    UBFH *p_ub = (UBFH *)buf;
    char *tree = NULL;
    short short_val;
    long long_val;
    assert_equal(Binit(p_ub, sizeof(buf)), EXSUCCEED);
    set_up_dummy_data(p_ub);
    load_field_table(); /* set field table environment variable */
    
    assert_equal(Bboolsetcbf ("callback_function_true", callback_function_true), EXSUCCEED);
    assert_equal(Bboolsetcbf ("callback_function_false", callback_function_false), EXSUCCEED);
    assert_equal(Bboolsetcbf ("callback_function_cond", callback_function_cond), EXSUCCEED);
    assert_equal(Bboolsetcbf ("callback_function_value", callback_function_value), EXSUCCEED);
    


    short_val = 137;
    assert_equal(Badd(p_ub, T_SHORT_FLD, (char *)&short_val, 0), EXSUCCEED);
    long_val = 177788;
    assert_equal(Badd(p_ub, T_LONG_FLD, (char *)&long_val, 0), EXSUCCEED);
    
    /*----------------------------------------------------------*/
        /* This should be true */
        tree=Bboolco ("callback_function_true()");
        assert_not_equal(tree, NULL);
        assert_equal(Bboolev(p_ub, tree), EXTRUE);
        Btreefree(tree);
    /*----------------------------------------------------------*/
        /* This should be false */
        tree=Bboolco ("callback_function_false()");
        assert_not_equal(tree, NULL);
        assert_equal(Bboolev(p_ub, tree), EXFALSE);
        Btreefree(tree);
    /*----------------------------------------------------------*/
        /* This should be true as buffer contains 137 */
        tree=Bboolco ("callback_function_cond()");
        assert_not_equal(tree, NULL);
        assert_equal(Bboolev(p_ub, tree), EXTRUE);
        Btreefree(tree);
    /*----------------------------------------------------------*/
        /* This should be true */
        tree=Bboolco ("callback_function_value()==177788");
        assert_not_equal(tree, NULL);
        assert_equal(Bboolev(p_ub, tree), EXTRUE);
        Btreefree(tree);
    /*----------------------------------------------------------*/
        /* This should be false */
        tree=Bboolco ("callback_function_value()==177787");
        assert_not_equal(tree, NULL);
        assert_equal(Bboolev(p_ub, tree), EXFALSE);
        Btreefree(tree);
    /*----------------------------------------------------------*/
        /* This should be true too as value != 0 */
        tree=Bboolco ("callback_function_value()");
        assert_not_equal(tree, NULL);
        assert_equal(Bboolev(p_ub, tree), EXTRUE);
        Btreefree(tree);
    /*----------------------------------------------------------*/
        /* This should be true, too */
        tree=Bboolco ("callback_function_true()+callback_function_true()==2");
        assert_not_equal(tree, NULL);
        assert_equal(Bboolev(p_ub, tree), EXTRUE);
        Btreefree(tree);
    /*----------------------------------------------------------*/
        /* This should be false */
        tree=Bboolco ("callback_function_true()-callback_function_true()");
        assert_not_equal(tree, NULL);
        assert_equal(Bboolev(p_ub, tree), EXFALSE);
        Btreefree(tree);
    /*----------------------------------------------------------*/
        /* This should be true */
        tree=Bboolco ("callback_function_value()==T_LONG_FLD");
        assert_not_equal(tree, NULL);
        assert_equal(Bboolev(p_ub, tree), EXTRUE);
        Btreefree(tree);
    /*----------------------------------------------------------*/
        /* This should be true */
        tree=Bboolco ("callback_function_value()=='177788'");
        assert_not_equal(tree, NULL);
        assert_equal(Bboolev(p_ub, tree), EXTRUE);
        Btreefree(tree);
    /*----------------------------------------------------------*/
        /* This should not compile */
        tree=Bboolco ("callback_function_none()");
        assert_equal(tree, NULL);
        assert_not_equal(Berror, BBADNAME);
        Btreefree(tree);
    /*----------------------------------------------------------*/
        /* This should not compile - syntax error */
        tree=Bboolco ("callback_function_value() %% '177788'");
        assert_equal(tree, NULL);
        assert_equal(Berror, BSYNTAX);
        Btreefree(tree);
    /*----------------------------------------------------------*/
        /* This should be true */
        tree=Bboolco ("T_SHORT_FLD || callback_function_false()");
        assert_not_equal(tree, NULL);
        assert_equal(Bboolev(p_ub, tree), EXTRUE);
        Btreefree(tree);
    /*----------------------------------------------------------*/
        /* Test unset function */
        assert_equal(Bboolsetcbf ("callback_function_value", NULL), EXSUCCEED);
        tree=Bboolco ("callback_function_value()=='177788'");
        assert_equal(tree, NULL);
        Btreefree(tree);
    /*----------------------------------------------------------*/
        
}
/* -------------------------------------------------------------------------- */

TestSuite *ubf_expr_tests(void)
{
    TestSuite *suite = create_test_suite();

    add_test(suite, test_expr_basic);
    add_test(suite, test_expr_basic_logic);
    add_test(suite, test_expr_basic_equality);
    add_test(suite, test_expr_basic_unary);
    add_test(suite, test_expr_basic_additive);
    add_test(suite, test_expr_basic_multiplicative);
    add_test(suite, test_expr_basic_relational);
    add_test(suite, test_expr_basic_scopes);
    add_test(suite, test_bboolpr);

    /* Test float ev */
    add_test(suite, test_expr_basic_floatev);

    /* Some other tests... */
    add_test(suite, test_expr_short);
    
    /* Function callback tests... */
    add_test(suite, test_cbfunc);

    return suite;
}

