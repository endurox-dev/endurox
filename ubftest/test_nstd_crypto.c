/* 
** Standard library tests - cryptography
**
** @file test_nstd_crypto.c
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
    
#define ENC_TEST_STRING "HELLO STRING _ 123 hello test"
    
    for (i=0; i<100; i++)
    {
        assert_equal(
                ndrx_crypto_enc_string(ENC_TEST_STRING, buf, sizeof(buf)),
                EXSUCCEED);

        NDRX_LOG(log_debug, "Encrypted string: [%s]", buf);

        assert_equal(
                ndrx_crypto_dec_string(buf, buf2, sizeof(buf2)),
                EXSUCCEED);

        /* decrypted strings must be equal... */
        assert_string_equal(buf2, ENC_TEST_STRING);
    }
}

/**
 * Test substitute functions
 */
Ensure(test_crypto_subst_func)
{
    char encdata[PATH_MAX];
    char fmt[PATH_MAX];
    int i;
    
#define ENC_SUBST_STRING "Enduro/X"
    
    for (i=0; i < 100; i++)
    {
        /* Encrypt "Enduro/X" */

        assert_equal(ndrx_crypto_enc_string(ENC_SUBST_STRING, encdata, sizeof(encdata)), 
                EXSUCCEED);

        snprintf(fmt, sizeof(fmt), "Hello ${dec=%s} from C", encdata);

        NDRX_LOG(log_debug, "String with decrypt func: [%s]", fmt);

        ndrx_str_env_subs_len(fmt, sizeof(fmt));

        assert_string_equal(fmt, "Hello Enduro/X from C");
    }
}

/**
 * Common suite entry
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
