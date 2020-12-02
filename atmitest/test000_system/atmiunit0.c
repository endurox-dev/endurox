/**
 * @brief ATMI unit tests (functions, etc..)
 *
 * @file atmiunit0.c
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
#include <unistd.h>
#include <cgreen/cgreen.h>
#include <ubf.h>
#include <ndrstandard.h>
#include <string.h>
#include "test.fd.h"
/*#include "ubfunit1.h"*/
#include "ndebug.h"
#include "xatmi.h"
#include <fdatatype.h>


extern TestSuite *atmiunit0_mbuf(void);
extern TestSuite *atmiunit0_exproto(void);

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
 * Test call info buffer processing routines
 * Check different extended modes.
 */
Ensure(test_tpcallinfo)
{
    char *call_buf;
    UBFH *p_meta_ubf;
    UBFH *p_other_buf;
    char tmp[100];
    char tmp2[1024];
    BFLDLEN len;
    assert_equal(tpinit(NULL), EXSUCCEED);
    
    /* try to set call infos buffer */
    call_buf=tpalloc("STRING", NULL, 1024);
    assert_not_equal(call_buf, NULL);
    
    p_meta_ubf=(UBFH *)tpalloc("UBF", NULL, 1024);
    assert_not_equal(p_meta_ubf, NULL);
    
    p_other_buf=(UBFH *)tpalloc("STRING", NULL, 100);
    assert_not_equal(p_other_buf, NULL);
    
    /* set some fields... */
    assert_equal(Bchg(p_meta_ubf, T_STRING_FLD, 0, "HELLO", 0), EXSUCCEED);
    assert_equal(Bchg(p_meta_ubf, T_STRING_FLD, 5, "WORLD", 0), EXSUCCEED);
    
    assert_equal(tpsetcallinfo(call_buf, p_meta_ubf, 0), EXSUCCEED);
    
    /* the output buffer shall be realloc'd to UBF and data filled */
    assert_equal(tpgetcallinfo(call_buf, &p_other_buf, 0), EXSUCCEED);
    
    len=sizeof(tmp);
    assert_equal(Bget(p_other_buf, T_STRING_FLD, 0, tmp, &len), EXSUCCEED);
    assert_string_equal(tmp, "HELLO");
    
    len=sizeof(tmp);
    assert_equal(Bget(p_other_buf, T_STRING_FLD, 5, tmp, &len), EXSUCCEED);
    assert_string_equal(tmp, "WORLD");
    
    /* the same shall work with NUL buffer too */
    tpfree((char *)p_other_buf);
    p_other_buf = NULL;
    
    assert_equal(tpgetcallinfo(call_buf, &p_other_buf, 0), EXSUCCEED);
    
    len=sizeof(tmp);
    assert_equal(Bget(p_other_buf, T_STRING_FLD, 0, tmp, &len), EXSUCCEED);
    assert_string_equal(tmp, "HELLO");
    
    len=sizeof(tmp);
    assert_equal(Bget(p_other_buf, T_STRING_FLD, 5, tmp, &len), EXSUCCEED);
    assert_string_equal(tmp, "WORLD");
    
    /* free up */
    tpfree((char *)p_other_buf);
    
    /* now normal case with UBF... */
    assert_equal(Binit(p_meta_ubf, Bsizeof(p_meta_ubf)), EXSUCCEED);
    len=sizeof(tmp);
    assert_equal(Bget(p_meta_ubf, T_STRING_FLD, 0, tmp, &len), EXFAIL);
    
    assert_equal(tpgetcallinfo(call_buf, &p_meta_ubf, 0), EXSUCCEED);
    
    len=sizeof(tmp);
    assert_equal(Bget(p_meta_ubf, T_STRING_FLD, 0, tmp, &len), EXSUCCEED);
    assert_string_equal(tmp, "HELLO");
    
    len=sizeof(tmp);
    assert_equal(Bget(p_meta_ubf, T_STRING_FLD, 5, tmp, &len), EXSUCCEED);
    assert_string_equal(tmp, "WORLD");
    
    
    /* check custom set buffer */
    
    p_other_buf=(UBFH *)tmp2;
    Binit(p_other_buf, sizeof(tmp2));
    assert_equal(Bchg(p_other_buf, T_STRING_10_FLD, 0, "10 test", 0), EXSUCCEED);
    
    assert_equal(tpsetcallinfo(call_buf, p_other_buf, 0), EXSUCCEED);
    
    Binit(p_other_buf, sizeof(tmp2));
    
    /* this shall fail -> unknown buffer */
    assert_equal(tpgetcallinfo(call_buf, &p_other_buf, 0), EXFAIL);
    assert_equal(tperrno, TPEINVAL);
    
    /* try to get to null / new */
    p_other_buf=NULL;
    assert_equal(tpgetcallinfo(call_buf, &p_other_buf, 0), EXSUCCEED);
    assert_equal(Bget(p_other_buf, T_STRING_10_FLD, 0, tmp, 0), EXSUCCEED);
    assert_string_equal(tmp, "10 test");
    
    /* free up other buf */
    tpfree((char *)p_other_buf);
    
    
    NDRX_LOG(log_info, "Testing errors...");
    
    assert_equal(tpsetcallinfo(NULL, p_meta_ubf, 0), EXFAIL);
    assert_equal(tperrno, TPEINVAL);
    
    assert_equal(tpgetcallinfo(NULL, &p_meta_ubf, 0), EXFAIL);
    assert_equal(tperrno, TPEINVAL);
    
    /* flags no 0 */
    assert_equal(tpsetcallinfo(call_buf, p_meta_ubf, 1), EXFAIL);
    assert_equal(tperrno, TPEINVAL);
    
    assert_equal(tpgetcallinfo(call_buf, &p_meta_ubf, 1), EXFAIL);
    assert_equal(tperrno, TPEINVAL);
    
    /* check output buffer is NULL */
    assert_equal(tpgetcallinfo(call_buf, NULL, 0), EXFAIL);
    assert_equal(tperrno, TPEINVAL);
    
    assert_equal(tpsetcallinfo(call_buf, NULL, 0), EXFAIL);
    assert_equal(tperrno, TPEINVAL);
    
    tpfree(call_buf);
    tpfree((char *)p_meta_ubf);
    
    assert_equal(tpterm(), EXSUCCEED);
}

/**
 * Base ATMI tests
 * @return
 */
TestSuite *atmiunit0_base(void)
{
    TestSuite *suite = create_test_suite();

    add_test(suite, test_tpcallinfo);
    
    return suite;
}

/*
 * Main test entry.
 */
int main(int argc, char** argv)
{    
    TestSuite *suite = create_test_suite();
    int ret;

    add_suite(suite, atmiunit0_exproto());
    add_suite(suite, atmiunit0_base());
    add_suite(suite, atmiunit0_mbuf());

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

/* vim: set ts=4 sw=4 et smartindent: */
