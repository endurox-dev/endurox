/*
** Field database support routines
**
** @file fielddb.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2018, Mavimax, Ltd. All Rights Reserved.
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
#include <string.h>
#include <ubf_int.h>
#include <stdlib.h>
#include <errno.h>

#include <fieldtable.h>
#include <fdatatype.h>
#include <ferror.h>
#include <utlist.h>

#include "ndebug.h"
#include "ubf_tls.h"
#include "cconfig.h"
#include "expr.h"
#include <ubfdb.h>
#include <edbutil.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
expublic int ndrx_G_ubf_db_triedload = EXFALSE; /* Have we tried to load? */
/* If NULL and tried, then no db defined  */
expublic ndrx_ubf_db_t * ndrx_G_ubf_db = NULL;

/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Map DB error
 * @param unixerr unix error
 * @return UBF error
 */
expublic int ndrx_ubfdb_maperr(int unixerr)
{
    int ret = BEUNIX;
    
    switch (unixerr)
    {
        case EDB_NOTFOUND:
            
            ret = BBADFLD;
            
            break;
    }
    
    return ret;
}

/**
 * Load database of fields
 * 
 * @return -1 EXFAIL, 0 -> no DB defined, 1 -> DB defined and loaded
 */
expublic int ndrx_ubfdb_Bflddbload(void)
{
    int ret = EXSUCCEED;
    int is_loaded=EXFALSE;
    int any_config=EXFALSE;
    int tran_started = EXFALSE;
    EDB_txn *txn = NULL;
    ndrx_inicfg_section_keyval_t * csection = NULL, *val = NULL, *val_tmp = NULL;
    
    ndrx_G_ubf_db_triedload=EXTRUE;
            
    if (NULL!=ndrx_G_ubf_db)
    {
        UBF_LOG(log_warn, "UBF DB already loaded!");
        goto out;
    }
    
    if (NULL==(ndrx_G_ubf_db = NDRX_CALLOC(1, sizeof(ndrx_ubf_db_t))))
    {
        int err = errno;
        UBF_LOG(log_error, "%s: Failed to alloc %d bytes: %s",
                    __func__, sizeof(ndrx_ubf_db_t), strerror(err));
        
        ndrx_Bset_error_fmt(BEUNIX, "%s: Failed to alloc %d bytes: %s",
                    __func__, sizeof(ndrx_ubf_db_t), strerror(err));
        
        userlog("%s: Failed to alloc %d bytes: %s",
                    __func__, sizeof(ndrx_ubf_db_t), strerror(err));
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=ndrx_cconfig_get(NDRX_CONF_SECTION_UBFDB, &csection))
    {
        UBF_LOG(log_debug, "UBF DB not defined");
        goto out;
    }
    
    /* loop over the config... */
    ndrx_G_ubf_db->max_readers = NDRX_UBFDB_MAX_READERS_DFLT;
    ndrx_G_ubf_db->map_size = NDRX_UBFDB_MAP_SIZE_DFLT;
    ndrx_G_ubf_db->perms = NDRX_UBFDB_PERMS_DFLT;
    
    EXHASH_ITER(hh, csection, val, val_tmp)
    {
        
        any_config = EXTRUE;
        
        UBF_LOG(log_debug, "%s: config: key: [%s] value: [%s]",
                    __func__, val->key, val->val);
        
        if (0==strcmp(val->key, NDRX_UBFDB_KWD_RESOURCE))
        {
            NDRX_STRCPY_SAFE(ndrx_G_ubf_db->resource, val->val);
        }
        else if (0==strcmp(val->key, NDRX_UBFDB_KWD_PERMS))
        {
            char *pend;
            ndrx_G_ubf_db->perms = strtol(val->val, &pend, 0);
        }
        /* Also float: Parse 1000, 1K, 1M, 1G */
        else if (0==strcmp(val->key, NDRX_UBFDB_KWD_MAX_READERS))
        {
            ndrx_G_ubf_db->max_readers = (long)ndrx_num_dec_parsecfg(val->val);
        }
        /* Parse float: 1000.5, 1.2K, 1M, 1G */
        else if (0==strcmp(val->key, NDRX_UBFDB_KWD_MAP_SIZE))
        {
            ndrx_G_ubf_db->map_size = (long)ndrx_num_dec_parsecfg(val->val);
        }
        else
        {
            UBF_LOG(log_warn, "Ignoring unknown cache configuration param: [%s]", 
                    val->key);
            userlog("Ignoring unknown cache configuration param: [%s]", 
                    val->key);
        }
    }
 
    NDRX_UBFDB_DUMPCFG(log_debug, ndrx_G_ubf_db);
    
    if (EXSUCCEED!=(ret=edb_env_create(&ndrx_G_ubf_db->env)))
    {
        NDRX_UBFDB_BERROR(ndrx_ubfdb_maperr(ret), 
                "%s: Failed to create env for UBF table DB: %s", 
                __func__, edb_strerror(errno));
        EXFAIL_OUT(ret);
    }
    
    
    if (EXSUCCEED!=(ret=edb_env_set_maxreaders(ndrx_G_ubf_db->env, 
            ndrx_G_ubf_db->max_readers)))
    {
        NDRX_UBFDB_BERROR(ndrx_ubfdb_maperr(ret), 
                "%s: Failed to set max readers for ubf db: %s", 
                __func__, edb_strerror(ret));
        
        EXFAIL_OUT(ret);
    }

    if (EXSUCCEED!=(ret=edb_env_set_maxdbs(ndrx_G_ubf_db->env, 2)))
    {
        NDRX_UBFDB_BERROR(ndrx_ubfdb_maperr(ret), 
                "%s: Failed to set max dbs for ubf db: %s", 
                __func__, edb_strerror(ret));
        
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=(ret=edb_env_set_mapsize(ndrx_G_ubf_db->env, 
            ndrx_G_ubf_db->map_size)))
    {
        NDRX_UBFDB_BERROR(ndrx_ubfdb_maperr(ret), 
                "%s: Failed to set map size for ubf db: %s", 
                __func__, edb_strerror(ret));
        
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=(ret=edb_env_open(ndrx_G_ubf_db->env, ndrx_G_ubf_db->resource, 
            0, ndrx_G_ubf_db->perms)))
    {
        NDRX_UBFDB_BERROR(ndrx_ubfdb_maperr(ret), 
                "%s: Failed to open env for resource [%s]: %s", 
                __func__, ndrx_G_ubf_db->resource, edb_strerror(ret));
        
        EXFAIL_OUT(ret);
    }
    
    /* Prepare the DB */
    if (EXSUCCEED!=(ret=edb_txn_begin(ndrx_G_ubf_db->env, NULL, 0, &txn)))
    {
        NDRX_UBFDB_BERROR(ndrx_ubfdb_maperr(ret), 
                "%s: Failed to begin transaction for ubf db: %s", 
                __func__, edb_strerror(ret));
        
        EXFAIL_OUT(ret);
    }
    tran_started = EXTRUE;
    
    /* name database */
    if (EXSUCCEED!=(ret=edb_dbi_open(txn, "nm", EDB_CREATE, &ndrx_G_ubf_db->dbi_nm)))
    {
        NDRX_UBFDB_BERROR(ndrx_ubfdb_maperr(ret), 
                "%s: Failed to open named db for ubf db: %s", 
                __func__, edb_strerror(ret));
        
        EXFAIL_OUT(ret);
    }
    
    /* id database */
    if (EXSUCCEED!=(ret=edb_dbi_open(txn, "id", EDB_CREATE, &ndrx_G_ubf_db->dbi_id)))
    {
        NDRX_UBFDB_BERROR(ndrx_ubfdb_maperr(ret), 
                "%s: Failed to open named id for ubf db: %s", 
                __func__, edb_strerror(ret));
        
        EXFAIL_OUT(ret);
    }
    
    /* commit the tran */
    if (EXSUCCEED!=(ret=edb_txn_commit(txn)))
    {
        NDRX_UBFDB_BERROR(ndrx_ubfdb_maperr(ret), 
                "%s: Failed to open named commit: %s", 
                __func__, edb_strerror(ret));
        txn = NULL;
        EXFAIL_OUT(ret);        
    }
    
    tran_started = EXFALSE;
    
    
out:

    if (NULL!=csection)
    {
        ndrx_keyval_hash_free(csection);
    }
         
    if (tran_started)
    {
        edb_txn_abort(txn);
    }

    

    if (EXSUCCEED!=ret)
    {        
        if (NULL!=ndrx_G_ubf_db)
        {
            if (NULL!=ndrx_G_ubf_db->env)
            {
                edb_env_close(ndrx_G_ubf_db->env);
            }

            NDRX_FREE(ndrx_G_ubf_db);
        }
        
        ndrx_G_ubf_db = NULL;
    }

    /* return  */
    if (EXSUCCEED==ret && is_loaded)
    {
        return EXTRUE;
    }

    return ret;
}

