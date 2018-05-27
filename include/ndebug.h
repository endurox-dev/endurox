/* 
** Enduro/X Debug library header
**
** @file ndebug.h
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
#include <userlog.h>
    
/*---------------------------Externs------------------------------------*/
extern NDRX_API ndrx_debug_t G_ubf_debug;
extern NDRX_API ndrx_debug_t G_tp_debug;
extern NDRX_API ndrx_debug_t G_ndrx_debug;
extern NDRX_API ndrx_debug_t G_stdout_debug;
extern NDRX_API volatile int G_ndrx_debug_first;
/*---------------------------Macros-------------------------------------*/

/*
 * Log levels
 */
#define log_always                  1 
#define log_error                   2
#define log_warn                    3
#define log_info                    4
#define log_debug                   5
#define log_dump                    6

/*
 * Logging settings
 */
/** settings for ATMI logging        */
#define LOG_FACILITY_NDRX           0x00000001
/** settings for UBF logging         */
#define LOG_FACILITY_UBF            0x00000002
/** settings for TP logging          */
#define LOG_FACILITY_TP             0x00000004
/** settings for TP, thread based logging */
#define LOG_FACILITY_TP_THREAD      0x00000008
/** tp, Request logging, thread based*/
#define LOG_FACILITY_TP_REQUEST     0x00000010
/** ndrx thread logging              */
#define LOG_FACILITY_NDRX_THREAD    0x00000020
/** ubf thread logging               */
#define LOG_FACILITY_UBF_THREAD     0x00000040
/** ndrx request logging             */
#define LOG_FACILITY_NDRX_REQUEST   0x00000080
/** ubf request logging              */
#define LOG_FACILITY_UBF_REQUEST    0x00000100

/** Mask of the log facility bitwise flags */
#define LOG_FACILITY_MASK           0x0000ffff

/** first byte is reserved for log level (should not collide with LOG_FACILITY!) */
#define TPLOGQI_RET_HAVDETAILED     0x00010000
/** Bit offset for log level in return of info query */
#define TPLOGQI_RET_DBGLEVBITS      24

/** Macros for extracting log level from query results */
#define TPLOGQI_RET_DBGLEVGET(LEV)  LEV >> TPLOGQI_RET_DBGLEVBITS

/** Get log level for NDRXD        */
#define TPLOGQI_GET_NDRX            0x00000001
/** Get log level for UBF        */
#define TPLOGQI_GET_UBF             0x00000002
/** Get level for tp logger */
#define TPLOGQI_GET_TP              0x00000004
/** Eval detailed flag  */
#define TPLOGQI_EVAL_DETAILED       0x00000008
/** Return results (log level & detailed flag) event logging shall not be done */
#define TPLOGQI_EVAL_RETURN         0x00000010


/** Integration flags, detailed logging (incl. stack backtrace) */
#define LOG_IFLAGS_DETAILED         "detailed"

    
#define LOG_CODE_NDRX               'N'
#define LOG_CODE_UBF                'U'
#define LOG_CODE_TP                 't'
#define LOG_CODE_TP_THREAD          'T'
#define LOG_CODE_TP_REQUEST         'R'

#define LOG_CODE_NDRX_THREAD        'n'
#define LOG_CODE_NDRX_REQUEST       'm'

#define LOG_CODE_UBF_THREAD         'u'
#define LOG_CODE_UBF_REQUEST        'v'
    
#define NDRX_DBG_MAX_LEV log_dump
/* Have double check on G_ndrx_debug_first, as on after getting first mutex, object
 * might be already initialized
 */
#define NDRX_DBG_INIT_ENTRY    if (G_ndrx_debug_first) {ndrx_dbg_lock(); \
    if (G_ndrx_debug_first) {ndrx_init_debug();} ndrx_dbg_unlock();}

#define UBF_DBG_INIT(X) (ndrx_dbg_init X )
#define NDRX_DBG_INIT(X) (ndrx_dbg_init X )

/*
 * This logger can be used for very early bootstrapping logs - logging when
 * logger is not yet initialised.
 * Thus we run alternate route in case init lock is acquired.
 */
