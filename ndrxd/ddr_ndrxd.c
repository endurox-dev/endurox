/**
 * @brief Real time handling routines
 *  note we use here standard malloc, as that is the same as which is used by
 *  flex/bison.
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
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>

#include <regex.h>
#include <exregex.h>
#include <ndrx_ddr.h>
#include <ndrxd.h>
#include <typed_buf.h>
#include "expr_range.tab.h"
#include "utlist.h"
#include "lcfint.h"
#include <atmi_shm.h>
#include <lcfint.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/

expublic ndrx_ddr_parser_t ndrx_G_ddrp;      /**< Parsing time attributes*/
/*---------------------------Statics------------------------------------*/
exprivate regex_t M_floatexp;       /**< float check regexp             */
exprivate regex_t M_intexp;         /**< Integer expression             */
exprivate int M_floatexp_comp=EXFALSE;  /**< is float regexp compiled?  */
exprivate int M_was_loaded=EXFALSE;       /**< Was routing loaded?      */
exprivate int M_do_reload=EXFALSE;        /**< Is reload waiting        */
exprivate int M_do_reload_cycles=EXFAIL;  /**< Number of sanity cycles, till apply */

/*---------------------------Prototypes---------------------------------*/

/**
 * Perform sanity checks of scheduled load
 */
expublic void ndrx_ddr_apply_sanity(void)
{
    if (M_do_reload)
    {
        M_do_reload_cycles++;
        
        if (M_do_reload_cycles > G_app_config->ddrreload)
        {
            ndrx_ddr_install();
        }
    }
}

/**
 * Install the DDR data to shared memory segments, swap the counters.
 * This also must detect, if there are no services, then DDR shall be disabled.
 * if installing then segments must be swapped
 */
expublic void ndrx_ddr_install(void)
{
    int do_disable = EXFALSE;
    int page;
    
    NDRX_LOG(log_info, "Installing DDR infos to shared memory");
    /* only one time please*/
    M_do_reload=EXFALSE;
    M_was_loaded=EXTRUE;
    
    if (NULL==G_app_config->services)
    {
        do_disable=EXTRUE;
    }
    
    if (do_disable)
    {
        ndrx_G_shmcfg->use_ddr=EXFALSE;
    }
    else if (!ndrx_G_shmcfg->use_ddr)
    {
        NDRX_LOG(log_info, "Loading NEW DDR, page 0");
        /* copy off just new config, start with page 0*/
        ndrx_G_shmcfg->ddr_page = 0;
        ndrx_G_shmcfg->ddr_ver1++;
        memcpy(ndrx_G_routcrit.mem, G_app_config->routing_block, G_atmi_env.rtcrtmax);
        memcpy(ndrx_G_routsvc.mem, G_app_config->services_block, G_atmi_env.rtsvcmax * sizeof(ndrx_services_t));
        
        /* Now service start to route ... */
        ndrx_G_shmcfg->use_ddr = 1;
        
    }
    else
    {
        /* check the current page, if cirt / or rout is not the same
         * then to new page copy all the routing blobs
         */
        page = ndrx_G_shmcfg->ddr_page;
        
        /* check current page is it changed or not ?  */
        
        if (0 == memcmp(ndrx_G_routcrit.mem + page*G_atmi_env.rtcrtmax, 
                G_app_config->routing_block, G_atmi_env.rtcrtmax) &&
                
            0 == memcmp(ndrx_G_routsvc.mem + page*G_atmi_env.rtsvcmax * sizeof(ndrx_services_t), 
                G_app_config->services_block, G_atmi_env.rtsvcmax * sizeof(ndrx_services_t))
                )
        {
            NDRX_LOG(log_info, "DDR routing configuration not changed");
        }
        else
        {
            /* choose the new page */
            page+=1;
            
            if (page>=NDRX_LCF_DDR_PAGES)
            {
                page=0;
            }
            
            NDRX_LOG(log_info, "DDR Changing configuration into page %d", page);
            
            ndrx_G_shmcfg->ddr_ver1++;
            memcpy(ndrx_G_routcrit.mem + page*G_atmi_env.rtcrtmax, 
                G_app_config->routing_block, G_atmi_env.rtcrtmax);
            
            memcpy(ndrx_G_routsvc.mem + page*G_atmi_env.rtsvcmax * sizeof(ndrx_services_t), 
                G_app_config->services_block, G_atmi_env.rtsvcmax * sizeof(ndrx_services_t));
            
            /* open new segment to processes to route. */
            ndrx_G_shmcfg->ddr_page = page;
        }
    }
    
}

/**
 * Load config
 * This will either schedule or apply directly if first start.
 */
expublic void ndrx_ddr_apply(void)
{
    /* Check the status, are we going to disable routing? if
     * so then we can to it directly too...
     */
    int do_disable = EXFALSE;
    
    if (NULL==G_app_config->services)
    {
        do_disable=EXTRUE;
    }
    
    if (!M_was_loaded || do_disable)
    {
        ndrx_ddr_install();
    }
    else
    {
        NDRX_LOG(log_info, "DDR routing change scheduled");
        M_do_reload=EXTRUE;
        M_do_reload_cycles=0;
    }
}

/**
 * Free up resources used by parser
 * @param ptr
 */
expublic void ndrx_ddr_delete_buffer(void *ptr)
{
    NDRX_FREE(ptr);
}

/**
 * Add sequence to the hashmap
 * What we need to do additionally is:
 * - check the integer types
 * - check the float types
 * - check that group contains valid symbols
 * If all OK, add the entry to linked list
 * @param seq parsed sequence
 * @param grp parsed group
 * @param is_mallocd is grp allocated by previous stages
 */
