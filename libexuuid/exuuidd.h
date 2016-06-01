/*
 * Definitions used by the exuuidd daemon
 *
 * Copyright (C) 2007 Theodore Ts'o.
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

#ifndef _EXUUID_EXUUIDD_H
#define _EXUUID_EXUUIDD_H

#define EXUUIDD_DIR		_PATH_LOCALSTATEDIR "/exuuidd"
#define EXUUIDD_SOCKET_PATH	EXUUIDD_DIR "/request"
#define EXUUIDD_PIDFILE_PATH	EXUUIDD_DIR "/exuuidd.pid"
#define EXUUIDD_PATH		"/usr/sbin/exuuidd"

#define EXUUIDD_OP_GETPID			0
#define EXUUIDD_OP_GET_MAXOP		1
#define EXUUIDD_OP_TIME_EXUUID		2
#define EXUUIDD_OP_RANDOM_EXUUID		3
#define EXUUIDD_OP_BULK_TIME_EXUUID		4
#define EXUUIDD_OP_BULK_RANDOM_EXUUID	5
#define EXUUIDD_MAX_OP			EXUUIDD_OP_BULK_RANDOM_EXUUID

extern int __exuuid_generate_time(exuuid_t out, int *num);
extern void __exuuid_generate_random(exuuid_t out, int *num);

#endif /* _EXUUID_EXUUID_H */
