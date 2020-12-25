/**
 * @brief VIEW buffer type support
 *   Basically all data is going to be stored in FB.
 *   For view not using base as 3000. And the UBF field numbers follows the
 *   the field number in the view (1, 2, 3, 4, ...)
 *   Sample file:
 *   #
 *   #type    cname   fbname count flag  size  null
 *   #
 *   int     in    -   1   -   -   -
 *   short    sh    -   2   -   -   -
 *   long     lo    -   3   -   -   -
 *   char     ch    -   1   -   -   -
 *   float    fl    -   1   -   -   -
 *   double    db    -   1   -   -   -
 *   string    st    -   1   -   15   -
 *   carray    ca    -   1   -   15   -
 *   END
 *
 * @file typed_view.c
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
#include <dirent.h>

#include <ndrstandard.h>
#include <typed_buf.h>
#include <ndebug.h>
#include <tperror.h>
#include <ubfutil.h>

#include <userlog.h>
#include <typed_view.h>
#include <view_cmn.h>
#include <atmi_tls.h> 
#include <fieldtable.h>

#include "Exfields.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
/**
 * Allocate the view object
 * @param descr
 * @param subtype
 * @param len
 * @return 
 */
expublic char * VIEW_tpalloc (typed_buffer_descr_t *descr, char *subtype, long *len)
{
    char *ret=NULL;
    ndrx_typedview_t *v;

    if (EXSUCCEED!=ndrx_prepare_type_tables())
    {
        ndrx_TPset_error_fmt(TPESYSTEM, "%s: Failed to load UBF FD files:%s",
            __func__, Bstrerror(Berror));
        ret = NULL;
        goto out;
    }
    
    if (EXSUCCEED!=ndrx_view_init())
    {
        ndrx_TPset_error_fmt(TPESYSTEM, "%s: Failed to load view files:%s",
            __func__, Bstrerror(Berror));
        ret = NULL;
        goto out;
    }
            
    v = ndrx_view_get_view(subtype);
    
    if (NULL==v)
    {
        NDRX_LOG(log_error, "%s: VIEW [%s] NOT FOUND!", __func__, subtype);
        ndrx_TPset_error_fmt(TPENOENT, "%s: VIEW [%s] NOT FOUND!", 
                __func__, subtype);
        
        goto out;
    }
    
    if (NDRX_VIEW_SIZE_DEFAULT_SIZE>*len)
    {
        *len = NDRX_VIEW_SIZE_DEFAULT_SIZE;
    }
    else if (v->ssize < *len)
    {
        NDRX_LOG(log_warn, "VIEW [%s] structure size is %ld, requested %ld -> "
		    "upgrading to view size!", subtype, *len, v->ssize);   
        *len = v->ssize;
    }
    
    /* Allocate VIEW buffer */
    /* Maybe still malloc? */
    ret=(char *)NDRX_CALLOC(1, *len);

    if (NULL==ret)
    {
        NDRX_LOG(log_error, "%s: Failed to allocate VIEW buffer!", __func__);
        ndrx_TPset_error_msg(TPEOS, Bstrerror(Berror));
        goto out;
    }
    
    if (EXSUCCEED!=ndrx_Bvsinit_int(v, ret))
    {
        NDRX_LOG(log_error, "%s: Failed to init view: %s!", 
		    __func__, Bstrerror(Berror));

        ndrx_TPset_error_fmt(TPESYSTEM, "%s: Failed to init view: %s!", 
		    __func__, Bstrerror(Berror));

        goto out;    
    }
    
out:
    return ret;
}

/**
 * Re-allocate CARRAY buffer. Firstly we will find it in the list.
 * @param ptr
 * @param size
 * @return
 */
expublic char * VIEW_tprealloc(typed_buffer_descr_t *descr, char *cur_ptr, long len)
{
    char *ret=NULL;

    if (0==len)
    {
        len = NDRX_VIEW_SIZE_DEFAULT_SIZE;
    }

    /* Allocate CARRAY buffer */
    ret=(char *)NDRX_REALLOC(cur_ptr, len);
    

    return ret;
}

/**
 * Build the outgoing buffer... This will convert the VIEW to internal UBF
 * format.
 * 
 * @param descr
 * @param idata
 * @param ilen
 * @param obuf
 * @param olen
 * @param flags
 * @return 
 */
