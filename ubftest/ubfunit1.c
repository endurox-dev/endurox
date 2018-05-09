/* 
**
** @file ubfunit1.c
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

#define DEFAULT_BUFFER  128
UBFH *M_p_ub = NULL;


char M_test_temp_filename[]="/tmp/ubf-test-com-XXXXXX";
FILE *M_test_temp_file=NULL;

/**
 * Open test file for write
 */
void open_test_temp(char *mode)
{
    NDRX_STRCPY_SAFE(M_test_temp_filename, "/tmp/ubf-test-com-XXXXXX");
    assert_not_equal(mkstemp(M_test_temp_filename), EXFAIL);
    assert_not_equal((M_test_temp_file=fopen(M_test_temp_filename, mode)), NULL);
}

/**
 * Open temp file for reading
 */
void open_test_temp_for_read(char *mode)
{

    assert_not_equal((M_test_temp_file=fopen(M_test_temp_filename, mode)), NULL);
}

/**
 * Write to temp file
 * @param data
 */
void write_to_temp(char **data)
{
    int i;
    for (i=0; NULL!=data[i]; i++)
    {
         assert_equal(fwrite (data[i] , 1 , strlen(data[i]) , M_test_temp_file ), strlen(data[i]));
    }
}

/**
 * Close temp file
 */
void close_test_temp(void)
{
    fclose(M_test_temp_file);
}

/**
 * Remove temp file
 */
void remove_test_temp(void)
{
    /* Remove test file */
    assert_equal(unlink(M_test_temp_filename), EXSUCCEED);
}

/**
 * Set up field table environment
 */
void load_field_table(void)
{
    setenv("FLDTBLDIR", "./ubftab", 1);
    setenv("FIELDTBLS", "test.fd,Exfields", 1);
}

/**
 * Fill up buffer with soemthing so that we have more interesting stuff there
 * bigger buffer, etc...
 * @param p_ub
 */
void set_up_dummy_data(UBFH *p_ub)
{
    assert_equal(CBadd(p_ub, T_STRING_9_FLD, "01", 0, BFLD_STRING), EXSUCCEED);
    assert_equal(CBadd(p_ub, T_STRING_9_FLD, "20", 0, BFLD_STRING), EXSUCCEED);
    assert_equal(CBadd(p_ub, T_STRING_9_FLD, "31", 0, BFLD_STRING), EXSUCCEED);

    assert_equal(CBadd(p_ub, T_DOUBLE_3_FLD, "1.11", 0, BFLD_STRING), EXSUCCEED);
    assert_equal(CBadd(p_ub, T_DOUBLE_3_FLD, "2.41231", 0, BFLD_STRING), EXSUCCEED);

    assert_equal(CBadd(p_ub, T_STRING_4_FLD, "HELLO WORLD", 0, BFLD_STRING), EXSUCCEED);
    assert_equal(CBadd(p_ub, T_STRING_8_FLD, "HELLO WORLD1", 0, BFLD_STRING), EXSUCCEED);
    assert_equal(CBadd(p_ub, T_STRING_5_FLD, "HELLO WORLD2", 0, BFLD_STRING), EXSUCCEED);
    assert_equal(CBadd(p_ub, T_STRING_7_FLD, "HELLO WORLD3", 0, BFLD_STRING), EXSUCCEED);
    assert_equal(CBadd(p_ub, T_STRING_6_FLD, "HELLO WORLD4", 0, BFLD_STRING), EXSUCCEED);
    assert_equal(CBadd(p_ub, T_STRING_10_FLD, "HELLO WORLD5", 0, BFLD_STRING), EXSUCCEED);
    assert_equal(CBadd(p_ub, T_STRING_4_FLD, "HELLO WORLD6", 0, BFLD_STRING), EXSUCCEED);
    assert_equal(CBadd(p_ub, T_STRING_3_FLD, "HELLO WORLD7", 0, BFLD_STRING), EXSUCCEED);
    assert_equal(CBchg(p_ub, T_CARRAY_2_FLD, 1, "TEST CARRAY", 0, BFLD_STRING), EXSUCCEED);

}

void do_dummy_data_test(UBFH *p_ub)
{
    char buf[128];
    int len;
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
}

/**
 * Basic preparation before the test
 */
void basic_setup(void)
{
    /*printf("basic_setup\n");*/
    M_p_ub = malloc(DEFAULT_BUFFER);
    memset(M_p_ub, 255, DEFAULT_BUFFER);
    if (EXFAIL==Binit(M_p_ub, DEFAULT_BUFFER))
    {
        fprintf(stderr, "Binit failed!\n");
    }

    load_field_table();
    
}

void basic_teardown(void)
{
    /*printf("basic_teardown\n"); */
    free (M_p_ub);
}

