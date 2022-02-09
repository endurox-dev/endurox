/**
 * @brief UBF read/write function implementations
 *   Enduro Execution Library
 *
 * @file b_readwrite.c
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
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/


/**
 * Remove PTR from UBF, in recursive way
 * @param p_ub UBF buffer
 * @param maxlen max buffer size to be used for sub-buffers
 * @param did_mod did we modify something?
 * @return EXSUCCEED/EXFAIL
 */
exprivate int strip_ptr(UBFH *p_ub, BFLDLEN maxlen, int *did_mod)
{
    int ret = EXSUCCEED;
    int restart;
    int did_local_mod;
    UBFH *p_ub_tmp = NULL;
    int cnt, i;
    
    UBF_LOG(log_debug, "strip_ptr enter p_ub=%p maxlen=%d did_mode=%d",
            p_ub, maxlen, *did_mod);
    do
    {
        Bnext_state_t state;
        BFLDID bfldid=BBADFLDOCC;
        BFLDOCC occ;
        char *d_ptr;
        UBF_header_t *hdr = (UBF_header_t *)p_ub;
        BFLDID   *p_bfldid_start = &hdr->bfldid;
        int ftyp;
        
        restart=EXFALSE;
        state.p_cur_bfldid = (BFLDID *)(((char *)p_bfldid_start) + hdr->cache_ptr_off);
        state.cur_occ = 0;
        state.p_ub = p_ub;
        state.size = hdr->bytes_used;

        if (EXTRUE==(ret=ndrx_Bnext(&state, p_ub, &bfldid, &occ, NULL, NULL, &d_ptr)))
        {
            ftyp = bfldid >> EFFECTIVE_BITS;
            
            if (BFLD_PTR==ftyp)
            {
                /* remove field... */
                *did_mod=EXTRUE;
                
                UBF_LOG(log_debug, "Removing fldid=%d as ptr", bfldid);
                if (EXSUCCEED!=Bdel(p_ub, bfldid, occ))
                {
                    EXFAIL_OUT(ret);
                }
                restart=EXTRUE;
            }
            else 
            {
                /* we are done */
                ret=EXSUCCEED;
            }
        }
        
    } while (restart);
    
    /* If we have UBF buffer... fetch &  */
    cnt = 0;
    i = 0;
    do
    {
        Bnext_state_t state;
        BFLDID bfldid=BBADFLDOCC;
        BFLDOCC occ;
        char *d_ptr;
        UBF_header_t *hdr = (UBF_header_t *)p_ub;
        BFLDID   *p_bfldid_start = &hdr->bfldid;
        int ftyp;
        BFLDLEN blen;
        
        restart=EXFALSE;
        state.p_cur_bfldid = (BFLDID *)(((char *)p_bfldid_start) + hdr->cache_ubf_off);
        state.cur_occ = 0;
        state.p_ub = p_ub;
        state.size = hdr->bytes_used;

        UBF_LOG(log_debug, "Searching for sub-buffers, cnt=%d", cnt);
        for(i=0; (EXTRUE==(ret=ndrx_Bnext(&state, p_ub, &bfldid, &occ, NULL, NULL, &d_ptr))); i++)
        {
            ftyp = bfldid >> EFFECTIVE_BITS;
            
            if (BFLD_UBF==ftyp && i==cnt)
            {
                if (NULL==p_ub_tmp)
                {
                    p_ub_tmp = (UBFH *)NDRX_FPMALLOC(maxlen, 0);
                    
                    if (NULL==p_ub_tmp)
                    {
                        ndrx_Bset_error_fmt(BMALLOC, "%s: Failed to malloc %d bytes",
                                __func__, maxlen);
                        EXFAIL_OUT(ret);
                    }
                }
                
                if (EXSUCCEED!=Binit(p_ub_tmp, maxlen))
                {
                    UBF_LOG(log_error, "Failed to init temp buffer!");
                    EXFAIL_OUT(ret);
                }
                
                blen = maxlen;
                        
                if (EXSUCCEED!=Bget(p_ub, bfldid, occ, (char *)p_ub_tmp, &blen))
                {
                    UBF_LOG(log_error, "Failed to read sub-buffer %d occ %d",
                            bfldid, occ);
                    EXFAIL_OUT(ret);
                }
                
                did_local_mod=EXFALSE;
                
                if (EXSUCCEED!=strip_ptr(p_ub_tmp, maxlen, &did_local_mod))
                {
                    EXFAIL_OUT(ret);
                }
                
                if (did_local_mod)
                {
                    *did_mod=EXTRUE;
                    
                    /* Overwrite the field */
                    
                    if (EXSUCCEED!=Bchg(p_ub, bfldid, occ, (char *)p_ub_tmp, 0))
                    {
                        UBF_LOG(log_error, "Failed to update sub-buffer %d occ %d",
                                bfldid, occ);
                        EXFAIL_OUT(ret);
                    }
                    
                }
                /* next time continue with cnt pos */
                restart=EXTRUE;
                cnt++;
                break;
            }
            else if (BFLD_UBF!=ftyp)
            {
                /* we are done */
                ret=EXSUCCEED;
                break;
            }
        }
        
    } while (restart);

out:
    
    if (NULL!=p_ub_tmp)
    {
        NDRX_FPFREE(p_ub_tmp);
    }
    
    return ret;
}

