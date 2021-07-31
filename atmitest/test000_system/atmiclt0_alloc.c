/**
 * @brief ATMI allocation MT tests
 *
 * @file atmiclt0_alloc.c
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
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys_mqueue.h>
#include "test000.h"
#include <ndrstandard.h>
#include <ndebug.h>
#include <errno.h>
#include <exthpool.h>
#include <nstopwatch.h>
#include <atmi.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define POOL_SIZE   30
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * run allocation for 30 sec...
 * mainly monitored by asan
 */
void do_loop (void *ptr, int *p_finish_off)
{
    ndrx_stopwatch_t w;

    ndrx_stopwatch_reset(&w);
    while (ndrx_stopwatch_get_delta_sec(&w) < 30)
    {
        char *buf = tpalloc("UBF", NULL, 1024);
        buf = tprealloc(buf, 8192);
        buf = tprealloc(buf, 18192);
        buf = tprealloc(buf, 1024);
        tpfree(buf);
    }

    tpterm();
}
/**
 * Run allocator in the loop
 * @param argc
 * @param argv
 * @return 
 */
int main( int argc , char **argv )
{
    int ret = EXSUCCEED;

    int i;

    threadpool testpool;

    /* service request handlers */
    if (NULL==(testpool = ndrx_thpool_init(POOL_SIZE,
            NULL, NULL, NULL, 0, NULL)))
    {
        NDRX_LOG(log_error, "Failed to initialize thread pool (cnt: %d)!",
                POOL_SIZE);
        EXFAIL_OUT(ret);
    }

    for (i=0; i<POOL_SIZE; i++)
    {
        ndrx_thpool_add_work(testpool, (void *)do_loop, NULL);
    }

    while (POOL_SIZE!=ndrx_thpool_nr_not_working(testpool))
    {
        sleep(1);
    }

    /* wait for pool to complete... */
    ndrx_thpool_destroy(testpool);

out:
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */

