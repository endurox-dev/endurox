/* 
 * @brief ATMI Level UBF utilities
 *
 * @file ubfutil.h
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or Mavimax's license for commercial use.
 * -----------------------------------------------------------------------------
 * GPL license:
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * -----------------------------------------------------------------------------
 * A commercial use license is available from Mavimax, Ltd
 * contact@mavimax.com
 * -----------------------------------------------------------------------------
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
    
#define UBFUTIL_EXPORT      0x00000001  /* Export Field           */
#define UBFUTIL_OPTIONAL    0x00000002  /* Field is opitional     */

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
    short ftype;        /* user field type                              */
} ubf_c_map_t;

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

extern NDRX_API int atmi_cvt_c_to_ubf(ubf_c_map_t *map, void *c_struct, UBFH *p_ub, long *rules);
extern NDRX_API int atmi_cvt_ubf_to_c(ubf_c_map_t *map, UBFH *p_ub, void *c_struct, long *rules);

extern NDRX_API void ndrx_debug_dump_UBF(int lev, char *title, UBFH *p_ub);
extern NDRX_API void ndrx_debug_dump_UBF_ubflogger(int lev, char *title, UBFH *p_ub);

    
#ifdef	__cplusplus
}
#endif

#endif	/* UBFUTIL_H */

