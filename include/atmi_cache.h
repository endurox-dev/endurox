/* 
** ATMI cache strctures
**
** @file atmi_cache.h
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

#ifndef ATMI_CACHE_H
#define	ATMI_CACHE_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <cconfig.h>
#include <atmi.h>
#include <atmi_int.h>
#include <exhash.h>
#include <exdb.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define NDRX_TPCACHE_FLAGS_EXPIRY    0x00000001   /* Cache recoreds expires after add */
#define NDRX_TPCACHE_FLAGS_LRU       0x00000002   /* limited, last recently used stays*/
#define NDRX_TPCACHE_FLAGS_HITS      0x00000004   /* limited, more hits, longer stay  */
#define NDRX_TPCACHE_FLAGS_FIFO      0x00000008   /* First in, first out cache        */
#define NDRX_TPCACHE_FLAGS_BOOTRST   0x00000010   /* reset cache on boot              */
    
    
#define NDRX_TPCACH_INIT_NORMAL     0             /* Normal init (client & server)    */
#define NDRX_TPCACH_INIT_BOOT       1             /* Boot mode init (ndrxd startst)   */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/**
 * Cache database 
 */
struct ndrx_tpcache_db
{
    char cachedbnm[NDRX_CCTAG_MAX]; /* cache db logical name (subsect of @cachedb)  */
    char resource[PATH_MAX+1];  /* physical path of the cache folder                */
    long limit;                 /* number of records limited for cache used by 2,3,4*/
    long flags;                 /* configuration flags for this cache               */
    long max_readers;           /* db settings? */
    long map_size;              /* db settings? */
    
    /* LMDB Related */
    
    EDB_env *env; /* env handler */
    
#if 0
    /*
     see test_nstd_mtest.c
     * 
     * 
     * Prepare the env:
                E(edb_env_create(&env));
                E(edb_env_set_maxreaders(env, 1));
                E(edb_env_set_mapsize(env, 10485760));
                E(edb_env_open(env, "./testdb", EDB_FIXEDMAP /*|EDB_NOSYNC*/, 0664));
     */
#endif
    
    
    /* Make structure hashable: */
    EX_hash_handle hh;
};
typedef struct ndrx_tpcache_db ndrx_tpcache_db_t;

/**
 * cache entry, this is linked list as 
 */
typedef struct ndrx_tpcallcache ndrx_tpcallcache_t;
struct ndrx_tpcallcache
{
    char cachedbnm[NDRX_CCTAG_MAX+1]; /* cache db logical name (subsect of @cachedb)  */
    char keyfmt[PATH_MAX+1];
    char save[PATH_MAX+1];
    char rule[PATH_MAX+1];
    typed_buffer_descr_t *buf_type;
    
    /* optional return code expression 
     * In case if missing, only TPSUCCESS messages are saved.
     * If expression is set, we will load the tperrno() and tpurcode() in the
     * following UBF fields:
     * EX_TPERRNO and TPURCODE. Then the user might evalue the value to decide
     * keep the values or not.
     * 
     * The tperrno and tpurcode must be smulated when stored in cache db.
     */
    char errfmt[PATH_MAX/2];
    
    /* this is linked list of caches */
    ndrx_tpcallcache_t *next, *prev;
};

/**
 * This is hash of services which are cached.
 */
struct ndrx_tpcache_svc
{
    char svcnm[MAXTIDENT+1]; /* cache db logical name (subsect of @cachedb)  */
    
    ndrx_tpcallcache_t *caches; /* This list list of caches */
        
    /* Make structure hashable: */
    EX_hash_handle hh;
};
typedef struct ndrx_tpcache_svc ndrx_tpcache_svc_t;


/**
 * Structure for holding data up
 */
struct ndrx_tpcache_data
{
    int atmi_error;
    long saved_tpurcode;
    char atmi_buf[0]; /* the data follows (th */
};

/*
 * NOTE: Key is used directly as binary data and length 
 */

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

extern NDRX_API int ndrx_cache_init(int mode);

#ifdef	__cplusplus
}
#endif

#endif	/* ATMI_CACHE_H */

