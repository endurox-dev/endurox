/* edb_dump.c - memory-mapped database dump tool */
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
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>
#include "exdb.h"

#define Yu	EDB_PRIy(u)

#define PRINT	1
static int mode;

typedef struct flagbit {
	int bit;
	char *name;
} flagbit;

flagbit dbflags[] = {
	{ EDB_REVERSEKEY, "reversekey" },
	{ EDB_DUPSORT, "dupsort" },
	{ EDB_INTEGERKEY, "integerkey" },
	{ EDB_DUPFIXED, "dupfixed" },
	{ EDB_INTEGERDUP, "integerdup" },
	{ EDB_REVERSEDUP, "reversedup" },
	{ 0, NULL }
};

static volatile sig_atomic_t gotsig;

static void dumpsig( int sig )
{
	gotsig=1;
}

static const char hexc[] = "0123456789abcdef";

static void hex(unsigned char c)
{
	putchar(hexc[c >> 4]);
	putchar(hexc[c & 0xf]);
}

static void text(EDB_val *v)
{
	unsigned char *c, *end;

	putchar(' ');
	c = v->mv_data;
	end = c + v->mv_size;
	while (c < end) {
		if (isprint(*c)) {
			putchar(*c);
		} else {
			putchar('\\');
			hex(*c);
		}
		c++;
	}
	putchar('\n');
}

static void byte(EDB_val *v)
{
	unsigned char *c, *end;

	putchar(' ');
	c = v->mv_data;
	end = c + v->mv_size;
	while (c < end) {
		hex(*c++);
	}
	putchar('\n');
}

/* Dump in BDB-compatible format */
static int dumpit(EDB_txn *txn, EDB_dbi dbi, char *name)
{
	EDB_cursor *mc;
	EDB_stat ms;
	EDB_val key, data;
	EDB_envinfo info;
	unsigned int flags;
	int rc, i;

	rc = edb_dbi_flags(txn, dbi, &flags);
	if (rc) return rc;

	rc = edb_stat(txn, dbi, &ms);
	if (rc) return rc;

	rc = edb_env_info(edb_txn_env(txn), &info);
	if (rc) return rc;

	printf("VERSION=3\n");
	printf("format=%s\n", mode & PRINT ? "print" : "bytevalue");
	if (name)
		printf("database=%s\n", name);
	printf("type=btree\n");
	printf("mapsize=%"Yu"\n", info.me_mapsize);
	if (info.me_mapaddr)
		printf("mapaddr=%p\n", info.me_mapaddr);
	printf("maxreaders=%u\n", info.me_maxreaders);

	if (flags & EDB_DUPSORT)
		printf("duplicates=1\n");

	for (i=0; dbflags[i].bit; i++)
		if (flags & dbflags[i].bit)
			printf("%s=1\n", dbflags[i].name);

	printf("db_pagesize=%d\n", ms.ms_psize);
	printf("HEADER=END\n");

	rc = edb_cursor_open(txn, dbi, &mc);
	if (rc) return rc;

	while ((rc = edb_cursor_get(mc, &key, &data, EDB_NEXT) == EDB_SUCCESS)) {
		if (gotsig) {
			rc = EINTR;
			break;
		}
		if (mode & PRINT) {
			text(&key);
			text(&data);
		} else {
			byte(&key);
			byte(&data);
		}
	}
	printf("DATA=END\n");
	if (rc == EDB_NOTFOUND)
		rc = EDB_SUCCESS;

	return rc;
}

static void usage(char *prog)
{
	fprintf(stderr, "usage: %s [-V] [-f output] [-l] [-n] [-p] [-a|-s subdb] dbpath\n", prog);
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
	int i, rc;
	EDB_env *env;
	EDB_txn *txn;
	EDB_dbi dbi;
	char *prog = argv[0];
	char *envname;
	char *subname = NULL;
	int alldbs = 0, envflags = 0, list = 0;

	if (argc < 2) {
		usage(prog);
	}

	/* -a: dump main DB and all subDBs
	 * -s: dump only the named subDB
	 * -n: use NOSUBDIR flag on env_open
	 * -p: use printable characters
	 * -f: write to file instead of stdout
	 * -V: print version and exit
	 * (default) dump only the main DB
	 */
	while ((i = getopt(argc, argv, "af:lnps:V")) != EOF) {
		switch(i) {
		case 'V':
			printf("%s\n", EDB_VERSION_STRING);
			exit(0);
			break;
		case 'l':
			list = 1;
			/*FALLTHROUGH*/;
		case 'a':
			if (subname)
				usage(prog);
			alldbs++;
			break;
		case 'f':
			if (freopen(optarg, "w", stdout) == NULL) {
				fprintf(stderr, "%s: %s: reopen: %s\n",
					prog, optarg, strerror(errno));
				exit(EXIT_FAILURE);
			}
			break;
		case 'n':
			envflags |= EDB_NOSUBDIR;
			break;
		case 'p':
			mode |= PRINT;
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

#ifdef SIGPIPE
	signal(SIGPIPE, dumpsig);
#endif
#ifdef SIGHUP
	signal(SIGHUP, dumpsig);
#endif
	signal(SIGINT, dumpsig);
	signal(SIGTERM, dumpsig);

	envname = argv[optind];
	rc = edb_env_create(&env);
	if (rc) {
		fprintf(stderr, "edb_env_create failed, error %d %s\n", rc, edb_strerror(rc));
		return EXIT_FAILURE;
	}

	if (alldbs || subname) {
		edb_env_set_maxdbs(env, 2);
	}

	rc = edb_env_open(env, envname, envflags | EDB_RDONLY, 0664);
	if (rc) {
		fprintf(stderr, "edb_env_open failed, error %d %s\n", rc, edb_strerror(rc));
		goto env_close;
	}

	rc = edb_txn_begin(env, NULL, EDB_RDONLY, &txn);
	if (rc) {
		fprintf(stderr, "edb_txn_begin failed, error %d %s\n", rc, edb_strerror(rc));
		goto env_close;
	}

	rc = edb_open(txn, subname, 0, &dbi);
	if (rc) {
		fprintf(stderr, "edb_open failed, error %d %s\n", rc, edb_strerror(rc));
		goto txn_abort;
	}

	if (alldbs) {
		EDB_cursor *cursor;
		EDB_val key;
		int count = 0;

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
			count++;
			str = malloc(key.mv_size+1);
			memcpy(str, key.mv_data, key.mv_size);
			str[key.mv_size] = '\0';
			rc = edb_open(txn, str, 0, &db2);
			if (rc == EDB_SUCCESS) {
				if (list) {
					printf("%s\n", str);
					list++;
				} else {
					rc = dumpit(txn, db2, str);
					if (rc)
						break;
				}
				edb_close(env, db2);
			}
			free(str);
			if (rc) continue;
		}
		edb_cursor_close(cursor);
		if (!count) {
			fprintf(stderr, "%s: %s does not contain multiple databases\n", prog, envname);
			rc = EDB_NOTFOUND;
		} else if (rc == EDB_NOTFOUND) {
			rc = EDB_SUCCESS;
		}
	} else {
		rc = dumpit(txn, dbi, subname);
	}
	if (rc && rc != EDB_NOTFOUND)
		fprintf(stderr, "%s: %s: %s\n", prog, envname, edb_strerror(rc));

	edb_close(env, dbi);
txn_abort:
	edb_txn_abort(txn);
env_close:
	edb_env_close(env);

	return rc ? EXIT_FAILURE : EXIT_SUCCESS;
}
