/* 
** Enduro/X Standard library
**
** @file ndrstandard.h
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, ATR Baltic, SIA. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or ATR Baltic's license for commercial use.
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
** A commercial use license is available from ATR Baltic, SIA
** contact@atrbaltic.com
** -----------------------------------------------------------------------------
*/
#ifndef NDRSTANDARD_H
#define	NDRSTANDARD_H

#ifdef	__cplusplus
extern "C" {
#endif
    
#include <ndrx_config.h>
#include <stdint.h>

    
#if UINTPTR_MAX == 0xffffffff
/* 32-bit */
#define SYS32BIT
#elif UINTPTR_MAX == 0xffffffffffffffff
#define SYS64BIT
#else
/* wtf */
#endif

#ifndef FAIL
#define FAIL		-1
#endif
    
#ifndef SUCCEED
#define SUCCEED		0
#endif

#ifndef public
#define public
#endif
    
#ifndef private
#define private     	static
#endif
    

#ifndef EOS
#define EOS             '\0'
#endif
    
#ifndef BYTE
#define BYTE(x) ((x) & 0xff)
#endif


#ifndef  __cplusplus
#ifndef bool
typedef int         bool;
#endif
#endif

#ifndef FALSE
#define FALSE        0
#endif

#ifndef TRUE
#define TRUE         1
#endif

#define N_DIM(a)        (sizeof(a)/sizeof(*(a)))

#ifndef FAIL_OUT
#define FAIL_OUT(X)    {X=FAIL; goto out;}
#endif


#ifndef OFFSET
#ifdef SYS64BIT
#define OFFSET(s,e)   ((long) &(((s *)0)->e) )
#else
#define OFFSET(s,e)   ((const int) &(((s *)0)->e) )
#endif
#endif


#ifndef ELEM_SIZE
#define ELEM_SIZE(s,e)        (sizeof(((s *)0)->e))
#endif

#define NDRX_MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

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

