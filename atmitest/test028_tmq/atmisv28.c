/**
 *
 * @file atmisv28.c
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
    int ret=EXSUCCEED;
    UBFH *p_ub = (UBFH *)p_svc->data;
    
    if (EXSUCCEED!=Bchg(p_ub, T_STRING_FLD, 0, "OK", 0L))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to set T_STRING_FLD!");
    }

out:
    tpreturn(  ret==EXSUCCEED?TPSUCCESS:TPFAIL,
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
    int ret=EXSUCCEED;
    static int cnt = 0;
    UBFH *p_ub = (UBFH *)p_svc->data;
    
    cnt++;
    
    if (1 == (cnt % 4) )
    {
        ret=EXFAIL;
        goto out;
    }
    
    if (EXSUCCEED!=Bchg(p_ub, T_STRING_FLD, 0, "OK", 0L))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to set T_STRING_FLD!");
    }
    
    if (!tpgetlev())
    {
        NDRX_LOG(log_error, "TESTERROR: invocation must be transactional!");
    }

out:
    tpreturn(  ret==EXSUCCEED?TPSUCCESS:TPFAIL,
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
    int ret=EXFAIL;
    UBFH *p_ub = (UBFH *)p_svc->data;
    
out:
    tpreturn(  ret==EXSUCCEED?TPSUCCESS:TPFAIL,
                0L,
                (char *)p_ub,
                0L,
                0L);
}

/**
 * Broadcast to the message to the client
 */
void CLTBCAST(TPSVCINFO *p_svc)
{
    int ret=EXFAIL;
    UBFH *p_ub = (UBFH *)p_svc->data;

    if (EXSUCCEED!=tpbroadcast(NULL, NULL, "atmiclt28", (char *)p_ub, 0L, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to broadcast: %s", tpstrerror(tperrno));
        goto out;
    }
    
out:
    tpreturn(  ret==EXSUCCEED?TPSUCCESS:TPFAIL,
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
    int ret = EXSUCCEED;
    NDRX_LOG(log_debug, "tpsvrinit called");
    
    if (EXSUCCEED!=tpopen())
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to tpopen: %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }

    if (EXSUCCEED!=tpadvertise("SVCOK", SVCOK))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to initialize SVCOK!");
        EXFAIL_OUT(ret);
    }
    else if (EXSUCCEED!=tpadvertise("SVCFAIL", SVCFAIL))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to initialize SVCFAIL!");
        EXFAIL_OUT(ret);
    }
    else if (EXSUCCEED!=tpadvertise("FAILRND", FAILRND))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to initialize FAILRND!");
        EXFAIL_OUT(ret);
    }
    else if (EXSUCCEED!=tpadvertise("CLTBCAST", CLTBCAST))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to initialize CLTBCAST!");
        EXFAIL_OUT(ret);
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
    
    if (EXSUCCEED!=tpclose())
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to tpclose: %s", tpstrerror(tperrno));
    }
}
/* vim: set ts=4 sw=4 et smartindent: */
