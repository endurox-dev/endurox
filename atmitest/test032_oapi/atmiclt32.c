/* 
** Basic test client
**
** @file atmiclt32.c
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include <unistd.h>

#include <oatmi.h>
#include <oubf.h>
#include <ubf.h>
#include <atmi.h>
#include <ndebug.h>
#include <test.fd.h>
#include <ndrstandard.h>
#include <nstdutil.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/*
 * Do the test call to the server
 */
int main(int argc, char** argv)
{
    int ret = SUCCEED;
    UBFH *p_ub1 = NULL;
    UBFH *p_ub2 = NULL;
    UBFH *p_ub3 = NULL;
    
    int ret_ctx;
    long rsplen;
    
    /* Allocate the context */
    TPCONTEXT_T ctx1 = tpnewctxt();
    TPCONTEXT_T ctx2 = tpnewctxt();
    TPCONTEXT_T ctx3 = tpnewctxt();
    
    TPCONTEXT_T tmpt;
    
    if (NULL==ctx1 || NULL==ctx2 || NULL==ctx3)
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to get new context (%p/%p/%p): %s",
                ctx1, ctx2, ctx3, tpstrerror(tperrno));
        exit(FAIL);
    }
    
    if (NULL==(p_ub1 = (UBFH *)Otpalloc(&ctx1, "UBF", NULL, 8192)))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to Otpalloc ub1: %s",
                    Otpstrerror(ctx3, Otperrno(ctx3)));
        exit(FAIL);
    }
    
    if (NULL==(p_ub2 = (UBFH *)Otpalloc(&ctx2, "UBF", NULL, 8192)))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to Otpalloc ub2: %s",
                Otpstrerror(ctx3, Otperrno(ctx3)));
        exit(FAIL);
    }
    
    if (NULL==(p_ub3 = (UBFH *)Otpalloc(&ctx3, "UBF", NULL, 8192)))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to Otpalloc ub3: %s", 
                Otpstrerror(ctx3, Otperrno(ctx3)));
        exit(FAIL);
    }
    
    /* get the current context, as the thread, we must be at NULL context */
    if (TPNULLCONTEXT!=(ret_ctx=tpgetctxt(&tmpt, 0)))
    {
        NDRX_LOG(log_error, "TESTERROR: tpgetctxt() NOT %d", ret_ctx);
        FAIL_OUT(ret);
    }
    
    /* call service in async way, keep the following order:
     * - ctx1 Oacall()
     * - ctx2 Oacall()
     * - ctx3 Oacall()
     * - ctx3 Ogetrply()
     * - ctx2 Ogetrply()
     * - ctx1 Ogetrply()
     */
    
    
out:

    return ret;
}

