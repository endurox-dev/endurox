/* edb_stat.c - memory-mapped database status tool */
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "exdb.h"

#define Z	EDB_FMT_Z
#define Yu	EDB_PRIy(u)

static void prstat(EDB_stat *ms)
{
#if 0
	printf("  Page size: %u\n", ms->ms_psize);
#endif
	printf("  Tree depth: %u\n", ms->ms_depth);
	printf("  Branch pages: %"Yu"\n",   ms->ms_branch_pages);
	printf("  Leaf pages: %"Yu"\n",     ms->ms_leaf_pages);
	printf("  Overflow pages: %"Yu"\n", ms->ms_overflow_pages);
	printf("  Entries: %"Yu"\n",        ms->ms_entries);
}

static void usage(char *prog)
{
	fprintf(stderr, "usage: %s [-V] [-n] [-e] [-r[r]] [-f[f[f]]] [-a|-s subdb] dbpath\n", prog);
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
	int i, rc;
	EDB_env *env;
	EDB_txn *txn;
	EDB_dbi dbi;
	EDB_stat mst;
	EDB_envinfo mei;
	char *prog = argv[0];
	char *envname;
	char *subname = NULL;
	int alldbs = 0, envinfo = 0, envflags = 0, freinfo = 0, rdrinfo = 0;

	if (argc < 2) {
		usage(prog);
	}

	/* -a: print stat of main DB and all subDBs
	 * -s: print stat of only the named subDB
	 * -e: print env info
	 * -f: print freelist info
	 * -r: print reader info
	 * -n: use NOSUBDIR flag on env_open
	 * -V: print version and exit
	 * (default) print stat of only the main DB
	 */
	while ((i = getopt(argc, argv, "Vaefnrs:")) != EOF) {
		switch(i) {
		case 'V':
			printf("%s\n", EDB_VERSION_STRING);
			exit(0);
			break;
		case 'a':
			if (subname)
				usage(prog);
			alldbs++;
			break;
		case 'e':
			envinfo++;
			break;
		case 'f':
			freinfo++;
			break;
		case 'n':
			envflags |= EDB_NOSUBDIR;
			break;
		case 'r':
			rdrinfo++;
			break;
		case 's':
			if (alldbs)
				usage(prog);
			subname = optarg;
			break;
		default:
			usage(prog);
		}
	}

	if (optind != argc - 1)
		usage(prog);

	envname = argv[optind];
	rc = edb_env_create(&env);
	if (rc) {
		fprintf(stderr, "edb_env_create failed, error %d %s\n", rc, edb_strerror(rc));
		return EXIT_FAILURE;
	}

	if (alldbs || subname) {
		edb_env_set_maxdbs(env, 4);
	}

	rc = edb_env_open(env, envname, envflags | EDB_RDONLY, 0664);
	if (rc) {
		fprintf(stderr, "edb_env_open failed, error %d %s\n", rc, edb_strerror(rc));
		goto env_close;
	}

	if (envinfo) {
		(void)edb_env_stat(env, &mst);
		(void)edb_env_info(env, &mei);
		printf("Environment Info\n");
		printf("  Map address: %p\n", mei.me_mapaddr);
		printf("  Map size: %"Yu"\n", mei.me_mapsize);
		printf("  Page size: %u\n", mst.ms_psize);
		printf("  Max pages: %"Yu"\n", mei.me_mapsize / mst.ms_psize);
		printf("  Number of pages used: %"Yu"\n", mei.me_last_pgno+1);
		printf("  Last transaction ID: %"Yu"\n", mei.me_last_txnid);
		printf("  Max readers: %u\n", mei.me_maxreaders);
		printf("  Number of readers used: %u\n", mei.me_numreaders);
	}

	if (rdrinfo) {
		printf("Reader Table Status\n");
		rc = edb_reader_list(env, (EDB_msg_func *)fputs, stdout);
		if (rdrinfo > 1) {
			int dead;
			edb_reader_check(env, &dead);
			printf("  %d stale readers cleared.\n", dead);
			rc = edb_reader_list(env, (EDB_msg_func *)fputs, stdout);
		}
		if (!(subname || alldbs || freinfo))
			goto env_close;
	}

	rc = edb_txn_begin(env, NULL, EDB_RDONLY, &txn);
	if (rc) {
		fprintf(stderr, "edb_txn_begin failed, error %d %s\n", rc, edb_strerror(rc));
		goto env_close;
	}

	if (freinfo) {
		EDB_cursor *cursor;
		EDB_val key, data;
		edb_size_t pages = 0, *iptr;

		printf("Freelist Status\n");
		dbi = 0;
		rc = edb_cursor_open(txn, dbi, &cursor);
		if (rc) {
			fprintf(stderr, "edb_cursor_open failed, error %d %s\n", rc, edb_strerror(rc));
			goto txn_abort;
		}
		rc = edb_stat(txn, dbi, &mst);
		if (rc) {
			fprintf(stderr, "edb_stat failed, error %d %s\n", rc, edb_strerror(rc));
			goto txn_abort;
		}
		prstat(&mst);
		while ((rc = edb_cursor_get(cursor, &key, &data, EDB_NEXT)) == 0) {
			iptr = data.mv_data;
			pages += *iptr;
			if (freinfo > 1) {
				char *bad = "";
				edb_size_t pg, prev;
				ssize_t i, j, span = 0;
				j = *iptr++;
				for (i = j, prev = 1; --i >= 0; ) {
					pg = iptr[i];
					if (pg <= prev)
						bad = " [bad sequence]";
					prev = pg;
					pg += span;
					for (; i >= span && iptr[i-span] == pg; span++, pg++) ;
				}
				printf("    Transaction %"Yu", %"Z"d pages, maxspan %"Z"d%s\n",
					*(edb_size_t *)key.mv_data, j, span, bad);
				if (freinfo > 2) {
					for (--j; j >= 0; ) {
						pg = iptr[j];
						for (span=1; --j >= 0 && iptr[j] == pg+span; span++) ;
						printf(span>1 ? "     %9"Yu"[%"Z"d]\n" : "     %9"Yu"\n",
							pg, span);
					}
				}
			}
		}
		edb_cursor_close(cursor);
		printf("  Free pages: %"Yu"\n", pages);
	}

	rc = edb_open(txn, subname, 0, &dbi);
	if (rc) {
		fprintf(stderr, "edb_open failed, error %d %s\n", rc, edb_strerror(rc));
		goto txn_abort;
	}

	rc = edb_stat(txn, dbi, &mst);
	if (rc) {
		fprintf(stderr, "edb_stat failed, error %d %s\n", rc, edb_strerror(rc));
		goto txn_abort;
	}
	printf("Status of %s\n", subname ? subname : "Main DB");
	prstat(&mst);

	if (alldbs) {
		EDB_cursor *cursor;
		EDB_val key;

		rc = edb_cursor_open(txn, dbi, &cursor);
		if (rc) {
			fprintf(stderr, "edb_cursor_open failed, error %d %s\n", rc, edb_strerror(rc));
			goto txn_abort;
		}
		while ((rc = edb_cursor_get(cursor, &key, NULL, EDB_NEXT_NODUP)) == 0) {
			char *str;
			EDB_dbi db2;
			if (memchr(key.mv_data, '\0', key.mv_size))
				continue;
			str = malloc(key.mv_size+1);
			memcpy(str, key.mv_data, key.mv_size);
			str[key.mv_size] = '\0';
			rc = edb_open(txn, str, 0, &db2);
			if (rc == EDB_SUCCESS)
				printf("Status of %s\n", str);
			free(str);
			if (rc) continue;
			rc = edb_stat(txn, db2, &mst);
			if (rc) {
				fprintf(stderr, "edb_stat failed, error %d %s\n", rc, edb_strerror(rc));
				goto txn_abort;
			}
			prstat(&mst);
			edb_close(env, db2);
		}
		edb_cursor_close(cursor);
	}

	if (rc == EDB_NOTFOUND)
		rc = EDB_SUCCESS;

	edb_close(env, dbi);
txn_abort:
	edb_txn_abort(txn);
env_close:
	edb_env_close(env);

	return rc ? EXIT_FAILURE : EXIT_SUCCESS;
}
