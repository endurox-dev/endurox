/**
 * @brief BFLD_VIEW type support
 *
 * @file fld_view.c
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

/*---------------------------Includes-----------------------------------*/
#include <stdio.h>
#include <stdarg.h>
#include <memory.h>
#include <stdlib.h>
#include <regex.h>
#include <ubf.h>
#include <ubf_int.h>
#include <ndebug.h>
#include <ferror.h>
#include <errno.h>
#include <ubf_tls.h>
#include <ubfutil.h>

#include "ubfview.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define VIEW_SIZE_OVERHEAD   (sizeof(BFLDLEN)+NDRX_VIEW_NAME_LEN+1)
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Get total size of the view
 * @param t type descriptor
 * @param fb data in fb
 * @param payload_size return view size
 * @return total size in bytes, aligned
 */
expublic int ndrx_get_fb_view_size(dtype_str_t *t, char *fb, int *payload_size)
{
    UBF_view_t *view = (UBF_view_t *)fb;
    
    if (NULL!=payload_size)
    {
        /* what size? */
        *payload_size = view->dlen;
    }
    
    return ALIGNED_SIZE_T(UBF_view_t, view->dlen);
}

/**
 * Put the buffer data.
 * If we cannot extract the data length from the view, we shall generate
 * error: BBADVIEW
 * @param t type descriptor
 * @param fb UBF buffer position for field
 * @param bfldid field id
 * @param data must be UBF type buffer
 * @param len not used, we take datak
 * @return status (BBADVIEW if view not found...)
 */
expublic int ndrx_put_data_view(dtype_str_t *t, char *fb, BFLDID bfldid, 
        char *data, int len)
{
    int ret = EXSUCCEED;
    UBF_view_t *viewfb = (UBF_view_t *)fb;
    BVIEWFLD *vdata = (BVIEWFLD *)data;
    ndrx_typedview_t * vdef;
    long vsize;
    VIEW_ENTRY;
    
    /* find the view */
    
    if (EXEOS!=vdata->vname[0])
    {
        vdef = ndrx_view_get_view(vdata->vname);

        if (NULL==vdef)
        {
            ndrx_Bset_error_fmt(BBADVIEW, "View [%s] not loaded, check VIEWFILES env");
            EXFAIL_OUT(ret);
        }
        
        vsize=vdef->ssize;
    }
    else
    {
        /* for empty fields */
        vsize=0;
    }
    
    /* int align; */
    viewfb->bfldid = bfldid;
    viewfb->vflags = vdata->vflags;
    viewfb->dlen=vsize;
    
    NDRX_STRCPY_SAFE(viewfb->vname, vdata->vname);
    
    if (vsize>0)
    {
        memcpy(viewfb->data, vdata->data, vsize);
    }
    
out:
    return ret;
}

/**
 * Return estimated required data size
 * This size is used for calculation of data adding, not for retrieval
 * the real data used by view.
 * @param t type descr
 * @param data data
 * @param len passed len, not used
 * @return bytes needed. 
 */
expublic int ndrx_get_d_size_view (struct dtype_str *t, char *data, 
        int len, int *payload_size)
{
    int ret;
    BVIEWFLD *vdata = (BVIEWFLD *)data;
    ndrx_typedview_t * vdef;
    long vsize;
    VIEW_ENTRY;
    
    /* find the view */
    
    if (EXEOS!=vdata->vname[0])
    {
        vdef = ndrx_view_get_view(vdata->vname);

        if (NULL==vdef)
        {
            ndrx_Bset_error_fmt(BBADVIEW, "View [%s] not loaded, check VIEWFILES env", 
                    vdata->vname);
            EXFAIL_OUT(ret);
        }
        vsize=vdef->ssize;
    }
    else
    {
        vsize=0;
    }
    
    if (NULL!=payload_size)
        *payload_size=vsize;

    ret=ALIGNED_SIZE_T(UBF_view_t, vsize);
    
out:
    return ret;
}

/**
 * Get data, data is unloaded to BVIEWFLD. It is expected that BVIEWFLD.data
 * has enough space to hold the buffer.
 * @param t data type descr
 * @param fb field position in UBF buffer
 * @param buf output buffer
 * @param len output buffer len. 
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_get_data_view (struct dtype_str *t, char *fb, char *buf, int *len)
{
    int ret=EXSUCCEED;
    UBF_view_t *viewfb = (UBF_view_t *)fb;
    BVIEWFLD *vdata = (BVIEWFLD *)buf;

    
    if (NULL!=len && *len>0 && *len < viewfb->dlen)
    {
        /* Set error, that string buffer too short */
        ndrx_Bset_error_fmt(BNOSPACE, "output buffer too short. Data len %d in buf, "
                                "output: %d", viewfb->dlen, *len);
        EXFAIL_OUT(ret);
    }
    
    /* copy the data */
    NDRX_STRCPY_SAFE(vdata->vname, viewfb->vname);
    vdata->vflags = (unsigned int )viewfb->vflags;
    memcpy(vdata->data, viewfb->data, viewfb->dlen);

    if (NULL!=len)
    {
        *len=viewfb->dlen;
    }