expublic int ndrx_ddr_add_group(ndrx_routcritseq_dl_t * seq, char *grp, int is_mallocd)
{
    int ret = EXSUCCEED;
    int len;
    char *dflt_group = "*";
    regex_t *p_rex = NULL;
    if (NULL==grp)
    {
        grp = dflt_group;
    }
    
    NDRX_LOG(log_debug, "Adding routing group: [%s]", grp);
    
    /* Number check:  */
    
    /* we run single threaded here, so no worry... */
    if (!M_floatexp_comp)
    {
        if (EXSUCCEED!=ndrx_regcomp(&M_floatexp, "^[-+]?(([0-9]*[.]?[0-9]+([ed][-+]?[0-9]+)?))$"))
        {
            NDRX_LOG(log_error, "Failed to compile regexp of tag float check");
            NDRXD_set_error_fmt(NDRXD_EOS, "(%s) Failed to compile regexp of tag float check!", 
                G_sys_config.config_file_short);
            EXFAIL_OUT(ret);
        }
        
        /* compile for longs too, then select which expression to use */
        
        if (EXSUCCEED!=ndrx_regcomp(&M_intexp, "^[+-]?([0-9])+$"))
        {
            NDRX_LOG(log_error, "Failed to compile regexp of tag int check");
            NDRXD_set_error_fmt(NDRXD_EOS, "(%s) Failed to compile regexp of tag int check!", 
                G_sys_config.config_file_short);
            EXFAIL_OUT(ret);
        }
        
        M_floatexp_comp=EXTRUE;
    }
    
    /* Check the group code -> cannot contain the @ */
    if (NULL!=strchr(grp, '@') || NULL!=strchr(grp, '/') || NULL!=strchr(grp, ' ') || NULL!=strchr(grp, '\t')
            || NULL!=strchr(grp, ','))
    {
        NDRX_LOG(log_error, "Invalid group code [%s]", grp);
        NDRXD_set_error_fmt(NDRXD_ESYNTAX, "(%s) Invalid group code [%s]", 
            G_sys_config.config_file_short, grp);
        EXFAIL_OUT(ret);
    }
    
    len = strlen(grp);
    if (len > NDRX_DDR_GRP_MAX)
    {
        NDRX_LOG(log_error, "Group code [%s] too long max %d", grp, NDRX_DDR_GRP_MAX);
        NDRXD_set_error_fmt(NDRXD_ESYNTAX, "(%s) Group code [%s] too long max %d",
            G_sys_config.config_file_short, grp, NDRX_DDR_GRP_MAX);
        EXFAIL_OUT(ret);
    }
    
    if (0==strcmp(grp, "*"))
    {
        seq->cseq.flags|= NDRX_DDR_FLAG_DEFAULT_GRP;
    }
    
    /* ok setup group code */
    NDRX_STRCPY_SAFE(seq->cseq.grp, grp);
    
    /* 
     * Resolve ranges
     */
    if (BFLD_DOUBLE==ndrx_G_ddrp.p_crit->routcrit.fieldtypeid ||
            BFLD_LONG==ndrx_G_ddrp.p_crit->routcrit.fieldtypeid
            )
    {
        
        if (BFLD_DOUBLE==ndrx_G_ddrp.p_crit->routcrit.fieldtypeid)
        {
            p_rex=&M_floatexp;
        }
        else
        {
            p_rex=&M_intexp;
        }
        
        /* calculate the final len: 
         */
        seq->cseq.len = sizeof(seq->cseq);
        
        /* check min / max if have min/max/default indications .. */
        if (seq->cseq.flags & NDRX_DDR_FLAG_DEFAULT_VAL ||
                (seq->cseq.flags & NDRX_DDR_FLAG_MIN && seq->cseq.flags & NDRX_DDR_FLAG_MAX)
                )
        {
            /* nothing todo */
        }
        else if (seq->cseq.flags & NDRX_DDR_FLAG_MIN)
        {
            /* we can setup max + validate string */
            
            if (EXSUCCEED!=ndrx_regexec(p_rex, seq->cseq.strrange))
            {
                NDRX_LOG(log_error, "Invalid upper range [%s] for grp [%s] "
                        "routing [%s] buffer type [%s]", 
                        seq->cseq.strrange, grp,
                        ndrx_G_ddrp.p_crit->routcrit.criterion, ndrx_G_ddrp.p_crit->routcrit.buftype);
                NDRXD_set_error_fmt(NDRXD_ESYNTAX, "(%s) Invalid upper range [%s] "
                        "for grp [%s] routing [%s] buffer type [%s]",
                    G_sys_config.config_file_short, seq->cseq.strrange, grp,
                        ndrx_G_ddrp.p_crit->routcrit.criterion, ndrx_G_ddrp.p_crit->routcrit.buftype);
                EXFAIL_OUT(ret);
            }
            
            seq->cseq.upperd = ndrx_atof(seq->cseq.strrange);
            seq->cseq.upperl = atol(seq->cseq.strrange);
        }
        else if (seq->cseq.flags & NDRX_DDR_FLAG_MAX)
        {
            if (EXSUCCEED!=ndrx_regexec(p_rex, seq->cseq.strrange))
            {
                NDRX_LOG(log_error, "Invalid lower range [%s] for grp [%s] "
                        "routing [%s] buffer type [%s]", 
                        seq->cseq.strrange, grp,
                        ndrx_G_ddrp.p_crit->routcrit.criterion, ndrx_G_ddrp.p_crit->routcrit.buftype);
                NDRXD_set_error_fmt(NDRXD_ESYNTAX, "(%s) Invalid lower range [%s] "
                        "for grp [%s] routing [%s] buffer type [%s]",
                    G_sys_config.config_file_short,  seq->cseq.strrange, grp,
                        ndrx_G_ddrp.p_crit->routcrit.criterion, ndrx_G_ddrp.p_crit->routcrit.buftype);
                EXFAIL_OUT(ret);
            }
            
            seq->cseq.lowerd = ndrx_atof(seq->cseq.strrange);
            seq->cseq.lowerl = atol(seq->cseq.strrange);
        }
        else
        {
            
            if (EXSUCCEED!=ndrx_regexec(p_rex, seq->cseq.strrange))
            {
                NDRX_LOG(log_error, "Invalid lower range [%s] for grp [%s] "
                        "routing [%s] buffer type [%s]", 
                        seq->cseq.strrange, grp,
                        ndrx_G_ddrp.p_crit->routcrit.criterion, ndrx_G_ddrp.p_crit->routcrit.buftype);
                NDRXD_set_error_fmt(NDRXD_ESYNTAX, "(%s) Invalid lower range [%s] "
                        "for grp [%s] routing [%s] buffer type [%s]",
                    G_sys_config.config_file_short,  seq->cseq.strrange, grp,
                        ndrx_G_ddrp.p_crit->routcrit.criterion, ndrx_G_ddrp.p_crit->routcrit.buftype);
                EXFAIL_OUT(ret);
            }
            
            if (EXSUCCEED!=ndrx_regexec(p_rex, seq->cseq.strrange+seq->cseq.strrange_upper))
            {
                NDRX_LOG(log_error, "Invalid upper range [%s] for grp [%s] "
                        "routing [%s] buffer type [%s]", 
                        seq->cseq.strrange, grp, 
                        ndrx_G_ddrp.p_crit->routcrit.criterion, ndrx_G_ddrp.p_crit->routcrit.buftype);
                NDRXD_set_error_fmt(NDRXD_ESYNTAX, "(%s) Invalid upper range [%s] "
                        "for grp [%s] routing [%s] buffer type [%s]",
                    G_sys_config.config_file_short,  seq->cseq.strrange, grp,
                        ndrx_G_ddrp.p_crit->routcrit.criterion, ndrx_G_ddrp.p_crit->routcrit.buftype);
                EXFAIL_OUT(ret);
            }
            
            seq->cseq.lowerd = ndrx_atof(seq->cseq.strrange);
            seq->cseq.lowerl = atol(seq->cseq.strrange);
            
            seq->cseq.upperd = ndrx_atof(seq->cseq.strrange+seq->cseq.strrange_upper);
            seq->cseq.upperl = atol(seq->cseq.strrange+seq->cseq.strrange_upper);
            
            if (seq->cseq.lowerd > seq->cseq.upperd)
            {
                NDRX_LOG(log_error, "Lower [%lf] greater than upper [%lf] for grp [%s] "
                        "routing [%s] buffer type [%s]", 
                        seq->cseq.lowerd, seq->cseq.upperd, grp, 
                        ndrx_G_ddrp.p_crit->routcrit.criterion, ndrx_G_ddrp.p_crit->routcrit.buftype);
                NDRXD_set_error_fmt(NDRXD_ESYNTAX, "(%s) Lower [%lf] greater than upper [%lf] for grp [%s] "
                        "routing [%s] buffer type [%s]",
                        G_sys_config.config_file_short,  seq->cseq.lowerd, 
                        seq->cseq.upperd, grp,
                        ndrx_G_ddrp.p_crit->routcrit.criterion, 
                        ndrx_G_ddrp.p_crit->routcrit.buftype);
                EXFAIL_OUT(ret);
            }
        }
    }
    else
    {
        /* it is string OK -> leave as is ... 
         * Also here to calculate the size needs to understand
         * min/max/default  settings
         */
        if (seq->cseq.flags & NDRX_DDR_FLAG_DEFAULT_VAL ||
                (seq->cseq.flags & NDRX_DDR_FLAG_MIN && seq->cseq.flags & NDRX_DDR_FLAG_MAX)
                )
        {
            seq->cseq.len = sizeof(seq->cseq);
        }
        else if (seq->cseq.flags & NDRX_DDR_FLAG_MIN)
        {
            seq->cseq.len = sizeof(seq->cseq) + strlen(seq->cseq.strrange)+1;
        }
        else if (seq->cseq.flags & NDRX_DDR_FLAG_MAX)
        {
           seq->cseq.len = sizeof(seq->cseq) + strlen(seq->cseq.strrange)+1;
        }
        else
        {
            /* OK check there that strings follow the rules higher lower */
            if (strcmp(seq->cseq.strrange, seq->cseq.strrange + seq->cseq.strrange_upper) > 0)
            {
                NDRX_LOG(log_error, "Lower [%s] greater than upper [%s] for grp [%s] "
                        "routing [%s] buffer type [%s]", 
                        seq->cseq.strrange, seq->cseq.strrange + seq->cseq.strrange_upper, grp, 
                        ndrx_G_ddrp.p_crit->routcrit.criterion, ndrx_G_ddrp.p_crit->routcrit.buftype);
                NDRXD_set_error_fmt(NDRXD_ESYNTAX, "(%s) Lower [%s] greater than upper [%s] for grp [%s] "
                        "routing [%s] buffer type [%s]",
                        G_sys_config.config_file_short,  
                        seq->cseq.strrange, seq->cseq.strrange + seq->cseq.strrange_upper, grp,
                        ndrx_G_ddrp.p_crit->routcrit.criterion, 
                        ndrx_G_ddrp.p_crit->routcrit.buftype);
                EXFAIL_OUT(ret);
            }
            
            /* both strings are used... */
            seq->cseq.len = sizeof(seq->cseq) + strlen(seq->cseq.strrange)+1
                    +strlen(seq->cseq.strrange + seq->cseq.strrange_upper)+1;
        }
    }
    
    
    /* have some debug... */
    NDRX_LOG(log_debug, "--- Adding Route [%s] group range ---", seq->cseq.grp);
    
    NDRX_LOG(log_debug, "CRITERION: %s", ndrx_G_ddrp.p_crit->routcrit.criterion);
    NDRX_LOG(log_debug, "BUFTYPE: %s", ndrx_G_ddrp.p_crit->routcrit.buftype);
    
    NDRX_LOG(log_debug, "lowerl: %ld", seq->cseq.lowerl);
    NDRX_LOG(log_debug, "upperl: %ld", seq->cseq.upperl);
    
    NDRX_LOG(log_debug, "lowerd: %lf", seq->cseq.lowerd);
    NDRX_LOG(log_debug, "upperd: %lf", seq->cseq.upperd);
    NDRX_LOG(log_debug, "len: %d (sturct size: %d)", seq->cseq.len, sizeof(seq->cseq));
    NDRX_LOG(log_debug, "flags: %d", seq->cseq.flags);
    
    
    NDRX_LOG(log_debug, "MIN: %d", seq->cseq.flags & NDRX_DDR_FLAG_MIN);
    NDRX_LOG(log_debug, "MAX: %d", seq->cseq.flags & NDRX_DDR_FLAG_MAX);
    NDRX_LOG(log_debug, "DEFAULT VAL: %d", seq->cseq.flags & NDRX_DDR_FLAG_DEFAULT_VAL);
    NDRX_LOG(log_debug, "DEFAULT GRP: %d", seq->cseq.flags & NDRX_DDR_FLAG_DEFAULT_GRP);
    
    NDRX_LOG(log_debug, "strrange_upper: %d", seq->cseq.strrange_upper);
    NDRX_LOG(log_debug, "strrange: [%s]", seq->cseq.strrange);
    NDRX_LOG(log_debug, "strrange_upper: [%s]", seq->cseq.strrange+seq->cseq.strrange_upper);
    
    NDRX_LOG(log_debug, "-------------------------------------");
    
    /* add to DL finally */
    DL_APPEND(ndrx_G_ddrp.p_crit->seq, seq)
    ndrx_G_ddrp.p_crit->routcrit.rangesnr++;

out:
    if (is_mallocd)
    {
        NDRX_FREE(grp);
    }

    if (EXSUCCEED!=ret)
    {
        /* not added to DL, thus will leak with out doing this... */
        NDRX_FREE(seq);
    }
    NDRX_LOG(log_error, "ret %d", ret);
    return ret;
}

