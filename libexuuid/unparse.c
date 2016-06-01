/*
 * unparse.c -- convert a EXUUID to string
 *
 * Copyright (C) 1996, 1997 Theodore Ts'o.
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

#include <stdio.h>

#include "exuuidP.h"

static const char *fmt_lower =
	"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x";

static const char *fmt_upper =
	"%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X";

#ifdef EXUUID_UNPARSE_DEFAULT_UPPER
#define FMT_DEFAULT fmt_upper
#else
#define FMT_DEFAULT fmt_lower
#endif

static void exuuid_unparse_x(const exuuid_t uu, char *out, const char *fmt)
{
	struct exuuid exuuid;

	exuuid_unpack(uu, &exuuid);
	sprintf(out, fmt,
		exuuid.time_low, exuuid.time_mid, exuuid.time_hi_and_version,
		exuuid.clock_seq >> 8, exuuid.clock_seq & 0xFF,
		exuuid.node[0], exuuid.node[1], exuuid.node[2],
		exuuid.node[3], exuuid.node[4], exuuid.node[5]);
}

void exuuid_unparse_lower(const exuuid_t uu, char *out)
{
	exuuid_unparse_x(uu, out,	fmt_lower);
}

void exuuid_unparse_upper(const exuuid_t uu, char *out)
{
	exuuid_unparse_x(uu, out,	fmt_upper);
}

void exuuid_unparse(const exuuid_t uu, char *out)
{
	exuuid_unparse_x(uu, out, FMT_DEFAULT);
}
