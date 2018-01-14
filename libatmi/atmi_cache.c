/* 
** ATMI level cache
** No using named databases. Each environment is associated with single db
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
#include <exparson.h>
#include <atmi_cache.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

#define NDRX_TPCACHE_DEBUG

#define CACHES_BLOCK    "caches"

#define CACHE_MAX_READERS_DFLT      1000
#define CACHE_MAP_SIZE_DFLT         160000 /* 160K */
#define CACHE_PERMS_DFLT            0664
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/

exprivate ndrx_tpcache_db_t *M_tpcache_db = NULL; /* ptr to cache database */
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Map unix error
 * @param unixerr unix error
 * @return TP Error
 */
expublic int ndrx_cache_maperr(int unixerr)
{
    int ret = TPEOS;
    
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
exprivate void ndrx_cache_close_db(ndrx_tpcache_db_t *db)
{
    /* func checks the dbi validity */
    edb_dbi_close(db->env, db->dbi);

    if (NULL!=db->env)
    {
        edb_env_close(db->env);
    }
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
        NDRX_LOG(log_error, "%s: Failed to get section [%s]: %s", 
                __func__, cachesection, Nstrerror(Nerror));
        userlog("%s: Failed to get section [%s]: %s", __func__, cachesection, 
                Nstrerror(Nerror));
        EXFAIL_OUT(ret);
   }
    
   /* Parse arguments in the loop */
    
    NDRX_MALLOC_OUT(db, sizeof(ndrx_tpcache_db_t), ndrx_tpcache_db_t);
    
    /* Set max_readers, map_size defaults */
    
    db->max_readers = CACHE_MAX_READERS_DFLT;
    db->map_size = CACHE_MAP_SIZE_DFLT;
    db->perms = CACHE_PERMS_DFLT;
    
    EXHASH_ITER(hh, csection, val, val_tmp)
    {
        if (0==strcmp(val->key, "dbpath"))
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
                else
                {
                    /* unknown flag */
                    NDRX_LOG(log_warn, "Ignoring unknown cachedb [%s] flag: [%s]",
                            cachedb, p);
                    userlog("Ignoring unknown cachedb [%s] flag: [%s]",
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
        NDRX_LOG(log_error, "CACHE: Failed to open env for [%s]: %s", 
                db->cachedb, edb_strerror(ret));
        userlog("CACHE: Failed to open env for [%s]: %s", 
                db->cachedb, edb_strerror(ret));
        
        ndrx_TPset_error_fmt(ndrx_cache_maperr(ret), 
                "CACHE: Failed to create env for [%s]: %s", 
                db->cachedb, edb_strerror(errno));
        
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=(ret=edb_env_set_maxreaders(db->env, db->max_readers)))
    {
        NDRX_LOG(log_error, "CACHE: Failed to set max readers for [%s]: %s", 
                db->cachedb, edb_strerror(ret));
        userlog("CACHE: Failed to set max readers for [%s]: %s", 
                db->cachedb, edb_strerror(ret));
        
        ndrx_TPset_error_fmt(ndrx_cache_maperr(ret), 
                "CACHE: Failed to set max readers for [%s]: %s", 
                db->cachedb, edb_strerror(ret));
        
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=(ret=edb_env_set_mapsize(db->env, db->map_size)))
    {
        NDRX_LOG(log_error, "CACHE: Failed to set map size for [%s]: %s", 
                db->cachedb, edb_strerror(ret));
        userlog("Failed to set map size for [%s]: %s", 
                db->cachedb, edb_strerror(ret));
        
        ndrx_TPset_error_fmt(ndrx_cache_maperr(ret), 
                "Failed to set map size for [%s]: %s", 
                db->cachedb, edb_strerror(ret));
        
        EXFAIL_OUT(ret);
    }
    
    /* Open the DB */
    if (EXSUCCEED!=(ret=edb_env_open(db->env, db->resource, 0L, db->perms)))
    {
        NDRX_LOG(log_error, "Failed to open env [%s]: %s", 
                db->cachedb, edb_strerror(ret));
        userlog("Failed to open env [%s]: %s", 
                db->cachedb, edb_strerror(ret));
        
        ndrx_TPset_error_fmt(ndrx_cache_maperr(ret), 
                "Failed to open env [%s]: %s", 
                db->cachedb, edb_strerror(ret));
        
        EXFAIL_OUT(ret);
    }
    
    /* Prepare the DB */
    if (EXSUCCEED!=(ret=edb_txn_begin(db->env, NULL, 0, &txn)))
    {
        NDRX_LOG(log_error, "Failed to begin transaction for [%s]: %s", 
                db->cachedb, edb_strerror(ret));
        userlog("Failed to begin transaction for [%s]: %s", 
                db->cachedb, edb_strerror(ret));
        
        ndrx_TPset_error_fmt(ndrx_cache_maperr(ret), 
                "Failed to begin transaction for [%s]: %s", 
                db->cachedb, edb_strerror(ret));
        
        EXFAIL_OUT(ret);
    }
    
    /* open named db */
    if (EXSUCCEED!=(ret=edb_dbi_open(txn, NULL, 0, &db->dbi)))
    {
        NDRX_LOG(log_error, "Failed to open named db for [%s]: %s", 
                db->cachedb, edb_strerror(ret));
        userlog("Failed to open named db for [%s]: %s", 
                db->cachedb, edb_strerror(ret));
        
        ndrx_TPset_error_fmt(ndrx_cache_maperr(ret), 
                "Failed to open named db for [%s]: %s", 
                db->cachedb, edb_strerror(ret));
        
        EXFAIL_OUT(ret);
    }
    
    if (NDRX_TPCACH_INIT_BOOT==mode)
    {
        NDRX_LOG(log_info, "Resetting cache db [%s]", db->cachedb);
        if (EXSUCCEED!=(ret=edb_drop(txn, db->dbi, 0)))
        {
            NDRX_LOG(log_error, "CACHE: Failed to clear db: [%s]: %s", 
                    db->cachedb, edb_strerror(ret));
            userlog("CACHE: Failed to clear db: [%s]: %s", 
                    db->cachedb, edb_strerror(ret));

            ndrx_TPset_error_fmt(ndrx_cache_maperr(ret), 
                    "CACHE: Failed to clear db: [%s]: %s", 
                    db->cachedb, edb_strerror(ret));

            EXFAIL_OUT(ret);
        }
        
    }
    
    /* commit the tran */
    if (EXSUCCEED!=(ret=edb_txn_commit(txn)))
    {
        NDRX_LOG(log_error, "Failed to open named db for [%s]: %s", 
                db->cachedb, edb_strerror(ret));
        userlog("Failed to open named db for [%s]: %s", 
                db->cachedb, edb_strerror(ret));
        
        ndrx_TPset_error_fmt(ndrx_cache_maperr(ret), 
                "Failed to open named db for [%s]: %s", 
                db->cachedb, edb_strerror(ret));
        txn = NULL;
        EXFAIL_OUT(ret);        
    }
    
    /* Add object to the hash */
    
    EXHASH_ADD_PTR(M_tpcache_db, cachedb, db);
    
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
            ndrx_cache_close_db(db);
            NDRX_FREE((char *)db);
        }       
        return NULL;
    }
    else
    {
        return db;
    }
}

