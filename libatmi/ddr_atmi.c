/**
 * @brief Real time handling routines
 *
 * @file ddr_atmi.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2019, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * AGPL (with Java and Go exceptions) or Mavimax's license for commercial use.
 * See LICENSE file for full text.
 * -----------------------------------------------------------------------------
 * AGPL license:
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License, version 3 as published
 * by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU Affero General Public License, version 3
 * for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along 
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * -----------------------------------------------------------------------------
 * A commercial use license is available from Mavimax, Ltd
 * contact@mavimax.com
 * -----------------------------------------------------------------------------
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <math.h>
#include <ndrx_ddr.h>
#include <fcntl.h>

#include "atmi_int.h"
#include <lcfint.h>
#include <atmi_shm.h>
#include <typed_buf.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define NDRX_DDRV_SVC_INDEX(MEM, IDX) ((ndrx_services_t*)(((char*)MEM)+(int)(sizeof(ndrx_services_t)*IDX)))
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/** standard hashing stuff */
exprivate int ndrx_ddr_key_hash(ndrx_lh_config_t *conf, void *key_get, size_t key_len)
{
    return ndrx_hash_fn(key_get) % conf->elmmax;
}

/** standard hashing stuff */
exprivate void ndrx_ddr_key_debug(ndrx_lh_config_t *conf, void *key_get, size_t key_len, 
    char *dbg_out, size_t dbg_len)
{
    NDRX_STRCPY_SAFE_DST(dbg_out, key_get, dbg_len);
}

/** standard hashing stuff */
exprivate void ndrx_ddr_debug(ndrx_lh_config_t *conf, int idx, char *dbg_out, size_t dbg_len)
{
    snprintf(dbg_out, dbg_len, "%s", NDRX_DDRV_SVC_INDEX((*conf->memptr), idx)->svcnm);
    
}
/** standard hashing stuff */
exprivate int ndrx_ddr_compare(ndrx_lh_config_t *conf, void *key_get, size_t key_len, int idx)
{
    return strcmp(NDRX_DDRV_SVC_INDEX((*conf->memptr), idx)->svcnm, key_get);
}

/**
 * Resolve the free place where to install the service
 * @param svcnm service name to resolve
 * @param[out] pos position found suitable to succeed request
 * @param[out] have_value valid value is found? EXTRUE/EXFALSE.
 * @param mem memory segment with which we shall work
 * @param memmax max number of elements
 * @return EXTRUE -> found position/ EXFALSE - no position found
 */
exprivate int ndrx_ddr_position_get(char *svcnm, int oflag, int *pos, 
        int *have_value, char **mem, long memmax)
{
    ndrx_lh_config_t conf;
    
    conf.elmmax = memmax;
    conf.elmsz = sizeof(ndrx_services_t);
    conf.flags_offset = EXOFFSET(ndrx_services_t, flags);
    conf.memptr = (void **)mem;
    conf.p_key_hash=&ndrx_ddr_key_hash;
    conf.p_key_debug=&ndrx_ddr_key_debug;
    conf.p_val_debug=&ndrx_ddr_debug;
    conf.p_compare=&ndrx_ddr_compare;
    
    return ndrx_lh_position_get(&conf, svcnm, 0, oflag, pos, have_value, "ddrsvc");
}

/**
 * put service in the hashmap
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_ddr_services_put(ndrx_services_t *svc, char *mem, long memmax)
{
    int have_value=EXFALSE;
    int pos=0;
    int ret = EXSUCCEED;
    ndrx_services_t *ptr = (ndrx_services_t *)mem;
    
    /* get position */
    
    if (!ndrx_ddr_position_get(svc->svcnm, O_CREAT, &pos, &have_value, &mem, memmax))
    {
        NDRX_LOG(log_error, "Failed to get position for [%s] in LH", svc->svcnm);
        EXFAIL_OUT(ret);
    }
    
    /* put value down */
    memcpy(&ptr[pos], svc, sizeof(*svc));
    ptr[pos].flags=NDRX_LH_FLAG_ISUSED|NDRX_LH_FLAG_WASUSED;
    
out:
    return ret;
}

/** detect that unacceptable change has happened */
#define DDR_SHM_CHANGED ndrx_G_shmcfg->ddr_ver1!=ver1_ok && ndrx_G_shmcfg->ddr_ver1!=ver2_ok

