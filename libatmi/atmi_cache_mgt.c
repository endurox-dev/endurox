/* 
** ATMI cache management routines
**
** @file atmi_cache_mgt.c
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
#include <ubfutil.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define OFSZ(s,e)   EXOFFSET(s,e), EXELEM_SIZE(s,e)
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/


/**
 * Cache data mapping structure to UBF buffer
 */
static ubf_c_map_t M_cachedata_map[] = 
{
    {EX_CACHE_TPERRNO,  0, OFSZ(ndrx_tpcache_data_t, saved_tperrno),  BFLD_INT}, /* 0 */
    {EX_CACHE_TPRUCODE, 0, OFSZ(ndrx_tpcache_data_t, saved_tpurcode), BFLD_LONG}, /* 1 */
    {EX_CACHE_TIM,      0, OFSZ(ndrx_tpcache_data_t, t),              BFLD_LONG}, /* 2 */
    {EX_CACHE_TIMUSEC,  0, OFSZ(ndrx_tpcache_data_t, tusec),          BFLD_LONG}, /* 3 */
    {EX_CACHE_HITT,     0, OFSZ(ndrx_tpcache_data_t, hit_t),          BFLD_LONG}, /* 4 */
    {EX_CACHE_HITTU,    0, OFSZ(ndrx_tpcache_data_t, hit_tusec),      BFLD_LONG}, /* 5 */
    {EX_CACHE_HITS,     0, OFSZ(ndrx_tpcache_data_t, hits),           BFLD_LONG}, /* 6 */
    {EX_CACHE_NODEID,   0, OFSZ(ndrx_tpcache_data_t, nodeid),         BFLD_SHORT}, /* 7 */
    {EX_CACHE_BUFTYP,   0, OFSZ(ndrx_tpcache_data_t, atmi_type_id),   BFLD_SHORT}, /* 8 */
    {EX_CACHE_BUFLEN,   0, OFSZ(ndrx_tpcache_data_t, atmi_buf_len),   BFLD_LONG}, /* 9 */    
    {BBADFLDID}
};

/**
 * Cache data exchange optionality - all required.
 */
static long M_cachedata_req[] = 
{
    UBFUTIL_EXPORT,/* 0 */
    UBFUTIL_EXPORT,/* 1 */
    UBFUTIL_EXPORT,/* 2 */
    UBFUTIL_EXPORT,/* 3 */
    UBFUTIL_EXPORT,/* 4 */
    UBFUTIL_EXPORT,/* 5 */
    UBFUTIL_EXPORT,/* 6 */
    UBFUTIL_EXPORT,/* 7 */
    UBFUTIL_EXPORT,/* 8 */
    UBFUTIL_EXPORT /* 9 */
};

/*---------------------------Prototypes---------------------------------*/

/**
 * Return Management service name
 * @return static char buffer
 */
expublic char* ndrx_cache_mgt_getsvc(void)
{
    static char svcnm[XATMI_SERVICE_NAME_LENGTH+1];
    
    snprintf(svcnm, sizeof(svcnm), NDRX_CACHE_MGSVC, tpgetnodeid());
    
    return svcnm;
}

