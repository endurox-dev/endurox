/* 
 * @brief UBF read/write function implementations
 *   Enduro Execution Library
 *
 * @file b_readwrite.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or Mavimax's license for commercial use.
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

/*---------------------------Includes-----------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>

#include <ubf.h>
#include <ubf_int.h>	/* Internal headers for UBF... */
#include <fdatatype.h>
#include <ferror.h>
#include <fieldtable.h>
#include <ndrstandard.h>
#include <ndebug.h>
#include <cf.h>
#include <ubf_impl.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Internal version of buffer reader from stream
 * @param p_ub - UBF buffer
 * @param inf - stream to read from 
 * @return SUCCEED/FAIL
 */
expublic int ndrx_Bread  (UBFH * p_ub, FILE * inf)
{
    int ret=EXSUCCEED;
    UBF_header_t *hdr = (UBF_header_t *)p_ub;
    UBF_header_t hdr_src;
    BFLDLEN dst_buf_len;
    BFLDLEN dst_buf_free;
    char *p = (char*)hdr;
    
    int read;
    char fn[]="_Bread";

    UBF_LOG(log_debug, "%s: enter", fn);

    /* Reset dest buffer */    
    memset(&hdr_src, 0, sizeof(hdr_src));
    /* Read the header from input file. */
    read=fread(&hdr_src, 1, sizeof(hdr_src), inf);

    /* Was header read OK? */
    if (sizeof(hdr_src)!=read)
    {
        ndrx_Bset_error_fmt(BEUNIX, "%s:Failed to read header from input file, unix err: [%s]",
                                     fn, strerror(errno));
        ret=EXFAIL;
    }

    /* Check header */
    if (EXSUCCEED==ret && 0!=strncmp(hdr_src.magic, UBF_MAGIC, UBF_MAGIC_SIZE))
    {
        ndrx_Bset_error_fmt(BNOTFLD, "%s:Source buffer not bisubf!", fn);
        ret=EXFAIL;
    }

    /*
     * Reset dest buffer.
     */
    if (EXSUCCEED==ret)
    {
        dst_buf_len = hdr->buf_len;
        ret=Binit(p_ub, dst_buf_len);
        dst_buf_free = hdr->buf_len - hdr->bytes_used;
    }

    /* Check dest buffer size. */
    if (EXSUCCEED==ret && dst_buf_free<hdr_src.bytes_used)
    {
        ndrx_Bset_error_fmt(BNOSPACE, "%s:No space in source buffer - free: %d bytes, requested: %d",
                                    fn, dst_buf_free, hdr_src.bytes_used);
        ret=EXFAIL;   
    }

    /* Now read the left bytes from the stream & prepare header*/
    if (EXSUCCEED==ret)
    {
        int to_read = hdr_src.bytes_used - read;
        p+=read;
        read=fread(p, 1, to_read, inf);

        if (read!=to_read)
        {
            ndrx_Bset_error_fmt(BEUNIX, "%s:Failed to read buffer data from input file, unix err: [%s]",
                                     fn, strerror(errno));
            ret=EXFAIL;
        }
        else
        {
            /* update header of dst */
            memcpy(hdr, &hdr_src, sizeof(hdr_src));
            hdr->buf_len = dst_buf_len;
        }
    }

    if (EXSUCCEED==ret)
    {
        /* Validate the buffer */
        if (EXSUCCEED!=(ret=validate_entry(p_ub, 0, 0, VALIDATE_MODE_NO_FLD)))
        {
            UBF_LOG(log_error, "Restored buffer is invalid!");
           ndrx_Bappend_error_msg("(restored buffer is invalid)");

        }
        else
        {
            UBF_DUMP(log_always, "_Bread: restored buffer data:", p_ub, hdr->bytes_used);
        }
    }

    UBF_LOG(log_debug, "%s: return %d", fn, ret);
    
    return ret;
}

/**
 * Internal version of buffer writer to stream
 * This will write only used area of the buffer to the stream.
 * @param p_ub -UBF buffer
 * @param outf - stream to write to 
 * @return SUCCEED/FAIL
 */
expublic int ndrx_Bwrite (UBFH *p_ub, FILE * outf)
{
    int ret=EXSUCCEED;
    UBF_header_t *hdr = (UBF_header_t *)p_ub;
    int written;
    char fn[]="_Bwrite";

    UBF_LOG(log_debug, "%s: enter", fn);

    /* Dump the buffer, whihc is about to write */

    UBF_DUMP(log_always, "_Bwrite: buffer data:", p_ub, hdr->bytes_used);
    
    written=fwrite(p_ub, 1, hdr->bytes_used, outf);

    if (written!=hdr->bytes_used)
    {
        /* we have error condition! */
        ndrx_Bset_error_fmt(BEUNIX, "%s:Write failed! Requested for write %d bytes, "
                                    "but written %d. Unix error: [%s]",
                                     fn, hdr->bytes_used, written, strerror(errno));
        ret=EXFAIL;
    }

    /* Flush written data. */
    if (EXSUCCEED==ret)
    {
        fflush(outf);
    }
    
    if (EXSUCCEED==ret && ferror(outf))
    {
        ndrx_Bset_error_fmt(BEUNIX, "%s: On Write fflush failed, Unix error: [%s]",
                                   fn, strerror(errno));
        ret=EXFAIL;
    }

    UBF_LOG(log_debug, "%s: return %d", fn, ret);
    
    return ret;
}