/**
 * Base Binit test
 */
Ensure(test_Binit) {
    char tmpbuf[1024];
    UBFH * p_ub =  (UBFH *) tmpbuf;
    /* Check basic Binit */
    assert_equal(Binit(p_ub, 1024), EXSUCCEED);
    /* Check the size of Binit */
    assert_equal(Bsizeof(p_ub), 1024);
}

/**
 * Basic Field table tests
 */
Ensure(test_fld_table)
{
    assert_equal(strcmp(Bfname(T_STRING_FLD), "T_STRING_FLD"), 0);
    assert_equal(Bfldid("T_STRING_FLD"), T_STRING_FLD);
}

/**
 * NOTE: These are running in 16 bit mode (currently, unless we recompiled the
 * header files for fields.
 *
 * Or replace with generic numbers?
 */
Ensure(test_Bmkfldid)
{
    /* Short */
    assert_equal(Bmkfldid(BFLD_SHORT, 1021), T_SHORT_FLD);
    /* Long */
    assert_equal(Bmkfldid(BFLD_LONG, 1031), T_LONG_FLD);
    /* Char */
    assert_equal(Bmkfldid(BFLD_CHAR, 1011), T_CHAR_FLD);
    /* Float */
    assert_equal(Bmkfldid(BFLD_FLOAT, 1041), T_FLOAT_FLD);
    /* Double */
    assert_equal(Bmkfldid(BFLD_DOUBLE, 1051), T_DOUBLE_FLD);
    /* String */
    assert_equal(Bmkfldid(BFLD_STRING, 1061), T_STRING_FLD);
    /* Carray */
    assert_equal(Bmkfldid(BFLD_CARRAY, 1081), T_CARRAY_FLD);
}

/**
 * Test function the returns field number out of ID
 */
Ensure(test_Bfldno)
{
       /* Short */
    assert_equal(Bfldno(T_SHORT_FLD), 1021);
    /* Long */
    assert_equal(Bfldno(T_LONG_FLD), 1031);
    /* Char */
    assert_equal(Bfldno(T_CHAR_FLD), 1011);
    /* Float */
    assert_equal(Bfldno(T_FLOAT_FLD), 1041);
    /* Double */
    assert_equal(Bfldno(T_DOUBLE_FLD), 1051);
    /* String */
    assert_equal(Bfldno(T_STRING_FLD), 1061);
    /* Carray */
    assert_equal(Bfldno(T_CARRAY_FLD), 1081);
}

Ensure(test_Btype)
{
    assert_string_equal(Btype(T_SHORT_FLD), "short");
    assert_string_equal(Btype(T_LONG_FLD), "long");
    assert_string_equal(Btype(T_CHAR_FLD), "char");
    assert_string_equal(Btype(T_FLOAT_FLD), "float");
    assert_string_equal(Btype(T_DOUBLE_FLD), "double");
    assert_string_equal(Btype(T_STRING_FLD), "string");
    assert_string_equal(Btype(T_CARRAY_FLD), "carray");
    assert_string_equal(Btype(0xfffffff), NULL);
    assert_equal(Berror, BTYPERR);
}

/**
 * Test Bisubf function
 */
Ensure(test_Bisubf)
{
    char tmpbuf[64];
    UBFH * p_ub =  (UBFH *) tmpbuf;
    /* Check basic Binit */
    assert_equal(Binit(p_ub, sizeof(tmpbuf)), EXSUCCEED);
    assert_equal(Bisubf(p_ub), EXTRUE);
    memset(p_ub, 0, sizeof(tmpbuf));
    assert_equal(Bisubf(p_ub), EXFALSE);
    /* Error should not be set */
    assert_equal(Berror, BMINVAL);
}

/**
 * Test Bsizeof
 */
Ensure(test_Bsizeof)
{
    char tmpbuf[64];
    UBFH * p_ub =  (UBFH *) tmpbuf;

    assert_equal(Binit(p_ub, sizeof(tmpbuf)), EXSUCCEED);
    assert_equal(Bsizeof(p_ub), sizeof(tmpbuf));

}

/**
 * Test buffer usage.
 */
Ensure(test_Bunused)
{
    char tmpbuf[64]; /* +2 for short align */
    short s;
    UBFH * p_ub =  (UBFH *) tmpbuf;

    /* Check basic Binit */
    assert_equal(Binit(p_ub, sizeof(tmpbuf)), EXSUCCEED);
    assert_equal(Bunused(p_ub), sizeof(tmpbuf) - sizeof(UBF_header_t) + sizeof(BFLDID));
    /* Add some field and then see what happens */
    assert_equal(Bchg(p_ub, T_SHORT_FLD, 0, (char *)&s, 0), EXSUCCEED);
    assert_equal(Bunused(p_ub), sizeof(tmpbuf) - sizeof(UBF_header_t)-sizeof(s)-2/* align of short */);
    /* fill up to zero */
    assert_equal(Bchg(p_ub, T_STRING_FLD, 0, "abc", 0), EXSUCCEED);
    assert_equal(Bunused(p_ub), sizeof(BFLDID));
}


