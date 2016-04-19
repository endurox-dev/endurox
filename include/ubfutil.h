/* 
** ATMI Level UBF utilities 
**
** @file nstdutil.h
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
#ifndef UBFUTIL_H
#define	UBFUTIL_H

#ifdef	__cplusplus
extern "C" {
#endif
/*---------------------------Includes-----------------------------------*/
#include <stdint.h>

#include "ubf.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
    
#define UBFUTIL_BUFOPT      0x00000001  /* Buffer field is optional  */

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
   
/**
 * map the c fields to UBF buffer and vice versa
 */
typedef struct
{
    BFLDID fld;         /* UBF buffer field                             */
    BFLDOCC occ;        /* UBF occurrence                               */
    size_t offset;      /* offset to data field (pointer arithmetics)   */
    BFLDLEN buf_size;   /* buffer size                                  */
    int ftype;          /* field type                                   */
    long flags;         /* special flags passed for mapping             */
} ubf_c_map_t;

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

#ifdef	__cplusplus
}
#endif

#endif	/* UBFUTIL_H */

