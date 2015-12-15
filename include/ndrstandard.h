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
    
#include <stdint.h>

    
#if UINTPTR_MAX == 0xffffffff
/* 32-bit */
#define SYS32BIT
#elif UINTPTR_MAX == 0xffffffffffffffff
#define SYS64BIT
#else
/* wtf */
#endif

#define FAIL		-1
#define SUCCEED		0
#define public
#define private     	static
#define EOS             '\0'
#define BYTE(x) ((x) & 0xff)


#ifndef  __cplusplus
#ifndef bool
typedef int         bool;
#endif
#endif

#define FALSE        0
#define TRUE         1

#define N_DIM(a)        (sizeof(a)/sizeof(*(a)))

#define FAIL_OUT(X)    {X=FAIL; goto out;}

#ifdef SYS64BIT
#define OFFSET(s,e)   ((long) &(((s *)0)->e) )
#else
#define OFFSET(s,e)   ((const int) &(((s *)0)->e) )
#endif

#define ELEM_SIZE(s,e)        (sizeof(((s *)0)->e))

#ifdef	__cplusplus
}
#endif

#endif	/* NDRSTANDARD_H */

