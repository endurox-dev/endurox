/* mtest6.c - memory-mapped database tester/toy 
 * Test for duplicate keys when doing cursor loop
 */
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

/* have custom sort func */
int sort_data(const EDB_val *a, const EDB_val *b)
{
    int ret = strcmp((char *)b->mv_data, (char *)a->mv_data);
    
    fprintf(stderr, "sort called [%s] vs [%s]=%d\n", (char *)a->mv_data, (char *)b->mv_data, ret);
    return ret;
}

Ensure(test_nstd_mtest6_dupcursor)
{
	int i = 0, j = 0, rc;
	EDB_env *env;
	EDB_dbi dbi;
	EDB_val key, data;
	EDB_txn *txn;
	EDB_stat mst;
	EDB_cursor *cursor;
	int count=100;
	char sval[32];
	char kval[sizeof(int)];

#define KEY_SIZE	4 /* 3 + EOS */

	memset(sval, 0, sizeof(sval));

	E(edb_env_create(&env));
	E(edb_env_set_mapsize(env, 10485760));
	E(edb_env_set_maxdbs(env, 4));
	E(edb_env_open(env, "./testdb", EDB_FIXEDMAP|EDB_NOSYNC, 0664));

	E(edb_txn_begin(env, NULL, 0, &txn));
        
        
	E(edb_dbi_open(txn, "id2", EDB_CREATE|EDB_DUPSORT, &dbi));
	E(edb_set_dupsort(txn, dbi, sort_data));
	/* drop database */
        E(edb_drop(txn, dbi, 0));
	
        
	key.mv_size = KEY_SIZE;
	key.mv_data = kval;
	data.mv_size = sizeof(sval);
	data.mv_data = sval;

	fprintf(stderr, "Adding %d values\n", count);
	for (i=0;i<count;i++) {
            sprintf(kval, "%03d", i);
            sprintf(sval, "%03d %03d foo bar", i, i);
            RES(0, edb_put(txn, dbi, &key, &data, 0L));
	}
        
        /* Add some random keys */
        i=5;
        sprintf(kval, "%03d", i);
        sprintf(sval, "%03d %03d foo bar", i, 12);
        RES(0, edb_put(txn, dbi, &key, &data, 0L));
        
        
        sprintf(sval, "%03d %03d foo bar", i, 10);
        RES(0, edb_put(txn, dbi, &key, &data, 0L));
                
                
        sprintf(sval, "%03d %03d foo bar", i, 13);
        RES(0, edb_put(txn, dbi, &key, &data, 0L));
                    
        i=16;
        sprintf(kval, "%03d", i);
        sprintf(sval, "%03d %03d foo bar", i, 17);
        RES(0, edb_put(txn, dbi, &key, &data, 0L));
        
        
        i=25;
        sprintf(kval, "%03d", i);
        sprintf(sval, "%03d %03d foo bar", i, 14);
        RES(0, edb_put(txn, dbi, &key, &data, 0L));
        
        
	E(edb_txn_commit(txn));
	E(edb_env_stat(env, &mst));

	E(edb_txn_begin(env, NULL, EDB_RDONLY, &txn));
        
    /* E(edb_set_dupsort(txn, dbi, sort_data)); */
        
        
	E(edb_cursor_open(txn, dbi, &cursor));
	while ((rc = edb_cursor_get(cursor, &key, &data, EDB_NEXT)) == 0) {
		fprintf(stderr, "key: %p %.*s, data: %p %.*s\n",
			key.mv_data,  (int) key.mv_size,  (char *) key.mv_data,
			data.mv_data, (int) data.mv_size, (char *) data.mv_data);
	}
        
	CHECK(rc == EDB_NOTFOUND, "edb_cursor_get");
        
        /* get stats... */
        E(edb_stat(txn, dbi, &mst));
        fprintf(stderr, "keys in db: %d\n", (int)mst.ms_entries);
        
        /* TODO: get first? */
        
        
	edb_cursor_close(cursor);
        
        E(edb_cursor_open(txn, dbi, &cursor));
        
        /* OK fetch the first rec of cursor, next records we shall kill (if any) */
        /* first: EDB_FIRST_DUP - this we accept and process */
        
        sprintf(kval, "%03d", 5);
        
        key.mv_size = KEY_SIZE;
	key.mv_data = kval;
        
        /* E(edb_get(txn, dbi, &key, &data)); */
        
        E(edb_cursor_get(cursor, &key, &data, EDB_SET_KEY));
        
        /* ok, test the data */
        if (0!=strcmp("005 013 foo bar", data.mv_data))
        {
            fprintf(stderr, "expected [005 013 foo bar] got [%s]\n", (char *)data.mv_data);
            abort();
        }
        
        /* now check the order.. of others... */
        
        E(edb_cursor_get(cursor, &key, &data, EDB_NEXT_DUP));
        
        if (0!=strcmp("005 012 foo bar", data.mv_data))
        {
            fprintf(stderr, "expected [005 012 foo bar] got [%s]\n", (char *)data.mv_data);
            abort();
        }
        
        E(edb_cursor_get(cursor, &key, &data, EDB_NEXT_DUP));
        
        if (0!=strcmp("005 010 foo bar", data.mv_data))
        {
            fprintf(stderr, "expected [005 010 foo bar] got [%s]\n", (char *)data.mv_data);
            abort();
        }
        
        RES(EDB_NOTFOUND, edb_cursor_get(cursor, &key, &data, EDB_NEXT_DUP));
        
	edb_txn_abort(txn);

	edb_dbi_close(env, dbi);
	edb_env_close(env);
	return;
}

/**
 * LMDB/EDB tests
 * @return
 */
    TestSuite *ubf_nstd_mtest6_dupcursor(void)
{
    TestSuite *suite = create_test_suite();

    add_test(suite, test_nstd_mtest6_dupcursor);
            
    return suite;
}


