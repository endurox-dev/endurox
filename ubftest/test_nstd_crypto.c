/**
 * @brief Standard library tests - cryptography
 *
 * @file test_nstd_crypto.c
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
#include <ndebug.h>
#include <excrypto.h>
#include "test.fd.h"
#include "ubfunit1.h"

/**
 * Test string encrypt & decrypt
 */
Ensure(test_crypto_enc_string)
{
    char buf[1024]="";
    char buf2[1024]="";
    int i;
    long len;
#define ENC_TEST_STRING "HELLO STRING _ 123 hello test"
    
    for (i=0; i<100; i++)
    {
        len=sizeof(buf);
        assert_equal(
                ndrx_crypto_enc_string(ENC_TEST_STRING, buf, &len),
                EXSUCCEED);

        assert_equal(strlen(buf)+1, len);
        
        NDRX_LOG(log_debug, "Encrypted string: [%s]", buf);

        len=sizeof(buf2);
        assert_equal(
                ndrx_crypto_dec_string(buf, buf2, &len),
                EXSUCCEED);

        /* decrypted strings must be equal... */
        assert_string_equal(buf2, ENC_TEST_STRING);
        assert_equal(strlen(buf2)+1, len);
    }
}

/**
 * Test substitute functions
 * Bug #268 issues with env substituion after crypto
 */
Ensure(test_crypto_subst_func)
{
    char encdata[PATH_MAX];
    char fmt[PATH_MAX];
    int i;
    long len;
    
#define ENC_SUBST_STRING "Enduro/X"

    assert_equal(setenv("UBFTESTENV", "ENVTEST", EXTRUE), EXSUCCEED);
    assert_equal(setenv("UBFTESTENV2", "ENVTEST2", EXTRUE), EXSUCCEED);

    
    for (i=0; i < 100; i++)
    {
        /* Encrypt "Enduro/X" */

        len=sizeof(encdata);
        assert_equal(ndrx_crypto_enc_string(ENC_SUBST_STRING, encdata, &len), 
                EXSUCCEED);

        snprintf(fmt, sizeof(fmt), "Hello ${dec=%s} ${UBFTESTENV} "
            "${dec=%s} ${UBFTESTENV2} from C", encdata, encdata);

        NDRX_LOG(log_debug, "String with decrypt func: [%s]", fmt);

        ndrx_str_env_subs_len(fmt, sizeof(fmt));

        assert_string_equal(fmt, "Hello Enduro/X ENVTEST Enduro/X ENVTEST2 from C");
    }
}

/**
 * LMDB/EXDB tests
 * @return
 */
TestSuite *ubf_nstd_crypto(void)
{
    TestSuite *suite = create_test_suite();

    /*
    set_setup(suite, basic_setup1);
    set_teardown(suite, basic_teardown1);
    */
    
    add_test(suite, test_crypto_enc_string);
    add_test(suite, test_crypto_subst_func);
            
    return suite;
}
/* vim: set ts=4 sw=4 et smartindent: */
