/* 
 * @brief VIEW Support structures
 *
 * @file typed_view.h
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

#ifndef TYPED_VIEW_H
#define	TYPED_VIEW_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <ndrstandard.h>
#include <utlist.h>
#include <exhash.h>
#include <fdatatype.h>
#include <view_cmn.h>
#include <atmi_int.h>
#include <ubfview.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/  
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
extern NDRX_API int VIEW_test(typed_buffer_descr_t *descr, char *buf, BFLDLEN len, char *expr);
extern NDRX_API void VIEW_tpfree(typed_buffer_descr_t *descr, char *buf);
extern NDRX_API int VIEW_prepare_incoming (typed_buffer_descr_t *descr, char *rcv_data, 
                        long rcv_len, char **odata, long *olen, long flags);
extern NDRX_API int VIEW_prepare_outgoing (typed_buffer_descr_t *descr, char *idata, long ilen, 
                    char *obuf, long *olen, long flags);
extern NDRX_API char * VIEW_tpalloc (typed_buffer_descr_t *descr, char *subtype, long *len);
extern NDRX_API char * VIEW_tprealloc(typed_buffer_descr_t *descr, char *cur_ptr, long len);

#ifdef	__cplusplus
}
#endif

#endif	/* TYPED_VIEW_H */

