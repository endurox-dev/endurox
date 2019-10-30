/**
 * @brief tpadm .TMIB interface tests - client
 *
 * @file atmiclt68.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2019, Mavimax, Ltd. All Rights Reserved.
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
#include "test68.h"
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
    int cd;
    long revent;
    TPCONTEXT_T ctx = NULL;

    if (argc<2)
    {
        fprintf(stderr, "Usage: %s call|fail|conv\n", argv[0]);
        EXFAIL_OUT(ret);
    }
    
    tpinit(NULL);
    
    if (0==strcmp(argv[1], "call"))
    {
        if (EXFAIL==CBchg(p_ub, T_STRING_FLD, 0, VALUE_EXPECTED, 0, BFLD_STRING))
        {
            NDRX_LOG(log_debug, "Failed to set T_STRING_FLD[0]: %s", Bstrerror(Berror));
            ret=EXFAIL;
            goto out;
        }    

        for (i=0; i<100; i++)
        {
            if (EXFAIL == tpcall("TESTSV", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0))
            {
                NDRX_LOG(log_error, "TESTSV failed: %s", tpstrerror(tperrno));
                ret=EXFAIL;
                goto out;
            }
        }
    }
    else if (0==strcmp(argv[1], "fail"))
    {
        if (EXFAIL == tpcall("FAILSV", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0))
        {
            NDRX_LOG(log_error, "FAILSV failed: %s", tpstrerror(tperrno));
            ret=EXFAIL;
            goto out;
        }
    }
    else if (0==strcmp(argv[1], "conv"))
    {
        /* avoid memory leaks, just get a handler for current ATMI TLS*/
        tpgetctxt(&ctx, 0L);
        
        /* lets create new context !!!! to see the cpm output ...  */
        
        tpnewctxt(EXTRUE, EXTRUE);
        
        /* run some init to open the queues! */
        tpinit(NULL);
        
        if (EXFAIL == (cd=tpconnect("CONVSV", (char *)p_ub, 0L, TPRECVONLY)))
        {
            NDRX_LOG(log_error, "FAILSV failed: %s", tpstrerror(tperrno));
            ret=EXFAIL;
            goto out;
        }
        
        if (EXFAIL==tprecv(cd, (char **)&p_ub, &rsplen, 0, &revent))
        {
            NDRX_LOG(log_error, "Failed to recv: %s", tpstrerror(tperrno));
            ret=EXFAIL;
            goto out;
        }
        
    }
    else
    {
        NDRX_LOG(log_error, "TESTERROR! invalid command [%s]", argv[1]);
    }
    
out:

    tpterm();

    if (NULL!=ctx)
    {
        tpsetctxt(ctx, 0L);
        tpterm();
        tpfreectxt(ctx);
    }
    fprintf(stderr, "Exit with %d\n", ret);

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
