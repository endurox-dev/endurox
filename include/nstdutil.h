/**
 * @brief NDR 'standard' common utilites
 *   Enduro Execution system platform library
 *
 * @file nstdutil.h
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2019, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * AGPL (with Java and Go exceptions) or Mavimax's license for commercial use.
 * See LICENSE file for full text.
 * -----------------------------------------------------------------------------
 * AGPL license:
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License, version 3 as published
 * by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU Affero General Public License, version 3
 * for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along 
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * -----------------------------------------------------------------------------
 * A commercial use license is available from Mavimax, Ltd
 * contact@mavimax.com
 * -----------------------------------------------------------------------------
 */
#ifndef NSTDUTIL_H
#define	NSTDUTIL_H

#ifdef	__cplusplus
extern "C" {
#endif
/*---------------------------Includes-----------------------------------*/
#include <ndrx_config.h>
#include <stdint.h>
#include <unistd.h>
#include <exhash.h>
#include <arpa/inet.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
    
/**
 * Quick env subst. via static buffer
 */
#define NDRX_QENV_SUBST(t, p) strncpy(t, p, sizeof(t)-1);\
                t[sizeof(t)-1] = (char)0;\
                ndrx_str_env_subs_len(t, sizeof(t));

   
/**
 * Access the GROWLIST element by index and cast to the type
 * @param LIST__ pointer to growlist
 * @param INDEX__ index to be accessed
 * @param TYPE__ pointer type
 */
#define NDRX_GROWLIST_ACCESS(LIST__, INDEX__, TYPE__) \
    (TYPE__*)(((char *)(LIST__)->mem) + (INDEX__) * (LIST__)->size)
            
#define NDRX_LOCALE_STOCK_DECSEP        '.'    /**< default decimal seperator */
    
/**
 * @defgroup argsgrp Value types for ndrx_args_ functions
 * @{
 */
#define NDRX_ARGS_BOOL                  1     /**< boolean type               */
#define NDRX_ARGS_INT                   2     /**< integer type               */
/** @} */ /* end of argsgrp */
    
    
#define NDRX_STOR_KBYTE                 1024    /**< number of bytes in KB    */


#if EX_SIZEOF_LONG==4 && EX_SIZEOF_INT==4

# define htonll(x) htonl(x)
# define ntohll(x) ntohl(x)

#else

#if __BIG_ENDIAN__

#ifndef htonll
# define htonll(x) (x)
#endif

#ifndef ntohll
# define ntohll(x) (x)
#endif

#else
#ifndef htonll
# define htonll(x) (((uint64_t)htonl( (x) & 0xFFFFFFFF) << 32) | htonl( (x) >> 32))
#endif
#ifndef ntohll
# define ntohll(x) (((uint64_t)ntohl( (x) & 0xFFFFFFFF) << 32) | ntohl( (x) >> 32))
#endif
#endif

#endif


/**
 * Linear hashing flags, used for short data type
 * @defgroup linhash Linear hashing value flags
 * @{
 */    
#define NDRX_LH_FLAG_ISUSED         0x0001  /**< Entry is used              */
#define NDRX_LH_FLAG_WASUSED        0x0002  /**< Entry is used              */
/** @} */ /* end of linhash */
    
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

typedef struct longstrmap longstrmap_t;
struct longstrmap
{
    long from;
    char *to;
};

typedef struct charstrmap charstrmap_t;
struct charstrmap
{
    long from;
    char *to;
};

/** Keeps the growing list */
typedef struct ndrx_growlist ndrx_growlist_t;
/** Linear array growing support structure */
struct ndrx_growlist
{
    /** number of items allocated */
    int itemsalloc;
    /** allocate increment step */
    int step;
    
    /** Maximum index used by add function, -1 if not added any elm */
    int maxindexused;
    
    /** elm size */
    size_t size;
    
    /** memory pointer alloc/realloc */
    void *mem;
};


/**
 * List of posix queues
 */
typedef struct string_list string_list_t;
struct string_list
{
    char *qname;
    string_list_t *next;
};


typedef struct string_hash string_hash_t;
struct string_hash
{
    char *str;
    EX_hash_handle hh;
};

/**
 * This is arguments parser and loader for mapped structures
 */
struct ndrx_args_loader
{
    long    offset;             /**< field offset                           */
    size_t  elmsz;              /**< element size                           */
    char*   cname;              /**< field name                             */
    int     fld_type;           /**< field type, see  EXF_*                 */
    int     min_len;            /**< string min len                         */
    int     max_len;            /**< string max len                         */
    double  min_value;          /**< minimum value for field                */
    double  max_value;          /**< maximum value for the field            */
};

typedef struct ndrx_args_loader ndrx_args_loader_t;


/**
 * Linear memory hash
 */
typedef struct ndrx_lh_config ndrx_lh_config_t;

/**
 * Linear hashing config
 */
struct ndrx_lh_config
{
    /** memory pointer */
    void **memptr;
    
    /** Max number of elements */
    int elmmax;
    
    /** Element size */
    size_t elmsz;
    
    /** Key hash func */
    int (*p_key_hash)(ndrx_lh_config_t *conf, void *key_get, size_t key_len);
    
    /** Flags (short type) offset in element */
    int  flags_offset;
    
    /** Key hash func   */
    void (*p_key_debug)(ndrx_lh_config_t *conf, void *key_get, size_t key_len, 
        char *dbg_out, size_t dbg_len);
    
    /** Value debug     */
    void (*p_val_debug)(ndrx_lh_config_t *conf, int idx, char *dbg_out, size_t dbg_len);
    
    /** Compare value at index, ret 0 if equals */
    int (*p_compare)(ndrx_lh_config_t *conf, void *key_get, size_t key_len, int idx);
};

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

extern NDRX_API int ndrx_string_list_splitadd(string_list_t**list, char *string, char *sep);
extern NDRX_API int ndrx_string_list_add(string_list_t**list, char *string);
extern NDRX_API void ndrx_string_list_free(string_list_t* list);

extern NDRX_API void ndrx_growlist_init(ndrx_growlist_t *list, int step, size_t size);
extern NDRX_API int ndrx_growlist_add(ndrx_growlist_t *list, void *item, int index);
extern NDRX_API int ndrx_growlist_append(ndrx_growlist_t *list, void *item);
extern NDRX_API void ndrx_growlist_free(ndrx_growlist_t *list);

extern NDRX_API void ndrx_get_dt_local(long *p_date, long *p_time, long *p_usec);
extern NDRX_API long ndrx_timespec_get_delta(struct timespec *stop, struct timespec *start);

extern NDRX_API long ndrx_ceil(long x, long y);
extern NDRX_API unsigned long long ndrx_utc_tstamp(void);
extern NDRX_API unsigned long long ndrx_utc_tstamp_micro(void);
extern NDRX_API char * ndrx_get_strtstamp_from_sec(int slot, long ts);
extern NDRX_API unsigned long long ndrx_get_micro_resolution_for_sec(void);
extern NDRX_API char * ndrx_str_env_subs(char * str);
extern NDRX_API char * ndrx_str_env_subs_len(char * str, int buf_size);
extern NDRX_API int ndrx_str_subs_context(char * str, int buf_size, char opensymb, char closesymb,
        void *data1, void *data2, void *data3, void *data4,
        int (*pf_get_data) (void *data1, void *data2, void *data3, void *data4,
            char *symbol, char *outbuf, long outbufsz));
extern NDRX_API char *ndrx_str_replace(char *orig, char *rep, char *with);
extern NDRX_API char *ndrx_strchr_repl (char *str, char from_char, char to_char);
extern NDRX_API void ndrx_utc_tstamp2(long *t, long *tusec);
extern NDRX_API int ndrx_utc_cmp(long *t1, long *tusec1, long *t2, long *tusec2);
extern NDRX_API char * ndrx_get_strtstamp2(int slot, long t, long tusec);
extern NDRX_API int ndrx_compare3(long a1, long a2, long a3, long b1, long b2, long b3);

extern NDRX_API char *ndrx_decode_num(long tt, int slot, int level, int levels);
extern NDRX_API double ndrx_num_dec_parsecfg(char * str);
extern NDRX_API double ndrx_num_time_parsecfg(char * str);
extern NDRX_API char *ndrx_str_strip(char *haystack, char *needle);
extern NDRX_API char* ndrx_str_rstrip(char* s, char *needle);
extern NDRX_API char* ndrx_str_lstrip_ptr(char* s, char *needle);

extern NDRX_API int ndrx_isint(char *str);
extern NDRX_API int ndrx_nr_chars(char *str, char chkchar);

extern NDRX_API int ndrx_file_exists(char *filename);
extern NDRX_API int ndrx_file_regular(char *path);
extern NDRX_API char * ndrx_fgets_stdin_strip(char *buf, int bufsz);

extern NDRX_API char * ndrx_get_executable_path(char * out_path, size_t bufsz, 
        char * in_binary);
extern NDRX_API int ndrx_get_cksum(char *file);
extern NDRX_API ssize_t ndrx_getline(char **lineptr, size_t *n, FILE *stream);
extern NDRX_API char * ndrx_memdup(char *org, size_t len);
extern NDRX_API int ndrx_tokens_extract(char *str1, char *fmt, void *tokens, 
        int tokens_elmsz, int len, int start_tok, int stop_tok);
extern NDRX_API void ndrx_chomp(char *str);
extern NDRX_API uint32_t ndrx_rotl32b (uint32_t x, uint32_t n);
extern NDRX_API int ndrx_proc_get_line(int line_no, char *cmd, char *buf, int bufsz);

extern NDRX_API size_t ndrx_strnlen(char *str, size_t max);

extern NDRX_API double ndrx_atof(char *str);

/* Mapping functions: */
extern NDRX_API char *ndrx_dolongstrgmap(longstrmap_t *map, long val, long endval);
extern NDRX_API char *ndrx_docharstrgmap(longstrmap_t *map, char val, char endval);

/* Threading functions */
extern NDRX_API uint64_t ndrx_gettid(void);

/* Internal testing */
extern NDRX_API int ndrx_bench_write_stats(double msgsize, double callspersec);

/* Standard library TLS: */
extern NDRX_API void * ndrx_nstd_tls_get(void);
extern NDRX_API int ndrx_nstd_tls_set(void *data);
extern NDRX_API void ndrx_nstd_tls_free(void *data);
extern NDRX_API void * ndrx_nstd_tls_new(int auto_destroy, int auto_set);

/* Platform: */
extern NDRX_API long ndrx_platf_stack_get_size(void);
extern NDRX_API void ndrx_platf_stack_set(void *pthread_custom_attr);


extern NDRX_API unsigned long ndrx_Crc32_ComputeBuf( unsigned long inCrc32, const void *buf,
                                       size_t bufLen );


extern NDRX_API char *ndrx_strsep(char **s1, char *s2);


extern NDRX_API int ndrx_args_loader_get(ndrx_args_loader_t *args, void *obj, 
        char *fldnm, char *value, int valuesz,
        char *errbuf, size_t errbufsz);


extern NDRX_API int ndrx_args_loader_set(ndrx_args_loader_t *args, void *obj, 
        char *fldnm, char *value,
        char *errbuf, size_t errbufsz);


extern NDRX_API int ndrx_file_gen_embed(char *in_fname, char *out_fname, 
        char *out_suffix);


extern NDRX_API int ndrx_storage_decode(char *bytesenc, long *outnrbytes);
extern NDRX_API void ndrx_storage_encode(long bytes, char *outbuf, int outbufsz);

extern NDRX_API int ndrx_lh_position_get(ndrx_lh_config_t *conf, 
        void *key_get, size_t key_len, 
        int oflag, int *pos, int *have_value, char *key_typ);

#ifdef	__cplusplus
}
#endif

#endif	/* NSTDUTIL_H */

/* vim: set ts=4 sw=4 et smartindent: */
