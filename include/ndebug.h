/* 
** Enduro/X Debug library header
**
** @file ndebug.h
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
#ifndef NDEBUG_H
#define	NDEBUG_H

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
    
/**
 * Log levels
 */
#define log_always      1 
#define log_error       2
#define log_warn        3
#define log_info        4
#define log_debug       5
#define log_dump        6

/*
 * Logging settings
 */
#define LOG_FACILITY_NDRX       0x00001 /* settings for ATMI logging             */
#define LOG_FACILITY_UBF        0x00002 /* settings for UBF logging              */
#define LOG_FACILITY_TP         0x00004 /* settings for TP logging               */
#define LOG_FACILITY_TP_THREAD  0x00008 /* settings for TP, thread based logging */
#define LOG_FACILITY_TP_REQUEST 0x00010 /* Request logging, thread based         */
    
#define LOG_CODE_NDRX       'N'
#define LOG_CODE_UBF        'U'
#define LOG_CODE_TP         't'
#define LOG_CODE_TP_THREAD  'T'
#define LOG_CODE_TP_REQUEST 'R'
    
#define DBG_MAXLEV log_dump

#define NDRX_DBG_INIT_ENTRY    if (G_ndrx_debug_first) {ndrx_dbg_lock(); ndrx_init_debug(); ndrx_dbg_unlock();}

#define UBF_DBG_INIT(X) (ndrx_dbg_init X )
#define NDRX_DBG_INIT(X) (ndrx_dbg_init X )

/* Kind of GCC way only... */
#define NDRX_LOG(lev, fmt, ...) {NDRX_DBG_INIT_ENTRY; if (lev<=G_ndrx_debug.level)\
            {__ndrx_debug__(&G_ndrx_debug, lev, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__);}}

#define UBF_LOG(lev, fmt, ...) {NDRX_DBG_INIT_ENTRY; if (lev<=G_ubf_debug.level)\
            {__ndrx_debug__(&G_ubf_debug, lev, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__);}}
/* User logging */
#define TP_LOG(lev, fmt, ...) {NDRX_DBG_INIT_ENTRY; if (lev<=G_tp_debug.level)\
            {__ndrx_debug__(&G_tp_debug, lev, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__);}}


#define UBF_DUMP(lev,comment,ptr,len) {NDRX_DBG_INIT_ENTRY; if (lev<=G_ndrx_debug.level)\
            {__ndrx_debug_dump__(&G_ndrx_debug, lev, __FILE__, __LINE__, __func__, comment, ptr, len);}}

#define NDRX_DUMP(lev,comment,ptr,len) {NDRX_DBG_INIT_ENTRY; if (lev<=G_ndrx_debug.level)\
            {__ndrx_debug_dump__(&G_ubf_debug, lev, __FILE__, __LINE__, __func__, comment, ptr, len);}}

#define STDOUT_DUMP(lev,comment,ptr,len) {NDRX_DBG_INIT_ENTRY; if (lev<=G_stdout_debug.level)\
            {__ndrx_debug_dump__(&G_stdout_debug, lev, __FILE__, __LINE__, __func__, comment, ptr, len);}}

#define TP_DUMP(lev,comment,ptr,len) {NDRX_DBG_INIT_ENTRY; if (lev<=G_tp_debug.level)\
            {__ndrx_debug_dump__(&G_tp_debug, lev, __FILE__, __LINE__, __func__, comment, ptr, len);}}



#define UBF_DUMP_DIFF(lev,comment,ptr,ptr2,len) {NDRX_DBG_INIT_ENTRY; if (lev<=G_ndrx_debug.level)\
            {__ndrx_debug_dump_diff__(&G_ndrx_debug, lev, __FILE__, __LINE__, __func__, comment, ptr, ptr2, len);}}

#define NDRX_DUMP_DIFF(lev,comment,ptr,ptr2,len) {NDRX_DBG_INIT_ENTRY; if (lev<=G_ndrx_debug.level)\
            {__ndrx_debug_dump_diff__(&G_ubf_debug, lev, __FILE__, __LINE__, __func__, comment, ptr, ptr2, len);}}

#define TP_DUMP_DIFF(lev,comment,ptr,ptr2,len) {NDRX_DBG_INIT_ENTRY; if (lev<=G_tp_debug.level)\
            {__ndrx_debug_dump_diff__(&G_tp_debug, lev, __FILE__, __LINE__, __func__, comment, ptr, ptr2, len);}}

#define NDRX_DBG_SETTHREAD(X) ndrx_dbg_setthread(X)
    
/*---------------------------Externs------------------------------------*/
extern NDRX_API ndrx_debug_t G_ubf_debug;
extern NDRX_API ndrx_debug_t G_tp_debug;
extern NDRX_API ndrx_debug_t G_ndrx_debug;
extern NDRX_API ndrx_debug_t G_stdout_debug;
extern NDRX_API volatile int G_ndrx_debug_first;
/*---------------------------Macros-------------------------------------*/

/* Standard replacements for system errors */
/* Debug msg for malloc: */
#define NDRX_ERR_MALLOC(SZ)        NDRX_LOG(log_error, "Failed to allocate %ld bytes", SZ)

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
/* Real externals... */
extern NDRX_API void ndrx_dbg_init(char *module, char *config_key);
extern NDRX_API void ndrx_dbg_setlev(ndrx_debug_t *dbg_ptr, int level);
extern NDRX_API int debug_get_ndrx_level(void);
extern NDRX_API int debug_get_ubf_level(void);
extern NDRX_API ndrx_debug_t * debug_get_ndrx_ptr(void);
extern NDRX_API ndrx_debug_t * debug_get_ubf_ptr(void);
extern NDRX_API ndrx_debug_t * debug_get_tp_ptr(void);

extern NDRX_API void __ndrx_debug__(ndrx_debug_t *dbg_ptr, int lev, const char *file, 
        long line, const char *func, char *fmt, ...);

extern NDRX_API void __ndrx_debug_dump_diff__(ndrx_debug_t *dbg_ptr, int lev, const char *file, 
        long line, const char *func, char *comment, void *ptr, void *ptr2, long len);

extern NDRX_API void __ndrx_debug_dump__(ndrx_debug_t *dbg_ptr, int lev, const char *file, 
        long line, const char *func, char *comment, void *ptr, long len);

extern NDRX_API void ndrx_dbg_lock(void);
extern NDRX_API void ndrx_dbg_unlock(void);
extern NDRX_API void ndrx_init_debug(void);
extern NDRX_API void ndrx_dbg_setthread(long threadnr);
extern NDRX_API int ndrx_init_parse_line(char *in_tok1, char *in_tok2, int *p_finish_off,
        ndrx_debug_t *dbg_ptr);

/* TPLOG: */
extern NDRX_API void tplogdumpdiff(int lev, char *comment, void *ptr1, void *ptr2, int len);
extern NDRX_API void tplogdump(int lev, char *comment, void *ptr, int len);
extern NDRX_API void tplog(int lev, char *message);
extern NDRX_API int tploggetreqfile(char *filename, int bufsize);
extern NDRX_API int tplogconfig(int logger, int lev, char *debug_string, char *module, 
        char *new_file);
extern NDRX_API void tplogclosereqfile(void);
extern NDRX_API void tplogclosethread(void);
extern NDRX_API void tplogsetreqfile_direct(char *filename);

#ifdef	__cplusplus
}
#endif

#endif	/* NDEBUG_H */

