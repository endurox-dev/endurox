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
    int have_value;
    int pos;
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

/* TODO: Resolve services */

/* TODO: Check routing & return group */

/* TODO: Resolve service name by routing? */

/* TODO: NDRXD apply config to shared mem with with the timeout setting */


/* vim: set ts=4 sw=4 et smartindent: */