/*
 * Detect that system is slow and we cannot detect the service settings
 */
#define DDR_SHM_VALIDATE \
    if (DDR_SHM_CHANGED) \
    {\
        NDRX_LOG(log_error, "Unable to get DDR data for [%s] service - increase <ddrreload> "\
                "time (accepted version: %u,%u current: %u)", \
                svcnm, ver1_ok, ver2_ok, ndrx_G_shmcfg->ddr_ver1);\
        userlog("Unable to get DDR data for [%s] service - increase <ddrreload> "\
                "time (accepted version: %u,%u current: %u)", \
                svcnm, ver1_ok, ver2_ok, ndrx_G_shmcfg->ddr_ver1);\
        ndrx_TPset_error_fmt(TPESYSTEM, "Unable to get DDR data for [%s] service - increase <ddrreload> "\
                "time (accepted version: %u,%u current: %u)", \
                svcnm, ver1_ok, ver2_ok, ndrx_G_shmcfg->ddr_ver1);\
        EXFAIL_OUT(ret);\
    }

/**
 * Get services entry.
 * Find the entry in current page of the DDR memory.
 * Thus DDR reloads shall be deferred for as long as possible time.
 * @param svcnm service name 
 * @param svc service descriptor
 * @return EXSUCCEED/EXFALSE/EXFAIL (tp error loaded)
 */
expublic int ndrx_ddr_services_get(char *svcnm, ndrx_services_t **svc)
{
    int ret = EXFALSE;
    int have_value=EXFALSE;
    int pos=0;
    int page;
    unsigned char ver1_ok;
    unsigned char ver2_ok;
    ndrx_services_t *ptr;
    
    /* ddr not used. */
    if (!ndrx_G_shmcfg->use_ddr)
    {
        /* no DDR settings here... */
        return EXFALSE;
    }
    
    ver1_ok = ndrx_G_shmcfg->ddr_ver1;
    ver2_ok = ndrx_G_shmcfg->ddr_ver1+1;

    page = ndrx_G_shmcfg->ddr_page;
    ptr = (ndrx_services_t *) (ndrx_G_routsvc.mem + page*G_atmi_env.rtsvcmax * sizeof(ndrx_services_t));
    
    if (EXTRUE==ndrx_ddr_position_get(svcnm, 0, &pos, 
        &have_value, (char **)&ptr, G_atmi_env.rtsvcmax) && have_value)
    {
        *svc = &ptr[pos];
        
        NDRX_LOG(log_debug, "Found service [%s] in ddr service table, autotran=%d", 
                (*svc)->svcnm, (*svc)->autotran);
        ret=EXTRUE;
    }

    DDR_SHM_VALIDATE;
    
    
out:
    return ret;
}

/**
 * Get routing group defined for service
 * Note the page shall not be changed while we work here.
 * Thus DDR reloads shall be deferred for as long as possible time.
 * If routing is found, svcnm will be updated
 * @param[in,out] svcnm service to route, will be updated 
 * @param[in,out] svcnmsz buffer size of service
 * @param[in] data data to send to service
 * @param[in] len dat len to send to service
 * @param[out] prio if service if found return the priority
 * @return EXFALSE/EXSUCCEED group not found (or default), EXTRUE group loaded, EXFAIL - error
 */
