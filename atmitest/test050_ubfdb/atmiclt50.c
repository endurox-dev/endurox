/* 
 * @brief UBF Field table database tests - client
 *
 * @file atmiclt50.c
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

char *M_ref_print[]= {
"T_HELLO_SHORT\t56\n",
"T_HELLO_LONG\t777777\n",
"T_HELLO_CHAR\tA\n",
"T_STRING_FLD\tABC\n",
"T_HELLO_STR\tHELLO WORLD\n",
NULL
};

char *M_ref_print_nofld[]= {
"((BFLDID32)3001)\t56\n",
"T_HELLO_LONG\t777777\n",
"T_HELLO_CHAR\tA\n",
"T_STRING_FLD\tABC\n",
NULL
};

/**
 * Bfprint testing.
 * @return
 */
exprivate void test_Bprint(UBFH *p_ub, char **ref_print)
{
    FILE *f=NULL;
    char filename[]="/tmp/ubf-test-XXXXXX";
    char readbuf[1024];
    int line_counter=0;

    assert_not_equal(mkstemp(filename), EXFAIL);
    assert_not_equal((f=fopen(filename, "w")), NULL);
    assert_equal(Bfprint(p_ub, f), EXSUCCEED);
    fclose(f);

    /* Re-open file in read mode end re-compare the buffer. */
    assert_not_equal((f=fopen(filename, "r")), NULL);

    /* compare the buffers */
    while(NULL!=fgets(readbuf, sizeof(readbuf), f))
    {
        if (NULL==ref_print[line_counter])
        {
            /* reached end of our knowledge of the test data */
            assert_equal_with_message(0, 1, "output file too big!");
            break;
        }
        assert_string_equal(ref_print[line_counter], readbuf);
        line_counter++;
    }
    fclose(f);

    /* remove the file */
    assert_equal(unlink(filename), EXSUCCEED);

    /* cannot print on null file */
    assert_equal(Bfprint(p_ub, NULL), EXFAIL);
    assert_equal(Berror, BEINVAL);
    
}


/**
 * Local testing of the UBF DB
 */
