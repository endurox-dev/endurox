/**
 * @brief Singleton group support
 *
 * @file singlegrp.h
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
#ifndef SINGLEGRP_H_
#define SINGLEGRP_H_

#if defined(__cplusplus)
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <ndrx_config.h>
#include <limits.h>
#include <sys/types.h>
#include <nstopwatch.h>
#include <stdatomic.h>
#include <sys/types.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define NDRX_SG_IN_USE      0x0001  /**< Given group is used           */
#define NDRX_SG_NO_ORDER    0x0002  /**< Do not use boot order         */
#define NDRX_SG_CHK_PID     0x0004  /**< Check that PID is alive       */
#define NDRX_SG_NOORDER_LCK 0x0008  /**< Report norder group as locked */

#define NDRX_SG_RSN_NONE        0       /**< No reason                   */
#define NDRX_SG_RSN_EXPIRED     1       /**< Expird by missing refresh   */
#define NDRX_SG_RSN_NOPID       2       /**< PID missing of lock holder  */
#define NDRX_SG_RSN_REFNOFILE   3       /**< Reference file is missing   */
#define NDRX_SG_RSN_REFFFUT     4       /**< Reference file is in future */

#define NDRX_SG_PROCNAMEMAX	16	/**< Max len of the lock process */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/**
 * Shared memory entries for singleton group support
 */
typedef struct
{
    _Atomic unsigned char volatile is_locked;       /**< Is group locked? */
    _Atomic unsigned char volatile is_mmon;         /**< Is maintenace mode ON? */
    _Atomic unsigned char volatile is_srv_booted;   /**< Is servers booted, when group locked? */
    _Atomic unsigned char volatile is_clt_booted;   /**< Is clients boooted, when group locked? */
    _Atomic unsigned short volatile flags;          /**< Flags for given entry */
    _Atomic time_t last_refresh;                    /**< Last lock refresh time */
    _Atomic short volatile lockprov_srvid;          /**< Lock provder server id */  
    _Atomic short volatile lockprov_nodeid;         /**< Lock provider E/X cluster node id */
    _Atomic pid_t volatile lockprov_pid;            /**< Lock provider pid */
    char volatile lockprov_procname[NDRX_SG_PROCNAMEMAX+1];  /**< Lock provider name */
    _Atomic int reason;                             /**< Reason code for unlock */     

} ndrx_sg_shm_t;

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/** Return the ptr to single group in shared memory */
extern NDRX_API ndrx_sg_shm_t *ndrx_sg_get(int singlegrp_no);

/** Is given group locked? */
extern NDRX_API int ndrx_sg_is_locked(int singlegrp_no, char *reference_file, long flags);

/** Return snapshoot of current locking */
extern NDRX_API void ndrx_sg_get_lock_snapshoot(int *lock_status_out, int *lock_status_out_len, long flags);

/** Reset shared memory block having the singleton gorup infos */
extern NDRX_API void ndrx_sg_reset(void);

#if defined(__cplusplus)
}
#endif

#endif
/* vim: set ts=4 sw=4 et smartindent: */