/**
 * This will create routing criterion sequence entry
 * @param range_min
 * @param range_max
 * @return linked list sequence object
 */
expublic ndrx_routcritseq_dl_t * ndrx_ddr_new_rangeexpr(char *range_min, char *range_max)
{
    int str_min_sz=0;
    int str_max_sz=0;
    ndrx_routcritseq_dl_t *ret=NULL;
    int flags=0;
    int sz;
    
    if (NULL==range_min && NULL==range_max)
    {
        flags|=NDRX_DDR_FLAG_DEFAULT_VAL;
        NDRX_LOG(log_debug, "DEFAULT range");
    }
    else if (NULL==range_min)
    {
        flags|=NDRX_DDR_FLAG_MIN;
        str_max_sz=strlen(range_max)+1;
        
        NDRX_LOG(log_debug, "range_max=[%s]", range_max);
    }
    else if (NULL==range_max)
    {
        flags|=NDRX_DDR_FLAG_MAX;
        str_min_sz=strlen(range_min)+1;
        NDRX_LOG(log_debug, "range_min=[%s]", range_min);
    }
    else
    {
        /* both strings are used */
        str_min_sz=strlen(range_min)+1;
        str_max_sz=strlen(range_max)+1;
        
        NDRX_LOG(log_debug, "range_min=[%s]", range_min);
        NDRX_LOG(log_debug, "range_max=[%s]", range_max);
    }
    
    sz = sizeof(ndrx_routcritseq_dl_t) + str_max_sz + str_min_sz;
    ret = NDRX_MALLOC(sz);
    
    if (NULL==ret)
    {
        NDRX_LOG(log_error, "(%s) malloc %d bytes!", 
                G_sys_config.config_file_short, sz);
        NDRXD_set_error_fmt(NDRXD_EOS, "(%s) malloc %d bytes!", 
                G_sys_config.config_file_short, sz);
        goto out;
    }
    
    memset(ret, 0, sz);
    
    ret->cseq.flags=flags;
    ret->cseq.strrange_upper=0;

    if ( !((flags & NDRX_DDR_FLAG_MIN) ||
            (flags & NDRX_DDR_FLAG_MAX) || (flags & NDRX_DDR_FLAG_DEFAULT_VAL)
            ))
    {
        
        /* both values are used */
        NDRX_STRCPY_SAFE_DST(ret->cseq.strrange, range_min, sz);
        
        /* this is where upper starts */
        ret->cseq.strrange_upper=str_min_sz;
        NDRX_STRCPY_SAFE_DST((ret->cseq.strrange+str_min_sz), range_max, str_max_sz);
    }
    else if (flags & NDRX_DDR_FLAG_MAX)
    {
        /* only min is stored */
        NDRX_STRCPY_SAFE_DST(ret->cseq.strrange, range_min, sz);
    }
    else if (flags & NDRX_DDR_FLAG_MIN)
    {
        NDRX_STRCPY_SAFE_DST(ret->cseq.strrange, range_max, sz);
    }
    
out:
    
    
    if (NULL!=range_min)
    {
        NDRX_FREE(range_min);
    }

    /* free if having different pointers */
    if (NULL!=range_max && range_max!=range_min)
    {
        NDRX_FREE(range_max);
    }

    return ret;
    
}
/**
 * Allocate range value with sign, if having the one
 * @param range range token
 * @param is_negative is negative?
 * @param dealloc - remove the range
 * @return NULL in case of error
 */
