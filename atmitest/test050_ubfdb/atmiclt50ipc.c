/* 
** UBF Field table database tests - client, IPC part
**
** @file atmiclt50ipc.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, Mavimax, Ltd. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or Mavimax's license for commercial use.
** -----------------------------------------------------------------------------
** GPL license:
** 
** This program is free software; you can redistribute it and/or modify it under
** the terms of the GNU General Public License as published by the Free Software
** Foundation; either version 2 of the License, or (at your option) any later
** version.
**
** This program is distributed in the hope that it will be useful, but WITHOUT ANY
** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
** PARTICULAR PURPOSE. See the GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License along with
** this program; if not, write to the Free Software Foundation, Inc., 59 Temple
** Place, Suite 330, Boston, MA 02111-1307 USA
**
** -----------------------------------------------------------------------------
** A commercial use license is available from Mavimax, Ltd
** contact@mavimax.com
** -----------------------------------------------------------------------------
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
#include <cgreen/cgreen.h>
#include "test50.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/


/**
 * Basic preparation before the test
 */
exprivate void basic_setup(void)
{

}

exprivate void basic_teardown(void)
{

}

/**
 * Get transaction object..
 * @param txn
 */
exprivate void get_tran(EDB_txn **txn)
{
    EDB_env * dbenv = NULL;
    EDB_dbi *dbi_id = NULL, *dbi_nm = NULL;
    
    /* get DB Env */
    dbenv = Bfldddbgetenv(&dbi_id, &dbi_nm);
    
    assert_not_equal(dbenv, NULL);
    assert_not_equal(dbi_id, NULL);
    assert_not_equal(dbi_nm, NULL);
    
    /* begin transaction */
    assert_equal(edb_txn_begin(dbenv, NULL, 0, txn), EXSUCCEED);
}

/**
 * Local testing of the UBF DB
 */
Ensure(ubfdb_setup)
{
    EDB_txn *txn;
    
    /* Unlink db firstly... */
    assert_equal(Bflddbunlink(), EXSUCCEED);
    
    /* Load DB... (open handles, etc...) */
    assert_equal(Bflddbload(), EXTRUE);
    
    /* add some fields now */
    get_tran(&txn);

    UBF_LOG(log_error, "About to ADD custom fields!!!");
    assert_equal(Bflddbadd(txn, BFLD_STRING, 3000, "T_HELLO_STR"), EXSUCCEED);
    assert_equal(Bflddbadd(txn, BFLD_SHORT, 3001, "T_HELLO_SHORT"), EXSUCCEED);
    assert_equal(Bflddbadd(txn, BFLD_CHAR, 3002, "T_HELLO_CHAR"), EXSUCCEED);
    assert_equal(Bflddbadd(txn, BFLD_LONG, 3003, "T_HELLO_LONG"), EXSUCCEED);
    
    assert_equal(edb_txn_commit(txn), EXSUCCEED);
}

/**
 * Call server. This assumes that custom tables are already loaded
 */
Ensure(ubfdb_callsv)
{
    UBFH *p_ub;
    long l;
    long rsplen;
    
    for (l=0; l<100; l++)
    {
        p_ub = (UBFH *)tpalloc("UBF", NULL, 56000);
        assert_not_equal(p_ub, NULL);
        assert_equal(CBchg(p_ub, Bfldid("T_HELLO_STR"), 0, "HELLO WORLD", 0L, BFLD_STRING), 
                EXSUCCEED);
        assert_equal(CBchg(p_ub, Bfldid("T_HELLO_SHORT"), 0, "56", 0L, BFLD_STRING), 
                EXSUCCEED);
        assert_equal(CBchg(p_ub, Bfldid("T_HELLO_CHAR"), 0, "A", 0L, BFLD_STRING),
                EXSUCCEED);
        assert_equal(Bchg(p_ub, Bfldid("T_HELLO_LONG"), 0, (char *)&l, 0L),
                EXSUCCEED);
        /* test standard field too */
        assert_equal(CBchg(p_ub, T_STRING_FLD, 0, "ABC", 0L, BFLD_STRING),
                EXSUCCEED);
        
        assert_equal(tpcall("TESTSV", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0), 
                EXSUCCEED);
        
        tpfree((char *)p_ub);
    }
}

/**
 * UBFDB testing suite
 * @return
 */
TestSuite *ubfdb_ipc_tests() 
{
    TestSuite *suite = create_test_suite();
    
    set_setup(suite, basic_setup);
    set_teardown(suite, basic_teardown);

    add_test(suite, ubfdb_setup);
    add_test(suite, ubfdb_callsv);
    
    return suite;
}

/*
 * Main test entry.
 */
int main(int argc, char** argv)
{
    TestSuite *suite = create_test_suite();
    int ret;

    add_suite(suite, ubfdb_ipc_tests());
    
    if (argc > 1)
    {
        ret=run_single_test(suite,argv[1],create_text_reporter());
    }
    else
    {
        ret=run_test_suite(suite, create_text_reporter());
    }

    destroy_test_suite(suite);

    return ret;
}
