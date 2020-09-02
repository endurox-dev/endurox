/**
 * @brief BFLD_UBF type support
 *
 * @file fld_ubf.c
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
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Get total data size. The payload size shall be measured with Bused()
 * The dlen contains cached value for 
 * @param t type descriptor
 * @param ptr on UBF field start in UBF buffer
 * @return size in bytes, aligned
 */
expublic int ndrx_get_fb_ubf_size(dtype_str_t *t, char *fb, int *payload_size)
{
    UBF_ubf_t *ubf = (UBF_ubf_t *)fb;
    UBF_header_t *hdr = (UBF_header_t *)ubf->ubfdata;
    
    if (NULL!=payload_size)
    {
        *payload_size = hdr->bytes_used;
    }
    
    return ALIGNED_SIZE(hdr->bytes_used);
}

/**
 * Put the buffer data
 * @param t type descriptor
 * @param fb UBF buffer position for field
 * @param bfldid field id
 * @param data must be UBF type buffer
 * @param len not used, we take datak
 * @return SUCCEED
 */
expublic int ndrx_put_data_ubf(dtype_str_t *t, char *fb, BFLDID bfldid, 
        char *data, int len)
{
    UBF_ubf_t *ubf = (UBF_ubf_t *)fb;
    UBF_header_t *hdr = (UBF_header_t *)data;
    
    /* int align; */
    ubf->bfldid = bfldid;

    memcpy(ubf->ubfdata, data, hdr->bytes_used);
        
    return EXSUCCEED;
}


/**
 * Return estimated required data size
 * @param t type descr
 * @param data data
 * @param len passed len, not used
 * @return bytes needed
 */
expublic int ndrx_get_d_size_ubf (struct dtype_str *t, char *data, 
        int len, int *payload_size)
{
    UBF_header_t *hdr = (UBF_header_t *)data;
    
    
    /* Check that input buffer is valid? */
    if (EXSUCCEED!=validate_entry((UBFH *)data, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "Invalid sub-UBF to add to UBF: %p", data);
        return EXFAIL;
    }
    
    if (NULL!=payload_size)
        *payload_size=hdr->bytes_used;

    return ALIGNED_SIZE(hdr->bytes_used);
}

/**
 * Read the data from given position in fb
 * needs to decide what output will be?
 * Will it be UBF? Or it can be any byte array?
 * 
 * @param t data type descr
 * @param fb field position in UBF buffer
 * @param buf output buffer
 * @param len output buffer len, shall we use this?
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_get_data_ubf (struct dtype_str *t, char *fb, char *buf, int *len)
{
    UBF_ubf_t *ubf = (UBF_ubf_t *)fb;
    UBF_header_t *hdr = (UBF_header_t *)ubf->ubfdata;
    int ret=EXSUCCEED;
    BFLDLEN      org_buf_len; /* keep the org buffer len */
    
    org_buf_len = hdr->buf_len;

    if (NULL!=len && *len>0 && *len < hdr->bytes_used)
    {
        /* Set error, that string buffer too short */
        ndrx_Bset_error_fmt(BNOSPACE, "output buffer too short. Data len %d in buf, "
                                "output: %d", hdr->bytes_used, *len);
        EXFAIL_OUT(ret);
    }
    else
    {
        /* copy the data */
        memcpy(buf, ubf->ubfdata, hdr->bytes_used);
        
        if (NULL!=len)
        {
            *len=hdr->bytes_used;
        }
    }
out:
    return ret;
}

/**
 * Return empty size of UBF buffer.
 * Question here is -> if we add empty UBF buffer, is it normal buffer
 * THe buffer is normal empty buffer...
 * @param t data type
 * @return aligned bytes required for the UBF buffer
 */
expublic int ndrx_g_ubf_empty(struct dtype_ext1* t)
{
    return ALIGNED_SIZE(sizeof(UBF_header_t)-FF_USED_BYTES); /* empty string eos */
}

/**
 * Put empty UBF buffer in UBF buffer
 * @param t type descr
 * @param fb UBF buffer where to install the data
 * @param bfldid field id to set
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_put_empty_ubf(struct dtype_ext1* t, char *fb, BFLDID bfldid)
{
    int ret=EXSUCCEED;
    UBF_ubf_t *ubf = (UBF_ubf_t *)fb;
    UBF_header_t empty_buf;
    
    
    /* buffer len will be incorrect here, as we operate only with data used
     * and not the total len...
     * But any way that's how we work...
     */
    Binit((UBFH *)&empty_buf, sizeof(empty_buf));
    
    ubf->bfldid = bfldid;
    /* copy off temp buffer */
    memcpy(ubf->ubfdata, &empty_buf, empty_buf.bytes_used);
    
    return ret;
}

/**
 * Print the buffer to log
 * @param t type descr
 * @param text extra debug msg
 * @param data ptr to UBF field to print
 * @param len not used..
 */
expublic void ndrx_dump_ubf(struct dtype_ext1 *t, char *text, char *data, int *len)
{
    if (NULL!=data)
    {
        ndrx_debug_dump_UBF_ubflogger(log_debug, text, (UBFH *)data);
    }
    else
    {
        UBF_LOG(log_debug, "%s:\n[null data or len]", text);
    }
}

/**
 * Compare two buffers
 * @param t data type
 * @param val1 UBF1
 * @param len1 not used
 * @param val2 UBF2
 * @param len2 not used
 * @param mode UBF_CMP_MODE_STD? 
 * @return EXTRUE/EXFALSE, or -1,0,1
 */
expublic int ndrx_cmp_ubf (struct dtype_ext1 *t, char *val1, BFLDLEN len1, 
        char *val2, BFLDLEN len2, long mode)
{
    
    if (mode & UBF_CMP_MODE_STD)
    {
        return Bcmp((UBFH *)val1, (UBFH *)val2);
    }
    else
    {
        if (0==Bcmp((UBFH *)val1, (UBFH *)val2))
        {
            return EXTRUE;
        }
        else
        {
            return EXFALSE;
        }
    }
}

/* vim: set ts=4 sw=4 et smartindent: */