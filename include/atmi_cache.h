/* 
** ATMI cache strctures
**
** @file atmi_cache.h
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

#ifndef ATMI_CACHE_H
#define	ATMI_CACHE_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <cconfig.h>
#include <atmi.h>
#include <atmi_int.h>
#include <exhash.h>
#include <exdb.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
    
#define NDRX_TPCACHE_DEBUG

#define NDRX_TPCACHE_FLAGS_EXPIRY    0x00000001   /* Cache recoreds expires after add */
#define NDRX_TPCACHE_FLAGS_LRU       0x00000002   /* limited, last recently used stays*/
#define NDRX_TPCACHE_FLAGS_HITS      0x00000004   /* limited, more hits, longer stay  */
#define NDRX_TPCACHE_FLAGS_FIFO      0x00000008   /* First in, first out cache        */
#define NDRX_TPCACHE_FLAGS_BOOTRST   0x00000010   /* reset cache on boot              */
#define NDRX_TPCACHE_FLAGS_BCASTPUT  0x00000020   /* Shall we broadcast the events?   */
#define NDRX_TPCACHE_FLAGS_BCASTDEL  0x00000040   /* Broadcast delete events?         */
#define NDRX_TPCACHE_FLAGS_TIMESYNC  0x00000080   /* Perfrom timsync                  */
#define NDRX_TPCACHE_FLAGS_SCANDUP   0x00000100   /* Scan for duplicates by tpcached  */

#define NDRX_TPCACHE_TPCF_SAVEREG    0x00000001      /* Save record can be regexp     */
#define NDRX_TPCACHE_TPCF_REPL       0x00000002      /* Replace buf                   */
#define NDRX_TPCACHE_TPCF_MERGE      0x00000004      /* Merge buffers                 */
#define NDRX_TPCACHE_TPCF_SAVEFULL   0x00000008      /* Save full buffer              */
#define NDRX_TPCACHE_TPCF_SAVESETOF  0x00000010      /* Save set of fields            */
#define NDRX_TPCACHE_TPCF_INVAL      0x00000020      /* Invalidate other cache        */
#define NDRX_TPCACHE_TPCF_NEXT       0x00000040      /* Process next rule (only for inval)*/
    
#define NDRX_TPCACHE_TPCF_DELREG     0x00000080      /* Delete record can be regexp   */
#define NDRX_TPCACHE_TPCF_DELFULL    0x00000100      /* Delete full buffer            */
#define NDRX_TPCACHE_TPCF_DELSETOF   0x00000200      /* Delete set of fields          */

    
#define NDRX_TPCACH_INIT_NORMAL      0   /* Normal init (client & server)    */
#define NDRX_TPCACH_INIT_BOOT        1   /* Boot mode init (ndrxd startst)   */

/* -1 = EXFAIL standard error */
#define NDRX_TPCACHE_ENOTFOUND      -2   /* Record not found                 */
#define NDRX_TPCACHE_ENOCACHEDATA   -3   /* Data not cached                  */
#define NDRX_TPCACHE_ENOCACHE       -4   /* Service not in cache config      */
#define NDRX_TPCACHE_ENOKEYDATA     -5   /* No key data found                */
#define NDRX_TPCACHE_ENOTYPESUPP    -6   /* Type not supported               */


#define NDRX_TPCACHE_BCAST_DFLT     ""   /* default event                    */
#define NDRX_TPCACHE_BCAST_DELFULL  "F"  /* delete full                      */
    
#define NDRX_CACHES_BLOCK           "caches"
#define NDRX_CACHE_MAX_READERS_DFLT 1000
#define NDRX_CACHE_MAP_SIZE_DFLT    160000 /* 160K */
#define NDRX_CACHE_PERMS_DFLT       0664

#define NDRX_CACHE_BCAST_MODE_PUT   1
#define NDRX_CACHE_BCAST_MODE_DEL   2
#define NDRX_CACHE_BCAST_MODE_KIL   3       /* drop the database              */
#define NDRX_CACHE_BCAST_MODE_MSK   4       /* Delete by mask                 */
#define NDRX_CACHE_BCAST_MODE_DKY   5       /* Delete by key                  */

/*
 * Command code sent to tpcachesv
 */
#define NDRX_CACHE_SVCMD_DELBYEXPR  'E'     /* Delete by expression           */
#define NDRX_CACHE_SVCMD_DELBYKEY   'K'     /* Delete by key (direct lookup)  */
    
    
#define NDRX_CACHE_OPEXPRMAX        PATH_MAX /* max len of operation expression*/

