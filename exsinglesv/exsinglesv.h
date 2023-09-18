/**
 * Singleton group lock provider
 * @file exsinglesv.h
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2023, Mavimax, Ltd. All Rights Reserved.
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

#ifndef EXSINGLESV_H
#define	EXSINGLESV_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <limits.h>
#include <exthpool.h>
#include <singlegrp.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define NDRX_LOCK_FILE_1         0 /**< first lock file name, permanent  */
#define NDRX_LOCK_FILE_2         1/**< Ping lock                         */

#define NDRX_LOCKE_FILE          -1 /**< failed to create file           */
#define NDRX_LOCKE_LOCK          -2 /**< failed to lock file             */
#define NDRX_LOCKE_BUSY          -3 /**< file is locked                  */
#define NDRX_LOCKE_NOLOCK        -4 /**< File is not locked              */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/**
 * Admin server configuration
 */
typedef struct
{
    int procgrp_lp_no; /**< Providing lock for this group   */
    char lockfile_1[PATH_MAX+1]; /**< Lock file 1       */
    char lockfile_2[PATH_MAX+1]; /**< Lock file 2       */
    char exec_on_bootlocked[PATH_MAX+1]; /**< Exec on boot locked */
    char exec_on_locked[PATH_MAX+1]; /**< Exec on locked*/
    int chkinterval; /**< Check interval                */
    int locked_wait; /**< Number of check cycles for lock takeover */
    int locked1; /**< Locked                            */
    int locked2; /**< File segment 1 is locked          */
    int first_boot; /**< Is booting up                  */
    
    int wait_counter;   /**< if this is not first boot, then wait */
    int is_locked;  /**< Is process fully locked        */

    threadpool thpool_local;    /**< local requests */
    threadpool thpool_remote;   /**< remote requests */
    int threads;
    int svc_timeout; /**< Service timeout (for remote calls) */
    int clock_tolerance; /**< Allowed between us and them*/

} ndrx_exsinglesv_conf_t;

/**
 * Locking state machine context
 */
typedef struct
{
    ndrx_sg_shm_t *pshm; /**< Shared memory ptr of current group        */
    ndrx_sg_shm_t  local;   /**< Atomically copied shard memory entry   */
    /** 
     * New precalcualted refresh time, so that at of the system
     * freeze this would already be old
     */
    time_t new_refresh;
} ndrx_locksm_ctx_t;

/*---------------------------Globals------------------------------------*/
extern ndrx_exsinglesv_conf_t ndrx_G_exsinglesv_conf; /**< Configuration */
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
extern int ndrx_exsinglesv_file_lock(int lock_no, char *lockfile);
extern int ndrx_exsinglesv_file_unlock(int lock_no);
extern int ndrx_exsinglesv_file_chkpid(int lock_no, char *lockfile);
extern void ndrx_exsinglesv_uninit(int normal_unlock, int force_unlock);
extern int ndrx_exsinglesv_sm_run(void);
extern int ndrx_exsinglesv_sm_comp(void);
extern int ndrx_exsinglesv_sg_is_locked(ndrx_locksm_ctx_t *lock_ctx);
extern void ndrx_exsingle_local (void *ptr, int *p_finish_off);
extern void ndrx_exsingle_remote (void *ptr, int *p_finish_off);
extern void ndrx_exsinglesv_set_error_fmt(UBFH *p_ub, long error_code, const char *fmt, ...);
extern long ndrx_exsinglesv_ping_read(int procgrp_no);
extern int ndrx_exsinglesv_ping_do(ndrx_locksm_ctx_t *lock_ctx);

#ifdef	__cplusplus
}
#endif

#endif	/* EXSINGLESV_H */

/* vim: set ts=4 sw=4 et smartindent: */
