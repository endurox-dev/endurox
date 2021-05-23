/**
 * @brief Test read only transaction slots
 *
 * @file test_nstd_mtest7.c
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
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <cgreen/cgreen.h>
#include <ndebug.h>
#include <edbutil.h>
#include <signal.h>
#include "exdb.h"
#include "ndebug.h"

#define E(expr) CHECK((rc = (expr)) == EDB_SUCCESS, #expr)
#define RES(err, expr) ((rc = expr) == (err) || (CHECK(!rc, #expr), 0))
#define CHECK(test, msg) ((test) ? (void)0 : ((void)fprintf(stderr, \
	"%s:%d: %s: %s\n", __FILE__, __LINE__, msg, edb_strerror(rc)), abort()))

Ensure(test_nstd_mtest7)
{
    int rc;
    EDB_env *env;
    EDB_txn *txn;
    char errdet[PATH_MAX];
    pid_t pid;
    
    signal(SIGCHLD, SIG_IGN);
    
    E(ndrx_mdb_unlink("./testdb", errdet, sizeof(errdet), 
            LOG_FACILITY_UBF));
   
    if (0==(pid=fork()))
    {
        E(edb_env_create(&env));
        E(edb_env_set_maxreaders(env, 1));
        E(edb_env_set_mapsize(env, 10485760));
        E(edb_env_open(env, "./testdb", 0 /*|EDB_NOSYNC*/, 0664));
        E(edb_txn_begin(env, NULL, EDB_RDONLY, &txn));
        sleep(9999);
    }
    
    sleep(2);
    
    E(edb_env_create(&env));
    E(edb_env_set_maxreaders(env, 1));
    E(edb_env_set_mapsize(env, 10485760));
    E(edb_env_open(env, "./testdb", 0 /*|EDB_NOSYNC*/, 0664));
    
    assert_equal(edb_txn_begin(env, NULL, EDB_RDONLY, &txn), EDB_READERS_FULL);
    assert_equal(kill(pid, SIGKILL), EXSUCCEED);
    
    assert_equal(edb_txn_begin(env, NULL, 0, &txn), EDB_SUCCESS);
    
    edb_txn_abort(txn);
    edb_env_close(env);

    return;
}


/**
 * Test for read-only transaction capacity recovery
 * @return
 */
TestSuite *ubf_nstd_mtest7(void)
{
    TestSuite *suite = create_test_suite();

    add_test(suite, test_nstd_mtest7);
            
    return suite;
}

/* vim: set ts=4 sw=4 et smartindent: */
