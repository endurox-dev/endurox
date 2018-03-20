/* mtest3.c - memory-mapped database tester/toy */
/*
 * Copyright 2011-2017 Howard Chu, Symas Corp.
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

/* Tests for sorted duplicate DBs */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <cgreen/cgreen.h>
#include "exdb.h"

#define E(expr) CHECK((rc = (expr)) == EDB_SUCCESS, #expr)
#define RES(err, expr) ((rc = expr) == (err) || (CHECK(!rc, #expr), 0))
#define CHECK(test, msg) ((test) ? (void)0 : ((void)fprintf(stderr, \
	"%s:%d: %s: %s\n", __FILE__, __LINE__, msg, edb_strerror(rc)), abort()))

Ensure(test_nstd_mtest3)
{
	int i = 0, j = 0, rc;
	EDB_env *env;
	EDB_dbi dbi;
	EDB_val key, data;
	EDB_txn *txn;
	EDB_stat mst;
	EDB_cursor *cursor;
	int count;
	int *values;
	char sval[32];
	char kval[sizeof(int)];

	srand(time(NULL));

	memset(sval, 0, sizeof(sval));

	count = (rand()%384) + 64;
	values = (int *)malloc(count*sizeof(int));

	for(i = 0;i<count;i++) {
		values[i] = rand()%1024;
	}

	E(edb_env_create(&env));
	E(edb_env_set_mapsize(env, 10485760));
	E(edb_env_set_maxdbs(env, 4));
	E(edb_env_open(env, "./testdb", EDB_FIXEDMAP|EDB_NOSYNC, 0664));

	E(edb_txn_begin(env, NULL, 0, &txn));
	E(edb_dbi_open(txn, "id2", EDB_CREATE|EDB_DUPSORT, &dbi));
        E(edb_drop(txn, dbi, 0));

	key.mv_size = sizeof(int);
	key.mv_data = kval;
	data.mv_size = sizeof(sval);
	data.mv_data = sval;

	fprintf(stderr, "Adding %d values\n", count);
	for (i=0;i<count;i++) {
		if (!(i & 0x0f))
			sprintf(kval, "%03x", values[i]);
		sprintf(sval, "%03x %d foo bar", values[i], values[i]);
		if (RES(EDB_KEYEXIST, edb_put(txn, dbi, &key, &data, EDB_NODUPDATA)))
			j++;
	}
	if (j) fprintf(stderr, "%d duplicates skipped\n", j);
	E(edb_txn_commit(txn));
	E(edb_env_stat(env, &mst));

	E(edb_txn_begin(env, NULL, EDB_RDONLY, &txn));
	E(edb_cursor_open(txn, dbi, &cursor));
	while ((rc = edb_cursor_get(cursor, &key, &data, EDB_NEXT)) == 0) {
		fprintf(stderr, "key: %p %.*s, data: %p %.*s\n",
			key.mv_data,  (int) key.mv_size,  (char *) key.mv_data,
			data.mv_data, (int) data.mv_size, (char *) data.mv_data);
	}
	CHECK(rc == EDB_NOTFOUND, "edb_cursor_get");
	edb_cursor_close(cursor);
	edb_txn_abort(txn);

	j=0;

	for (i= count - 1; i > -1; i-= (rand()%5)) {
		j++;
		txn=NULL;
		E(edb_txn_begin(env, NULL, 0, &txn));
		sprintf(kval, "%03x", values[i & ~0x0f]);
		sprintf(sval, "%03x %d foo bar", values[i], values[i]);
		key.mv_size = sizeof(int);
		key.mv_data = kval;
		data.mv_size = sizeof(sval);
		data.mv_data = sval;
		if (RES(EDB_NOTFOUND, edb_del(txn, dbi, &key, &data))) {
			j--;
			edb_txn_abort(txn);
		} else {
			E(edb_txn_commit(txn));
		}
	}
	free(values);
	fprintf(stderr, "Deleted %d values\n", j);

	E(edb_env_stat(env, &mst));
	E(edb_txn_begin(env, NULL, EDB_RDONLY, &txn));
	E(edb_cursor_open(txn, dbi, &cursor));
	fprintf(stderr, "Cursor next\n");
	while ((rc = edb_cursor_get(cursor, &key, &data, EDB_NEXT)) == 0) {
		fprintf(stderr, "key: %.*s, data: %.*s\n",
			(int) key.mv_size,  (char *) key.mv_data,
			(int) data.mv_size, (char *) data.mv_data);
	}
	CHECK(rc == EDB_NOTFOUND, "edb_cursor_get");
	fprintf(stderr, "Cursor prev\n");
	while ((rc = edb_cursor_get(cursor, &key, &data, EDB_PREV)) == 0) {
		fprintf(stderr, "key: %.*s, data: %.*s\n",
			(int) key.mv_size,  (char *) key.mv_data,
			(int) data.mv_size, (char *) data.mv_data);
	}
	CHECK(rc == EDB_NOTFOUND, "edb_cursor_get");
	edb_cursor_close(cursor);
	edb_txn_abort(txn);

	edb_dbi_close(env, dbi);
	edb_env_close(env);
	return;
}

/**
 * LMDB/EDB tests
 * @return
 */
TestSuite *ubf_nstd_mtest3(void)
{
    TestSuite *suite = create_test_suite();

    add_test(suite, test_nstd_mtest3);
            
    return suite;
}