Ensure(ubfdb_local_tests)
{
    BFLDID fld;
    BFLDID fld1;
    BFLDID fld2;
    BFLDID fld3;
    EDB_txn *txn;
    UBFH *p_ub;
    char *tree = NULL;
    
    /* Unlink db firstly... */
    
    /* close any open... */
    Bflddbunload();
    
    assert_equal(Bflddbunlink(), EXSUCCEED);
    
    /* Load DB... (open handles, etc...) */
    assert_equal(Bflddbload(), EXTRUE);
    
    /* Check that fields are missing */
    assert_equal(Bflddbid("T_HELLO_STR"), BBADFLDID);
    /* error shall be set to bad field */
    assert_equal(Berror, BBADNAME);
    
    /* the same here */
    UBF_LOG(log_error, "YOPTEL 88888!!!!");
    assert_equal(Bfldid("T_HELLO_STR"), BBADFLDID);
    assert_equal(Berror, BBADNAME);
    
    /* Check by field id */
    fld = Bmkfldid (BFLD_STRING, 3000);
    assert_not_equal(fld, BBADFLDID);
    
    fld1 = Bmkfldid (BFLD_SHORT, 3001);
    assert_not_equal(fld1, BBADFLDID);
    
    fld2 = Bmkfldid (BFLD_CHAR, 3002);
    assert_not_equal(fld2, BBADFLDID);
    
    fld3 = Bmkfldid (BFLD_LONG, 3003);
    assert_not_equal(fld3, BBADFLDID);
    
    /* field must not exists in the db */
    
    /* Check that fields are missing */
    assert_equal(Bflddbname(fld), NULL);
    /* error shall be set to bad field */
    assert_equal(Berror, BBADFLD);
    
    /* the same here */
    assert_equal(Bfname(fld), BBADFLDID);
    assert_equal(Berror, BBADFLD);
    
    
    /* add some fields now */
    get_tran(&txn);

    UBF_LOG(log_error, "About to ADD custom fields!!!");
    assert_equal(Bflddbadd(txn, BFLD_STRING, 3000, "T_HELLO_STR"), EXSUCCEED);
    assert_equal(Bflddbadd(txn, BFLD_SHORT, 3001, "T_HELLO_SHORT"), EXSUCCEED);
    assert_equal(Bflddbadd(txn, BFLD_CHAR, 3002, "T_HELLO_CHAR"), EXSUCCEED);
    assert_equal(Bflddbadd(txn, BFLD_LONG, 3003, "T_HELLO_LONG"), EXSUCCEED);
    
    assert_equal(edb_txn_commit(txn), EXSUCCEED);
    
    
    assert_equal(Bfldid("T_HELLO_STR"), fld);
    assert_equal(Bfldid("T_HELLO_SHORT"), fld1);
    assert_equal(Bfldid("T_HELLO_CHAR"), fld2);
    assert_equal(Bfldid("T_HELLO_LONG"), fld3);
    
    
    assert_string_equal(Bfname(fld), "T_HELLO_STR");
    assert_string_equal(Bfname(fld1), "T_HELLO_SHORT");
    assert_string_equal(Bfname(fld2), "T_HELLO_CHAR");
    assert_string_equal(Bfname(fld3), "T_HELLO_LONG");
    
    /* run some boolean expression */
    
    UBF_LOG(log_error, "About to run boolean expression...");
    
    p_ub = (UBFH *)tpalloc("UBF", NULL, 56000);
    assert_not_equal(p_ub, NULL);
    assert_equal(CBchg(p_ub, Bfldid("T_HELLO_STR"), 0, "HELLO WORLD", 0L, BFLD_STRING), 
            EXSUCCEED);
    assert_equal(CBchg(p_ub, Bfldid("T_HELLO_SHORT"), 0, "56", 0L, BFLD_STRING), 
            EXSUCCEED);
    assert_equal(CBchg(p_ub, Bfldid("T_HELLO_CHAR"), 0, "A", 0L, BFLD_STRING),
            EXSUCCEED);
    assert_equal(CBchg(p_ub, Bfldid("T_HELLO_LONG"), 0, "777777", 0L, BFLD_STRING),
            EXSUCCEED);
    /* test standard field too */
    assert_equal(CBchg(p_ub, T_STRING_FLD, 0, "ABC", 0L, BFLD_STRING),
            EXSUCCEED);
    
    /* run some expression */
    tree=Bboolco ("T_HELLO_STR=='HELLO WORLD' && T_HELLO_SHORT==56 && "
            "T_HELLO_CHAR=='A' && T_HELLO_LONG==777777 && T_STRING_FLD=='ABC'");
    assert_not_equal(tree, NULL);
    assert_equal(Bboolev(p_ub, tree), EXTRUE);
    Btreefree(tree);
    
    /* Test buffer printing */
    test_Bprint(p_ub, M_ref_print);
    
    tpfree((char *)p_ub);    
}

/**
 * Test field delete
 */
