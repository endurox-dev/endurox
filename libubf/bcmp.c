/* 
** Implementation of Bcmp and Bsubset
**
** @file bcmp.c
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

/*---------------------------Includes-----------------------------------*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include <ubf.h>
#include <ubf_int.h>	/* Internal headers for UBF... */
#include <fdatatype.h>
#include <ferror.h>
#include <fieldtable.h>
#include <ndrstandard.h>
#include <ndebug.h>
#include <cf.h>
#include <ubf_impl.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Compare two UBF buffers
 * @param p_ubf1 buffer 1 to compare
 * @param p_ubf2 buffer 2 to compare with 1
 * @return 
 */
expublic int Bcmp(UBFH *p_ubf1, UBFH *p_ubf2)
{
    int ret = EXFAIL;
    
out:
    return ret;
}

/**
 * Compares the buffer and returns TRUE if all p_ubf2 fields and values 
 * are present in ubf1
 * @param p_ubf1 haystack
 * @param p_ubf2 needle
 * @return EXFALSE not a subset, EXTRUE is subset, EXFAIL (error occurred)
 */
expublic int Bsubset(UBFH *p_ubf1, UBFH *p_ubf2)
{
    int ret = EXFAIL;
    
out:
    return ret;
}

