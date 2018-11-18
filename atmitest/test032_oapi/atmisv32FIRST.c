/**
 * @brief Test server
 *
 * @file atmisv32FIRST.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * AGPL or Mavimax's license for commercial use.
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
#include <ndebug.h>
#include <oubf.h>
#include <oatmisrv.h>

void TEST32_1ST (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;
    UBFH *p_ub = (UBFH *)p_svc->data;
    long l;
    TPCONTEXT_T c;
    
    /* Get the field at 0 occurrence and set it at 2nd occurrence */
    
    if (TPMULTICONTEXTS!=tpgetctxt(&c, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to get context!");
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=OBget(&c, p_ub, T_LONG_FLD, 0, (char *)&l, 0L))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to get T_LONG_FLD[0]!");
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_info, "got l = %ld", l);
    
    if (EXSUCCEED!=OBchg(&c, p_ub, T_LONG_FLD, 1, (char *)&l, 0L))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to set T_LONG_FLD[1]!");
        EXFAIL_OUT(ret);
    }
    
out:

    Otpreturn(&c, EXSUCCEED==ret?TPSUCCESS:TPFAIL,
            0L,
            (char *)p_ub,
            0L,
            0L);

}

/**
 * Do initialization
 */
int NDRX_INTEGRA(tpsvrinit)(int argc, char **argv)
{
    NDRX_LOG(log_debug, "tpsvrinit called");
    
    if (EXSUCCEED!=tpadvertise("TEST32_1ST", TEST32_1ST))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to initialize TEST32_1ST (first)!");
    }
    
    return EXSUCCEED;
}

/**
 * Do de-initialization
 */
void NDRX_INTEGRA(tpsvrdone)(void)
{
    NDRX_LOG(log_debug, "tpsvrdone called");
}

/* vim: set ts=4 sw=4 et smartindent: */
