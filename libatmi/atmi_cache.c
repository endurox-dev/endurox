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
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
expublic ndrx_tpcache_db_t *ndrx_G_tpcache_db = NULL; /* ptr to cache database */
expublic ndrx_tpcache_svc_t *ndrx_G_tpcache_svc = NULL; /* service cache       */
/*---------------------------Prototypes---------------------------------*/

/* NOTE! Table shall contain all defined buffer types  */
expublic ndrx_tpcache_typesupp_t ndrx_G_tpcache_types[] =
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
    if (NULL!=ndrx_G_tpcache_db && NULL!=ndrx_G_tpcache_svc)
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
    
    EXHASH_FIND_STR( ndrx_G_tpcache_db, cachedb, ret);
    
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
            NULL!=ndrx_G_tpcache_types[tpc->buf_type->type_id].pf_cache_delete)
    {
        ndrx_G_tpcache_types[tpc->buf_type->type_id].pf_cache_delete(tpc);
    }
    
    if (NULL!=tpc->rsprule_tree)
    {
        Btreefree(tpc->rsprule_tree);
    }
    
    if (tpc->saveproj.regex_compiled)
    {
        ndrx_regfree(&tpc->saveproj.regex);
    }
    
    if (tpc->delproj.regex_compiled)
    {
        ndrx_regfree(&tpc->delproj.regex);
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
    EXHASH_ITER(hh, ndrx_G_tpcache_svc, el, elt)
    {
        EXHASH_DEL(ndrx_G_tpcache_svc, el);
        ndrx_cache_svc_free(el);
    }
}

/**
 * Delete all databases open
 */
expublic void ndrx_cache_dbs_free(void)
{
    ndrx_tpcache_db_t *el, *elt;
    EXHASH_ITER(hh, ndrx_G_tpcache_db, el, elt)
    {
        EXHASH_DEL(ndrx_G_tpcache_db, el);
        ndrx_cache_db_free(el);
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
    
    db->max_readers = NDRX_CACHE_MAX_READERS_DFLT;
    db->map_size = NDRX_CACHE_MAP_SIZE_DFLT;
    db->perms = NDRX_CACHE_PERMS_DFLT;
    
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
                else if (0==strcmp(p, "bcastput"))
                {
                    db->flags|=NDRX_TPCACHE_FLAGS_BCASTPUT;
                }
                else if (0==strcmp(p, "bcastdel"))
                {
                    db->flags|=NDRX_TPCACHE_FLAGS_BCASTDEL;
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
    
    EXHASH_ADD_STR(ndrx_G_tpcache_db, cachedb, db);
    
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
            
            if (ndrx_G_tpcache_types[el->buf_type->type_id].pf_rule_eval)
            {
                ret = ndrx_G_tpcache_types[el->buf_type->type_id].pf_rule_eval(el, 
                        idata, ilen, errdet, sizeof(errdet));
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
        
        if (0!=strcmp(NDRX_CACHES_BLOCK, name))
        {
            NDRX_CACHE_ERROR("CACHE: Expected [%s] got [%s]",
                    NDRX_CACHES_BLOCK, name);
            EXFAIL_OUT(ret);
        }

        /* getting array from root value */
        array = exjson_object_get_array(root_object, NDRX_CACHES_BLOCK);
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
                    /*
                     * New flags: delrex, delfull - used for delete buffer prepration
                     * if not set, defaults to delfull
                     */
                    if (0==strcmp(p_flags, "delrex"))
                    {
                        cache->flags|=NDRX_TPCACHE_TPCF_DELREG;
                    }
                    else if (0==strcmp(p_flags, "delfull"))
                    {
                        cache->flags|=NDRX_TPCACHE_TPCF_DELFULL;
                    }
                    else if (0==strcmp(p_flags, "putrex"))
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
                EXHASH_FIND_STR(ndrx_G_tpcache_svc, cache->inval_svc, svcc);

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
            if (NULL==ndrx_G_tpcache_types[cache->buf_type->type_id].pf_get_key)
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
                
                NDRX_STRCPY_SAFE(cache->saveproj.expression, tmp);
                
                /* if it is regex, then compile */
                if (cache->flags & NDRX_TPCACHE_TPCF_SAVEREG)
                {
                    if (EXSUCCEED!=ndrx_regcomp(&cache->saveproj.regex, 
                            cache->saveproj.expression))
                    {
                        NDRX_CACHE_TPERROR(TPEINVAL, "CACHE: failed to compile [save] "
                            "regex [%s] for svc [%s], "
                            "buffer index: %d - see ndrx logs", 
                                svc, cache->saveproj.expression, i);
                    }
                    cache->saveproj.regex_compiled=EXTRUE;
                }
            }
            
            /* process delete fields */
            if (!(cache->flags & NDRX_TPCACHE_TPCF_DELFULL))
            {
                if (NULL!=(tmp = exjson_object_get_string(array_object, "delete")))
                {                    
                    NDRX_STRCPY_SAFE(cache->delproj.expression, tmp);

                    /* if it is regex, then compile */
                    if (cache->flags & NDRX_TPCACHE_TPCF_DELREG)
                    {
                        if (EXSUCCEED!=ndrx_regcomp(&cache->delproj.regex, 
                                cache->delproj.expression))
                        {
                            NDRX_CACHE_TPERROR(TPEINVAL, "CACHE: failed to "
                                "compile [delete] regex [%s] for svc [%s], "
                                "buffer index: %d - see ndrx logs", 
                                        svc, cache->delproj.expression, i);
                        }
                        cache->delproj.regex_compiled=EXTRUE;
                    }
                }
            }
            
            if (NULL!=ndrx_G_tpcache_types[cache->buf_type->type_id].pf_process_flags)
            {
                if (EXSUCCEED!=ndrx_G_tpcache_types[cache->buf_type->type_id].pf_process_flags(cache, 
                        errdet, sizeof(errdet)))
                    
                {
                    NDRX_CACHE_TPERROR(TPEINVAL, "CACHE: failed to check flags [%s] "
                            "for service [%s], buffer index: %d: %s", 
                            cache->flagsstr, svc, i, errdet);
                    EXFAIL_OUT(ret);
                }
            }
            
            /* Rule to be true to save to cache */
            
            if (NULL!=ndrx_G_tpcache_types[cache->buf_type->type_id].pf_rule_compile)
            {
                /* Compile the boolean expression! */
                if (EXSUCCEED!=ndrx_G_tpcache_types[cache->buf_type->type_id].pf_rule_compile(
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
        
        EXHASH_ADD_STR(ndrx_G_tpcache_svc, svcnm, cachesvc);
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
