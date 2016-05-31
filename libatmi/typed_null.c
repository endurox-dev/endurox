/* 
** TPNULL buffer type support
**
** @file typed_null.c
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
#include <stdlib.h>
#include <memory.h>

#include <ndrstandard.h>
#include <typed_buf.h>
#include <ndebug.h>
#include <tperror.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Basic init data structure allocator
 * @param subtype
 * @param len
 * @return
 */
public char	* TPNULL_tpalloc (typed_buffer_descr_t *descr, long len)
{
    char *ret=NULL;
    char fn[] = "TPNULL_tpalloc";

    /* Allocate UBF buffer, 1 byte, what so ever.. */
    ret=malloc(1);

    if (NULL==ret)
    {
        NDRX_LOG(log_error, "%s: Failed to allocate TPNULL buffer!", fn);
        _TPset_error_fmt(TPEOS, "TPNULL failed to allocate: %d bytes", sizeof(TPINIT));
        goto out;
    }

out:
    return ret;
}

/**
 * Gracefully remove free up the buffer
 * @param descr
 * @param buf
 */
public void TPNULL_tpfree(typed_buffer_descr_t *descr, char *buf)
{
    free(buf);
}
