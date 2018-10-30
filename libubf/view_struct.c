/**
 * @brief VIEW buffer type support - structure management
 *
 * @file view_struct.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * AGPL or Mavimax's license for commercial use.
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
#include <dirent.h>
#include <limits.h>

#include <ndrstandard.h>
#include <ubfview.h>
#include <ndebug.h>

#include <userlog.h>
#include <view_cmn.h>

#include "Exfields.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define API_ENTRY {ndrx_Bunset_error(); \
}\

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
expublic ndrx_typedview_t *ndrx_G_view_hash = NULL;
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Update checksum from given string
 * @param v view object
 * @param str string to add to add to checksum
 * @return 
 */
expublic void ndrx_view_cksum_update(ndrx_typedview_t *v, char *str, int len)
{
    int i;
    uint32_t s;
    long l;
    
#if 0
    UBF_DUMP(log_debug, "ADDING TO CHECKSUM", str, len);
#endif
    
    for (i=0; i<len; i++)
    {
        s = (uint32_t)str[i];
        v->cksum=ndrx_rotl32b(v->cksum, 1);
        
        /* xor the value with intput... */
        v->cksum^=s;
    }
    
    l = v->cksum;
#if 0
    UBF_LOG(log_debug, "Current checksum: %ld", l);
#endif
}

/**
 * Resolve view by name
 * @param vname view name
 * @return  NULL or ptr to view object
 */
expublic ndrx_typedview_t * ndrx_view_get_view(char *vname)
{
    ndrx_typedview_t *ret;
    
    EXHASH_FIND_STR(ndrx_G_view_hash, vname, ret);
    
    return ret;
}

/**
 * Resolve field by name
 * @param v view object
 * @param cname field name to resolve
 * @return  NULL or ptr to view object
 */
expublic ndrx_typedview_field_t * ndrx_view_get_field(ndrx_typedview_t *v, char *cname)
{
    ndrx_typedview_field_t *ret;
    
    EXHASH_FIND_STR(v->fields_h, cname, ret);
    
    return ret;
}


/**
 * Return handle to current view objects
 * @return ndrx_G_view_hash var
 */
expublic ndrx_typedview_t * ndrx_view_get_handle(void)
{
    return ndrx_G_view_hash;
}

/**
 * Delete all objects from memory
 * @return 
 */
expublic void ndrx_view_deleteall(void)
{
    ndrx_typedview_t * vel, *velt;
    ndrx_typedview_field_t * fld, *fldt;
    
    EXHASH_ITER(hh, ndrx_G_view_hash, vel, velt)
    {
        /* Delete all from hash */
        EXHASH_ITER(hh, vel->fields_h, fld, fldt)
        {
            EXHASH_DEL(vel->fields_h, fld);
        }
        
        /* Delete all from linked-list */
        DL_FOREACH_SAFE(vel->fields, fld, fldt)
        {
            DL_DELETE(vel->fields, fld);
            
            NDRX_FREE(fld);
        }
        
        EXHASH_DEL(ndrx_G_view_hash, vel);
        
        NDRX_FREE(vel);
    }
    
}

/**
 * Update view object properties
 * @param vname view name
 * @param ssize struct size
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_view_update_object(char *vname, long ssize)
{
    int ret = EXSUCCEED;
    ndrx_typedview_t * v;

    v = ndrx_view_get_view(vname);
    
    if (NULL==v)
    {
        UBF_LOG(log_error, "Failed to get view object by [%s]", vname);
        UBF_LOG(log_error, "View not found [%s]", vname);
        EXFAIL_OUT(ret);
    }
    
    v->ssize = ssize;
    
    UBF_LOG(log_info, "View [%s] struct size %ld", vname, v->ssize);
    
out:
    return ret;
}

/**
 * Update view offsets
 * @param vname view name 
 * @param p offsets table
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_view_update_offsets(char *vname, ndrx_view_offsets_t *p)
{
    int ret = EXSUCCEED;
    ndrx_typedview_t * v;
    ndrx_typedview_field_t *f;
    
    /* Get the handler and iterate over the hash and here.. */
    v = ndrx_view_get_view(vname);
    
    if (NULL==v)
    {
        UBF_LOG(log_error, "Failed to get view object by [%s]", vname);
        UBF_LOG(log_error, "View not found [%s]", vname);
        EXFAIL_OUT(ret);
    }
    
    DL_FOREACH(v->fields, f)
    {
        if (NULL==p->cname)
        {
            UBF_LOG(log_error, "Field descriptor table does not match v object");
            EXFAIL_OUT(ret);
        }
        else if (0!=strcmp(f->cname, p->cname))
        {
            UBF_LOG(log_error, "Invalid field name, loaded object [%s] "
                    "vs compiled code [%s]",
                        f->cname, p->cname);
            EXFAIL_OUT(ret);
        }
        
        f->offset=p->offset;
        f->fldsize=p->fldsize;
        f->count_fld_offset=p->count_fld_offset;
        f->length_fld_offset=p->length_fld_offset;
        
        p++;
    }
    
out:
    return ret;
    
}
/* vim: set ts=4 sw=4 et smartindent: */
