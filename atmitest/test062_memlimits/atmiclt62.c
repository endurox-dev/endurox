/**
 * @brief Test memory limit restarts - client
 *
 * @file atmiclt62.c
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
#include "test62.h"
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
    int ret=EXSUCCEED;
    static char *buf = NULL;
    int i, chk=0;
        
    if (argc < 2)
    {
        NDRX_LOG(log_error, "Usage: %s srvrss|srvvsz|cltrss|cltvsz", argv[0]);
        EXFAIL_OUT(ret);
    }

    /* server tests */
    if (0==strcmp("srvrss", argv[1]))
    {
        if (EXFAIL == tpcall("RSSALLOC", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0))
        {
            NDRX_LOG(log_error, "RSSALLOC failed: %s", tpstrerror(tperrno));
            ret=EXFAIL;
            goto out;
        }
    }
    else if (0==strcmp("srvvsz", argv[1]))
    {
        if (EXFAIL == tpcall("VSZALLOC", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0))
        {
            NDRX_LOG(log_error, "VSZALLOC failed: %s", tpstrerror(tperrno));
            ret=EXFAIL;
            goto out;
        }
    }
    
    /* client tests: */
    else if (0==strcmp("cltrss", argv[1]))
    {
        /* let system to pick up the pid */
        sleep(10);
        if (NULL==buf)
        {
            buf = NDRX_MALLOC(RSS_BLOCK);

            if (NULL==buf)
            {
                NDRX_LOG(log_error, "TESTERROR ! Failed to alloc %d bytes: %s", 
                        RSS_BLOCK, strerror(errno));
                EXFAIL_OUT(ret);
            }
        }

        /* store some values there and trick out the kernel not to
         * figure out our strategy */
        for (i=0; i<RSS_BLOCK; i++)
        {
            buf[i] = i/3;
        }
	
        for (i=0; i<RSS_BLOCK; i++)
        {
            chk = chk+buf[i];
        }

	printf("chk: %d\n", chk);

        sleep(9999);
    }
    else if (0==strcmp("cltvsz", argv[1]))
    {
        /* let system to pick up the pid */
        sleep(10);
        buf = NDRX_MALLOC(VSZ_BLOCK);
        if (NULL==buf)
        {
            NDRX_LOG(log_error, "TESTERROR ! Failed to alloc %d bytes: %s", 
                    VSZ_BLOCK, strerror(errno));
            EXFAIL_OUT(ret);
        }
        sleep(9999);
    }
    else
    {
        NDRX_LOG(log_error, "Invalid test case: [%s]", argv[1]);
        EXFAIL_OUT(ret);
    }
out:
    tpterm();
    fprintf(stderr, "Exit with %d\n", ret);

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