#define NDRX_LOG_EARLY(lev, fmt, ...) {if (ndrx_dbg_intlock_isset()) {\
    __ndrx_debug__(&G_ndrx_debug, lev, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__);} else {\
    NDRX_DBG_INIT_ENTRY; if (lev<=G_ndrx_debug.level)\
    {__ndrx_debug__(&G_ndrx_debug, lev, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__);}}}

#define UBF_LOG_EARLY(lev, fmt, ...) {if (ndrx_dbg_intlock_isset()) {\
    __ndrx_debug__(&G_ubf_debug, lev, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__);} else {\
    NDRX_DBG_INIT_ENTRY; if (lev<=G_ubf_debug.level)\
    {__ndrx_debug__(&G_ubf_debug, lev, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__);}}}

#define TP_LOG_EARLY(lev, fmt, ...) {if (ndrx_dbg_intlock_isset()) {\
    __ndrx_debug__(&G_tp_debug, lev, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__);} else {\
    NDRX_DBG_INIT_ENTRY; if (lev<=G_tp_debug.level)\
    {__ndrx_debug__(&G_tp_debug, lev, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__);}}}

/*
 * Normal loggers 
 */
#define NDRX_LOG(lev, fmt, ...) {NDRX_DBG_INIT_ENTRY; if (lev<=G_ndrx_debug.level)\
    {__ndrx_debug__(&G_ndrx_debug, lev, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__);}}

#define NDRX_LOGEX(lev, file, line, fmt, ...) {NDRX_DBG_INIT_ENTRY; if (lev<=G_ndrx_debug.level)\
    {__ndrx_debug__(&G_ndrx_debug, lev, file, line, __func__, fmt, ##__VA_ARGS__);}}

#define UBF_LOG(lev, fmt, ...) {NDRX_DBG_INIT_ENTRY; if (lev<=G_ubf_debug.level)\
    {__ndrx_debug__(&G_ubf_debug, lev, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__);}}

#define UBF_LOGEX(lev, file, line, fmt, ...) {NDRX_DBG_INIT_ENTRY; if (lev<=G_ubf_debug.level)\
    {__ndrx_debug__(&G_ubf_debug, lev, file, line, __func__, fmt, ##__VA_ARGS__);}}

/* User logging */
#define TP_LOG(lev, fmt, ...) {NDRX_DBG_INIT_ENTRY; if (lev<=G_tp_debug.level)\
    {__ndrx_debug__(&G_tp_debug, lev, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__);}}

/* Extended user logging, with filename name and line */
#define TP_LOGEX(lev, file, line, fmt, ...) {NDRX_DBG_INIT_ENTRY; if (lev<=G_tp_debug.level)\
    {__ndrx_debug__(&G_tp_debug, lev, file, line, __func__, fmt, ##__VA_ARGS__);}}

#define TP_LOGGETIFLAGS {NDRX_DBG_INIT_ENTRY; return G_tp_debug.iflags; }

#define UBF_DUMP(lev,comment,ptr,len) {NDRX_DBG_INIT_ENTRY; if (lev<=G_ubf_debug.level)\
    {__ndrx_debug_dump__(&G_ubf_debug, lev, __FILE__, __LINE__, __func__, comment, ptr, len);}}

#define NDRX_DUMP(lev,comment,ptr,len) {NDRX_DBG_INIT_ENTRY; if (lev<=G_ndrx_debug.level)\
    {__ndrx_debug_dump__(&G_ndrx_debug, lev, __FILE__, __LINE__, __func__, comment, ptr, len);}}

#define STDOUT_DUMP(lev,comment,ptr,len) {NDRX_DBG_INIT_ENTRY; if (lev<=G_stdout_debug.level)\
    {__ndrx_debug_dump__(&G_stdout_debug, lev, __FILE__, __LINE__, __func__, comment, ptr, len);}}

#define TP_DUMP(lev,comment,ptr,len) {NDRX_DBG_INIT_ENTRY; if (lev<=G_tp_debug.level)\
    {__ndrx_debug_dump__(&G_tp_debug, lev, __FILE__, __LINE__, __func__, comment, ptr, len);}}


