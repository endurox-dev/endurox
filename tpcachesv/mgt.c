/**
 * @brief Management routines of cache server
 *
 * @file mgt.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or Mavimax's license for commercial use.
 * -----------------------------------------------------------------------------
 * GPL license:
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * -----------------------------------------------------------------------------
 * A commercial use license is available from Mavimax, Ltd
 * contact@mavimax.com
 * -----------------------------------------------------------------------------
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <regex.h>
#include <utlist.h>

#include <ndebug.h>
#include <atmi.h>
#include <sys_unix.h>
#include <atmi_int.h>
#include <typed_buf.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <Exfields.h>
#include <atmi_shm.h>
#include <exregex.h>
#include "tpcachesv.h"
#include <atmi_cache.h>
#include <ubfutil.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define REJECT(UB, TPE, MSG)\
            NDRX_LOG(log_error, "Reject with %d: %s", TPE, MSG);\
            atmi_reject(UB, TPE, MSG);
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Reject request with installing error in buffer
 * @param p_svc
 */
exprivate void atmi_reject(UBFH *p_ub, long in_tperrno, char *in_msg)
{
    Bchg(p_ub, EX_TPERRNO, 0, (char *)&in_tperrno, 0L);
    Bchg(p_ub, EX_TPSTRERROR, 0, in_msg, 0L);
}

/**
 * Loop over the database and stream results to terminal
 * @param pp_ub double ptr to DUBF
 * @return 
 */
exprivate int cache_show(int cd, UBFH **pp_ub)
{
    int ret = EXSUCCEED;
    char cachedb[NDRX_CCTAG_MAX+1];
    ndrx_tpcache_db_t *db;
    char tmp[256];
    EDB_txn *txn = NULL;
    EDB_cursor *cursor;
    EDB_cursor_op op;
    EDB_val keydb, val;
    int tran_started = EXFALSE;
    ndrx_tpcache_data_t *cdata;
    long revent;
    int cursor_open = EXFALSE;
    
    /* ok get db name */
    
    if (EXSUCCEED!=Bget(*pp_ub, EX_CACHE_DBNAME, 0, cachedb, 0L))
    {
        NDRX_LOG(log_error, "Missing EX_CACHE_DBNAME: %s", Bstrerror(Berror));
        REJECT(*pp_ub, TPESYSTEM, "Failed to get EX_CACHE_DBNAME!");
    }
    
    NDRX_LOG(log_debug, "[%s] db lookup", cachedb);
    
    if (NULL==(db = ndrx_cache_dbresolve(cachedb, NDRX_TPCACH_INIT_NORMAL)))
    {
        /* if have ATMI error use, else failed to open db */
        
        if (ndrx_TPis_error())
        {
            REJECT(*pp_ub, tperrno, tpstrerror(tperrno));
        }
        else
        {
            snprintf(tmp, sizeof(tmp), "Failed to resolve [%s] db", cachedb);
            REJECT(*pp_ub, TPENOENT, tmp);
        }
        
        EXFAIL_OUT(ret);
    }
    
    /* OK we have a cache, so loop over (this will include duplicate records too
     * so that admin can see the picture of all this)
     */

    /* start transaction */
    if (EXSUCCEED!=(ret=ndrx_cache_edb_begin(db, &txn, EDB_RDONLY)))
    {
        NDRX_CACHE_TPERROR(TPESYSTEM, "%s: failed to start tran", __func__);
        goto out;
    }
    
    tran_started = EXTRUE;

    /* loop over the database */
    if (EXSUCCEED!=ndrx_cache_edb_cursor_open(db, txn, &cursor))
    {
        NDRX_LOG(log_error, "Failed to open cursor");
        EXFAIL_OUT(ret);
    }
    
    /* loop over the db and match records  */
    
    op = EDB_FIRST;
    do
    {
        if (EXSUCCEED!=(ret = ndrx_cache_edb_cursor_getfullkey(db, cursor, 
                &keydb, &val, op)))
        {
            if (EDB_NOTFOUND==ret)
            {
                /* this is ok */
                ret = EXSUCCEED;
                break;
            }
            else
            {
                REJECT(*pp_ub, tperrno, tpstrerror(tperrno));
                NDRX_LOG(log_error, "Failed to loop over the [%s] db", cachedb);
                break;
            }
        }
        
        /* Validate DB rec */
        
        if (EXEOS!=((char *)keydb.mv_data)[keydb.mv_size-1])
        {
            NDRX_DUMP(log_error, "Invalid cache key", 
                    keydb.mv_data, keydb.mv_size);
            
            NDRX_LOG(log_error, "%s: Invalid cache key, len: %ld not "
                    "terminated with EOS!", __func__, keydb.mv_size);
            REJECT(*pp_ub, TPESYSTEM, "Corrupted db - Invalid key format");
            EXFAIL_OUT(ret);
        }
        
        if (val.mv_size < sizeof(ndrx_tpcache_data_t))
        {
            snprintf(tmp, sizeof(tmp), "Corrupted data - invalid minimums size, "
                    "expected: %ld, got %ld", 
                    (long)sizeof(ndrx_tpcache_data_t), (long)val.mv_size);
            REJECT(*pp_ub, TPESYSTEM, tmp);
            EXFAIL_OUT(ret);
        }
        
        /* Load record into UBF and send it away... */
        
        cdata = (ndrx_tpcache_data_t *)val.mv_data;
        
        
        if (NDRX_CACHE_MAGIC!=cdata->magic)
        {
            snprintf(tmp, sizeof(tmp), "Corrupted data - invalid magic expected: %x got %x",
                    cdata->magic, NDRX_CACHE_MAGIC);
            REJECT(*pp_ub, TPESYSTEM, tmp);
            EXFAIL_OUT(ret);
        }
        
        if (EXSUCCEED!=ndrx_cache_mgt_data2ubf(cdata, keydb.mv_data, pp_ub, EXFALSE))
        {
            REJECT(*pp_ub, TPESYSTEM, "Failed to load data info UBF!");
            EXFAIL_OUT(ret);
        }
        
        ndrx_debug_dump_UBF(log_debug, "Sending packet", *pp_ub);
        
        /* send stuff away */
        if (EXFAIL == tpsend(cd,
                            (char *)*pp_ub,
                            0L,
                            0,
                            &revent))
        {
            
            NDRX_LOG(log_error, "Send data failed [%s] %ld",
                                tpstrerror(tperrno), revent);
            
            REJECT(*pp_ub, tperrno, "Failed to stream reply data");
            
            EXFAIL_OUT(ret);
        }
        else
        {
            NDRX_LOG(log_debug,"sent ok");
        }
        
        if (EDB_FIRST == op)
        {
            op = EDB_NEXT;
        }
        
    } while (EXSUCCEED==ret);
    
out:
    
    if (cursor_open)
    {
        edb_cursor_close(cursor);
    }

    if (tran_started)
    {
        ndrx_cache_edb_abort(db, txn);
    }
    return ret;
}

