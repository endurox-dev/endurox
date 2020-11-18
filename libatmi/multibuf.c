/**
 * @brief Multi-buffer serialization handler
 *
 * @file multibuf.c
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
#include <ndrx_config.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <sys/sem.h>

#include <atmi.h>
#include <multibuf.h>
#include <atmi_shm.h>
#include <ndrstandard.h>
#include <ndebug.h>
#include <ndrxd.h>
#include <ndrxdcmn.h>
#include <userlog.h>
#include <ubf_int.h>
#include <typed_buf.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/


/**
 * Find the particular ptr in t
 * @param ptr real pointer to check for multi buf descriptor
 * @return NULL (not found), ptr to MB if found 
 */
exprivate ndrx_mbuf_ptrs_t* ndrx_mbuf_ptr_find(ndrx_mbuf_ptrs_t **ptrs, char *ptr)
{
    ndrx_mbuf_ptrs_t *ret=NULL;
    
    EXHASH_FIND_PTR( (*ptrs), ((void **)&ptr), ret);
    
    return ret;   
}

/**
 * Register exported pointer in the hash
 * @param ptrs hashmap of the real pointers
 * @param ptr pointer to register
 * @param tag tag to add
 * @return ptr descr / NULL (OOM)
 */
exprivate ndrx_mbuf_ptrs_t * ndrx_mbuf_ptr_add(ndrx_mbuf_ptrs_t **ptrs, char *ptr, int tag)
{
    ndrx_mbuf_ptrs_t *ret = NULL;
    
    /* Alloc the memory block */
    if (NULL==(ret=(ndrx_mbuf_ptrs_t *)NDRX_FPMALLOC(sizeof(ndrx_mbuf_ptrs_t), 0)))
    {
        ndrx_TPset_error_fmt(TPEOS, "%s: Failed temporary hash space");
        goto out;
    }
    
    ret->ptr = ptr;
    ret->tag = tag;
    
    EXHASH_ADD_PTR((*ptrs), ptr, ret);
    
out:
    return ret;
}

/**
 * Free up the hash list of pointers
 * @param ptrs pointer hashmap
 */
exprivate void ndrx_mbuf_ptr_free(ndrx_mbuf_ptrs_t **ptrs)
{
    ndrx_mbuf_ptrs_t * el, *elt;
    
    EXHASH_ITER(hh, (*ptrs), el, elt)
    {
        EXHASH_DEL((*ptrs), el);
        NDRX_FPFREE(el);
    }
}

/**
 * receive TLV data with several buffers
 * buffers needs to be indexed and pointer remapped for embedded PTR
 * types
 * @param rcv_data data received (with out header)
 * @param rcv_len data received length
 * @param odata output buffer. This is master buffer. Call header
 *  may be linked. The master buffer could be ubf with ptrs to ubfs which
 *  may also contain ptrs... Received data pointer are virtually numbered
 *  and corresponds to TLV index. Thus atmi buffers needs to be allocated
 *  and their pointers needs to updated in ubf buffer. Also one UBF
 *  may point to one atmi buffer several times. Thus needs some hash of
 *  pointers which would map the linear array index. 
 * @param olen master buffer received length
 * @param flags atmi flags
 * @param mflags MBUF flags
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_mbuf_prepare_incoming (char *rcv_data, long rcv_len, char **odata, 
        long *olen, long flags, long mflags)
{
    int ret = EXSUCCEED;
    
out:
    return ret;
}

/**
 * Write entry to TLV
 * @param idata input data
 * @param ilen input data buffer len (seems for PTR -> carrays we need to
 *  send all allocated space, for others we can evaluate the size)
 * @param obuf output buffer where to print the TLV
 * @param olen_max max size of buf
 * @param olen_used current un-aligned buffer use
 * @param tag tag number to assign for this buffer.
 * @param flags ATMI flags
 * @return EXSUCCEED/EXFAIL (ATMI error loaded)
 */
exprivate int ndrx_mbuf_tlv_do(char *idata, long ilen, char *obuf, 
        long olen_max, long *olen_used, unsigned int tag, long flags)
{
    int ret = EXSUCCEED;
    ndrx_mbuf_tlv_t *hdr;
    long new_used = ALIGNED_SIZE(*olen_used);
    int pad=*olen_used-new_used;
    char *work_buf = obuf + new_used;
    long tmp_olen;
    buffer_obj_t * buffer_info = ndrx_find_buffer(idata);
    typed_buffer_descr_t *descr;
    
    if (NULL==buffer_info)
    {
        NDRX_LOG(log_error, "Input buffer %p not atmi allocated", idata);
        ndrx_TPset_error_fmt(TPEINVAL, "Input buffer %p not atmi allocated", idata);
        EXFAIL_OUT(ret);
    }
    
    if (EXFAIL==ilen)
    {
        /* in case of carray we do not know the real used len */
        ilen=buffer_info->size;
    }
    
    /* get get aligned size*/
    new_used += sizeof(ndrx_mbuf_tlv_t);
    
    if (new_used> olen_max)
    {
        ndrx_TPset_error_fmt(TPEINVAL, "%s: Internal buffer size %ld, new data %ld (tag)", 
                __func__, olen_max, new_used);
        ret=EXFAIL;
        goto out;
    }
            
    /* put the header down */
    hdr = (ndrx_mbuf_tlv_t *)work_buf;
    hdr->tag = tag | (buffer_info->type_id << NDRX_MBUF_OFFSET);
    /* get the buffer type */
    
    /* calculate the amount left */
    tmp_olen = olen_max-new_used;
    
    /* move next.. */
    work_buf+=new_used;
    
    descr = &G_buf_descr[buffer_info->type_id];
    
    /* prepare buffer for call */
    NDRX_LOG(log_debug, "Preparing buffer tag: %u (typed %u, type %d). Source buffer %p, "
            "dest master buffer %p (work place header %p data %p) olen_max=%ld new_used=%ld pad=%d", 
            tag, hdr->tag, buffer_info->type_id, idata, obuf, hdr, work_buf, olen_max, new_used, pad);
    
    if (EXSUCCEED!=descr->pf_prepare_outgoing(descr, idata, ilen, work_buf, 
            &tmp_olen, flags))
    {
        NDRX_LOG(log_error, "Failed to prepare buffer tag: %u (typed %u, type %d). Source buffer %p, "
            "dest master buffer %p (work place header %p data %p) olen_max=%ld new_used=%ld pad=%d", 
            tag, hdr->tag, buffer_info->type_id, idata, obuf, hdr, work_buf, olen_max, new_used, pad);
        EXFAIL_OUT(ret);
    }
    hdr->len = tmp_olen;
    /* new used is loaded */
    *olen_used=new_used+tmp_olen;
    NDRX_LOG(log_debug, "tag=%d loaded new_used=%ld", tag, *olen_used);
    
out:
    return ret;    
}