/**
 * Internal version of buffer reader from stream. If \p p_readf is not present
 * then uses read from input file. If ptr is present, then data read is made
 * from callbacks.
 * @param p_ub UBF buffer
 * @param inf stream to read from 
 * @param [in] p_readf read callback function. for which `buffer' is where to
 *  put data read. `bufz' is buffer size and `dataptr1' is \p dataptr1 passed to
 *  function. Note that buffer must be filled filled with exact of requested
 *  bufsz. This is mandatory for first read as header of UBF of processed with
 *  single data read request. In case of error -1 shall be returned.
 * @param [in] dataptr1 data pointer passed down to read function \p p_readf 
 * @return SUCCEED/FAIL
 */
expublic int ndrx_Bread  (UBFH * p_ub, FILE * inf,
        long (*p_readf)(char *buffer, long bufsz, void *dataptr1), void *dataptr1)
{
    int ret=EXSUCCEED;
    UBF_header_t *hdr = (UBF_header_t *)p_ub;
    UBF_header_t hdr_src;
    BFLDLEN dst_buf_len;
    BFLDLEN dst_buf_free;
    char *p = (char*)hdr;
    int read;
    int to_read;

    UBF_LOG(log_debug, "%s: enter", __func__);

    /* Reset dest buffer */    
    memset(&hdr_src, 0, sizeof(hdr_src));
    
    if (NULL!=p_readf)
    {
        read = (int) p_readf((char *)&hdr_src, sizeof(hdr_src), dataptr1);
    }
    else
    {
        /* Read the header from input file. */
        read=fread((char *)&hdr_src, 1, sizeof(hdr_src), inf);
    }

    /* Was header read OK? */
    if (sizeof(hdr_src)!=read)
    {
        ndrx_Bset_error_fmt(BEUNIX, "%s: Failed to read header from input "
                                    "file, unix err %d (read) vs %d (expected): [%s]",
                                    __func__, read, (int)sizeof(hdr_src),
                                    strerror(errno));
        EXFAIL_OUT(ret);
    }

    /* Check header */
    if (0!=strncmp(hdr_src.magic, UBF_MAGIC, UBF_MAGIC_SIZE))
    {
        ndrx_Bset_error_fmt(BNOTFLD, "%s: Source buffer not UBF!", __func__);
        EXFAIL_OUT(ret);
    }

    /*
     * Reset dest buffer.
     */
    dst_buf_len = hdr->buf_len;
    ret=Binit(p_ub, dst_buf_len);
    dst_buf_free = hdr->buf_len - hdr->bytes_used;

    /* Check dest buffer size. */
    if (dst_buf_free < hdr_src.bytes_used)
    {
        ndrx_Bset_error_fmt(BNOSPACE, "%s:No space in source buffer - free: %d "
                                    "bytes, requested: %d",
                                    __func__, dst_buf_free, hdr_src.bytes_used);
        EXFAIL_OUT(ret);
    }

    /* Now read the left bytes from the stream & prepare header*/
    to_read = hdr_src.bytes_used - read;
    p+=read;
    
    if (NULL!=p_readf)
    {
        read = (int) p_readf(p, to_read, dataptr1);
    }
    else
    {
        read=fread(p, 1, to_read, inf);
    }

    if (read!=to_read)
    {
        ndrx_Bset_error_fmt(BEUNIX, "%s:Failed to read buffer data from "
                                " input file %d (read) vs %d (expected), unix err: [%s]",
                                __func__,
                                read, to_read,
                                strerror(errno));
        EXFAIL_OUT(ret);
    }
    else
    {
        /* update header of dst */
        memcpy(hdr, &hdr_src, sizeof(hdr_src));
        hdr->buf_len = dst_buf_len;
    }

    /* Validate the buffer */
    if (EXSUCCEED!=(ret=validate_entry(p_ub, 0, 0, VALIDATE_MODE_NO_FLD)))
    {
        UBF_LOG(log_error, "Restored buffer is invalid!");
        ndrx_Bappend_error_msg("(restored buffer is invalid)");
        EXFAIL_OUT(ret);
    }
    else
    {
        UBF_DUMP(log_always, "_Bread: restored buffer data:", 
                p_ub, hdr->bytes_used);
    }
    
    /* wipe out any BFLD_PTR fields, if not enabled... */
    if (!(ndrx_G_apiflags & NDRX_APIFLAGS_UBFPTRPARSE))
    {
        /* strip off BFLD_PTR in recursive way from the imported data...
         * New func: Find if any BFLD_PTR is used, remove the off.
         *  - return flag that buffer is changed
         * Search for any BFLD_UBF, if any found, read the buffer (to temp space)
         *  (one by one as sizes may change)
         * - call the new func.
         * - If changes has been detected, perform Bchg() over the current occurrence,
         * -- how about to loop next? Count the fields processed?
         */
        int did_mod = EXFALSE;
        if (EXSUCCEED!=strip_ptr(p_ub, Bused(p_ub), &did_mod))
        {
            EXFAIL_OUT(ret);
        }
        
        if (did_mod)
        {
            UBF_LOG(log_debug, "PTRs removed.");
        }
    }
    
out:
    UBF_LOG(log_debug, "%s: return %d", __func__, ret);
    
    return ret;
}

