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
#include <stdio.h>
#include <limits.h>
#include <thlock.h>
#include <stdarg.h>

/* Create main debug structure */
typedef struct
{
    int   level;
    FILE *dbg_f_ptr;
    char filename[PATH_MAX];
    pid_t pid;
    int buf_lines;
    int buffer_size;
    int lines_written;
} ndrx_debug_t;

/**
 * Log levels
 */
#define log_always      1 
#define log_error       2
#define log_warn        3
#define log_info        4
#define log_debug       5
#define log_dump        6

#define DBG_MAXLEV log_dump

#define NDRX_DBG_INIT_ENTRY    if (G_ndrx_debug_first) {ndrx_dbg_lock(); ndrx_init_debug(); ndrx_dbg_unlock();}

#define UBF_DBG_INIT(X) (ndrx_dbg_init X )
#define NDRX_DBG_INIT(X) (ndrx_dbg_init X )

#define UBF_DBG_SETLEV(lev) (ndrx_dbg_setlev(&G_ubf_debug, lev))
#define NDRX_DBG_SETLEV(lev) (ndrx_dbg_setlev(&G_ndrx_debug, lev))

/* Kind of GCC way only... */
#define NDRX_LOG(lev, fmt, ...) {NDRX_DBG_INIT_ENTRY; if (lev<=G_ndrx_debug.level) {__ndrx_debug__(&G_ndrx_debug, lev, "NDRX", __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__);}}
#define UBF_LOG(lev, fmt, ...) {NDRX_DBG_INIT_ENTRY; if (lev<=G_ubf_debug.level) {__ndrx_debug__(&G_ubf_debug, lev, "UBF ", __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__);}}


#define UBF_DUMP(lev,comment,ptr,len) {NDRX_DBG_INIT_ENTRY; if (lev<=G_ndrx_debug.level) {__ndrx_debug_dump__(&G_ndrx_debug, lev, "UBF ", __FILE__, __LINE__, __func__, comment, ptr, len);}}
#define NDRX_DUMP(lev,comment,ptr,len) {NDRX_DBG_INIT_ENTRY; if (lev<=G_ndrx_debug.level) {__ndrx_debug_dump__(&G_ubf_debug, lev, "NDRX ", __FILE__, __LINE__, __func__, comment, ptr, len);}}
#define STDOUT_DUMP(lev,comment,ptr,len) {NDRX_DBG_INIT_ENTRY; if (lev<=G_stdout_debug.level) {__ndrx_debug_dump__(&G_stdout_debug, lev, "UBF ", __FILE__, __LINE__, __func__, comment, ptr, len);}}

#define UBF_DUMP_DIFF(lev,comment,ptr,len) {NDRX_DBG_INIT_ENTRY; if (lev<=G_ndrx_debug.level) {__ndrx_debug_dump__(&G_ndrx_debug, lev, "UBF ", __FILE__, __LINE__, __func__, comment, ptr, ptr2, len);}}
#define NDRX_DUMP_DIFF(lev,comment,ptr,len) {NDRX_DBG_INIT_ENTRY; if (lev<=G_ndrx_debug.level) {__ndrx_debug_dump__(&G_ubf_debug, lev, "NDRX", __FILE__, __LINE__, __func__, comment, ptr, ptr2, len);}}

#define NDRX_DBG_SETTHREAD(X) ndrx_dbg_setthread(X)
    
/*---------------------------Externs------------------------------------*/
extern ndrx_debug_t G_ubf_debug;
extern ndrx_debug_t G_ndrx_debug;
extern ndrx_debug_t G_stdout_debug;
extern volatile int G_ndrx_debug_first;
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
extern void ndrx_dbg_init(char *module, char *config_key);
extern int debug_get_ndrx_level(void);
extern int debug_get_ubf_level(void);
extern ndrx_debug_t * debug_get_ndrx_ptr(void);
extern ndrx_debug_t * debug_get_ubf_ptr(void);

extern void __ndrx_debug__(ndrx_debug_t *dbg_ptr, int lev, char *mod, const char *file, 
        long line, const char *func, char *fmt, ...);

extern void __ndrx_debug_dump_diff__(ndrx_debug_t *dbg_ptr, int lev, char *mod, const char *file, 
        long line, const char *func, char *comment, void *ptr, void *ptr2, long len);

extern void __ndrx_debug_dump__(ndrx_debug_t *dbg_ptr, int lev, char *mod, const char *file, 
        long line, const char *func, char *comment, void *ptr, long len);

extern void ndrx_dbg_lock(void);
extern void ndrx_dbg_unlock(void);
extern void ndrx_init_debug(void);
extern void ndrx_dbg_setthread(long threadnr);

#ifdef	__cplusplus
}
#endif

#endif	/* NDEBUG_H */

