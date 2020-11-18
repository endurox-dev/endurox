/**
 * @brief Mutli-buffer TLV handler
 *
 * @file multibuf.h
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
#ifndef __EXREGEX_H
#define	__EXREGEX_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <ndrstandard.h>
#include <ndrx_config.h>
#include <atmi_int.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
    
#define NDRX_MBUF_TBITS             4   /**< Number of type id bits             */
    
/** Data type tag offset */
#define NDRX_MBUF_OFFSET            (sizeof(int)*8-NDRX_MBUF_TBITS)

#define NDRX_MBUF_FLAG_NOCALLINFO   0x00000001  /**< Do not serialize callinfo  */

#define NDRX_MBUF_TAG_CALLINFO      0   /**< Call info reserved tag             */
#define NDRX_MBUF_TAG_BASE          1   /**< Base buffer                        */
/* Tags 2+ - are virtual pointers */

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
    
/**
 * This is multibuf TLV entry
 * The buffer type needs to be stored here too.
 * Ta
 */
typedef struct
{
    /**< This is logical tag number started from 0, 
     * last 4 bit are buffer type    
     */
    unsigned int tag;
    unsigned int len; /**< this is data length                           */
    
    /** Memory data of the buffer padded up 
     * till the modulus of EX_ALIGNMENT_BYTES is 0 
     */
    char data[0];
} ndrx_mbuf_tlv_t;

/**
 * This is list of virtual pointers descriptors
 */
typedef struct
{
    int vptr;   /**< this matches index in growlist         */
    char *data; /**< pointer to allocated memory block      */
} ndrx_mbuf_vptrs_t;

/**
 * In case if we export ptr field, we firstly add new ptr to growlist
 * and in second time we verify that this pointer is already exported
 * If already exported, use already mapped vptr.
 * (this is hash of real pointers)
 */
typedef struct
{
    char *ptr;                 /**< pointer to atmi buffer                  */
    int tag;                   /**< mapped tag                              */
    
    EX_hash_handle hh;         /**< makes this structure hashable           */
} ndrx_mbuf_ptrs_t;

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
    
#ifdef	__cplusplus
}
#endif

#endif	/* UBF_CONTEXT_H */

/* vim: set ts=4 sw=4 et smartindent: */
