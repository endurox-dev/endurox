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
#include <ubf_impl.h>
#include <ubfutil.h>
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
    
    NDRX_LOG(log_debug, "%s enter", __func__);
    
    NDRX_STRCPY_SAFE(key, cache->keygrpfmt);
    
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
    
    while (!cachekey_found)
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
        
        ret=0;

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
    
    NDRX_LOG(log_debug, "cachekey_found=%d, cache->keygroupmax=%ld numkeys=%ld",
            cachekey_found, cache->keygroupmax, numkeys);
    
    if (!cachekey_found && cache->keygroupmax > 0 )
    {
        /* Check the key count and see if reject is needed...? */
        
        if (numkeys >= cache->keygroupmax)
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
            
            goto out;
        }
    }
    
    if (!cachekey_found)
    {
        NDRX_LOG(log_debug, "Key not found in group");
        ret=NDRX_TPCACHE_ENOCACHEDATA;
    }
    else
    {
        NDRX_LOG(log_debug, "Key found in group, ret=%d", ret);
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
 * if single keyitem is deleted in non-inval mode, then keyitem must be removed
 * from the list. Only in inval trigger, we shall invalidate full group...
 * @param deleteop use EXTRUE if performing delete operation from the keygroup
 * @param have_keygrp we have a keygroup, do not use data for this purpose
 * @param cachekey key item key
 */
expublic int ndrx_cache_keygrp_addupd(ndrx_tpcallcache_t *cache, 
        char *idata, long ilen, char *cachekey, char *have_keygrp, int deleteop)
{
    int ret = EXSUCCEED;
    char key[NDRX_CACHE_KEY_MAX+1];
    char errdet[MAX_TP_ERROR_LEN+1];
    EDB_txn *txn;
    int tran_started = EXFALSE;
    EDB_val cachedata;
    ndrx_tpcache_data_t *exdata=NULL;
    typed_buffer_descr_t *buf_type = &G_buf_descr[BUF_TYPE_UBF];
    UBFH *p_ub_keys = NULL;        /* list of keys in keygroup */
    long rsplen;
    Bnext_state_t state1;
    BFLDID bfldid1;
    long numkeys = 0;
    BFLDOCC occ, occ_found=EXFAIL;
    char *dptr;
    BFLDLEN dlen;
    int got_dbname = EXFALSE;
    int cachekey_found = EXFALSE;
    char buf[NDRX_MSGSIZEMAX];
    char *kg_ptr;
    
    
    if (NULL!=have_keygrp)
    {
        kg_ptr=have_keygrp;
    }
    else
    {
        NDRX_STRCPY_SAFE(key, cache->keygrpfmt);

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
        kg_ptr = key;
    }
    
    NDRX_LOG(log_debug, "Key group key [%s]", kg_ptr);
    if (EXSUCCEED!=(ret=ndrx_cache_edb_begin(cache->keygrpdb, &txn, 0)))
    {
        NDRX_LOG(log_error, "%s: failed to start tran", __func__);
        goto out;
    }
    
    tran_started = EXTRUE;
    
    if (NULL==(p_ub_keys = (UBFH *)tpalloc("UBF", 0, 1024)))
    {
        NDRX_LOG(log_error, "Failed to allocate UBF buffer: %s", tpstrerror(tperrno));
    }
    
    if (EXSUCCEED!=(ret=ndrx_cache_edb_get(cache->keygrpdb, txn, kg_ptr, &cachedata,
            EXFALSE)))
    {
        /* error already provided by wrapper */
        if (EDB_NOTFOUND==ret)
        {
            if (deleteop)
            {
                NDRX_LOG(log_debug, "Key group record does not exists - "
                        "assume keyitem deleted ok");
                ret=EXSUCCEED;
                goto out;
            }
            
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
            NDRX_LOG(log_debug, "%s: failed to get cache by [%s]", __func__, kg_ptr);
            goto out;
        }
    }
    else
    {
        /* Check the record validity */
        exdata = (ndrx_tpcache_data_t *)cachedata.mv_data;
        NDRX_CACHE_CHECK_DBDATA((&cachedata), exdata, kg_ptr, TPESYSTEM);


        /* Receive data as UBF buffer, so that we can test it... 
         * this is just list 
         */

        if (EXSUCCEED!=buf_type->pf_prepare_incoming(buf_type, exdata->atmi_buf, 
                    exdata->atmi_buf_len, (char **)&p_ub_keys, &rsplen, 0))
        {
            /* the error shall be set already */
            NDRX_LOG(log_error, "Failed to read keygroup record for [%s]", kg_ptr);
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
                                kg_ptr);
                        EXFAIL_OUT(ret)
                    }

                    break;

                /* keys ops */
                case EX_CACHE_OPEXPR:

                    numkeys++;

                    if (0==strcmp(dptr, cachekey))
                    {
                        cachekey_found=EXTRUE;
                        occ_found = occ;
                        NDRX_LOG(log_debug, "Key found in group at occ [%d]", 
                                occ_found);
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
            if (deleteop)
            {
                NDRX_LOG(log_debug, "Keyitem not found in group - assume deleted ok");
                goto out;
            }
        }
        else
        {
            NDRX_LOG(log_debug, "Key found in group");
            
        }
    }
    
    if (cachekey_found && deleteop || !cachekey_found && !deleteop)
    {
        if (deleteop)
        {
            NDRX_LOG(log_debug, "Removing key from the group");
            
            if (EXSUCCEED!=Bdel(p_ub_keys, EX_CACHE_OPEXPR, occ_found))
            {
                NDRX_CACHE_TPERROR(TPESYSTEM, "%s: Failed to delete "
                        "EX_CACHE_OPEXPR[%d]: %s",
                        occ_found, Bstrerror(Berror));
                EXFAIL_OUT(ret)
            }
        }
        else
        {
            NDRX_LOG(log_debug, "Adding key to the group");
        
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
        }
        /* write record off to DB... */
        
        ndrx_debug_dump_UBF(log_debug, "Saving to keygroup", (UBFH *)p_ub_keys);
    
        if (NULL==exdata)
        {
            exdata = (ndrx_tpcache_data_t *)buf;
            memset(exdata, 0, sizeof(ndrx_tpcache_data_t));
            
            exdata->magic = NDRX_CACHE_MAGIC;
            NDRX_STRCPY_SAFE(exdata->svcnm, cache->svcnm);
            exdata->nodeid = (short)tpgetnodeid();

            /* get current timestamp */
            ndrx_utc_tstamp2(&exdata->t, &exdata->tusec);
            
            exdata->cache_idx = cache->idx;
        }
        else
        {
            /* update existing data.. */
            memcpy(buf, exdata, sizeof(ndrx_tpcache_data_t));
            exdata = (ndrx_tpcache_data_t *)buf;
        }
        
        
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
                kg_ptr, &cachedata, 0)))
        {
            NDRX_LOG(log_debug, "Failed to put DB for keygroup...!");
            goto out;
        }
    }
    
