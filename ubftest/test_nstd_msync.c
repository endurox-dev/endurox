/* test_nstd_msync.c - parallel writes & locking test */
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

/* Just like mtest.c, but using a subDB instead of the main DB */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "exdb.h"

#define E(expr) CHECK((rc = (expr)) == EDB_SUCCESS, #expr)
#define RES(err, expr) ((rc = expr) == (err) || (CHECK(!rc, #expr), 0))
#define CHECK(test, msg) ((test) ? (void)0 : ((void)fprintf(stderr, \
	"%s:%d: %s: %s\n", __FILE__, __LINE__, msg, edb_strerror(rc)), abort()))

int main(int argc, char **argv)
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
	char sval[32] = "";
    int sleeps;

    if (argc < 2)
    {
        fprintf(stderr, "usage: %s <sync_seconds_sleeps> [R]\n where R - start with RO tran\n", argv[0]);
        exit(1);
    }
    sleeps = atoi(argv[1]);

	srand(time(NULL));

	count = (rand()%384) + 64;
	values = (int *)malloc(count*sizeof(int));

	for(i = 0;i<count;i++) {
		values[i] = rand()%1024;
	}

	E(edb_env_create(&env));
	E(edb_env_set_maxreaders(env, 2));
	E(edb_env_set_mapsize(env, 10485760));
	E(edb_env_set_maxdbs(env, 4));
	E(edb_env_open(env, "./testdb", 0, 0664));

    if (2==argc)
    {
	fprintf(stderr, "About to begin RW....\n");
	E(edb_txn_begin(env, NULL, 0, &txn));
	fprintf(stderr, "After begin RW....\n");
	E(edb_dbi_open(txn, "id1", EDB_CREATE, &dbi));
    }
    else
    {
	fprintf(stderr, "About to begin RO (env)....\n");
	E(edb_txn_begin(env, NULL, EDB_RDONLY, &txn));
	fprintf(stderr, "After begin RO (env)....\n");
	E(edb_dbi_open(txn, "id1", 0, &dbi));
    }

	fprintf(stderr, "About to commit (env)....\n");
	E(edb_txn_commit(txn));
	fprintf(stderr, "About commit (env)....\n");
    
   
	key.mv_size = sizeof(int);
	key.mv_data = sval;

    /* if argc > 2 then skip the first phase, and run directly to 
     * to read only tran
     */
    if (2==argc)
    {
	fprintf(stderr, "About to begin RW....\n");
	E(edb_txn_begin(env, NULL, 0, &txn));
	fprintf(stderr, "After begin RW....\n");
	fprintf(stderr, "Adding %d values -> about to sleep (2) %d sec\n", count, sleeps);
    sleep(atoi(argv[1]));

	fprintf(stderr, "COUNT=%d, After sleep (1) %d sec\n", count, sleeps);
	for (i=0;i<count;i++) {	
		sprintf(sval, "%03x %d foo bar %d", values[i], values[i], sleeps);
		data.mv_size = sizeof(sval);
		data.mv_data = sval;
		if (RES(EDB_KEYEXIST, edb_put(txn, dbi, &key, &data, EDB_NOOVERWRITE)))
			j++;
	}

	if (j) fprintf(stderr, "%d duplicates skipped\n", j);
    
	fprintf(stderr, "About to begin RW COMMIT....\n");
	E(edb_txn_commit(txn));
	fprintf(stderr, "After RW COMMIT....\n");
    }

	E(edb_env_stat(env, &mst));

    
	fprintf(stderr, "About to begin RO TRAN....\n");
	E(edb_txn_begin(env, NULL, EDB_RDONLY, &txn));
	fprintf(stderr, "After RO TRAN....\n");

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
	key.mv_data = sval;
	for (i= count - 1; i > -1; i-= (rand()%5)) {
		j++;
		txn=NULL;

	    fprintf(stderr, "About to begin RW TRAN....\n");
		E(edb_txn_begin(env, NULL, 0, &txn));
	    fprintf(stderr, "After begin RW TRAN....\n");

		sprintf(sval, "%03x ", values[i]);
		if (RES(EDB_NOTFOUND, edb_del(txn, dbi, &key, NULL))) {
			j--;
	        fprintf(stderr, "About to ABORT RW TRAN....\n");
			edb_txn_abort(txn);
	        fprintf(stderr, "After ABORT RW TRAN....\n");
		} else {
	        fprintf(stderr, "About to COMMIT RW TRAN....\n");
			E(edb_txn_commit(txn));
	        fprintf(stderr, "After COMMIT RW TRAN....\n");
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
	return 0;
}

