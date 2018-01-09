/* 
** ATMI level cache
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
#include <ndrstandard.h>
#include <atmi.h>
#include <atmi_tls.h>
#include <string.h>
#include "thlock.h"
#include "userlog.h"
#include <exparson.h>
#include <atmi_cache.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

#define NDRX_TPCACHE_DEBUG

#define CACHES_BLOCK    "caches"
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/

exprivate ndrx_tpcache_db_t *M_tpcache_db = NULL; /* ptr to cache database */
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

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
 * Reslove cache db
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
                        csection))
    {
        NDRX_LOG(log_error, "%s: Failed to get section [%s]: %s", 
                __func__, cachesection, Nstrerror(Nerror));
        userlog("%s: Failed to get section [%s]: %s", __func__, cachesection, 
                Nstrerror(Nerror));
        EXFAIL_OUT(ret);
   }
    
   /* TODO: Parse arguments in the loop */
    
out:

    if (NULL!=csection)
    {
        ndrx_keyval_hash_free(csection);
    }

    return ret;
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
    size_t i;
    
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
            
            /* Resolve the cache db - if missing load & open */
            
            
        }
    }

out:

    if (NULL!=csection)
    {
        ndrx_keyval_hash_free(csection);
    }

    return ret;
}
