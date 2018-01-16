/* 
** UBF Support for ATMI cache
**
** @file atmi_cache.c
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
#include <exparson.h>
#include <atmi_cache.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Get key for UBF typed buffer
 * @param idata input buffer
 * @param ilen input buffer len
 * @param okey output key
 * @param okey_bufsz output key buffer size
 * @return EXSUCCEED/EXFAIL (syserr) /NDRX_TPCACHE_ENOKEYDATA (cannot build key)
 */
expublic int ndrx_tpcache_keyget_ubf (char *idata, long ilen, char *okey, char *okey_bufsz)
{
    /* TODO Build key */
}

/**
 * Compile the rule for checking is buffer applicable for cache or not
 * @param cache chache to process
 * @param errdet error detail out
 * @param errdetbufsz errro detail buffer size
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_tpcache_rulcomp_ubf (ndrx_tpcallcache_t *cache, 
        char *errdet, int errdetbufsz)
{
    int ret = EXSUCCEED;
    
    if (NULL==(cache->rule_tree=Bboolco (cache->rule)))
    {
        snprintf(errdet, errdetbufsz, "%s", Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
out:
    return ret;
}

/**
 * Evaluate the rule of cache. Do we need to cache this record or not?
 * @param cache cache on which to test
 * @param idata input buffer
 * @param ilen input buffer len (if applicable)
 * @param errdet error detail out
 * @param errdetbufsz error detail buffer size
 * @return EXFAIL/EXTRUE/EXFALSE
 */
expublic int ndrx_tpcache_ruleval_ubf (ndrx_tpcallcache_t *cache, 
        char *idata, long ilen,  char *errdet, int errdetbufsz)
{
    int ret = EXFALSE;
    
    if (EXFAIL==(ret=Bboolev((UBFH *)idata, cache->rule_tree)))
    {
        snprintf(errdet, errdetbufsz, "%s", Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
out:
    return ret;
}