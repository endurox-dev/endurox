/**
 * @brief Test tpsprio, tpgprio - client
 *
 * @file atmiclt85.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2023, Mavimax, Ltd. All Rights Reserved.
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>

#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>
#include <test.fd.h>
#include <ndrstandard.h>
#include <nstopwatch.h>
#include <fcntl.h>
#include <unistd.h>
#include <nstdutil.h>
#include "test85.h"
#include <exassert.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Do the test call to the server
 */
int main(int argc, char** argv)
{

    UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, 56000);
    long rsplen;
    int i;
    int ret=EXSUCCEED;
    
    if (EXFAIL==CBchg(p_ub, T_STRING_FLD, 0, VALUE_EXPECTED, 0, BFLD_STRING))
    {
        NDRX_LOG(log_debug, "Failed to set T_STRING_FLD[0]: %s", Bstrerror(Berror));
        ret=EXFAIL;
        goto out;
    }
    
    /* 
     * Check Absolute
     */
    if (EXSUCCEED==tpsprio(0, TPABSOLUTE))
    {
        NDRX_LOG(log_error, "TESTERROR: 0 prio ABS shall fail");
        ret=EXFAIL;
        goto out;
    }
    NDRX_ASSERT_VAL_OUT(tperrno==TPEINVAL, "INvalid error, expected TPEINVAL got %d", tperrno);
    
    if (EXSUCCEED==tpsprio(101, TPABSOLUTE))
    {
        NDRX_LOG(log_error, "TESTERROR: 101 prio ABS shall fail");
        ret=EXFAIL;
        goto out;
    }
    NDRX_ASSERT_VAL_OUT(tperrno==TPEINVAL, "Invalid error, expected TPEINVAL got %d", tperrno);
    
    
    if (EXSUCCEED!=tpsprio(60, TPABSOLUTE))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to set tpsprio()");
        ret=EXFAIL;
        goto out;
    }
    
    if (EXFAIL == tpcall("TESTSV", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0))
    {
        NDRX_LOG(log_error, "TESTSV failed: %s", tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }
    
    if (60!=(ret=tpgprio()))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to verify tpgprio() got %d", ret);
        ret=EXFAIL;
        goto out;
    }
    
    ret=EXSUCCEED;
    
    if (EXFAIL == tpcall("TESTSV", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0))
    {
        NDRX_LOG(log_error, "TESTSV failed: %s", tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }
    
    if (50!=(ret=tpgprio()))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to verify tpgprio() got %d", ret);
        ret=EXFAIL;
        goto out;
    }
    ret=EXSUCCEED;
    
    
    /* Check relative.. */
    if (EXSUCCEED==tpsprio(101, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: 101 prio rel shall fail");
        ret=EXFAIL;
        goto out;
    }
    NDRX_ASSERT_VAL_OUT(tperrno==TPEINVAL, "INvalid error, expected TPEINVAL got %d", tperrno);
    
    if (EXSUCCEED==tpsprio(-101, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: 101 prio rel shall fail");
        ret=EXFAIL;
        goto out;
    }
    NDRX_ASSERT_VAL_OUT(tperrno==TPEINVAL, "INvalid error, expected TPEINVAL got %d", tperrno);
    
    /* call relative -10 */
    if (EXSUCCEED!=tpsprio(-10, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to set tpsprio()");
        ret=EXFAIL;
        goto out;
    }
    
    if (EXFAIL == tpcall("TESTSV", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0))
    {
        NDRX_LOG(log_error, "TESTSV failed: %s", tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }
    
    if (40!=(ret=tpgprio()))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to verify tpgprio() got %d", ret);
        ret=EXFAIL;
        goto out;
    }
    
    ret=EXSUCCEED;
    
    /* call relative -99 */
    if (EXSUCCEED!=tpsprio(-99, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to set tpsprio()");
        ret=EXFAIL;
        goto out;
    }
    
    if (EXFAIL == tpcall("TESTSV", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0))
    {
        NDRX_LOG(log_error, "TESTSV failed: %s", tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }
    
    if (1!=(ret=tpgprio()))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to verify tpgprio() got %d", ret);
        ret=EXFAIL;
        goto out;
    }
    
    ret=EXSUCCEED;
    
    
    /* call relative 99 */
    if (EXSUCCEED!=tpsprio(99, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to set tpsprio()");
        ret=EXFAIL;
        goto out;
    }
    
    if (EXFAIL == tpcall("TESTSV", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0))
    {
        NDRX_LOG(log_error, "TESTSV failed: %s", tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }
    
    if (100!=(ret=tpgprio()))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to verify tpgprio() got %d", ret);
        ret=EXFAIL;
        goto out;
    }
    
    ret=EXSUCCEED;
    
    if (EXFAIL == tpcall("TESTSV", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0))
    {
        NDRX_LOG(log_error, "TESTSV failed: %s", tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }
    
    if (50!=(ret=tpgprio()))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to verify tpgprio() got %d", ret);
        ret=EXFAIL;
        goto out;
    }
    
    ret=EXSUCCEED;

    
out:
    tpterm();
    fprintf(stderr, "Exit with %d\n", ret);

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
