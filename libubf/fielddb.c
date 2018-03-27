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
#include <ubfdb.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
expublic ndrx_ubf_db_t ndrx_G_ubf_db = {.is_init = EXFALSE, 
                .is_loaded = EXFALSE,
                .resource[0] = EXEOS};

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
            
            ret = BNOTPRES;
            
            break;
    }
    
    return ret;
}

/**
 * Load database of fields
 * 
 * @return -1 EXFAIL, 0 -> no DB defined, 1 -> DB defined and loaded
 */
expublic int ndrx_ubf_db_load(void)
{
    int ret = EXSUCCEED;
    int is_loaded=EXFALSE;
    int any_config=EXFALSE;
    EDB_txn *txn = NULL;
    ndrx_inicfg_section_keyval_t * csection = NULL, *val = NULL, *val_tmp = NULL;
    
    if (EXSUCCEED!=ndrx_cconfig_get(NDRX_CONF_SECTION_UBFDB, &csection))
    {
        UBF_LOG(log_debug, "UBF DB not defined");
        ndrx_G_ubf_db.is_init = EXTRUE;
        ndrx_G_ubf_db.is_loaded = EXFALSE;
        goto out;
    }
    
    /* loop over the config... */
    ndrx_G_ubf_db.max_readers = NDRX_UBFDB_MAX_READERS_DFLT;
    ndrx_G_ubf_db.map_size = NDRX_UBFDB_MAP_SIZE_DFLT;
    ndrx_G_ubf_db.perms = NDRX_UBFDB_PERMS_DFLT;
    
    EXHASH_ITER(hh, csection, val, val_tmp)
    {
        
        any_config = EXTRUE;
        
        UBF_LOG(log_debug, "%s: config: key: [%s] value: [%s]",
                    __func__, val->key, val->val);
        
        if (0==strcmp(val->key, NDRX_UBFDB_KWD_RESOURCE))
        {
            NDRX_STRCPY_SAFE(ndrx_G_ubf_db.resource, val->val);
        }
        else if (0==strcmp(val->key, NDRX_UBFDB_KWD_PERMS))
        {
            char *pend;
            ndrx_G_ubf_db.perms = strtol(val->val, &pend, 0);
        }
        /* Also float: Parse 1000, 1K, 1M, 1G */
        else if (0==strcmp(val->key, NDRX_UBFDB_KWD_MAX_READERS))
        {
            ndrx_G_ubf_db.max_readers = (long)ndrx_num_dec_parsecfg(val->val);
        }
        /* Parse float: 1000.5, 1.2K, 1M, 1G */
        else if (0==strcmp(val->key, NDRX_UBFDB_KWD_MAP_SIZE))
        {
            ndrx_G_ubf_db.map_size = (long)ndrx_num_dec_parsecfg(val->val);
        }
        else
        {
            NDRX_LOG(log_warn, "Ignoring unknown cache configuration param: [%s]", 
                    val->key);
            userlog("Ignoring unknown cache configuration param: [%s]", 
                    val->key);
        }
    }
 
    if (EXSUCCEED!=(ret=edb_env_create(&ndrx_G_ubf_db.env)))
    {
        NDRX_UBFDB_BERROR(ndrx_ubfdb_maperr(ret), 
                "%s: Failed to create env for UBF table DB: %s", 
                __func__, edb_strerror(errno));
        EXFAIL_OUT(ret);
    }
    
    
    if (EXSUCCEED!=(ret=edb_env_set_maxreaders(ndrx_G_ubf_db.env, 
            ndrx_G_ubf_db.max_readers)))
    {
        NDRX_UBFDB_BERROR(ndrx_ubfdb_maperr(ret), 
                "%s: Failed to set max readers for ubf db: %s", 
                __func__, edb_strerror(ret));
        
        EXFAIL_OUT(ret);
    }

    if (EXSUCCEED!=(ret=edb_env_set_maxdbs(ndrx_G_ubf_db.env, 2)))
    {
        NDRX_UBFDB_BERROR(ndrx_ubfdb_maperr(ret), 
                "%s: Failed to set max dbs for ubf db: %s", 
                __func__, edb_strerror(ret));
        
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=(ret=edb_env_set_mapsize(ndrx_G_ubf_db.env, 
            ndrx_G_ubf_db.map_size)))
    {
        NDRX_UBFDB_BERROR(ndrx_ubfdb_maperr(ret), 
                "%s: Failed to set map size for ubf db: %s", 
                __func__, edb_strerror(ret));
        
        EXFAIL_OUT(ret);
    }
    
    /* Prepare the DB */
    if (EXSUCCEED!=(ret=edb_txn_begin(ndrx_G_ubf_db.env, NULL, 0, &txn)))
    {
        NDRX_UBFDB_BERROR(ndrx_ubfdb_maperr(ret), 
                "%s: Failed to begin transaction for ubf db: %s", 
                __func__, edb_strerror(ret));
        
        EXFAIL_OUT(ret);
    }
    
    /* name database */
    if (EXSUCCEED!=(ret=edb_dbi_open(txn, "nm", 0, &ndrx_G_ubf_db.dbi_nm)))
    {
        NDRX_UBFDB_BERROR(ndrx_ubfdb_maperr(ret), 
                "%s: Failed to open named db for ubf db: %s", 
                __func__, edb_strerror(ret));
        
        EXFAIL_OUT(ret);
    }
    
    /* id database */
    if (EXSUCCEED!=(ret=edb_dbi_open(txn, "id", 0, &ndrx_G_ubf_db.dbi_id)))
    {
        NDRX_UBFDB_BERROR(ndrx_ubfdb_maperr(ret), 
                "%s: Failed to open named db for ubf db: %s", 
                __func__, edb_strerror(ret));
        
        EXFAIL_OUT(ret);
    }
    
    /* commit the tran */
    if (EXSUCCEED!=(ret=edb_txn_commit(txn)))
    {
        NDRX_UBFDB_BERROR(ndrx_ubfdb_maperr(ret), 
                "%s: Failed to open named db for ubf db: %s", 
                __func__, edb_strerror(ret));
        txn = NULL;
        EXFAIL_OUT(ret);        
    }
    
    ndrx_G_ubf_db.is_init = EXTRUE;
    ndrx_G_ubf_db.is_loaded = EXTRUE;
    
    NDRX_UBFDB_DUMPCFG(log_debug, (&ndrx_G_ubf_db));
    
out:

    if (EXSUCCEED!=ret)
    {        
        if (NULL!=txn)
        {
            edb_txn_abort(txn);
        }
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
 * @param bfldid field id (non compiled)
 * @param fldtype filed id
 * @param fldname field name
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_ubfdb_Bfldadd(BFLDID bfldid, _UBF_SHORT fldtype, char *fldname)
{
    return EXFAIL;
}

/**
 * Delete field by name
 * @param fldname
 * @return 
 */
expublic int ndrx_ubfdb_Bflddel(char *fldname)
{
    return EXFAIL;
}

/**
 * Unlink the field database (delete data files)
 * @return 
 */
expublic int ndrx_ubfdb_Bfldunlink(void)
{
    return EXFAIL;
}

/**
 * Delete all records form db
 * @return 
 */
expublic int ndrx_ubfdb_Bflddrop(void)
{
    return EXFAIL;
}

/**
 * Loop over the fields
 * @return 
 */
expublic int ndrx_ubfdb_Bfldnext(void)
{
    return EXFAIL;
}

/**
 * Close the loop over the fields
 * @return 
 */
expublic int ndrx_ubfdb_Bfldnextclose(void)
{
    return EXFAIL;
}

/**
 * Close the database
 * @return 
 */
expublic int ndrx_ubfdb_uninit(void)
{
    return EXFAIL;
}

/**
 * Resolve field id from field name
 * @param fldnm
 * @return 
 */
expublic BFLDID ndrx_ubfdb_Bfldid (char *fldnm)
{
    return EXFAIL;
}

/**
 * Return field name, stored in TLS, only one copy at the time!
 * @param bfldid
 * @return 
 */
expublic char * ndrx_ubfdb_Bfname (BFLDID bfldid)
{
    return NULL;
}