/**
 * Dump the cache database configuration
 */
#define NDRX_TPCACHEDB_DUMPCFG(LEV, CACHEDB)\
    NDRX_LOG(LEV, "============ CACHE DB CONFIG DUMP ===============");\
    NDRX_LOG(LEV, "cachedb=[%s]", CACHEDB->cachedb);\
    NDRX_LOG(LEV, "resource=[%s]", CACHEDB->resource);\
    NDRX_LOG(LEV, "limit=[%ld]", CACHEDB->limit);\
    NDRX_LOG(LEV, "expiry=[%ld] msec", CACHEDB->expiry);\
    NDRX_LOG(LEV, "flags=[%ld]", CACHEDB->flags);\
    NDRX_LOG(LEV, "flags, 'expiry' = [%d]", \
                    !!(CACHEDB->flags &  NDRX_TPCACHE_FLAGS_EXPIRY));\
    NDRX_LOG(LEV, "flags, 'lru' = [%d]", \
                    !!(CACHEDB->flags &  NDRX_TPCACHE_FLAGS_LRU));\
    NDRX_LOG(LEV, "flags, 'hits' = [%d]", \
                    !!(CACHEDB->flags &  NDRX_TPCACHE_FLAGS_HITS));\
    NDRX_LOG(LEV, "flags, 'fifo' = [%d]", \
                    !!(CACHEDB->flags &  NDRX_TPCACHE_FLAGS_FIFO));\
    NDRX_LOG(LEV, "flags, 'bootreset' = [%d]", \
                    !!(CACHEDB->flags &  NDRX_TPCACHE_FLAGS_BOOTRST));\
    NDRX_LOG(LEV, "flags, 'bcastput' = [%d]", \
                    !!(CACHEDB->flags &  NDRX_TPCACHE_FLAGS_BCASTPUT));\
    NDRX_LOG(LEV, "flags, 'bcastdel' = [%d]", \
                    !!(CACHEDB->flags &  NDRX_TPCACHE_FLAGS_BCASTDEL));\
    NDRX_LOG(LEV, "flags, 'timesync' = [%d]", \
                    !!(CACHEDB->flags &  NDRX_TPCACHE_FLAGS_TIMESYNC));\
    NDRX_LOG(LEV, "max_readers=[%ld]", CACHEDB->max_readers);\
    NDRX_LOG(LEV, "map_size=[%ld]", CACHEDB->map_size);\
    NDRX_LOG(LEV, "perms=[%o]", CACHEDB->perms);\
    NDRX_LOG(LEV, "subscr=[%s]", CACHEDB->subscr);\
    NDRX_LOG(LEV, "=================================================");

    
/**
 * Dump tpcall configuration
 */
