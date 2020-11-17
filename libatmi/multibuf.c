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
#include <atmi_int.h>
#include <atmi_shm.h>
#include <ndrstandard.h>
#include <ndebug.h>
#include <ndrxd.h>
#include <ndrxdcmn.h>
#include <userlog.h>

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
 * @param vptrs virtual pointer associated with real pointer
 * @return ptr descr / NULL (OOM)
 */
exprivate ndrx_mbuf_ptrs_t * ndrx_mbuf_ptr_add(ndrx_mbuf_ptrs_t **ptrs, char *ptr, 
        ndrx_mbuf_vptrs_t *vptr)
{
    ndrx_mbuf_ptrs_t *ret = NULL;
    
    /* Alloc the memory block */
    if (NULL==(ret=(ndrx_mbuf_ptrs_t *)NDRX_FPMALLOC(sizeof(ndrx_mbuf_ptrs_t), 0)))
    {
        ndrx_TPset_error_fmt(TPEOS, "%s: Failed temporary hash space");
        goto out;
    }
    
    ret->ptr = ptr;
    ret->vptr = vptr;
    
    EXHASH_ADD_PTR((*ptrs), vptr, ret);
    
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
    
    /* TODO: check for call info, if any, serialize to TLV */
    
    /* TODO: serialize main buffer to TLV */
    
    /* TODO: iterate over the main buffer, remap the pointers on the fly 
     * looping shall be done in outer function to suport the recursion
     */
    
out:
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