/**
 * Convert data to UBF
 * @param cdata data to convert
 * @param pp_ub double ptr to UBF, will be reallocated to get some space
 * @param incl_blob if !=0, then data will be loaded to UBF
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_cache_mgt_data2ubf(ndrx_tpcache_data_t *cdata, char *keydata,
        UBFH **pp_ub, int incl_blob)
{
    int ret = EXSUCCEED;
    int new_size;
    
    /* Remove any existing fields (may be left from previous run) */
    Bdel(*pp_ub, EX_CACHE_TPERRNO, 0);
    Bdel(*pp_ub, EX_CACHE_TPRUCODE, 0);
    Bdel(*pp_ub, EX_CACHE_TIM, 0);
    Bdel(*pp_ub, EX_CACHE_TIMUSEC, 0);
    Bdel(*pp_ub, EX_CACHE_HITT, 0);
    Bdel(*pp_ub, EX_CACHE_TIMUSEC, 0);
    Bdel(*pp_ub, EX_CACHE_HITS, 0);
    Bdel(*pp_ub, EX_CACHE_NODEID, 0);
    Bdel(*pp_ub, EX_CACHE_BUFTYP, 0);
    
    /* Get some 1K free */
    
    new_size=Bused(*pp_ub) + strlen(keydata) + 1024;
    
    *pp_ub = (UBFH *)tprealloc((char *)*pp_ub, new_size);
    
    if (NULL==*pp_ub)
    {
        NDRX_LOG(log_error, "Failed to reallocate new buffer size: %ld", new_size);
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_debug, "tusec=%ld", cdata->tusec);
    
    if (EXSUCCEED!=(ret=atmi_cvt_c_to_ubf(M_cachedata_map, cdata, 
            *pp_ub, M_cachedata_req)))
    {
        NDRX_LOG(log_error, "%s: failed to convert data to UBF", __func__);
        NDRX_TPCACHETPCALL_DBDATA(log_debug, cdata);
        
        EXFAIL_OUT(ret);
    }
    
    /* Load they key */
    
    if (EXSUCCEED!=Bchg(*pp_ub, EX_CACHE_OPEXPR, 0, (char *)keydata, 0L))
    {
        NDRX_LOG(log_error, "Failed to set EX_CACHE_OPEXPR field: %s", 
                    Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    /* if putting blob, then even more we need. */
    
    if (incl_blob)
    {
        /* Have a bit over for TLV. */
        new_size=Bused(*pp_ub)+cdata->atmi_buf_len+256;

        *pp_ub = (UBFH *)tprealloc((char *)*pp_ub, new_size);

        if (NULL==*pp_ub)
        {
            NDRX_LOG(log_error, "Failed to reallocate new buffer size: %ld", 
                    new_size);
            EXFAIL_OUT(ret);
        }
        
        if (EXSUCCEED!=Bchg(*pp_ub, EX_CACHE_DUMP, 0, cdata->atmi_buf, 
                cdata->atmi_buf_len))
        {
            NDRX_LOG(log_error, "Failed to set EX_CACHE_DUMP field: %s", 
                    Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
    }
    
out:
    return ret;
}


/**
 * Convert UBF to data structure. The blob will be loaded separately in allocated
 * space.
 * @param pp_ub UBF will EX_CACHE* fields
 * @param cdata output struct where to load the incoming data
 * @param data ptr to space where to allocate the incoming blob (if present in UBF)
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_cache_mgt_ubf2data(UBFH *p_ub, ndrx_tpcache_data_t *cdata, 
        char **blob, char **keydata)
{
    int ret = EXSUCCEED;
    BFLDLEN len;
    
    if (EXSUCCEED!=atmi_cvt_ubf_to_c(M_cachedata_map, p_ub, cdata, M_cachedata_req))
    {
        NDRX_LOG(log_error, "Failed to convert ubf to tpcache_data");
        EXFAIL_OUT(ret);
    }
    
    /* Load the blob if present and data is set */
    
    if (NULL!=blob)
    {
        if (0>(len = Blen(p_ub, EX_CACHE_DUMP, 0)))
        {
            NDRX_LOG(log_error, "Failed to estimate EX_CACHE_DUMP size: %s", 
                    Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
        
        NDRX_MALLOC_OUT(*blob, len, char);
        
        /* hmm we need to get some statistics about the field */
        if (EXSUCCEED!=Bget(p_ub, EX_CACHE_DUMP, 0, *blob, &len))
        {
            NDRX_LOG(log_error, "Failed to get cache data: %s", Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
        
        if (cdata->atmi_buf_len != len)
        {
            NDRX_LOG(log_error, "ERROR ! real data len: %d, but "
                    "EX_CACHE_BUFLEN says: %ld",
                    len, cdata->atmi_buf_len);
            EXFAIL_OUT(ret);
        }
    }
    
    if (NULL!=keydata)
    {
        if (0>(len = Blen(p_ub, EX_CACHE_OPEXPR, 0)))
        {
            NDRX_LOG(log_error, "Failed to estimate EX_CACHE_OPEXPR size: %s", 
                    Bstrerror(Berror));
        }
        
        NDRX_MALLOC_OUT(*keydata, len, char);
        
        if (EXSUCCEED!=Bget(p_ub, EX_CACHE_OPEXPR, 0, *keydata, &len))
        {
            NDRX_LOG(log_error, "Failed to get key data: %s", Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
    }
    
out:

    if (EXSUCCEED!=ret)
    {
        if (NULL!=blob && *blob!=NULL)
        {
            NDRX_FREE(*blob);
            *keydata=NULL;
        }
        
        if (NULL!=keydata && *keydata!=NULL)
        {
            NDRX_FREE(*keydata);
            *keydata=NULL;
        }
    }
    return ret;
}
