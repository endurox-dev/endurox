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
 * Add update keygroup. Only intrersting question is how about duplicate
 * keys? If it is duplciate, then key group does not change. So there shall be
 * no problems.
 */
expublic int ndrx_cache_keygrp_addupd(ndrx_tpcallcache_t *cache, 
            char *idata, long ilen, char *cachekey)
{
    int ret = EXSUCCEED;
    /* The storage will be standard UBF buffer
     * with some extra fields, like dbname from which data is stored here...
     * EX_CACHE_DBNAME - the database of the group
     * EX_CACHE_OPEXPR - this will be mulitple occurrences with linked pages
     */

    /* 1. build the key (keygrp) */
    
    /* 2. Find record in their db */
    
    /* 2.1 check all occurrenences of EX_CACHE_OPEXPR (we could use Bnext
     * for fast data checking...), if key is found, then we are done, return. */
    
    /* 3. if not found, allocate new buffer. Add  EX_CACHE_DBNAME */
    
    /* 4. ensure that in buffer we have cachekey bytes + 1024 */
    
    /* 5. add key to the buffer */
    
    /* 6. save record to the DB.  */
    
out:
            
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