expublic int VIEW_prepare_outgoing (typed_buffer_descr_t *descr, char *idata, long ilen, 
                    char *obuf, long *olen, long flags)
{
    int ret = EXSUCCEED;
    ndrx_view_header *hdr;
    buffer_obj_t *bo;
    ndrx_typedview_t *v;
    
    /* WARNING!!! views must be loaded already... 
     * otherwise not way to allocate the VIEW buffer...
     */
    /* get the view */
    
    bo = ndrx_find_buffer(idata);
 
    if (NULL==bo)
    {
        ndrx_TPset_error_fmt(TPEINVAL, "Input buffer not allocated by tpalloc!");
        EXFAIL_OUT(ret);
    }
    
    NDRX_DUMP(log_dump, "Outgoing VIEW struct", idata, ilen);
    
    NDRX_LOG(log_debug, "Preparing outgoing for VIEW [%s]", bo->subtype);
    
    v = ndrx_view_get_view(bo->subtype);
    
    if (NULL==v)
    {
        ndrx_TPset_error_fmt(TPEINVAL, "View not found [%s]!", bo->subtype);
        EXFAIL_OUT(ret);
    }
    
    /* Check that we have space enough to prepare for send 
     * TODO: send also view name
     * TODO: Do we nee
     */
    
    ilen = v->ssize + sizeof(ndrx_view_header);
    
    if (NULL!=olen && *olen < ilen)
    {
        ndrx_TPset_error_fmt(TPEINVAL, "Internal buffer space: %d, "
                "but requested: %d", *olen, ilen);
        ret=EXFAIL;
        goto out;
    }
    
    hdr = (ndrx_view_header *)obuf;
    
    hdr->vflags=0;
    hdr->cksum = v->cksum;
    NDRX_STRCPY_SAFE(hdr->vname, v->vname);

    memcpy(hdr->data, idata, v->ssize);
    
    /* return the actual length! */
    if (NULL!=olen)
    {
        *olen = ilen;
    }
    
out:
    
    return ret;
}

/**
 * Prepare incoming buffer. the rcv_data is non xatmi buffer. Thus we will
 * just make ptr 
 * @param descr
 * @param rcv_data
 * @param rcv_len
 * @param odata
 * @param olen
 * @param flags
 * @return 
 */