/**
 * Write cache record to UBF
 * @param pp_ub
 * @return EXSUCCEED/EXFAIL
 */
exprivate int cache_dump(UBFH **pp_ub)
{
    int ret = EXSUCCEED;
    char cachedb[NDRX_CCTAG_MAX+1];
    ndrx_tpcache_db_t *db;
    char tmp[256];
    EDB_txn *txn = NULL;
    EDB_cursor *cursor;
    EDB_val val;
    int tran_started = EXFALSE;
    ndrx_tpcache_data_t *cdata;
    char *key = NULL;
    
    /* ok get db name */
    
    if (EXSUCCEED!=Bget(*pp_ub, EX_CACHE_DBNAME, 0, cachedb, 0L))
    {
        NDRX_LOG(log_error, "Missing EX_CACHE_DBNAME: %s", Bstrerror(Berror));
        REJECT(*pp_ub, TPESYSTEM, "Failed to get EX_CACHE_DBNAME!");
    }
    
    NDRX_LOG(log_debug, "[%s] db lookup", cachedb);
    
    if (NULL==(db = ndrx_cache_dbresolve(cachedb, NDRX_TPCACH_INIT_NORMAL)))
    {
        /* if have ATMI error use, else failed to open db */
        
        if (ndrx_TPis_error())
        {
            REJECT(*pp_ub, tperrno, tpstrerror(tperrno));
        }
        else
        {
            snprintf(tmp, sizeof(tmp), "Failed to resolve [%s] db", cachedb);
            REJECT(*pp_ub, TPENOENT, tmp);
        }
        
        EXFAIL_OUT(ret);
    }

    /* start transaction */
    if (EXSUCCEED!=(ret=ndrx_cache_edb_begin(db, &txn, EDB_RDONLY)))
    {
        NDRX_CACHE_TPERROR(TPESYSTEM, "%s: failed to start tran", __func__);
        goto out;
    }
    
    tran_started = EXTRUE;

    /* loop over the database */
    if (EXSUCCEED!=ndrx_cache_edb_cursor_open(db, txn, &cursor))
    {
        NDRX_LOG(log_error, "Failed to open cursor");
        EXFAIL_OUT(ret);
    }
    
    if (NULL==(key = Bgetalloc(*pp_ub, EX_CACHE_OPEXPR, 0, NULL)))
    {
        REJECT(*pp_ub, TPESYSTEM, "Failed to get EX_CACHE_OPEXPR");
        EXFAIL_OUT(ret);
    }
    
    /* read db record */
    if (EXSUCCEED!=ndrx_cache_edb_get(db, txn, key, &val, EXTRUE))
    {
        REJECT(*pp_ub, tperrno, tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    /* Validate DB rec */

    if (val.mv_size < sizeof(ndrx_tpcache_data_t))
    {
        snprintf(tmp, sizeof(tmp), "Corrupted data - invalid minimums size, "
                "expected: %ld, got %ld", 
                (long)sizeof(ndrx_tpcache_data_t), (long)val.mv_size);
        REJECT(*pp_ub, TPESYSTEM, tmp);
        EXFAIL_OUT(ret);
    }

    /* Load record into UBF and send it away... */

    cdata = (ndrx_tpcache_data_t *)val.mv_data;

    if (NDRX_CACHE_MAGIC!=cdata->magic)
    {
        snprintf(tmp, sizeof(tmp), "Corrupted data - invalid magic expected: %x got %x",
                cdata->magic, NDRX_CACHE_MAGIC);
        REJECT(*pp_ub, TPESYSTEM, tmp);
        EXFAIL_OUT(ret);
    }

    if (EXSUCCEED!=ndrx_cache_mgt_data2ubf(cdata, key, pp_ub, EXTRUE))
    {
        REJECT(*pp_ub, TPESYSTEM, "Failed to load data info UBF!");
        EXFAIL_OUT(ret);
    }
    
    ndrx_debug_dump_UBF(log_debug, "Dump buffer built", *pp_ub);
    

out:

    if (tran_started)
    {
        ndrx_cache_edb_abort(db, txn);
    }

    if (NULL!=key)
    {
        NDRX_FREE(key);
    }

    return ret;
}

/**
 * Delete cache
 * - if present only db, then drop full db
 * - if present key only, then remove by key
 * - if present key with EX_CACHE_FLAGS, then delete 
 * @param pp_ub
 * @return EXSUCCEED/EXFAIL
 */
exprivate int cache_del(UBFH **pp_ub)
{
    int ret = EXSUCCEED;
    char cachedb[NDRX_CCTAG_MAX+1];
    long flags;
    char *key = NULL;
    char tmp[256];
    long deleted;
    
    /* ok get db name */
    
    /* resize buffer a bit */
    
    if (NULL==(*pp_ub = (UBFH *)tprealloc((char *)*pp_ub, Bused(*pp_ub) + 1024)))
    {
        NDRX_LOG(log_error, "Failed to realloc!");
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=Bget(*pp_ub, EX_CACHE_DBNAME, 0, cachedb, 0L))
    {
        NDRX_LOG(log_error, "Missing EX_CACHE_DBNAME: %s", Bstrerror(Berror));
        REJECT(*pp_ub, TPESYSTEM, "Failed to get EX_CACHE_DBNAME!");
    }
    
    /* get key if have one */
    key = Bgetalloc(*pp_ub, EX_CACHE_OPEXPR, 0, 0L);
    
    if (Bpres(*pp_ub, EX_CACHE_FLAGS, 0))
    {
        if (EXSUCCEED!=Bget(*pp_ub, EX_CACHE_FLAGS, 0, (char *)&flags, 0L))
        {
            snprintf(tmp, sizeof(tmp), "Failed to get EX_CACHE_FLAGS: %s", 
                    Bstrerror(Berror));
            REJECT(*pp_ub, TPENOENT, tmp);
            EXFAIL_OUT(ret);
        }
    }
    
    if (NULL==key)
    {
        /* Drop full DB */
        NDRX_LOG(log_info, "Delete full database: [%s]", cachedb);
        
        if (EXSUCCEED!=ndrx_cache_drop(cachedb, (short)tpgetnodeid()))
        {
            REJECT(*pp_ub, tperrno, tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
        
    }
    else if (flags & NDRX_CACHE_SVCMDF_DELREG)
    {
        NDRX_LOG(log_info, "Delete by regular expression: [%s] expr: [%s]", 
                cachedb, key);
        
        if (0>(deleted=ndrx_cache_inval_by_expr(cachedb, key, (short)tpgetnodeid())))
        {
            REJECT(*pp_ub, tperrno, tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
        
        NDRX_LOG(log_info, "Deleted %ld records", deleted);
        
        if (EXSUCCEED!=Bchg(*pp_ub, EX_CACHE_NRDEL, 0, (char *)&deleted, 0L))
        {
            snprintf(tmp, sizeof(tmp), "Failed to set EX_CACHE_NRDEL: %s", 
                    Bstrerror(Berror));
            REJECT(*pp_ub, TPESYSTEM, Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
    }
    else
    {
        /* delete by key */
        NDRX_LOG(log_info, "Delete by key. DB: [%s] Key: [%s]", 
                cachedb, key);
        
        if (0>(deleted=ndrx_cache_inval_by_key(cachedb, NULL, key, 
                (short)tpgetnodeid(), NULL, EXFALSE)))
        {
            REJECT(*pp_ub, tperrno, tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
        
        NDRX_LOG(log_info, "Deleted %ld records", deleted);
        
        if (EXSUCCEED!=Bchg(*pp_ub, EX_CACHE_NRDEL, 0, (char *)&deleted, 0L))
        {
            snprintf(tmp, sizeof(tmp), "Failed to set EX_CACHE_NRDEL: %s", 
                    Bstrerror(Berror));
            REJECT(*pp_ub, TPESYSTEM, Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
    }

out:


    if (NULL!=key)
    {
        NDRX_FREE(key);
    }

    return ret;
}

/**
 * Process management requests (list headers, list all, list 
 * single, delete all, delete single).
 * The dump shall be provided in tpexport format.
 * 
 * @param p_svc
 */
void CACHEMG (TPSVCINFO *p_svc)
{
    int ret = EXSUCCEED;
    UBFH *p_ub = (UBFH *)p_svc->data;
    ndrx_tpcache_db_t *db;
    char cmd;
    char tmp[256];
    /* Reallocate the p_svc  */
    
    if (NULL==(p_ub=(UBFH *)tprealloc((char *)p_ub, Bused(p_ub)+1024)))
    {
        NDRX_LOG(log_error, "Failed to realloc UBF!");
        EXFAIL_OUT(ret);
    }
    
    /* get command code */
    
    if (EXSUCCEED!=Bget(p_ub, EX_CACHE_CMD, 0, &cmd, 0L))
    {
        REJECT(p_ub, TPESYSTEM, "internal error: missing command field");
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_info, "Management command: %c", cmd);
    
    switch (cmd)
    {
        case NDRX_CACHE_SVCMD_CLSHOW:
            
            if (EXSUCCEED!=cache_show(p_svc->cd, &p_ub))
            {
                NDRX_LOG(log_error, "Failed to call cache_show()");
                EXFAIL_OUT(ret);
            }
            
            break;
        case NDRX_CACHE_SVCMD_CLCDUMP:
            
            if (EXSUCCEED!=cache_dump(&p_ub))
            {
                NDRX_LOG(log_error, "Failed to call cache_dump()");
                EXFAIL_OUT(ret);
            }
            
            break;
        case NDRX_CACHE_SVCMD_CLDEL:
            
            if (EXSUCCEED!=cache_del(&p_ub))
            {
                NDRX_LOG(log_error, "Failed to call cache_del()");
                EXFAIL_OUT(ret);
            }
            
            break;
        default:
            snprintf(tmp, sizeof(tmp), "Invalid command [%c]", cmd);
            REJECT(p_ub, TPESYSTEM, tmp);
            break;
    }
    
out:

    tpreturn(  ret==EXSUCCEED?TPSUCCESS:TPFAIL,
        0L,
        (char *)p_ub,
        0L,
        0L);

}
/* vim: set ts=4 sw=4 et smartindent: */