Ensure(ubfdb_delete)
{
    BFLDID fld;
    BFLDID fld1;
    BFLDID fld2;
    BFLDID fld3;
    EDB_txn *txn;
    UBFH *p_ub;
    FILE *f=NULL;
    
    p_ub = (UBFH *)tpalloc("UBF", NULL, 56000);
    assert_not_equal(p_ub, NULL);
    
    /* Unlink db firstly... */
    Bflddbunload();
    assert_equal(Bflddbunlink(), EXSUCCEED);
    
    /* Load DB... (open handles, etc...) */
    assert_equal(Bflddbload(), EXTRUE);
    
    fld = Bmkfldid (BFLD_STRING, 3000);
    assert_not_equal(fld, BBADFLDID);
    
    fld1 = Bmkfldid (BFLD_SHORT, 3001);
    assert_not_equal(fld1, BBADFLDID);
    
    fld2 = Bmkfldid (BFLD_CHAR, 3002);
    assert_not_equal(fld2, BBADFLDID);
    
    fld3 = Bmkfldid (BFLD_LONG, 3003);
    assert_not_equal(fld3, BBADFLDID);
    
    
    /* add some fields now */
    get_tran(&txn);
    assert_equal(Bflddbadd(txn, BFLD_STRING, 3000, "T_HELLO_STR"), EXSUCCEED);
    assert_equal(Bflddbadd(txn, BFLD_SHORT, 3001, "T_HELLO_SHORT"), EXSUCCEED);
    assert_equal(Bflddbadd(txn, BFLD_CHAR, 3002, "T_HELLO_CHAR"), EXSUCCEED);
    assert_equal(Bflddbadd(txn, BFLD_LONG, 3003, "T_HELLO_LONG"), EXSUCCEED);
    assert_equal(edb_txn_commit(txn), EXSUCCEED);
    
    
    /* Test field that field is present */
    assert_string_equal(Bfname(fld), "T_HELLO_STR");
    assert_string_equal(Bfname(fld1), "T_HELLO_SHORT");
    assert_string_equal(Bfname(fld2), "T_HELLO_CHAR");
    assert_string_equal(Bfname(fld3), "T_HELLO_LONG");
    
    /* Load some data: */
    assert_equal(CBchg(p_ub, Bfldid("T_HELLO_SHORT"), 0, "56", 0L, BFLD_STRING), 
            EXSUCCEED);
    assert_equal(CBchg(p_ub, Bfldid("T_HELLO_CHAR"), 0, "A", 0L, BFLD_STRING),
            EXSUCCEED);
    assert_equal(CBchg(p_ub, Bfldid("T_HELLO_LONG"), 0, "777777", 0L, BFLD_STRING),
            EXSUCCEED);
    /* test standard field too */
    assert_equal(CBchg(p_ub, T_STRING_FLD, 0, "ABC", 0L, BFLD_STRING),
            EXSUCCEED);
    
    get_tran(&txn);
    assert_equal(Bflddbdel(txn, fld), EXSUCCEED);
    assert_equal(Bflddbdel(txn, fld1), EXSUCCEED);
    
    /* try to delete already removed fields, what will happen? */
    
    assert_equal(Bflddbdel(txn, fld), EXSUCCEED);
    assert_equal(Bflddbdel(txn, fld1), EXSUCCEED);
    
    assert_equal(edb_txn_commit(txn), EXSUCCEED);
    
    assert_string_equal(Bfname(fld), NULL);
    assert_equal(Berror, BBADFLD);
    
    assert_string_equal(Bfname(fld1), NULL);
    assert_equal(Berror, BBADFLD);
    
    assert_string_equal(Bfname(fld2), "T_HELLO_CHAR");
    assert_string_equal(Bfname(fld3), "T_HELLO_LONG");
    
    
    /* Test buffer printing */
    test_Bprint(p_ub, M_ref_print_nofld);
    
    /* try to parse back id... */
    /* assert_equal(Bfldid("((BFLDID32)3001)"), 3001); */
    f = fopen("./extfile", "r");
    assert_not_equal(f, NULL);
    
    assert_equal(Bextread(p_ub, f), EXSUCCEED);
    
    fclose(f);
    
    assert_equal(Bpres(p_ub, fld1, 0), EXTRUE);
    assert_equal(Bpres(p_ub, fld2, 0), EXTRUE);
    
    
    UBF_LOG(log_info, "drop the database now...");
    get_tran(&txn);
    assert_equal(Bflddbdrop(txn), EXSUCCEED);
    assert_equal(edb_txn_commit(txn), EXSUCCEED);
    
    
    
    assert_equal(Bfldid("T_HELLO_STR"), BBADFLDID);
    assert_equal(Bfldid("T_HELLO_SHORT"), BBADFLDID);
    assert_equal(Bfldid("T_HELLO_CHAR"), BBADFLDID);
    assert_equal(Bfldid("T_HELLO_LONG"), BBADFLDID);
    
    assert_string_equal(Bfname(fld), NULL);
    assert_string_equal(Bfname(fld1), NULL);
    assert_string_equal(Bfname(fld2), NULL);
    assert_string_equal(Bfname(fld3), NULL);
    
    /* unload finally */
    Bflddbunload();
    
    tpfree((char *)p_ub);    
}

/**
 * UBFDB testing suite
 * @return
 */
TestSuite *ubfdb_tests() 
{
    TestSuite *suite = create_test_suite();
    
    set_setup(suite, basic_setup);
    set_teardown(suite, basic_teardown);

    add_test(suite, ubfdb_local_tests);
    add_test(suite, ubfdb_delete);
    
    return suite;
}

/*
 * Main test entry.
 */
int main(int argc, char** argv)
{
    TestSuite *suite = create_test_suite();
    int ret;

    add_suite(suite, ubfdb_tests());
    
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
