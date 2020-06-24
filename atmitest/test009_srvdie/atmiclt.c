/**
 * @brief Basic test client
 *
 * @file atmiclt.c
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

#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>
#include <test.fd.h>
#include <ndrstandard.h>
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
int main(int argc, char** argv) {

    UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, 1024);
    long rsplen;
    int ret=EXSUCCEED;
    int tpcall_err;
    int cd;
    if (argc > 1 && 0==strcmp(argv[1], "tpacall_norply"))
    {
        /* this one is called and server processes it for 4 sec and then die */
        ret = tpacall("EXITSVC", (char *)p_ub, 0L, TPNOREPLY);
        if (EXSUCCEED!=ret)
        {
            NDRX_LOG(log_error, "TESTERROR first tpacall got %d: %s",
                ret, tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
        
        /* we make next calls */
        ret = tpacall("EXITSVC", (char *)p_ub, 0L, TPNOREPLY);
        if (EXSUCCEED!=ret)
        {
            NDRX_LOG(log_error, "TESTERROR second tpacall got %d: %s",
                ret, tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
        
        NDRX_LOG(log_error, "Wait 10 for dead + sanity check...");
        sleep(20);
        
        if (EXSUCCEED==(ret = tpgetrply(&cd, (char **)&p_ub, &rsplen, 
                TPNOBLOCK | TPGETANY)))
        {
            NDRX_LOG(log_error, "TESTERROR There must be TPEBLOCK error set!");
            EXFAIL_OUT(ret);
        }
        
        if (TPEBLOCK!=tperrno)
        {
            NDRX_LOG(log_error, "TESTERROR: tperrno must be TPEBLOCK but is: %d: %s",
                    tperrno, tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
        
        return 0;
    }
    
    if (EXFAIL == tpcall("TESTSVFN", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0))
    {
        NDRX_LOG(log_error, "TESTSVFN failed: %s", tpstrerror(tperrno));
        
        /* Check the code TOUT & SVCERR is OK*/
        tpcall_err = tperrno;
        if (tpcall_err==TPENOENT)
        {
            /* service error is reply from ndrxd on behalf of the server */
            NDRX_LOG(log_error, "OK-TPESVCERR");
        }
        else if (tpcall_err==TPETIME)
        {
            /* tout because first is dead */
            NDRX_LOG(log_error, "OK-TPETIME");
        }
        else
        {
            NDRX_LOG(log_error, "TESTERROR: UNEXPECTED ERROR");
        }
        
        goto out;
    }

out:
    /* Terminate the session */
    tpterm();

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
