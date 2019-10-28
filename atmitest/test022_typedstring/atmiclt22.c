/**
 * @brief Typed STRING testing
 *
 * @file atmiclt22.c
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
#include "test022.h"

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
    char *buf = tpalloc("STRING", NULL, 30);
    
    if (NULL==buf)
    {
        NDRX_LOG(log_error, "TESTERROR: failed to alloc STRING buffer!");
        EXFAIL_OUT(ret);
    }

    for (j=0; j<10000; j++)
    {
        strcpy(buf, "HELLO WORLD");

        if (EXSUCCEED!=tpcall("TEST22_STRING", buf, 0, &buf, &rsplen, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: failed to call TEST22_STRING");
            EXFAIL_OUT(ret);
        }

        NDRX_LOG(log_debug, "Got message [%s] ", buf);

        /* Check the len */
        if (TEST_REPLY_SIZE!=strlen(buf))
        {
            NDRX_LOG(log_error, "TESTERROR: Invalid message len for [%s] got: %d, "
                    "but need: %d ", buf, strlen(buf), TEST_REPLY_SIZE);
        }

        for (i=0; i<TEST_REPLY_SIZE; i++)
        {
            if (((char)buf[i])!=((char)(i%255+1)))
            {
                NDRX_LOG(log_error, "TESTERROR: Invalid char at %d, "
                        "expected: 0x%1x, but got 0x%1x", i, 
                        (unsigned char)buf[i], (unsigned char)(i%255+1));
            }
        }

        if (EXEOS!=buf[TEST_REPLY_SIZE])
        {
            NDRX_LOG(log_error, "TESTERROR: Not EOS!");
        }

        strcpy(buf, "Hello Enduro/X from Mars");

        ret=tppost("TEST22EV", buf, 0L, TPSIGRSTRT);

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

/* vim: set ts=4 sw=4 et smartindent: */