expublic int ndrx_ddr_grp_get(char *svcnm, size_t svcnmsz, char *data, long len,
        int *prio)
{
    int ret = EXSUCCEED;
    ndrx_services_t *svc;
    int page;
    ndrx_routcrit_t *ccrit;
    ndrx_routcritseq_t *range;
    int offset_step=0;
    int i;
    double floatval;
    long longval;
    char *strval=NULL;
    buffer_obj_t *buf;
    char *mem_start;
    int in_range=EXFALSE;
    BFLDID fldid;
    int  fieldtypeid;
    char fldnm[UBFFLDMAX+1];
    int len_new;
    char grp[NDRX_DDR_GRP_MAX+1];
    unsigned char ver1_ok;
    unsigned char ver2_ok;
    int is_default = EXFALSE;
    
    /* not attached, nothing to return */
    if (!ndrx_shm_is_attached(&ndrx_G_routsvc))
    {
        goto out;
    }

    /* DDR not used system wide */
    if (!ndrx_G_shmcfg->use_ddr)
    {
        goto out;
    }
    
    ver1_ok = ndrx_G_shmcfg->ddr_ver1;
    ver2_ok = ndrx_G_shmcfg->ddr_ver1+1;
    
    ret=ndrx_ddr_services_get(svcnm, &svc);
    
    if (EXTRUE!=ret)
    {
        if (EXFALSE==ret)
        {
            NDRX_LOG(log_debug, "No DDR service for [%s]", svcnm);
        }
        
        /* no group defined at all - either failure loaded or not found
         * and we finish off
         * the ret is configured
         */
        goto out;
    }
    
    ret = EXSUCCEED; /* start to search now... */
    
    /* return default service call priority */
    *prio = svc->prio;
    
    if (EXEOS==svc->criterion[0])
    {
        /* criterion not found */
        goto out;
    }
    
    /* loop over the criterions at the offset 
     * and match the first buffer type on which we go over the sequence of the
     * rnages
     * Get the offset of the criterion stuff...
     */
    buf = ndrx_find_buffer(data);
    if (NULL==buf)
    {
        NDRX_LOG(log_error, "Cannot route [%s] service - invalid data buffer",
                svcnm);
        userlog("Cannot route [%s] service - invalid data buffer",
                svcnm);
        ndrx_TPset_error_fmt(TPESYSTEM, "Cannot route [%s] service - invalid data buffer",
                svcnm);
        EXFAIL_OUT(ret);
    }
    
    /* OK start to run... */
    page = ndrx_G_shmcfg->ddr_page;
    mem_start = ndrx_G_routcrit.mem + page * G_atmi_env.rtcrtmax+svc->offset; 
    
    do
    {
        /* 
         * Check that we approach un-changed memory
         */
        DDR_SHM_VALIDATE;

        ccrit = (ndrx_routcrit_t *)(mem_start + offset_step);
        
        if (ccrit->criterionid!=svc->cirterionid)
        {
            /* no matching buffer - ok, this is finish in the search... 
             * generate error if memory has changed.
             */
            DDR_SHM_VALIDATE;
            goto out_rej;
        }
        
        /* check the types, shall match the current buffer */
        
        if (buf->type_id == ccrit->buffer_type_id)
        {
            /*  OK this is ours to check... */
            if (BUF_TYPE_UBF==buf->type_id)
            {
                fieldtypeid = ccrit->fieldtypeid;
                fldid = ccrit->fldid;
                NDRX_STRCPY_SAFE(fldnm, ccrit->field);
                
                /* Check that we are still valid here */
                DDR_SHM_VALIDATE;
                
                if (BFLD_LONG == fieldtypeid)
                {
                    if (EXSUCCEED!=CBget((UBFH *)data, fldid, 0, 
                            (char *)&longval, 0L, BFLD_LONG))
                    {
                        NDRX_LOG(log_error, "Cannot route [%s] - routing field not found [%s]: %s", 
                                svcnm, fldnm, Bstrerror(Berror));
                        userlog("Cannot route [%s] - routing field not found [%s]: %s", 
                                svcnm, fldnm, Bstrerror(Berror));
                        ndrx_TPset_error_fmt(TPESYSTEM, "Cannot route [%s] - routing field not found [%s]: %s", 
                                svcnm, fldnm, Bstrerror(Berror));
                        EXFAIL_OUT(ret);
                    }
                }
                else if (BFLD_DOUBLE == fieldtypeid)
                {
                    if (EXSUCCEED!=CBget((UBFH *)data, fldid, 0, 
                            (char *)&floatval, 0L, BFLD_DOUBLE))
                    {
                        NDRX_LOG(log_error, "Cannot route [%s] - routing field not found [%s]: %s", 
                                svcnm, fldnm, Bstrerror(Berror));
                        userlog("Cannot route [%s] - routing field not found [%s]: %s", 
                                svcnm, fldnm, Bstrerror(Berror));
                        ndrx_TPset_error_fmt(TPESYSTEM, "Cannot route [%s] - routing field not found [%s]: %s", 
                                svcnm, fldnm, Bstrerror(Berror));
                        EXFAIL_OUT(ret);
                    }
                }
                else
                {
                    /* alloc string copy with CF or to avoid allocations
                     * just use sysbuf page? as that size would gurantee that we
                     * fit in...
                     */
                    strval = Bgetsa ((UBFH *)data, fldid, 0, NULL);
                    
                    if (NULL==strval)
                    {
                        NDRX_LOG(log_error, "Cannot route [%s] - routing field not found [%s]: %s", 
                                svcnm, fldnm, Bstrerror(Berror));
                        userlog("Cannot route [%s] - routing field not found [%s]: %s", 
                                svcnm, fldnm, Bstrerror(Berror));
                        ndrx_TPset_error_fmt(TPESYSTEM, "Cannot route [%s] - routing field not found [%s]: %s", 
                                svcnm, fldnm, Bstrerror(Berror));
                        EXFAIL_OUT(ret);
                    }
                }
                
            }
            
            offset_step+=sizeof(ndrx_routcrit_t);
            
            for (i=0; i < ccrit->rangesnr; i++)
            {
                /* check that memory is not changed... */
                DDR_SHM_VALIDATE;
                 
                /* OK we got the range so loop over... */
                range = (ndrx_routcritseq_t *)(mem_start + offset_step);
                
                /* check that we are in boundries */
                if (range->flags & NDRX_DDR_FLAG_DEFAULT_VAL)
                {
                    /* ranges OK, return group */
                    in_range=EXTRUE;
                }
                else if (range->flags & NDRX_DDR_FLAG_MIN)
                {
                    /* so if bellow upper or equals then we are in the range */
                    if (BFLD_LONG == fieldtypeid)
                    {
                        if (longval <= range->upperl)
                        {
                            in_range=EXTRUE;
                        }
                    }
                    else if (BFLD_DOUBLE == fieldtypeid)
                    {    
                        if (floatval < range->upperd ||
                                fabs(floatval - range->upperd) < DOUBLE_EQUAL)
                        {
                            in_range=EXTRUE;
                        }
                    }
                    else if (BFLD_STRING == fieldtypeid)
                    {
                        if (strcmp(strval, range->strrange) <= 0)
                        {
                            in_range=EXTRUE;
                        }
                    }
                }
                else if (range->flags & NDRX_DDR_FLAG_MAX)
                {
                    /* so if bellow upper or equals then we are in the range */
                    if (BFLD_LONG == fieldtypeid)
                    {
                        if (longval >= range->lowerl)
                        {
                            in_range=EXTRUE;
                        }
                    }
                    else if (BFLD_DOUBLE == fieldtypeid)
                    {    
                        if (floatval > range->lowerd ||
                                fabs(floatval - range->lowerd) < DOUBLE_EQUAL)
                        {
                            in_range=EXTRUE;
                        }
                    }
                    else if (BFLD_STRING == fieldtypeid)
                    {
                        /* if mem changed it can match or not match
                         * but anyway, the routing blobs are terminated with
                         * EOS in last routing block
                         * or rangesnr is set to 0
                         * thus it will never overflow.
                         * Also prior setting the data, memset is performed.
                         */
                        if (strcmp(strval, range->strrange) >=0)
                        {
                            in_range=EXTRUE;
                        }
                    }
                }
                else
                {
                    /* full check not flags... */
                    if (BFLD_LONG == fieldtypeid)
                    {
                        if (longval >= range->lowerl &&
                                longval <= range->upperl)
                        {
                            in_range=EXTRUE;
                        }
                    }
                    else if (BFLD_DOUBLE == fieldtypeid)
                    {    
                        if ( (floatval > range->lowerd ||
                                fabs(floatval - range->lowerd) < DOUBLE_EQUAL) &&
                                
                                (floatval < range->upperd ||
                                fabs(floatval - range->upperd) < DOUBLE_EQUAL)
                                )
                        {
                            in_range=EXTRUE;
                        }
                    }
                    else if (BFLD_STRING == fieldtypeid)
                    {
                        if (strcmp(strval, range->strrange) >=0 &&
                                strcmp(strval, range->strrange+range->strrange_upper) <=0
                                )
                        {
                            in_range=EXTRUE;
                        }
                    }
                }
                
                if (in_range)
                {
                    ret=EXTRUE;
                    if (range->flags & NDRX_DDR_FLAG_DEFAULT_GRP)
                    {
                        NDRX_LOG(log_debug, "Default group matched");
                        is_default=EXTRUE;
                    }
                    else
                    {
                        /* copy off the group code */
                        NDRX_STRCPY_SAFE(grp, range->grp);
                        NDRX_LOG(log_debug, "Group [%s]  matched", grp);
                    }
                    break;
                }
                    
                /* step to next..., this is aligned step */
                offset_step+=range->len;
            }
        }
        
        /* check for next... */
    } while (svc->offset + offset_step + sizeof(ndrx_routcrit_t) < G_atmi_env.rtcrtmax && EXSUCCEED==ret);
    
    /* Validate that mem is not changed */
    DDR_SHM_VALIDATE;
    
    /* Route is found, if running default, no additional routing shall be done
     * Though one issue is that in cluster if sending the default
     * the bridge will attempt to route again...?
     */
    if (EXTRUE==ret && !is_default)
    {
        /* add the group suffx, also check the buffer space 
         * If there is not enough space, return the error.
         */
        len_new =  strlen(svcnm) + strlen(grp) + 2;
        
        if (len_new > svcnmsz)
        {
            NDRX_LOG(log_error, "Cannot route [%s] - routed service "
                    "name too long incl 0x00: %d (max: %d)",
                    svcnm, len_new, svcnmsz);
            userlog("Cannot route [%s] - routed service "
                    "name too long incl 0x00: %d (max: %d)",
                    svcnm, len_new, svcnmsz);
            ndrx_TPset_error_fmt(TPESYSTEM, "Cannot route [%s] - routed service "
                    "name too long incl 0x00: %d (max: %d)",
                    svcnm, len_new, svcnmsz);
            EXFAIL_OUT(ret);
        }
        NDRX_STRCAT_S(svcnm, svcnmsz, NDRX_SYS_SVC_PFX);
        NDRX_STRCAT_S(svcnm, svcnmsz, grp);    
    }
    
    
out_rej:
    
    /* terminate the request, if was routing, but route  or group / not found */
    if (EXSUCCEED==ret)
    {
        NDRX_LOG(log_error, "Routing criterion for service [%s] buffer type [%s] is not found",
                svcnm, G_buf_descr[buf->type_id].type);
        userlog("Routing criterion for service [%s] buffer type [%s] is not found",
                svcnm, G_buf_descr[buf->type_id].type);
        ndrx_TPset_error_fmt(TPESYSTEM, "Routing criterion for service [%s] buffer type [%s] is not found",
                svcnm, G_buf_descr[buf->type_id].type);
        ret=EXFAIL;
    }
    
    
out:
    
    if (NULL!=strval)
    {
        NDRX_FREE(strval);
    }

    NDRX_LOG(log_debug, "returns %d [%s]", ret, svcnm);
    
    return ret;
}

