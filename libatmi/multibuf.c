/**
 * @brief Multi-buffer serialization handler
 *  call info, primary buffer and other sub-ptrs buffers are all written off
 *  in TLV master blob.
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
    
    NDRX_LOG(log_debug, "pointer %p mapped to tag %d", ptr, tag);
    
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
 * Remap the buffer back to real pointers
 * @param list list of ptrs by index
 * @param p_ub ubf to scan (recursively)
 * @return EXSUCCEED/EXFAIL
 */
exprivate int ndrx_mbuf_ptrs_map_in(ndrx_growlist_t *list, UBFH *p_ub)
{
    int ret = EXSUCCEED;
    Bnext_state_t state;
    BFLDID bfldid=BBADFLDOCC;
    BFLDOCC occ;
    char *d_ptr;
    char **lptr;
    ndrx_mbuf_vptrs_t *access_vptr;
    UBF_header_t *hdr = (UBF_header_t *)p_ub;
    int ftyp;
    unsigned int tag;
    BFLDID   *p_bfldid_start = &hdr->bfldid;
    
    state.p_cur_bfldid = (BFLDID *)(((char *)p_bfldid_start) + hdr->cache_ptr_off);
    state.cur_occ = 0;
    state.p_ub = p_ub;
    state.size = hdr->bytes_used;

    while (EXTRUE==(ret=ndrx_Bnext(&state, p_ub, &bfldid, &occ, NULL, NULL, &d_ptr)))
    {
        ftyp = bfldid >> EFFECTIVE_BITS;
        
        
        if (BFLD_PTR==ftyp)
        {
            
            /* if we get field, then this needs to be updated with real
             * ptr. Also in this case we shall reprocess the field
             * if it is UBF (but also if there are several ptrs to one
             * field, we shall do it once)
             */
            
            /* resolve the VPTR */
            lptr=(char **)d_ptr;
            access_vptr = (ndrx_mbuf_vptrs_t *)(list->mem + ((long)(*lptr))*sizeof(ndrx_mbuf_vptrs_t));
            
            NDRX_LOG(log_debug, "Mapping tag %ld to %p", (long)*lptr, access_vptr->data);
            
            tag = (unsigned int)(ndrx_longptr_t)*lptr;
            *lptr = access_vptr->data;

        }
        else if (BFLD_UBF==ftyp)
        {
            /* process the sub-buffer... (re-map sub-fields) */
            if (EXSUCCEED!=ndrx_mbuf_ptrs_map_in(list, (UBFH *)d_ptr))
            {
                NDRX_LOG(log_error, "Failed to re-map master buffer %p "
                        "sub-field (UBF) %d", p_ub, bfldid);
                EXFAIL_OUT(ret);
            }
        }
        else
        {
            /* we are done */
            ret=EXSUCCEED;
            break;
        }
    }
    
    NDRX_LOG(log_error, "YOPT! END OF SCAN");
    
    if (EXFAIL==ret)
    {
        NDRX_LOG(log_error, "Failed to loop ubf: %s", Bstrerror(Berror));
        ndrx_TPset_error_fmt(TPESYSTEM, "Failed to loop ubf: %s", Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
            
out:

    return ret;
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
    ndrx_growlist_t list;
    ndrx_mbuf_tlv_t *tlv_hdr;
    long tlv_pos;
    unsigned int tag_exp=0;
    ndrx_mbuf_vptrs_t current_vptr;
    ndrx_mbuf_vptrs_t *access_vptr;
    typed_buffer_descr_t *descr;
    int primary_loaded = EXFALSE;
    int i;
    /* parse the TLV and load into growlist with index of VPTRs */
    ndrx_growlist_init(&list, 50, sizeof(ndrx_mbuf_vptrs_t));
    
    /* OK load the stuff ... */
    NDRX_LOG(log_debug, "Parse incoming buffer TLV");
    for (tlv_pos=0; tlv_pos<rcv_len; tlv_pos+=(ALIGNED_GEN(tlv_hdr->len)+sizeof(ndrx_mbuf_tlv_t)), tag_exp++)
    {
        int is_callinfo;
        unsigned int tag;
        int btype;
        
        tlv_hdr=(ndrx_mbuf_tlv_t *)(rcv_data + tlv_pos);
        
        tag = NDRX_MBUF_TAGTAG(tlv_hdr->tag);
        btype = NDRX_MBUF_TYPE(tlv_hdr->tag);
        is_callinfo = !!(tlv_hdr->tag & NDRX_MBUF_CALLINFOBIT);
        
        NDRX_LOG(log_debug, "Received buffer tag: %u type: %d callinfo: %d len: %u",
                tag, btype, is_callinfo, tlv_hdr->len);
        
        if (tag!=tag_exp)
        {
            NDRX_LOG(log_error, "ERROR: Expected tag %u but got %u", tag_exp, tag);
            userlog("ERROR: Expected tag %u but got %u", tag_exp, tag);
            ndrx_TPset_error_fmt(TPESYSTEM, "ERROR: Expected tag %u but got %u", 
                    tag_exp, tag);
            EXFAIL_OUT(ret);
        }
        
        /* get the work area: */
        descr = &G_buf_descr[btype];

        current_vptr.btype = btype;/**< cache buffer type               */
        /* set the primary data, if any */
        if (!primary_loaded && !is_callinfo)
        {
            current_vptr.data=*odata;
            current_vptr.len=*olen;
        }
        else
        {
            current_vptr.data=NULL;
            current_vptr.len=0;
        }
                
        /* check is this primary or not? */
        ret=descr->pf_prepare_incoming(descr,
                tlv_hdr->data,
                tlv_hdr->len,
                &current_vptr.data,
                &current_vptr.len,
                flags);
        
        if (EXSUCCEED!=ret)
        {
            NDRX_LOG(log_error, "Failed to prepare incoming buffer tag: %u type: %d callinfo: %d",
                tag, btype, is_callinfo);
            EXFAIL_OUT(ret);
        }
        
        /* add buffer to vptr list */
        if (EXSUCCEED!=ndrx_growlist_append(&list, &current_vptr))
        {
            NDRX_LOG(log_error, "Failed to append vptr list with tag: %u addr: %p len: %ld - OOM?",
                    tag, current_vptr.data, current_vptr.len);
            ndrx_TPset_error_fmt(TPEOS, "Failed to append vptr list with tag: %u addr: %p len: %ld - OOM?",
                    tag, current_vptr.data, current_vptr.len);
            EXFAIL_OUT(ret);
        }
        
        /* check the primary buffer status */
        if (!primary_loaded && !is_callinfo)
        {
            *odata = current_vptr.data;
            *olen = current_vptr.len;   
            
            /* get buffer object, assoc new call header, destroy oldone if
             * existed */
            if (1==tag)
            {
                buffer_obj_t * buffer_info = ndrx_find_buffer(*odata);
                
                if (NULL!=buffer_info->callinfobuf)
                {
                    tpfree(buffer_info->callinfobuf);
                }
                /* callinfos are at pos 0 */
                access_vptr = (ndrx_mbuf_vptrs_t *)list.mem;
                buffer_info->callinfobuf = access_vptr->data;
                buffer_info->callinfobuf_len = access_vptr->len;
            }
            
            primary_loaded=EXTRUE;
        }
        
        if (primary_loaded && is_callinfo)
        {
            NDRX_LOG(log_error, "Call info expected only for primary buffer, "
                    "but at the tag %u", tag);
            ndrx_TPset_error_fmt(TPESYSTEM, "Call info expected only for primary buffer, "
                    "but at the tag %u", tag);
            EXFAIL_OUT(ret);
        }
        
    }
    
    NDRX_LOG(log_debug, "Remap the vptrs (tags) to real pointers");
    
    /* go over the list of tags, and perform map-in operation on each of it */
    
    for (i=0; i<=list.maxindexused; i++)
    {
        
        access_vptr = (ndrx_mbuf_vptrs_t *)(list.mem + (i)*sizeof(ndrx_mbuf_vptrs_t));
        
        if (BUF_TYPE_UBF==access_vptr->btype)
        {
            if (EXSUCCEED!=ndrx_mbuf_ptrs_map_in(&list, (UBFH *)access_vptr->data))
            {
                NDRX_LOG(log_error, "Failed to re-map tag %i", i);
                EXFAIL_OUT(ret);
            }
        }
    }
    
out:
    
    ndrx_growlist_free(&list);
    
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
    long new_used = ALIGNED_GEN(*olen_used);
    int pad=*olen_used-new_used;
    long tmp_olen;
    buffer_obj_t * buffer_info = ndrx_find_buffer(idata);
    typed_buffer_descr_t *descr;
    
    NDRX_LOG(log_error, "YOPT TAG START: %u", tag);
    
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
    
    NDRX_LOG(log_error, "YOPT new_use=%ld", new_used);
    
    /* put the header down */
    hdr = (ndrx_mbuf_tlv_t *)(obuf+new_used);
    
    /* get get aligned size*/
    new_used += sizeof(ndrx_mbuf_tlv_t);
    
    if (new_used> olen_max)
    {
        ndrx_TPset_error_fmt(TPEINVAL, "%s: Internal buffer size %ld, new data %ld (tag)", 
                __func__, olen_max, new_used);
        ret=EXFAIL;
        goto out;
    }
    
    hdr->tag = tag | (buffer_info->type_id << NDRX_MBUF_OFFSET);
    
    
    /* get the buffer type */    
    /* calculate the amount left */
    tmp_olen = olen_max-new_used;
    
    descr = &G_buf_descr[buffer_info->type_id];
    
    /* prepare buffer for call */
    NDRX_LOG(log_debug, "Preparing buffer tag: %u (typed %u, type %d). Source buffer %p, "
            "dest master buffer %p (work place header %p data %p) olen_max=%ld new_used=%ld pad=%d", 
            tag, hdr->tag, buffer_info->type_id, idata, obuf, hdr, hdr->data, olen_max, new_used, pad);
    
    if (EXSUCCEED!=descr->pf_prepare_outgoing(descr, idata, ilen, hdr->data, 
            &tmp_olen, flags))
    {
        NDRX_LOG(log_error, "Failed to prepare buffer tag: %u (typed %u, type %d). Source buffer %p, "
            "dest master buffer %p (work place header %p data %p) olen_max=%ld new_used=%ld pad=%d", 
            tag, hdr->tag, buffer_info->type_id, idata, obuf, hdr, hdr->data, olen_max, new_used, pad);
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
    BFLDID bfldid=BBADFLDOCC;
    BFLDOCC occ;
    char *d_ptr;
    char **lptr;
    ndrx_longptr_t tmp_ptr;
    ndrx_mbuf_ptrs_t *hptr; /* hash pointer */
    UBF_header_t *hdr = (UBF_header_t *)p_ub;
    int ftyp;
    BFLDID   *p_bfldid_start = &hdr->bfldid;
    
    NDRX_LOG(log_error, "YOPT ptr offet: %d", hdr->cache_ptr_off);
    
    state.p_cur_bfldid = (BFLDID *)(((char *)p_bfldid_start) + hdr->cache_ptr_off);
    state.cur_occ = 0;
    state.p_ub = p_ub;
    state.size = hdr->bytes_used;

    while (EXTRUE==(ret=ndrx_Bnext(&state, p_ub, &bfldid, &occ, NULL, NULL, &d_ptr)))
    {
        ftyp = bfldid >> EFFECTIVE_BITS;
        
        NDRX_LOG(log_error, "YOPT GOT FLD: %d typ %d", bfldid, ftyp);
        if (BFLD_PTR==ftyp)
        {
            lptr=(char **)d_ptr;
            
            if (NULL==(hptr = ndrx_mbuf_ptr_find(ptrs, *lptr)))
            {
                /* map the pointer (add to hash the tag, or resolve the 
                 * existing tag, and just update the UBF buffer)
                 */
                *p_tag=*p_tag+1;
                
                hptr = ndrx_mbuf_ptr_add(ptrs, *lptr, *p_tag);
                
                if (NULL==hptr)
                {
                    NDRX_LOG(log_error, "Failed to allocate ptr hash element");
                    EXFAIL_OUT(ret);
                }
                
                NDRX_LOG(log_debug, "fldid=%d occ=%d ptr to %p -> serialize to tag %u",
                        bfldid, occ, *lptr, *p_tag);

                /* how about buffer len? we do not know what is used len, so
                 * needs to extract from types? */
                if (EXSUCCEED!=ndrx_mbuf_tlv_do((char *)*lptr, EXFAIL, obuf, 
                        olen_max, olen_used, hptr->tag, flags))
                {
                    NDRX_LOG(log_error, "Failed to add ptr %p to export data tag=%u",
                            (char *)lptr, *p_tag);
                    EXFAIL_OUT(ret);
                }
            }
            
            /* update ubf buffer with tag */
            tmp_ptr=hptr->tag;
            *lptr = (char *)tmp_ptr;
        }
        else if (BFLD_UBF==ftyp)
        {
            NDRX_LOG(log_debug, "Processing sub-buffer field %d", bfldid);
            if (EXSUCCEED!=ndrx_mbuf_ptrs_map_out(ptrs, (UBFH *)d_ptr,
                obuf, olen_max, olen_used, p_tag, flags))
            {
                NDRX_LOG(log_error, "Sub-buffer for fld %d failed to map");
                EXFAIL_OUT(ret);
            }
        }
        else
        {
            /* we are done */
            ret=EXSUCCEED;
            break;
        }
    }
    
    NDRX_LOG(log_error, "YOPT! END OF SCAN");
    
    if (EXFAIL==ret)
    {
        NDRX_LOG(log_error, "Failed to loop ubf: %s", Bstrerror(Berror));
        ndrx_TPset_error_fmt(TPESYSTEM, "Failed to loop ubf: %s", Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
            
out:

    if (NULL!=ptrs)
    {
        ndrx_mbuf_ptr_free(ptrs);
    }

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
    long tlv_pos;
    ndrx_mbuf_tlv_t *tlv_hdr;
    unsigned int ptr_tag=0;
    
    if (NULL==ndrx_find_buffer)
    {
        NDRX_LOG(log_error, "Input buffer %p not atmi allocated", idata);
        ndrx_TPset_error_fmt(TPEINVAL, "Input buffer %p not atmi allocated", idata);
        EXFAIL_OUT(ret);
    }
    
    /* check for call info, if any, serialize to TLV */
    if (NULL!=ibuf->callinfobuf && !(mflags & NDRX_MBUF_FLAG_NOCALLINFO))
    {
        if (EXSUCCEED!=ndrx_mbuf_tlv_do(ibuf->callinfobuf, ibuf->callinfobuf_len, obuf, 
            *olen, &used, ptr_tag | NDRX_MBUF_CALLINFOBIT, flags))
        {
            NDRX_LOG(log_error, "Failed to run TLV on callinfo");
        }
        ptr_tag++;
    }
    
    /* serialize main buffer to TLV */
    if (EXSUCCEED!=ndrx_mbuf_tlv_do(idata, ilen, obuf, *olen, &used, 
            ptr_tag, flags))
    {
        NDRX_LOG(log_error, "Failed to run TLV on base buffer");
        EXFAIL_OUT(ret);
    }
    
    /* iterate over the main buffer, remap the pointers on the fly 
     * looping shall be done in outer function to support the recursion
     */ 
    for (tlv_pos=0; tlv_pos<used; tlv_pos+=(ALIGNED_GEN(tlv_hdr->len)+sizeof(ndrx_mbuf_tlv_t)))
    {
        int is_callinfo;
        
        tlv_hdr=(ndrx_mbuf_tlv_t *) (obuf + tlv_pos);
        is_callinfo = !!(tlv_hdr->tag & NDRX_MBUF_CALLINFOBIT);
       
        NDRX_LOG(log_error, "YOPT TAG: %x %u", tlv_hdr->tag, NDRX_MBUF_TAGTAG(tlv_hdr->tag));
        NDRX_LOG(log_debug, "Post-processing (vptr mapping) tag: %u typed: %d callinfo: %d offset: %ld", 
               NDRX_MBUF_TAGTAG(tlv_hdr->tag), NDRX_MBUF_TYPE(tlv_hdr->tag), is_callinfo, tlv_pos);
       
        /* check the buffer type, if it is UBF, then run the mapping... */
        if (BUF_TYPE_UBF == NDRX_MBUF_TYPE(tlv_hdr->tag))
        {
           if (EXSUCCEED!=ndrx_mbuf_ptrs_map_out(&ptrs, (UBFH *)tlv_hdr->data,
                obuf, *olen, &used, &ptr_tag, flags))
           {
                NDRX_LOG(log_debug, "Post-processing (vptr mapping) for tag: %d typed: %d failed callinfo: %d", 
                    NDRX_MBUF_TAGTAG(tlv_hdr->tag), NDRX_MBUF_TYPE(tlv_hdr->tag), is_callinfo);
                EXFAIL_OUT(ret);
           }
        }
    }
    
    /* finally set the data len */
    *olen = used;
    
out:
    
    NDRX_LOG(log_debug, "%ld data bytes", *olen);

    return ret;
}

/**
 * Dump the TLV data
 * @param rcv_data serialized buffer
 * @param rcv_len total buffer len
 */
expublic void ndrx_mbuf_tlv_debug (char *rcv_data, long rcv_len)
{
    ndrx_mbuf_tlv_t *tlv_hdr;
    long tlv_pos;
    unsigned int tag_exp=0;

    
    /* OK load the stuff ... */
    NDRX_LOG(log_debug, "** DUMP TLV START **");
    for (tlv_pos=0; tlv_pos<rcv_len; tlv_pos+=(ALIGNED_GEN(tlv_hdr->len)+sizeof(ndrx_mbuf_tlv_t)), tag_exp++)
    {
        int is_callinfo;
        unsigned int tag;
        int btype;
        
        tlv_hdr=(ndrx_mbuf_tlv_t *)(rcv_data + tlv_pos);
        
        tag = NDRX_MBUF_TAGTAG(tlv_hdr->tag);
        btype = NDRX_MBUF_TYPE(tlv_hdr->tag);
        is_callinfo = !!(tlv_hdr->tag & NDRX_MBUF_CALLINFOBIT);
        
        NDRX_LOG(log_debug, "Buffer tag: %u type: %d callinfo: %d len: %u aligned: %d",
                tag, btype, is_callinfo, tlv_hdr->len, ALIGNED_GEN(tlv_hdr->len));
        
        if (tag!=tag_exp)
        {
            NDRX_LOG(log_error, "ERROR: Expected tag %u but got %u", tag_exp, tag);
            return;
        }
        
        NDRX_DUMP(log_debug, "TAG data", tlv_hdr->data, tlv_hdr->len);
        
    }
    NDRX_LOG(log_debug, "** DUMP TLV END **");
    
}


/* vim: set ts=4 sw=4 et smartindent: */