/**
 * Add the field to database
 * @param txn exdb(lmdb) transaction
 * @param bfldno field number (not compiled)
 * @param fldtype filed id
 * @param fldname field name
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_ubfdb_Bflddbadd(EDB_txn *txn, BFLDID bfldno, 
        short fldtype, char *fldname)
{
    int ret = EXSUCCEED;
    ndrx_ubfdb_entry_t entry;
    BFLDID idcomp = Bmkfldid(fldtype, bfldno);
    EDB_val key;
    EDB_val data;
    
    /* prepare data object */
    entry.bfldid = idcomp;
    NDRX_STRCPY_SAFE(entry.fldname, fldname);
    
    data.mv_size = sizeof(entry);
    data.mv_data = &entry;
    
    
    key.mv_data = &idcomp;
    key.mv_size = sizeof(idcomp);
    
    /* Add ID: */
    UBF_LOG(log_debug, "About to put ID record (%d) / [%s]", (int)idcomp, 
            entry.fldname);
    
    if (EXSUCCEED!=(ret=edb_put(txn, ndrx_G_ubf_db->dbi_id, &key, &data, 0)))
    {
        NDRX_UBFDB_BERROR(ndrx_ubfdb_maperr(ret), 
                "%s: Failed to put ID (id=%d/[%s]) record: %s", 
                __func__, (int)idcomp, entry.fldname, edb_strerror(ret));   
        EXFAIL_OUT(ret);
    }
    
    UBF_LOG(log_debug, "About to put NAME record (%d) / [%s]", (int)idcomp, 
            entry.fldname);
    
    key.mv_data = entry.fldname;
    key.mv_size = strlen(entry.fldname)+1;
    
    if (EXSUCCEED!=(ret=edb_put(txn, ndrx_G_ubf_db->dbi_nm, &key, &data, 0)))
    {
        NDRX_UBFDB_BERROR(ndrx_ubfdb_maperr(ret), 
                "%s: Failed to put ID (id=%d/[%s]) record: %s", 
                __func__, (int)idcomp, entry.fldname, edb_strerror(ret));   
        EXFAIL_OUT(ret);
    }
    
