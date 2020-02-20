/* mtest6.c - memory-mapped database tester/toy */
/*
 * Copyright 2011-2020 Howard Chu, Symas Corp.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */

/* Tests for DB splits and merges */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <cgreen/cgreen.h>
#include <ndebug.h>
#include <edbutil.h>
#include "exdb.h"

#define E(expr) CHECK((rc = (expr)) == EDB_SUCCESS, #expr)
#define RES(err, expr) ((rc = expr) == (err) || (CHECK(!rc, #expr), 0))
#define CHECK(test, msg) ((test) ? (void)0 : ((void)fprintf(stderr, \
	"%s:%d: %s: %s\n", __FILE__, __LINE__, msg, edb_strerror(rc)), abort()))

char dkbuf[1024];

Ensure(test_nstd_mtest6)
{
	int i = 0, j = 0, rc;
	EDB_env *env;
	EDB_dbi dbi;
	EDB_val key, data, sdata;
	EDB_txn *txn;
	EDB_stat mst;
	EDB_cursor *cursor;
	int count;
	int *values;
	long kval;
	char *sval;
        char errdet[PATH_MAX];

	srand(time(NULL));
        

        E(ndrx_mdb_unlink("./testdb", errdet, sizeof(errdet), 
            LOG_FACILITY_UBF));

	E(edb_env_create(&env));
	E(edb_env_set_mapsize(env, 10485760));
	E(edb_env_set_maxdbs(env, 4));
	E(edb_env_open(env, "./testdb", EDB_FIXEDMAP|EDB_NOSYNC, 0664));

	E(edb_txn_begin(env, NULL, 0, &txn));
	E(edb_dbi_open(txn, "id6", EDB_CREATE|EDB_INTEGERKEY, &dbi));
	E(edb_cursor_open(txn, dbi, &cursor));
	E(edb_stat(txn, dbi, &mst));

	sval = calloc(1, mst.ms_psize / 4);
	key.mv_size = sizeof(long);
	key.mv_data = &kval;
	sdata.mv_size = mst.ms_psize / 4 - 30;
	sdata.mv_data = sval;

	fprintf(stderr, "Adding 12 values, should yield 3 splits\n");
	for (i=0;i<12;i++) {
		kval = i*5;
		sprintf(sval, "%08lx", kval);
		data = sdata;
		(void)RES(EDB_KEYEXIST, edb_cursor_put(cursor, &key, &data, EDB_NOOVERWRITE));
	}
	fprintf(stderr, "Adding 12 more values, should yield 3 splits\n");
	for (i=0;i<12;i++) {
		kval = i*5+4;
		sprintf(sval, "%08lx", kval);
		data = sdata;
		(void)RES(EDB_KEYEXIST, edb_cursor_put(cursor, &key, &data, EDB_NOOVERWRITE));
	}
	fprintf(stderr, "Adding 12 more values, should yield 3 splits\n");
	for (i=0;i<12;i++) {
		kval = i*5+1;
		sprintf(sval, "%08lx", kval);
		data = sdata;
		(void)RES(EDB_KEYEXIST, edb_cursor_put(cursor, &key, &data, EDB_NOOVERWRITE));
	}
	E(edb_cursor_get(cursor, &key, &data, EDB_FIRST));

	do {
		printf("key: %.*s, data: %.*s\n",
			(int) key.mv_size,  (char *) key.mv_data,
			(int) data.mv_size, (char *) data.mv_data);
	} while ((rc = edb_cursor_get(cursor, &key, &data, EDB_NEXT)) == 0);
	CHECK(rc == EDB_NOTFOUND, "edb_cursor_get");
	edb_cursor_close(cursor);
	edb_txn_commit(txn);

	edb_env_close(env);

	return;
}


/**
 * LMDB/EDB tests
 * @return
 */
TestSuite *ubf_nstd_mtest6(void)
{
    TestSuite *suite = create_test_suite();

    add_test(suite, test_nstd_mtest6);
            
    return suite;
}