expublic char *ndrx_ddr_new_rangeval(char *range, int is_negative, int dealloc)
{
    char *ret;
    int len;
    
    if (!is_negative)
    {
        ret=NDRX_STRDUP(range);
    }
    else
    {
        NDRX_ASPRINTF(&ret, &len, "-%s", range);
    }
    
    if (dealloc)
    {
        NDRX_FREE(range);
    }
    
    return ret;
}

/**
 * Generate parsing error
 * @param s
 * @param ...
 */
void ddrerror(char *s, ...)
{
    /* Log only first error! */
    if (EXFAIL!=ndrx_G_ddrp.error)
    {
        va_list ap;
        va_start(ap, s);
        char errbuf[2048];

        snprintf(errbuf, sizeof(errbuf), "Routing range of [%s] buftype [%s]. Near of %d-%d: ", 
                ndrx_G_ddrp.p_crit->routcrit.criterion, 
                ndrx_G_ddrp.p_crit->routcrit.buftype,
                ddrlloc.first_column, ddrlloc.last_column);
        vsprintf(errbuf+strlen(errbuf), s, ap);

        if (NDRXD_is_error())
        {
          /* append message */
          NDRXD_append_error_msg(errbuf);
        }
        else
        {
          NDRXD_set_error_msg(NDRXD_ESYNTAX, errbuf);
        }

        ndrx_G_ddrp.error = EXFAIL;
    }
}

extern void ddr_scan_string (char *yy_str  );
extern int ddrlex_destroy  (void);
extern int ndrx_G_ddrcolumn;
/**
 * Parse range
 * @param p_crit current criterion in parse subject
 * @return EXSUCCEED/EXFAIL
 */
exprivate int ndrx_parse_range(ndrx_routcrit_typehash_t *p_crit)
{
    int ret = EXSUCCEED;
    memset(&ndrx_G_ddrp, 0, sizeof(ndrx_G_ddrp));
    ndrx_G_ddrp.p_crit=p_crit;
    /* init the string builder */
    ndrx_growlist_init(&ndrx_G_ddrp.stringbuffer, 200, sizeof(char));
    
    /* start to parse... */
    
    ndrx_G_ddrcolumn=0;
    NDRX_LOG(log_info, "Parsing range: [%s]", p_crit->ranges);
    ddr_scan_string(p_crit->ranges);
            
    if (EXSUCCEED!=ddrparse() || EXSUCCEED!=ndrx_G_ddrp.error)
    {
        NDRX_LOG(log_error, "Failed to parse range: [%s]", p_crit->ranges);
        NDRXD_set_error_fmt(NDRXD_EOS, "(%s) failed to parse range", 
                G_sys_config.config_file_short, p_crit->ranges);
        
        /* free parsers... */
        ddrlex_destroy();
        
        
        /* well if we hav */
        
        EXFAIL_OUT(ret);
    }
    ddrlex_destroy();
    
out:
    /* free up string buffer */
    ndrx_growlist_free(&ndrx_G_ddrp.stringbuffer);

    return ret;    
}

/**
 * Lookup the criterion in hashmap, if not found, just add
 * @param config current config loading
 * @param criterion criterion code to lookup
 * @return NULL in case of failure
 */
exprivate ndrx_routcrit_hash_t * ndrx_criterion_get(config_t *config, char *criterion)
{
    ndrx_routcrit_hash_t *ret=NULL;
    int err;
    int len = strlen(criterion);
    
    if (len > NDRX_DDR_CRITMAX)
    {
        NDRX_LOG(log_error, "(%s) Invalid criterion name [%s] len is %d, but max is %d",
                G_sys_config.config_file_short, criterion, len, NDRX_DDR_CRITMAX);
        NDRXD_set_error_fmt(NDRXD_EINVAL, "(%s) Invalid criterion name [%s] len is %d but max is %d",
                G_sys_config.config_file_short, criterion, len, NDRX_DDR_CRITMAX);
        goto out;
    }
    
    if (0==len)
    {
        NDRX_LOG(log_error, "(%s) Empty criterion name",
                G_sys_config.config_file_short);
        NDRXD_set_error_fmt(NDRXD_ECFGINVLD, "(%s) Empty criterion name",
                G_sys_config.config_file_short);
        goto out;
    }
    
    EXHASH_FIND_STR(config->cirthash, criterion, ret);
    
    if (NULL==ret)
    {
        /* Alloc the criterion */
        ret = NDRX_MALLOC(sizeof(ndrx_routcrit_hash_t));
        
        if (NULL==ret)
        {
            err = errno;
            
            NDRX_LOG(log_error, "(%s) Failed to malloc %d byte (ndrx_routcrit_hash_t): %s", 
                    G_sys_config.config_file_short, sizeof(ndrx_routcrit_hash_t), strerror(err));
            
            NDRXD_set_error_fmt(NDRXD_EOS, "(%s) Failed to malloc %d byte (ndrx_routcrit_hash_t): %s", 
                    G_sys_config.config_file_short, sizeof(ndrx_routcrit_hash_t), strerror(err));
            goto out;
        }
        
        memset(ret, 0, sizeof(ndrx_routcrit_hash_t));
        
        NDRX_STRCPY_SAFE(ret->criterion, criterion);
        EXHASH_ADD_STR(config->cirthash, criterion, ret);
    }
    
out:
    return ret;
}


