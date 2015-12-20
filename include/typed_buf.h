/* 
** XATMI Typed buffer support
**
** @file typed_buf.h
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
#ifndef TYPED_BUF_H
#define	TYPED_BUF_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <atmi_int.h> /* include ATMI internal structures */
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

#define BUF_TYPE_MIN        0 /* min buffer type, for integrity */
#define BUF_TYPE_UBF        0
#define BUF_TYPE_INIT       2 /* Keep the G_buf_descr indexes */
#define BUF_TYPE_NULL       3
#define BUF_TYPE_STRING     4
#define BUF_TYPE_CARRAY     5 /* RFU */
#define BUF_TYPE_JSON       6 /* RFU */
#define BUF_TYPE_MAX        6 /* max buffer type, for integrity */

#define BUF_TYPE_UBF_STR        "UBF"
#define BUF_TYPE_INIT_STR       "INIT"
#define BUF_TYPE_STRING_STR     "STRING"
#define BUF_TYPE_CARRAY_STR     "CARRAY"
#define BUF_TYPE_NULL_STR       "NULL"

/* others: VIEW X_COMMON X_C_TYPE X_OCTET FML32 VIEW32 - not supported currently */

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
extern buffer_obj_t *G_buffers;
extern typed_buffer_descr_t G_buf_descr[];
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
/*extern typed_buffer_descr_t * get_buffer_descr(char *type, char *subtype);*/
extern char * _tprealloc (char *buf, long len);
extern char * _tpalloc (typed_buffer_descr_t *known_type,
                    char *type, char *subtype, long len);
extern buffer_obj_t * find_buffer(char *ptr);
/*extern void free_up_buffers(void);*/

/* UBF support */
extern int UBF_prepare_outgoing (typed_buffer_descr_t *descr, char *idata, long ilen, char *obuf, long *olen, long flags);
extern int UBF_prepare_incoming (typed_buffer_descr_t *descr, char *rcv_data, long rcv_len, char **odata, long *olen, long flags);
extern char * UBF_tprealloc(typed_buffer_descr_t *descr, char *cur_ptr, long len);
extern char	* UBF_tpalloc (typed_buffer_descr_t *descr, long len);
extern void UBF_tpfree(typed_buffer_descr_t *descr, char *buf);
extern int UBF_test(typed_buffer_descr_t *descr, char *buf, BFLDLEN len, char *expr);
    
/* Type buffer support */
extern char * TPINIT_tpalloc (typed_buffer_descr_t *descr, long len);
extern void TPINIT_tpfree(typed_buffer_descr_t *descr, char *buf);
/* Type null buffer */
extern char * TPNULL_tpalloc (typed_buffer_descr_t *descr, long len);
extern void TPNULL_tpfree(typed_buffer_descr_t *descr, char *buf);

/* TODO: Add support for string buffers */
#ifdef	__cplusplus
}
#endif

#endif	/* TYPED_BUF_H */

