/**
 * @brief Feedback Pool Allocator API
 *  Basically for few fixed sizes we have configured cache for allocated
 *  blocks. If we go over the list we just malloc and free.
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
#define NDRX_FPNOFLAG         0           /**< No special flags             */
#define NDRX_FPNOPOOL         0x0001      /**< do not use alloc pool        */
#define NDRX_FPSYSBUF         0x0002      /**< use NDRX_MSGSIZEMAX          */
#define NDRX_FPABRSIZE        0x0004      /**< arbtirary size buf, free     */
    
#define NDRX_FPA_SIZE_DEFAULT   -1   /**< these are default settings, use by init */
#define NDRX_FPA_SIZE_SYSBUF   -2    /**< settings are for system buffer */

/**
 * max number of sizes    
 * Note that dynamic range will be
 * NDRX_FPA_MAX - 1, as the last entry is SYSBUF
 */
#define NDRX_FPA_MAX            6                   /**< NDRX_MSGSIZEMAX pool no*/
#define NDRX_FPA_DYN_MAX        (NDRX_FPA_MAX-1)    /**< dynamic range          */
    
#define NDRX_FPA_0_SIZE         (256)       /**< 256b pool                 */
#define NDRX_FPA_0_DNUM         25          /**< default cache size        */
    
#define NDRX_FPA_1_SIZE         (512)       /**< 512b pool                 */
#define NDRX_FPA_1_DNUM         15          /**< default cache size        */
    
#define NDRX_FPA_2_SIZE         (1*1024)    /**< 1KB pool                  */
#define NDRX_FPA_2_DNUM         10          /**< default cache size        */
    
#define NDRX_FPA_3_SIZE         (2*1024)    /**< 2KB pool                  */
#define NDRX_FPA_3_DNUM         10          /**< default cache size        */
    
#define NDRX_FPA_4_SIZE         (4*1024)    /**< 4KB pool                  */
#define NDRX_FPA_4_DNUM         10          /**< default cache size        */
    
#define NDRX_FPA_SIZE_MAX       NDRX_FPA_4_SIZE   /**< max dynamic range   */

#define NDRX_FPA_SYSBUF_SIZE         NDRX_FPA_SIZE_SYSBUF   /**< sysbuf pool    */
#define NDRX_FPA_SYSBUF_DNUM    10          /**< default cache size             */
#define NDRX_FPA_SYSBUF_POOLNO    (NDRX_FPA_MAX-1)    /**< pool number of sysbuf*/
    
/** print FPA statistics to ULOG 
#define NDRX_FPA_STATS  1*/

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
    
/**
 * Feedback alloc block memory block
 */
typedef struct ndrx_fpablock ndrx_fpablock_t;
struct  ndrx_fpablock
{
    int magic;              /**< magic constant                             */
    int poolno;               /**< slot number to which block belongs         */
    int flags;              /**< flags for given alloc block                */
    ndrx_fpablock_t *next;  /**< Next free block                            */
};

/**
 * One size stack for allocator
 */
typedef struct ndrx_fpastack ndrx_fpapool_t;
struct  ndrx_fpastack
{
    int bsize;              /**< this does not include header size          */
    int flags;              /**< flags for given entry                      */
    int num_blocks;         /**< min number of blocks int given size range  */
    int cur_blocks;         /**< Number of blocks allocated                 */
    long allocs;            /**< number of allocs done, for stats           */
    ndrx_fpablock_t *stack; /**< stack head                                 */
    pthread_spinlock_t spinlock;    /**< spinlock for protecting given size */
};

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

extern NDRX_API void ndrx_fpuninit(void);
extern NDRX_API void ndrx_fpstats(int poolno, ndrx_fpapool_t *p_stats);
extern NDRX_API void *ndrx_fpmalloc(size_t size, int flags);
extern NDRX_API void ndrx_fpfree(void *);

#if defined(__cplusplus)
}
#endif

#endif
/* vim: set ts=4 sw=4 et smartindent: */
