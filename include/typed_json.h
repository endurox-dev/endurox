/* 
 * @brief Typed json support
 *
 * @file typed_json.h
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
#ifndef __TYPED_JSON_H
#define	__TYPED_JSON_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <atmi_int.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
    
extern int JSON_prepare_outgoing (typed_buffer_descr_t *descr, char *idata, long ilen, 
                    char *obuf, long *olen, long flags);
extern int JSON_prepare_incoming (typed_buffer_descr_t *descr, char *rcv_data, 
                        long rcv_len, char **odata, long *olen, long flags);
extern char * JSON_tpalloc (typed_buffer_descr_t *descr, char *subtype, long *len);
extern char * JSON_tprealloc(typed_buffer_descr_t *descr, char *cur_ptr, long len);
extern void JSON_tpfree(typed_buffer_descr_t *descr, char *buf);
extern int JSON_test(typed_buffer_descr_t *descr, char *buf, BFLDLEN len, char *expr);

#ifdef	__cplusplus
}
#endif

#endif	/* __TYPED_JSON_H */