/**
 * Parse one route entry
 * @param config current config loading
 * @param doc
 * @param cur
 * @param is_defaults is this parsing of default
 * @param p_defaults default settings
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_service_parse(config_t *config, xmlDocPtr doc, xmlNodePtr cur,
        int is_defaults, ndrx_services_hash_t *p_defaults)
{
    int ret=EXSUCCEED;
    xmlAttrPtr attr;
    ndrx_services_hash_t *p_svc=NULL, *elt=NULL;
    char *p;
    
    /* service shall not be defined */
    
    if (is_defaults)
    {
        p_svc=p_defaults;
    }
    else
    {
        /* first of all, we need to get server name from attribs */
        p_svc = NDRX_MALLOC(sizeof(ndrx_services_hash_t));
        if (NULL==p_svc)
        {
            NDRX_LOG(log_error, "malloc failed for ndrx_routsvc_t!");
            NDRXD_set_error_fmt(NDRXD_EOS, "(%s) malloc failed for srvnode!", 
                    G_sys_config.config_file_short);
            EXFAIL_OUT(ret);
        }
        memcpy(p_svc, p_defaults, sizeof(ndrx_services_hash_t));
    }
    
    for (attr=cur->properties; attr; attr = attr->next)
    {
        p = (char *)xmlNodeGetContent(attr->children);
        
        if (0==strcmp((char *)attr->name, "svcnm"))
        {
            /* check the name max */
            if (strlen(p) > XATMI_SERVICE_NAME_LENGTH)
            {
                NDRX_LOG(log_error, "(%s) Too long service name [%s] in <services> section max %d", 
                    G_sys_config.config_file_short, p, XATMI_SERVICE_NAME_LENGTH);
                NDRXD_set_error_fmt(NDRXD_ECFGINVLD, "(%s) Too long service name "
                        "[%s] in <services> section max %d", 
                    G_sys_config.config_file_short, p, XATMI_SERVICE_NAME_LENGTH);
                xmlFree(p);
                EXFAIL_OUT(ret);
            }
            NDRX_STRCPY_SAFE(p_svc->svcnm, p);
            NDRX_STRCPY_SAFE(p_svc->routsvc.svcnm, p);
            
        }
        else if (0==strcmp((char *)attr->name, "prio"))
        {
            p_svc->routsvc.prio = atoi(p);

            if (p_svc->routsvc.prio<NDRX_MSGPRIO_MIN ||
                p_svc->routsvc.prio>NDRX_MSGPRIO_MAX)
            {

                NDRX_LOG(log_error, "(%s) Invalid prio %d in <services> section min %d max %d", 
                    G_sys_config.config_file_short, p_svc->routsvc.prio,
                    NDRX_MSGPRIO_MIN, NDRX_MSGPRIO_MAX);

                NDRXD_set_error_fmt(NDRXD_ECFGINVLD,
                    "(%s) Invalid prio %d in <services> section min %d max %d",    
                    G_sys_config.config_file_short, p_svc->routsvc.prio,
                    NDRX_MSGPRIO_MIN, NDRX_MSGPRIO_MAX);

                xmlFree(p);
                EXFAIL_OUT(ret);
            }
        }
        else if (0==strcmp((char *)attr->name, "routing"))
        {
            
            /* for debug purposes have criterion too
             * also if criterion is missing
             * we do not need to do routing
             */
            NDRX_STRCPY_SAFE(p_svc->routsvc.criterion, p);
        }
        else if (0==strcmp((char *)attr->name, "autotran"))
        {
            if ('y'==*p || 'Y'==*p)
            {
                p_svc->routsvc.autotran=EXTRUE;
            }
        }
        else if (0==strcmp((char *)attr->name, "trantime"))
        {
            p_svc->routsvc.trantime = atol(p);
        }
        
        xmlFree(p);
    }
    
        /* no hashing for defaults */
    if (!is_defaults)
    {
        /* Check service settings */
        if (EXEOS==p_svc->routsvc.svcnm[0])
        {
            NDRX_LOG(log_error, "(%s) Empty service definition", 
                    G_sys_config.config_file_short);

            NDRXD_set_error_fmt(NDRXD_ECFGINVLD, "(%s) Empty service definition", 
                    G_sys_config.config_file_short);
            EXFAIL_OUT(ret);
        }
    
        EXHASH_FIND_STR(config->services, p_svc->routsvc.svcnm, elt);

        if (NULL!=elt)
        {
            NDRX_LOG(log_error, "(%s) Service [%s] already defined", 
                    G_sys_config.config_file_short, p_svc->svcnm);

            NDRXD_set_error_fmt(NDRXD_ECFGINVLD, "(%s) Service [%s] already defined", 
                    G_sys_config.config_file_short,p_svc->svcnm);
            EXFAIL_OUT(ret);
        }

        NDRX_LOG(log_debug, "SERVICES Entry: SVCNM=%s PRIO=%d ROUTING=%s AUTOTRAN=%c TRANTIME=%lu",
                p_svc->routsvc.svcnm, p_svc->routsvc.prio, p_svc->routsvc.criterion, 
                p_svc->routsvc.autotran?'Y':'N', p_svc->routsvc.trantime);

        EXHASH_ADD_STR(config->services, svcnm, p_svc);
    }
out:
    
    /* defaults are static..  */
    if (EXFAIL==ret && !is_defaults && p_svc)
    {
        NDRX_FREE(p_svc);
    }
    return ret;
}

/**
 * Parse routing
 */
expublic int ndrx_services_parse(config_t *config, xmlDocPtr doc, xmlNodePtr cur)
{
    int ret=EXSUCCEED;
    ndrx_services_hash_t default_svc;
    
    int is_service;
    int is_defaults;
    
    memset(&default_svc, 0, sizeof(default_svc));
    
        /* set default prio */
    default_svc.routsvc.prio = NDRX_MSGPRIO_DEFAULT;
    default_svc.routsvc.trantime = NDRX_DDR_TRANTIMEDFLT;
    
    for (; cur ; cur=cur->next)
    {
        is_service= (0==strcmp((char*)cur->name, "service"));
        is_defaults= (0==strcmp((char*)cur->name, "defaults"));
        
        if (is_service || is_defaults)
        {
            /* Get the server name */
            if (EXSUCCEED!=ndrx_service_parse(config, doc, cur, is_defaults, &default_svc))
            {
                NDRXD_set_error_fmt(NDRXD_ECFGINVLD, "(%s) Failed to "
                        "parse <service> section", G_sys_config.config_file_short);
                ret=EXFAIL;
                goto out;
            }
        }
    }
out:
                
    return ret;
}

/**
 * Free services if any found in config object
 * @param config config object
 */
