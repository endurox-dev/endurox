/**
 * @brief Test for tpcancel - client
 *
 * @file atmiclt65.c
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
#include <exassert.h>
#include "test65.h"
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
    long rsplen;
    char *rspbuf = NULL;
    int i, cd;
    int ret=EXSUCCEED;
    int err;
    
    /* attempt to cancel only */
    for (i=0; i<10000; i++)
    {
        cd = tpacall("TESTSV", NULL, 0L, 0);
        NDRX_ASSERT_TP_OUT(EXFAIL!=cd, "failed to tpacall");
        NDRX_ASSERT_TP_OUT(EXFAIL!=tpcancel(cd), "failed to cancel");

    }

    for (i=0; i<10000; i++)
    {
        //the reply queue to client might fill up
        //and server then might get blocked and thus we time out.
        cd = tpacall("TESTSV", NULL, 0L, TPNOBLOCK);
        if (cd < 1)
        {
            /* ignore timeout condition ...*/
            if (TPEBLOCK==tperrno)
            {
                continue;
            }
            NDRX_LOG(log_error, "TESTERROR ! cd < 1: %s", tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
        
        tpcancel(cd);
        
        if (EXSUCCEED==tpgetrply(&cd, &rspbuf, &rsplen, 0))
        {
            NDRX_LOG(log_error, "TESTERROR ! tpgetrply succeed!");
            EXFAIL_OUT(ret);
        }
        
        /* the error shall be  */
        err = tperrno;
        
        if (TPEBADDESC!=err)
        {
            NDRX_LOG(log_error, "TESTERROR ! Excpected err %d got %d", 
                    TPEBADDESC, err);
            EXFAIL_OUT(ret);
        }
    }
    
out:
    tpterm();
    fprintf(stderr, "Exit with %d\n", ret);

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
