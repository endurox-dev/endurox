/* 
** Growlist testing from Enduro/X Standard Library
**
** @file test_nstd_growlist.c
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
#include <exbase64.h>
#include "test.fd.h"
#include "ubfunit1.h"
#include "xatmi.h"

/**
 * Basic grow list testing
 */
Ensure(test_nstd_growlist)
{
#if 0
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
#endif
    
}

/**
 * Grow list testing
 * @return
 */
TestSuite *ubf_nstd_growlist(void)
{
    TestSuite *suite = create_test_suite();

    add_test(suite, test_nstd_growlist);
            
    return suite;
}
