/**
 * @brief Test tpcall noblock operation - client
 *
 * @file atmiclt45.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or Mavimax's license for commercial use.
 * -----------------------------------------------------------------------------
 * GPL license:
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
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
#include "test45.h"
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
    
    if (EXFAIL==CBchg(p_ub, T_STRING_FLD, 0, VALUE_EXPECTED, 0, BFLD_STRING))
    {
        NDRX_LOG(log_debug, "Failed to set T_STRING_FLD[0]: %s", Bstrerror(Berror));
        ret=EXFAIL;
        goto out;
    }    
    
    /* firstly we will do tpacall to fill the queue */
    while (EXFAIL!=tpacall("TESTSV", (char *)p_ub, 0L, TPNOBLOCK))
    {
        
    }
    
    if (tperrno!=TPEBLOCK)
    {
        NDRX_LOG(log_error, "TESTERROR: tpacall other failure: %s - must be TPEBLOCK!",
                tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }
    
    /* then then these must give TPEBLOCK */
    for (i=0; i<1000; i++)
    {
        if (EXFAIL != tpcall("TESTSV", (char *)p_ub, 0L, (char **)&p_ub, 
                &rsplen,TPNOBLOCK))
        {
            NDRX_LOG(log_error, "TESTERROR: tpcall must fail!");
            ret=EXFAIL;
            goto out;
        }
        
        if (tperrno!=TPEBLOCK)
        {
            NDRX_LOG(log_error, "TESTERROR: tpcall other failure: %s - must be TPEBLOCK!",
                    tpstrerror(tperrno));
            ret=EXFAIL;
            goto out;
        }
    }
    
out:
    tpterm();
    fprintf(stderr, "Exit with %d\n", ret);

    return ret;
}