/**
 * Simple Blen test.
 */
Ensure(test_Blen)
{
    char fb[1024];
    UBFH *p_ub = (UBFH *)fb;

    short s = 88;
    long l = -1021;
    char c = 'c';
    float f = 17.31;
    double d = 12312.1111;
    char carr[] = "CARRAY1 TEST STRING DATA";
    BFLDLEN len = strlen(carr);
    char *str="TEST STR VAL";
    
    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);

    assert_equal(Bchg(p_ub, T_SHORT_FLD, 1, (char *)&s, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_LONG_FLD, 1, (char *)&l, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_CHAR_FLD, 1, (char *)&c, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_FLOAT_FLD, 1, (char *)&f, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_DOUBLE_FLD, 1, (char *)&d, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_STRING_FLD, 1, (char *)str, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_CARRAY_FLD, 1, (char *)carr, len), EXSUCCEED);
    
    
    assert_equal(Blen(p_ub, T_SHORT_FLD, 1), sizeof(s));
    assert_equal(Blen(p_ub, T_LONG_FLD, 1), sizeof(l));
    assert_equal(Blen(p_ub, T_CHAR_FLD, 1), sizeof(c));
    assert_equal(Blen(p_ub, T_FLOAT_FLD, 1), sizeof(f));
    assert_equal(Blen(p_ub, T_DOUBLE_FLD, 1), sizeof(d));
    assert_equal(Blen(p_ub, T_STRING_FLD, 1), strlen(str)+1);
    assert_equal(Blen(p_ub, T_CARRAY_FLD, 1), len);
    
    assert_equal(Blen(p_ub, T_CARRAY_FLD, 2), EXFAIL);
    assert_equal(Berror, BNOTPRES);
}

/**
 * This tests so that buffer terminats with BBADFLDID
 * and then pre-last item is the data.
 */
Ensure(test_buffer_align_fadd)
{
#if 0
    /* Cannot test this directly because of align... */
    char buf[1024];
    UBFH *p_ub = (UBFH *)buf;
    short data=0xffff;
    int *err;
    char *p = buf+1024-sizeof(BFLDID);
    BFLDID *check = (BFLDID *)p;
    short *short_check;
    assert_equal(Binit(p_ub, sizeof(buf)), EXSUCCEED);
    while (EXSUCCEED==Badd(p_ub, T_SHORT_FLD, (char *)&data, 0)){}
    /* check that buffer is full */
    err = ndrx_Bget_Ferror_addr();
    assert_equal(*err, BNOSPACE);
    /* Check last element */
    assert_equal(*check, BBADFLDID);
    /* Pre last must data! */
    p = buf+1024-sizeof(BFLDID)-sizeof(short)-4;
    short_check = (short *)p;
    assert_equal(*short_check, data);
#endif
}

/**
 * Test is not actual anymore - we do not end with BBADFLD
 * - we operation with actual length of the buffer to find the EOF
 * ---------------------------------------------------------------
 * This tests so that buffer terminats with BBADFLDID
 * and then pre-last item is the data.
 * Do test with Bchg
 * Tests Bpres and Boccur
 */
Ensure(test_buffer_align_fchg_and_fpresocc)
{
    char buf[1024];
    UBFH *p_ub = (UBFH *)buf;
    short data=0xffff;
    int *err;
    char *p = buf+1024-sizeof(BFLDID);
    BFLDID *check = (BFLDID *)p;
    short *short_check;
    assert_equal(Binit(p_ub, sizeof(buf)), EXSUCCEED);
    BFLDOCC occ=0;
    int i;

    while (EXSUCCEED==Bchg(p_ub, T_SHORT_FLD, occ, (char *)&data, 0)){occ++;}
    /* check that buffer is full */
    err = ndrx_Bget_Ferror_addr();
    assert_equal(*err, BNOSPACE);
    /* Check last element */
    assert_equal(*check, BBADFLDID);
    /* Pre last must data! */
#if 0
    /* Due to align it is hard to check */
    p = buf+1024-sizeof(BFLDID)-sizeof(short)-4;
    short_check = (short *)p;
    assert_equal(*short_check, data);
#endif
    /* test the Boccur */
    assert_equal(Boccur(p_ub, T_SHORT_FLD), occ);
    /* If not found, then 0 */
    assert_equal(Boccur(p_ub, T_LONG_FLD), 0);

    /* Check that every field is present */
    for (i=0; i<occ; i++)
    {
        assert_equal(Bpres(p_ub, T_SHORT_FLD, i), EXTRUE);
    }
    /* check for non existing, should fail */
    assert_equal(Bpres(p_ub, T_SHORT_FLD, occ), EXFALSE);
}

