/* 
** ATMI level cache - keygroup routines
** Invalidate their cache shall be done by buffer. And not by key. Thus for
** invalidate their, our key is irrelevant.
** Delete shall be performed last on keygrp
** The insert first shall be done in keygrp and then keyitems.
**
** @file atmi_cache_keygrp.c
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
/*---------------------------Prototypes---------------------------------*/

/**
 * Perform lookup in keygroup.
 * If keys counter is overreached and data is not found, then request shall be
 * rejected (basically we respond from cache with reject).
 * 
 * If key is not found in group, then lookup again (even if it might be in the
 * key item db (as these are two database and we might crash in the middle)...
 * 
 * @param cache
 * @param idata
 * @param ilen
 * @param cachekey
 * @return TPFAIL/NDRX_TPCACHE_ENOKEYDATA/NDRX_TPCACHE_ENOTFOUNDLIM - if limit
 * /EDB_NOTFOUND
 * reached.
 */
expublic int ndrx_cache_keygrp_lookup(ndrx_tpcallcache_t *cache, 
            char *idata, long ilen, char **odata, long *olen, char *cachekey,
            long flags)
{
    int ret = EXSUCCEED;
    char key[NDRX_CACHE_KEY_MAX+1];
    char errdet[MAX_TP_ERROR_LEN+1];
    EDB_txn *txn;
    int tran_started = EXFALSE;
    EDB_val cachedata;
    ndrx_tpcache_data_t *exdata;
    typed_buffer_descr_t *buf_type = &G_buf_descr[BUF_TYPE_UBF];
    UBFH *p_ub_keys = NULL;        /* list of keys in keygroup */
    long rsplen;
    Bnext_state_t state1;
    BFLDID bfldid1;
    long numkeys = 0;
    BFLDOCC occ;
    char *dptr;
    BFLDLEN dlen;
    int cachekey_found = EXFALSE;
    int got_dbname = EXFALSE;
    if (EXSUCCEED!=(ret = ndrx_G_tpcache_types[cache->buf_type->type_id].pf_get_key(
                cache, idata, ilen, key, sizeof(key), errdet, sizeof(errdet))))
    {
        if (NDRX_TPCACHE_ENOKEYDATA==ret)
        {
            NDRX_LOG(log_debug, "Failed to build key (no data for key): %s", errdet);
            goto out;
        }
        else
        {
            NDRX_CACHE_TPERRORNOU(TPESYSTEM, "%s: Failed to build cache key: %s", 
                    __func__, errdet);
            goto out;
        }
    }
    
    NDRX_LOG(log_debug, "Key group key [%s]", key);
    
    
    if (EXSUCCEED!=(ret=ndrx_cache_edb_begin(cache->keygrpdb, &txn, EDB_RDONLY)))
    {
        NDRX_LOG(log_error, "%s: failed to start tran", __func__);
        goto out;
    }
    
    tran_started = EXTRUE;
    
    
    if (EXSUCCEED!=(ret=ndrx_cache_edb_get(cache->keygrpdb, txn, key, &cachedata,
            EXFALSE)))
    {
        /* error already provided by wrapper */
        NDRX_LOG(log_debug, "%s: failed to get cache by [%s]", __func__, key);
        goto out;
    }
    
    /* Check the record validity */
    exdata = (ndrx_tpcache_data_t *)cachedata.mv_data;
    NDRX_CACHE_CHECK_DBDATA((&cachedata), exdata, key, TPESYSTEM);
    
    
    /* Receive data as UBF buffer, so that we can test it... 
     * this is just list 
     */
    
    if (EXSUCCEED!=buf_type->pf_prepare_incoming(buf_type, exdata->atmi_buf, 
                exdata->atmi_buf_len, (char **)&p_ub_keys, &rsplen, 0))
    {
        /* the error shall be set already */
        NDRX_LOG(log_error, "Failed to read keygroup record for [%s]", key);
        EXFAIL_OUT(ret);
    }
    
    /* iterate the buffer to find they key, and check the total count of the
     * keys if limit is defined
     */
    
    bfldid1 = BFIRSTFLDID;
    
    while (1)
    {
        ret=ndrx_Bnext(&state1, p_ub_keys, &bfldid1, &occ, NULL, &dlen, &dptr);
        
        if (0==ret)
        {
            /* we are at EOF */
            break;
        }
        else if (0 > ret)
        {
            /* we got an error while scanning key storage */
            NDRX_CACHE_TPERROR(TPESYSTEM, "%s: Failed to iterate key group items: %s", 
                    __func__, Bstrerror(Berror));
            EXFAIL_OUT(ret)
        }

        switch (bfldid1)
        {
            /* in which db keys are stored? 
             * DB name will be always first as the field id is less than OPEXPR
             * and 
             */
            case EX_CACHE_DBNAME:
                
                got_dbname = EXTRUE;
                if (0!=strcmp(dptr, cache->cachedbnm))
                {
                    NDRX_CACHE_TPERROR(TPESYSTEM, "%s: consistency error, expected "
                            "db [%s] but got [%s] "
                            "for group record of cache item key [%s], groupkey [%s]",
                            __func__, cache->cachedbnm, dptr, cachekey, 
                            key);
                    EXFAIL_OUT(ret)
                }
                
                break;
            
            /* keys ops */
            case EX_CACHE_OPEXPR:
                
                numkeys++;
                
                if (0==strcmp(dptr, cachekey))
                {
                    cachekey_found=EXTRUE;
                    NDRX_LOG(log_debug, "Key found in group");
                    break;
                }
                
                break;
                
            default:
                /* raise error as key is not supported */
                NDRX_CACHE_TPERROR(TPESYSTEM, "%s: Invalid field [%s][%d] in "
                        "keygroup [%s] db [%s]",
                        __func__, Bfname(bfldid1), bfldid1, 
                        cachekey, cache->keygrpdb->cachedb);
                EXFAIL_OUT(ret)
                break;
        }

    }
    
    if (!got_dbname)
    {
        NDRX_CACHE_TPERROR(TPESYSTEM, "%s: Invalid data saved in "
                        "keygroup [%s] db [%s] - missing EX_CACHE_DBNAME!",
                        __func__,  cachekey, cache->keygrpdb->cachedb);
        EXFAIL_OUT(ret)
    }
    
    if (!cachekey_found && cache->keygroupmax > 0 )
    {
        /* Check the key count and see if reject is needed...? */
        
        if (numkeys > cache->keygroupmax)
        {
            NDRX_LOG(log_error, "Number keys in group [%ld] max allowed in group [%ld]"
                    " - reject",
                    numkeys, cache->keygroupmax);
            
            ret = NDRX_TPCACHE_ENOTFOUNDLIM;

            /* generate response...  this shall be done via type selector */
            if (EXSUCCEED!=ndrx_G_tpcache_types[cache->buf_type->type_id].pf_cache_maxreject(
                    cache, idata, ilen, odata, olen, flags))
            {
                NDRX_LOG(log_error, "%s: Failed to reject user buffer!", __func__);
                EXFAIL_OUT(ret);
            }
        }
    }
    
    if (!cachekey_found)
    {
        NDRX_LOG(log_debug, "Key not found in group");
        ret=NDRX_TPCACHE_ENOCACHEDATA;
    }
    else
    {
        NDRX_LOG(log_debug, "Key found in group");
    }
    
out:

    if (tran_started)
    {
        /* terminate transaction please */
        ndrx_cache_edb_abort(cache->keygrpdb, txn);
    }

    return ret;
}

