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

/* TODO:
#include <oatmi.h>
#include <oubf.h>
*/
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
    UBFH *p_ub = NULL;
    int i;
    long rsplen;
    
    
    /* Allocate the context 
    TPCONTEXT_T ctx1 = (TPCONTEXT_T)ndrx_atmi_tls_new(FALSE, FALSE);
    
    if (NULL==ctx)
    {
        NDRX_LOG(log_error, "Failed to get new context");
    }
    */
    
    (UBFH *)tpalloc("UBF", NULL, 8192);
    for (i=0; i<1000; i++)
    {   

        /* Add some data to buffer */
        if (0==(i % 100))
        {
            if (SUCCEED!=Badd(p_ub, T_STRING_FLD, "HELLO WORLD!", 0L))
            {
                NDRX_LOG(log_error, "TESTERROR: Failed to add T_STRING_FLD:%s", 
                    Bstrerror(Berror));
                FAIL_OUT(ret);
            }
        }
        
        /* About to call service */
        tplog(log_warn, "Calling TEST32_1ST!");
        if (FAIL == tpcall("TEST32_1ST", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0))
        {
            NDRX_LOG(log_error, "TEST32_1ST failed: %s", tpstrerror(tperrno));
            FAIL_OUT(ret);
        }
        
    }
    
out:

    return ret;
}

