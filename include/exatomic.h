/**
 * @brief Enduro/X Atomic API
 *
 * @file exatomic.h
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2019, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * AGPL (with Java and Go exceptions) or Mavimax's license for commercial use.
 * See LICENSE file for full text.
 * -----------------------------------------------------------------------------
 * AGPL license:
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License, version 3 as published
 * by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU Affero General Public License, version 3
 * for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along 
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * -----------------------------------------------------------------------------
 * A commercial use license is available from Mavimax, Ltd
 * contact@mavimax.com
 * -----------------------------------------------------------------------------
 */
#ifndef EXATOMIC_H_
#define EXATOMIC_H_

#if defined(__cplusplus)
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <ndrx_config.h>
    
#ifdef EX_HAVE_STDATOMIC
#include <stdatomic.h>
#endif
    
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
    
/**
 * Increment value atomically (where platform allows this)
 * @param OBJ value to increment (pointer to)
 * @param ARG increment by
 */
#if defined(EX_HAVE_STDATOMIC)   
#define NDRX_ATOMIC_ADD(OBJ, ARG)   atomic_fetch_add((OBJ), (ARG))
#elif defined(EX_HAVE_SYNCFETCHADD)    
#define NDRX_ATOMIC_ADD(OBJ, ARG)   __sync_fetch_and_add((OBJ), (ARG))
#else                                   
#define NDRX_ATOMIC_ADD(OBJ, ARG) *(OBJ) = *(OBJ) + *(ARG)
#endif

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

#if defined(__cplusplus)
}
#endif

#endif
/* vim: set ts=4 sw=4 et smartindent: */
