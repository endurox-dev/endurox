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
 * Return key data (read field from UBF...).
 * This will read the field from buffer. We detect OK, field not found or error!
 * 
 * All fields are processed as zero terminated strings. !!!
 * 
 * @param data1 ptr to UBF buffer
 * @param data2 error detail buffer
 * @param data3 err det buffer size
 * @param data4 NULL
 * @param symbol filed to lookup
 * @param outbuf output buffer
 * @param outbufsz output buffer size
 * @return EXSUCCEED/EXFAIL (syserr)/NDRX_TPCACHE_ENOKEYDATA (cannot build key)
 */
exprivate int get_key_data (void *data1, void *data2, void *data3, void *data4,
        char *symbol, char *outbuf, long outbufsz)
{
    int ret = EXSUCCEED;
    BFLDID fid;
    int *p_errdetbufsz = (int *)data3;
    UBFH *p_ub = (UBFH *)data1;
    char tmpsymbol[BF_LENGTH+2+5+1]; /* +2 [] + NNNNN (occ) + EOS */
    char *p_start_sq;
    char *p_stop_sq;
    char tmp[256];
    BFLDLEN len = (BFLDLEN)outbufsz;
    BFLDOCC occ = 0;
    NDRX_STRCPY_SAFE(tmpsymbol, symbol);
    
    
    if (NULL!=(p_start_sq = strchr(tmpsymbol, '[')))
    {
        p_stop_sq = strchr(tmpsymbol, ']');
        
        if (NULL==p_stop_sq)
        {
            NDRX_LOG(log_error, "Invalid field id (%s): cannot "
                    "find closing bracket ']'", tmpsymbol);
            snprintf(tmp, sizeof(tmp), "Invalid field id (%s): cannot "
                    "find closing bracket ']'", tmpsymbol);
            NDRX_STRNCPY_SAFE(((char *)data2), tmp, *p_errdetbufsz);
            EXFAIL_OUT(ret);
        }
        
        if (p_start_sq >= p_stop_sq)
        {
            NDRX_LOG(log_error, "Invalid/empty field (%s) brackets", 
                    tmpsymbol);
            snprintf(tmp, sizeof(tmp), "Invalid/empty field (%s) brackets", 
                    tmpsymbol);
            NDRX_STRNCPY_SAFE(((char *)data2), tmp, *p_errdetbufsz);
            EXFAIL_OUT(ret);
        }
        
        *p_start_sq = EXEOS;
        *p_stop_sq = EXEOS;
        
        p_start_sq++;
        
#ifdef NDRX_TPCACHE_DEBUG
        NDRX_LOG(log_debug, "Converting to occurrence: [%s]", p_start_sq);
#endif
        
        occ = atoi(p_start_sq);
        
    }
    /* resolve field id */
    if (EXSUCCEED!=(fid = Bfldid(tmpsymbol)))
    {
        NDRX_LOG(log_error, "Failed to resolve field [%s] id: %s", 
                tmpsymbol, Bstrerror(Berror));
        NDRX_STRNCPY_SAFE(((char *)data2), Bstrerror(Berror), *p_errdetbufsz);
        EXFAIL_OUT(ret);
    }

#ifdef NDRX_TPCACHE_DEBUG
    NDRX_LOG(log_debug, "Reading occurrence: %d", occ);
#endif
    
    /* we shall extract index if any within a key */    
    
    if (EXSUCCEED!=CBget(p_ub, fid, occ, outbuf, &len, BFLD_STRING))
    {
        
#ifdef NDRX_TPCACHE_DEBUG
        NDRX_LOG(log_debug, "Failed to get field %d[%d]: %s", 
                fid, occ, Bstrerror(Berror));
#endif
        if (BNOTPRES==Berror)
        {
            ret = NDRX_TPCACHE_ENOKEYDATA;
        }
        else
        {
            ret = EXFAIL;
        }
        goto out;
    }
    
    NDRX_LOG(log_error, "Field (%s) extracted: [%s]", symbol, outbuf);
    
out:

    return ret;    
}

/**
 * Get key for UBF typed buffer
 * @param idata input buffer
 * @param ilen input buffer len
 * @param okey output key
 * @param okey_bufsz output key buffer size
 * @return EXSUCCEED/EXFAIL (syserr)/NDRX_TPCACHE_ENOKEYDATA (cannot build key)
 */
expublic int ndrx_tpcache_keyget_ubf (ndrx_tpcallcache_t *cache, 
        char *idata, long ilen, char *okey, int okey_bufsz, 
        char *errdet, int errdetbufsz)
{
    int ret = EXSUCCEED;
    
    if (EXSUCCEED!=(ret=ndrx_str_subs_context(okey, okey_bufsz, '(', ')',
        (void *)idata, errdet, &errdetbufsz, NULL, get_key_data)))
    {
        EXFAIL_OUT(ret);
    }
    
out:
    return ret;
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

/**
 * We receive data from cache
 * @param exdata database data
 * @param idata incoming xatmi data buffer
 * @param ilen incoming xatmi data len
 * @param odata output data buffer
 * @param olen output len
 * @param flags flags
 * @return EXSUCCED/EXFAIL
 */
expublic int ndrx_cache_get_ubf (ndrx_tpcache_data_t *exdata, 
        typed_buffer_descr_t *buf_type, char *idata, long ilen, 
        char **odata, long *olen, long flags)
{
    int ret = EXSUCCEED;
    
    if (EXSUCCEED!=(ret = buf_type->pf_prepare_incoming(buf_type, exdata->atmi_buf, 
            exdata->atmi_buf_len, odata, olen, flags)))
    {
        /* the error shall be set already */
        NDRX_LOG(log_error, "Failed to prepare data from cache to buffer");
    }
    
out:
    return ret;
}

/**
 * Prepare data for saving to UBF buffer
 * At this stage we have to filter 
 * @param exdata
 * @param descr
 * @param idata
 * @param ilen
 * @param flags
 * @return 
 */
expublic int ndrx_cache_put_ubf (ndrx_tpcache_data_t *exdata, 
        typed_buffer_descr_t *descr, char *idata, long ilen, long flags)
{
    /* Figure out the  */
}


/**
 * Process flags
 * @param cache
 * @param errdet
 * @param errdetbufsz
 * @return 
 */
expublic int ndrx_process_flags_ubf(ndrx_tpcallcache_t *cache, char *errdet, int errdetbufsz)
{
    /* TODO: Process some addtional rules
     * - If no save strategy is given, then '*' means full buffer
     * - If '*' is not found, then build a project copy list
     */
    
}

/**
 * Free up internal resources
 * @param cache
 * @return 
 */
expublic int ndrx_cache_delete_ubf(ndrx_tpcallcache_t *cache)
{
    if (NULL!=cache->rule_tree)
    {
        Btreefree(cache->rule_tree);
    }
    
    return EXSUCCEED;
}