out:

    if (tran_started)
    {
        /* terminate transaction please */
        if (EXSUCCEED==ret)
        {
            if (EXSUCCEED!=ndrx_cache_edb_commit(cache->keygrpdb, txn))
            {
                NDRX_LOG(log_error, "Failed to commit - aborting...!");
                ndrx_cache_edb_abort(cache->keygrpdb, txn);
                ret=EXFAIL;
            }
        }
        else
        {
            ndrx_cache_edb_abort(cache->keygrpdb, txn);
        }
    }

    return ret;
}


/**
 * Delete keygroup keyitems. The group by it self should be removed by outer 
 * caller.
 * @param dbkeygroup This is keygroup database
 * @param p_ub
 * @param keyitem_dbname test dbname against (optional, to test if buffer compares)
 * @return 
 */
exprivate int ndrx_cache_invalgroup(ndrx_tpcache_db_t* dbkeygroup, 
        UBFH *p_ub, char *keyitem_dbname)
{
    int ret = EXSUCCEED;
    Bnext_state_t state1;
    BFLDID bfldid1;
    BFLDOCC occ;
    char *dptr;
    BFLDLEN dlen;
    long numkeys = 0;
    EDB_txn *txn;
    int tran_started = EXFALSE;
    ndrx_tpcache_db_t* db = NULL;
    
    bfldid1 = BFIRSTFLDID;

    while (1)
    {
         ret=ndrx_Bnext(&state1, p_ub, &bfldid1, &occ, NULL, &dlen, &dptr);

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
             case EX_CACHE_DBNAME:
                /* Resolve DB name... */
                NDRX_LOG(log_debug, "Key item DB Lookup: [%s]", dptr);
                
                /* Check DB name if have one... */
                if (NULL!=keyitem_dbname)
                {
                    if (0!=strcmp(keyitem_dbname, dptr))
                    {
                        NDRX_CACHE_TPERRORNOU(TPESYSTEM, "Expected db name of keyitems "
                                "[%s] does not match actual in UBF [%s]",
                                keyitem_dbname, dptr);
                        EXFAIL_OUT(ret);
                    }
                }
                
                if (NULL==(db = ndrx_cache_dbresolve(dptr, NDRX_TPCACH_INIT_NORMAL)))
                {
                    NDRX_CACHE_TPERRORNOU(TPENOENT, "Failed to get db record for [%s]: %s", 
                           dptr, tpstrerror(tperrno));
                    EXFAIL_OUT(ret);
                }
                 
                /* Open transaction here... */
                if (EXSUCCEED!=(ret=ndrx_cache_edb_begin(db, &txn, 0)))
                {
                    NDRX_LOG(log_error, "%s: failed to start tran", __func__);
                    goto out;
                }

                tran_started = EXTRUE;

                break;
                 
             /* keys ops */
             case EX_CACHE_OPEXPR:

                numkeys++;
                
                if (NULL==db)
                {
                    NDRX_CACHE_TPERROR(TPESYSTEM, "Missing EX_CACHE_DBNAME in keygroup!");
                    EXFAIL_OUT(ret);
                }
                
                NDRX_LOG(log_debug, "About to erase: [%s] from [%s] db", 
                        dptr, db->cachedb);
                
                if (EXSUCCEED!=(ret=ndrx_cache_edb_del (db, txn, dptr, NULL)))
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

                break;
         }
     }
    
    
    /* TODO: Delete group by it self */
    
    