#define UBF_DUMP_DIFF(lev,comment,ptr,ptr2,len) {NDRX_DBG_INIT_ENTRY; if (lev<=G_ubf_debug.level)\
    {__ndrx_debug_dump_diff__(&G_ubf_debug, lev, __FILE__, __LINE__, __func__, comment, ptr, ptr2, len);}}

#define NDRX_DUMP_DIFF(lev,comment,ptr,ptr2,len) {NDRX_DBG_INIT_ENTRY; if (lev<=G_ndrx_debug.level)\
    {__ndrx_debug_dump_diff__(&G_ndrx_debug, lev, __FILE__, __LINE__, __func__, comment, ptr, ptr2, len);}}

#define TP_DUMP_DIFF(lev,comment,ptr,ptr2,len) {NDRX_DBG_INIT_ENTRY; if (lev<=G_tp_debug.level)\
    {__ndrx_debug_dump_diff__(&G_tp_debug, lev, __FILE__, __LINE__, __func__, comment, ptr, ptr2, len);}}

#define NDRX_DBG_SETTHREAD(X) ndrx_dbg_setthread(X)

/* Standard replacements for system errors */
/* Debug msg for malloc: */
#define NDRX_ERR_MALLOC(SZ)        NDRX_LOG(log_error, "Failed to allocate %ld bytes", SZ)

/* Memory debug macros */

#ifdef NDRX_MEMORY_DEBUG

#define NDRX_MALLOC(size) ndrx_malloc_dbg(size, __LINE__, __FILE__, __func__)
#define NDRX_FREE(ptr) ndrx_free_dbg(ptr, __LINE__, __FILE__, __func__)
#define NDRX_CALLOC(nmemb, size) ndrx_calloc_dbg(nmemb, size, __LINE__, __FILE__, __func__)
#define NDRX_REALLOC(ptr, size) ndrx_realloc_dbg(ptr, size, __LINE__, __FILE__, __func__)
#define NDRX_STRDUP(ptr) ndrx_strdup_dbg(ptr, __LINE__, __FILE__, __func__)

#define NDRX_FOPEN(path, mode) ndrx_fopen_dbg(path, mode, __LINE__, __FILE__, __func__)
#define NDRX_FCLOSE(fp) ndrx_fclose_dbg(fp,__LINE__, __FILE__, __func__)

#else

#define NDRX_MALLOC(size) malloc(size)
#define NDRX_FREE(ptr) free(ptr)
#define NDRX_CALLOC(nmemb, size) calloc(nmemb, size)
#define NDRX_REALLOC(ptr, size) realloc(ptr, size)
#define NDRX_STRDUP(ptr) strdup(ptr)

#define NDRX_FOPEN(path, mode) fopen(path, mode)
#define NDRX_FCLOSE(fp) fclose(fp)


#endif


/* Quick macros for standard memory allocation with error printing and
 * returning to out with ret flag
 */

/**
 * Allocate buffer, if error goto out. Do not init memory to 0
 * @param PTR__ pointer to give the address to 
 * @param SIZE__ number of bytes to allocate
 * @param TYPE__ type to allocated buffer (dest type not ptr)
 */
#define NDRX_MALLOC_OUT(PTR__, SIZE__, TYPE__) \
if ( (PTR__ = (TYPE__ *)NDRX_MALLOC(SIZE__)) == NULL) \
{\
    int ERR__ = errno;\
    NDRX_LOG(log_error, "%s: Failed to mallocate %ld bytes: %s",\
        __func__, (long)SIZE__, strerror(ERR__));\
    userlog("%s: Failed to mallocate %ld bytes: %s",\
        __func__, (long)SIZE__, strerror(ERR__));\
    EXFAIL_OUT(ret);\
}

/**
 * Duplicate the string and in case of error fail out in standard way
 * @param DST__ dest pointer
 * @param SRC__ source pointer
 */
#define NDRX_STRDUP_OUT(DST__, SRC__) \
if ( (DST__ = NDRX_STRDUP(SRC__)) == NULL) \
{\
    int ERR__ = errno;\
    NDRX_LOG(log_error, "%s: Failed to strdup [%s]: %s",\
        __func__, SRC__, strerror(ERR__));\
    userlog("%s: Failed to strdup [%s] bytes: %s",\
        __func__, SRC__, strerror(ERR__));\
    EXFAIL_OUT(ret);\
}

