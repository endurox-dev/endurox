/* 
 * @brief test big messages over 64kb - client
 *
 * @file atmiclt42.c
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
#include "test42.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Do the test call to the server
 * We will send 1 mega-byte message to server and expect to receive response back
 */
int main(int argc, char** argv) {

    UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, TEST_MSGSIZE+1024); /* + hdr */
    long rsplen;
    int i, j;
    int ret=EXSUCCEED;
    char bufferreq[TEST_MSGSIZE];
    BFLDLEN retlen;

	NDRX_LOG(log_error, "ATMI message size: %ld", ATMI_MSG_MAX_SIZE);
    
    for (j=0; j<2000; j++)
    {
        for (i=0; i<TEST_MSGSIZE; i++)
        {
            char c = (char) ((i+2) & 0xff);
            bufferreq[i] = c;
        }

        if (EXFAIL==Bchg(p_ub, T_CARRAY_FLD, 0, bufferreq, TEST_MSGSIZE))
        {
            NDRX_LOG(log_debug, "Failed to set T_CARRAY_FLD[0]: %s", Bstrerror(Berror));
            ret=EXFAIL;
            goto out;
        }    

        if (EXFAIL == tpcall("TESTSV", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0))
        {
            NDRX_LOG(log_error, "TESTSV failed: %s", tpstrerror(tperrno));
            ret=EXFAIL;
            goto out;
        }

        /* test the response... */
        retlen = TEST_MSGSIZE;
        if (EXFAIL==Bget(p_ub, T_CARRAY_FLD, 0, bufferreq, &retlen))
        {
            NDRX_LOG(log_debug, "Failed to get T_CARRAY_FLD[0]: %s", Bstrerror(Berror));
            ret=EXFAIL;
            goto out;
        }

        if (retlen != TEST_MSGSIZE)
        {
            NDRX_LOG(log_error, "Invalid message size received, expected: %d, got: %d", 
                    (int)retlen, (int)TEST_MSGSIZE);
            ret=EXFAIL;
            goto out;
        }

        for (i=0; i<TEST_MSGSIZE; i++)
        {
            char c = (char) ((i+3) & 0xff);
            if (bufferreq[i] != c)
            {
                NDRX_LOG(log_error, "TESTERROR and index %d: expected %x but got %x",
                            i, (int)bufferreq[i], (int) (c));
                ret=EXFAIL;
                goto out;
            }
        }
    }
    
out:
    tpterm();
    fprintf(stderr, "Exit with %d\n", ret);

    return ret;
}
