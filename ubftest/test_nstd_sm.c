/**
 * @brief State Machine tests
 *
 * @file test_nstd_sm.c
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
#include <exsm.h>
#include <cgreen/cgreen.h>
#include <ndebug.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define NR_TRANS    5
/*---------------------------Enums--------------------------------------*/
enum
{
    st_entry
    , st_say_hello
    , st_go_home
    , st_go_work
    , st_count /* used for state index counting..., not functional */
};

enum
{
    ev_ok
    , ev_err
    , ev_timeout
    , ev_fatal
    , ev_normal
};

/*---------------------------Typedefs-----------------------------------*/
NDRX_SM_T(ndrx_sm_test_t, NR_TRANS);
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
exprivate int M_entry_called;
exprivate int M_say_hello_called;
exprivate int M_go_home_called;
exprivate int M_go_work_called;
/*---------------------------Prototypes---------------------------------*/

exprivate int entry(void *data);
exprivate int say_hello(void *data);
exprivate int go_home(void *data);
exprivate int go_work(void *data);

ndrx_sm_test_t M_sm1[st_count] = {

    NDRX_SM_STATE( st_entry, entry,
          NDRX_SM_TRAN      (ev_ok,         st_say_hello)
        , NDRX_SM_TRAN      (ev_err,        st_go_home)
        , NDRX_SM_TRAN      (ev_timeout,    st_go_work)
        , NDRX_SM_TRAN      (ev_normal,     NDRX_SM_ST_RETURN0)
        , NDRX_SM_TRAN_END
        )
    , NDRX_SM_STATE(st_say_hello, say_hello,
          NDRX_SM_TRAN      (ev_ok,         st_go_home)
        , NDRX_SM_TRAN      (ev_err,        st_go_work)
        , NDRX_SM_TRAN      (ev_timeout,    st_go_work)
        , NDRX_SM_TRAN_END
        )
    , NDRX_SM_STATE(st_go_home, go_home,
          NDRX_SM_TRAN      (ev_ok,         st_go_work)
        , NDRX_SM_TRAN      (ev_err,        st_go_work)
        , NDRX_SM_TRAN      (ev_timeout,    st_go_work)
        , NDRX_SM_TRAN_END
        )
    , NDRX_SM_STATE(st_go_work, go_work,
          NDRX_SM_TRAN      (ev_ok,         NDRX_SM_ST_RETURN)
        , NDRX_SM_TRAN      (ev_err,        NDRX_SM_ST_RETURN)
        , NDRX_SM_TRAN_END
        )
};

/**
 * last state is st_go_home and not st_go_work
 */
ndrx_sm_test_t M_sm2[st_count] = {

    NDRX_SM_STATE( st_entry, entry,
          NDRX_SM_TRAN      (ev_ok,          st_go_home)
        , NDRX_SM_TRAN      (ev_err,        st_go_home)
        , NDRX_SM_TRAN      (ev_timeout,    st_go_work)
        , NDRX_SM_TRAN      (ev_normal,     NDRX_SM_ST_RETURN0)
        , NDRX_SM_TRAN_END
        )
    , NDRX_SM_STATE(st_go_home, go_home,
          NDRX_SM_TRAN      (ev_ok,         st_go_work)
        , NDRX_SM_TRAN      (ev_err,        st_go_work)
        , NDRX_SM_TRAN      (ev_timeout,    st_go_work)
        , NDRX_SM_TRAN_END
        )
};

/* have strange event codes, use scanning*/
ndrx_sm_test_t M_sm3[st_count] = {

    NDRX_SM_STATE( st_entry, entry,
          NDRX_SM_TRAN      (-500,          st_go_home)
        , NDRX_SM_TRAN      (511,           st_go_home)
        , NDRX_SM_TRAN_END
        )
    , NDRX_SM_STATE(st_go_home, go_home,
          NDRX_SM_TRAN      (ev_ok,         st_go_work)
        , NDRX_SM_TRAN_END
        )
    , NDRX_SM_STATE(st_go_work, go_work,
          NDRX_SM_TRAN      (-500,         NDRX_SM_ST_RETURN)
        , NDRX_SM_TRAN      (511,         NDRX_SM_ST_RETURN)
        , NDRX_SM_TRAN_END
        )
};


/* missing state (out of range) */
ndrx_sm_test_t M_sm4[st_count] = {

    NDRX_SM_STATE( st_entry, entry,
          NDRX_SM_TRAN      (-500,          st_go_home)
        , NDRX_SM_TRAN      (511,           777)
        , NDRX_SM_TRAN_END
        )
    , NDRX_SM_STATE(st_go_home, go_home,
          NDRX_SM_TRAN      (ev_ok,         st_go_work)
        , NDRX_SM_TRAN_END
        )
};