#define NDRX_TPCACHETPCALL_DUMPCFG(LEV, TPCALLCACHE)\
    NDRX_LOG(LEV, "============ TPCALL CACHE CONFIG DUMP ===============");\
    NDRX_LOG(LEV, "cachedbnm=[%s]", TPCALLCACHE->cachedbnm);\
    NDRX_LOG(LEV, "cachedb=[%p]", TPCALLCACHE->cachedb);\
    NDRX_LOG(LEV, "idx=[%d]", TPCALLCACHE->idx);\
    NDRX_LOG(LEV, "keyfmt=[%s]", TPCALLCACHE->keyfmt);\
    NDRX_LOG(LEV, "save=[%s]", TPCALLCACHE->saveproj.expression);\
    NDRX_LOG(LEV, "delete=[%s]", TPCALLCACHE->delproj.expression);\
    NDRX_LOG(LEV, "rule=[%s]", TPCALLCACHE->rule);\
    NDRX_LOG(LEV, "rule_tree=[%p]", TPCALLCACHE->rule_tree);\
    NDRX_LOG(LEV, "refreshrule=[%s]", TPCALLCACHE->refreshrule);\
    NDRX_LOG(LEV, "refreshrule_tree=[%p]", TPCALLCACHE->refreshrule_tree);\
    NDRX_LOG(LEV, "rsprule=[%s]", TPCALLCACHE->rsprule);\
    NDRX_LOG(LEV, "rsprule_tree=[%p]", TPCALLCACHE->rsprule_tree);\
    NDRX_LOG(LEV, "str_buf_type=[%s]", TPCALLCACHE->str_buf_type);\
    NDRX_LOG(LEV, "str_buf_subtype=[%s]", TPCALLCACHE->str_buf_subtype);\
    NDRX_LOG(LEV, "buf_type=[%s]", TPCALLCACHE->buf_type->type);\
    NDRX_LOG(LEV, "errfmt=[%s]", TPCALLCACHE->errfmt);\
    NDRX_LOG(LEV, "flags=[%s]", TPCALLCACHE->flagsstr);\
    NDRX_LOG(LEV, "flags, 'putrex' = [%d]", \
                    !!(TPCALLCACHE->flags &  NDRX_TPCACHE_TPCF_SAVEREG));\
    NDRX_LOG(LEV, "flags, 'getreplace' = [%d]", \
                    !!(TPCALLCACHE->flags &  NDRX_TPCACHE_TPCF_REPL));\
    NDRX_LOG(LEV, "flags, 'getmerge' = [%d]", \
                    !!(TPCALLCACHE->flags &  NDRX_TPCACHE_TPCF_MERGE));\
    NDRX_LOG(LEV, "flags, 'putfull' = [%d]", \
                    !!(TPCALLCACHE->flags &  NDRX_TPCACHE_TPCF_SAVEFULL));\
    NDRX_LOG(LEV, "flags 'inval' = [%d]", \
                    !!(TPCALLCACHE->flags &  NDRX_TPCACHE_TPCF_INVAL));\
    NDRX_LOG(LEV, "flags (computed) save list = [%d]", \
                    !!(TPCALLCACHE->flags &  NDRX_TPCACHE_TPCF_SAVESETOF));\
    NDRX_LOG(LEV, "flags, 'delrex' = [%d]", \
                    !!(TPCALLCACHE->flags &  NDRX_TPCACHE_TPCF_DELREG));\
    NDRX_LOG(LEV, "flags, 'delfull' = [%d]", \
                    !!(TPCALLCACHE->flags &  NDRX_TPCACHE_TPCF_DELFULL));\
    NDRX_LOG(LEV, "flags (computed) delete list = [%d]", \
                    !!(TPCALLCACHE->flags &  NDRX_TPCACHE_TPCF_DELSETOF));\
    NDRX_LOG(LEV, "inval_cache=[%p]", TPCALLCACHE->inval_cache);\
    NDRX_LOG(LEV, "inval_svc=[%s]", TPCALLCACHE->inval_svc);\
    NDRX_LOG(LEV, "inval_idx=[%d]", TPCALLCACHE->inval_idx);\
    NDRX_LOG(LEV, "=================================================");


#define NDRX_TPCACHETPCALL_DBDATA(LEV, DBDATA)\
    NDRX_LOG(LEV, "================== DB DATA DUMP =================");\
    NDRX_LOG(LEV, "saved_tperrno = [%d]", DBDATA->saved_tperrno);\
    NDRX_LOG(LEV, "saved_tpurcode = [%ld]", DBDATA->saved_tpurcode);\
    NDRX_LOG(LEV, "atmi_buf_len = [%ld]", DBDATA->saved_tpurcode);\
    NDRX_DUMP(LEV, "BLOB data", DBDATA->atmi_buf, DBDATA->atmi_buf_len);\
    NDRX_LOG(LEV, "=================================================");
    

