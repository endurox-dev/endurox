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
#include <sys_mqueue.h>
#include <xa.h>
#include <atmi.h>
#include <atmi_int.h>
#include <cconfig.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define NDRX_TPCACHE_STRATEGY_NONE      0   /* records persist for ever         */
#define NDRX_TPCACHE_STRATEGY_EXPIRY    1   /* Cache recoreds expires after add */
#define NDRX_TPCACHE_STRATEGY_LRU       2   /* limited, last recently used stays*/
#define NDRX_TPCACHE_STRATEGY_HITS      3   /* limited, more hits, longer stay  */
#define NDRX_TPCACHE_STRATEGY_FIFO      4   /* First in, first out cache        */
    
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/**
 * Cache database hash
 */
struct ndrx_tpcache_db
{
    char cachedbnm[NDRX_CCTAG_MAX]; /* cache db logical name (subsect of @cachedb)  */
    char resource[PATH_MAX+1];  /* physical path of the cache, file or folder       */
    long expiry;                /* cache expiry milliseconds                        */
    int strategy;               /* cache strategy                                   */
    long limit;                 /* number of records limited for cache used by 2,3,4*/
};
typedef struct ndrx_tpcache_db ndrx_tpcache_db_t;

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
#ifdef	__cplusplus
}
#endif

#endif	/* ATMI_CACHE_H */

