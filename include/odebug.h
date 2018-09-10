/**
 * @brief Enduro/X Object-API Debug library header, ATMI level
 *
 * @file odebug.h
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
#ifndef ODEBUG_H
#define	ODEBUG_H

#ifdef	__cplusplus
extern "C" {
#endif
/*---------------------------Includes-----------------------------------*/
#include <ndrx_config.h>
#include <stdio.h>
#include <limits.h>
#include <thlock.h>
#include <stdarg.h>
#include <ndebugcmn.h>
#include <nstd_tls.h>
#include <ndebug.h>
#include <atmi.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/* Kind of GCC way only... */
#define ONDRX_LOG(p_ctxt, lev, fmt, ...) {NDRX_DBG_INIT_ENTRY; if (lev<=G_ndrx_debug.level)\
            {ndrx_tpsetctxt(*p_ctxt, 0, CTXT_PRIV_NSTD); \
                __ndrx_debug__(&G_ndrx_debug, lev, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__);\
            ndrx_tpgetctxt(p_ctxt, 0, CTXT_PRIV_NSTD);\
            }}

#define OUBF_LOG(p_ctxt, lev, fmt, ...) {NDRX_DBG_INIT_ENTRY; if (lev<=G_ubf_debug.level)\
            {ndrx_tpsetctxt(*p_ctxt, 0, CTXT_PRIV_NSTD); \
            __ndrx_debug__(&G_ubf_debug, lev, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__);\
            ndrx_tpgetctxt(p_ctxt, 0, CTXT_PRIV_NSTD);\
            }}
    
/* User logging */
#define OTP_LOG(p_ctxt, lev, fmt, ...) {NDRX_DBG_INIT_ENTRY; if (lev<=G_tp_debug.level)\
        {ndrx_tpsetctxt(*p_ctxt, 0, CTXT_PRIV_NSTD); \
            __ndrx_debug__(&G_tp_debug, lev, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__);\
            ndrx_tpgetctxt(p_ctxt, 0, CTXT_PRIV_NSTD);\
            }}


#define OUBF_DUMP(p_ctxt, lev,comment,ptr,len) {NDRX_DBG_INIT_ENTRY; if (lev<=G_ndrx_debug.level)\
        {ndrx_tpsetctxt(*p_ctxt, 0, CTXT_PRIV_NSTD); \
            __ndrx_debug_dump__(&G_ndrx_debug, lev, __FILE__, __LINE__, __func__, comment, ptr, len);\
            ndrx_tpgetctxt(p_ctxt, 0, CTXT_PRIV_NSTD);\
            }}

#define ONDRX_DUMP(p_ctxt, lev,comment,ptr,len) {NDRX_DBG_INIT_ENTRY; if (lev<=G_ndrx_debug.level)\
        {ndrx_tpsetctxt(*p_ctxt, 0, CTXT_PRIV_NSTD); \
            __ndrx_debug_dump__(&G_ubf_debug, lev, __FILE__, __LINE__, __func__, comment, ptr, len);\
            ndrx_tpgetctxt(p_ctxt, 0, CTXT_PRIV_NSTD);\
            }}

#define OSTDOUT_DUMP(p_ctxt, lev,comment,ptr,len) {NDRX_DBG_INIT_ENTRY; if (lev<=G_stdout_debug.level)\
        {ndrx_tpsetctxt(*p_ctxt, 0, CTXT_PRIV_NSTD); \
            __ndrx_debug_dump__(&G_stdout_debug, lev, __FILE__, __LINE__, __func__, comment, ptr, len);\
            ndrx_tpgetctxt(p_ctxt, 0, CTXT_PRIV_NSTD);\
            }}

#define OTP_DUMP(p_ctxt, lev,comment,ptr,len) {NDRX_DBG_INIT_ENTRY; if (lev<=G_tp_debug.level)\
        {ndrx_tpsetctxt(*p_ctxt, 0, CTXT_PRIV_NSTD); \
            __ndrx_debug_dump__(&G_tp_debug, lev, __FILE__, __LINE__, __func__, comment, ptr, len);\
            ndrx_tpgetctxt(p_ctxt, 0, CTXT_PRIV_NSTD);\
            }}

#define OUBF_DUMP_DIFF(p_ctxt, lev,comment,ptr,ptr2,len) {NDRX_DBG_INIT_ENTRY; if (lev<=G_ndrx_debug.level)\
        {ndrx_tpsetctxt(*p_ctxt, 0, CTXT_PRIV_NSTD); \
            __ndrx_debug_dump_diff__(&G_ndrx_debug, lev, __FILE__, __LINE__, __func__, comment, ptr, ptr2, len);\
            ndrx_tpgetctxt(p_ctxt, 0, CTXT_PRIV_NSTD);\
            }}

#define ONDRX_DUMP_DIFF(p_ctxt, lev,comment,ptr,ptr2,len) {NDRX_DBG_INIT_ENTRY; if (lev<=G_ndrx_debug.level)\
        {ndrx_tpsetctxt(*p_ctxt, 0, CTXT_PRIV_NSTD); \
            __ndrx_debug_dump_diff__(&G_ubf_debug, lev, __FILE__, __LINE__, __func__, comment, ptr, ptr2, len);\
            ndrx_tpgetctxt(p_ctxt, 0, CTXT_PRIV_NSTD);\
            }}

#define OTP_DUMP_DIFF(p_ctxt, lev,comment,ptr,ptr2,len) {NDRX_DBG_INIT_ENTRY; if (lev<=G_tp_debug.level)\
        {ndrx_tpsetctxt(*p_ctxt, 0, CTXT_PRIV_NSTD); \
            __ndrx_debug_dump_diff__(&G_tp_debug, lev, __FILE__, __LINE__, __func__, comment, ptr, ptr2, len);\
            ndrx_tpgetctxt(p_ctxt, 0, CTXT_PRIV_NSTD);\
            }}
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

#ifdef	__cplusplus
}
#endif

#endif	/* ODEBUG_H */

/* vim: set ts=4 sw=4 et smartindent: */
