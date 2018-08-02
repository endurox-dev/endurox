/* 
 * @brief XATMI Typed buffer support
 *
 * @file typed_buf.h
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
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
#define BUF_TYPE_CARRAY     5 
#define BUF_TYPE_JSON       6
#define BUF_TYPE_VIEW       7
#define BUF_TYPE_MAX        7 /* max buffer type, for integrity */

#define BUF_TYPE_UBF_STR        "UBF"
#define BUF_TYPE_INIT_STR       "INIT"
#define BUF_TYPE_STRING_STR     "STRING"
#define BUF_TYPE_CARRAY_STR     "CARRAY"
#define BUF_TYPE_NULL_STR       "NULL"
#define BUF_TYPE_JSON_STR       "JSON"
#define BUF_TYPE_VIEW_STR       "VIEW"

/**
 * Automatic buffer conversion:
 */
#define BUF_CVT_INCOMING_JSON2UBF_STR           "JSON2UBF"
#define BUF_CVT_INCOMING_UBF2JSON_STR           "UBF2JSON"
#define BUF_CVT_INCOMING_JSON2VIEW_STR          "JSON2VIEW"
#define BUF_CVT_INCOMING_VIEW2JSON_STR          "VIEW2JSON"
    

/* others: VIEW X_COMMON X_C_TYPE X_OCTET FML32 VIEW32 - not supported currently */
/* see G_buf_descr */
#define BUF_IS_TYPEID_VALID(X) (0<=X && X <= 6)
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
extern NDRX_API buffer_obj_t *G_buffers;
extern NDRX_API typed_buffer_descr_t G_buf_descr[];
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
/*extern NDRX_API typed_buffer_descr_t * get_buffer_descr(char *type, char *subtype);*/
extern NDRX_API char * ndrx_tprealloc (char *buf, long len);
extern NDRX_API char * ndrx_tpalloc (typed_buffer_descr_t *known_type,
                    char *type, char *subtype, long len);
extern NDRX_API buffer_obj_t * ndrx_find_buffer(char *ptr);
extern NDRX_API typed_buffer_descr_t * ndrx_get_buffer_descr(char *type, 
        char *subtype);

/*extern NDRX_API void free_up_buffers(void);*/

/* UBF support */
extern NDRX_API int UBF_prepare_outgoing (typed_buffer_descr_t *descr, 
        char *idata, long ilen, char *obuf, long *olen, long flags);
extern NDRX_API int UBF_prepare_incoming (typed_buffer_descr_t *descr, 
        char *rcv_data, long rcv_len, char **odata, long *olen, long flags);
extern NDRX_API char * UBF_tprealloc(typed_buffer_descr_t *descr, char *cur_ptr, long len);
extern NDRX_API char	* UBF_tpalloc (typed_buffer_descr_t *descr, char *subtype, long *len);
extern NDRX_API void UBF_tpfree(typed_buffer_descr_t *descr, char *buf);
extern NDRX_API int UBF_test(typed_buffer_descr_t *descr, char *buf, BFLDLEN len, char *expr);
    
/* Type buffer support */
extern NDRX_API char * TPINIT_tpalloc (typed_buffer_descr_t *descr, char *subtype, long *len);
extern NDRX_API void TPINIT_tpfree(typed_buffer_descr_t *descr, char *buf);
/* Type null buffer */
extern NDRX_API char * TPNULL_tpalloc (typed_buffer_descr_t *descr, char *subtype, long *len);
extern NDRX_API void TPNULL_tpfree(typed_buffer_descr_t *descr, char *buf);

/* Automatic buffer convert: */
extern NDRX_API int typed_xcvt(buffer_obj_t **buffer, long xcvtflags, int is_reverse);

extern NDRX_API int typed_xcvt_json2ubf(buffer_obj_t **buffer);
extern NDRX_API int typed_xcvt_ubf2json(buffer_obj_t **buffer);

extern NDRX_API int typed_xcvt_json2view(buffer_obj_t **buffer);
extern NDRX_API int typed_xcvt_view2json(buffer_obj_t **buffer, long flags);

    
#ifdef	__cplusplus
}
#endif

#endif	/* TYPED_BUF_H */

