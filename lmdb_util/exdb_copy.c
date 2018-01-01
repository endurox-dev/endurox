/* exdb_copy.c - memory-mapped database backup tool */
/*
 * Copyright 2012-2017 Howard Chu, Symas Corp.
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
#ifdef _WIN32
#include <windows.h>
#define	EXDB_STDOUT	GetStdHandle(STD_OUTPUT_HANDLE)
#else
#define	EXDB_STDOUT	1
#endif
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "ndrxdb.h"

static void
sighandle(int sig)
{
}

int main(int argc,char * argv[])
{
	int rc;
	EXDB_env *env;
	const char *progname = argv[0], *act;
	unsigned flags = EXDB_RDONLY;
	unsigned cpflags = 0;

	for (; argc > 1 && argv[1][0] == '-'; argc--, argv++) {
		if (argv[1][1] == 'n' && argv[1][2] == '\0')
			flags |= EXDB_NOSUBDIR;
		else if (argv[1][1] == 'c' && argv[1][2] == '\0')
			cpflags |= EXDB_CP_COMPACT;
		else if (argv[1][1] == 'V' && argv[1][2] == '\0') {
			printf("%s\n", EXDB_VERSION_STRING);
			exit(0);
		} else
			argc = 0;
	}

	if (argc<2 || argc>3) {
		fprintf(stderr, "usage: %s [-V] [-c] [-n] srcpath [dstpath]\n", progname);
		exit(EXIT_FAILURE);
	}

#ifdef SIGPIPE
	signal(SIGPIPE, sighandle);
#endif
#ifdef SIGHUP
	signal(SIGHUP, sighandle);
#endif
	signal(SIGINT, sighandle);
	signal(SIGTERM, sighandle);

	act = "opening environment";
	rc = exdb_env_create(&env);
	if (rc == EXDB_SUCCESS) {
		rc = exdb_env_open(env, argv[1], flags, 0600);
	}
	if (rc == EXDB_SUCCESS) {
		act = "copying";
		if (argc == 2)
			rc = exdb_env_copyfd2(env, EXDB_STDOUT, cpflags);
		else
			rc = exdb_env_copy2(env, argv[2], cpflags);
	}
	if (rc)
		fprintf(stderr, "%s: %s failed, error %d (%s)\n",
			progname, act, rc, exdb_strerror(rc));
	exdb_env_close(env);

	return rc ? EXIT_FAILURE : EXIT_SUCCESS;
}
