/*
 * tst_exuuid.c --- test program from the EXUUID library
 *
 * Copyright (C) 1996, 1997, 1998 Theodore Ts'o.
 *
 * %Begin-Header%
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, and the entire permission notice in its entirety,
 *    including the disclaimer of warranties.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, ALL OF
 * WHICH ARE HEREBY DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF NOT ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * %End-Header%
 */

#ifdef _WIN32
#define _WIN32_WINNT 0x0500
#include <windows.h>
#define EXUUID MYEXUUID
#endif

#include <stdio.h>
#include <stdlib.h>

#include "exuuid.h"

static int test_exuuid(const char * exuuid, int isValid)
{
	static const char * validStr[2] = {"invalid", "valid"};
	exuuid_t exuuidBits;
	int parsedOk;

	parsedOk = exuuid_parse(exuuid, exuuidBits) == 0;

	printf("%s is %s", exuuid, validStr[isValid]);
	if (parsedOk != isValid) {
		printf(" but exuuid_parse says %s\n", validStr[parsedOk]);
		return 1;
	}
	printf(", OK\n");
	return 0;
}

#ifdef __GNUC__
#define ATTR(x) __attribute__(x)
#else
#define ATTR(x)
#endif

int
main(int argc ATTR((unused)) , char **argv ATTR((unused)))
{
	exuuid_t		buf, tst;
	char		str[100];
	struct timeval	tv;
	time_t		time_reg;
	unsigned char	*cp;
	int i;
	int failed = 0;
	int type, variant;

	exuuid_generate(buf);
	exuuid_unparse(buf, str);
	printf("EXUUID generate = %s\n", str);
	printf("EXUUID: ");
	for (i=0, cp = (unsigned char *) &buf; i < 16; i++) {
		printf("%02x", *cp++);
	}
	printf("\n");
	type = exuuid_type(buf); 	variant = exuuid_variant(buf);
	printf("EXUUID type = %d, EXUUID variant = %d\n", type, variant);
	if (variant != EXUUID_VARIANT_DCE) {
		printf("Incorrect EXUUID Variant; was expecting DCE!\n");
		failed++;
	}
	printf("\n");

	exuuid_generate_random(buf);
	exuuid_unparse(buf, str);
	printf("EXUUID random string = %s\n", str);
	printf("EXUUID: ");
	for (i=0, cp = (unsigned char *) &buf; i < 16; i++) {
		printf("%02x", *cp++);
	}
	printf("\n");
	type = exuuid_type(buf); 	variant = exuuid_variant(buf);
	printf("EXUUID type = %d, EXUUID variant = %d\n", type, variant);
	if (variant != EXUUID_VARIANT_DCE) {
		printf("Incorrect EXUUID Variant; was expecting DCE!\n");
		failed++;
	}
	if (type != 4) {
		printf("Incorrect EXUUID type; was expecting "
		       "4 (random type)!\n");
		failed++;
	}
	printf("\n");

	exuuid_generate_time(buf);
	exuuid_unparse(buf, str);
	printf("EXUUID string = %s\n", str);
	printf("EXUUID time: ");
	for (i=0, cp = (unsigned char *) &buf; i < 16; i++) {
		printf("%02x", *cp++);
	}
	printf("\n");
	type = exuuid_type(buf); 	variant = exuuid_variant(buf);
	printf("EXUUID type = %d, EXUUID variant = %d\n", type, variant);
	if (variant != EXUUID_VARIANT_DCE) {
		printf("Incorrect EXUUID Variant; was expecting DCE!\n");
		failed++;
	}
	if (type != 1) {
		printf("Incorrect EXUUID type; was expecting "
		       "1 (time-based type)!\\n");
		failed++;
	}
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	time_reg = exuuid_time(buf, &tv);
	printf("EXUUID time is: (%ld, %ld): %s\n", tv.tv_sec, tv.tv_usec,
	       ctime(&time_reg));
	exuuid_parse(str, tst);
	if (!exuuid_compare(buf, tst))
		printf("EXUUID parse and compare succeeded.\n");
	else {
		printf("EXUUID parse and compare failed!\n");
		failed++;
	}
	exuuid_clear(tst);
	if (exuuid_is_null(tst))
		printf("EXUUID clear and is null succeeded.\n");
	else {
		printf("EXUUID clear and is null failed!\n");
		failed++;
	}
	exuuid_copy(buf, tst);
	if (!exuuid_compare(buf, tst))
		printf("EXUUID copy and compare succeeded.\n");
	else {
		printf("EXUUID copy and compare failed!\n");
		failed++;
	}
	failed += test_exuuid("84949cc5-4701-4a84-895b-354c584a981b", 1);
	failed += test_exuuid("84949CC5-4701-4A84-895B-354C584A981B", 1);
	failed += test_exuuid("84949cc5-4701-4a84-895b-354c584a981bc", 0);
	failed += test_exuuid("84949cc5-4701-4a84-895b-354c584a981", 0);
	failed += test_exuuid("84949cc5x4701-4a84-895b-354c584a981b", 0);
	failed += test_exuuid("84949cc504701-4a84-895b-354c584a981b", 0);
	failed += test_exuuid("84949cc5-470104a84-895b-354c584a981b", 0);
	failed += test_exuuid("84949cc5-4701-4a840895b-354c584a981b", 0);
	failed += test_exuuid("84949cc5-4701-4a84-895b0354c584a981b", 0);
	failed += test_exuuid("g4949cc5-4701-4a84-895b-354c584a981b", 0);
	failed += test_exuuid("84949cc5-4701-4a84-895b-354c584a981g", 0);

	if (failed) {
		printf("%d failures.\n", failed);
		exit(1);
	}
	return 0;
}
