/* 
** UBF field database
**
** @file ubfdb.h
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
#ifndef __UBFDB_H
#define	__UBFDB_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <ndrx_config.h>
#include <ndrstandard.h>
#include <exdb.h>

#include "ferror.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
    
#define NDRX_UBFDB_BERROR(atmierr, fmt, ...)\
        UBF_LOG(log_error, fmt, ##__VA_ARGS__);\
        userlog(fmt, ##__VA_ARGS__);\
        ndrx_Bset_error_fmt(atmierr, fmt, ##__VA_ARGS__);

#define NDRX_UBFDB_ERROR(fmt, ...)\
        UBF_LOG(log_error, fmt, ##__VA_ARGS__);\
        userlog(fmt, ##__VA_ARGS__);
    
    
#define NDRX_UBFDB_BERRORNOU(errlev, atmierr, fmt, ...)\
        UBF_LOG(errlev, fmt, ##__VA_ARGS__);\
        ndrx_Bset_error_fmt(atmierr, fmt, ##__VA_ARGS__);
    
#define NDRX_UBFDB_MAGIC        0xf19c5da3

#define NDRX_UBFDB_MAX_READERS_DFLT 1000
#define NDRX_UBFDB_MAP_SIZE_DFLT    512000 /* 500K */
#define NDRX_UBFDB_PERMS_DFLT       0664

/* KWD -> keywords for database */
#define NDRX_UBFDB_KWD_MAX_READERS            "max_readers"
#define NDRX_UBFDB_KWD_MAP_SIZE               "map_size"
#define NDRX_UBFDB_KWD_RESOURCE               "resource"
#define NDRX_UBFDB_KWD_PERMS                  "perms"
    
    
/**
 * Dump the UBF database configuration
 */
#define NDRX_UBFDB_DUMPCFG(LEV, UBFDB)\
    NDRX_LOG(LEV, "============== UBF FIELD TABLE DB ===============");\
    NDRX_LOG(LEV, "%s=[%s]", NDRX_UBFDB_KWD_RESOURCE, UBFDB->resource);\
    NDRX_LOG(LEV, "%s=[%ld]", NDRX_UBFDB_KWD_MAX_READERS, UBFDB->max_readers);\
    NDRX_LOG(LEV, "%s=[%ld]", NDRX_UBFDB_KWD_MAP_SIZE, UBFDB->map_size);\
    NDRX_LOG(LEV, "%s=[%o]", NDRX_UBFDB_KWD_PERMS, UBFDB->perms);\
    NDRX_LOG(LEV, "=================================================");
    
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
    
/**
 * UBF dynamic field table database
 */
typedef struct ndrx_ubf_db ndrx_ubf_db_t;
struct ndrx_ubf_db
{
    char resource[PATH_MAX+1];  /* physical path of the cache folder        */
    long max_readers;           /* db settings                              */
    long map_size;              /* db settings                              */
    int perms;                  /* permissions of the database resource     */
    
    /* LMDB Related */
    EDB_env *env;               /* env handler                              */
    EDB_dbi dbi_id;             /* named id mapping db                      */
    EDB_dbi dbi_nm;             /* named field name database                */
};

/**
 * field definition found in .fd files
 */
typedef struct ndrx_ubfdb_entry ndrx_ubfdb_entry_t;
struct  ndrx_ubfdb_entry
{
    BFLDID bfldid;
    char fldname[UBFFLDMAX+1];
};

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

extern NDRX_API ndrx_ubf_db_t *ndrx_G_ubf_db;
extern NDRX_API int ndrx_G_ubf_db_triedload; /* did we try to load? */
        
/*---------------------------Prototypes---------------------------------*/
extern int ndrx_ubfdb_Bflddbload(void);
extern int ndrx_ubfdb_Bflddbadd(EDB_txn *txn, BFLDID bfldno, 
        short fldtype, char *fldname);
extern int ndrx_ubfdb_Bflddbdel(EDB_txn *txn, BFLDID bfldid);
extern int ndrx_ubfdb_Bflddbdrop(EDB_txn *txn);
extern void ndrx_ubfdb_Bflddbunload(void);
extern int ndrx_ubfdb_Bflddbunlink(void);
extern int ndrx_ubfdb_Bflddbget(EDB_val *key, EDB_val *data,
        BFLDID *p_bfldno, BFLDID *p_bfldid, 
        short *p_fldtype, char *fldname, int fldname_bufsz);
extern char * ndrx_ubfdb_Bflddbname (BFLDID bfldid);
extern BFLDID ndrx_ubfdb_Bflddbid (char *fldname);
extern EDB_env * ndrx_ubfdb_Bfldddbgetenv (EDB_dbi **dbi_id, EDB_dbi **dbi_nm);

#ifdef	__cplusplus
}
#endif

#endif	/* __UBFDB_H */