out:    

    UBF_LOG(log_debug, "%s returns %d", __func__, ret);

    return ret;
}

/**
 * Delete field by id (deletes from both NM & ID bases)
 * @param txn LMDB transaction into which delete the field
 * @param bfldid compiled field id
 * @return EXSUCCEED/EXFAIL (Berror set)
 */
expublic int ndrx_ubfdb_Bflddbdel(EDB_txn *txn, BFLDID bfldid)
{
    int ret = EXSUCCEED;
    char fldname[UBFFLDMAX+1] = {EXEOS};
    char *p;
    EDB_val key;
    
    key.mv_data = &bfldid;
    key.mv_size = sizeof(bfldid);
    
    if (NULL==(p = Bfname(bfldid)))
    {
        NDRX_UBFDB_BERRORNOU(log_info, BNOTPRES, "Field by id: %d not found!",
                    (int)bfldid);
        EXFAIL_OUT(ret);
    }
    
    NDRX_STRCPY_SAFE(fldname, p);
    /* Delete ID: */
    UBF_LOG(log_debug, "%s: delete by %d", __func__, (int)bfldid);
    
    if (EXSUCCEED!=(ret=edb_del(txn, ndrx_G_ubf_db->dbi_id, &key, NULL)))
    {
        if (ret!=EDB_NOTFOUND)
        {
            NDRX_UBFDB_BERROR(ndrx_ubfdb_maperr(ret), 
                    "%s: Failed to delete by ID (id=%d) record: %s", 
                    __func__, (int)bfldid, edb_strerror(ret));   
            EXFAIL_OUT(ret);
        }
        else
        {
            UBF_LOG(log_info, "%s: Field [%d] not found in db", __func__, 
                    (int)bfldid);
        }
    }
    
    UBF_LOG(log_debug, "About to delete by NAME [%s]", fldname);
    
    key.mv_data = fldname;
    key.mv_size = strlen(fldname)+1;
    
    if (EXSUCCEED!=(ret=edb_del(txn, ndrx_G_ubf_db->dbi_nm, &key, NULL)))
    {
        if (ret!=EDB_NOTFOUND)
        {
            NDRX_UBFDB_BERROR(ndrx_ubfdb_maperr(ret), 
                    "%s: Failed to delete by field name ([%s]) record: %s", 
                    __func__, fldname, edb_strerror(ret));   
            EXFAIL_OUT(ret);
        }
        else
        {
            UBF_LOG(log_info, "%s: Field [%s] not found in db", 
                    __func__, fldname);
        }
    }
    
out:    

    UBF_LOG(log_debug, "%s returns %d", __func__, ret);

    return ret;
}

