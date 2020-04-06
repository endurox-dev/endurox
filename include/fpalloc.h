/**
 * @brief Feedback Pool Allocator API
 *  Basically we have sorted array with fixed block sizes. For each block size
 *  there is stack of free memory blocks. When malloc is requested library
 *  does binary search to find suitable block size. Once block size is found
 *  stack is checked, if there are free blocks in stack, block is returned. If
 *  block is not found, usual malloc is performed. When block is made free
 *  for particular size limits are checked and if needed page is added back to the
 *  stack.
 *  Following settings are used: blocks_min, blocks_max, blocks_hits.
 *  - if number of entries in stack are less than blocks_min, then add page
 *  back to stack.
 *  - if number of entries in stack are higher than blocks_min, but less than
 *  blocks_max and current hits is less than blocks_hits, then add page to
 *  to the stack.
 *  - if number of entries in stack are higher than blocks_min, but less than
 *  blocks_max and current hits is less than blocks_hits, then add page to
 *  to the stack.
 *  - if number of entries in stack higher than min, less than max, but blockhits>=
 *  blocks_hits, free the entry
 *  - if number of entries in stack higher than min, less than max, but blockhits
 *   >blocks_hits, add entry to fpa
 *  - if number of entries in stack equals or higher than blocks_max, free the entry
 *  - use spin locks during the calculations
 *  Metadata for each page is store in header of the malloc'd block.
 *  Standard library by it self should avoid FPA when doing config loading.
 * - In case if no block sizes matched, and there is no request for sysbuf, usual
 *  malloc shall be maded.
 *
 * @file fpalloc.h
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
#ifndef FPALLOC_H_
#define FPALLOC_H_

#if defined(__cplusplus)
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <ndrx_config.h>
#include <pthread.h>
#include <sys_primitives.h> /**< spin locks for MacOS */
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define NDRX_FPA_MAGIC          0xFEEDBCA1  /**< FPA Block magic            */
    
/* FPA FLAGS: */
#define NDRX_FPA_FNOFLAG         0           /**< No special flags           */
#define NDRX_FPA_FNOPOOL         0x0001      /**< do not use alloc pool      */
#define NDRX_FPA_FSYSBUF         0x0002      /**< use NDRX_MSGSIZEMAX        */
#define NDRX_FPA_FABRSIZE        0x0004      /**< arbtirary size buf, free   */

/**
 * max number of sizes    
 * Note that dynamic range will be
 * NDRX_FPA_MAX - 1, as the last entry is SYSBUF
 */
#define NDRX_FPA_MAX            8
#define NDRX_FPA_DYN_MAX        (NDRX_FPA_MAX-1)    /**< dynamic range      */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
    
/**
 * Feedback alloc block memory block
 */
typedef struct ndrx_fpablock ndrx_fpablock_t;
struct  ndrx_fpablock
{
    int magic;              /**< magic constant                             */
    int slot;               /**< slot number to which block belongs         */
    ndrx_fpablock_t *next;  /**< Next free block                            */
};

/**
 * One size stack for allocator
 */
typedef struct ndrx_fpastack ndrx_fpastack_t;
struct  ndrx_fpastack
{
    int bsize;           /**< this does not include header size          */
    int bmin;               /**< min number of blocks int given size range  */
    int bmax;               /**< max number of blocks int given size range  */
    int flags;              /**< flags for given entry                      */
    int blocks;             /**< Number of blocks allocated                 */
    ndrx_fpablock_t *stack; /**< stack head                                 */
    pthread_spinlock_t spinlock;    /**< spinlock for protecting given size */
    int hits;               /**< number of pool hits when goes over the min */
};

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

extern NDRX_API void *ndrx_fpmalloc(size_t size, int flags);
extern NDRX_API void ndrx_fpfree(void *);

#if defined(__cplusplus)
}
#endif

#endif
/* vim: set ts=4 sw=4 et smartindent: */
