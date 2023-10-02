/**
 * @brief Verify time accurrancy of the tpext_addperiodcb() callbacks - client
 *
 * @file atmiclt106.c
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
#include "test106.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*#define DEVIATION_ALLOW 1 */
#define DEVIATION_ALLOW 3 /**< seems on aix some issues, with larger gaps */
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
    ndrx_stopwatch_t w;
    long l=0;
    int test_case;

    /* the accurracy of system-v set by NDRX_SCANUNIT is 10ms
     * thus running for 1 minute, we shall get around 60 calls (as 60*10 ms is still less than 1 second)
     */
    for (test_case=0; test_case<2; test_case++)
    {
        ndrx_stopwatch_reset(&w);

        NDRX_LOG(log_error, "START");
        while (ndrx_stopwatch_get_delta(&w) <= 60*1000)
        {
            if (EXFAIL == tpcall("TESTSV", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0))
            {
                NDRX_LOG(log_error, "TESTSV failed: %s", tpstrerror(tperrno));
                ret=EXFAIL;
                goto out;
            }

            if (0==test_case)
            {
                /* This shall make in non fixed versions to count
                 * get callbacks every 1.5 sec, thus totally 40.
                 * in fixed verison it must be 60.
                 */
                usleep(800*1000);
            }
            /* for test_case 1, ensure that callbackups work correctly at full load
             * of the service
             */
        }
        NDRX_LOG(log_error, "FINISH");

        /* read fresh results... */
        if (EXFAIL == tpcall("TESTSV", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0))
        {
            NDRX_LOG(log_error, "TESTSV failed: %s", tpstrerror(tperrno));
            ret=EXFAIL;
            goto out;
        }

        if (EXSUCCEED!=Bget(p_ub, T_LONG_FLD, 0, (char *)&l, 0L))
        {
            NDRX_LOG(log_error, "Failed to get T_LONG_FLD");
            ret=EXFAIL;
            goto out;
        }

        if (0==test_case)
        {
            /* Ensure that fix is working (+1 as during the run.sh there is sleep) */
            if (labs(l-60)> DEVIATION_ALLOW)
            {
                NDRX_LOG(log_error, "TESTERROR: Expected 60 +- %d callback calls, got %ld", 
                    DEVIATION_ALLOW, l);
                ret=EXFAIL;
                goto out;
            }
        }
        else
        {
            /* Ensure that fix is working (+1 as during the run.sh there is sleep) */
            if (labs(l-120)> DEVIATION_ALLOW)
            {
                NDRX_LOG(log_error, "TESTERROR: Expected 120 +- %d callback calls, got %ld", 
                    DEVIATION_ALLOW*2, l);
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

/* vim: set ts=4 sw=4 et smartindent: */