/**
 * Add update keygroup. Only interesting question is how about duplicate
 * keys? If it is duplicate, then key group does not change. So there shall be
 * no problems.
 */
expublic int ndrx_cache_keygrp_addupd(ndrx_tpcallcache_t *cache, 
            char *idata, long ilen, char *cachekey)
{
    int ret = EXSUCCEED;
    char key[NDRX_CACHE_KEY_MAX+1];
    char errdet[MAX_TP_ERROR_LEN+1];
    EDB_txn *txn;
    int tran_started = EXFALSE;
    EDB_val cachedata;
    ndrx_tpcache_data_t *exdata;
    typed_buffer_descr_t *buf_type = &G_buf_descr[BUF_TYPE_UBF];
    UBFH *p_ub_keys = NULL;        /* list of keys in keygroup */
    long rsplen;
    Bnext_state_t state1;
    BFLDID bfldid1;
    long numkeys = 0;
    BFLDOCC occ;
    char *dptr;
    BFLDLEN dlen;
    int got_dbname = EXFALSE;
    int cachekey_found = EXFALSE;
    char buf[NDRX_MSGSIZEMAX];
    
    if (EXSUCCEED!=(ret = ndrx_G_tpcache_types[cache->buf_type->type_id].pf_get_key(
                cache, idata, ilen, key, sizeof(key), errdet, sizeof(errdet))))
    {
        if (NDRX_TPCACHE_ENOKEYDATA==ret)
        {
            NDRX_LOG(log_debug, "Failed to build key (no data for key): %s", errdet);
            goto out;
        }
        else
        {
            NDRX_CACHE_TPERRORNOU(TPESYSTEM, "%s: Failed to build cache key: %s", 
                    __func__, errdet);
            goto out;
        }
    }
    
    NDRX_LOG(log_debug, "Key group key [%s]", key);
    
    
    if (EXSUCCEED!=(ret=ndrx_cache_edb_begin(cache->keygrpdb, &txn, EDB_RDONLY)))
    {
        NDRX_LOG(log_error, "%s: failed to start tran", __func__);
        goto out;
    }
    
    tran_started = EXTRUE;
    
    if (NULL==(p_ub_keys = (UBFH *)tpalloc("UBF", 0, 1024)))
    {
        NDRX_LOG(log_error, "Failed to allocate UBF buffer: %s", tpstrerror(tperrno));
    }
    
    if (EXSUCCEED!=(ret=ndrx_cache_edb_get(cache->keygrpdb, txn, key, &cachedata,
            EXFALSE)))
    {
        /* error already provided by wrapper */
        if (EDB_NOTFOUND==ret)
        {
            /* prepare buffer where to write off the keys */
            NDRX_LOG(log_debug, "Key group is missing -> must be added");
            
            /* Add db name to buffer (of source cache) */
            if (EXSUCCEED!=Bchg(p_ub_keys, EX_CACHE_DBNAME, 0, cache->cachedbnm, 0L))
            {
                NDRX_CACHE_TPERROR(TPESYSTEM, "%s: Set install `EX_CACHE_DBNAME': %s", 
                    __func__, Bstrerror(Berror));
                EXFAIL_OUT(ret);
            }
        }
        else
        {
            NDRX_LOG(log_debug, "%s: failed to get cache by [%s]", __func__, key);
            goto out;
        }
    }
    else
    {
        /* Check the record validity */
        exdata = (ndrx_tpcache_data_t *)cachedata.mv_data;
        NDRX_CACHE_CHECK_DBDATA((&cachedata), exdata, key, TPESYSTEM);


        /* Receive data as UBF buffer, so that we can test it... 
         * this is just list 
         */

        if (EXSUCCEED!=buf_type->pf_prepare_incoming(buf_type, exdata->atmi_buf, 
                    exdata->atmi_buf_len, (char **)&p_ub_keys, &rsplen, 0))
        {
            /* the error shall be set already */
            NDRX_LOG(log_error, "Failed to read keygroup record for [%s]", key);
            EXFAIL_OUT(ret);
        }

        /* iterate the buffer to find they key, and check the total count of the
         * keys if limit is defined
         */

        bfldid1 = BFIRSTFLDID;

        while (1)
        {
            ret=ndrx_Bnext(&state1, p_ub_keys, &bfldid1, &occ, NULL, &dlen, &dptr);

            if (0==ret)
            {
                /* we are at EOF */
                break;
            }
            else if (0 > ret)
            {
                /* we got an error while scanning key storage */
                NDRX_CACHE_TPERROR(TPESYSTEM, "%s: Failed to iterate key group items: %s", 
                        __func__, Bstrerror(Berror));
                EXFAIL_OUT(ret)
            }

            switch (bfldid1)
            {
                /* in which db keys are stored? 
                 * DB name will be always first as the field id is less than OPEXPR
                 * and 
                 */
                case EX_CACHE_DBNAME:

                    got_dbname = EXTRUE;
                    if (0!=strcmp(dptr, cache->cachedbnm))
                    {
                        NDRX_CACHE_TPERROR(TPESYSTEM, "%s: consistency error, expected "
                                "db [%s] but got [%s] "
                                "for group record of cache item key [%s], groupkey [%s]",
                                __func__, cache->cachedbnm, dptr, cachekey, 
                                key);
                        EXFAIL_OUT(ret)
                    }

                    break;

                /* keys ops */
                case EX_CACHE_OPEXPR:

                    numkeys++;

                    if (0==strcmp(dptr, cachekey))
                    {
                        cachekey_found=EXTRUE;
                        NDRX_LOG(log_debug, "Key found in group");
                        break;
                    }

                    break;

                default:
                    /* raise error as key is not supported */
                    NDRX_CACHE_TPERROR(TPESYSTEM, "%s: Invalid field [%s][%d] in "
                            "keygroup [%s] db [%s]",
                            __func__, Bfname(bfldid1), bfldid1, 
                            cachekey, cache->keygrpdb->cachedb);
                    EXFAIL_OUT(ret)
                    break;
            }

        }

        if (!got_dbname)
        {
            NDRX_CACHE_TPERROR(TPESYSTEM, "%s: Invalid data saved in "
                            "keygroup [%s] db [%s] - missing EX_CACHE_DBNAME!",
                            __func__,  cachekey, cache->keygrpdb->cachedb);
            EXFAIL_OUT(ret)
        }

        if (!cachekey_found)
        {
            NDRX_LOG(log_debug, "Key not found in group");
            ret=NDRX_TPCACHE_ENOCACHEDATA;
            goto out;
        }
        else
        {
            NDRX_LOG(log_debug, "Key found in group");
        }
    
    }
    
    if (!cachekey_found)
    {
        if (NULL==(p_ub_keys = (UBFH *)tprealloc((char *)p_ub_keys, 
                Bsizeof(p_ub_keys) + strlen(cachekey))))
        {
            NDRX_LOG(log_error, "Failed to allocate UBF buffer: %s", 
                    tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
        
        if (EXSUCCEED!=Badd(p_ub_keys, EX_CACHE_OPEXPR, cachekey, 0L))
        {
            NDRX_CACHE_TPERROR(TPESYSTEM, "%s: Failed to add EX_CACHE_OPEXPR to UBF: %s",
                    Bstrerror(Berror));
            EXFAIL_OUT(ret)
        }
        
        /* write record off to DB... */
        
        ndrx_debug_dump_UBF(log_debug, "Saving to keygroup", (UBFH *)p_ub_keys);
    
        
        memset(exdata, 0, sizeof(ndrx_tpcache_data_t));
    
        exdata->magic = NDRX_CACHE_MAGIC;
        NDRX_STRCPY_SAFE(exdata->svcnm, cache->svcnm);
        exdata->nodeid = (short)tpgetnodeid();

        /* get current timestamp */
        ndrx_utc_tstamp2(&exdata->t, &exdata->tusec);
        
        
        exdata->cache_idx = cache->idx;
        exdata->atmi_type_id = buf_type->type_id;
        exdata->atmi_buf_len = NDRX_MSGSIZEMAX - sizeof(ndrx_tpcache_data_t);
    
        if (EXSUCCEED!=buf_type->pf_prepare_outgoing (buf_type, (char *)p_ub_keys, 
                    0, exdata->atmi_buf, &exdata->atmi_buf_len, 0L))
        {
            userlog("Failed to prepare buffer for saving in keygroup: %s", 
                    tpstrerror(tperrno));
            NDRX_LOG(log_error, "Failed to prepare buffer for saving in keygroup");
            EXFAIL_OUT(ret);
        }
        
        cachedata.mv_data = (void *)exdata;
        cachedata.mv_size = exdata->atmi_buf_len + sizeof(ndrx_tpcache_data_t);
        
        if (EXSUCCEED!=(ret=ndrx_cache_edb_put (cache->keygrpdb, txn, 
                key, &cachedata, 0)))
        {
            NDRX_LOG(log_debug, "Failed to put DB for keygroup...!");
            goto out;
        }
    }
    
out:

    if (tran_started)
    {
        /* terminate transaction please */
        if (EXSUCCEED!=ndrx_cache_edb_commit(cache->keygrpdb, txn))
        {
            NDRX_LOG(log_error, "Failed to commit - aborting...!");
            ndrx_cache_edb_abort(cache->keygrpdb, txn);
            ret=EXFAIL;
        }
    }

    return ret;
}

/**
 * Delete keygroup record by data.
 * Can reuse transaction...
 * use this func in ndrx_cache_inval_by_data
 */
expublic int ndrx_cache_keygrp_inval_by_data(char *svc, ndrx_tpcallcache_t *cache, 
        char *key, char *idata, long ilen, EDB_txn **txn)
{
    int ret = EXSUCCEED;
    
out:
            
    return ret;
}

/**
 * Delete by key, group values...
 * use this func in ndrx_cache_inval_by_key()
 */
expublic int ndrx_cache_keygrp_inval_by_key(ndrx_tpcache_db_t* db, char *key, EDB_txn **txn)
{
    int ret = EXSUCCEED;
    
out:
            
    return ret;
}


