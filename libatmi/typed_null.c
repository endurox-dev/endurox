/**
 * @brief TPNULL buffer type support
 *
 * @file typed_null.c
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

#include <ndrstandard.h>
#include <typed_buf.h>
#include <ndebug.h>
#include <tperror.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Basic init data structure allocator
 * @param subtype
 * @param len
 * @return
 */
expublic char	* TPNULL_tpalloc (typed_buffer_descr_t *descr, 
        char *subtype, long *len)
{
    char *ret=NULL;
    char fn[] = "TPNULL_tpalloc";

    /* Allocate UBF buffer, 1 byte, what so ever.. 
    ret=NDRX_MALLOC(1);

    if (NULL==ret)
    {
        NDRX_LOG(log_error, "%s: Failed to allocate TPNULL buffer!", fn);
        ndrx_TPset_error_fmt(TPEOS, "TPNULL failed to allocate: %d bytes", 
                sizeof(TPINIT));
        goto out;
    }
    */

out:
    return ret;
}

/**
 * Gracefully remove free up the buffer
 * @param descr
 * @param buf
 */
expublic void TPNULL_tpfree(typed_buffer_descr_t *descr, char *buf)
{
    /* NDRX_FREE(buf); - nothing to do on NULL! */
}

/**
 * Prepare outgoing message, just do nothing
 * @param descr buffer type descr
 * @param idata input data/null
 * @param ilen 0
 * @param obuf no data to install
 * @param olen olen is 0
 * @param flags ?
 * @return EXSUCCEED
 */
expublic int TPNULL_prepare_outgoing (typed_buffer_descr_t *descr, char *idata, 
        long ilen, char *obuf, long *olen, long flags)
{
    if (NULL!=olen)
    {
        *olen = 0;
    }
    
    return EXSUCCEED;
}

/**
 * Prepare incoming buffer, for NULL we just free any other previous buffer
 * pointed to.
 * @param descr description ptr
 * @param rcv_data received data
 * @param rcv_len data len received
 * @param odata original XATMI buffer to push data in
 * @param olen data buffer len
 * @param flags flags
 * @return EXSUCCEED/EXFAIL
 */
expublic int TPNULL_prepare_incoming (typed_buffer_descr_t *descr, char *rcv_data, 
                        long rcv_len, char **odata, long *olen, long flags)
{
    
    /* reject type switch if requested... */
    int ret=EXSUCCEED;
    buffer_obj_t *outbufobj=NULL;

    NDRX_LOG(log_debug, "Entering %s", __func__);
        
    /* Figure out the passed in buffer */
    if (NULL==(outbufobj=ndrx_find_buffer(*odata)))
    {
        ndrx_TPset_error_fmt(TPEINVAL, "Output buffer %p is not allocated "
                                        "with tpalloc()!", *odata);
        EXFAIL_OUT(ret);
    }

    /* Check the data types */
    if (NULL!=outbufobj)
    {
        /* If we cannot change the data type, then we trigger an error */
        if (flags & TPNOCHANGE && outbufobj->type_id!=BUF_TYPE_NULL)
        {
            /* Raise error! */
            ndrx_TPset_error_fmt(TPEOTYPE, "Receiver expects %s but got %s buffer",
                                        G_buf_descr[BUF_TYPE_NULL].type,
                                        G_buf_descr[outbufobj->type_id].type
                                        );
            EXFAIL_OUT(ret);
        }
    }
    
    if (NULL!=*odata)
    {
        tpfree(*odata);
        *odata = NULL;
    }
    
    if (NULL!=olen)
    {
        *olen = 0;
    }
    
out:
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