expublic int VIEW_prepare_incoming (typed_buffer_descr_t *descr, char *rcv_data, 
                        long rcv_len, char **odata, long *olen, long flags)
{
    int ret=EXSUCCEED;
    long existing_size;
    ndrx_view_header *p_hdr = (ndrx_view_header *)rcv_data;
    char *p_out;
    buffer_obj_t *outbufobj=NULL;
    ndrx_typedview_t *v;
    
    NDRX_LOG(log_debug, "Entering %s", __func__);
    
    if (EXSUCCEED!=ndrx_view_init())
    {
        ndrx_TPset_error_fmt(TPESYSTEM, "%s: Failed to load view files!",  __func__);
        EXFAIL_OUT(ret);
    }
    
    /* Resolve the view data, get the size... */
    
    v = ndrx_view_get_view(p_hdr->vname);
    
    if (NULL==v)
    {
        userlog("View [%s] not defined!", p_hdr->vname);
        ndrx_TPset_error_fmt(TPEINVAL, "View [%s] not defined!", p_hdr->vname);
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_debug, "Received VIEW [%s]", p_hdr->vname);

    /* Figure out the passed in buffer */
    if (NULL==(outbufobj=ndrx_find_buffer(*odata)))
    {
        ndrx_TPset_error_fmt(TPEINVAL, "Output buffer %p is not allocated "
                                        "with tpalloc()!", *odata);
        ret=EXFAIL;
        goto out;
    }

    /* Check the data types */
    if (NULL!=outbufobj)
    {
        /* If we cannot change the data type, then we trigger an error */
        if (flags & TPNOCHANGE && (outbufobj->type_id!=BUF_TYPE_VIEW ||
                0!=strcmp(outbufobj->subtype, p_hdr->vname)))
        {
            /* Raise error! */
            ndrx_TPset_error_fmt(TPEOTYPE, "Receiver expects %s/%s but got %s/%s buffer",
                    G_buf_descr[BUF_TYPE_VIEW].type, 
                    p_hdr->vname,
                    G_buf_descr[outbufobj->type_id].type,
                    outbufobj->subtype);
            EXFAIL_OUT(ret);
        }
        
        /* If we can change data type and this does not match, then
         * we should firstly free it up and then bellow allow to work in mode
         * when odata is NULL!
         */
        if (outbufobj->type_id!=BUF_TYPE_VIEW || 0!=strcmp(outbufobj->subtype, 
                p_hdr->vname) )
        {
            NDRX_LOG(log_info, "User buffer %s/%s is different, "
                    "free it up and re-allocate as VIEW/%s", 
                    G_buf_descr[outbufobj->type_id].type,
                    (outbufobj->subtype==NULL?"NULL":outbufobj->subtype),
                    p_hdr->vname);
            
            ndrx_tpfree(*odata, outbufobj);
            *odata=NULL;
        }
    }
    
    /* check the output buffer */
    if (NULL!=*odata)
    {
        p_out = (char *)*odata;
        NDRX_LOG(log_debug, "%s: Output buffer exists", __func__);
        
        existing_size=outbufobj->size;

        NDRX_LOG(log_debug, "%s: Output buffer size (struct size): %ld, "
                "received %ld", __func__,
                            existing_size, v->ssize);
        
        if (existing_size>=v->ssize)
        {
            /* re-use existing buffer */
            NDRX_LOG(log_debug, "%s: Using existing buffer", __func__);
        }
        else
        {
            /* Reallocate the buffer, because we have missing some space */
            char *new_addr;
            NDRX_LOG(log_debug, "%s: Reallocating", __func__);
            
            if (NULL==(new_addr=ndrx_tprealloc(*odata, v->ssize)))
            {
                NDRX_LOG(log_error, "%s: _tprealloc failed!", __func__);
                EXFAIL_OUT(ret);
            }

            /* allocated OK, return new address */
            *odata = new_addr;
            p_out = *odata;
        }
    }
    else
    {
        /* allocate the buffer */
        NDRX_LOG(log_debug, "%s: Incoming buffer where missing - "
                                         "allocating new!", __func__);

        *odata = ndrx_tpalloc(&G_buf_descr[BUF_TYPE_VIEW], NULL, 
                p_hdr->vname, v->ssize);

        if (NULL==*odata)
        {
            /* error should be set already */
            NDRX_LOG(log_error, "Failed to allocate new buffer!");
            goto out;
        }

        p_out = *odata;
    }
    
    
    /* Verify checksum 
    if (v->cksum!=p_hdr->cksum)
    {
        NDRX_LOG(log_error, "Invalid checksum for VIEW [%s] received. Our cksum: "
                "%ld, their: %ld - try to recompile VIEW with viewc!",
                v->vname, (long)v->cksum, (long)p_hdr->cksum);
        
        ndrx_TPset_error_fmt(TPEINVAL, "Invalid checksum for VIEW [%s] "
                "received. Our cksum: "
                "%ld, their: %ld - try to recompile VIEW with viewc!",
                v->vname, (long)v->cksum, (long)p_hdr->cksum);
        EXFAIL_OUT(ret);
    }
    */
    /* copy off the data */
    memcpy(*odata, p_hdr->data, v->ssize);
    
    if (NULL!=olen)
    {
        *olen = v->ssize;
    }
    
    NDRX_DUMP(log_dump, "Incoming VIEW struct", *odata, v->ssize);
    
out:
    return ret;
}

/**
 * Gracefully remove free up the buffer
 * @param descr
 * @param buf
 */
expublic void VIEW_tpfree(typed_buffer_descr_t *descr, char *buf)
{
    NDRX_FREE(buf);
}


/**
 * Check the expression on buffer.
 * @param descr
 * @param buf
 * @param expr
 * @return TRUE/FALSE.
 * In case of error we just return FALSE as not matched!
 */
expublic int VIEW_test(typed_buffer_descr_t *descr, char *buf, BFLDLEN len, char *expr)
{
    int ret=EXFALSE;

    NDRX_LOG(log_error, "VIEW buffers do not support event filters! Expr: [%s]", expr);
    userlog("VIEW buffers do not support event filters! Expr: [%s]", expr);
    
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