/**
 * Loop the UBF buffer and add any ptr to output memory block.
 * This may go in recursion if we find any sub-UBF buffer of particular
 * UBF.
 * @param ptrs pointer hashmap
 * @param p_ub ubf to scan
 * @param obuf start address of serialized block of buffers
 * @param olen_max buffer max len
 * @param olen_used currently used space
 * @param p_tag currently used tag and next tag (set here)
 * @param flags RFU
 * @return EXSUCCEED/EXFAIL
 */
exprivate int ndrx_mbuf_ptrs_map_out(ndrx_mbuf_ptrs_t **ptrs, UBFH *p_ub,
        char *obuf, long olen_max, long *olen_used, unsigned int *p_tag, long flags)
{
    int ret = EXSUCCEED;
    Bnext_state_t state;
    BFLDID fldid;
    BFLDOCC occ;
    char *d_ptr;
    ndrx_longptr_t *lptr;
    
    memset(&state, 0, sizeof(state));
    
    while (EXSUCCEED==(ret=ndrx_Bnext(&state, p_ub, &fldid, &occ, NULL, NULL, &d_ptr)))
    {
        if (BFLD_PTR==Bfldtype(fldid))
        {
            lptr=(ndrx_longptr_t *)d_ptr;

            /* TOOD: map the pointer (add to hash the tag, or resolve the 
             * existing tag, and just update the UBF buffer)
             */
             *p_tag=*p_tag+1;
            
            NDRX_LOG(log_debug, "fldid=%d occ=%d ptr to %p -> serialize to tag %d",
                    fldid, occ, (char *)*lptr, *p_tag);
            /* how about buffer len? we do not know what is used len, so
             * needs to extract from types? */
            if (EXSUCCEED!=ndrx_mbuf_tlv_do((char *)*lptr, EXFAIL, obuf, 
                    olen_max, olen_used, *p_tag, flags))
            {
                NDRX_LOG(log_error, "Failed to add ptr %p to export data tag=%d",
                        (char *)*lptr, *p_tag);
                EXFAIL_OUT(ret);
            }
            
            /* TODO: update ubf buffer with tag */
            
        }
    }
            
    
out:
    return ret;
}

/**
 * Prepare outgoing data. Needs to check for call info existence
 * That would be set to ptr 0. Starting with index 10, data buffers
 * begins. The buffer 10 is master buffer and 11+ are ptr buffers.
 * Once we parse the TLV, each of the buffers needs to be allocated. Real
 * ptr needs to be stored in growlist.
 * then in the loop all in recursive way buffers needs to be updated
 * from virtual pointer to real pointer.
 * @param idata input master buffer (may have linked call info associated) 
 * and embedded buffers with pointer to atmi buffers
 * @param ilen input buffer used space
 * @param obuf output buffer
 * @param olen output buffer size
 * @param flags atmi flags
 * @param mflags MBUF flags
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_mbuf_prepare_outgoing (char *idata, long ilen, char *obuf, long *olen, 
        long flags, long mflags)
{
    int ret = EXSUCCEED;
    buffer_obj_t * ibuf = ndrx_find_buffer(idata);
    long used=0;
    /* pointer mapping hash */
    ndrx_mbuf_ptrs_t *ptrs=NULL;
    
    if (NULL==ndrx_find_buffer)
    {
        NDRX_LOG(log_error, "Input buffer %p not atmi allocated", idata);
        ndrx_TPset_error_fmt(TPEINVAL, "Input buffer %p not atmi allocated", idata);
        EXFAIL_OUT(ret);
    }
    
    /* check for call info, if any, serialize to TLV */
    if (NULL!=ibuf->callinfobuf && (mflags & NDRX_MBUF_FLAG_NOCALLINFO))
    {
        if (EXSUCCEED!=ndrx_mbuf_tlv_do(ibuf->callinfobuf, ibuf->callinfobuf_len, obuf, 
            *olen, &used, NDRX_MBUF_TAG_CALLINFO, flags))
        {
            NDRX_LOG(log_error, "Failed to run TLV on callinfo");
        }
    }
    
    /* serialize main buffer to TLV */
    
    if (EXSUCCEED!=ndrx_mbuf_tlv_do(idata, ilen, obuf, *olen, &used, 
            NDRX_MBUF_TAG_BASE, flags))
    {
        NDRX_LOG(log_error, "Failed to run TLV on base buffer");
    }
    
    /* iterate over the main buffer, remap the pointers on the fly 
     * looping shall be done in outer function to suport the recursion
     */
    
out:
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