/**
 * Normal init (used by server & clients)
 * @param mode See NDRX_TPCACH_INIT_*
 */
expublic int ndrx_cache_init(int mode)
{
    int ret = EXSUCCEED;
    char *svc;
    EXJSON_Value *root_value, *array_value;
    EXJSON_Object *root_object, *array_object;
    EXJSON_Array *array;
    int type;
    int nrcaches;
    char *name;
    int cnt;
    const char *tmp;
    size_t i;
    ndrx_tpcallcache_t *cache = NULL;
    
    ndrx_inicfg_section_keyval_t * csection = NULL, *val = NULL, *val_tmp = NULL;
    
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
            NDRX_LOG(log_error, "cache: invalid service [%s] cache: Failed "
                    "to parse root element", svc);
            userlog("cache: invalid service [%s] cache: Failed to parse root element", 
                    svc);
            EXFAIL_OUT(ret);
        }

        root_object = exjson_value_get_object(root_value);

        nrcaches = exjson_object_get_count(root_object);
        
        if (1!=nrcaches)
        {
            NDRX_LOG(log_error, "cache: Expected single element in JSON block: [%s], "
                    "got nr: [%d]", CACHES_BLOCK, nrcaches);
            
            userlog("cache: invalid service [%s] cache: Failed to parse root element", 
                    svc);
            
            EXFAIL_OUT(ret);
        }
        
        name = (char *)exjson_object_get_name(root_object, 0);
        
        if (0!=strcmp(CACHES_BLOCK, name))
        {
            NDRX_LOG(log_error, "cache: Expected [%s] got [%s]",
                    CACHES_BLOCK, name);
            
            userlog("cache: Expected [%s] got [%s]",
                    CACHES_BLOCK, name);
            EXFAIL_OUT(ret);
        }

        /* getting array from root value */
        array = exjson_object_get_array(root_object, CACHES_BLOCK);
        cnt = exjson_array_get_count(array);

