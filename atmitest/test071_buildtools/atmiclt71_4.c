/**
 * @brief Test buildserver, buildclient and buildtms - client
 *  This will run with default switch.
 *  - tms will be built with TestSw
 *  - atmisv71 will be built with TestSw too.
 *
 * @file atmiclt71_2.c
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
#include "test71.h"

#ifdef __cplusplus
extern "C"
{
#endif

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
int run_echo_services(int argc, char** argv)
{
    UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, 56000);
    char *echoservices[] = {"Z", "ECHOSV", "FI1", "FI2", "ECHO2SV", NULL};
    long rsplen;
    long i, j;
    int ret=EXSUCCEED;
    
    /***************************************************************************/
    NDRX_LOG(log_debug, "Check echo service...");
    /***************************************************************************/
    
    for (j=0; NULL!=echoservices[j]; j++)
    {
        for (i=0; i<1000; i++)
        {
            long testval;
            if (EXFAIL==CBchg(p_ub, T_STRING_FLD, 0, (char *)&i, 0, BFLD_LONG))
            {
                NDRX_LOG(log_debug, "Failed to set T_STRING_FLD[0]: %s", Bstrerror(Berror));
                ret=EXFAIL;
                goto out;
            }

            if (EXFAIL == tpcall(echoservices[j], (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0))
            {
                NDRX_LOG(log_error, "%s failed: %s", echoservices[j], tpstrerror(tperrno));
                ret=EXFAIL;
                goto out;
            }
            
            /* get value back.. */
            if (EXFAIL==CBget(p_ub, T_STRING_2_FLD, 0, (char *)&testval, 0, BFLD_LONG))
            {
                NDRX_LOG(log_debug, "Failed to set T_STRING_2_FLD[0]: %s", Bstrerror(Berror));
                ret=EXFAIL;
                goto out;
            }
            
            if (i!=testval)
            {
                NDRX_LOG(log_error, "TESTERROR echosvc: %s fail %ld vs %ld", 
                        echoservices[j], i, testval);
            }
            
        }
    }
    
out:

    return ret;
}

#ifdef __cplusplus
} // extern "C"
#endif

/* vim: set ts=4 sw=4 et smartindent: */