/**
 * Allocate buffer, if error goto out. Do init memory to 0
 * @param PTR__ pointer to give the address to 
 * @param NMEMB__ Number of members
 * @param SIZE__ number of bytes to allocate
 * @param TYPE__ type to allocated buffer (dest type not ptr)
 */
#define NDRX_CALLOC_OUT(PTR__, NMEMB__, SIZE__, TYPE__) \
if ( (PTR__ = (TYPE__ *)NDRX_CALLOC(NMEMB__, SIZE__)) == NULL) \
{\
    int ERR__ = errno;\
    NDRX_LOG(log_error, "%s: Failed to callocate %ld bytes: %s",\
        __func__, (long)(NMEMB__ *SIZE__), strerror(ERR__));\
    userlog("%s: Failed to mallocate %ld bytes: %s",\
        __func__, (long)(NMEMB__ *SIZE__), strerror(ERR__));\
    EXFAIL_OUT(ret);\
}

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
extern NDRX_API int debug_get_tp_level(void);
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
extern NDRX_API int ndrx_dbg_intlock_isset(void);
extern NDRX_API int ndrx_init_parse_line(char *in_tok1, char *in_tok2, int *p_finish_off, ndrx_debug_t *dbg_ptr);

/* TPLOG: */

extern NDRX_API void tplogdumpdiff(int lev, char *comment, void *ptr1, void *ptr2, int len);
extern NDRX_API void tplogdump(int lev, char *comment, void *ptr, int len);
extern NDRX_API void tplog(int lev, char *message);
extern NDRX_API long tplogqinfo(int lev, long flags);

/* extended logging: */
extern NDRX_API void tplogex(int lev, char *file, long line, char *message);
extern NDRX_API void ndrxlogex(int lev, char *file, long line, char *message);
extern NDRX_API void ubflogex(int lev, char *file, long line, char *message);

/* get integration flags: */
extern NDRX_API char * tploggetiflags(void);

extern NDRX_API void ndrxlogdumpdiff(int lev, char *comment, void *ptr1, void *ptr2, int len);
extern NDRX_API void ndrxlogdump(int lev, char *comment, void *ptr, int len);
extern NDRX_API void ndrxlog(int lev, char *message);

extern NDRX_API void ubflogdumpdiff(int lev, char *comment, void *ptr1, void *ptr2, int len);
extern NDRX_API void ubflogdump(int lev, char *comment, void *ptr, int len);
extern NDRX_API void ubflog(int lev, char *message);

extern NDRX_API int tploggetreqfile(char *filename, int bufsize);
extern NDRX_API int tplogconfig(int logger, int lev, char *debug_string, char *module, char *new_file);
extern NDRX_API void tplogclosereqfile(void);
extern NDRX_API void tplogclosethread(void);
extern NDRX_API void tplogsetreqfile_direct(char *filename);
extern NDRX_API void ndrx_nstd_tls_loggers_close(nstd_tls_t *tls);

/* memory debugging */
extern NDRX_API void *ndrx_malloc_dbg(size_t size, long line, const char *file, const char *func);
extern NDRX_API void ndrx_free_dbg(void *ptr, long line, const char *file, const char *func);
extern NDRX_API void *ndrx_calloc_dbg(size_t nmemb, size_t size, long line, const char *file, const char *func);
extern NDRX_API void *ndrx_realloc_dbg(void *ptr, size_t size, long line, const char *file, const char *func);


extern NDRX_API FILE *ndrx_fopen_dbg(const char *path, const char *mode, long line, const char *file, const char *func);
extern NDRX_API int ndrx_fclose_dbg(FILE *fp, long line, const char *file, const char *func);
extern NDRX_API char *ndrx_strdup_dbg(char *ptr, long line, const char *file, const char *func);


/* Bootstrapping: */
extern NDRX_API int ndrx_dbg_intlock_isset(void);
extern NDRX_API void ndrx_dbg_intlock_set(void);
extern NDRX_API void ndrx_dbg_intlock_unset(void);

#ifdef	__cplusplus
}
#endif

#endif	/* NDEBUG_H */

