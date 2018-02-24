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
#include "exregex.h"
#include <exparson.h>
#include <ubfutil.h>
#include <atmi_cache.h>

/*---------------------------Externs------------------------------------*/
#define NDRX_TPCACHE_MINLIST          100    /* Min number of elements */
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

exprivate int add_proj_field(char **arr, long *arrsz, int idx, BFLDID fid, 
        char *errdet, int errdetbufsz);

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
    
    /* extract occurrences by [] */
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
    if (BBADFLDID==(fid = Bfldid(tmpsymbol)))
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
expublic int ndrx_cache_keyget_ubf (ndrx_tpcallcache_t *cache, 
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
expublic int ndrx_cache_rulcomp_ubf (ndrx_tpcallcache_t *cache, 
        char *errdet, int errdetbufsz)
{
    int ret = EXSUCCEED;
    
    if (EXEOS!=cache->rule[0])
    {
        if (NULL==(cache->rule_tree=Bboolco (cache->rule)))
        {
            snprintf(errdet, errdetbufsz, "%s", Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
    }
    
    /* Compile refresh rule too */
    
    if (EXEOS!=cache->refreshrule[0])
    {
        if (NULL==(cache->refreshrule_tree=Bboolco (cache->refreshrule)))
        {
            snprintf(errdet, errdetbufsz, "%s", Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
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
expublic int ndrx_cache_ruleval_ubf (ndrx_tpcallcache_t *cache, 
        char *idata, long ilen,  char *errdet, int errdetbufsz)
{
    int ret = EXFALSE;
    
    if (EXEOS==cache->rule[0])
    {
        ret=EXTRUE;
    }
    else if (EXFAIL==(ret=Bboolev((UBFH *)idata, cache->rule_tree)))
    {
        snprintf(errdet, errdetbufsz, "%s", Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
out:
    return ret;
}

/**
 * Refresh rule evaluate
 * @param cache cache on which to test
 * @param idata input buffer
 * @param ilen input buffer len (if applicable)
 * @param errdet error detail out
 * @param errdetbufsz error detail buffer size
 * @return EXFAIL/EXTRUE/EXFALSE
 */
expublic int ndrx_cache_refeval_ubf (ndrx_tpcallcache_t *cache, 
        char *idata, long ilen,  char *errdet, int errdetbufsz)
{
    int ret = EXFALSE;
    
    if (EXFAIL==(ret=Bboolev((UBFH *)idata, cache->refreshrule_tree)))
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
expublic int ndrx_cache_get_ubf (ndrx_tpcallcache_t *cache,
        ndrx_tpcache_data_t *exdata, typed_buffer_descr_t *buf_type, 
        char *idata, long ilen, char **odata, long *olen, long flags)
{
    int ret = EXSUCCEED;
    UBFH *p_ub;
    UBFH *p_ub_cache = NULL;
    long olen_merge;
    /* Figure out how data to replace, either full replace or merge */
    
    if (cache->flags & NDRX_TPCACHE_TPCF_REPL)
    {
        if (EXSUCCEED!=buf_type->pf_prepare_incoming(buf_type, exdata->atmi_buf, 
                exdata->atmi_buf_len, odata, olen, flags))
        {
            /* the error shall be set already */
            NDRX_LOG(log_error, "Failed to prepare data from cache to buffer");
            EXFAIL_OUT(ret);
        }
    }
    else if (cache->flags & NDRX_TPCACHE_TPCF_MERGE)
    {
        /* So assume data we have normal data buffer in cache
         * and we just run update */
        
        p_ub = (UBFH *)idata;
        
        if (NULL==(p_ub_cache = (UBFH *)tpalloc("UBF", NULL, 1024)))
        {
            NDRX_CACHE_ERROR("Failed to realloc input buffer %p to size: %ld: %s", 
                    idata, *olen, tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
        
        /* we cannot do this way, because stored format is different than UBF... */
        
        if (EXSUCCEED!=buf_type->pf_prepare_incoming(buf_type, exdata->atmi_buf, 
                exdata->atmi_buf_len, (char **)&p_ub_cache, &olen_merge, flags))
        {
            /* the error shall be set already */
            NDRX_LOG(log_error, "Failed to prepare data from cache to buffer");
            EXFAIL_OUT(ret);
        }
        
        /* reallocate place in output buffer */
        
        *olen = Bsizeof(p_ub) + exdata->atmi_buf_len + 1024;
        if (NULL==(*odata = tprealloc(idata, *olen)))
        {
            /* tperror will be set already */
            
            NDRX_CACHE_ERROR("Failed to realloc input buffer %p to size: %ld: %s", 
                    idata, *olen, tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
        
        p_ub = (UBFH *)*odata;
        
#ifdef NDRX_TPCACHE_DEBUG
        ndrx_debug_dump_UBF(log_debug, "Updating output with", p_ub_cache);
#endif
        if (EXSUCCEED!=Bupdate(p_ub, p_ub_cache))
        {
            NDRX_CACHE_TPERROR(TPESYSTEM, 
                            "Failed to update/merge buffer: %s", 
                    Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
    }
    else
    {
        NDRX_CACHE_TPERROR(TPEINVAL, 
                        "Invalid buffer get mode: flags %ld", 
                cache->flags);
        EXFAIL_OUT(ret);
    }
    
out:

    if (NULL!=p_ub_cache)
    {
        tpfree((char *)p_ub_cache);
    }

    return ret;
}

/**
 * Prepare UBF buffer projection (for pur or delete)
 * @param cache cache descriptor
 * @param pb projection descriptor
 * @param p_ub_in UBF buffer in
 * @param p_ub_out UBF buffer out (if addr not changed, then same as in)
 * @param flags_projreg flag of NDRX_TPCACHE_TPCF_*REG
 * @param flags_projfull flag of NDRX_TPCACHE_TPCF_*FULL
 * @param flags_projsetof flag of NDRX_TPCACHE_TPCF_*SETOF
 * @return EXSUCCED/EXFAIL (tp error set)
 */
expublic int ndrx_cache_prepproj_ubf (ndrx_tpcallcache_t *cache, 
        ndrx_tpcache_projbuf_t *pb,
        UBFH *p_ub_in, UBFH **p_ub_out,
        long flags_projreg, long flags_projfull, long flags_projsetof)
{
    int ret = EXSUCCEED;
    char *list = NULL;
    long list_len = 0;
    BFLDID fid;
    BFLDOCC occ;
    int idx = 0;
    char errdet[MAX_TP_ERROR_LEN+1];
    /* Figure out what to put */
    
    if (cache->flags & flags_projreg)
    {
        NDRX_LOG(log_debug, "project buffer by regular expression, field by field");
        fid = BFIRSTFLDID;
        
        while(1==Bnext(p_ub_in, &fid, &occ, NULL, NULL))
        {
            if (0==occ)
            {
                /* Test the field against regex */
                char * nm = Bfname(fid);
                
                NDRX_LOG(log_debug, "REX testing [%s]", nm);
                if (EXSUCCEED==ndrx_regexec(&pb->regex, nm))
                {
                    NDRX_LOG(log_debug, "Testing [%s] - OK for projection", nm);
                    /* loop over, match regexp, if ok, add field to projection list */
                    if (EXSUCCEED!=add_proj_field(&list, &list_len, idx, fid, 
                            errdet, sizeof(errdet)))
                    {
                        NDRX_CACHE_TPERROR(TPESYSTEM, 
                            "Failed to add field to projection list: %s", 
                                errdet);
                        EXFAIL_OUT(ret);
                    }
                    idx++;
                }
            }
        } /* loop over the buffer */
        /* copy off the projection */
        
    }
    
    if (cache->flags & flags_projfull)
    {
        NDRX_LOG(log_debug, "Project full buffer");
        *p_ub_out = p_ub_in;
    }
    else if (cache->flags & flags_projsetof ||
            cache->flags & flags_projreg)
    {
        BFLDID * cpylist;
        
        *p_ub_out = (UBFH *)tpalloc("UBF", NULL, Bsizeof((UBFH *)p_ub_in));
        
        if (NULL==*p_ub_out)
        {
            NDRX_LOG(log_error, "Failed to alloc temp buffer!");
            userlog("Failed to alloc temp buffer: %s", tpstrerror(tperrno));
        }
        
        if (cache->flags & flags_projsetof)
        {
            NDRX_LOG(log_debug, "Projection set of");
            
            cpylist = (BFLDID *)pb->typpriv;
        }
        else
        {
            NDRX_LOG(log_debug, "Projection regexp");
            cpylist = (BFLDID *)list;
        }
        
        /* OK, we have to make a projection copy */
        
        if (EXSUCCEED!=Bprojcpy(*p_ub_out, (UBFH *)p_ub_in, cpylist))
        {
            NDRX_CACHE_TPERROR(TPESYSTEM, 
                "Projection copy failed for cache data: %s", Bstrerror(Berror));
            
            EXFAIL_OUT(ret);
        }
    }
    
out:

    if (NULL!=list)
    {
        NDRX_FREE(list);
    }

    return ret;
}

/**
 * Prepare delete buffer to project to 
 * @param cache cache
 * @param idata input data
 * @param ilen input data len (not used)
 * @param odata output data (double ptr) maybe same as input -> not allocated
 * @param olen not used
 * @return EXSUCCEED/EXFAIL (tperror set)
 */
expublic int ndrx_cache_del_ubf (ndrx_tpcallcache_t *cache, char *idata, long ilen,
        char **odata, long *olen)
{
    int ret = EXSUCCEED;
    
    if (EXSUCCEED!=ndrx_cache_prepproj_ubf (cache, &cache->saveproj,
        (UBFH *)idata, (UBFH **)odata,
            NDRX_TPCACHE_TPCF_DELREG, 
            NDRX_TPCACHE_TPCF_DELFULL, 
            NDRX_TPCACHE_TPCF_DELSETOF))
    {
        NDRX_LOG(log_error, "Failed to prepare outgoing buffer for delete call!");
        EXFAIL_OUT(ret);
    }
    
out:
    return ret;    
}

/**
 * Prepare data for saving to UBF buffer
 * At this stage we have to filter 
 * @param exdata node the len must be set to free space..
 * @param descr type description
 * @param idata input data
 * @param ilen input data len
 * @param flags flags used for prepare outgoing (from tpcall)
 * @return EXSUCEED/EXFAIL (tp error set)
 */
expublic int ndrx_cache_put_ubf (ndrx_tpcallcache_t *cache,
        ndrx_tpcache_data_t *exdata, typed_buffer_descr_t *descr, char *idata,
        long ilen, long flags)
{
    int ret = EXSUCCEED;
    UBFH *p_ub =(UBFH *)idata;
    char *buf_to_save;
    
    if (EXSUCCEED!=ndrx_cache_prepproj_ubf (cache, &cache->saveproj,
        (UBFH *)idata, (UBFH **)&buf_to_save, 
            NDRX_TPCACHE_TPCF_SAVEREG, 
            NDRX_TPCACHE_TPCF_SAVEFULL, 
            NDRX_TPCACHE_TPCF_SAVESETOF))
    {
        NDRX_LOG(log_error, "Failed to prepare buffer for save to cache!");
        EXFAIL_OUT(ret);
    }
    
    ndrx_debug_dump_UBF(log_debug, "Saving to cache", (UBFH *)buf_to_save);
    
    if (EXSUCCEED!=descr->pf_prepare_outgoing (descr, buf_to_save, 
                0, exdata->atmi_buf, &exdata->atmi_buf_len, flags))
    {
        NDRX_LOG(log_error, "Failed to prepare buffer for saving");
        userlog("Failed to prepare buffer for saving: %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
out:

    if (buf_to_save!=idata)
    {
        tpfree((char *)buf_to_save);
    }

    return ret;
}

/**
 * Store UBF field list in dynamic array
 * @param arr list to alloc
 * @param arsz array size
 * @param idx field index zero based
 * @param fid field id (compiled)
 * @param errdet error detail
 * @param errdetbufsz error detail buffer size
 * @return EXSUCCEED/EXFAIL
 */
exprivate int add_proj_field(char **arr, long *arrsz, int idx, BFLDID fid, 
        char *errdet, int errdetbufsz)
{
    int ret = EXSUCCEED;
    BFLDID *arri;
    if (NULL==*arr)
    {
        /* Allocate it... */
        
        *arrsz = NDRX_TPCACHE_MINLIST;
#ifdef NDRX_TPCACHE_DEBUG
        NDRX_LOG(log_debug, "About to alloc UBF list storage: %ld", 
                (*arrsz)*sizeof(BFLDID));
#endif
        *arr = NDRX_MALLOC((*arrsz)*sizeof(BFLDID));
        
        if (NULL==*arr)
        {
            int err = errno;
            NDRX_LOG(log_error, "%s: Failed to malloc %ld: %s", __func__, 
                    (*arrsz)*sizeof(BFLDID), strerror(err));
            snprintf(errdet, errdetbufsz, "%s: Failed to malloc %ld: %s", __func__, 
                    (*arrsz)*sizeof(BFLDID), strerror(err));
            EXFAIL_OUT(ret);        
        }
        
    }
    else if (*arrsz<idx+2) /* idx zero based, thus +1 and 1+ for BBADFLDID */
    {
        
        *arrsz = *arrsz + NDRX_TPCACHE_MINLIST;
#ifdef NDRX_TPCACHE_DEBUG
        NDRX_LOG(log_debug, "About to realloc UBF list storage: %ld", 
                (*arrsz)*sizeof(BFLDID));
#endif        
        *arr = NDRX_REALLOC(*arr, 
                (*arrsz)*sizeof(BFLDID));
        
        if (NULL==*arr)
        {
            int err = errno;
            NDRX_LOG(log_error, "%s: Failed to realloc (%ld): %s", __func__, 
                    (*arrsz)*sizeof(BFLDID), strerror(err));
            snprintf(errdet, errdetbufsz, "%s: Failed to malloc (%ld): %s", __func__, 
                    (*arrsz)*sizeof(BFLDID), strerror(err));
            EXFAIL_OUT(ret);        
        }
    }
    
    /* OK we are done... add at index */
    arri = (BFLDID *)(*arr);
    arri[idx] = fid;
    arri[idx+1] = BBADFLDID;
    
out:
    return ret;
}

/**
 * Process typed flags of the buffer projection
 * @param cache cache object
 * @param pb projection buffer
 * @param op operation (for debug - string)
 * @param flags_projreg flag constant for regex save/del
 * @param flags_projfull flag constant for proj full
 * @param flags_projsetof flag constant for save set of
 * @param errdet error detail return
 * @param errdetbufsz error buffer size
 * @return EXSUCCEED/EXFAIL (TP error not set ar error detail returned)
 */
exprivate int proc_flags_typed(ndrx_tpcallcache_t *cache, 
        ndrx_tpcache_projbuf_t *pb, char *op,
        long flags_projreg, long flags_projfull, long flags_projsetof,
        char *errdet, int errdetbufsz)
{
   int ret = EXSUCCEED;
    char *saveptr1 = NULL;
    char *p;
    char tmp[PATH_MAX+1];
    BFLDID fid;
    int idx = 0;
    
    /* Process some additional rules
     * - If no save strategy is given, then '*' means full buffer
     * - If '*' is not found, then build a project copy list
     */
    if (!(cache->flags & flags_projreg) && !(cache->flags & flags_projfull))
    {
        if (0==strcmp(pb->expression, "*") || EXEOS==pb->expression[0])
        {
#ifdef NDRX_TPCACHE_DEBUG
            NDRX_LOG(log_debug, "%s strategy defaulted to full UBF buffer", op);
#endif
            cache->flags |= flags_projfull;
        }
        else
        {
            cache->flags |= flags_projsetof;
#ifdef NDRX_TPCACHE_DEBUG
            NDRX_LOG(log_debug, "%s strategy: list of fields - parsing...", op);
#endif
            /* expression max = PATH_MAX on given platform */
            NDRX_STRCPY_SAFE(tmp, pb->expression);
            
            /* clean up save string... */            
            ndrx_str_strip(tmp, "\t ");
            
            /* Strtok... & build the list of projection copy fields */
            p = strtok_r (tmp, ",", &saveptr1);
            while (p != NULL)
            {
                /* Lookup the field id,  */
                
                NDRX_LOG(log_debug, "Got field [%s]", p);
                
                if (EXFAIL==(fid=Bfldid(p)))
                {
                    NDRX_LOG(log_error, "Failed to resolve filed id: [%s]: %s", 
                            p, Bstrerror(Berror));
                    snprintf(errdet, errdetbufsz, "Failed to resolve filed id: [%s]: %s", 
                            p, Bstrerror(Berror));
                    EXFAIL_OUT(ret);
                }
                
                if (EXSUCCEED!=add_proj_field((char **)&pb->typpriv, 
                            &pb->typpriv2, idx, fid, errdet, errdetbufsz))
                {
                    NDRX_LOG(log_error, "Failed to add field to projection list!");
                    EXFAIL_OUT(ret);
                }
                
                p = strtok_r (NULL, ",", &saveptr1);
                idx++;
            }
        }
    }
out:
    return ret;    
}

/**
 * Process flags. Here we prepare projection copy if needed.
 * @param cache cache on which to execute
 * @param errdet error detail
 * @param errdetbufsz error buffer size
 * @return EXSUCCEED/EXFAIL (no tperror)
 */
expublic int ndrx_cache_proc_flags_ubf(ndrx_tpcallcache_t *cache, 
        char *errdet, int errdetbufsz)
{
    int ret = EXSUCCEED;
    
    if (EXSUCCEED!=(ret = proc_flags_typed(cache, 
        &cache->saveproj, "save",
        NDRX_TPCACHE_TPCF_SAVEREG, 
        NDRX_TPCACHE_TPCF_SAVEFULL,
        NDRX_TPCACHE_TPCF_SAVESETOF,
        errdet, errdetbufsz)))
    {
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=(ret = proc_flags_typed(cache, 
        &cache->delproj, "delete",
        NDRX_TPCACHE_TPCF_DELREG, 
        NDRX_TPCACHE_TPCF_DELFULL,
        NDRX_TPCACHE_TPCF_DELSETOF,
        errdet, errdetbufsz)))
    {
        EXFAIL_OUT(ret);
    }
    
    
    /* Process reject buffer if any */
    
    if (NULL!=cache->keygroupmrej)
    {
        /* parse it. JSON2UBF */
        UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, strlen(cache->keygroupmrej)*3+1024);
        
        if (EXSUCCEED!=ndrx_tpjsontoubf(p_ub, cache->keygroupmrej))
        {
            snprintf(errdet, errdetbufsz, "%s: Failed to parse json: [%s]", 
                    __func__, cache->keygroupmrej);
            NDRX_LOG(log_error, errdet);
            EXFAIL_OUT(ret);
        }
        
        cache->keygroupmrej_abuf = (char *)p_ub;
        
    }
out:
    return ret;
}

/**
 * Free up internal resources
 * @param cache cache to free-up
 * @return EXSUCCEED
 */
expublic int ndrx_cache_delete_ubf(ndrx_tpcallcache_t *cache)
{
    if (NULL!=cache->rule_tree)
    {
        Btreefree(cache->rule_tree);
    }
    
    if (NULL!=cache->refreshrule_tree)
    {
        Btreefree(cache->refreshrule_tree);
    }
    
    if (NULL!=cache->saveproj.typpriv)
    {
        NDRX_FREE(cache->saveproj.typpriv);
    }
    
    if (NULL!=cache->delproj.typpriv)
    {
        NDRX_FREE(cache->delproj.typpriv);
    }
    
    return EXSUCCEED;
}

/**
 * Reject request when max reached
 * @param cache
 * @param idata
 * @param ilen
 * @param odata
 * @param olen
 * @param flags
 * @return 
 */
expublic int ndrx_cache_maxreject_ubf(ndrx_tpcallcache_t *cache, char *idata, long ilen, 
        char **odata, long *olen, long flags)
{
    return EXFAIL;
}