#define NDRX_CACHE_TPERROR(atmierr, fmt, ...)\
        NDRX_LOG(log_error, fmt, ##__VA_ARGS__);\
        userlog(fmt, ##__VA_ARGS__);\
        ndrx_TPset_error_fmt(atmierr, fmt, ##__VA_ARGS__);

#define NDRX_CACHE_ERROR(fmt, ...)\
        NDRX_LOG(log_error, fmt, ##__VA_ARGS__);\
        userlog(fmt, ##__VA_ARGS__);
    
    
#define NDRX_CACHE_TPERRORNOU(atmierr, fmt, ...)\
        NDRX_LOG(log_error, fmt, ##__VA_ARGS__);\
        ndrx_TPset_error_fmt(atmierr, fmt, ##__VA_ARGS__);
    
#define NDRX_CACHE_MAGIC        0xab4388ef

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/**
 * Cache database 
 */
struct ndrx_tpcache_db
{
    char cachedb[NDRX_CCTAG_MAX];/* cache db logical name (subsect of @cachedb)     s*/
    char resource[PATH_MAX+1];  /* physical path of the cache folder                */
    long limit;                 /* number of records limited for cache used by 2,3,4*/
    long expiry;                /* Number of milli-seconds for record to live       */
    long flags;                 /* configuration flags for this cache               */
    long max_readers;           /* db settings                                      */
    long map_size;              /* db settings                                      */
    int broadcast;              /* Shall we broadcast the events                    */
    int perms;                  /* permissions of the database resource             */
    
    char subscr[NDRX_EVENT_EXPR_MAX]; /* expression for consuming PUT events    */
    
    /* LMDB Related */
    
    EDB_env *env; /* env handler */
    EDB_dbi dbi;  /* named (unnamed) db */
    
    /* Make structure hashable: */
    EX_hash_handle hh;
};
typedef struct ndrx_tpcache_db ndrx_tpcache_db_t;

/**
 * This structure describes how to project a slice of the buffer
 */
struct ndrx_tpcache_projbuf
{
    char expression[PATH_MAX+1]; /* Projection expression               */
    
    /* Save can be regexp, so we need to compile it...!                 */
    int regex_compiled;
    regex_t regex;
    void *typpriv; /* private list of save data, could be projcpy list? */
    long typpriv2;
};
typedef struct ndrx_tpcache_projbuf ndrx_tpcache_projbuf_t;

/**
 * cache entry, this is linked list as 
 */
typedef struct ndrx_tpcallcache ndrx_tpcallcache_t;
struct ndrx_tpcallcache
{
    char svcnm[XATMI_SERVICE_NAME_LENGTH+1];
    char cachedbnm[NDRX_CCTAG_MAX+1]; /* cache db logical name (subsect of @cachedb)  */
    ndrx_tpcache_db_t *cachedb;
    char keyfmt[PATH_MAX+1];
    int idx;                            /* index of this cache for service  */
    
    ndrx_tpcache_projbuf_t saveproj;    /* Save buffer projection           */
    ndrx_tpcache_projbuf_t delproj;     /* Delete buffer projection         */
    
    /* We need a flags here to allow regex, for example. But the regex is */
    char flagsstr[NDRX_CACHE_FLAGS_MAX+1];
    long flags;
    
    /* Rule for refreshing the data (this is higher priority) */
    char refreshrule[PATH_MAX+1];
    char *refreshrule_tree;
    
    /* Rule for saving the data: */
    char rule[PATH_MAX+1];
    char *rule_tree;
    
    char rsprule[PATH_MAX+1];
    char *rsprule_tree;
    char str_buf_type[XATMI_TYPE_LEN+1];
    char str_buf_subtype[XATMI_SUBTYPE_LEN+1];
    
    typed_buffer_descr_t *buf_type;
    
    /* optional return code expression 
     * In case if missing, only TPSUCCESS messages are saved.
     * If expression is set, we will load the tperrno() and tpurcode() in the
     * following UBF fields:
     * EX_TPERRNO and TPURCODE. Then the user might evalue the value to decide
     * keep the values or not.
     * 
     * The tperrno and tpurcode must be smulated when stored in cache db.
     */
    char errfmt[PATH_MAX/2];
    
    
    /* For invalidating their cache, in case if rule matched */
    ndrx_tpcallcache_t *inval_cache;    /* their cache to invalidate        */
    char inval_svc[MAXTIDENT+1];        /* Service name of their cache      */
    int inval_idx;                      /* Index of their cache, 0 based    */
    
    /* this is linked list of caches */
    ndrx_tpcallcache_t *next, *prev;
};

/**
 * This is hash of services which are cached.
 */
struct ndrx_tpcache_svc
{
    char svcnm[MAXTIDENT+1];    /* cache db logical name (subsect of @cachedb)*/

    int in_hash;                /* Are we added to hash list?                 */
    ndrx_tpcallcache_t *caches; /* This list list of caches */
        
    /* Make structure hashable: */
    EX_hash_handle hh;
};
typedef struct ndrx_tpcache_svc ndrx_tpcache_svc_t;


/**
 * Structure for holding data up, payload
 */
struct ndrx_tpcache_data
{
    int magic;          /* Magic bytes              */
    int saved_tperrno;
    long saved_tpurcode;
    long t;             /* UTC timestamp of message */
    long tusec;         /* UTC microseconds         */
    
    /* time when we picked up the record */
    long hit_t;         /* UTC timestamp of message */
    long hit_tusec;     /* UTC microseconds         */
    unsigned long hits; /* Number of cache hits     */
    
    int  nodeid;        /* Node id who put the msg  */
    short atmi_type_id; /* ATMI type id           */
    
    /* Payload data */
    long atmi_buf_len;  /* saved buffer len         */
    char atmi_buf[0]; /* the data follows           */
};
typedef struct ndrx_tpcache_data ndrx_tpcache_data_t;


struct ndrx_tpcache_datasort
{
    /* we need a ptr to key too... */
    
    EDB_val key; /* allocated key */
    
    ndrx_tpcache_data_t data; /* just copy header of data block */
};
typedef struct ndrx_tpcache_datasort ndrx_tpcache_datasort_t;
/*
 * NOTE: Key is used directly as binary data and length 
 */

/*
 * Need a structure for holding the buffer rules according to data types
 */
typedef struct ndrx_tpcache_typesupp ndrx_tpcache_typesupp_t;
struct ndrx_tpcache_typesupp
{
    int type_id;
    /* This shall compile the refresh rule too */
    int (*pf_rule_compile) (ndrx_tpcallcache_t *cache, char *errdet, int errdetbufsz);
    
    int (*pf_rule_eval) (ndrx_tpcallcache_t *cache, char *idata, long ilen, 
                char *errdet, int errdetbufsz);
    
    /* Refresh rule evaluate */
    int (*pf_refreshrule_eval) (ndrx_tpcallcache_t *cache, char *idata, long ilen, 
                char *errdet, int errdetbufsz);
    
    int (*pf_get_key) (ndrx_tpcallcache_t *cache, char *idata, long ilen, char
                *okey, int okey_bufsz, char *errdet, int errdetbufsz);
    
    /* Receive message from cache */
    int (*pf_cache_get) (ndrx_tpcallcache_t *cache, ndrx_tpcache_data_t *exdata, 
            typed_buffer_descr_t *buf_type,
            char *idata, long ilen, char **odata, long *olen, long flags);
    
    int (*pf_cache_put) (ndrx_tpcallcache_t *cache, ndrx_tpcache_data_t *exdata, 
        typed_buffer_descr_t *descr, char *idata, long ilen, long flags);
    
    int (*pf_cache_del) (ndrx_tpcallcache_t *cache, 
        char *idata, long ilen, char **odata, long *olen);
    
    /* check flags for given type and process the save rule if any */
    int (*pf_process_flags)(ndrx_tpcallcache_t *cache, char *errdet, int errdetbufsz);
    
    /* cache delete callback, to free up memory of any */
    int (*pf_cache_delete)(ndrx_tpcallcache_t *cache);
};


/*---------------------------Globals------------------------------------*/

extern NDRX_API ndrx_tpcache_db_t *ndrx_G_tpcache_db; /* ptr to cache database */
extern NDRX_API ndrx_tpcache_svc_t *ndrx_G_tpcache_svc; /* service cache       */
extern NDRX_API ndrx_tpcache_typesupp_t ndrx_G_tpcache_types[];

/*---------------------------Prototypes---------------------------------*/


/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

extern NDRX_API int ndrx_cache_init(int mode);
extern NDRX_API ndrx_tpcache_db_t *ndrx_cache_dbgethash(void);
extern NDRX_API int ndrx_cache_used(void);

extern NDRX_API int ndrx_cache_save (char *svc, char *idata, 
        long ilen, int save_tperrno, long save_tpurcode, int nodeid, long flags,
        long t, int tusec, int is_event);

extern NDRX_API int ndrx_cache_lookup(char *svc, char *idata, long ilen, 
        char **odata, long *olen, long flags, int *should_cache,
        int *saved_tperrno, long *saved_tpurcode);
extern NDRX_API int ndrx_cache_inval_their(char *svc, ndrx_tpcallcache_t *cache, 
        char *key, char *idata, long ilen);

extern NDRX_API int ndrx_cache_inval_by_data(char *svc, char *idata, long ilen,
        char *flags);
extern NDRX_API int ndrx_cache_drop(char *cachedbnm, short nodeid);
extern NDRX_API long ndrx_cache_inval_by_expr(char *cachedbnm, 
        char *keyexpr, short nodeid);
extern NDRX_API int ndrx_cache_inval_by_key(char *cachedbnm, char *key, short nodeid);
extern NDRX_API int ndrx_cache_maperr(int unixerr);
extern NDRX_API ndrx_tpcallcache_t* ndrx_cache_findtpcall(ndrx_tpcache_svc_t *svcc, 
        typed_buffer_descr_t *buf_type, char *idata, long ilen, int idx);

extern NDRX_API int ndrx_cache_cmp_fun(const EDB_val *a, const EDB_val *b);

extern NDRX_API int ndrx_cache_edb_get(ndrx_tpcache_db_t *db, EDB_txn *txn, 
        char *key, EDB_val *data_out);
extern NDRX_API int ndrx_cache_edb_abort(ndrx_tpcache_db_t *db, EDB_txn *txn);
extern NDRX_API int ndrx_cache_edb_commit(ndrx_tpcache_db_t *db, EDB_txn *txn);
extern NDRX_API int ndrx_cache_edb_begin(ndrx_tpcache_db_t *db, EDB_txn **txn);

extern NDRX_API int ndrx_cache_edb_set_dupsort(ndrx_tpcache_db_t *db, EDB_txn *txn, 
            EDB_cmp_func *cmp);

extern NDRX_API int ndrx_cache_edb_del (ndrx_tpcache_db_t *db, EDB_txn *txn, 
        char *key, EDB_val *data);

extern NDRX_API int ndrx_cache_edb_put (ndrx_tpcache_db_t *db, EDB_txn *txn, 
        char *key, EDB_val *data, unsigned int flags);

extern NDRX_API int ndrx_cache_edb_stat (ndrx_tpcache_db_t *db, EDB_txn *txn, 
        EDB_stat * stat);

extern NDRX_API  int ndrx_cache_edb_cursor_open(ndrx_tpcache_db_t *db, EDB_txn *txn, 
            EDB_cursor ** cursor);
extern NDRX_API int ndrx_cache_edb_cursor_get(ndrx_tpcache_db_t *db, EDB_cursor * cursor,
        char *key, EDB_val *data_out, EDB_cursor_op op);

extern NDRX_API int ndrx_cache_edb_cursor_getfullkey(ndrx_tpcache_db_t *db, 
        EDB_cursor * cursor, EDB_val *keydb, EDB_val *data_out, EDB_cursor_op op);

extern NDRX_API int ndrx_cache_edb_delfullkey (ndrx_tpcache_db_t *db, EDB_txn *txn, 
        EDB_val *keydb, EDB_val *data);

extern NDRX_API ndrx_tpcache_db_t* ndrx_cache_dbresolve(char *cachedb, int mode);

/* UBF support: */
extern NDRX_API int ndrx_cache_delete_ubf(ndrx_tpcallcache_t *cache);
extern NDRX_API int ndrx_cache_proc_flags_ubf(ndrx_tpcallcache_t *cache, 
        char *errdet, int errdetbufsz);
extern NDRX_API int ndrx_cache_put_ubf (ndrx_tpcallcache_t *cache,
        ndrx_tpcache_data_t *exdata,  typed_buffer_descr_t *descr, 
        char *idata, long ilen, long flags);
extern NDRX_API int ndrx_cache_del_ubf (ndrx_tpcallcache_t *cache, 
        char *idata, long ilen, char **odata, long *olen);
extern NDRX_API int ndrx_cache_get_ubf (ndrx_tpcallcache_t *cache,
        ndrx_tpcache_data_t *exdata, typed_buffer_descr_t *buf_type, 
        char *idata, long ilen, char **odata, long *olen, long flags);
extern NDRX_API int ndrx_cache_ruleval_ubf (ndrx_tpcallcache_t *cache, 
        char *idata, long ilen,  char *errdet, int errdetbufsz);
extern NDRX_API int ndrx_cache_rulcomp_ubf (ndrx_tpcallcache_t *cache, 
        char *errdet, int errdetbufsz);
extern NDRX_API int ndrx_cache_keyget_ubf (ndrx_tpcallcache_t *cache, 
        char *idata, long ilen, char *okey, int okey_bufsz, 
        char *errdet, int errdetbufsz);
extern NDRX_API int ndrx_cache_refeval_ubf (ndrx_tpcallcache_t *cache, 
        char *idata, long ilen,  char *errdet, int errdetbufsz);

/* eventing: */
extern NDRX_API int ndrx_cache_broadcast(ndrx_tpcallcache_t *cache, char *svc, 
        char *idata, long ilen, int event_type, char *flags, int user1, long user2,
        int user3, long user4);
extern NDRX_API int ndrx_cache_broadcast_by_delkey(char *cachedbnm, char *key, 
        short nodeid);
extern NDRX_API int ndrx_cache_events_get(string_list_t **list);

#ifdef	__cplusplus
}
#endif

#endif	/* ATMI_CACHE_H */