expublic void ndrx_services_free(config_t *config)
{
    ndrx_services_hash_t *el, *elt;
    
    EXHASH_ITER(hh, config->services, el, elt)
    {
        EXHASH_DEL(config->services, el);
        NDRX_FREE(el);
    }
}

/**
 * Free up route criterion
 * @param p_crit allocd object
 */
exprivate void ndrx_routcrit_type_free(ndrx_routcrit_typehash_t *o_type)
{
     ndrx_routcritseq_dl_t *el, *elt;
    
    if (NULL!=o_type->ranges)
    {
        NDRX_FREE(o_type->ranges);
    }
     
    /* clean up the DL */
    DL_FOREACH_SAFE(o_type->seq, el, elt)
    {
        DL_DELETE(o_type->seq, el);
        NDRX_FREE(el);
    }

    NDRX_FREE(o_type);
}

/**
 * Free up all routcrit elements
 * @param config config to free up
 */
exprivate void ndrx_routcrit_free(config_t *config)
{
    ndrx_routcrit_hash_t *el, *elt;
    ndrx_routcrit_typehash_t *t, *tt;
    
    /* remove all routes */
    EXHASH_ITER(hh, config->cirthash, el, elt)
    {
        
        /* remove all types */
        EXHASH_ITER(hh, el->btypes, t, tt)
        {
            
            EXHASH_DEL(el->btypes, t);
            ndrx_routcrit_type_free(t);
        }
        
        EXHASH_DEL(config->cirthash, el);
        NDRX_FREE(el);
    }
}

/**
 * Free up all routing structs
 * including allocated work blocks for generating the layout
 * @param p_crit
 */
expublic void ndrx_ddr_free_all(config_t *config)
{
    if (NULL!=config->services_block)
    {
        NDRX_FREE(config->services_block);
    }
    
    if (NULL!=config->routing_block)
    {
        NDRX_FREE(config->routing_block);
    }
    
    /* free up the temporary lists */
    ndrx_services_free(config);
    config->services=NULL;
    
    ndrx_routcrit_free(config);
    config->cirthash=NULL;
}

/**
 * Load the routing
 * @param mem memory segment start
 * @param size of the segment
 * @param routes
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_ddr_criterion_put(char *mem, long size, ndrx_routcrit_hash_t *routes)
{
    ndrx_routcrit_hash_t *el, *elt;
    ndrx_routcrit_typehash_t *t, *tt;
    ndrx_routcritseq_dl_t *dl;
    int block_size;
    int critid = 1;
    long pos=0;
    long size_left = size;
    int org_len;
    int ret = EXSUCCEED;
    
    EXHASH_ITER(hh, routes, el, elt)
    {
        /* assign for later quick lookup */
        el->criterionid = critid;
        
        /* this is where criterion starts, for quick lookup  */
        el->offset = pos;
        
        EXHASH_ITER(hh, el->btypes, t, tt)
        {
            t->routcrit.criterionid=critid;
            /* this shall be already aligned */
            block_size = sizeof(t->routcrit);
            
            if (block_size>size_left)
            {
                /* cannot install block */
                NDRX_LOG(log_error, "(%s) Cannot install route [%s] type [%s] "
                        "block. %s too small (block size %d pos %ld size %ld)", 
                        G_sys_config.config_file_short, el->criterion, t->buftype, 
                        CONF_NDRX_RTCRTMAX, block_size, pos, size);
                NDRXD_set_error_fmt(NDRXD_EINVAL, "(%s) Cannot install route [%s] type [%s] "
                        "block. %s too small (block size %d pos %ld size %ld)", 
                        G_sys_config.config_file_short, el->criterion, t->buftype, 
                        CONF_NDRX_RTCRTMAX, block_size, pos, size);
                EXFAIL_OUT(ret);
            }
            
            memcpy(mem+pos, &t->routcrit, block_size);
            
            pos+=block_size;
            size_left-=block_size;
            
            /* process now ranges */
            DL_FOREACH(t->seq, dl)
            {
                org_len=dl->cseq.len;
                dl->cseq.len = DDR_ALIGNED_GEN(dl->cseq.len);
                block_size = dl->cseq.len;
                
                NDRX_LOG(log_debug, "Range Block size %ld", block_size);
                
                if (block_size>size_left)
                {
                    /* cannot install block */
                    NDRX_LOG(log_error, "(%s) Cannot install route seq [%s] type [%s] block. %s too small", 
                            G_sys_config.config_file_short, el->criterion, t->buftype, CONF_NDRX_RTCRTMAX);
                    NDRXD_set_error_fmt(NDRXD_EINVAL, "(%s) Cannot install route seq [%s] type [%s] block. %s too small", 
                            G_sys_config.config_file_short, el->criterion, t->buftype, CONF_NDRX_RTCRTMAX);
                    EXFAIL_OUT(ret);
                }
                
                memcpy(mem+pos, &dl->cseq, org_len);
                pos+=block_size;
                size_left-=block_size;
            }
        }
        critid++;
    }
    
out:
    return ret;
}

/**
 * - We need to populate the memory block with services (use standard
 *  linear hashing routines)
 * - Also... needs to populate the memory block routes
 *  use custom TLV build.
 * @param config generate blocks
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_ddr_gen_blocks(config_t *config)
{
    int ret = EXSUCCEED;
    ndrx_services_hash_t *el, *elt;
    ndrx_routcrit_hash_t *rt;
    /* loop over all routing blocks
     * and calculate the mem & updates offset for the criterion start
     */
    
    config->routing_block = NDRX_CALLOC(G_atmi_env.rtcrtmax, 1);
    
    if (NULL==config->routing_block)
    {
        NDRX_LOG(log_error, "(%s) Failed to malloc %d bytes (routing_block)", 
                G_sys_config.config_file_short, G_atmi_env.rtcrtmax);
        NDRXD_set_error_fmt(NDRXD_EINVAL, "(%s) Failed to malloc %d bytes (routing_block)",
                G_sys_config.config_file_short, G_atmi_env.rtcrtmax);
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=ndrx_ddr_criterion_put(config->routing_block, G_atmi_env.rtcrtmax, 
            config->cirthash))
    {
        EXFAIL_OUT(ret);
    }
    
    /* loop over services - lookup the criterion start
     * update the and write to memory block
     * We need hashing for services.
     * Check if criterion needs to be checked or not
     */
    config->services_block = NDRX_CALLOC(sizeof(ndrx_services_t)*G_atmi_env.rtsvcmax, 1);
    
    if (NULL==config->services_block)
    {
        NDRX_LOG(log_error, "(%s) Failed to malloc %d bytes (services_block)", 
                G_sys_config.config_file_short, sizeof(ndrx_services_t)*G_atmi_env.rtsvcmax);
        NDRXD_set_error_fmt(NDRXD_EINVAL, "(%s) Failed to malloc %d bytes (services_block)",
                G_sys_config.config_file_short, sizeof(ndrx_services_t)*G_atmi_env.rtsvcmax);
        EXFAIL_OUT(ret);
    }
    
    EXHASH_ITER(hh, config->services, el, elt)
    {
        
        if (EXEOS!=el->routsvc.criterion[0])
        {    
            /* lookup criterion here & get offset
             * also check if criterion is missing, in such case generate error
             */
            EXHASH_FIND_STR(config->cirthash, el->routsvc.criterion, rt);
            
            if (NULL==rt)
            {
                NDRX_LOG(log_error, "(%s) Service [%s] routing criterion [%s] not defined", 
                    G_sys_config.config_file_short, el->routsvc.svcnm, el->routsvc.criterion);
                NDRXD_set_error_fmt(NDRXD_EINVAL, "(%s) Service [%s] routing criterion [%s] not defined", 
                    G_sys_config.config_file_short, el->routsvc.svcnm, el->routsvc.criterion);
                EXFAIL_OUT(ret);
            }
            el->routsvc.offset=rt->offset;
            el->routsvc.cirterionid =rt->criterionid;
        }
        /* else it is just infos for auto-transactions/timeouts and priority... */
        
        NDRX_LOG(log_debug, "Installing routing service [%s]", el->routsvc.svcnm);
        if (EXSUCCEED!=ndrx_ddr_services_put(&el->routsvc, config->services_block, G_atmi_env.rtsvcmax))
        {
            NDRX_LOG(log_error, "(%s) Failed to put service config [%s] into memory, check %s size", 
                G_sys_config.config_file_short, el->routsvc.svcnm, CONF_NDRX_RTSVCMAX);
            NDRXD_set_error_fmt(NDRXD_EINVAL, "(%s) Failed to put service config [%s] into memory, check %s size", 
                G_sys_config.config_file_short, el->routsvc.svcnm, CONF_NDRX_RTSVCMAX);
            EXFAIL_OUT(ret);
        }
    }
    
