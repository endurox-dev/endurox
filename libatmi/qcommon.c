/* 
** Persistent queue commons (used between tmqsrv and "userspace" api)
**
** @file qcommon.c
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
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <dlfcn.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <ndrxdcmn.h>
#include <userlog.h>
#include <ubf.h>
#include <ubfutil.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

static ubf_c_map_t M_tpqctl_map[] = 
{
#if 0
    BFLDID fld;         /* UBF buffer field                             */
    BFLDOCC occ;        /* UBF occurrence                               */
    size_t offset;      /* offset to data field (pointer arithmetics)   */
    BFLDLEN buf_size;   /* buffer size                                  */
    int ftype;          /* field type                                   */
    long flags;         /* special flags passed for mapping              */
#endif
    
    {BBADFLDID}
};


/**
 * Copy the TPQCTL data to buffer
 * @param p_ub destination buffer
 * @param ctl source struct
 * @return SUCCEED/FAIL
 */
public int tmq_tpqctl_to_ubf(UBFH *p_ub, TPQCTL *ctl)
{
    int ret = SUCCEED;
    
    /* TODO: */
    
    
    
out:
    return ret;
}

/**
 * Build the TPQCTL from UBF buffer
 * @param p_ub source buffer
 * @param ctl destination strcture
 * @return SUCCEED/FAIL
 */
public int tmq_tpqctl_from_ubf(UBFH *p_ub, TPQCTL *ctl)
{
    int ret = SUCCEED;
    
    /* TODO: */
    
out:
    return ret;
}

