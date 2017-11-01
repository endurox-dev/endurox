/* 
** UBF buffer support
**
** @file typed_ubf.c
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <utlist.h>

#include <tperror.h>
#include <ubf.h>
#include <atmi.h>
#include <typed_buf.h>
#include <ndebug.h>
#include <fdatatype.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define UBF_DEFAULT_SIZE    1024
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
/*
 * Prepare buffer for outgoing call/reply
 * idata - buffer data
 * ilen - data len (if needed)
 * obuf - place where to copy prepared buffer
 * olen - the actual lenght of data that should sent. Also this may represent
 *          space for the buffer to copy to.
 */
expublic int UBF_prepare_outgoing (typed_buffer_descr_t *descr, char *idata, 
        long ilen, char *obuf, long *olen, long flags)
{
    int ret=EXSUCCEED;
    UBFH *p_ub = (UBFH *)idata;
    int ubf_used;
    char fn[]="UBF_prepare_outgoing";
    UBF_header_t *hdr;
    if (EXFAIL==(ubf_used=Bused(p_ub)))
    {
        ndrx_TPset_error_msg(TPEINVAL, Bstrerror(Berror));
        ret=EXFAIL;
        goto out;
    }

    /* Check that we have space enough to prepare for send */
    if (NULL!=olen && 0!=*olen && *olen < ubf_used)
    {
        ndrx_TPset_error_fmt(TPEINVAL, "%s: Internal buffer space: %d, "
                "but requested: %d", fn, *olen, ubf_used);
        ret=EXFAIL;
        goto out;
    }

    memcpy(obuf, idata, ubf_used);
    /* Now re-size FB, so that buffer is correct */
    hdr = (UBF_header_t *) obuf;

    hdr->buf_len = ubf_used;
    /* return the actual length! */
    if (NULL!=olen)
        *olen = ubf_used;
    
out:
    return ret;
}

/*
 * Prepare received buffer for internal processing.
 * May re-allocate the buffer.
 * rcv_data - received data buffer
 * odata - ptr to handler. Existing buffer may be reused or re-allocated
 * olen - output data length
 */
expublic int UBF_prepare_incoming (typed_buffer_descr_t *descr, char *rcv_data, 
                        long rcv_len, char **odata, long *olen, long flags)
{
    int ret=EXSUCCEED;
    long rcv_buf_size;
    long existing_size;
    UBFH *p_ub = (UBFH *)rcv_data;
    UBFH *p_ub_out;
    char fn[]="UBF_prepare_incoming";
    buffer_obj_t *outbufobj=NULL;

    NDRX_LOG(log_debug, "Entering %s", fn);
    if (EXFAIL==(rcv_buf_size=Bused(p_ub)))
    {
        ndrx_TPset_error_msg(TPEINVAL, Bstrerror(Berror));
        ret=EXFAIL;
        goto out;
    }

    /* Figure out the passed in buffer */
    if (NULL!=*odata && NULL==(outbufobj=ndrx_find_buffer(*odata)))
    {
        ndrx_TPset_error_fmt(TPEINVAL, "Output buffer %p is not allocated "
                                        "with tpalloc()!", odata);
        ret=EXFAIL;
        goto out;
    }

    /* Check the data types */
    if (NULL!=outbufobj)
    {
        /* If we cannot change the data type, then we trigger an error */
        if (flags & TPNOCHANGE && outbufobj->type_id!=BUF_TYPE_UBF)
        {
            /* Raise error! */
            ndrx_TPset_error_fmt(TPEINVAL, "Receiver expects %s but got %s buffer",
                                        G_buf_descr[BUF_TYPE_UBF],
                                        G_buf_descr[outbufobj->type_id]);
            ret=EXFAIL;
            goto out;
        }
        /* If we can change data type and this does not match, then
         * we should firstly free it up and then bellow allow to work in mode
         * when odata is NULL!
         */
        if (outbufobj->type_id!=BUF_TYPE_UBF)
        {
            NDRX_LOG(log_warn, "User buffer %d is different, "
                    "free it up and re-allocate as UBF", G_buf_descr[outbufobj->type_id]);
            ndrx_tpfree(*odata, outbufobj);
            *odata=NULL;
        }
    }
    
    /* check the output buffer */
    if (NULL!=*odata)
    {
        p_ub_out = (UBFH *)*odata;
        NDRX_LOG(log_debug, "%s: Output buffer exists", fn);
        
        if (EXFAIL==(existing_size=Bsizeof(p_ub_out)))
        {
            ndrx_TPset_error_msg(TPEINVAL, Bstrerror(Berror));
            ret=EXFAIL;
            goto out;
        }

        NDRX_LOG(log_debug, "%s: Output buffer size: %d, received %d", fn,
                            existing_size, rcv_buf_size);
        
        if (existing_size>=rcv_buf_size)
        {
            /* re-use existing buffer */
            NDRX_LOG(log_debug, "%s: Using existing buffer", fn);
        }
        else
        {
            /* Reallocate the buffer, because we have missing some space */
            char *new_addr;
            NDRX_LOG(log_debug, "%s: Reallocating", fn);
            
            if (NULL==(new_addr=ndrx_tprealloc(*odata, rcv_buf_size)))
            {
                NDRX_LOG(log_error, "%s: _tprealloc failed!", fn);
                ret=EXFAIL;
                goto out;
            }

            /* allocated OK, return new address */
            *odata = new_addr;
            p_ub_out = (UBFH *)*odata;
        }
    }
    else
    {
        /* allocate the buffer */
        NDRX_LOG(log_debug, "%s: Incoming buffer where missing - "
                "allocating new (size: %d)!",  fn, rcv_buf_size);

        *odata = ndrx_tpalloc(&G_buf_descr[BUF_TYPE_UBF], NULL, NULL, rcv_buf_size);

        if (NULL==*odata)
        {
            /* error should be set already */
            NDRX_LOG(log_error, "Failed to allocat new buffer!");
            goto out;
        }

        p_ub_out = (UBFH *)*odata;
    }

    /* Do the actual data copy */
    if (EXSUCCEED!=Bcpy(p_ub_out, p_ub))
    {
        ret=EXFAIL;
        NDRX_LOG(log_error, "Bcpy failed!: %s", Bstrerror(Berror));
        ndrx_TPset_error_msg(TPEOS, Bstrerror(Berror));
        goto out;
    }

out:
    return ret;
}

