/**
 * @brief Standard Library internals
 *
 * @file nstd_int.h
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

#ifndef NSTD_INT_H
#define	NSTD_INT_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <ndrstandard.h>
#include <inicfg.h>
#include <sys_primitives.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
    
#define NDRX_TPLOGCONFIG_VERSION_INC            0x00000001  /**< increment version */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/**
 * Feedback alloc block memory block
 * TODO: move to internal header
 */
typedef struct ndrx_fpablock ndrx_fpablock_t;
struct  ndrx_fpablock
{
    int magic;              /**< magic constant                             */
    int poolno;             /**< slot number to which block belongs         */
    int flags;              /**< flags for given alloc block                */
    volatile ndrx_fpablock_t *next;  /**< Next free block                   */
};

/**
 * One size stack for allocator
 */
typedef struct ndrx_fpastack ndrx_fpapool_t;
struct  ndrx_fpastack
{
    int bsize;                       /**< this does not include header size          */
    int flags;                       /**< flags for given entry                      */
    volatile int num_blocks;         /**< min number of blocks int given size range  */
    volatile int cur_blocks;         /**< Number of blocks allocated                 */
    volatile long allocs;            /**< number of allocs done, for stats           */
    volatile ndrx_fpablock_t *stack; /**< stack head                                 */
    NDRX_SPIN_LOCKDECL(spinlock);    /**< spinlock for protecting given size         */
};

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
    
extern NDRX_API int ndrx_inicfg_get_subsect_int(ndrx_inicfg_t *cfg, 
        char **resources, char *section, ndrx_inicfg_section_keyval_t **out);

extern NDRX_API void ndrx_fpstats(int poolno, ndrx_fpapool_t *p_stats);

extern NDRX_API void ndrx_init_fail_banner(void);

extern NDRX_API ndrx_debug_file_sink_t* ndrx_debug_get_sink(char *fname, int do_lock, 
        ndrx_debug_t *dbg_ptr, int *p_ret);

extern NDRX_API int ndrx_debug_changename(char *toname, int do_lock, 
        ndrx_debug_t *dbg_ptr, ndrx_debug_file_sink_t* fileupdate);
extern NDRX_API void ndrx_debug_force_closeall(void);
extern NDRX_API void ndrx_debug_refcount(int *sinks, int *refs);
extern NDRX_API int ndrx_debug_unset_sink(ndrx_debug_file_sink_t* mysink, int do_lock, int force);
extern NDRX_API void ndrx_debug_addref(ndrx_debug_file_sink_t* mysink);
extern NDRX_API int ndrx_debug_reopen_all(void);

extern NDRX_API int tplogconfig_int(int logger, int lev, char *debug_string, char *module, 
        char *new_file, long flags);
extern NDRX_API int ndrx_debug_is_proc_stderr(void);

#ifdef	__cplusplus
}
#endif

#endif	/* NSTD_INT_H */

/* vim: set ts=4 sw=4 et smartindent: */
