/* 
 * @brief Base64 tests (Bug #261)
 *
 * @file test_nstd_debug.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or ATR Baltic's license for commercial use.
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
#include <ndebug.h>
#include <string.h>
#include <ndebug.h>
#include <exbase64.h>
#include "test.fd.h"
#include "ubfunit1.h"
#include "xatmi.h"

/**
 * Test base64 functions of Enduro/X standard library
 */
Ensure(test_nstd_tplogqinfo)
{
    int ret;
    
    /* lets configure our logger */
    
    assert_equal(tplogconfig(LOG_FACILITY_NDRX | LOG_FACILITY_UBF | LOG_FACILITY_TP, 
            EXFAIL, "ndrx=5 ubf=4 tp=3 iflags=detailed", NULL, NULL), EXSUCCEED);
    
    
    ret = tplogqinfo(log_error, TPLOGQI_GET_NDRX);
    
    NDRX_LOG(log_debug, "got ret=%d", ret);
    
    /* The debug shall be returned for given level
     * debugger
     */
    /* as we have something to log */
    assert_not_equal(ret, 0);
    assert_not_equal(ret, EXFAIL);
    /* the logger shall be LOG_FACILITY_NDRX */
    assert_equal(ret & LOG_FACILITY_MASK, LOG_FACILITY_NDRX);
    assert_equal(TPLOGQI_RET_DBGLEVGET(ret), log_debug);
    assert_equal(ret & TPLOGQI_RET_HAVDETAILED, 0);
    
    /* eval detailed flags */
    
    ret = tplogqinfo(log_error, TPLOGQI_GET_TP | TPLOGQI_EVAL_DETAILED);
    
    assert_not_equal(ret, 0);
    assert_not_equal(ret, EXFAIL);
    /* the logger shall be LOG_FACILITY_NDRX */
    assert_equal(ret & LOG_FACILITY_MASK, LOG_FACILITY_TP);
    assert_equal(TPLOGQI_RET_DBGLEVGET(ret), log_warn);
    assert_not_equal(ret & TPLOGQI_RET_HAVDETAILED, 0);
    
    
    /* test UBF, nothing to return */
    
    ret = tplogqinfo(log_debug, TPLOGQI_GET_UBF | TPLOGQI_EVAL_DETAILED);
    assert_equal(ret, 0);
    
    
    /* return anyway */
    ret = tplogqinfo(log_debug, TPLOGQI_GET_UBF | TPLOGQI_EVAL_DETAILED | TPLOGQI_EVAL_RETURN);
    assert_not_equal(ret, 0);
    assert_not_equal(ret, EXFAIL);
    
    assert_equal(ret & LOG_FACILITY_MASK, LOG_FACILITY_UBF);
    assert_equal(TPLOGQI_RET_DBGLEVGET(ret), log_info);
    assert_not_equal(ret & TPLOGQI_RET_HAVDETAILED, 0);
    
    /* test error */
    ret = tplogqinfo(log_debug, 0);
    assert_equal(ret, EXFAIL);
    assert_equal(Nerror, NEINVAL);
    
}

/**
 * Debug routine tests
 * @return
 */
TestSuite *ubf_nstd_debug(void)
{
    TestSuite *suite = create_test_suite();

    add_test(suite, test_nstd_tplogqinfo);
            
    return suite;
}
