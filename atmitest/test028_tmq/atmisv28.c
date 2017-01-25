/* 
**
** @file atmisv3.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, ATR Baltic, SIA. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or ATR Baltic's license for commercial use.
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
** A commercial use license is available from ATR Baltic, SIA
** contact@atrbaltic.com
** -----------------------------------------------------------------------------
*/

#include <stdio.h>
#include <stdlib.h>
#include <ndebug.h>
#include <atmi.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <test.fd.h>

/**
 * This service will add T_STRING
 * @param p_svc
 */
void SVCOK (TPSVCINFO *p_svc)
{
    int ret=SUCCEED;
    UBFH *p_ub = (UBFH *)p_svc->data;
    
    if (SUCCEED!=Bchg(p_ub, T_STRING_FLD, 0, "OK", 0L))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to set T_STRING_FLD!");
    }

out:
    tpreturn(  ret==SUCCEED?TPSUCCESS:TPFAIL,
                0L,
                (char *)p_ub,
                0L,
                0L);
}

/**
 * Fail 'randomly'
 * @param p_svc
 */
void FAILRND (TPSVCINFO *p_svc)
{
    int ret=SUCCEED;
    static int cnt = 0;
    UBFH *p_ub = (UBFH *)p_svc->data;
    
    cnt++;
    
    if (1 == (cnt % 4) )
    {
        ret=FAIL;
        goto out;
    }
    
    if (SUCCEED!=Bchg(p_ub, T_STRING_FLD, 0, "OK", 0L))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to set T_STRING_FLD!");
    }

out:
    tpreturn(  ret==SUCCEED?TPSUCCESS:TPFAIL,
                0L,
                (char *)p_ub,
                0L,
                0L);
}

/**
 * Returns failure to caller
 * @param p_svc
 */
void SVCFAIL (TPSVCINFO *p_svc)
{
    int ret=FAIL;
    UBFH *p_ub = (UBFH *)p_svc->data;
    
out:
    tpreturn(  ret==SUCCEED?TPSUCCESS:TPFAIL,
                0L,
                (char *)p_ub,
                0L,
                0L);
}

/*
 * Do initialization
 */
int NDRX_INTEGRA(tpsvrinit)(int argc, char **argv)
{
    int ret = SUCCEED;
    NDRX_LOG(log_debug, "tpsvrinit called");

    if (SUCCEED!=tpadvertise("SVCOK", SVCOK))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to initialize SVCOK!");
        FAIL_OUT(ret);
    }
    else if (SUCCEED!=tpadvertise("SVCFAIL", SVCFAIL))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to initialize SVCFAIL!");
        FAIL_OUT(ret);
    }
    else if (SUCCEED!=tpadvertise("FAILRND", FAILRND))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to initialize FAILRND!");
        FAIL_OUT(ret);
    }
    
out:
    return ret;
    
}

/**
 * Do de-initialization
 */
void NDRX_INTEGRA(tpsvrdone)(void)
{
    NDRX_LOG(log_debug, "tpsvrdone called");
}