/**
 * Delete all records form db (ID & NM)
 * @param txn LMDB transaction into which delete the fields
 * @return EXSUCCEED/EXFAIL (B error set)
 */
expublic int ndrx_ubfdb_Bflddbdrop(EDB_txn *txn)
{
    int ret = EXSUCCEED;
    
    if (EXSUCCEED!=(ret=edb_drop(txn, ndrx_G_ubf_db->dbi_id, 0)))
    {
        NDRX_UBFDB_BERROR(ndrx_ubfdb_maperr(ret), 
                "%s: Failed to drop id db: %s", 
                __func__, edb_strerror(ret));   
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=(ret=edb_drop(txn, ndrx_G_ubf_db->dbi_nm, 0)))
    {
        NDRX_UBFDB_BERROR(ndrx_ubfdb_maperr(ret), 
                "%s: Failed to drop name db: %s", 
                __func__, edb_strerror(ret));   
        EXFAIL_OUT(ret);
    }
    
out:
    return ret;
}

/**
 * Close the database
 * @return 
 */
expublic void ndrx_ubfdb_Bflddbunload(void)
{
    ndrx_G_ubf_db_triedload=EXFALSE;
    
    if (NULL!=ndrx_G_ubf_db)
    {
        edb_dbi_close(ndrx_G_ubf_db->env, ndrx_G_ubf_db->dbi_id);
        edb_dbi_close(ndrx_G_ubf_db->env, ndrx_G_ubf_db->dbi_nm);
        edb_env_close(ndrx_G_ubf_db->env);
        
        NDRX_FREE(ndrx_G_ubf_db);
        
        ndrx_G_ubf_db = NULL;
    } 
}

/**
 * Unlink the field database (delete data files)
 * @return EXSUCCEED/EXFAIL (UBF error set)
 */
expublic int ndrx_ubfdb_Bflddbunlink(void)
{
    int ret = EXSUCCEED;
    char errdet[MAX_TP_ERROR_LEN+1];
    ndrx_inicfg_section_keyval_t * csection = NULL, *res = NULL;
    
    if (EXSUCCEED!=ndrx_cconfig_get(NDRX_CONF_SECTION_UBFDB, &csection))
    {
        UBF_LOG(log_debug, "UBF DB not defined");
        goto out;
    }
    
    EXHASH_FIND_STR( csection, NDRX_UBFDB_KWD_RESOURCE, res);
    
    if (NULL!=res)
    {
        if (EXSUCCEED!=ndrx_mdb_unlink(res->val, errdet, sizeof(errdet), 
                LOG_CODE_UBF))
        {
            NDRX_UBFDB_BERROR(BEUNIX, 
                    "%s: Failed to unlink [%s] UBF DB: %s", 
                    __func__, res->val, errdet);
            EXFAIL_OUT(ret);
        }
    }
    else
    {
        UBF_LOG(log_debug, "%s: no UBF DB [%s] section found in config", 
                __func__, NDRX_CONF_SECTION_UBFDB);
    }
    
out:

    if (NULL!=csection)
        ndrx_keyval_hash_free(csection);

    return ret;
}