/**
 * Lookup the service
 * and provide settings
 * @param[in] svcnm service name to lookup
 * @param[out] autotran auto tran setting from services section
 * @param[out] trantime transaction timeout setting from services section
 * @return EXTRUE (loaded), EXFALSE (not found), EXFAIL (error!)
 */
expublic int ndrx_ddr_service_get(char *svcnm, int *autotran, unsigned long *trantime)
{
    ndrx_services_t *svc;
    int ret = EXFALSE;
    char svcnmtmp[XATMI_SERVICE_NAME_LENGTH+1];
    char *p;
    
    /* Check only part till the @, as we do not not care about the groups
     * here
     */
    NDRX_STRCPY_SAFE(svcnmtmp, svcnm);
    
    p = strchr(svcnmtmp, NDRX_SYS_SVC_PFXC);
    
    if (NULL!=p)
    {
        if (p==svcnmtmp)
        {
            /* no service */
            return EXFALSE;
        }
        
        /* terminate before the group */
        *p = EXEOS;
    }
    
    /* not attached, nothing to return */
    if (!ndrx_shm_is_attached(&ndrx_G_routsvc))
    {
        goto out;
    }
    
    /* not we might get EXFAIL here too.. */
    if (EXTRUE==(ret=ndrx_ddr_services_get(svcnm, &svc)))
    {
        *autotran = svc->autotran;
        *trantime = svc->trantime;
    }
    
out:
    return ret;
}


/* vim: set ts=4 sw=4 et smartindent: */
