/**
 * @brief Basic test client
 *
 * @file atmiclt.c
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
    char buf[128]={EXEOS};
    
    if (0==strcmp(argv[1], "DOADV"))
    {
        if (EXFAIL == tpcall("DOADV", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0))
        {
            NDRX_LOG(log_error, "TESTERROR: DOADV failed: %s", tpstrerror(tperrno));
            ret=EXFAIL;
            goto out;
        }
    }
    else if (0==strcmp(argv[1], "UNADV"))
    {
        if (EXFAIL == tpcall("UNADV", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0))
        {
            NDRX_LOG(log_error, "TESTERROR: UNADV failed: %s", tpstrerror(tperrno));
            ret=EXFAIL;
            goto out;
        }
    }
    else if (0==strcmp(argv[1], "TEST"))
    {
        if (EXFAIL == tpcall("TESTSVFN", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0))
        {
            NDRX_LOG(log_error, "TESTSVFN failed: %s", tpstrerror(tperrno));
            ret=EXFAIL;
            goto out;
        }
        /* Verify the data */
        if (EXFAIL==Bget(p_ub, T_STRING_FLD, 0, (char *)buf, 0))
        {
            NDRX_LOG(log_debug, "Failed to get T_STRING_FLD[0]");
            ret=EXFAIL;
            goto out;
        }
        else if (0!=strcmp(buf, "THIS IS TEST - OK!"))
        {
            NDRX_LOG(log_debug, "Call test failed");
            ret=EXFAIL;
            goto out;
        }
    }
    else
    {
        NDRX_LOG(log_error, "ERROR: Invalid command, valid ones are: DOADV, UNADV, TEST");
                
        ret=EXFAIL;
        goto out;
    }

out:
    /* Terminate the session */
    tpterm();

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
