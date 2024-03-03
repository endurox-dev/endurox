/**
 * @brief Daemon thread tests
 *
 * @file test_nstd_dmn.c
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

/*---------------------------Includes-----------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <cgreen/cgreen.h>
#include <ubf.h>
#include <ndrstandard.h>
#include <string.h>
#include <ndebug.h>
#include <exbase64.h>
#include <nstdutil.h>
#include <fcntl.h>
#include "test.fd.h"
#include "ubfunit1.h"
#include "thlock.h"
#include <nstopwatch.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
exprivate ndrx_dmnthread_t M_t;
/*---------------------------Prototypes---------------------------------*/

void *start_routine(void *arg)
{
    ndrx_longptr_t ret=0;
    int sleep_ret;
    assert_equal(0x1234, (ndrx_longptr_t)arg);

    /* do the daemon loop */
    while (!(sleep_ret=ndrx_dmnthread_is_shutdown(&M_t)))
    {
        sleep_ret = ndrx_dmnthread_sleep(&M_t, 1000);
        assert_not_equal(sleep_ret, EXFAIL);
        if (EXSUCCEED==sleep_ret)
        {
            ret++;
        }
    }
    assert_equal(sleep_ret, EXTRUE);

    return (void*)ret;
}

/**
 * Verify that daemon read is terminated in time.
 */
Ensure(test_nstd_dmn_1)
{
    ndrx_stopwatch_t w;
    int i;
    long tdiff;
    ndrx_longptr_t ret;
    
    /* do in the loop, measure that we shutdown imediatelly,
     * always. And the loop count shall be something like 1 or 2
     */
    for (i=0; i<10; i++)
    {
        ndrx_dmnthread_init(&M_t, start_routine, (void *)0x1234);
        sleep(3);
        ndrx_stopwatch_reset(&w);
        ret=(ndrx_longptr_t)ndrx_dmnthread_shutdown(&M_t);
        tdiff=ndrx_stopwatch_get_delta(&w);
        NDRX_LOG(log_warn, "Shutdown returned: %ld (tdiff=%ld)", ret, tdiff);
        /* check that finished in time...
         * + 200ms on shutdown if max sleep was performed
         */
        assert_equal(tdiff<1200, EXTRUE);
        assert_equal(ret>1, EXTRUE);
        assert_equal(ret<5, EXTRUE);
    }
}

/**
 * Standard library tests
 * @return
 */
TestSuite *ubf_nstd_dmn(void)
{
    TestSuite *suite = create_test_suite();

    add_test(suite, test_nstd_dmn_1);
    
    return suite;
}
/* vim: set ts=4 sw=4 et smartindent: */
