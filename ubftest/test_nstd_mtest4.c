/* mtest4.c - memory-mapped database tester/toy */
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

/* Tests for sorted duplicate DBs with fixed-size keys */
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

Ensure(test_nstd_mtest4)
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
	char sval[8];
	char kval[sizeof(int)];

	memset(sval, 0, sizeof(sval));

	count = 510;
	values = (int *)malloc(count*sizeof(int));

	for(i = 0;i<count;i++) {
		values[i] = i*5;
	}

	E(edb_env_create(&env));
	E(edb_env_set_mapsize(env, 10485760));
	E(edb_env_set_maxdbs(env, 4));
	E(edb_env_open(env, "./testdb", EDB_FIXEDMAP|EDB_NOSYNC, 0664));

	E(edb_txn_begin(env, NULL, 0, &txn));
	E(edb_dbi_open(txn, "id4", EDB_CREATE|EDB_DUPSORT|EDB_DUPFIXED, &dbi));
        E(edb_drop(txn, dbi, 0));

	key.mv_size = sizeof(int);
	key.mv_data = kval;
	data.mv_size = sizeof(sval);
	data.mv_data = sval;

	printf("Adding %d values\n", count);
	strcpy(kval, "001");
	for (i=0;i<count;i++) {
		sprintf(sval, "%07x", values[i]);
		if (RES(EDB_KEYEXIST, edb_put(txn, dbi, &key, &data, EDB_NODUPDATA)))
			j++;
	}
	if (j) printf("%d duplicates skipped\n", j);
	E(edb_txn_commit(txn));
	E(edb_env_stat(env, &mst));

	/* there should be one full page of dups now.
	 */
	E(edb_txn_begin(env, NULL, EDB_RDONLY, &txn));
	E(edb_cursor_open(txn, dbi, &cursor));
	while ((rc = edb_cursor_get(cursor, &key, &data, EDB_NEXT)) == 0) {
		printf("key: %p %.*s, data: %p %.*s\n",
			key.mv_data,  (int) key.mv_size,  (char *) key.mv_data,
			data.mv_data, (int) data.mv_size, (char *) data.mv_data);
	}
	CHECK(rc == EDB_NOTFOUND, "edb_cursor_get");
	edb_cursor_close(cursor);
	edb_txn_abort(txn);

	/* test all 3 branches of split code:
	 * 1: new key in lower half
	 * 2: new key at split point
	 * 3: new key in upper half
	 */

	key.mv_size = sizeof(int);
	key.mv_data = kval;
	data.mv_size = sizeof(sval);
	data.mv_data = sval;

	sprintf(sval, "%07x", values[3]+1);
	E(edb_txn_begin(env, NULL, 0, &txn));
	(void)RES(EDB_KEYEXIST, edb_put(txn, dbi, &key, &data, EDB_NODUPDATA));
	edb_txn_abort(txn);

	sprintf(sval, "%07x", values[255]+1);
	E(edb_txn_begin(env, NULL, 0, &txn));
	(void)RES(EDB_KEYEXIST, edb_put(txn, dbi, &key, &data, EDB_NODUPDATA));
	edb_txn_abort(txn);

	sprintf(sval, "%07x", values[500]+1);
	E(edb_txn_begin(env, NULL, 0, &txn));
	(void)RES(EDB_KEYEXIST, edb_put(txn, dbi, &key, &data, EDB_NODUPDATA));
	E(edb_txn_commit(txn));

	/* Try EDB_NEXT_MULTIPLE */
	E(edb_txn_begin(env, NULL, 0, &txn));
	E(edb_cursor_open(txn, dbi, &cursor));
	while ((rc = edb_cursor_get(cursor, &key, &data, EDB_NEXT_MULTIPLE)) == 0) {
		printf("key: %.*s, data: %.*s\n",
			(int) key.mv_size,  (char *) key.mv_data,
			(int) data.mv_size, (char *) data.mv_data);
	}
	CHECK(rc == EDB_NOTFOUND, "edb_cursor_get");
	edb_cursor_close(cursor);
	edb_txn_abort(txn);
	j=0;

	for (i= count - 1; i > -1; i-= (rand()%3)) {
		j++;
		txn=NULL;
		E(edb_txn_begin(env, NULL, 0, &txn));
		sprintf(sval, "%07x", values[i]);
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
	printf("Deleted %d values\n", j);

	E(edb_env_stat(env, &mst));
	E(edb_txn_begin(env, NULL, EDB_RDONLY, &txn));
	E(edb_cursor_open(txn, dbi, &cursor));
	printf("Cursor next\n");
	while ((rc = edb_cursor_get(cursor, &key, &data, EDB_NEXT)) == 0) {
		printf("key: %.*s, data: %.*s\n",
			(int) key.mv_size,  (char *) key.mv_data,
			(int) data.mv_size, (char *) data.mv_data);
	}
	CHECK(rc == EDB_NOTFOUND, "edb_cursor_get");
	printf("Cursor prev\n");
	while ((rc = edb_cursor_get(cursor, &key, &data, EDB_PREV)) == 0) {
		printf("key: %.*s, data: %.*s\n",
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
TestSuite *ubf_nstd_mtest4(void)
{
    TestSuite *suite = create_test_suite();

    add_test(suite, test_nstd_mtest4);
            
    return suite;
}

        
