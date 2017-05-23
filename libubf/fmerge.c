/* 
** UBF library
** The emulator of UBF library
** Enduro Execution Library
** Implementation of Bupdate, Bconcat
**
** @file fmerge.c
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
 * We will use temp buffer here.
 * This is not the most optimal version. But we will re-use existing components.
 * @param p_ub_dst
 * @param p_ub_src
 * @return
 */
public int _Bupdate (UBFH *p_ub_dst, UBFH *p_ub_src)
{
    int ret=SUCCEED;
    UBF_header_t *hdr = (UBF_header_t *)p_ub_dst;
    char *p_fld;
    BFLDID bfldid = BFIRSTFLDID;
    BFLDOCC occ = 0;
    BFLDLEN len=0;
    Bnext_state_t state;
    int nxt_stat;
    get_fld_loc_info_t chg_state;
    memset(&chg_state, 0, sizeof(chg_state));
    memset(&state, 0, sizeof(state));
    chg_state.last_checked = &hdr->bfldid;
            
    while(SUCCEED==ret &&
        1==(nxt_stat=_Bnext(&state, p_ub_src, &bfldid, &occ, NULL, &len, &p_fld)))
    {
        /*
         * Update the occurrence in target buffer.
         */
        if (SUCCEED!=(ret=_Bchg(p_ub_dst, bfldid, occ, p_fld, len, &chg_state)))
        {
            UBF_LOG(log_debug, "Failed to set %s[%d]", 
                                _Bfname_int(bfldid), occ);
        }
    }

    if (FAIL==nxt_stat)
        ret=FAIL;

    return ret;
}

/**
 * Contact two buffers.
 * @param p_ub_dst
 * @param p_ub_src
 * @return
 */
public int _Bconcat (UBFH *p_ub_dst, UBFH *p_ub_src)
{
    int ret=SUCCEED;
    UBF_header_t *hdr = (UBF_header_t *)p_ub_dst;
    char *p_fld;
    BFLDID bfldid = BFIRSTFLDID;
    BFLDOCC occ = 0;
    BFLDLEN len=0;
    Bnext_state_t state;
    int nxt_stat;
    get_fld_loc_info_t add_state;

   
    memset(&add_state, 0, sizeof(add_state));
    add_state.last_checked = &hdr->bfldid;

    memset(&state, 0, sizeof(state));

    while(SUCCEED==ret &&
               1==(nxt_stat=_Bnext(&state, p_ub_src, &bfldid, &occ, NULL, &len, &p_fld)))
    {
        /*
         * Add new occurrances to the buffer.
         */
        if (SUCCEED!=(ret=_Badd(p_ub_dst, bfldid, p_fld, len, &add_state)))
        {
            UBF_LOG(log_debug, "Failed to set %s[%d]",
                                                _Bfname_int(bfldid), occ);
        }
    }

    if (FAIL==nxt_stat)
        ret=FAIL;

    return ret;
}