out:
    return ret;
}

/**
 * Return aligned empty view size
 * @param t data type
 * @return 
 */
expublic int ndrx_g_view_empty(struct dtype_ext1* t)
{
    return ALIGNED_SIZE_T(UBF_view_t, 0);
}

/**
 * Put empty UBF buffer in UBF buffer
 * @param t type descr
 * @param fb UBF buffer where to install the data
 * @param bfldid field id to set
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_put_empty_view(struct dtype_ext1* t, char *fb, BFLDID bfldid)
{
    int ret=EXSUCCEED;
    UBF_view_t *viewfb = (UBF_view_t *)fb;
    
    memset(viewfb, 0, sizeof(UBF_view_t));    
    viewfb->bfldid = bfldid;

    return ret;
}

/**
 * Print the buffer to log
 * Field must match the buffer format of UBF_view_t
 * @param t type descr
 * @param text extra debug msg
 * @param data ptr to UBF_view_t
 * @param len not used..
 */
expublic void ndrx_dump_view(struct dtype_ext1 *t, char *text, char *data, int *len)
{
    UBF_view_t *viewfb = (UBF_view_t *)data;
    
    if (NULL!=data)
    {
        NDRX_LOG(log_debug, "%s: View [%s] vflags [%ld]", text, 
                viewfb->vname, viewfb->vflags);
        ndrx_debug_dump_VIEW_ubflogger(log_debug, text, viewfb->data, viewfb->vname);
    }
    else
    {
        UBF_LOG(log_debug, "%s:\n[null data]", text);
    }
}

/**
 * Compare two views
 * This shall point to BVIEWFLD. So that later any finds and next ops
 * would return the same BVIEWFLD and could be used in existing places where
 * data is compared by value, and not by headers.
 * @param t data type
 * @param val1 ptr in buffer
 * @param len1 not used
 * @param val2 ptr in buffer
 * @param len2 not used
 * @param mode UBF_CMP_MODE_STD? 
 * @return EXTRUE/EXFALSE, or -1,0,1,-2 on error
 */
expublic int ndrx_cmp_view (struct dtype_ext1 *t, char *val1, BFLDLEN len1, 
        char *val2, BFLDLEN len2, long mode)
{
    int ret = 0;
    
    BVIEWFLD *vdata1 = (BVIEWFLD *)val1;
    BVIEWFLD *vdata2 = (BVIEWFLD *)val2;
    
    if (mode & UBF_CMP_MODE_STD)
    {
        ret = Bvcmp(vdata1->data, vdata1->vname, 
                vdata2->data, vdata2->vname);
        goto out;
    }
    else
    {
        ret = Bvcmp(vdata1->data, vdata1->vname, 
                vdata2->data, vdata2->vname);
        
        if (-2==ret)
        {
            goto out;
        }
        
        if (0==ret)
        {
            ret=EXTRUE;
        }
        else
        {
            ret=EXFALSE;
        }
    }
    
out:
    return ret;
}

/**
 * Allocate buffer for view (in case of Bgetalloc()).
 * Also prepare internal structures -> calculate data pointer.
 * We return real size + extra if requested.
 * @param t
 * @param len - buffer length to allocate.
 * @return NULL/ptr to allocated mem
 */
expublic char *ndrx_talloc_view (struct dtype_ext1 *t, int *len)
{
    char *ret=NULL;
    int alloc_size = *len;
    BVIEWFLD *ptr;
    
    ret= NDRX_MALLOC(sizeof(BVIEWFLD) + alloc_size);
    
    ptr = (BVIEWFLD *)ret;
    
    if (NULL==ret)
    {
        ndrx_Bset_error_fmt(BMALLOC, "Failed to allocate %d bytes (with hdr) for user", 
                sizeof(BVIEWFLD) + alloc_size);
    }
    else
    {
        *len = alloc_size;
        /* this is the trick, we alloc one block with data to self offset */
        ptr->data = ret + sizeof(BVIEWFLD);
    }

    return ret;
}

/**
 * Prepare view data for presenting to user.
 * This will setup BVIEWFLD and return the pointer to it
 * @param t type descriptor
 * @param storage where to store the returned data ptr
 * @param data start in UBF buffer
 * @return ptr to data block of BVIEWFLD
 */
expublic char* ndrx_prep_viewp (struct dtype_ext1 *t, 
        ndrx_ubf_tls_bufval_t *storage, char *data)
{
    UBF_view_t *viewfb = (UBF_view_t *)data;

    NDRX_STRCPY_SAFE(storage->vdata.vname, viewfb->vname);
    storage->vdata.vflags = (unsigned int)viewfb->vflags;
    storage->vdata.data=viewfb->data;

    return (char *)&storage->vdata;
}

/* vim: set ts=4 sw=4 et smartindent: */