out:
                 
    if (tran_started)
    {
        /* terminate transaction please */
        if (EXSUCCEED==ret)
        {
            if (EXSUCCEED!=ndrx_cache_edb_commit(db, txn))
            {
                NDRX_LOG(log_error, "Failed to commit - aborting...!");
                ndrx_cache_edb_abort(db, txn);
                ret=EXFAIL;
            }
        }
        else
        {
            ndrx_cache_edb_abort(db, txn);
        }
    }

    return ret;
}

/**
 * return a keygroup.
 * This will start read only transactions
 * @param db database
 * @param txn transaction in progress
 * @param key key to search for (it is a group key)
 * @param pp_ub UBF buffer with keys
 * @return EXSUCCEED/EXFAIL
 */
exprivate int ndrx_cache_keygrp_getgroup(ndrx_tpcache_db_t* db, EDB_txn *txn, 
        char *key, UBFH **pp_ub)
{
    int ret = EXSUCCEED;
    EDB_val cachedata;
    ndrx_tpcache_data_t *exdata;
    typed_buffer_descr_t *buf_type = &G_buf_descr[BUF_TYPE_UBF];
    long rsplen;
    
    NDRX_LOG(log_debug, "%s: Key group key [%s]", __func__, key);
    
    if (EXSUCCEED!=(ret=ndrx_cache_edb_get(db, txn, key, &cachedata,
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
                exdata->atmi_buf_len, (char **)pp_ub, &rsplen, 0))
    {
        /* the error shall be set already */
        NDRX_LOG(log_error, "Failed to read keygroup record for [%s]", key);
        EXFAIL_OUT(ret);
    }
    
out:
                    
    NDRX_LOG(log_debug, "%s returns %d", __func__, ret);

    return ret;
}

/**
 * Delete by key, group values...
 * use this func in ndrx_cache_inval_by_key()
 * This shall be done only if we delete 
 * 
 * Supported modes:
 * - delete keyitem by data - remove all group
 * - delete keygrp by key - remove full group
 * - all other modes works by single db record.
 * - if we delete from keyitems, then keygroup record shall be updated accordingly... (after delete happended)
 * @param db this is db of keygroup
 * @param txn this is keygroup transaction, data transaction will be made internally
 * @param keyitem_dbname
 */
expublic int ndrx_cache_keygrp_inval_by_key(ndrx_tpcache_db_t* db, 
        char *key, EDB_txn *txn, char *keyitem_dbname)
{
    int ret = EXSUCCEED;
    UBFH *p_ub = NULL;
    int tran_started = EXFALSE;
    
    NDRX_LOG(log_debug, "%s enter", __func__);
    
    if (NULL==txn)
    {
        /* start transaction locally */
        
        if (EXSUCCEED!=(ret=ndrx_cache_edb_begin(db, &txn, 0)))
        {
            NDRX_LOG(log_error, "%s: failed to start tran", __func__);
            goto out;
        }

        tran_started = EXTRUE;
    }
    
    if (EXSUCCEED!=(ret=ndrx_cache_keygrp_getgroup(db, txn, key, &p_ub)))
    {
        NDRX_LOG(log_info, "Failed to get keygroup: %s", tpstrerror(tperrno));
        goto out;
    }
    
    if (EXSUCCEED!=(ret=ndrx_cache_invalgroup(db, p_ub, keyitem_dbname)))
    {
        NDRX_LOG(log_info, "Failed to get keygroup: %s", tpstrerror(tperrno));
        goto out;
    }
    
    /* Remove group record by it self */
    if (EXSUCCEED!=(ret=ndrx_cache_edb_del (db, txn, key, NULL)))
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
    
out:
                 
    if (tran_started)
    {
        /* terminate transaction please */
        if (EXSUCCEED==ret)
        {
            if (EXSUCCEED!=ndrx_cache_edb_commit(db, txn))
            {
                NDRX_LOG(log_error, "Failed to commit - aborting...!");
                ndrx_cache_edb_abort(db, txn);
                ret=EXFAIL;
            }
        }
        else
        {
            ndrx_cache_edb_abort(db, txn);
        }
    }

    if (NULL!=p_ub)
    {
        NDRX_FREE((char *)p_ub);
    }

    NDRX_LOG(log_debug, "%s return %d", __func__, ret);
    return ret;
}

/**
 * Extract key by cache data
 * @param cache cache definition
 * @param exdata 
 * @param keyout
 * @param keyout_bufsz
 * @return 
 */
expublic int ndrx_cache_keygrp_getkey_from_data(ndrx_tpcallcache_t* cache, 
        ndrx_tpcache_data_t *exdata, char *keyout, long keyout_bufsz)
{
    int ret = EXSUCCEED;
    char *buf = NULL;
    long rsplen = 0;
    char errdet[MAX_TP_ERROR_LEN+1];
    
    typed_buffer_descr_t *buf_type = &G_buf_descr[exdata->atmi_type_id];
    
    if (EXSUCCEED!=ndrx_G_tpcache_types[exdata->atmi_type_id].pf_cache_get(
            cache, exdata, buf_type, buf, 0, &buf, &rsplen, 0))
    {
        NDRX_LOG(log_error, "%s: Failed to process ", __func__);
        EXFAIL_OUT(ret);
    }
    
    /* get the key */
    
    NDRX_STRNCPY_SAFE(keyout, cache->keygrpfmt, keyout_bufsz);
    
    if (EXSUCCEED!=(ret = ndrx_G_tpcache_types[cache->buf_type->type_id].pf_get_key(
                cache, buf, rsplen, keyout, keyout_bufsz, errdet, sizeof(errdet))))
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
    
out:

    /* free up the data buffer */
    if (NULL!=buf)
    {
        tpfree(buf);
    }

    return ret;
}

/**
 * Delete keygroup record by data.
 * Can reuse transaction...
 * use this func in ndrx_cache_inval_by_data
 * So if we delete key item, that will key the keygroup fully...
 * We shall return the status that full group is removed 
 * @param txn this is keygroup transaction, data transaction will be made internally
 * @param cache tpcall cache which being invalidated
 */
expublic int ndrx_cache_keygrp_inval_by_data(ndrx_tpcallcache_t *cache, 
        char *idata, long ilen, EDB_txn *txn)
{
    char key[NDRX_CACHE_KEY_MAX+1];
    char errdet[MAX_TP_ERROR_LEN+1];
    int ret = EXSUCCEED;
    int tran_started = EXFALSE;
    
    NDRX_LOG(log_debug, "%s enter", __func__);
    
    
    if (NULL==txn)
    {
        /* start transaction locally */
        
        if (EXSUCCEED!=(ret=ndrx_cache_edb_begin(cache->keygrpdb, &txn, 0)))
        {
            NDRX_LOG(log_error, "%s: failed to start tran", __func__);
            goto out;
        }

        tran_started = EXTRUE;
    }
 
    
    NDRX_STRCPY_SAFE(key, cache->keygrpfmt);
    
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
    
    NDRX_LOG(log_debug, "%s: Key group key built [%s]", __func__, key);
    
    
    if (EXSUCCEED!=(ret = ndrx_cache_keygrp_inval_by_key(cache->keygrpdb, key, 
            txn, cache->cachedbnm)))
    {
        NDRX_LOG(log_error, "Failed to remove key group [%s] of db [%s]",
                key, cache->keygrpdb->cachedb);
        EXFAIL_OUT(ret);
    }
    
    /* Remove group record by it self */
    
    if (EXSUCCEED!=(ret=ndrx_cache_edb_del (cache->keygrpdb, txn, key, NULL)))
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
    
out:
                 
    if (tran_started)
    {
        /* terminate transaction please */
        if (EXSUCCEED==ret)
        {
            if (EXSUCCEED!=ndrx_cache_edb_commit(cache->keygrpdb, txn))
            {
                NDRX_LOG(log_error, "Failed to commit - aborting...!");
                ndrx_cache_edb_abort(cache->keygrpdb, txn);
                ret=EXFAIL;
            }
        }
        else
        {
            ndrx_cache_edb_abort(cache->keygrpdb, txn);
        }
    }
   
    NDRX_LOG(log_debug, "%s return %d", __func__, ret);
    return ret;
}
