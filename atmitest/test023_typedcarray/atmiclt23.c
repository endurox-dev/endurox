/* 
 * @brief Typed CARRAY testing
 *
 * @file atmiclt23.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or ATR Baltic's license for commercial use.
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

#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>
#include <test.fd.h>
#include <ndrstandard.h>
#include "test023.h"

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

    long rsplen;
    int i, j;
    int ret=EXSUCCEED;
    char *buf = tpalloc("CARRAY", NULL, 100);
    
    if (NULL==buf)
    {
        NDRX_LOG(log_error, "TESTERROR: failed to alloc CARRAY buffer!");
        EXFAIL_OUT(ret);
    }

    for (j=0; j<10000; j++)
    {
        for (i=0; i<TEST_REQ_SIZE; i++)
        {
            buf[i] = i;
        }

        if (EXSUCCEED!=tpcall("TEST23_CARRAY", buf, TEST_REQ_SIZE, &buf, &rsplen, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: failed to call TEST23_CARRAY");
            EXFAIL_OUT(ret);
        }

        NDRX_LOG(log_debug, "Got message [%s] ", buf);

        /* Check the len */
        if (TEST_REPLY_SIZE!=rsplen)
        {
            NDRX_LOG(log_error, "TESTERROR: Invalid message len for [%s] got: %d, "
                    "but need: %d ", buf, strlen(buf), TEST_REPLY_SIZE);
        }

        for (i=0; i<TEST_REPLY_SIZE; i++)
        {
            if (((char)buf[i])!=((char)(i%256)))
            {
                NDRX_LOG(log_error, "TESTERROR: Invalid char at %d, "
                        "expected: 0x%1x, but got 0x%1x", i, 
                        (unsigned char)buf[i], (unsigned char)(i%256));
            }
        }

        strcpy(buf, "Hello Enduro/X from Mars");

        ret=tppost("TEST23EV", buf, strlen(buf), TPSIGRSTRT);

        if (1!=ret)
        {
            NDRX_LOG(log_error, "TESTERROR: Event is not processed by "
                    "exactly 1 service! (%d) ", ret);
            ret=EXFAIL;
            goto out;
        }
        /* Realloc to some more... */
        if (NULL== (buf = tprealloc(buf, 2048)))
        {
            NDRX_LOG(log_error, "TESTERROR: failed "
                    "to realloc the buffer");
        }
        
    }
    
out:

    if (ret>=0)
        ret=EXSUCCEED;

    return ret;
}