out:
    return ret;
}

/**
 * Parse single route entry.
 * @param config current config loading
 * @param doc
 * @param cur
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_route_parse(config_t *config, xmlDocPtr doc, xmlNodePtr cur)
{
    int ret=EXSUCCEED;
    int len;
    xmlAttrPtr attr;
    ndrx_routcrit_hash_t *p_route=NULL;
    ndrx_routcrit_typehash_t *p_crit=NULL;
    ndrx_routcrit_typehash_t *el=NULL;
    char *p;
    int i;
    typed_buffer_descr_t *descr;
    struct
    {
        char *typecode;
        int typeid;
    } typemap [] = {

        {"CHAR",  BFLD_CHAR},
        {"SHORT", BFLD_SHORT},
        {"LONG",  BFLD_LONG},
        {"FLOAT", BFLD_FLOAT},
        {"DOUBLE",BFLD_DOUBLE},
        {"STRING",BFLD_STRING}
    };
    
    for (attr=cur->properties; attr; attr = attr->next)
    {
        p = (char *)xmlNodeGetContent(attr->children);
           
        if (0==strcmp((char *)attr->name, "routing"))
        {
            if (NULL==(p_route = ndrx_criterion_get(config, p)))
            {
                xmlFree(p);
                EXFAIL_OUT(ret);
            }
        }
        
        xmlFree(p);
    }
    
    if (NULL==p_route)
    {
        NDRX_LOG(log_error, "(%s) Missing `routing' attribute",
                G_sys_config.config_file_short);
        NDRXD_set_error_fmt(NDRXD_ECFGINVLD, "(%s) Missing `routing' attribute",
                G_sys_config.config_file_short);
        EXFAIL_OUT(ret);
    }
    
    /* allocate criterion, so that we have place to work */
    p_crit = NDRX_MALLOC(sizeof(*p_crit)); 
    
    if (NULL==p_crit)
    {
        NDRX_LOG(log_error, "(%s) Failed to malloc %d bytes", 
                G_sys_config.config_file_short, *p_crit);
        NDRXD_set_error_fmt(NDRXD_EOS, "(%s) Failed to malloc %d bytes",
                G_sys_config.config_file_short, *p_crit);
        goto out;
    }
    
    memset(p_crit, 0, sizeof(*p_crit));
    
    p_crit->routcrit.fieldtypeid=EXFAIL;
    p_crit->routcrit.buffer_type_id=EXFAIL;
    NDRX_STRCPY_SAFE(p_crit->routcrit.criterion, p_route->criterion);
    
    
    /* Now grab the all stuff for server */
    cur=cur->children;
    for (; cur; cur=cur->next)
    {
        p = (char *)xmlNodeGetContent(cur);
        
        if (0==strcmp("field", (char *)cur->name))
        {
            NDRX_STRCPY_SAFE(p_crit->routcrit.field, p);
        }
        else if (0==strcmp("ranges", (char *)cur->name))
        {
            if (NULL!=p_crit->ranges)
            {
                NDRX_LOG(log_error, "(%s) <ranges> already loaded for route [%s]", 
                        G_sys_config.config_file_short, p_crit->routcrit.criterion);
                NDRXD_set_error_fmt(NDRXD_ECFGINVLD, "(%s) <ranges> already loaded for route [%s]", 
                        G_sys_config.config_file_short, p_crit->routcrit.criterion);
                xmlFree(p);
                EXFAIL_OUT(ret);
            }
            
            p_crit->ranges=NDRX_STRDUP(p);
            if (NULL==p_crit->ranges)
            {
                len = strlen(p);
                NDRX_LOG(log_error, "(%s) Failed to malloc %d bytes", 
                        G_sys_config.config_file_short, len);
                NDRXD_set_error_fmt(NDRXD_EOS, "(%s) Failed to malloc %d bytes",
                        G_sys_config.config_file_short, len);
                xmlFree(p);
                EXFAIL_OUT(ret);
            }
        }
        else if (0==strcmp("buftype", (char *)cur->name))
        {
            /* Resolve type -> currently only UBF/ supported 
             * Thus we do not check the sub-types
             * Probably in future if we support view, then on the fly string
             * needs to be scanned with substr for the view/field.
             */
            descr=ndrx_get_buffer_descr(p, NULL);
            
            if (NULL==descr || BUF_TYPE_UBF!=descr->type_id)
            {
                NDRX_LOG(log_error, "(%s) Invalid <routing> buftype [%s]",
                        G_sys_config.config_file_short, p);
                NDRXD_set_error_fmt(NDRXD_EINVAL, "(%s) Invalid <routing> buftype [%s]",
                        G_sys_config.config_file_short, p);
                xmlFree(p);
                EXFAIL_OUT(ret);
            }
            
            NDRX_STRCPY_SAFE(p_crit->routcrit.buftype, p);
            NDRX_STRCPY_SAFE(p_crit->buftype, p);
            p_crit->routcrit.buffer_type_id = descr->type_id;
            
            /* Check that this type range is not already allocated */
            EXHASH_FIND_STR(p_route->btypes, p, el);
            
            if (NULL!=el)
            {
                NDRX_LOG(log_error, "(%s) Buffer type [%s] already defined for route [%s]",
                        G_sys_config.config_file_short, p, p_route->criterion);
                NDRXD_set_error_fmt(NDRXD_EINVAL, "(%s) Buffer type [%s] already defined for route [%s]",
                        G_sys_config.config_file_short, p, p_route->criterion);
                xmlFree(p);
                EXFAIL_OUT(ret);
            }   
            
        }
        else if (0==strcmp("fieldtype", (char *)cur->name))
        {
            /* This is field type according to which we plan to route 
             */
            for (i=0; i<N_DIM(typemap); i++)
            {
                if (0==strcmp(typemap[i].typecode, p))
                {
                    p_crit->routcrit.fieldtypeid = typemap[i].typeid;
                    break;
                }
            }
            
            if (EXFAIL==p_crit->routcrit.fieldtypeid)
            {
                NDRX_LOG(log_error, "(%s) Invalid field type [%s]",
                        G_sys_config.config_file_short, p);
                NDRXD_set_error_fmt(NDRXD_EINVAL, "(%s) Invalid field type [%s]",
                        G_sys_config.config_file_short, p);
                xmlFree(p);
                EXFAIL_OUT(ret);
            }
        }
        
        xmlFree(p);
    }

    /* Resolve field & type 
     * Maybe in future we want to move this configuation to per buffer type
     * callbacks.
     */
    if (BUF_TYPE_UBF==p_crit->routcrit.buffer_type_id)
    {
        
        p_crit->routcrit.fldid = Bfldid(p_crit->routcrit.field);
        
        if (BBADFLDID==p_crit->routcrit.fldid)
        {
            NDRX_LOG(log_error, "(%s) Invalid routing field [%s] for route [%s]",
                        G_sys_config.config_file_short, 
                    p_crit->routcrit.field, p_route->criterion);
            NDRXD_set_error_fmt(NDRXD_EINVAL, "(%s) Invalid routing field [%s] for route [%s]",
                        G_sys_config.config_file_short, 
                    p_crit->routcrit.field, p_route->criterion);
            EXFAIL_OUT(ret);
        }
        
        /* if we go the field then type  */
        
        /* detect the routing type at which we plan to work */
        if (EXFAIL==p_crit->routcrit.fieldtypeid)
        {
            p_crit->routcrit.fieldtypeid = Bfldtype(p_crit->routcrit.fldid);
            
            if (EXFAIL==p_crit->routcrit.fieldtypeid)
            {
                NDRX_LOG(log_error, "(%s) Failed to detect field type [%s]: %s",
                            G_sys_config.config_file_short, 
                        p_crit->routcrit.field, Bstrerror(Berror));
                NDRXD_set_error_fmt(NDRXD_EINVAL, "(%s) Failed to detect field type [%s]: %s",
                            G_sys_config.config_file_short, 
                        p_crit->routcrit.field, Bstrerror(Berror));
                EXFAIL_OUT(ret);
            }
        }
        
        /* configure route type... */
        
        if (BFLD_SHORT == p_crit->routcrit.fieldtypeid
                || BFLD_LONG == p_crit->routcrit.fieldtypeid
                )
        {
            p_crit->routcrit.fieldtypeid = BFLD_LONG;
        }
        else if (BFLD_FLOAT == p_crit->routcrit.fieldtypeid
                || BFLD_DOUBLE == p_crit->routcrit.fieldtypeid
                )
        {
            p_crit->routcrit.fieldtypeid = BFLD_DOUBLE;
        }
        else if (BFLD_STRING == p_crit->routcrit.fieldtypeid
                || BFLD_CARRAY == p_crit->routcrit.fieldtypeid
                || BFLD_CHAR == p_crit->routcrit.fieldtypeid
                )
        {
            p_crit->routcrit.fieldtypeid = BFLD_STRING;
        }
        else
        {
            NDRX_LOG(log_error, "(%s) Unroutable UBF field [%s] type [%s] (%d)",
                            G_sys_config.config_file_short, 
                            p_crit->routcrit.field, Btype(p_crit->routcrit.fldid),
                            p_crit->routcrit.fieldtypeid);
            NDRXD_set_error_fmt(NDRXD_EINVAL, "(%s) Unroutable UBF field [%s] type [%s] (%d)",
                            G_sys_config.config_file_short, 
                            p_crit->routcrit.field, Btype(p_crit->routcrit.fldid),
                            p_crit->routcrit.fieldtypeid);
            EXFAIL_OUT(ret);
        }
    }
    
    /* parse the ranges, either range is string based
     * or range is float based
     */
    
    if (EXSUCCEED!=ndrx_parse_range(p_crit))
    {
        NDRX_LOG(log_error, "Failed to parse range");
        EXFAIL_OUT(ret);
    }
    
    
    NDRX_LOG(log_debug, "--- Adding Route [%s] range ---", p_route->criterion);
    
    
    NDRX_LOG(log_debug, "criterion: [%s]", p_crit->routcrit.criterion);
    
    NDRX_LOG(log_debug, "criterionid: [%d]", p_crit->routcrit.criterionid);
    NDRX_LOG(log_debug, "len: [%d]", p_crit->routcrit.len);
    NDRX_LOG(log_debug, "field: [%s]", p_crit->routcrit.field);
    NDRX_LOG(log_debug, "buffer_type_id: [%hd]", p_crit->routcrit.buffer_type_id);
    NDRX_LOG(log_debug, "buftype: [%s]", p_crit->routcrit.buftype);
    NDRX_LOG(log_debug, "fieldtype: [%s]", p_crit->routcrit.fieldtype);
    NDRX_LOG(log_debug, "fieldtypeid: [%d]", p_crit->routcrit.fieldtypeid);
    NDRX_LOG(log_debug, "fldid: [%d]", p_crit->routcrit.fldid);
    NDRX_LOG(log_debug, "rangesnr: [%ld]", p_crit->routcrit.rangesnr);
    
    NDRX_LOG(log_debug, "-------------------------------------");
    
    /* if parsed ok, add to type hash, not ?  */
    EXHASH_ADD_STR(p_route->btypes, buftype, p_crit);
    
out:

    if (EXSUCCEED!=ret && NULL!=p_crit)
    {
        ndrx_routcrit_type_free(p_crit);
    
    }
                
    return ret;
}


/**
 * Parse routing entry
 * @param config config in progress
 * @param doc
 * @param cur
 * @return 
 */
expublic int ndrx_routing_parse(config_t *config, xmlDocPtr doc, xmlNodePtr cur)
{
    int ret=EXSUCCEED;
    
    for (; cur ; cur=cur->next)
    {
        if (0==strcmp((char*)cur->name, "route"))
        {
            /* Get the server name */
            if (EXSUCCEED!=ndrx_route_parse(config, doc, cur))
            {
                NDRXD_set_error_fmt(NDRXD_ECFGINVLD, "(%s) Failed to "
                        "parse <route> section", G_sys_config.config_file_short);
                ret=EXFAIL;
                goto out;
            }
        }
    }
out:
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