/**
 * Loop over the fields
 * @param key key returned by cursor
 * @param val value returned by cursor
 * @param p_bfldno field number
 * @param p_bfldid field id
 * @param p_fldtype field type id
 * @param fldname field name
 * @param fldname_bufsz filed name buffer size
 * @return EXSUCCEED/EXFAIL (Berror set)
 */
expublic int ndrx_ubfdb_Bflddbget(EDB_val *key, EDB_val *data,
        BFLDID *p_bfldno, BFLDID *p_bfldid, 
        short *p_fldtype, char *fldname, int fldname_bufsz)
{
    int ret = EXSUCCEED;
    ndrx_ubfdb_entry_t *entry;
    
    if (data->mv_size!=sizeof(ndrx_ubfdb_entry_t))
    {
        NDRX_UBFDB_BERROR(BEINVAL, 
                    "%s: Expected data size %d, but got %d!", 
                    __func__, (int)sizeof(ndrx_ubfdb_entry_t), (int)data->mv_size);
            EXFAIL_OUT(ret);
    }
    
    entry = (ndrx_ubfdb_entry_t *)data->mv_data;
    
    *p_bfldid = entry->bfldid;
    *p_bfldno = entry->bfldid & EFFECTIVE_BITS_MASK;
    *p_fldtype = entry->bfldid >> EFFECTIVE_BITS;
    
    NDRX_STRNCPY_SAFE(fldname, entry->fldname, fldname_bufsz);
    
    UBF_LOG(log_debug, "%s: fldno=%d fldid=%d fldtype=%d fldname=[%s]",
                __func__, *p_bfldno, *p_bfldid, *p_fldtype, fldname);
out:   
    return ret;
}

/**
 * Resolve field id from field name
 * Lookup transaction is generated locally.
 * @param fldnm
 * @return 
 */
expublic char * ndrx_ubfdb_Bflddbname (BFLDID bfldid)
{
    int ret = EXSUCCEED;
    EDB_txn *txn = NULL;
    int tran_started = EXFALSE;
    EDB_val key, data;
    static __thread char fname[UBFFLDMAX+1];
    ndrx_ubfdb_entry_t *entry;
    /* Prepare the DB */
    if (EXSUCCEED!=(ret=edb_txn_begin(ndrx_G_ubf_db->env, NULL, EDB_RDONLY, &txn)))
    {
        NDRX_UBFDB_BERROR(ndrx_ubfdb_maperr(ret), 
                "%s: Failed to begin transaction for ubf db: %s", 
                __func__, edb_strerror(ret));
        
        EXFAIL_OUT(ret);
    }

    tran_started = EXTRUE;
    
    key.mv_size = sizeof(bfldid);
    key.mv_data = &bfldid;
    
    if (EXSUCCEED!=(ret=edb_get(txn, ndrx_G_ubf_db->dbi_id, &key, &data)))
    {
        if (ret==EDB_NOTFOUND)
        {
            /* ok, this is weak error */
            NDRX_UBFDB_BERRORNOU(log_info, ndrx_ubfdb_maperr(ret), 
                    "%s: Field not present in UBF DB (%d): %s", 
                    __func__, (int)bfldid, edb_strerror(ret));
            EXFAIL_OUT(ret);
        }
        else
        {
            NDRX_UBFDB_BERROR(ndrx_ubfdb_maperr(ret), 
                    "%s: Failed to get data by field id %d: %s", 
                    __func__, (int)bfldid, edb_strerror(ret));
        }
        EXFAIL_OUT(ret);
    }
    
    if (sizeof(*entry)!=data.mv_size)
    {
        NDRX_UBFDB_BERROR(BEINVAL, 
                "%s: Invalid data size expected %d got %d", 
                __func__, (int)sizeof(*entry), (int)data.mv_size);
        
        EXFAIL_OUT(ret);
    }
    
    entry = (ndrx_ubfdb_entry_t *)data.mv_data;
    
    NDRX_STRCPY_SAFE(fname, entry->fldname);
    
    
    UBF_LOG(log_debug, "%s: bfldid=%d resolved to [%s]", __func__, bfldid, 
            fname);
    
out:
    
    /* for reads we can abort easily */
    if (tran_started)
    {
        edb_txn_abort(txn);
    }

    if (EXSUCCEED==ret)
    {
        return fname;
    }

    return NULL;
}

