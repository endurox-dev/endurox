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
    int cd1, cd2, cd3;
    long rsplen;
    long l;
    int i;
    
    for (i=0; i<100000; i++)
    {
        /* Allocate the context */
        TPCONTEXT_T ctx1 = tpnewctxt();
        TPCONTEXT_T ctx2 = tpnewctxt();
        TPCONTEXT_T ctx3 = tpnewctxt();

        TPCONTEXT_T tmpt;

        if (NULL==ctx1 || NULL==ctx2 || NULL==ctx3)
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to get new context (%p/%p/%p): %s",
                    ctx1, ctx2, ctx3, tpstrerror(tperrno));
            FAIL_OUT(ret);
        }

        if (NULL==(p_ub1 = (UBFH *)Otpalloc(&ctx1, "UBF", NULL, 8192)))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to Otpalloc ub1: %s",
                        Otpstrerror(&ctx1, Otperrno(&ctx1)));
            FAIL_OUT(ret);
        }

        if (NULL==(p_ub2 = (UBFH *)Otpalloc(&ctx2, "UBF", NULL, 8192)))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to Otpalloc ub2: %s",
                    Otpstrerror(&ctx2, Otperrno(&ctx2)));
            FAIL_OUT(ret);
        }

        if (NULL==(p_ub3 = (UBFH *)Otpalloc(&ctx3, "UBF", NULL, 8192)))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to Otpalloc ub3: %s", 
                    Otpstrerror(&ctx3, Otperrno(&ctx3)));
            FAIL_OUT(ret);
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

        if (SUCCEED!=OCBchg(&ctx1, p_ub1, T_LONG_FLD, 0, "1", 0L, BFLD_STRING))
        {
            NDRX_LOG(log_error, "TESTERROR: OCBchg() failed %s", 
                    OBstrerror(&ctx1, OBerror(&ctx1)));
            FAIL_OUT(ret);
        }

        if (SUCCEED!=OCBchg(&ctx2, p_ub2, T_LONG_FLD, 0, "2", 0L, BFLD_STRING))
        {
            NDRX_LOG(log_error, "TESTERROR: OCBchg() failed %s", 
                    OBstrerror(&ctx2, OBerror(&ctx2)));
            FAIL_OUT(ret);
        }

        if (SUCCEED!=OCBchg(&ctx3, p_ub3, T_LONG_FLD, 0, "3", 0L, BFLD_STRING))
        {
            NDRX_LOG(log_error, "TESTERROR: OCBchg() failed %s", 
                    OBstrerror(&ctx3, OBerror(&ctx3)));
            FAIL_OUT(ret);
        }

        /* call the server */
        if (FAIL==(cd1=Otpacall(&ctx1, "TEST32_1ST", (char *)p_ub1, 0L, 0L)))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to Otpacall 1: %s", 
                    Otpstrerror(&ctx1, Otperrno(&ctx1)));
            FAIL_OUT(ret);
        }

        if (FAIL==(cd2=Otpacall(&ctx2, "TEST32_1ST", (char *)p_ub2, 0L, 0L)))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to Otpacall 2: %s", 
                    Otpstrerror(&ctx2, Otperrno(&ctx2)));
            FAIL_OUT(ret);
        }

        if (FAIL==(cd3=Otpacall(&ctx3, "TEST32_1ST", (char *)p_ub3, 0L, 0L)))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to Otpacall 3: %s", 
                    Otpstrerror(&ctx3, Otperrno(&ctx3)));
            FAIL_OUT(ret);
        }

        /* get the replyes... */

        /* reply 1 */
        if (SUCCEED!=Otpgetrply(&ctx1, &cd1, (char **)&p_ub1, &rsplen, 0L))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to Otpgetrply 1: %s", 
                    Otpstrerror(&ctx1, Otperrno(&ctx1)));
            FAIL_OUT(ret);
        }

        if (SUCCEED!=OBget(&ctx1, p_ub1, T_LONG_FLD, 1, (char *)&l, 0L))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to get T_LONG_FLD[1]!");
            FAIL_OUT(ret);
        }

        if (l!=1)
        {
            NDRX_LOG(log_error, "TESTERROR: got l=%ld, but must 1", l);
            FAIL_OUT(ret);
        }

        /* reply 2 */
        if (SUCCEED!=Otpgetrply(&ctx2, &cd2, (char **)&p_ub2, &rsplen, 0L))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to Otpgetrply 2: %s", 
                    Otpstrerror(&ctx2, Otperrno(&ctx2)));
            FAIL_OUT(ret);
        }

        if (SUCCEED!=OBget(&ctx2, p_ub2, T_LONG_FLD, 1, (char *)&l, 0L))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to get T_LONG_FLD[1]!");
            FAIL_OUT(ret);
        }

        if (l!=2)
        {
            NDRX_LOG(log_error, "TESTERROR: got l=%ld, but must 2", l);
            FAIL_OUT(ret);
        }

        /* reply 3 */
        if (SUCCEED!=Otpgetrply(&ctx3, &cd3, (char **)&p_ub3, &rsplen, 0L))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to Otpgetrply 3: %s", 
                    Otpstrerror(&ctx3, Otperrno(&ctx3)));
            FAIL_OUT(ret);
        }

        if (SUCCEED!=OBget(&ctx3, p_ub3, T_LONG_FLD, 1, (char *)&l, 0L))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to get T_LONG_FLD[1]!");
            FAIL_OUT(ret);
        }

        if (l!=3)
        {
            NDRX_LOG(log_error, "TESTERROR: got l=%ld, but must 3", l);
            FAIL_OUT(ret);
        }

        Otpfree(&ctx1, (char *)p_ub1);
        Otpfree(&ctx2, (char *)p_ub2);
        Otpfree(&ctx3, (char *)p_ub3);

        if (SUCCEED!=Otpterm(&ctx1))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to terminate client 1", 
                    Otpstrerror(&ctx1, Otperrno(&ctx1)));
            FAIL_OUT(ret);
        }

        if (SUCCEED!=Otpterm(&ctx2))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to terminate client 2", 
                    Otpstrerror(&ctx2, Otperrno(&ctx2)));
            FAIL_OUT(ret);
        }

        if (SUCCEED!=Otpterm(&ctx3))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to terminate client 3", 
                    Otpstrerror(&ctx3, Otperrno(&ctx3)));
            FAIL_OUT(ret);
        }

        tpfreectxt(ctx1);
        tpfreectxt(ctx2);
        tpfreectxt(ctx3);

    }
out:

    return ret;
}

