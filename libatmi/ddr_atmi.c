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

/**
 * Get services entry.
 * Find the entry in current page of the DDR memory.
 * Thus DDR reloads shall be deferred for as long as possible time.
 * @param svcnm service name 
 * @param svc service descriptor
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_ddr_services_get(char *svcnm, ndrx_services_t **svc)
{
    int ret = EXFAIL;
    int have_value=EXFALSE;
    int pos=0;
    int page;
    ndrx_services_t *ptr;
    /* ddr not used. */
    if (!ndrx_G_shmcfg->use_ddr)
    {
        return EXFAIL;
    }
    page = ndrx_G_shmcfg->ddr_page;
    ptr = (ndrx_services_t *) (ndrx_G_routsvc.mem + page*G_atmi_env.rtsvcmax * sizeof(ndrx_services_t));
    
    if (EXSUCCEED==ndrx_ddr_position_get(svcnm, 0, &pos, 
        &have_value, (char **)&ptr, G_atmi_env.rtsvcmax) && have_value)
    {
        *svc = &ptr[pos];
        
        NDRX_LOG(log_debug, "Found service [%s] in ddr service table, autotran=%d", 
                (*svc)->svcnm, (*svc)->autotran);
        ret=EXSUCCEED;
    }

out:
    return ret;
}

/**
 * Get routing group defined for service
 * Note the page shall not be changed while we work here.
 * Thus DDR reloads shall be deferred for as long as possible time.
 * @param svcnm service to route to
 * @param data data to send to service
 * @param len dat len to send to service
 * @param[out] grp group code found
 * @param[out] grpsz group code buffer size
 * @return EXSUCCEED -> group not found (or default), EXTRUE group loaded, EXFAIL - not resolved
 */
expublic int ndrx_ddr_grp_get(char *svcnm, char *data, long len, char *grp, size_t grpsz)
{
    int ret = EXSUCCEED;
    ndrx_services_t *svc;
    int page;
    ndrx_routcrit_t *ccrit;
    ndrx_routcritseq_t *range;
    int offset_step=0;
    int i;
    double floatval;
    char *strval=NULL;
    buffer_obj_t *buf;
    char *mem_start;
    int in_range;
    
    if (EXSUCCEED!=ndrx_ddr_services_get(svcnm, &svc))
    {
        /* no group defined at all */
        goto out;
    }
    
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
        /* nothing todo, just return, probably at later phases error will be generated */
        goto out;
    }
    
    mem_start = ndrx_G_routcrit.mem + page * G_atmi_env.rtcrtmax+svc->offset; 
    
    do
    {
        page = ndrx_G_shmcfg->ddr_page;
        ccrit = (ndrx_routcrit_t *)(mem_start + offset_step);
        
        NDRX_LOG(log_debug, "YOPT DDR scanning at %p for [%s]", ccrit, svcnm);
        
        if (ccrit->criterionid!=svc->cirterionid)
        {
            /* no matching buffer - ok */
            goto out;
        }
        
        /* check the types, shall match the current buffer */
        
        if (buf->type_id == ccrit->buffer_type_id)
        {
            /*  OK this is ours to check... */
            if (BUF_TYPE_UBF==buf->type_id)
            {
                if (BFLD_DOUBLE == ccrit->fieldtypeid)
                {
                    /* get double for route... 
                     * What shall we do if field is not present?
                     */
                    if (EXSUCCEED==CBget((UBFH *)data, ccrit->fldid, 0, 
                            (char *)&floatval, 0L, BFLD_DOUBLE))
                    {
                        NDRX_LOG(log_debug, "Routing field not found [%s]", ccrit->field);
                        EXFAIL_OUT(ret);
                    }
                }
                else
                {
                    /* alloc string copy with CF or to avoid allocations
                     * just use sysbuf page? as that size would gurantee that we
                     * fit in...
                     */
                    strval = Bgetsa ((UBFH *)data, ccrit->fldid, 0, NULL);
                    
                    if (NULL==strval)
                    {
                        NDRX_LOG(log_debug, "Failed to get routing string field");
                        EXFAIL_OUT(ret);
                    }
                }
                
            }
            
            offset_step+=sizeof(ndrx_routcrit_t);
            
            for (i=0; i < ccrit->rangesnr; i++)
            {
                /* OK we got the range so loop over... */
                range = (ndrx_routcritseq_t *)(mem_start + offset_step);
                
                /* TODO: check that we are in boundries */
                
                if (range->flags & NDRX_DDR_FLAG_DEFAULT_VAL)
                {
                    /* ranges OK, return group */
                    in_range=EXTRUE;
                }
                else if (range->flags & NDRX_DDR_FLAG_MIN)
                {
                    /* TODO: check that value is bellow max */
                    
                }
                else if (range->flags & NDRX_DDR_FLAG_MAX)
                {
                    /* TODO: check that value is above min */
                }
                else
                {
                    /* TODO: check is it in range */
                }
                
                if (in_range)
                {
                    if (range->flags & NDRX_DDR_FLAG_DEFAULT_GRP)
                    {
                        NDRX_LOG(log_debug, "Default group matched");
                        goto out;
                    }
                    else
                    {
                        /* copy off the group code */
                        NDRX_STRCPY_SAFE_DST(grp, range->grp, grpsz);
                        NDRX_LOG(log_debug, "Group [%s]  matched", grp);
                    }
                }
                    
                /* step to next..., this is aligned step */
                offset_step+=range->len;
            }
            
        }
        
        /* TODO: limit limit of ranges, to avoid handing
         * in case if doing quick reloads
         */
        
        /* check for next... */
    } while (svc->offset + offset_step + sizeof(ndrx_routcrit_t) < G_atmi_env.rtcrtmax);
    
    
out:
    
    if (NULL!=strval)
    {
        NDRX_FREE(strval);
    }
    
    return ret;
}


/* vim: set ts=4 sw=4 et smartindent: */
