/* 
 * @brief Base64 tests (Bug #261)
 *
 * @file test_nstd_b64.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or Mavimax's license for commercial use.
 * -----------------------------------------------------------------------------
 * GPL license:
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
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
#include <exbase64.h>
#include "test.fd.h"
#include "ubfunit1.h"
#include "xatmi.h"

/*
 * Bug #261 with this data bug gave incorrect base64 coding.
 */
exprivate char M_bin_data[] = {
    0x00, 0x00, 0x59, 0x5a, 0x54, 0x33, 0x6f, 0x52, 
    0x41, 0x46, 0x59, 0x5f, 0x62, 0x57, 0x48, 0x4e,
    0x4f, 0x46, 0x54, 0x70, 0x6d, 0x41, 0x46, 0x4e, 
    0x75, 0x64, 0x69, 0x6a, 0x2b, 0x6f, 0x63, 0x41,
    0x45, 0x42, 0x41, 0x44, 0x49, 0x41, 0x00, 0x00, 
    0x30, 0xf9, 0xa2, 0xd6, 0xff, 0x7f, 0x00, 0x00,
    0xa8, 0x2c, 0x93, 0xeb, 0x01, 0x00, 0x01, 0x00, 
    0x32, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x20, 0xf9, 0xa2, 0xd6, 0xff, 0x7f, 0x00, 0x00, 
    0x31, 0x2b, 0x7e, 0xec, 0x27, 0x7f, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0xf8, 0x38, 0x9c, 0xec, 0x01, 0x00, 0x00, 0x00
};

/**
 * Test base64 functions of Enduro/X standard library
 */
Ensure(test_nstd_base64)
{
    char bin[TPCONVMAXSTR];
    char str[TPCONVMAXSTR];
    size_t outlen;
    
    assert_not_equal(ndrx_xa_base64_encode((unsigned char *)M_bin_data, sizeof(M_bin_data), 
            &outlen, str), NULL);
    
    str[outlen] = EXEOS;
    NDRX_LOG(log_debug, "Got b64 string: [%s]", str);
    
    assert_not_equal(ndrx_xa_base64_decode((unsigned char *)str, strlen(str),
            &outlen, bin), NULL);
    
    assert_equal(memcmp(bin, M_bin_data, sizeof(M_bin_data)), 0);
    
}

/**
 * Base64 tests from Enduro/X Standard Library
 * @return
 */
TestSuite *ubf_nstd_base64(void)
{
    TestSuite *suite = create_test_suite();

    add_test(suite, test_nstd_base64);
            
    return suite;
}
