/* 
** ATMI level cache
** No using named databases. Each environment is associated with single db
** To resolve conflicts in cluster mode we shall compare time with a microseconds
** for this we will extend tppost so that we can send rval as userfield1 and
** rcode as userfield2. When cache server receives request we add the records
** even if it is duplicate (for this timesync flag must be present). Then when
** we read all records, if two or more records found, then we will use the youngest
** one by UTC. If times stamps equals, then we use record as priority from the cluster
** node with a highest node id number.Z
** TODO: If want in feature stream latent commands (LCS tech), 
** then we need locking! if config is changed.
**
** @file atmi_cache_init.c
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
expublic ndrx_tpcache_phydb_t *ndrx_G_tpcache_phydb = NULL; /* ptr to phys database */
expublic ndrx_tpcache_db_t *ndrx_G_tpcache_db = NULL; /* ptr to cache database */
expublic ndrx_tpcache_svc_t *ndrx_G_tpcache_svc = NULL; /* service cache       */
/*---------------------------Prototypes---------------------------------*/

/* NOTE! Table shall contain all defined buffer types  */
expublic ndrx_tpcache_typesupp_t ndrx_G_tpcache_types[] =
{
    /* typeid (idx), rule_compile func,         rule eval func,             refresh rule eval  */
    {BUF_TYPE_UBF, ndrx_cache_rulcomp_ubf,      ndrx_cache_ruleval_ubf,     ndrx_cache_refeval_ubf, 
                    ndrx_cache_keyget_ubf,      ndrx_cache_get_ubf,         ndrx_cache_put_ubf, 
                    ndrx_cache_del_ubf,         ndrx_cache_proc_flags_ubf,  ndrx_cache_delete_ubf, ndrx_cache_maxreject_ubf},
    /* dummy */
    {1,             NULL,                       NULL,                       NULL,
                    NULL,                       NULL,                       NULL,
                    NULL,                       NULL,                       NULL, NULL},
                    
    {BUF_TYPE_INIT, NULL,                       NULL,                       NULL,
                    NULL,                       NULL,                       NULL,
                    NULL,                       NULL,                       NULL, NULL},
                    
    {BUF_TYPE_NULL, NULL,                       NULL,                       NULL,
                    NULL,                       NULL,                       NULL,
                    NULL,                       NULL,                       NULL, NULL},
                    
    {BUF_TYPE_STRING,NULL,                      NULL,                       NULL,
                    NULL,                       NULL,                       NULL,
                    NULL,                       NULL,                       NULL, NULL},
                    
    {BUF_TYPE_CARRAY, NULL,                     NULL,                       NULL,
                    NULL,                       NULL,                       NULL,
                    NULL,                       NULL,                       NULL, NULL},
                    
    {BUF_TYPE_JSON, NULL,                       NULL,                       NULL,
                    NULL,                       NULL,                       NULL,
                    NULL,                       NULL,                       NULL, NULL},
                    
    {BUF_TYPE_VIEW, NULL,                       NULL,                       NULL,
                    NULL,                       NULL,                       NULL,
                    NULL,                       NULL,                       NULL, NULL},
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
 * Get the physical database name
 * @param cachedb cache name
 * @return ptr to cache db or NULL if not found
 */
expublic ndrx_tpcache_phydb_t* ndrx_cache_phydbget(char *cachedb)
{
    ndrx_tpcache_phydb_t *ret;
    
    EXHASH_FIND_STR( ndrx_G_tpcache_phydb, cachedb, ret);
    
    return ret;
}

/**
 * Close database, physical
 * @param db descr struct
 */
exprivate void ndrx_cache_phydb_free(ndrx_tpcache_phydb_t *phydb)
{

    phydb->num_usages--;
    
    if (phydb->num_usages<=0)
    {
        if (NULL!=phydb->env)
        {
            edb_env_close(phydb->env);
        }
        EXHASH_DEL(ndrx_G_tpcache_phydb, phydb);
        NDRX_FREE(phydb);
    }
}


/**
 * Close database, close also reference to physical db
 * @param db descr struct
 */
exprivate void ndrx_cache_db_free(ndrx_tpcache_db_t *db)
{
    /* func checks the dbi validity */
    if (NULL!=db->phy)
    {
        edb_dbi_close(db->phy->env, db->dbi);
        ndrx_cache_phydb_free(db->phy);
        
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
    
    if (NULL!=tpc->keygroupmrej)
    {
        NDRX_FREE(tpc->keygroupmrej);
    }
    
    if (NULL!=tpc->keygroupmrej_abuf)
    {
        tpfree(tpc->keygroupmrej_abuf);
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
 * Return hash handler of the global cache database
 * @return ptr to cache DB
 */
expublic ndrx_tpcache_db_t *ndrx_cache_dbgethash(void)
{
    return ndrx_G_tpcache_db;
}


/**
 * Sort data blocks by date
 * @param a
 * @param b
 * @return 
 */
exprivate int sort_data_bydate(const EDB_val *a, const EDB_val *b)
{
    ndrx_tpcache_data_t *ad = (ndrx_tpcache_data_t *)a->mv_data;
    ndrx_tpcache_data_t *bd = (ndrx_tpcache_data_t *)b->mv_data;
    
    /* to get newer rec first, we change the compare order */
    return ndrx_utc_cmp(&bd->t, &bd->tusec, &ad->t, &ad->tusec);
}

/**
 * Resolve physical db
 * @param db db object with filled db name and resource
 * @return EXSUCCEED/EXFAIL
 */
exprivate int ndrx_cache_phydb_getref(ndrx_tpcache_db_t *db)
{
    int ret = EXSUCCEED;
    ndrx_tpcache_phydb_t *phy;
    
    if (NULL!=(db->phy = ndrx_cache_phydbget(db->cachedbphy)))
    {
        db->phy->num_usages++;
        
        NDRX_LOG(log_debug, "Cache db [%s] already loaded, new usage: %d", 
                db->cachedbphy, db->phy->num_usages);
        goto out;       
    }
    
    /* allocate new phy object */
    NDRX_CALLOC_OUT(phy, 1, sizeof(ndrx_tpcache_phydb_t), ndrx_tpcache_phydb_t);
    
    /* if not found, then open the env, configure the db... and increment counter */
    /* allocate physical db, if exist one.. */
    /* Open the database */
    if (EXSUCCEED!=(ret=edb_env_create(&phy->env)))
    {
        NDRX_CACHE_TPERROR(ndrx_cache_maperr(ret), 
                "CACHE: Failed to create env for [%s]: %s", 
                db->cachedb, edb_strerror(errno));
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=(ret=edb_env_set_maxreaders(phy->env, db->max_readers)))
    {
        NDRX_CACHE_TPERROR(ndrx_cache_maperr(ret), 
                "CACHE: Failed to set max readers for [%s]: %s", 
                db->cachedb, edb_strerror(ret));
        
        EXFAIL_OUT(ret);
    }

    if (EXSUCCEED!=(ret=edb_env_set_maxdbs(phy->env, db->max_dbs)))
    {
        NDRX_CACHE_TPERROR(ndrx_cache_maperr(ret), 
                "Failed to set max dbs for [%s]: %s", 
                db->cachedb, edb_strerror(ret));
        
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=(ret=edb_env_set_mapsize(phy->env, db->map_size)))
    {
        NDRX_CACHE_TPERROR(ndrx_cache_maperr(ret), 
                "Failed to set map size for [%s]: %s", 
                db->cachedb, edb_strerror(ret));
        
        EXFAIL_OUT(ret);
    }
    
    NDRX_STRCPY_SAFE(phy->resource, db->resource);
    NDRX_STRCPY_SAFE(phy->cachedb, db->cachedbphy);
    
    /* Open the DB 
     * In case of named database, we shall search for existing env.
     * if not found, only then we open.
     */
    if (EXSUCCEED!=(ret=edb_env_open(phy->env, db->resource, 0L, db->perms)))
    {
        NDRX_CACHE_TPERROR(ndrx_cache_maperr(ret), 
                "Failed to open env [%s] [%s]: %s", 
                db->cachedb, db->resource, edb_strerror(ret));
        
        EXFAIL_OUT(ret);
    }
    
    /* Add record to hash */
    EXHASH_ADD_STR(ndrx_G_tpcache_phydb, cachedb, phy);
    
    
    phy->num_usages++;
    db->phy = phy;
    
out:
    return ret;
}

/**
 * Resolve cache db
 * @param cachedb name of cache db
 * @param cachedbnam database name in physical db
 * @param cachedbphy physical database name if cachedbname same as cachedb, then
 * two level lookup will be employed instead of three.
 * @param mode either normal or create mode (started by ndrxd)
 * @return NULL in case of failure, non NULL, resolved ptr to cache db record
 */
expublic ndrx_tpcache_db_t* ndrx_cache_dbresolve(char *cachedb, int mode)
{
    int ret = EXSUCCEED;
    ndrx_tpcache_db_t* db;
    ndrx_inicfg_section_keyval_t * csection = NULL, *val = NULL, *val_tmp = NULL;
    /* len of @cachedb + / + subsect + EOS */
    char cachesection[sizeof(NDRX_CONF_SECTION_CACHEDB)+1+NDRX_CCTAG_MAX*2+1+1];
    char *p; 
    char *saveptr1 = NULL;
    EDB_txn *txn = NULL;
    unsigned int dbi_flags;
    int any_config = EXFALSE;
    char dbnametmp[NDRX_CCTAG_MAX+1];

    if (NULL!=(db = ndrx_cache_dbget(cachedb)))
    {
#ifdef NDRX_TPCACHE_DEBUG
        NDRX_LOG(log_debug, "Cache db [%s] already loaded", cachedb);
        goto out;
#endif        
    }

    NDRX_CALLOC_OUT(db, 1, sizeof(ndrx_tpcache_db_t), ndrx_tpcache_db_t);
        
    /* Lookup the section from configuration */
    NDRX_STRCPY_SAFE(dbnametmp, cachedb);
    
    p=strchr(dbnametmp, NDRX_CACHE_NAMEDBSEP);

    if (NULL!=p)
    {
        NDRX_STRCPY_SAFE(db->cachedb, dbnametmp);

        *p=EXEOS;
        p++;
        /* the first is exact name */
        NDRX_STRCPY_SAFE(db->cachedbnam, dbnametmp);
        /* second is physical name */
        NDRX_STRCPY_SAFE(db->cachedbphy, p);
    }
    else
    {
        NDRX_STRCPY_SAFE(db->cachedbnam, dbnametmp);
        NDRX_STRCPY_SAFE(db->cachedb, dbnametmp);
        NDRX_STRCPY_SAFE(db->cachedbphy, dbnametmp);
    }

    NDRX_LOG(log_debug, "full name: [%s] logical name: [%s] physical name [%s]",
            db->cachedb, db->cachedbnam, db->cachedbphy);
    
    /* Add support for sub-sections for the key groups
     * the group is first sub-section and actual keyitem or keygroup
     * is second level 
     */
    if (0==strcmp(db->cachedb, db->cachedbnam))
    {
        snprintf(cachesection, sizeof(cachesection), "%s/%s",
                NDRX_CONF_SECTION_CACHEDB, cachedb);
    }
    else
    {
        snprintf(cachesection, sizeof(cachesection), "%s/%s/%s",
                NDRX_CONF_SECTION_CACHEDB, db->cachedbphy, db->cachedbnam);
    }
    
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
    
    /* Set max_readers, map_size defaults */
    
    db->max_readers = NDRX_CACHE_MAX_READERS_DFLT;
    db->map_size = NDRX_CACHE_MAP_SIZE_DFLT;
    db->perms = NDRX_CACHE_PERMS_DFLT;
    db->max_dbs = NDRX_CACHE_MAX_DBS_DFLT;
    
    EXHASH_ITER(hh, csection, val, val_tmp)
    {
        
        any_config = EXTRUE;
#ifdef NDRX_TPCACHE_DEBUG
        NDRX_LOG(log_debug, "%s: config: key: [%s] value: [%s]",
                    __func__, val->key, val->val);
#endif
        
        if (0==strcmp(val->key, NDRX_TPCACHE_KWD_CACHEDB))
        {
            if (0!=strcmp(val->val, db->cachedb))
            {
                NDRX_CACHE_ERROR("%s: Invalid cache name [%s] for section [%s] "
                            "should be [%s]!", 
                __func__, val->val, cachesection, db->cachedb);
                EXFAIL_OUT(ret);
            }
        } 
        else if (0==strcmp(val->key, NDRX_TPCACHE_KWD_RESOURCE))
        {
            NDRX_STRCPY_SAFE(db->resource, val->val);
        }
        else if (0==strcmp(val->key, NDRX_TPCACHE_KWD_PERMS))
        {
            char *pend;
            db->perms = strtol(val->val, &pend, 0);
        }
        /* Also float: Parse 1000, 1K, 1M, 1G */
        else if (0==strcmp(val->key, NDRX_TPCACHE_KWD_LIMIT))
        {
            db->limit = (long)ndrx_num_dec_parsecfg(val->val);
        }
        else if (0==strcmp(val->key, NDRX_TPCACHE_KWD_EXPIRY))
        {
            db->expiry = (long)ndrx_num_time_parsecfg(val->val) / 1000L;
            
            /* we run seconds basis, thus divide by 1000 */
            
            if (0>=db->expiry)
            {
                NDRX_CACHE_ERROR("Invalid expiry specified for db [%s], "
                        "the minimums is 1 second ", cachedb);
                EXFAIL_OUT(ret);
            }
            
            db->flags|=NDRX_TPCACHE_FLAGS_EXPIRY;
        }
        else if (0==strcmp(val->key, NDRX_TPCACHE_KWD_FLAGS))
        {
            /* Decode flags... */
            p = strtok_r (val->val, ",", &saveptr1);
            while (p != NULL)
            {
                /* strip off the string... */
                ndrx_str_strip(p, "\t ");
                
                /* bootreset,lru,hits,fifo */
                
                if (0==strcmp(p, NDRX_TPCACHE_KWD_BOOTRST))
                {
                    db->flags|=NDRX_TPCACHE_FLAGS_BOOTRST;
                }
                else if (0==strcmp(p, NDRX_TPCACHE_KWD_LRU))
                {
                    db->flags|=NDRX_TPCACHE_FLAGS_LRU;
                }
                else if (0==strcmp(p, NDRX_TPCACHE_KWD_HITS))
                {
                    db->flags|=NDRX_TPCACHE_FLAGS_HITS;
                }
                else if (0==strcmp(p, NDRX_TPCACHE_KWD_FIFO))
                {
                    db->flags|=NDRX_TPCACHE_FLAGS_FIFO;
                }
                else if (0==strcmp(p, NDRX_TPCACHE_KWD_BCASTPUT))
                {
                    db->flags|=NDRX_TPCACHE_FLAGS_BCASTPUT;
                }
                else if (0==strcmp(p, NDRX_TPCACHE_KWD_BCASTDEL))
                {
                    db->flags|=NDRX_TPCACHE_FLAGS_BCASTDEL;
                }
                else if (0==strcmp(p, NDRX_TPCACHE_KWD_TIMESYNC))
                {
                    db->flags|=NDRX_TPCACHE_FLAGS_TIMESYNC;
                }
                else if (0==strcmp(p, NDRX_TPCACHE_KWD_SCANDUP))
                {
                    db->flags|=NDRX_TPCACHE_FLAGS_SCANDUP;
                }
                else if (0==strcmp(p, NDRX_TPCACHE_KWD_CLRNOSVC))
                {
                    db->flags|=NDRX_TPCACHE_FLAGS_CLRNOSVC;
                }
                else if (0==strcmp(p, NDRX_TPCACHE_KWD_KEYITEMS))
                {
                    db->flags|=NDRX_TPCACHE_FLAGS_KEYITEMS;
                }
                else if (0==strcmp(p, NDRX_TPCACHE_KWD_KEYGRP))
                {
                    db->flags|=NDRX_TPCACHE_FLAGS_KEYGRP;
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
        else if (0==strcmp(val->key, NDRX_TPCACHE_KWD_MAX_READERS))
        {
            db->max_readers = (long)ndrx_num_dec_parsecfg(val->val);
        }
        else if (0==strcmp(val->key, NDRX_TPCACHE_KWD_MAX_DBS))
        {
            db->max_dbs = (long)ndrx_num_dec_parsecfg(val->val);
        }
        /* Parse float: 1000.5, 1.2K, 1M, 1G */
        else if (0==strcmp(val->key, NDRX_TPCACHE_KWD_MAP_SIZE))
        {
            db->map_size = (long)ndrx_num_dec_parsecfg(val->val);
        }
        else if (0==strcmp(val->key, NDRX_TPCACHE_KWD_SUBSCR))
        {
            NDRX_STRCPY_SAFE(db->subscr, val->val);
        }
        else
        {
            NDRX_LOG(log_warn, "Ignoring unknown cache configuration param: [%s]", 
                    val->key);
            userlog("Ignoring unknown cache configuration param: [%s]", 
                    val->key);
        }
    }
    
    if (!any_config)
    {
        NDRX_CACHE_ERROR("Cache db [%s] not found in configuration", cachedb);
        EXFAIL_OUT(ret);
    }
    
    /* check mandatory attributes */
    
    if (EXEOS==db->cachedb[0])
    {
        NDRX_CACHE_ERROR("Missing `%s' parameter for cache [%s]", 
                NDRX_TPCACHE_KWD_CACHEDB, cachedb);
        EXFAIL_OUT(ret);
    }
    
    if (EXEOS==db->resource[0])
    {
        NDRX_CACHE_ERROR("Missing `%s' parameter for cache [%s]", 
                NDRX_TPCACHE_KWD_RESOURCE, cachedb);
        EXFAIL_OUT(ret);
    }
    
    /* Check invalid flags */

    if (((db->flags & NDRX_TPCACHE_FLAGS_LRU) &&
                       ((db->flags & NDRX_TPCACHE_FLAGS_HITS) ||
                       (db->flags & NDRX_TPCACHE_FLAGS_FIFO)))
        ||
            
        ((db->flags & NDRX_TPCACHE_FLAGS_HITS) &&
                       ((db->flags & NDRX_TPCACHE_FLAGS_LRU) ||
                       (db->flags & NDRX_TPCACHE_FLAGS_FIFO)))
        ||
        ((db->flags & NDRX_TPCACHE_FLAGS_FIFO) &&
                       ((db->flags & NDRX_TPCACHE_FLAGS_LRU) ||
                       (db->flags & NDRX_TPCACHE_FLAGS_HITS)))
            
            )
    {
        
        NDRX_LOG(log_error, "lru = %d", (db->flags & NDRX_TPCACHE_FLAGS_LRU));
        NDRX_LOG(log_error, "hits = %d", (db->flags & NDRX_TPCACHE_FLAGS_HITS));
        NDRX_LOG(log_error, "fifo = %d", (db->flags & NDRX_TPCACHE_FLAGS_FIFO));
        
        NDRX_CACHE_ERROR("For cache db [%s] flags lru,hits and fifo cannot be mixed!", 
                cachedb);
        EXFAIL_OUT(ret);
    }
    
    /* Dump the DB config and open it and if we run in boot mode  
     * we have to reset the 
     */
#ifdef NDRX_TPCACHE_DEBUG
    NDRX_TPCACHEDB_DUMPCFG(log_info, db);
#endif
    
    NDRX_LOG(log_debug, "cachedb: [%s] boot mode: %d  flags: %ld reset: %d",
            db->cachedb, mode, db->flags, (int)(db->flags & NDRX_TPCACHE_FLAGS_BOOTRST));
    
    if (NDRX_TPCACH_INIT_BOOT==mode &&
            db->flags & NDRX_TPCACHE_FLAGS_BOOTRST)
    {
        char data_file[PATH_MAX+1];
        char lock_file[PATH_MAX+1];
        
        snprintf(data_file, sizeof(data_file), "%s/data.edb", db->resource);
        snprintf(lock_file, sizeof(data_file), "%s/lock.edb", db->resource);
        
        NDRX_LOG(log_info, "Resetting cache db [%s], data file: [%s], lock file: [%s]", 
                db->cachedb, data_file, lock_file);
        
        if (EXSUCCEED!=unlink(data_file))
        {
            int err = errno;
            NDRX_LOG(log_error, "unlink [%s] failed: %s", data_file, strerror(err));
            if (ENOENT!=err)
            {
                NDRX_CACHE_TPERROR(TPESYSTEM, 
                    "Failed to unlink: [%s]", strerror(err));
            }
        }
        
        if (EXSUCCEED!=unlink(lock_file))
        {
            int err = errno;
            NDRX_LOG(log_error, "unlink [%s] failed: %s", lock_file, strerror(err));
            if (ENOENT!=err)
            {
                NDRX_CACHE_TPERROR(TPESYSTEM, 
                    "Failed to unlink: [%s]", strerror(err));
            }
        }
        
        #if 0
        if (EXSUCCEED!=(ret=edb_drop(txn, db->dbi, 0)))
        {
            NDRX_CACHE_TPERROR(ndrx_cache_maperr(ret), 
                    "CACHE: Failed to clear db: [%s]: %s", 
                    db->cachedb, edb_strerror(ret));

            EXFAIL_OUT(ret);
        }
        #endif
    }
    
    
    if (EXSUCCEED!=ndrx_cache_phydb_getref(db))
    {
        NDRX_CACHE_ERROR("Failed to load physical db for [%s]/[%s]",
                db->cachedbphy, db->cachedb);
        EXFAIL_OUT(ret);
    }
    
    /* Prepare the DB */
    if (EXSUCCEED!=(ret=edb_txn_begin(db->phy->env, NULL, 0, &txn)))
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
        dbi_flags = EDB_DUPSORT;
    }
    else
    {
        dbi_flags = 0;
    }
    
    if (EXSUCCEED!=(ret=edb_dbi_open(txn, db->cachedbnam, dbi_flags|EDB_CREATE, &db->dbi)))
    {
        NDRX_CACHE_TPERROR(ndrx_cache_maperr(ret), 
                "Failed to open named db for [%s]: %s", 
                db->cachedb, edb_strerror(ret));
        
        EXFAIL_OUT(ret);
    }
    
    if (db->flags & NDRX_TPCACHE_FLAGS_TIMESYNC)
    {
        if (EXSUCCEED!=(ret=edb_set_dupsort(txn, db->dbi, sort_data_bydate)))
        {
            NDRX_CACHE_TPERROR(ndrx_cache_maperr(ret), 
                "Failed to begin transaction for [%s]: %s", 
                db->cachedb, edb_strerror(ret));
        
            goto out;
        }
    }
    
    NDRX_LOG(log_debug, "boot mode: %d  flags: %ld reset: %d",
            mode, db->flags, (int)(db->flags & NDRX_TPCACHE_FLAGS_BOOTRST));
    
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
    
    NDRX_LOG(log_debug, "Cache [%s] path: [%s] is open!", 
            db->resource, db->cachedb);
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
 * Return tpcall cache by service and index!
 * 
 * WARNING!!! If doing changes in idexes, then data will become invalid and shall
 * be dropped!!!
 * 
 * @param svcnm service name to search for 
 * @param idx cache index
 * @return NULL (not found) or ptr to tpcall cache.
 */
expublic ndrx_tpcallcache_t* ndrx_cache_findtpcall_byidx(char *svcnm, int idx)
{
    ndrx_tpcache_svc_t *svcc;
    ndrx_tpcallcache_t* el;
    int ret = EXSUCCEED;
    int i=0;
    
    EXHASH_FIND_STR(ndrx_G_tpcache_svc, svcnm, svcc);
    
    if (NULL==svcc)
    {
        NDRX_LOG(log_debug, "No cache defined for service [%s]", svcnm);
        return NULL;
    }
    
    DL_FOREACH(svcc->caches, el)
    {
        if (i==idx)
        {
            return el;
        }
        
        i++;
    }
    
out:
    return NULL;
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
    int i = -1;
    
    DL_FOREACH(svcc->caches, el)
    {
        i++;
        if (el->buf_type->type_id == buf_type->type_id)
        {
            if (i==idx)
            {
                return el;
            }
            /* in case if index used, no rule testing! */
            else if ( idx > EXFAIL)
            {
                continue;
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
    }
    
    return NULL;
}

/**
 * Close any open caches
 * @return 
 */
expublic void ndrx_cache_uninit(void)
{
    
    ndrx_cache_svcs_free();
    ndrx_cache_dbs_free();
    
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
        if (NULL!=root_value)
        {
            exjson_value_free(root_value);
        }
        
        root_value = exjson_parse_string_with_comments(val->val);
        
        NDRX_LOG(log_debug, "exjson_parse_string_with_comments %p", root_value);
                
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
                
            cache->idx = i;
            NDRX_STRCPY_SAFE(cache->svcnm, cachesvc->svcnm);
            /* process flags.. by strtok.. but we need a temp buffer
             * Process flags first as some logic depends on them!
             */
            if (NULL!=(tmp = exjson_object_get_string(array_object, 
                    NDRX_TPCACHE_KWC_FLAGS)))
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
                    if (0==strcmp(p_flags, NDRX_TPCACHE_KWC_DELREG))
                    {
                        cache->flags|=NDRX_TPCACHE_TPCF_DELREG;
                    }
                    else if (0==strcmp(p_flags, NDRX_TPCACHE_KWC_DELFULL))
                    {
                        cache->flags|=NDRX_TPCACHE_TPCF_DELFULL;
                    }
                    else if (0==strcmp(p_flags, NDRX_TPCACHE_KWC_SAVEREG))
                    {
                        cache->flags|=NDRX_TPCACHE_TPCF_SAVEREG;
                    }
                    else if (0==strcmp(p_flags, NDRX_TPCACHE_KWC_REPL)) /* default */
                    {
                        cache->flags|=NDRX_TPCACHE_TPCF_REPL;
                    }
                    else if (0==strcmp(p_flags, NDRX_TPCACHE_KWC_MERGE))
                    {
                        cache->flags|=NDRX_TPCACHE_TPCF_MERGE;
                    }
                    else if (0==strcmp(p_flags, NDRX_TPCACHE_KWC_SAVEFULL))
                    {
                        cache->flags|=NDRX_TPCACHE_TPCF_SAVEFULL;
                    }
                    else if (0==strcmp(p_flags, NDRX_TPCACHE_KWC_INVAL))
                    {
                        cache->flags|=NDRX_TPCACHE_TPCF_INVAL;
                    }
                    else if (0==strcmp(p_flags, NDRX_TPCACHE_KWC_NEXT))
                    {
                        cache->flags|=NDRX_TPCACHE_TPCF_NEXT;
                    }
                    else if (0==strcmp(p_flags, NDRX_TPCACHE_KWC_INVLKEYGRP))
                    {
                        cache->flags|=NDRX_TPCACHE_TPCF_INVLKEYGRP;
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
                        "flags `%s' and `%s' "
                        "for service [%s], buffer index: %d", 
                        NDRX_TPCACHE_KWC_REPL, NDRX_TPCACHE_KWC_MERGE, svc, i);
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
                        "flags `%s' and `%s' "
                        "for service [%s], buffer index: %d", 
                        NDRX_TPCACHE_KWC_SAVEREG, NDRX_TPCACHE_KWC_SAVEFULL,
                        svc, i);
                EXFAIL_OUT(ret);
            }
            
            if ((cache->flags & NDRX_TPCACHE_TPCF_NEXT) && 
                    !(cache->flags & NDRX_TPCACHE_TPCF_INVAL))
            {
                NDRX_CACHE_TPERROR(TPEINVAL, "CACHE: invalid config - conflicting "
                        "flags `%s' can be used only with `%s' "
                        "for service [%s], buffer index: %d", 
                        NDRX_TPCACHE_KWC_NEXT, NDRX_TPCACHE_KWC_INVAL, svc, i);
                EXFAIL_OUT(ret);
            }
            
            /* get buffer type */
            if (NULL==(tmp = exjson_object_get_string(array_object, 
                    NDRX_TPCACHE_KWC_TYPE)))
            {
                NDRX_CACHE_TPERROR(TPEINVAL, "CACHE: invalid config missing "
                        "[type] for service [%s], buffer index: %d", svc, i);
                EXFAIL_OUT(ret);
            }
            
            NDRX_STRCPY_SAFE(cache->str_buf_type, tmp);
            
             if (NULL!=(tmp = exjson_object_get_string(array_object, 
                     NDRX_TPCACHE_KWC_SUBTYPE)))
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

            if (NULL==(tmp = exjson_object_get_string(array_object, 
                    NDRX_TPCACHE_KWC_CACHEDB)))
            {
                NDRX_CACHE_TPERROR(TPEINVAL, "CACHE: invalid config missing "
                        "[%s] for service [%s], buffer index: %d", 
                        NDRX_TPCACHE_KWC_CACHEDB, svc, i);
                EXFAIL_OUT(ret);
            }

            NDRX_STRCPY_SAFE(cache->cachedbnm, tmp);

            /* split up */

            /* Resolve the DB */
            if (NULL==(cache->cachedb=ndrx_cache_dbresolve(cache->cachedbnm, mode)))
            {
                NDRX_LOG(log_error, "%s failed", __func__);
                EXFAIL_OUT(ret);
            }
                
            if (cache->flags & NDRX_TPCACHE_TPCF_INVAL)
            {
                ndrx_tpcache_svc_t *svcc;
                /* 
                 * So we are NDRX_TPCACHE_TPCF_INVAL resolve the other cache..
                 * And lookup other keys too of inval cache
                 */
                
                /* Get data for invalidating their cache */
                
                if (NULL==(tmp = exjson_object_get_string(array_object, 
                        NDRX_TPCACHE_KWC_INVAL_SVC)))
                {
                    NDRX_CACHE_TPERROR(TPEINVAL, "CACHE: invalid config missing "
                            "[%s] for service [%s], buffer index: %d", 
                            NDRX_TPCACHE_KWC_INVAL_SVC, svc, i);
                    EXFAIL_OUT(ret);
                }

                NDRX_STRCPY_SAFE(cache->inval_svc, tmp);
                
                
                if (NULL==(tmp = exjson_object_get_string(array_object, 
                        NDRX_TPCACHE_KWC_INVAL_IDX)))
                {
                    NDRX_CACHE_TPERROR(TPEINVAL, "CACHE: invalid config missing "
                            "[%s] for service [%s], buffer index: %d", 
                            NDRX_TPCACHE_KWC_INVAL_IDX, svc, i);
                    EXFAIL_OUT(ret);
                }
                
                cache->inval_idx = atoi(tmp);
                
                NDRX_LOG(log_debug, "Searching for %s: [%s]/%d", 
                        NDRX_TPCACHE_KWC_INVAL_SVC,
                        cache->inval_svc, cache->inval_idx);
                
                /* Find service in cache */
                EXHASH_FIND_STR(ndrx_G_tpcache_svc, cache->inval_svc, svcc);

                if (NULL==svcc)
                {
                    NDRX_CACHE_TPERROR(TPEINVAL, "CACHE: %s [%s] not found, "
                            "or defined later (at this or another config file)",
                            NDRX_TPCACHE_KWC_INVAL_SVC, cache->inval_svc);
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
                
                NDRX_LOG(log_debug, "Service found: %p", cache->inval_cache);
            }
            
            /* get db if key group is used */
            
            if (NULL!=(tmp = exjson_object_get_string(array_object, 
                    NDRX_TPCACHE_KWC_KEYGRPDB)))
            {
                /* Resolve the DB */
                if (NULL==(cache->keygrpdb=ndrx_cache_dbresolve((char *)tmp, mode)))
                {
                    NDRX_LOG(log_error, "%s failed", __func__);
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
            if (NULL==(tmp = exjson_object_get_string(array_object, 
                    NDRX_TPCACHE_KWC_KEYFMT)))
            {
                NDRX_CACHE_TPERROR(TPEINVAL, "CACHE: invalid config missing "
                        "[%s] for service [%s], buffer index: %d", 
                        NDRX_TPCACHE_KWC_KEYFMT, svc, i);
                EXFAIL_OUT(ret);
            }
            
            NDRX_STRCPY_SAFE(cache->keyfmt, tmp);
            
            
            if (NULL!=(tmp = exjson_object_get_string(array_object, 
                    NDRX_TPCACHE_KWC_KEYGRPFMT)))
            {
                NDRX_STRCPY_SAFE(cache->keygrpfmt, tmp);
            }
            
            if (NULL!=(tmp = exjson_object_get_string(array_object, 
                                        NDRX_TPCACHE_KWC_KEYGRPMAXTPERRNO)))
            {
                cache->keygroupmtperrno = atoi(tmp);
                
                if (cache->keygroupmtperrno<TPMINVAL || 
                        cache->keygroupmtperrno > TPMAXVAL)
                {
                    NDRX_LOG(log_error, "Invalid max keygroup reject tperrno code [%s] %d "
                            "(min: %d, max: %d)", NDRX_TPCACHE_KWC_KEYGRPMAXTPERRNO, 
                            cache->keygroupmtperrno, TPMINVAL, TPMAXVAL)
                    EXFAIL_OUT(ret);
                }
            }
            
            if (NULL!=(tmp = exjson_object_get_string(array_object, 
                                        NDRX_TPCACHE_KWC_KEYGRPMAXTPURCODE)))
            {
                 cache->keygroupmtpurcode = atol(tmp);
            }
            
            /* Rule to be true to save to cache */
            if (NULL!=(tmp = exjson_object_get_string(array_object, 
                    NDRX_TPCACHE_KWC_RULE)))
            {
                NDRX_STRCPY_SAFE(cache->rule, tmp);
            }
            else
            {
                NDRX_LOG(log_info, "[%s] is missing - assume cache always",
                        NDRX_TPCACHE_KWC_RULE);
            }
            
            if (NULL!=(tmp = exjson_object_get_string(array_object, 
                    NDRX_TPCACHE_KWC_REFRESHRULE)))
            {
                NDRX_STRCPY_SAFE(cache->refreshrule, tmp);
            }
            
            /* get fields to save */
            
            if (!(cache->flags & NDRX_TPCACHE_TPCF_SAVEFULL) &&
                    !(cache->flags & NDRX_TPCACHE_TPCF_INVAL)
                    )
            {
                if (NULL!=(tmp = exjson_object_get_string(array_object, 
                        NDRX_TPCACHE_KWC_SAVE)))
                {

                    NDRX_STRCPY_SAFE(cache->saveproj.expression, tmp);

                    /* if it is regex, then compile */
                    if (cache->flags & NDRX_TPCACHE_TPCF_SAVEREG)
                    {
                        if (EXSUCCEED!=ndrx_regcomp(&cache->saveproj.regex, 
                                cache->saveproj.expression))
                        {
                            NDRX_CACHE_TPERROR(TPEINVAL, "CACHE: failed to compile [%s] "
                                "regex [%s] for svc [%s], "
                                "buffer index: %d - see ndrx logs", 
                                    NDRX_TPCACHE_KWC_SAVE, 
                                    cache->saveproj.expression, svc, i);
                        }
                        cache->saveproj.regex_compiled=EXTRUE;
                    }
                }
                else
                {
                    NDRX_LOG(log_info, "No save strategy specified - default "
                            "to full save");
                    cache->flags |= NDRX_TPCACHE_TPCF_SAVEFULL;
                }
            }
            
            /* process delete fields 
             * RFU: Currently not used. Full buffer is used for broadcasting
             * invalidate data.
             */
            if (!(cache->flags & NDRX_TPCACHE_TPCF_DELFULL))
            {
                if (NULL!=(tmp = exjson_object_get_string(array_object, 
                        NDRX_TPCACHE_KWC_DELETE)))
                {                    
                    NDRX_STRCPY_SAFE(cache->delproj.expression, tmp);

                    /* if it is regex, then compile */
                    if (cache->flags & NDRX_TPCACHE_TPCF_DELREG)
                    {
                        if (EXSUCCEED!=ndrx_regcomp(&cache->delproj.regex, 
                                cache->delproj.expression))
                        {
                            NDRX_CACHE_TPERROR(TPEINVAL, "CACHE: failed to "
                                "compile [%s] regex [%s] for svc [%s], "
                                "buffer index: %d - see ndrx logs", 
                                        NDRX_TPCACHE_KWC_DELETE, 
                                    cache->delproj.expression, svc, i);
                        }
                        cache->delproj.regex_compiled=EXTRUE;
                    }
                }
            }
            /* Process the reject expression by flags func */
            
            if (NULL!=(tmp = exjson_object_get_string(array_object, 
                    NDRX_TPCACHE_KWC_KEYGROUPMREJ)))
            {
                cache->keygroupmrej = NDRX_STRDUP(tmp);
                
                if (NULL==cache->keygroupmrej)
                {
                    int err = errno;
                    NDRX_CACHE_TPERROR(TPEINVAL, "CACHE: failed to allocate [%s] buf"
                            "for svc [%s] buffer index: %d", 
                                NDRX_TPCACHE_KWC_KEYGROUPMREJ,
                            svc, i, strerror(err));
                    EXFAIL_OUT(ret);
                }
            }
            
            /* set keygroup limit - keygroupmax */
            if (NULL!=(tmp = exjson_object_get_string(array_object, 
                    NDRX_TPCACHE_KWC_KEYGROUPMAX)))
            {
                cache->keygroupmax = atol(tmp);
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
            if (NULL!=(tmp = exjson_object_get_string(array_object, 
                    NDRX_TPCACHE_KWC_RSPRULE)))
            {
                NDRX_STRCPY_SAFE(cache->rsprule, tmp);
                
                /* Compile the boolean expression! */
                if (NULL==(cache->rsprule_tree=Bboolco (cache->rsprule)))
                {
                    NDRX_CACHE_TPERROR(TPEINVAL, "CACHE: failed to "
                            "compile [%s] [%s] "
                            "for service [%s], buffer index: %d: %s", 
                            NDRX_TPCACHE_KWC_RSPRULE, 
                            cache->rsprule, svc, i, Bstrerror(Berror));
                    EXFAIL_OUT(ret);
                }
            }
            
            /* Validate key group storage, if used */
            
            if (EXEOS!=cache->keygrpfmt[0] &&
                    !(cache->flags & NDRX_TPCACHE_TPCF_INVAL))
            {
                /* this cache is part of key items... */
                cache->flags|=NDRX_TPCACHE_TPCF_KEYITEMS;
                
                /* The database must be defined */
                
                if (NULL==cache->keygrpdb)
                {
                    NDRX_CACHE_TPERROR(TPEINVAL, "CACHE: [%s] must be "
                            "defined if [%s] is used!",
                            NDRX_TPCACHE_KWC_KEYGRPDB, NDRX_TPCACHE_KWC_KEYGRPFMT);
                    EXFAIL_OUT(ret);
                }
                
                /* database must be with keygrp flag */
                
                if (!(cache->keygrpdb->flags & NDRX_TPCACHE_FLAGS_KEYGRP))
                {
                    NDRX_CACHE_TPERROR(TPEINVAL, "CACHE: [%s] flag must be "
                            "defined for [%s] database to use it for keyitems for"
                            " service [%s] buffer index %d",
                            NDRX_TPCACHE_KWC_KEYGRPDB, 
                            cache->keygrpdb->cachedb, svc, i);
                    EXFAIL_OUT(ret);
                }
                
                /* and linked database must be with keyitems flag */
                
                if (!(cache->cachedb->flags & NDRX_TPCACHE_FLAGS_KEYITEMS))
                {
                    NDRX_CACHE_TPERROR(TPEINVAL, "CACHE: [%s] flag must be "
                            "defined for [%s] database to use it for keyitems for",
                            " service [%s] buffer index %d",
                            NDRX_TPCACHE_KWD_KEYITEMS, 
                            cache->cachedb->cachedb, svc, i);
                    EXFAIL_OUT(ret);
                }
                
                if (NULL!=cache->keygroupmrej && cache->keygroupmax<=0)
                {
                    NDRX_CACHE_TPERROR(TPEINVAL, "CACHE: [%s] defined, but "
                            "[%s] is 0 - invalid config for"
                            " service [%s] buffer index %d",
                            NDRX_TPCACHE_KWC_KEYGROUPMREJ, 
                            NDRX_TPCACHE_KWC_KEYGROUPMAX, svc, i);
                    EXFAIL_OUT(ret);
                }
                
                if (NULL==cache->keygroupmrej && cache->keygroupmax>0)
                {
                    NDRX_CACHE_TPERROR(TPEINVAL, "CACHE: [%s] defined, but "
                            "[%s] not defined - invalid config for"
                            " service [%s] buffer index %d",
                            NDRX_TPCACHE_KWC_KEYGROUPMAX, 
                            NDRX_TPCACHE_KWC_KEYGROUPMREJ, svc, i);
                    EXFAIL_OUT(ret);
                }
                
                
                if (0!=strcmp(cache->cachedb->cachedbphy, cache->keygrpdb->cachedbphy))
                {
                    NDRX_CACHE_TPERROR(TPEINVAL, "CACHE: [%s] for when using keygroup, "
                            "the [%s] and [%s] must come from same physical db name! "
                            "(<name>@<physicalname>) ([%s] != [%s])",
                            NDRX_TPCACHE_KWC_CACHEDB, NDRX_TPCACHE_KWC_KEYGRPDB,
                            cache->cachedb->cachedbphy, cache->keygrpdb->cachedbphy);
                    EXFAIL_OUT(ret);
                }
                
                if (0!=strcmp(cache->cachedb->resource, cache->keygrpdb->resource))
                {
                    NDRX_CACHE_TPERROR(TPEINVAL, "CACHE: [%s] for when using keygroup, "
                            "the [%s] and [%s] must have from same physical db resource! "
                            "(<name>@<physicalname>) ([%s] != [%s])",
                            NDRX_TPCACHE_KWC_CACHEDB, NDRX_TPCACHE_KWC_KEYGRPDB,
                            cache->cachedb->resource, cache->keygrpdb->resource);
                    EXFAIL_OUT(ret);
                }
            }
            else
            {
                if (NULL!=cache->keygroupmrej)
                {
                    NDRX_CACHE_TPERROR(TPEINVAL, "CACHE: [%s] not used as "
                            "group format not defined - service [%s] buffer index %d",
                            NDRX_TPCACHE_KWC_KEYGROUPMREJ, svc, i);
                    EXFAIL_OUT(ret);
                }
                
                if (cache->keygroupmax > 0)
                {
                    NDRX_CACHE_TPERROR(TPEINVAL, "CACHE: [%s] defined, but "
                            "group format not defined - service [%s] buffer index %d",
                            NDRX_TPCACHE_KWC_KEYGROUPMAX, svc, i);
                    EXFAIL_OUT(ret);
                }
            }
            
            /* verify us of keygroup invalidate */
            if (cache->flags & NDRX_TPCACHE_TPCF_INVLKEYGRP &&
                    !(cache->flags & NDRX_TPCACHE_TPCF_INVAL))
            {
                NDRX_CACHE_TPERROR(TPEINVAL, "CACHE: [%s] can be only used with "
                        "[%s] for svc [%s] idx %d",
                        NDRX_TPCACHE_KWC_INVLKEYGRP, NDRX_TPCACHE_KWC_INVAL, 
                            svc, i);
                    EXFAIL_OUT(ret);
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