#ifdef NDRX_TPCACHE_DEBUG
        NDRX_LOG(log_debug, "Got array values %d", cnt);
#endif
        for (i = 0; i < cnt; i++)
        {
            array_object = exjson_array_get_object(array, i);
            
            
            NDRX_MALLOC_OUT(cache, sizeof(ndrx_tpcallcache_t), ndrx_tpcallcache_t);
            
            /* get db name */
            if (NULL==(tmp = exjson_object_get_string(array_object, "cachedb")))
            {
                NDRX_LOG(log_error, "CACHE: invalid config - missing [cachedb] "
                        "for service [%s], buffer index: %d", svc, i);
                userlog("CACHE: invalid config - missing [cachedb] for service [%s], "
                        "buffer index: %d", svc, i);

                ndrx_TPset_error_fmt(TPEINVAL, "CACHE: invalid config missing "
                        "[cachedb] for service [%s], buffer index: %d", svc, i);
                EXFAIL_OUT(ret);
            }
            
            NDRX_STRCPY_SAFE(cache->cachedbnm, tmp);
            
            /* Resolve the DB */
            if (EXSUCCEED!=(cache->cachedb=ndrx_cache_dbresolve(cache->cachedbnm, mode)))
            {
                NDRX_LOG(log_error, "%s failed", __func__);
                EXFAIL_OUT(ret);
            }
            
            /* get buffer type */
            if (NULL==(tmp = exjson_object_get_string(array_object, "type")))
            {
                NDRX_LOG(log_error, "CACHE: invalid config - missing [type] "
                        "for service [%s], buffer index: %d", svc, i);                
                userlog("CACHE: invalid config - missing [type] for service [%s], "
                        "buffer index: %d", svc, i);
                ndrx_TPset_error_fmt(TPEINVAL, "CACHE: invalid config missing "
                        "[type] for service [%s], buffer index: %d", svc, i);
                EXFAIL_OUT(ret);
            }
            
            NDRX_STRCPY_SAFE(cache->str_buf_type, tmp);
            
            if (NULL!=(tmp = exjson_object_get_string(array_object, "type")))
            {
                NDRX_STRCPY_SAFE(cache->str_buf_subtype, tmp);
            }
            
            /* Resolve buffer */
            if (NULL==(cache->buf_type = ndrx_get_buffer_descr(cache->str_buf_type, 
                    cache->str_buf_subtype)))
            {
                NDRX_LOG(log_error, "CACHE: invalid buffer type "
                        "for service [%s], buffer index: %d - Unknown type "
                        "[%s]/subtype[%s]", svc, i, cache->str_buf_type, 
                        cache->str_buf_subtype);
                
                userlog("CACHE: invalid buffer type "
                        "for service [%s], buffer index: %d - Unknown type "
                        "[%s]/subtype[%s]", svc, i, cache->str_buf_type, 
                        cache->str_buf_subtype);
                
                ndrx_TPset_error_fmt(TPEOTYPE, "CACHE: invalid buffer type "
                        "for service [%s], buffer index: %d - Unknown type "
                        "[%s]/subtype[%s]", svc, i, cache->str_buf_type, 
                        cache->str_buf_subtype);
                EXFAIL_OUT(ret);
            }
            
            /* get key format */
            if (NULL==(tmp = exjson_object_get_string(array_object, "keyfmt")))
            {
                NDRX_LOG(log_error, "CACHE: invalid config - missing [keyfmt] "
                        "for service [%s], buffer index: %d", svc, i);                
                userlog("CACHE: invalid config - missing [keyfmt] for service [%s], "
                        "buffer index: %d", svc, i);
                ndrx_TPset_error_fmt(TPEINVAL, "CACHE: invalid config missing "
                        "[keyfmt] for service [%s], buffer index: %d", svc, i);
                EXFAIL_OUT(ret);
            }
            
            NDRX_STRCPY_SAFE(cache->keyfmt, tmp);
            
            /* get fields to save */
            if (NULL==(tmp = exjson_object_get_string(array_object, "save")))
            {
                NDRX_LOG(log_error, "CACHE: invalid config - missing [save] "
                        "for service [%s], buffer index: %d", svc, i);                
                userlog("CACHE: invalid config - missing [save] for service [%s], "
                        "buffer index: %d", svc, i);
                ndrx_TPset_error_fmt(TPEINVAL, "CACHE: invalid config missing "
                        "[save] for service [%s], buffer index: %d", svc, i);
                EXFAIL_OUT(ret);
            }
            
            NDRX_STRCPY_SAFE(cache->save, tmp);
            
            /* Rule to be true to save to cache */
            if (NULL==(tmp = exjson_object_get_string(array_object, "rule")))
            {
                NDRX_LOG(log_error, "CACHE: invalid config - missing [rule] "
                        "for service [%s], buffer index: %d", svc, i);                
                userlog("CACHE: invalid config - missing [rule] for service [%s], "
                        "buffer index: %d", svc, i);
                ndrx_TPset_error_fmt(TPEINVAL, "CACHE: invalid config missing "
                        "[rule] for service [%s], buffer index: %d", svc, i);
                EXFAIL_OUT(ret);
            }
            
            NDRX_STRCPY_SAFE(cache->rule, tmp);
            
            /* TODO: Compile the boolean expression! */
            
            /* TODO: Get & Compile response rule */
            
            
#ifdef NDRX_TPCACHE_DEBUG
            NDRX_LOG(log_error, "cache[%d]: Object [%s] Value [%s]", i,
                    "cachedb", exjson_object_get_string(array_object, "cachedb"));
            NDRX_LOG(log_error, "cache[%d]: Object [%s] Value [%s]", i,
                    "buffer", exjson_object_get_string(array_object, "buffer"));
            NDRX_LOG(log_error, "cache[%d]: Object [%s] Value [%s]", i,
                    "keyfmt", exjson_object_get_string(array_object, "keyfmt"));
            NDRX_LOG(log_error, "cache[%d]: Object [%s] Value [%s]", i,
                    "save", exjson_object_get_string(array_object, "save"));
            NDRX_LOG(log_error, "cache[%d]: Object [%s] Value [%s]", i,
                    "rule", exjson_object_get_string(array_object, "rule"));
#endif
            
            /* Resolve the cache db - if missing load & open 
            expublic ndrx_tpcache_db_t* ndrx_cache_dbresolve(char *cachedb, int mode);
            */
            
            
        }
    }

out:

    if (NULL!=csection)
    {
        ndrx_keyval_hash_free(csection);
    }

    if (EXSUCCEED!=ret && NULL!=cache)
    {
        NDRX_FREE(cache);
    }

    return ret;
}
