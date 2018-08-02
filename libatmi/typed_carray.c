/* 
 * @brief CARRAY buffer support
 *
 * @file typed_carray.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or ATR Baltic's license for commercial use.
 * -----------------------------------------------------------------------------
 * GPL license:
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
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
#include <utlist.h>

#include <tperror.h>
#include <ubf.h>
#include <atmi.h>
#include <typed_buf.h>
#include <ndebug.h>
#include <fdatatype.h>
#include <userlog.h>
#include <typed_carray.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define CARRAY_DEFAULT_SIZE    512
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
expublic int CARRAY_prepare_outgoing (typed_buffer_descr_t *descr, char *idata, long ilen, 
                    char *obuf, long *olen, long flags)
{
    int ret=EXSUCCEED;
    char fn[]="CARRAY_prepare_outgoing";
    
    /* Check that we have space enought to prepare for send */
    if (NULL!=olen && 0!=*olen && *olen < ilen)
    {
        ndrx_TPset_error_fmt(TPEINVAL, "%s: Internal buffer space: %d, "
                "but requested: %d", fn, *olen, ilen);
        ret=EXFAIL;
        goto out;
    }

    memcpy(obuf, idata, ilen);
    
    /* return the actual length! */
    if (NULL!=olen)
        *olen = ilen;
    
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
expublic int CARRAY_prepare_incoming (typed_buffer_descr_t *descr, char *rcv_data, 
                        long rcv_len, char **odata, long *olen, long flags)
{
    int ret=EXSUCCEED;
    int rcv_buf_size;
    int existing_size;
    
    char fn[]="CARRAY_prepare_incoming";
    buffer_obj_t *outbufobj=NULL;

    NDRX_LOG(log_debug, "Entering %s", fn);
    
    rcv_buf_size = rcv_len;
   
    
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
        if (flags & TPNOCHANGE && outbufobj->type_id!=BUF_TYPE_CARRAY)
        {
            /* Raise error! */
            ndrx_TPset_error_fmt(TPEINVAL, "Receiver expects %s but got %s buffer",
                                        G_buf_descr[BUF_TYPE_CARRAY],
                                        G_buf_descr[outbufobj->type_id]);
            ret=EXFAIL;
            goto out;
        }
        /* If we can change data type and this does not match, then
         * we should firstly free it up and then bellow allow to work in mode
         * when odata is NULL!
         */
        if (outbufobj->type_id!=BUF_TYPE_CARRAY)
        {
            NDRX_LOG(log_warn, "User buffer %d is different, "
                    "free it up and re-allocate as CARRAY", G_buf_descr[outbufobj->type_id]);
            ndrx_tpfree(*odata, outbufobj);
            *odata=NULL;
        }
    }
    
    /* check the output buffer */
    if (NULL!=*odata)
    {
        NDRX_LOG(log_debug, "%s: Output buffer exists", fn);
        
        existing_size = outbufobj->size;
        

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
        }
    }
    else
    {
        /* allocate the buffer */
        NDRX_LOG(log_debug, "%s: Incoming buffer where missing - "
                                         "allocating new!", fn);

        *odata = ndrx_tpalloc(&G_buf_descr[BUF_TYPE_CARRAY], NULL, NULL, rcv_len);

        if (NULL==*odata)
        {
            /* error should be set already */
            NDRX_LOG(log_error, "Failed to allocat new buffer!");
            goto out;
        }
    }
    
    /* Copy off the received data including EOS ofcorse. */
    memcpy(*odata, rcv_data, rcv_len);

    if (NULL!=olen)
    {
        *olen = rcv_len;
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
expublic char * CARRAY_tpalloc (typed_buffer_descr_t *descr, char *subtype, long *len)
{
    char *ret;

    if (0==*len)
    {
        *len = CARRAY_DEFAULT_SIZE;
    }

    /* Allocate CARRAY buffer */
    ret=(char *)NDRX_MALLOC(*len);
    
    if (NULL!=ret)
    {
        ret[0] = EXEOS;
    }
    else
    {
        ndrx_TPset_error_fmt(TPEOS, "%s: Failed to allocate CARRAY buffer (len=%ld): %s",  
                __func__, len, strerror(errno));
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
expublic char * CARRAY_tprealloc(typed_buffer_descr_t *descr, char *cur_ptr, long len)
{
    char *ret=NULL;
    
    if (0==len)
    {
        len = CARRAY_DEFAULT_SIZE;
    }

    /* Allocate CARRAY buffer */
    ret=(char *)NDRX_REALLOC(cur_ptr, len);
    
    if (NULL==ret)
    {
        ndrx_TPset_error_fmt(TPEOS, "%s: Failed to reallocate CARRAY buffer (len=%ld): %s",  
                __func__, len, strerror(errno));
    }

    return ret;
}

/**
 * Gracefully remove free up the buffer
 * @param descr
 * @param buf
 */
expublic void CARRAY_tpfree(typed_buffer_descr_t *descr, char *buf)
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
expublic int CARRAY_test(typed_buffer_descr_t *descr, char *buf, BFLDLEN len, char *expr)
{
    int ret=EXFALSE;

    NDRX_LOG(log_error, "Carray buffers do not support event filters! Expr: [%s]", expr);
    userlog("Carray buffers do not support event filters! Expr: [%s]", expr);
    
    return ret;
}