exprivate void reset_counters(void)
{
    M_entry_called = 0;
    M_say_hello_called = 0;
    M_go_home_called = 0;
    M_go_work_called = 0;
}

exprivate int entry(void *data)
{
    M_entry_called++;
    int *case_no = (int *)data;
    return *case_no;
}

exprivate int say_hello(void *data)
{
    M_say_hello_called++;
    NDRX_LOG(log_info, "HELLO");
    return ev_timeout;
}

exprivate int go_home(void *data)
{
    M_go_home_called++;
    return ev_ok;
}

exprivate int go_work(void *data)
{
    M_go_work_called++;
    int *case_no =(int *)data;
    return *case_no;
}

Ensure(test_nstd_sm_test1)
{
    int data=ev_ok;
    reset_counters();

    /* compile the SM: */
    assert_equal(ndrx_sm_comp((void *)M_sm1, st_count, NR_TRANS, st_go_work), EXSUCCEED);

    assert_equal(ndrx_sm_run((void *)M_sm1, NR_TRANS, st_entry, (void *)&data), ev_ok);
    assert_equal(M_entry_called, 1);
    assert_equal(M_say_hello_called, 1);
    assert_equal(M_go_home_called, 0);
    assert_equal(M_go_work_called, 1);

    reset_counters();
    data=ev_err;
    assert_equal(ndrx_sm_run((void *)M_sm1, NR_TRANS, st_entry, (void *)&data), ev_err);
    assert_equal(M_entry_called, 1);
    assert_equal(M_say_hello_called, 0);
    assert_equal(M_go_home_called, 1);
    assert_equal(M_go_work_called, 1);

    /* transition not found: */
    reset_counters();
    data = ev_fatal;
    assert_equal(ndrx_sm_run((void *)M_sm1, NR_TRANS, st_entry, (void *)&data), EXFAIL);
    assert_equal(M_entry_called, 1);
    assert_equal(M_say_hello_called, 0);
    assert_equal(M_go_home_called, 0);
    assert_equal(M_go_work_called, 0);

    /* try different entry state */
    data=ev_ok;
    reset_counters();
    assert_equal(ndrx_sm_run((void *)M_sm1, NR_TRANS, st_go_home, (void *)&data), ev_ok);
    assert_equal(M_entry_called, 0);
    assert_equal(M_say_hello_called, 0);
    assert_equal(M_go_home_called, 1);
    assert_equal(M_go_work_called, 1);

    /* exit code 0 forced (not from event code) */
    data=ev_normal;
    reset_counters();
    assert_equal(ndrx_sm_run((void *)M_sm1, NR_TRANS, st_entry, (void *)&data), 0);
    assert_equal(M_entry_called, 1);
    assert_equal(M_say_hello_called, 0);
    assert_equal(M_go_home_called, 0);
    assert_equal(M_go_work_called, 0);
}

/**
 * Check SM validations
 */
Ensure(test_nstd_sm_validate)
{
    assert_equal(ndrx_sm_comp((void *)M_sm1, st_count, NR_TRANS, st_go_work), EXSUCCEED);
    /* missing state: */
    assert_equal(ndrx_sm_comp((void *)M_sm4, st_count, NR_TRANS, st_go_home), EXFAIL);
    /* missing EOF in transitions: */
    assert_equal(ndrx_sm_comp((void *)M_sm2, st_count, NR_TRANS, st_go_home), EXFAIL);
}

/**
 * Ensure non index events works
 */
Ensure(test_nstd_sm_scan_evs)
{
    int data;
    assert_equal(ndrx_sm_comp((void *)M_sm3, st_count, NR_TRANS, st_go_work), EXSUCCEED);

    data=-500;
    reset_counters();
    assert_equal(ndrx_sm_run((void *)M_sm3, NR_TRANS, st_entry, (void *)&data), -500);

    data=511;
    reset_counters();
    assert_equal(ndrx_sm_run((void *)M_sm3, NR_TRANS, st_entry, (void *)&data), 511);
}


/**
 * Standard library tests
 * @return
 */
TestSuite *ubf_nstd_sm(void)
{
    TestSuite *suite = create_test_suite();

    add_test(suite, test_nstd_sm_test1);
    add_test(suite, test_nstd_sm_validate);
    add_test(suite, test_nstd_sm_scan_evs);
    
    return suite;
}

/* vim: set ts=4 sw=4 et smartindent: */
