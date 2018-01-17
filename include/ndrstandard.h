/* 
** Enduro/X Standard library
**
** @file ndrstandard.h
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, Mavimax, Ltd. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or Mavimax's license for commercial use.
** -----------------------------------------------------------------------------
** GPL license:
** 
** This program is free software; you can redistribute it and/or modify it under
** the terms of the GNU General Public License as published by the Free Software
** Foundation; either version 2 of the License, or (at your option) any later
** version.
**
** This program is distributed in the hope that it will be useful, but WITHOUT ANY
** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
** PARTICULAR PURPOSE. See the GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License along with
** this program; if not, write to the Free Software Foundation, Inc., 59 Temple
** Place, Suite 330, Boston, MA 02111-1307 USA
**
** -----------------------------------------------------------------------------
** A commercial use license is available from Mavimax, Ltd
** contact@mavimax.com
** -----------------------------------------------------------------------------
*/
#ifndef NDRSTANDARD_H
#define	NDRSTANDARD_H

#ifdef	__cplusplus
extern "C" {
#endif
    
#include <ndrx_config.h>
#include <stdint.h>
#include <string.h>
    
#if UINTPTR_MAX == 0xffffffff
/* 32-bit */
#define SYS32BIT
#elif UINTPTR_MAX == 0xffffffffffffffff
#define SYS64BIT
#else
/* wtf */
#endif

#ifndef EXFAIL
#define EXFAIL		-1
#endif
    
#ifndef EXSUCCEED
#define EXSUCCEED		0
#endif

#ifndef expublic
#define expublic
#endif
    
#ifndef exprivate
#define exprivate     	static
#endif

#ifndef EXEOS
#define EXEOS             '\0'
#endif
    
#ifndef EXBYTE
#define EXBYTE(x) ((x) & 0xff)
#endif

#ifndef EXFALSE
#define EXFALSE        0
#endif

#ifndef EXTRUE
#define EXTRUE         1
#endif

#define N_DIM(a)        (sizeof(a)/sizeof(*(a)))

#ifndef EXFAIL_OUT
#define EXFAIL_OUT(X)    {X=EXFAIL; goto out;}
#endif


#ifndef EXOFFSET
#ifdef SYS64BIT
#define EXOFFSET(STRUCT,ELM)   ((long) &(((STRUCT *)0)->ELM) )
#else
#define EXOFFSET(STRUCT,ELM)   ((const int) &(((STRUCT *)0)->ELM) )
#endif
#endif

    
#define NDRX_WORD_SIZE  (int)sizeof(void *)*8

#ifndef EXELEM_SIZE
#define EXELEM_SIZE(STRUCT,ELM)        (sizeof(((STRUCT *)0)->ELM))
#endif

#define NDRX_MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
    
#define NDRX_ATMI_MSG_MAX_SIZE   65536 /* internal */
    
#define NDRX_STACK_MSG_FACTOR        30 /* max number of messages in stack */
    
/* Feature #127 
 * Allow dynamic buffer sizing with Variable Length Arrays (VLS) in C99
 */
extern NDRX_API long ndrx_msgsizemax (void);
#define NDRX_MSGSIZEMAX          ndrx_msgsizemax()

#define NDRX_PADDING_MAX         16 /* Max compiled padding in struct (assumed) */

#if 0
/*These are slow! */
#define NDRX_STRCPY_SAFE(X, Y) {strncpy(X, Y, sizeof(X)-1);\
                          X[sizeof(X)-1]=EXEOS;}
	
#define NDRX_STRNCPY_SAFE(X, Y, N) {strncpy(X, Y, N-1);\
                          X[N]=EXEOS;}
	
#endif

#ifdef HAVE_STRNLEN

#define NDRX_STRNLEN strnlen

#else

extern NDRX_API size_t ndrx_strnlen(char *str, size_t max);

#define NDRX_STRNLEN ndrx_strnlen

#endif
    
#ifdef HAVE_STRLCPY
#define NDRX_STRCPY_SAFE(X, Y) {strlcpy(X, Y, sizeof(X)-1);\
                          X[sizeof(X)-1]=EXEOS;}
	
#define NDRX_STRNCPY_SAFE(X, Y, N) {strlcpy(X, Y, N-1);\
                          X[N-1]=EXEOS;}
    
#else
    
/* TODO: switch to STRLCPY if  */
#define NDRX_STRCPY_SAFE(X, Y) {\
	int ndrx_I5SmWDM_len = strlen(Y);\
	int ndrx_XgCmDEk_bufzs = sizeof(X)-1;\
	if (ndrx_I5SmWDM_len > ndrx_XgCmDEk_bufzs)\
	{\
		ndrx_I5SmWDM_len = ndrx_XgCmDEk_bufzs;\
	}\
	memcpy(X, Y, ndrx_I5SmWDM_len);\
	X[ndrx_I5SmWDM_len]=0;\
	}
    
/* N - buffer size of X 
 * This always copies EOS
 */	
#define NDRX_STRNCPY_SAFE(X, Y, N) {\
	int ndrx_I5SmWDM_len = strlen(Y);\
	int ndrx_XgCmDEk_bufzs = N-1;\
	if (ndrx_I5SmWDM_len > ndrx_XgCmDEk_bufzs)\
	{\
		ndrx_I5SmWDM_len = ndrx_XgCmDEk_bufzs;\
	}\
	memcpy(X, Y, ndrx_I5SmWDM_len);\
	X[ndrx_I5SmWDM_len]=0;\
	}
#endif

/* in case if dest buffer allows, we copy EOS too
 * This is compatible with strncpy
 */
#define NDRX_STRNCPY(X, Y, N) {\
	int ndrx_I5SmWDM_len = strlen(Y)+1;\
	if (ndrx_I5SmWDM_len > N)\
	{\
		ndrx_I5SmWDM_len = N;\
	}\
	memcpy(X, Y, ndrx_I5SmWDM_len);\
	}

/* Copy the maxing at source buffer, not checking the dest
 * N - number of symbols to test in source buffer.
 * The dest buffer is assumed to be large enough.
 */
#define NDRX_STRNCPY_SRC(X, Y, N) {\
	int ndrx_I5SmWDM_len = NDRX_STRNLEN(Y, N);\
	memcpy(X, Y, ndrx_I5SmWDM_len);\
	}
/*
 * So we use these two macros where we need know that more times they will be
 * true, than false. This makes some boost for CPU code branching.
 * 
 * So for example if expect that some variable (c) must not be NULL
 * then we could run NDRX_UNLIKELY(NDRX_UNLIKELY).
 * This is useful for error handling, because mostly we do not have errors like
 * malloc fail etc..
 */
#if HAVE_EXPECT

#define NDRX_LIKELY(x)      __builtin_expect(!!(x), 1)
#define NDRX_UNLIKELY(x)    __builtin_expect(!!(x), 0)

#else

/*
 * And this is fallback if we do not have expect compiler option.
 */
#define NDRX_LIKELY(x)      (x)
#define NDRX_UNLIKELY(x)    (x)

#endif

#ifdef	__cplusplus
}
#endif

#endif	/* NDRSTANDARD_H */

