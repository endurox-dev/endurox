/* 
** ATMI level cache
** No using named databases. Each environment is associated with single db
** To resolve conflicts in cluster mode we shall compare time with a microseconds
** for this we will extend tppost so that we can send rval as userfield1 and
** rcode as userfield2. When cache server receives request we add the records
** even if it is duplicate (for this timesync flag must be present). Then when
** we read all records, if two or more records found, then we will use the youngest
** one by UTC. If times stamps equals, then we use record as priority from the cluster
** node with a highest node id number.
**
** @file atmi_cache.c
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

/*---------------------------Includes-----------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ndrstandard.h>
#include <atmi.h>
#include <atmi_tls.h>
#include <typed_buf.h>

#include "thlock.h"
#include "userlog.h"
#include "utlist.h"
#include "exregex.h"
#include <exparson.h>
#include <atmi_cache.h>
#include <Exfields.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

#define CACHES_BLOCK    "caches"

#define CACHE_MAX_READERS_DFLT      1000
#define CACHE_MAP_SIZE_DFLT         160000 /* 160K */
#define CACHE_PERMS_DFLT            0664

#define NDRX_CACHE_TPERROR(atmierr, fmt, ...)\
        NDRX_LOG(log_error, fmt, ##__VA_ARGS__);\
        userlog(fmt, ##__VA_ARGS__);\
        ndrx_TPset_error_fmt(atmierr, fmt, ##__VA_ARGS__);

#define NDRX_CACHE_ERROR(fmt, ...)\
        NDRX_LOG(log_error, fmt, ##__VA_ARGS__);\
        userlog(fmt, ##__VA_ARGS__);

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
exprivate ndrx_tpcache_db_t *M_tpcache_db = NULL; /* ptr to cache database */
exprivate ndrx_tpcache_svc_t *M_tpcache_svc = NULL; /* service cache       */
/*---------------------------Prototypes---------------------------------*/

/* NOTE! Table shall contain all defined buffer types  */
expublic ndrx_tpcache_typesupp_t M_types[] =
{
    /* typeid (idx), rule_compile func,         rule eval func,             refresh rule eval  */
    {BUF_TYPE_UBF, ndrx_cache_rulcomp_ubf,      ndrx_cache_ruleval_ubf,     ndrx_cache_refeval_ubf, 
                    ndrx_cache_keyget_ubf,      ndrx_cache_get_ubf,         ndrx_cache_put_ubf, 
                    ndrx_cache_proc_flags_ubf,  ndrx_cache_delete_ubf},
    /* dummy */
    {1,             NULL,                       NULL,                       NULL,
                    NULL,                       NULL,                       NULL,
                    NULL,                       NULL},
                    
    {BUF_TYPE_INIT, NULL,                       NULL,                       NULL,
                    NULL,                       NULL,                       NULL,
                    NULL,                       NULL},
                    
    {BUF_TYPE_NULL, NULL,                       NULL,                       NULL,
                    NULL,                       NULL,                       NULL,
                    NULL,                       NULL},
                    
    {BUF_TYPE_STRING,NULL,                      NULL,                       NULL,
                    NULL,                       NULL,                       NULL,
                    NULL,                       NULL},
                    
    {BUF_TYPE_CARRAY, NULL,                     NULL,                       NULL,
                    NULL,                       NULL,                       NULL,
                    NULL,                       NULL},
                    
    {BUF_TYPE_JSON, NULL,                       NULL,                       NULL,
                    NULL,                       NULL,                       NULL,
                    NULL,                       NULL},
                    
    {BUF_TYPE_VIEW, NULL,                       NULL,                       NULL,
                    NULL,                       NULL,                       NULL,
                    NULL,                       NULL},
    {EXFAIL}
};

/**
 * Function returns TRUE if cache is used
 * @return 
 */
expublic int ndrx_cache_used(void)
{
    if (NULL!=M_tpcache_db && NULL!=M_tpcache_svc)
    {
        return EXTRUE;
    }
    
    return EXFALSE;
}

/**
 * Map unix error
 * @param unixerr unix error
 * @return TP Error
 */
expublic int ndrx_cache_maperr(int unixerr)
{
    int ret = TPEOS;
    
    switch (unixerr)
    {
        case EDB_NOTFOUND:
            
            ret = TPENOENT;
            
            break;
    }
    
    return ret;
}


/**
 * Check the cachedb existence in cache, if exists, return
 * @param cachedb cache name
 * @return ptr to cache db or NULL if not found
 */
expublic ndrx_tpcache_db_t* ndrx_cache_dbget(char *cachedb)
{
    ndrx_tpcache_db_t *ret;
    
    EXHASH_FIND_STR( M_tpcache_db, cachedb, ret);
    
    return ret;
}


/**
 * Close database
 * @param db descr struct
 */
exprivate void ndrx_cache_db_free(ndrx_tpcache_db_t *db)
{
    /* func checks the dbi validity */
    edb_dbi_close(db->env, db->dbi);

    if (NULL!=db->env)
    {
        edb_env_close(db->env);
    }
    
    NDRX_FREE(db);
}

/**
 * Delete single tpcall cache
 * @param tpc Tpcall cache
 * @return 
 */
expublic void ndrx_cache_tpcallcache_free(ndrx_tpcallcache_t *tpc)
{
    
    if (tpc->buf_type && 
            NULL!=M_types[tpc->buf_type->type_id].pf_cache_delete)
    {
        M_types[tpc->buf_type->type_id].pf_cache_delete(tpc);
    }
    
    if (NULL!=tpc->rsprule_tree)
    {
        Btreefree(tpc->rsprule_tree);
    }
    
    if (tpc->save_regex_compiled)
    {
        ndrx_regfree(&tpc->save_regex);
    }
    
    NDRX_FREE(tpc);
}

/**
 * Delete single service
 * @param svc service cache
 */
expublic void ndrx_cache_svc_free(ndrx_tpcache_svc_t *svc)
{
    ndrx_tpcallcache_t *elt, *el;
    
    /* killall tpcalls */
    DL_FOREACH_SAFE(svc->caches, el, elt)
    {
        DL_DELETE(svc->caches, el);
        ndrx_cache_tpcallcache_free(el);
    }
    
    NDRX_FREE(svc);
}

/**
 * Delete all service caches
 */
expublic void ndrx_cache_svcs_free(void)
{
    ndrx_tpcache_svc_t *el, *elt;
    EXHASH_ITER(hh, M_tpcache_svc, el, elt)
    {
        EXHASH_DEL(M_tpcache_svc, el);
        ndrx_cache_svc_free(el);
    }
}

/**
 * Delete all databases open
 */
expublic void ndrx_cache_dbs_free(void)
{
    ndrx_tpcache_db_t *el, *elt;
    EXHASH_ITER(hh, M_tpcache_db, el, elt)
    {
        EXHASH_DEL(M_tpcache_db, el);
        ndrx_cache_db_free(el);
    }
}

/**
 * Compare cache entries
 * @param a
 * @param b
 * @return -1, 0, 1
 */
exprivate int ndrx_cache_cmp_fun(const EDB_val *a, const EDB_val *b)
{
    ndrx_tpcache_data_t *ad = (ndrx_tpcache_data_t *)a->mv_data;
    ndrx_tpcache_data_t *bd = (ndrx_tpcache_data_t *)b->mv_data;
    int result = 0;
    
    
    if (ad->t > bd->t)
    {
        result = 1;
    }
    else if (ad->t < bd->t)
    {
        result = -1;
    }
    else
    {
        /* equals compare, microsec */
        
        if (ad->tusec > bd->tusec)
        {
            result = 1;
        }
        else if (ad->tusec < bd->tusec)
        {
            result = -1;
        }
        else
        {
            /* equals now decide from node id, the higher number wins */
            
            if (ad->nodeid > bd->nodeid)
            {
                result = 1;
            }
            else if (ad->nodeid < bd->nodeid)
            {
                result = -1;
            }
            else
            {
                /* local node two records a the same time, so equals... */
                result = 0;
            }
        }
    }
    
    return result;
}

/**
 * Resolve cache db
 * @param cachedb name of cache db
 * @param mode either normal or create mode (started by ndrxd)
 * @return NULL in case of failure, non NULL, resolved ptr to cache db record
 */
expublic ndrx_tpcache_db_t* ndrx_cache_dbresolve(char *cachedb, int mode)
{
    int ret = EXSUCCEED;
    ndrx_tpcache_db_t* db;
    ndrx_inicfg_section_keyval_t * csection = NULL, *val = NULL, *val_tmp = NULL;
    /* len of @cachedb + / + subsect + EOS */
    char cachesection[sizeof(NDRX_CONF_SECTION_CACHEDB)+1+NDRX_CCTAG_MAX+1];
    char *p; 
    char *saveptr1 = NULL;
    EDB_txn *txn = NULL;
    unsigned int dbi_flags;

    if (NULL!=(db = ndrx_cache_dbget(cachedb)))
    {
#ifdef NDRX_TPCACHE_DEBUG
        NDRX_LOG(log_debug, "Cache db [%s] already loaded", cachedb);
        goto out;
#endif        
    }

    /* Lookup the section from configuration */
    
    snprintf(cachesection, sizeof(cachesection), "%s/%s",
            NDRX_CONF_SECTION_CACHEDB, cachedb);
    
    NDRX_LOG(log_debug, "cache db [%s] mode %d looking up: [%s]", 
            cachedb, mode, cachesection);
    
    if (EXSUCCEED!=ndrx_inicfg_get_subsect(ndrx_get_G_cconfig(), 
                        NULL,  /* all config files */
                        cachesection,  /* global section */
                        &csection))
    {
        NDRX_CACHE_ERROR("%s: Failed to get section [%s]: %s", 
                __func__, cachesection, Nstrerror(Nerror));
        EXFAIL_OUT(ret);
   }
    
   /* Parse arguments in the loop */
    
    NDRX_CALLOC_OUT(db, 1, sizeof(ndrx_tpcache_db_t), ndrx_tpcache_db_t);
    
    /* Set max_readers, map_size defaults */
    
    db->max_readers = CACHE_MAX_READERS_DFLT;
    db->map_size = CACHE_MAP_SIZE_DFLT;
    db->perms = CACHE_PERMS_DFLT;
    
    EXHASH_ITER(hh, csection, val, val_tmp)
    {
        
#ifdef NDRX_TPCACHE_DEBUG
        NDRX_LOG(log_debug, "%s: config: key: [%s] value: [%s]",
                    __func__, val->key, val->val);
#endif
        
        if (0==strcmp(val->key, "cachedb"))
        {
            NDRX_STRCPY_SAFE(db->cachedb, val->val);
        } 
        else if (0==strcmp(val->key, "resource"))
        {
            NDRX_STRCPY_SAFE(db->resource, val->val);
        }
        else if (0==strcmp(val->key, "perms"))
        {
            char *pend;
            db->perms = strtol(val->val, &pend, 0);
        }
        /* Also float: Parse 1000, 1K, 1M, 1G */
        else if (0==strcmp(val->key, "limit"))
        {
            db->limit = (long)ndrx_num_dec_parsecfg(val->val);
        }
        else if (0==strcmp(val->key, "expiry"))
        {
            db->expiry = (long)ndrx_num_time_parsecfg(val->val);
            db->flags|=NDRX_TPCACHE_FLAGS_EXPIRY;
        }
        else if (0==strcmp(val->key, "flags"))
        {
            /* Decode flags... */
            p = strtok_r (val->val, ",", &saveptr1);
            while (p != NULL)
            {
                /* strip off the string... */
                ndrx_str_strip(p, "\t ");
                
                /* bootreset,lru,hits,fifo */
                
                if (0==strcmp(p, "bootreset"))
                {
                    db->flags|=NDRX_TPCACHE_FLAGS_BOOTRST;
                }
                else if (0==strcmp(p, "lru"))
                {
                    db->flags|=NDRX_TPCACHE_FLAGS_LRU;
                }
                else if (0==strcmp(p, "hits"))
                {
                    db->flags|=NDRX_TPCACHE_FLAGS_HITS;
                }
                else if (0==strcmp(p, "fifo"))
                {
                    db->flags|=NDRX_TPCACHE_FLAGS_FIFO;
                }
                else if (0==strcmp(p, "broadcast"))
                {
                    db->flags|=NDRX_TPCACHE_FLAGS_BROADCAST;
                }
                else if (0==strcmp(p, "timesync"))
                {
                    db->flags|=NDRX_TPCACHE_FLAGS_TIMESYNC;
                }
                else
                {
                    /* unknown flag */
                    NDRX_CACHE_ERROR("Ignoring unknown cachedb [%s] flag: [%s]",
                            cachedb, p);
                }
                p = strtok_r (NULL, ",", &saveptr1);
            }
            
        } /* Also float: Parse 1000, 1K, 1M, 1G */
        else if (0==strcmp(val->key, "max_readers"))
        {
            db->max_readers = (long)ndrx_num_dec_parsecfg(val->val);
        }
        /* Parse float: 1000.5, 1.2K, 1M, 1G */
        else if (0==strcmp(val->key, "map_size"))
        {
            db->map_size = (long)ndrx_num_dec_parsecfg(val->val);
        }
        else if (0==strcmp(val->key, "subscr_put"))
        {
            NDRX_STRCPY_SAFE(db->subscr_put, val->val);
        }
        else if (0==strcmp(val->key, "subscr_del"))
        {
            NDRX_STRCPY_SAFE(db->subscr_del, val->val);
        }
        else
        {
            NDRX_LOG(log_warn, "Ignoring unknown cache configuration param: [%s]", 
                    val->key);
            userlog("Ignoring unknown cache configuration param: [%s]", 
                    val->key);
        }
    }
    
    /* Dump the DB config and open it and if we run in boot mode  
     * we have to reset the 
     */
#ifdef NDRX_TPCACHE_DEBUG
    NDRX_TPCACHEDB_DUMPCFG(log_info, db);
#endif
    
    /* Open the database */
    if (EXSUCCEED!=(ret=edb_env_create(&db->env)))
    {
        NDRX_CACHE_TPERROR(ndrx_cache_maperr(ret), 
                "CACHE: Failed to create env for [%s]: %s", 
                db->cachedb, edb_strerror(errno));
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=(ret=edb_env_set_maxreaders(db->env, db->max_readers)))
    {
        NDRX_CACHE_TPERROR(ndrx_cache_maperr(ret), 
                "CACHE: Failed to set max readers for [%s]: %s", 
                db->cachedb, edb_strerror(ret));
        
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=(ret=edb_env_set_mapsize(db->env, db->map_size)))
    {
        NDRX_CACHE_TPERROR(ndrx_cache_maperr(ret), 
                "Failed to set map size for [%s]: %s", 
                db->cachedb, edb_strerror(ret));
        
        EXFAIL_OUT(ret);
    }
    
    /* Open the DB */
    if (EXSUCCEED!=(ret=edb_env_open(db->env, db->resource, 0L, db->perms)))
    {
        NDRX_CACHE_TPERROR(ndrx_cache_maperr(ret), 
                "Failed to open env [%s]: %s", 
                db->cachedb, edb_strerror(ret));
        
        EXFAIL_OUT(ret);
    }
    
    /* Prepare the DB */
    if (EXSUCCEED!=(ret=edb_txn_begin(db->env, NULL, 0, &txn)))
    {
        NDRX_CACHE_TPERROR(ndrx_cache_maperr(ret), 
                "Failed to begin transaction for [%s]: %s", 
                db->cachedb, edb_strerror(ret));
        
        EXFAIL_OUT(ret);
    }
    
    /* open named db */
    if (db->flags & NDRX_TPCACHE_FLAGS_TIMESYNC)
    {
        /* We must perform sorting too so that first rec is 
         * is one we need to process and then we check for others to dump...
         */
        dbi_flags = EDB_DUPSORT | EDB_DUPSORT;
    }
    else
    {
        dbi_flags = 0;
    }
    
    if (EXSUCCEED!=(ret=edb_dbi_open(txn, NULL, dbi_flags, &db->dbi)))
    {
        NDRX_CACHE_TPERROR(ndrx_cache_maperr(ret), 
                "Failed to open named db for [%s]: %s", 
                db->cachedb, edb_strerror(ret));
        
        EXFAIL_OUT(ret);
    }
    
    if (NDRX_TPCACH_INIT_BOOT==mode)
    {
        NDRX_LOG(log_info, "Resetting cache db [%s]", db->cachedb);
        if (EXSUCCEED!=(ret=edb_drop(txn, db->dbi, 0)))
        {
            NDRX_CACHE_TPERROR(ndrx_cache_maperr(ret), 
                    "CACHE: Failed to clear db: [%s]: %s", 
                    db->cachedb, edb_strerror(ret));

            EXFAIL_OUT(ret);
        }
        
    }
    
    /* commit the tran */
    if (EXSUCCEED!=(ret=edb_txn_commit(txn)))
    {
        NDRX_CACHE_TPERROR(ndrx_cache_maperr(ret), 
                "Failed to open named db for [%s]: %s", 
                db->cachedb, edb_strerror(ret));
        txn = NULL;
        EXFAIL_OUT(ret);        
    }
    
    /* Add object to the hash */
    
    EXHASH_ADD_STR(M_tpcache_db, cachedb, db);
    
#ifdef NDRX_TPCACHE_DEBUG
        NDRX_LOG(log_debug, "Cache [%s] path: [%s] is open!", 
                db->resource, db->cachedb);
#endif
        
out:

    if (NULL!=csection)
    {
        ndrx_keyval_hash_free(csection);
    }

    if (EXSUCCEED!=ret)
    {
        
        if (NULL!=txn)
        {
            edb_txn_abort(txn);
        }
        
        if (NULL!=db)
        {
            ndrx_cache_db_free(db);
        }
        
        return NULL;
    }
    else
    {
        return db;
    }
}

/**
 * Find tpcall cache
 * @param idata XATMI buffer data
 * @param ilen data len
 * @param idx optional index (if not -1)
 * @return NULL - no cache found or not supported, ptr - to tpcall cache
 */
expublic ndrx_tpcallcache_t* ndrx_cache_findtpcall(ndrx_tpcache_svc_t *svcc, 
        typed_buffer_descr_t *buf_type, char *idata, long ilen, int idx)
{
    ndrx_tpcallcache_t* el;
    int ret = EXSUCCEED;
    char errdet[MAX_TP_ERROR_LEN+1];
    int i = 0;
    
    DL_FOREACH(svcc->caches, el)
    {
        if (el->buf_type->type_id == buf_type->type_id)
        {
            if (i==idx)
            {
                return el;
            }
            
            if (M_types[el->buf_type->type_id].pf_rule_eval)
            {
                ret = M_types[el->buf_type->type_id].pf_rule_eval(el, idata, ilen, 
                        errdet, sizeof(errdet));
                if (EXFAIL==ret)
                {
                    NDRX_CACHE_ERROR("%s: Failed to evaluate buffer [%s]: %s", 
                            __func__, el->rule, errdet);
                    return NULL;
                }
                else if (EXTRUE==ret)
                {
#ifdef NDRX_TPCACHE_DEBUG
                    NDRX_LOG(log_debug, "Buffer RULE TRUE [%s]", el->rule);
#endif
                    return el;
                }
                else
                {
#ifdef NDRX_TPCACHE_DEBUG
                    NDRX_LOG(log_debug, "Buffer RULE FALSE [%s]", el->rule);
#endif
                    /* search next... */
                }
            }
            else
            {
                /* We should not get here! */
                NDRX_CACHE_ERROR("%s: Unsupported buffer type [%s] for cache", 
                                __func__, el->buf_type->type);
                return NULL;
            }
        }
        else if (i==idx)
        {
            NDRX_CACHE_ERROR("%s: Cache found at index [%d] but types "
                    "does not match [%s] vs [%s]", __func__,  el->buf_type->type, 
                    buf_type->type);
                return NULL;
        }
        
        i++;
    }
    
    return NULL;
}


/**
 * Normal init (used by server & clients)
 * @param mode See NDRX_TPCACH_INIT_*
 */
expublic int ndrx_cache_init(int mode)
{
    int ret = EXSUCCEED;
    char *svc;
    EXJSON_Value *root_value=NULL;
    EXJSON_Object *root_object, *array_object;
    EXJSON_Array *array;
    int type;
    int nrcaches;
    char *name;
    int cnt;
    const char *tmp;
    char flagstr[NDRX_CACHE_FLAGS_MAX+1];
    size_t i;
    ndrx_tpcallcache_t *cache = NULL;
    ndrx_tpcache_svc_t *cachesvc = NULL;
    char errdet[MAX_TP_ERROR_LEN+1];
    char *p_flags;
    ndrx_inicfg_section_keyval_t * csection = NULL, *val = NULL, *val_tmp = NULL;
    char *saveptr1 = NULL;
    
    /* So if we are here, the configuration file should be already parsed 
     * We need to load the config file to AST
     */
    
    /* Firstly we search for [@cache[/sub-sect]] 
     * Then for each ^svc\ .* we parse the json block and build
     * up the cache descriptor
     */
    if (NULL==ndrx_get_G_cconfig())
    {
        /* no config used... nothing to do.. */
#ifdef NDRX_TPCACHE_DEBUG
        NDRX_LOG(log_debug, "No CConfig - skip cache init");
#endif
        goto out;
    }

    if (EXSUCCEED!=ndrx_cconfig_get(NDRX_CONF_SECTION_CACHE, &csection))
    {
        /* config not found... nothing todo.. */
#ifdef NDRX_TPCACHE_DEBUG
        NDRX_LOG(log_debug, "[%s] section not found - no cache init",
                NDRX_CONF_SECTION_CACHE);
#endif
        goto out;
    }
    
    EXHASH_ITER(hh, csection, val, val_tmp)
    {
        
#ifdef NDRX_TPCACHE_DEBUG
        NDRX_LOG(log_info, "Found cache service: key: [%s] value: [%s]",
                val->key, val->val);
#endif
        /* Have to sort out the entries and build the AST */
  
        if (0!=strncmp(val->key, "svc ", 4) && 0!=strncmp(val->key, "svc\t", 4) )
        {
            continue;
        }
        
        /* Strip to remove the whitespace */
        svc = ndrx_str_lstrip_ptr(val->key+4, "\t ");
        
#ifdef NDRX_TPCACHE_DEBUG
        NDRX_LOG(log_info, "Got service: [%s] ... parsing json config", svc);
#endif
        
        /* parse json block */
        root_value = exjson_parse_string_with_comments(val->val);

        type = exjson_value_get_type(root_value);
        NDRX_LOG(log_error, "Type is %d", type);

        if (exjson_value_get_type(root_value) != EXJSONObject)
        {
            NDRX_CACHE_ERROR("cache: invalid service [%s] cache: Failed "
                    "to parse root element", svc);
            EXFAIL_OUT(ret);
        }

        root_object = exjson_value_get_object(root_value);

        nrcaches = exjson_object_get_count(root_object);
        
        if (1!=nrcaches)
        {
            NDRX_CACHE_ERROR("CACHE: invalid service [%s] cache: "
                    "Failed to parse root element",  svc);
            EXFAIL_OUT(ret);
        }
        
        name = (char *)exjson_object_get_name(root_object, 0);
        
        if (0!=strcmp(CACHES_BLOCK, name))
        {
            NDRX_CACHE_ERROR("CACHE: Expected [%s] got [%s]",
                    CACHES_BLOCK, name);
            EXFAIL_OUT(ret);
        }

        /* getting array from root value */
        array = exjson_object_get_array(root_object, CACHES_BLOCK);
        cnt = exjson_array_get_count(array);

#ifdef NDRX_TPCACHE_DEBUG
        NDRX_LOG(log_debug, "Got array values %d", cnt);
#endif
        
        NDRX_CALLOC_OUT(cachesvc, 1, sizeof(ndrx_tpcache_svc_t), ndrx_tpcache_svc_t);
        
        NDRX_STRCPY_SAFE(cachesvc->svcnm, svc);
        
        for (i = 0; i < cnt; i++)
        {
            
            NDRX_CALLOC_OUT(cache, 1, sizeof(ndrx_tpcallcache_t), ndrx_tpcallcache_t);
            
            array_object = exjson_array_get_object(array, i);
            
            /* process flags.. by strtok.. but we need a temp buffer
             * Process flags first as some logic depends on them!
             */
            if (NULL!=(tmp = exjson_object_get_string(array_object, "flags")))
            {
                NDRX_STRCPY_SAFE(flagstr, tmp);
                
                /* clean up the string */
                ndrx_str_strip(flagstr, " \t");
                NDRX_STRCPY_SAFE(cache->flagsstr, flagstr);
                
#ifdef NDRX_TPCACHE_DEBUG
                NDRX_LOG(log_debug, "Processing flags: [%s]", flagstr);
#endif                
                        
                p_flags = strtok_r (flagstr, ",", &saveptr1);
                while (p_flags != NULL)
                {
                    if (0==strcmp(p_flags, "putrex"))
                    {
                        cache->flags|=NDRX_TPCACHE_TPCF_SAVEREG;
                    }
                    else if (0==strcmp(p_flags, "getreplace")) /* default */
                    {
                        cache->flags|=NDRX_TPCACHE_TPCF_REPL;
                    }
                    else if (0==strcmp(p_flags, "getmerge"))
                    {
                        cache->flags|=NDRX_TPCACHE_TPCF_MERGE;
                    }
                    else if (0==strcmp(p_flags, "putfull"))
                    {
                        cache->flags|=NDRX_TPCACHE_TPCF_SAVEFULL;
                    }
                    else if (0==strcmp(p_flags, "inval"))
                    {
                        cache->flags|=NDRX_TPCACHE_TPCF_INVAL;
                    }
                    else if (0==strcmp(p_flags, "next"))
                    {
                        cache->flags|=NDRX_TPCACHE_TPCF_NEXT;
                    }
                    else
                    {
                        NDRX_LOG(log_warn, "For service [%s] buffer index %d, "
                                "invalid flag: [%s] - ignore", svc, i, p_flags);
                        userlog("For service [%s] buffer index %d, "
                                "invalid flag: [%s] - ignore", svc, i, p_flags);
                    }
                    
                    p_flags = strtok_r (NULL, ",", &saveptr1);
                }
            }
            
            if ((cache->flags & NDRX_TPCACHE_TPCF_REPL) && 
                    (cache->flags & NDRX_TPCACHE_TPCF_MERGE) 
                    )
            {
                NDRX_CACHE_TPERROR(TPEINVAL, "CACHE: invalid config - conflicting "
                        "flags `getreplace' and `getreplace' "
                        "for service [%s], buffer index: %d", svc, i);
                EXFAIL_OUT(ret);
            }
            
            /* default to replace */
            if ( !(cache->flags & NDRX_TPCACHE_TPCF_REPL) && 
                    !(cache->flags & NDRX_TPCACHE_TPCF_MERGE) 
                    )
            {
                cache->flags |= NDRX_TPCACHE_TPCF_REPL;
            }
            
            /* set some defaults if not already set... */
            if ((cache->flags & NDRX_TPCACHE_TPCF_SAVEREG) && 
                    (cache->flags & NDRX_TPCACHE_TPCF_SAVEFULL))
            {
                NDRX_CACHE_TPERROR(TPEINVAL, "CACHE: invalid config - conflicting "
                        "flags `putrex' and `putfull' "
                        "for service [%s], buffer index: %d", svc, i);
                EXFAIL_OUT(ret);
            }
            
            if ((cache->flags & NDRX_TPCACHE_TPCF_NEXT) && 
                    !(cache->flags & NDRX_TPCACHE_TPCF_INVAL))
            {
                NDRX_CACHE_TPERROR(TPEINVAL, "CACHE: invalid config - conflicting "
                        "flags `next' can be used only with `inval' "
                        "for service [%s], buffer index: %d", svc, i);
                EXFAIL_OUT(ret);
            }
            
            /* get buffer type */
            if (NULL==(tmp = exjson_object_get_string(array_object, "type")))
            {
                NDRX_CACHE_TPERROR(TPEINVAL, "CACHE: invalid config missing "
                        "[type] for service [%s], buffer index: %d", svc, i);
                EXFAIL_OUT(ret);
            }
            
            NDRX_STRCPY_SAFE(cache->str_buf_type, tmp);
            
             if (NULL!=(tmp = exjson_object_get_string(array_object, "subtype")))
            {
                NDRX_STRCPY_SAFE(cache->str_buf_subtype, tmp);
            }
            
            /* Resolve buffer */
            if (NULL==(cache->buf_type = ndrx_get_buffer_descr(cache->str_buf_type, 
                    cache->str_buf_subtype)))
            {
                NDRX_CACHE_TPERROR(TPEOTYPE, "CACHE: invalid buffer type "
                        "for service [%s], buffer index: %d - Unknown type "
                        "[%s]/subtype[%s]", svc, i, cache->str_buf_type, 
                        cache->str_buf_subtype);
                EXFAIL_OUT(ret);
            }
            
            /* get db name */
            if (!(cache->flags & NDRX_TPCACHE_TPCF_INVAL))
                
            {
                if (NULL==(tmp = exjson_object_get_string(array_object, "cachedb")))
                {
                    NDRX_CACHE_TPERROR(TPEINVAL, "CACHE: invalid config missing "
                            "[cachedb] for service [%s], buffer index: %d", svc, i);
                    EXFAIL_OUT(ret);
                }

                NDRX_STRCPY_SAFE(cache->cachedbnm, tmp);

                /* Resolve the DB */
                if (NULL==(cache->cachedb=ndrx_cache_dbresolve(cache->cachedbnm, mode)))
                {
                    NDRX_LOG(log_error, "%s failed", __func__);
                    EXFAIL_OUT(ret);
                }
            }
            else
            {
                ndrx_tpcache_svc_t *svcc;
                /* 
                 !! TODO: So we are NDRX_TPCACHE_TPCF_INVAL resolve the other cache..
                 * And lookup other keys too of inval cache
                 */
                
                /* Get data for invalidating their cache */
                
                if (NULL==(tmp = exjson_object_get_string(array_object, "inval_svc")))
                {
                    NDRX_CACHE_TPERROR(TPEINVAL, "CACHE: invalid config missing "
                            "[inval_svc] for service [%s], buffer index: %d", svc, i);
                    EXFAIL_OUT(ret);
                }

                NDRX_STRCPY_SAFE(cache->inval_svc, tmp);
                
                
                if (NULL==(tmp = exjson_object_get_string(array_object, "inval_idx")))
                {
                    NDRX_CACHE_TPERROR(TPEINVAL, "CACHE: invalid config missing "
                            "[inval_idx] for service [%s], buffer index: %d", svc, i);
                    EXFAIL_OUT(ret);
                }
                
                cache->inval_idx = atoi(tmp);
                
                /* Find service in cache */
                EXHASH_FIND_STR(M_tpcache_svc, cache->inval_svc, svcc);

                if (NULL==svcc)
                {
                    NDRX_CACHE_TPERROR(TPEINVAL, "CACHE: inval_s    vc [%s] not found, "
                            "or defined later (at this or another config file)");
                    EXFAIL_OUT(ret);
                }
                
                if (NULL==(cache->inval_cache =  ndrx_cache_findtpcall(svcc, 
                    cache->buf_type, NULL, EXFAIL, cache->inval_idx)))
                {
                    NDRX_CACHE_TPERROR(TPEINVAL, "CACHE: tpcall cache not found "
                            "for service [%s], index=%d, type=[%s]", 
                            cache->inval_svc, cache->inval_idx, 
                            cache->buf_type->type);
                    EXFAIL_OUT(ret);
                }   
            }
            
            /* validate the type */
            if (NULL==M_types[cache->buf_type->type_id].pf_get_key)
            {
                NDRX_CACHE_TPERROR(TPEOTYPE, "CACHE: buffer type not supported "
                        "for service [%s], buffer index: %d - Unknown type "
                        "[%s]/subtype[%s]", svc, i, cache->str_buf_type, 
                        cache->str_buf_subtype);
                EXFAIL_OUT(ret);
            }
            
            /* get key format */
            if (NULL==(tmp = exjson_object_get_string(array_object, "keyfmt")))
            {
                NDRX_CACHE_TPERROR(TPEINVAL, "CACHE: invalid config missing "
                        "[keyfmt] for service [%s], buffer index: %d", svc, i);
                EXFAIL_OUT(ret);
            }
            
            NDRX_STRCPY_SAFE(cache->keyfmt, tmp);
            
            /* Rule to be true to save to cache */
            if (NULL==(tmp = exjson_object_get_string(array_object, "rule")))
            {
                NDRX_CACHE_TPERROR(TPEINVAL, "CACHE: invalid config missing "
                        "[rule] for service [%s], buffer index: %d", svc, i);
                EXFAIL_OUT(ret);
            }
            
            NDRX_STRCPY_SAFE(cache->rule, tmp);
            
            
            if (NULL!=(tmp = exjson_object_get_string(array_object, "refreshrule")))
            {
                NDRX_STRCPY_SAFE(cache->rule, tmp);
            }
            
            /* get fields to save */
            if (!(cache->flags & NDRX_TPCACHE_TPCF_SAVEFULL))
            {
                if (NULL==(tmp = exjson_object_get_string(array_object, "save")))
                {
                    NDRX_CACHE_TPERROR(TPEINVAL, "CACHE: invalid config missing "
                            "[save] for service [%s], buffer index: %d", svc, i);
                    EXFAIL_OUT(ret);
                }
                
                NDRX_STRCPY_SAFE(cache->save, tmp);
                
                /* if it is regex, then compile */
                if (cache->flags & NDRX_TPCACHE_TPCF_SAVEREG)
                {
                    if (EXSUCCEED!=ndrx_regcomp(&cache->save_regex, cache->save))
                    {
                        NDRX_CACHE_TPERROR(TPEINVAL, "CACHE: failed to compile [save] "
                            "regex [%s] for svc [%s], "
                            "buffer index: %d - see ndrx logs", svc, cache->save, i);
                    }
                    cache->save_regex_compiled=EXTRUE;
                }
            }
            
            if (NULL!=M_types[cache->buf_type->type_id].pf_process_flags)
            {
                if (EXSUCCEED!=M_types[cache->buf_type->type_id].pf_process_flags(cache, 
                        errdet, sizeof(errdet)))
                    
                {
                    NDRX_CACHE_TPERROR(TPEINVAL, "CACHE: failed to check flags [%s] "
                            "for service [%s], buffer index: %d: %s", 
                            cache->flagsstr, svc, i, errdet);
                    EXFAIL_OUT(ret);
                }
            }
            
            /* Rule to be true to save to cache */
            
            if (NULL!=M_types[cache->buf_type->type_id].pf_rule_compile)
            {
                /* Compile the boolean expression! */
                if (EXSUCCEED!=M_types[cache->buf_type->type_id].pf_rule_compile(
                        cache, errdet, sizeof(errdet)))
                {
                    NDRX_CACHE_TPERROR(TPEINVAL, "CACHE: failed to compile rule [%s] "
                            "for service [%s], buffer index: %d: %s", 
                            cache->rule, svc, i, errdet);
                    EXFAIL_OUT(ret);
                }
            }
            
            /* Get & Compile response rule */
            if (NULL!=(tmp = exjson_object_get_string(array_object, "rsprule")))
            {
                NDRX_STRCPY_SAFE(cache->rsprule, tmp);
                
                /* Compile the boolean expression! */
                if (NULL==(cache->rsprule_tree=Bboolco (cache->rsprule)))
                {
                    NDRX_CACHE_TPERROR(TPEINVAL, "CACHE: failed to "
                            "compile rsprule [%s] "
                            "for service [%s], buffer index: %d: %s", 
                            cache->rsprule, svc, i, Bstrerror(Berror));
                    EXFAIL_OUT(ret);
                }
            }
            /* Add to linked list */
            DL_APPEND(cachesvc->caches, cache);
            
#ifdef NDRX_TPCACHE_DEBUG
            NDRX_TPCACHETPCALL_DUMPCFG(log_debug, cache);
#endif
            
            cache = NULL;
        }
        
        EXHASH_ADD_STR(M_tpcache_svc, svcnm, cachesvc);
        cachesvc = NULL;
    }

out:
            
    /* cleanup code */
    if (NULL!=root_value)
    {
        exjson_value_free(root_value);
    }

    if (NULL!=csection)
    {
        ndrx_keyval_hash_free(csection);
    }

    if (EXSUCCEED!=ret)
    {
        if (NULL!=cache)
        {
            ndrx_cache_tpcallcache_free(cache);
        }
        
        if (NULL!=cachesvc)
        {
            ndrx_cache_svc_free(cachesvc);
        }
        
        /*  Remove all services already initialized... */
        ndrx_cache_svcs_free();
        
        /* Remove all databases */
        ndrx_cache_dbs_free();
    }

    return ret;
}

/**
 * Check response rule, should we cache this or not
 * @param cache cache object
 * @param save_tperrno tperror number
 * @param save_tpurcode user return code
 * @return EXFAIL/EXFALSE/EXTRUE
 */
exprivate int ndrx_cache_chkrsprule(ndrx_tpcallcache_t *cache, 
            long save_tperrno, long save_tpurcode)
{
    int ret = EXFALSE;
    char buf[512];
    UBFH *p_ub = (UBFH *)buf;
    
    if (EXSUCCEED!=Binit(p_ub, sizeof(buf)))
    {
        NDRX_CACHE_TPERROR(TPESYSTEM, "cache: failed to init response check buffer: %s",
                Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    /* Load the data into buffer */
    
    if (EXSUCCEED!=Bchg(p_ub, EX_TPERRNO, 0, (char *)&save_tperrno, 0L))
    {
        NDRX_CACHE_TPERROR(TPESYSTEM, "cache: Failed to set EX_TPERRNO[0] to %ld: %s",
                save_tperrno, Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=Bchg(p_ub, EX_TPURCODE, 0, (char *)&save_tpurcode, 0L))
    {
        NDRX_CACHE_TPERROR(TPESYSTEM, "cache: Failed to set EX_TPURCODE[0] to %ld: %s",
                save_tpurcode, Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    /* Finally evaluate the expression */
    
    if (EXFAIL==(ret=Bboolev(p_ub, cache->rsprule_tree)))
    {
        NDRX_CACHE_TPERROR(TPESYSTEM, "cache: Failed to evalute [%s] "
                "tree: %p expression: %s",
                cache->rsprule, cache->rsprule_tree, Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_debug, "Response expression [%s]: %s", cache->rsprule,
            (EXTRUE==ret?"TRUE":"FALSE"));
    
out:
            
    return ret;

}

/**
 * Save data to cache
 * @param idata
 * @param ilen
 * @param save_tperrno
 * @param save_tpurcode
 * @param nodeid
 * @return EXSUCCEED/EXFAIL/NDRX_TPCACHE_ENOCACHE
 */
expublic int ndrx_cache_save (char *svc, char *idata, 
        long ilen, int save_tperrno, long save_tpurcode, int nodeid, long flags)
{
    int ret = EXSUCCEED;
    /* have a buffer in size of ATMI message */
    char buf[NDRX_MSGSIZEMAX];
    ndrx_tpcache_svc_t *svcc = NULL;
    typed_buffer_descr_t *buf_type;
    buffer_obj_t *buffer_info;
    ndrx_tpcallcache_t *cache;
    ndrx_tpcache_data_t *exdata = (ndrx_tpcache_data_t *)buf;
    unsigned int dbflags;
    int tran_started = EXFALSE;
    EDB_txn *txn;
    char key[NDRX_CACHE_KEY_MAX+1];
    char errdet[MAX_TP_ERROR_LEN+1];
    EDB_val cachedata;
    
    memset(exdata, 0, sizeof(ndrx_tpcache_data_t));
    
    exdata->nodeid = nodeid;
    exdata->saved_tperrno = save_tperrno;
    exdata->saved_tpurcode = save_tpurcode;
    
    /* get current timestamp */
    ndrx_utc_tstamp2(&exdata->t, &exdata->tusec);
    
    /* OK now translate the thing to db format (i.e. make outgoing message) */
    
    
    /* Find service in cache */
    EXHASH_FIND_STR(M_tpcache_svc, svc, svcc);
    
    if (NULL==svcc)
    {
#ifdef NDRX_TPCACHE_DEBUG
        NDRX_LOG(log_debug, "No cache defined for [%s]", svc);
#endif
        ret = NDRX_TPCACHE_ENOCACHE;
        goto out;
    }
    
    if (NULL!=idata)
    {
        if (NULL==(buffer_info = ndrx_find_buffer(idata)))
        {
            ndrx_TPset_error_fmt(TPEINVAL, "%s: Buffer %p not known to system!", 
                    __func__, idata);
            EXFAIL_OUT(ret);
        }
    }
    
    buf_type = &G_buf_descr[buffer_info->type_id];
    
    /* Test the buffers rules */
    if (NULL==(cache = ndrx_cache_findtpcall(svcc, buf_type, idata, ilen, EXFAIL)))
    {
        ret = NDRX_TPCACHE_ENOCACHE;
        goto out;
    }
    
    exdata->atmi_buf_len = NDRX_MSGSIZEMAX - sizeof(ndrx_tpcache_data_t);
            
    if (NULL==M_types[cache->buf_type->type_id].pf_cache_put)
    {
        ret = NDRX_TPCACHE_ENOTYPESUPP;
        goto out;
        
    }
    
    /* Check the response rule if defined */
    
    if (NULL!=cache->rsprule_tree)
    {
        if (EXFAIL==(ret=ndrx_cache_chkrsprule(cache, (long)save_tperrno, 
                save_tpurcode)))
        {
            NDRX_LOG(log_error, "Failed to test response code");
            EXFAIL_OUT(ret);
        }
        
        if (EXFALSE==ret)
        {
            NDRX_LOG(log_info, "Response shall not be saved according to rsp rule");
            ret = EXSUCCEED;
            goto out;
        }
        
        ret = EXSUCCEED;
    }
    
    if (EXSUCCEED!=M_types[cache->buf_type->type_id].pf_cache_put(cache, exdata, 
            buf_type, idata, ilen, flags))
    {
        /* Error shall be set by func */
        NDRX_LOG(log_error, "Failed to convert to cache format!!!");
        EXFAIL_OUT(ret);
        
    }
    
    NDRX_LOG(log_info, "About to cache data for service: [%s]", svc);
    
    
    NDRX_STRCPY_SAFE(key, cache->keyfmt);
       
    /* Build the key... */
    if (EXSUCCEED!=(ret = M_types[buffer_info->type_id].pf_get_key(cache, idata, 
            ilen, key, sizeof(key), errdet, sizeof(errdet))))
    {
        if (NDRX_TPCACHE_ENOKEYDATA==ret)
        {
            NDRX_LOG(log_debug, "Failed to build key (no data for key): %s", errdet);
            goto out;
        }
        else
        {
            NDRX_LOG(log_error, "Failed to build key: ", errdet);
            
            /* generate TP error here! */
            ndrx_TPset_error_fmt(TPESYSTEM, "%s: Failed to build cache key: %s", 
                    __func__, errdet);
            goto out;
        }
            
    }
    
    if (EXSUCCEED!=(ret=ndrx_cache_edb_begin(cache->cachedb, &txn)))
    {
        NDRX_LOG(log_error, "%s: failed to start tran", __func__);
        goto out;
    }
    tran_started = EXTRUE;
    
    
    /* Add the record to the DB, to specific key! */
    if (cache->cachedb->flags & NDRX_TPCACHE_FLAGS_TIMESYNC)
    {
        dbflags = EDB_APPENDDUP;
    }
    else
    {
        dbflags = 0;
    }
    
    cachedata.mv_data = (void *)exdata;
    cachedata.mv_size = exdata->atmi_buf_len + sizeof(ndrx_tpcache_data_t);
    
    
    NDRX_LOG(log_info, "About to put to cache: svc: [%s] key: [%s]: size: %ld",
            svcc->svcnm, key, (long)cachedata.mv_size);
    
    if (EXSUCCEED!=(ret=ndrx_cache_edb_put (cache->cachedb, txn, 
            key, &cachedata, dbflags)))
    {
        NDRX_LOG(log_debug, "Failed to put DB record!");
        goto out;
    }
    
    NDRX_LOG(log_debug, "Data cached");
    
out:

    if (tran_started)
    {
        if (EXSUCCEED==ret)
        {
            ndrx_cache_edb_commit(cache->cachedb, txn);
        }
        else
        {
            ndrx_cache_edb_abort(cache->cachedb, txn);
        }
    }

    return ret;
}

/**
 * Lookup service in cache
 * @param svc service to call
 * @param idata input data buffer
 * @param ilen input len
 * @param odata output data buffer
 * @param olen output len
 * @param flags flags
 * @param should_cache should record be cached?
 * @return EXSUCCEED/EXFAIL (syserr)/NDRX_TPCACHE_ENOKEYDATA (cannot build key)
 */
expublic int ndrx_cache_lookup(char *svc, char *idata, long ilen, 
        char **odata, long *olen, long flags, int *should_cache, 
        int *saved_tperrno, long *saved_tpurcode)
{
    int ret = EXSUCCEED;
    ndrx_tpcache_svc_t *svcc = NULL;
    typed_buffer_descr_t *buf_type;
    buffer_obj_t *buffer_info;
    ndrx_tpcallcache_t *cache;
    char key[NDRX_CACHE_KEY_MAX+1];
    char errdet[MAX_TP_ERROR_LEN+1];
    EDB_txn *txn;
    int cursor_open = EXFALSE;
    EDB_cursor *cursor;
    int tran_started = EXFALSE;
    EDB_val cachedata;
    EDB_val cachedata_update;
    ndrx_tpcache_data_t *exdata;
    ndrx_tpcache_data_t *exdata_update;
    ndrx_tpcallcache_t* el;
    int is_matched;
        
    /* Key size - assume 16K should be fine */
    /* get buffer type & sub-type */
    cachedata_update.mv_size = 0;
    cachedata_update.mv_data = NULL;
        
    /* Find service in cache */
    EXHASH_FIND_STR(M_tpcache_svc, svc, svcc);
    
    if (NULL==svcc)
    {
#ifdef NDRX_TPCACHE_DEBUG
        NDRX_LOG(log_debug, "No cache defined for [%s]", svc);
#endif
        ret = NDRX_TPCACHE_ENOCACHE;
        goto out;
    }
    
    if (NULL!=idata)
    {
        if (NULL==(buffer_info = ndrx_find_buffer(idata)))
        {
            ndrx_TPset_error_fmt(TPEINVAL, "%s: Buffer %p not known to system!", 
                    __func__, idata);
            EXFAIL_OUT(ret);
        }
    }
    
    
    /* TODO: Loop over the tpcallcaches, if `next' flag present, then perform next
     * if we get invalidate their, then delete target records by the key */
    buf_type = &G_buf_descr[buffer_info->type_id];
#if 0
    /* Test the buffers rules */
    if (NULL==(cache = ndrx_cache_findtpcall(svcc, buf_type, idata, ilen, EXFAIL)))
    {
        ret = NDRX_TPCACHE_ENOCACHE;
        goto out;
    }
#endif
    
    /* TODO: *should_cache=EXTRUE; */
    DL_FOREACH(svcc->caches, cache)
    {
        is_matched = EXFALSE;
        
        if (cache->buf_type->type_id == buf_type->type_id)
        {
            if (M_types[el->buf_type->type_id].pf_rule_eval)
            {
                ret = M_types[el->buf_type->type_id].pf_rule_eval(cache, idata, ilen, 
                        errdet, sizeof(errdet));
                if (EXFAIL==ret)
                {
                    NDRX_CACHE_TPERROR(TPEINVAL, "%s: Failed to evaluate buffer [%s]: %s", 
                            __func__, cache->rule, errdet);
                    
                    EXFAIL_OUT(ret);
                }
                else if (EXFALSE==ret)
                {
#ifdef NDRX_TPCACHE_DEBUG
                    NDRX_LOG(log_debug, "Buffer RULE FALSE [%s] - continue", cache->rule);
#endif
                    continue;
                }
                
                is_matched = EXTRUE;
                ret = EXSUCCEED;
            }
            else
            {
                /* We should not get here! */
                NDRX_CACHE_TPERROR(TPEINVAL,"%s: Unsupported buffer type [%s] for cache", 
                                __func__, el->buf_type->type);
                EXFAIL_OUT(ret);
            }
        }
        
        /* if we are here, the cache is matched */
        
        if (!(cache->flags & NDRX_TPCACHE_TPCF_INVAL))
        {
            /* ONLY IN CASE IF NOT INVAL - Check should we refresh? */
            if (NULL!=M_types[cache->buf_type->type_id].pf_refreshrule_eval &&
                    EXEOS!=cache->refreshrule[0])
            {
                ret = M_types[cache->buf_type->type_id].pf_refreshrule_eval(cache, 
                        idata, ilen, errdet, sizeof(errdet));
                if (EXFAIL==ret)
                {
                    /* Failed to eval refresh rule */
                    NDRX_LOG(log_error, "Failed to eval refresh rule: %s", errdet);

                    ndrx_TPset_error_fmt(TPESYSTEM, "Failed to eval refresh rule: %s", 
                            errdet);
                    EXFAIL_OUT(ret);
                }
                else if (EXTRUE==ret)
                {
                    NDRX_LOG(log_info, "Cache will be refreshed - rule matched "
                            "(do not continue cache lookup)");
                    ret = NDRX_TPCACHE_ENOCACHEDATA;
                    goto out;
                }
            }
        }

        /* Test the rule, if and not found then stage to NDRX_TPCACHE_ENOTFOUNADD 
         * OK, we need to build a key
         */

        NDRX_STRCPY_SAFE(key, cache->keyfmt);

        /* Build the key... */
        if (EXSUCCEED!=(ret = M_types[buffer_info->type_id].pf_get_key(cache, idata, 
                ilen, key, sizeof(key), errdet, sizeof(errdet))))
        {
            if (NDRX_TPCACHE_ENOKEYDATA==ret)
            {
                NDRX_LOG(log_debug, "Failed to build key (no data for key): %s", errdet);
                goto out;
            }
            else
            {
                NDRX_LOG(log_error, "Failed to build key: ", errdet);

                /* generate TP error here! */
                ndrx_TPset_error_fmt(TPESYSTEM, "%s: Failed to build cache key: %s", 
                        __func__, errdet);
                goto out;
            }
        }
        
        if (cache->flags & NDRX_TPCACHE_TPCF_INVAL)
        {
            /* TODO: Invalidate their cache */
        }
        
        
        if (cache->flags & NDRX_TPCACHE_TPCF_NEXT)
        {
#ifdef NDRX_TPCACHE_DEBUG
            NDRX_LOG(log_debug, "Next flag present, go to next cache (if have one)");
#endif
            continue;
        }
        else
        {
            break;
        }
        
    }
    
    /* cache not found */
    if (!is_matched)
    {
        
#ifdef NDRX_TPCACHE_DEBUG
        NDRX_LOG(log_debug, "No cache defined for [%s], buffer type [%s] "
                "or all was invalidate", svc, buf_type->type);
#endif
        ret = NDRX_TPCACHE_ENOCACHE;
        goto out;
    }
    
    /* LOOP END */
    
    /* Lookup DB */
    
    if (EXSUCCEED!=(ret=ndrx_cache_edb_begin(cache->cachedb, &txn)))
    {
        NDRX_LOG(log_error, "%s: failed to start tran", __func__);
        goto out;
    }
    tran_started = EXTRUE;
    
    if (cache->cachedb->flags & NDRX_TPCACHE_FLAGS_TIMESYNC)
    {
#ifdef NDRX_TPCACHE_DEBUG
        NDRX_LOG(log_debug, "Performing timesync based complex lookup");
#endif        
        if (EXSUCCEED!=ndrx_cache_edb_set_dupsort(cache->cachedb, txn, 
                ndrx_cache_cmp_fun))
        {
            NDRX_LOG(log_error, "Failed to set dupsort!");
            EXFAIL_OUT(ret);
        }
        
        if (EXSUCCEED!=ndrx_cache_edb_cursor_open(cache->cachedb, txn, &cursor))
        {
            NDRX_LOG(log_error, "Failed to open cursor!");
            EXFAIL_OUT(ret);
        }
        
        /* OK fetch the first rec of cursor, next records we shall kill (if any) */
        /* first: EDB_FIRST_DUP - this we accept and process */
        
        if (EXSUCCEED!=(ret=ndrx_cache_edb_cursor_get(cache->cachedb, cursor,
                    key, &cachedata, EDB_SET_KEY)))
        {
            if (EDB_NOTFOUND!=ret)
            {
                NDRX_LOG(log_error, "Failed to scan for data!");
                EXFAIL_OUT(ret);
            }
            /* no data found */
            ret = NDRX_TPCACHE_ENOCACHEDATA;
            goto out;
        }
        
        /* not sure but we should position on first.. ? Do we need this ? */
        if (EXSUCCEED!=(ret=ndrx_cache_edb_cursor_get(cache->cachedb, cursor,
                    key, &cachedata, EDB_FIRST_DUP)))
        {
            if (EDB_NOTFOUND!=ret)
            {
                NDRX_LOG(log_error, "Failed to scan for data!");
                EXFAIL_OUT(ret);
            }
            /* no data found */
            ret = NDRX_TPCACHE_ENOCACHEDATA;
            goto out;
        }
        
    }
    else
    {
#ifdef NDRX_TPCACHE_DEBUG
        NDRX_LOG(log_debug, "Performing simple lookup");
#endif
        if (EXSUCCEED!=(ret=ndrx_cache_edb_get(cache->cachedb, txn, key, &cachedata)))
        {
            /* error already provided by wrapper */
            NDRX_LOG(log_debug, "%s: failed to get cache by [%s]", __func__, key);
            goto out;
        }
    }
    
    exdata = (ndrx_tpcache_data_t *)cachedata.mv_data;
    /* OK we have a raw data... lets dump something... */
#ifdef NDRX_TPCACHE_DEBUG
    NDRX_LOG(log_debug, "Got cache record for key [%s] of service [%s]", key, svc);
    /* Dump more correctly with admin info */
    NDRX_DUMP(6, "Got cache data", (char *)cachedata.mv_data, 
            (int)cachedata.mv_size);
    NDRX_TPCACHETPCALL_DBDATA(log_debug, exdata);
#endif
    
    /* Error shall be set by func */
    
    if (EXSUCCEED!=M_types[buffer_info->type_id].pf_cache_get(cache, exdata, 
            buf_type, idata, ilen, odata, olen, flags))
    {
        NDRX_LOG(log_error, "%s: Failed to receive data: ", __func__);
        goto out;
    }
    
    *saved_tperrno = exdata->saved_tperrno;
    *saved_tpurcode = exdata->saved_tpurcode;
    
    NDRX_LOG(log_debug, "cache tperrno: %d tpurcode: %ld",
            *saved_tperrno, *saved_tpurcode);
    
    /* Update cache (if needed) */
    
    
    /* perform copy if needed for cache update */
    if ((cache->cachedb->flags & NDRX_TPCACHE_FLAGS_LRU) ||
            (cache->cachedb->flags & NDRX_TPCACHE_FLAGS_HITS))
    {
        cachedata_update.mv_size = cachedata.mv_size;
        cachedata_update.mv_data = NDRX_MALLOC(cachedata.mv_size);
        
        if (NULL==cachedata_update.mv_data)
        {
            int err = errno;
            
            NDRX_CACHE_TPERROR(TPEOS, "Failed to allocate %ld bytes: %s",
                    (long)cachedata_update.mv_size, strerror(err));
        }
        
        memcpy(cachedata_update.mv_data, cachedata.mv_data, cachedata.mv_size);
        
        exdata_update = (ndrx_tpcache_data_t *)cachedata_update.mv_data;
        /* ok this might overflow, then it will be time for cache to reset...
         * but that will be long number of requests away...
         */

        exdata_update->hits++;
        ndrx_utc_tstamp2(&exdata_update->t, &exdata_update->tusec);

#ifdef NDRX_TPCACHE_DEBUG        
        NDRX_LOG(log_debug, "hits=%ld t=%ld t=%ld", exdata_update->hits,
                exdata_update->t, exdata_update->tusec);
#endif
        if (cursor_open)
        {
            edb_cursor_close(cursor);
        }
        cursor_open=EXFALSE;
        
        /* delete all records */
        if (EXSUCCEED!=(ret=ndrx_cache_edb_del (cache->cachedb, txn, 
            key, NULL)))
        {
            if (EDB_NOTFOUND==ret)
            {
                ret=EXSUCCEED;
            }
            else
            {
                EXFAIL_OUT(ret);
            }
        }
        
        /* Add record */
        
    }
    else if (cache->cachedb->flags & NDRX_TPCACHE_FLAGS_TIMESYNC)
    {
        /* fetch next for dups and remove them.. if any.. */
        /* next: MDB_NEXT_DUP  - we kill this! */
        while (EXSUCCEED==(ret=ndrx_cache_edb_cursor_get(cache->cachedb, cursor,
                    key, &cachedata, EDB_NEXT_DUP)))
        {
            /* delete the record, not needed, some old cache rec */
            
            if (EXSUCCEED!=(ret=ndrx_cache_edb_del (cache->cachedb, txn, 
                    key, &cachedata)))
            {
                if (ret!=EDB_NOTFOUND)
                {
                    /* if not found maybe next key will be found */
                    break;
                }
            }
        }
        
        if (ret!=EDB_NOTFOUND)
        {
            EXFAIL_OUT(ret);
        }
        else
        {
            ret = EXSUCCEED;
        }
    }
    

    
out:

    if (cursor_open)
    {
        edb_cursor_close(cursor);
    }

    if (tran_started)
    {
        if (EXSUCCEED==ret)
        {
            ndrx_cache_edb_commit(cache->cachedb, txn);
        }
        else
        {
            ndrx_cache_edb_abort(cache->cachedb, txn);
        }
    }

    return ret;
}