/**
 * Allocate the buffer & register this into list, that we have such
 * List maintenance should be done by parent process tpalloc!
 * @param subtype
 * @param len
 * @return
 */
expublic char * UBF_tpalloc (typed_buffer_descr_t *descr, char *subtype, long *len)
{
    char *ret=NULL;

    if (0==*len)
    {
        *len = UBF_DEFAULT_SIZE;
    }

    /* Allocate UBF buffer */
    ret=(char *)Balloc(1, *len);

    if (NULL==ret)
    {
        NDRX_LOG(log_error, "%s: Failed to allocate UBF buffer!", __func__);
        ndrx_TPset_error_msg(TPEOS, Bstrerror(Berror));
        goto out;
    }

out:
    return ret;
}

/**
 * Re-allocate UBF buffer. Firstly we will find it in the list.
 * @param ptr
 * @param size
 * @return
 */
expublic char * UBF_tprealloc(typed_buffer_descr_t *descr, char *cur_ptr, long len)
{
    char *ret=NULL;
    UBFH *p_ub = (UBFH *)cur_ptr;
    char fn[] = "UBF_tprealloc";

    if (0==len)
    {
        len = UBF_DEFAULT_SIZE;
    }

    /* Allocate UBF buffer */
    ret=(char *)Brealloc(p_ub, 1, len);

    if (NULL==ret)
    {
        NDRX_LOG(log_error, "%s: Failed to allocate UBF buffer!", fn);
        ndrx_TPset_error_msg(TPEOS, Bstrerror(Berror));
    }

    return ret;
}

/**
 * Gracefully remove free up the buffer
 * @param descr
 * @param buf
 */
expublic void UBF_tpfree(typed_buffer_descr_t *descr, char *buf)
{
    Bfree((UBFH *)buf);
}

/**
 * Check the boolean expression
 * @param descr
 * @param buf
 * @param expr
 * @return TRUE/FALSE.
 * In case of error we just return FALSE as not matched!
 */
expublic int UBF_test(typed_buffer_descr_t *descr, char *buf, BFLDLEN len, char *expr)
{
    int ret=EXFALSE;
    char *tree;

    /* compile the xpression */
    if (NULL==(tree=Bboolco (expr)))
    {
        NDRX_LOG(log_error, "Failed to compile expression [%s], err: %s",
                                    Bstrerror(Berror));
    }

    ret=Bboolev((UBFH *)buf, tree);
    
    /* Free up the buffer */
    Btreefree(tree);

out:
    return ret;
}
