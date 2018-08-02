/* 
 *
 * @file cltlib.c
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

#include <ndrstandard.h>
#include <atmi.h>
#include <tperror.h>
/**
 * Currently do nothing, just for build
 */
void __dummy(void)
{

}


/**
 * API function tpreturn - TP Proto
 * @param rval
 * @param rcode
 * @param data
 * @param len
 * @param flags
 */
expublic void     tpreturn (int rval, long rcode, char *data, long len, long flags)
{
    /*API_ENTRY;

     return _tpreturn(rval, rcode, data, len, flags); */
    ndrx_TPset_error_fmt(TPEPROTO, "tpreturn - not available for clients!!!");
    return;
}

/**
 * API function of tpforward - TP Proto
 * @param svc
 * @param data
 * @param len
 * @param flags
 */
expublic void tpforward (char *svc, char *data, long len, long flags)
{
    /*API_ENTRY;

    _tpforward (svc, data, len, flags);*/
    ndrx_TPset_error_fmt(TPEPROTO, "tpforward - not available for clients!!!");
    return;
}