/**
 * Basically we should test all API functions here which operate with FB!
 * This also seems to be not valid... We do not end with BADFLDID anymore.
 */
Ensure(test_buffer_alignity)
{
    char buf[1024];
    int short_v;
    UBFH *p_ub = (UBFH *)buf;
    BFLDID bfldid=BBADFLDID;
    BFLDOCC occ;
    
    assert_equal(Binit(p_ub, sizeof(buf)), EXSUCCEED);
    memset(buf+sizeof(UBF_header_t)-sizeof(BFLDID), 0xff,
            sizeof(buf)-sizeof(UBF_header_t)+sizeof(BFLDID));

    assert_equal(Bget(p_ub, T_SHORT_FLD, 0, (char *)&short_v, 0), EXFAIL);
    assert_equal(Berror, BALIGNERR);
    assert_equal(Badd(p_ub, T_SHORT_FLD, (char *)&short_v, 0), EXFAIL);
    assert_equal(Berror, BALIGNERR);
    assert_equal(Bchg(p_ub, T_SHORT_FLD, 0, (char *)&short_v, 0), EXFAIL);
    assert_equal(Berror, BALIGNERR);
    assert_equal(Bdel(p_ub, T_SHORT_FLD, 0), EXFAIL);
    assert_equal(Berror, BALIGNERR);
    assert_equal(Bnext(p_ub, &bfldid, &occ, NULL, 0), EXFAIL);
    assert_equal(Berror, BALIGNERR);
}

/**
 * Very basic tests of the framework
 * @return
 */
TestSuite *ubf_basic_tests() {
    TestSuite *suite = create_test_suite();
    
    set_setup(suite, basic_setup);
    set_teardown(suite, basic_teardown);

    /*  UBF init test */
    add_test(suite, test_Binit);
    add_test(suite, test_fld_table);
    add_test(suite, test_Bmkfldid);
    add_test(suite, test_Bfldno);
/* no more for new processing priciples of bytes used.
    add_test(suite, test_buffer_align_fadd);
    add_test(suite, test_buffer_align_fchg_and_fpresocc);
*/
/*
    - not valid any more the trailer might non zero
    add_test(suite, test_buffer_alignity);
*/
    add_test(suite, test_Bisubf);
    add_test(suite, test_Bunused);
    add_test(suite, test_Bsizeof);
    add_test(suite, test_Btype);
    add_test(suite, test_Blen);

    return suite;
}

/*
 * Main test entry.
 */
int main(int argc, char** argv)
{    
    TestSuite *suite = create_test_suite();
    int ret;

    /*
     * NSTD Library tests
     */
    
    add_suite(suite, ubf_nstd_debug());
    add_suite(suite, test_nstd_macros());
    add_suite(suite, ubf_nstd_crypto());
    add_suite(suite, ubf_nstd_base64());
    
    add_suite(suite, ubf_nstd_mtest());
    add_suite(suite, ubf_nstd_mtest2());
    add_suite(suite, ubf_nstd_mtest3());
    add_suite(suite, ubf_nstd_mtest4());
    add_suite(suite, ubf_nstd_mtest5());
    add_suite(suite, ubf_nstd_mtest6_dupcursor());
    
    /*
     * UBF tests
     */
    add_suite(suite, ubf_basic_tests());
    add_suite(suite, ubf_Badd_tests());
    add_suite(suite, ubf_genbuf_tests());
    add_suite(suite, ubf_cfchg_tests());
    add_suite(suite, ubf_cfget_tests());
    add_suite(suite, ubf_fdel_tests());
    add_suite(suite, ubf_expr_tests());
    add_suite(suite, ubf_fnext_tests());
    add_suite(suite, ubf_fproj_tests());
    add_suite(suite, ubf_mem_tests());
    add_suite(suite, ubf_fupdate_tests());
    add_suite(suite, ubf_fconcat_tests());
    add_suite(suite, ubf_find_tests());
    add_suite(suite, ubf_get_tests());
    add_suite(suite, ubf_print_tests());
    add_suite(suite, ubf_macro_tests());
    add_suite(suite, ubf_readwrite_tests());
    add_suite(suite, ubf_mkfldhdr_tests());
    add_suite(suite, ubf_bcmp_tests());

    if (argc > 1)
    {
        ret=run_single_test(suite,argv[1],create_text_reporter());
    }
    else
    {
        ret=run_test_suite(suite, create_text_reporter());
    }

    destroy_test_suite(suite);

    return ret;
    
}