/**
 * Resolve field by name
 * @param fldname filed name
 * @return Field id or EXFAIL (B error set)
 */
expublic BFLDID ndrx_ubfdb_Bflddbid (char *fldname)
{
    int ret = EXSUCCEED;
    EDB_txn *txn = NULL;
    int tran_started = EXFALSE;
    EDB_val key, data;
    ndrx_ubfdb_entry_t *entry;
    
    /* Prepare the DB */
    if (EXSUCCEED!=(ret=edb_txn_begin(ndrx_G_ubf_db->env, NULL, EDB_RDONLY, &txn)))
    {
        NDRX_UBFDB_BERROR(ndrx_ubfdb_maperr(ret), 
                "%s: Failed to begin transaction for ubf db: %s", 
                __func__, edb_strerror(ret));
        
        EXFAIL_OUT(ret);
    }

    tran_started = EXTRUE;
    
    key.mv_size = strlen(fldname)+1;
    key.mv_data = fldname;
    
    if (EXSUCCEED!=(ret=edb_get(txn, ndrx_G_ubf_db->dbi_nm, &key, &data)))
    {
        if (ret==EDB_NOTFOUND)
        {
            /* ok, this is weak error */
            NDRX_UBFDB_BERRORNOU(log_info, ndrx_ubfdb_maperr(ret), 
                    "%s: Field not present in UBF DB by name [%s]: %s", 
                    __func__, fldname, edb_strerror(ret));
            EXFAIL_OUT(ret);
        }
        else
        {
            NDRX_UBFDB_BERROR(ndrx_ubfdb_maperr(ret), 
                    "%s: Failed to get data by field name [%s]: %s", 
                    __func__, fldname, edb_strerror(ret));
        }
        EXFAIL_OUT(ret);
    }
    
    if (sizeof(*entry)!=data.mv_size)
    {
        NDRX_UBFDB_BERROR(BEINVAL, 
                "%s: Invalid data size expected %d got %d", 
                __func__, (int)sizeof(*entry), (int)data.mv_size);
        
        EXFAIL_OUT(ret);
    }
    
    entry = (ndrx_ubfdb_entry_t *)data.mv_data;
    
    ret = entry->bfldid;
    
    
    UBF_LOG(log_debug, "%s: name [%s] resolved to field id %d", __func__, 
            fldname, ret);
    
out:
    
    /* for reads we can abort easily */
    if (tran_started)
    {
        edb_txn_abort(txn);
    }

    if (ret<0)
    {
        return BBADFLDID;
    }

    return ret;
}

/**
 * Get the LMDB database environment handle
 * @param[out] dbi_id id database handler
 * @param[out] dbi_nm name database handler
 * @return NULL if no env set, or env ptr
 */
expublic EDB_env * ndrx_ubfdb_Bfldddbgetenv (EDB_dbi **dbi_id, EDB_dbi **dbi_nm)
{
    EDB_env * ret = NULL;
    
    if (NULL!=ndrx_G_ubf_db)
    {
        *dbi_id = &ndrx_G_ubf_db->dbi_id;
        *dbi_nm = &ndrx_G_ubf_db->dbi_nm;
        ret = ndrx_G_ubf_db->env;
    }
    
out:
    return ret;
}

