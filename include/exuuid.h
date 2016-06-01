/*
 * Public include file for the EXUUID library
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

#ifndef _EXUUID_EXUUID_H
#define _EXUUID_EXUUID_H

#include <sys/types.h>
#ifndef _WIN32
#include <sys/time.h>
#endif
#include <time.h>

typedef unsigned char exuuid_t[16];

/* EXUUID Variant definitions */
#define EXUUID_VARIANT_NCS	0
#define EXUUID_VARIANT_DCE	1
#define EXUUID_VARIANT_MICROSOFT	2
#define EXUUID_VARIANT_OTHER	3

/* EXUUID Type definitions */
#define EXUUID_TYPE_DCE_TIME   1
#define EXUUID_TYPE_DCE_RANDOM 4

/* Allow EXUUID constants to be defined */
#ifdef __GNUC__
#define EXUUID_DEFINE(name,u0,u1,u2,u3,u4,u5,u6,u7,u8,u9,u10,u11,u12,u13,u14,u15) \
	static const exuuid_t name __attribute__ ((unused)) = {u0,u1,u2,u3,u4,u5,u6,u7,u8,u9,u10,u11,u12,u13,u14,u15}
#else
#define EXUUID_DEFINE(name,u0,u1,u2,u3,u4,u5,u6,u7,u8,u9,u10,u11,u12,u13,u14,u15) \
	static const exuuid_t name = {u0,u1,u2,u3,u4,u5,u6,u7,u8,u9,u10,u11,u12,u13,u14,u15}
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* clear.c */
void exuuid_clear(exuuid_t uu);

/* compare.c */
int exuuid_compare(const exuuid_t uu1, const exuuid_t uu2);

/* copy.c */
void exuuid_copy(exuuid_t dst, const exuuid_t src);

/* gen_exuuid.c */
void exuuid_generate(exuuid_t out);
void exuuid_generate_random(exuuid_t out);
void exuuid_generate_time(exuuid_t out);
int exuuid_generate_time_safe(exuuid_t out);

/* isnull.c */
int exuuid_is_null(const exuuid_t uu);

/* parse.c */
int exuuid_parse(const char *in, exuuid_t uu);

/* unparse.c */
void exuuid_unparse(const exuuid_t uu, char *out);
void exuuid_unparse_lower(const exuuid_t uu, char *out);
void exuuid_unparse_upper(const exuuid_t uu, char *out);

/* exuuid_time.c */
time_t exuuid_time(const exuuid_t uu, struct timeval *ret_tv);
int exuuid_type(const exuuid_t uu);
int exuuid_variant(const exuuid_t uu);

#ifdef __cplusplus
}
#endif

#endif /* _EXUUID_EXUUID_H */