/**
 * Internal version of buffer writer to stream
 * This will write only used area of the buffer to the stream.
 * Function accepts either output file pointer or callback function which writes
 * the buffer output.
 * @param p_ub UBF buffer
 * @param outf stream to write to 
 * @param [in] p_writef data output function. `buffer' contains the data to be written
 *  `bufz' contains number of bytes to be written. `dataptr1' is forwared from
 *  function original requests. Function must return number of bytes written.
 *  in non-error scenario all passed bytes to function must be written. In case
 *  of error -1 can be returned.
 * @param [in] dataptr1 is forwarded to \p p_writef
 * @return SUCCEED/FAIL
 */
expublic int ndrx_Bwrite (UBFH *p_ub, FILE * outf,
        long (*p_writef)(char *buffer, long bufsz, void *dataptr1), void *dataptr1)
{
    int ret=EXSUCCEED;
    UBF_header_t *hdr = (UBF_header_t *)p_ub;
    int written;

    UBF_LOG(log_debug, "%s: enter", __func__);

    /* Dump the buffer, which is about to write */

    UBF_DUMP(log_always, "ndrx_Bwrite: buffer data:", p_ub, hdr->bytes_used);
    
    if (NULL!=p_writef)
    {
        written=(int)p_writef((char *)p_ub, hdr->bytes_used, dataptr1);
    }
    else
    {
        written=fwrite(p_ub, 1, hdr->bytes_used, outf);
    }

    if (written!=hdr->bytes_used)
    {
        /* we have error condition! */
        ndrx_Bset_error_fmt(BEUNIX, "%s:Write failed! Requested for write %d bytes, "
                            "but written %d. Unix error: [%s]",
                             __func__, hdr->bytes_used, written, strerror(errno));
        EXFAIL_OUT(ret);
    }

    /* Flush written data. */
    if (NULL==p_writef)
    {
        fflush(outf);

        if (ferror(outf))
        {
            ndrx_Bset_error_fmt(BEUNIX, "%s: On Write fflush failed, Unix error: [%s]",
                                __func__, strerror(errno));
            EXFAIL_OUT(ret);
        }
    }
    
out:
    
    UBF_LOG(log_debug, "%s: return %d", __func__, ret);
    
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
