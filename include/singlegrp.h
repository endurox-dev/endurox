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

/**
 * Flags used for process groups & singleton groups, including internal
 * flags: 
 */
#define NDRX_SG_IN_USE      0x0001  /**< Given group is used           */
#define NDRX_SG_NO_ORDER    0x0002  /**< Do not use boot order         */
#define NDRX_SG_CHK_PID     0x0004  /**< Check that PID is alive       */
#define NDRX_SG_SRVBOOTCHK  0x0008  /**< Check that servers are booted */
#define NDRX_SG_VERIFY      0x0010  /**< Shall extra checks be made in locked mode */
#define NDRX_SG_SINGLETON   0x0020  /**< Singleton group (used for pgs) */

#define NDRX_SG_RSN_NONE        0       /**< No reason                   */
#define NDRX_SG_RSN_EXPIRED     1       /**< Expird by missing refresh   */
#define NDRX_SG_RSN_NOPID       2       /**< PID missing of lock holder  */
#define NDRX_SG_RSN_REFNOFILE   3       /**< Reference file is missing   */
#define NDRX_SG_RSN_REFFFUT     4       /**< Reference file is in future */
#define NDRX_SG_RSN_NORMAL      5       /**< Normal shutdown             */
#define NDRX_SG_RSN_LOCKE       6       /**< Locking errro (by exsinglesv)*/
#define NDRX_SG_RSN_CORRUPT     7       /**< Corrupted structures        */
#define NDRX_SG_RSN_NETLOCK     8       /**< Locked by network rsp (other node) */
#define NDRX_SG_RSN_NETSEQ      9       /**< Network seq ahead of us (>=)   */
#define NDRX_SG_RSN_FSEQ        10      /**< Their seq in file (>=) our lck time    */

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
    _Atomic long sequence;                          /**< Current ping lock seuqence */
    _Atomic int volatile lockprov_srvid;            /**< Lock provder server id */  
    _Atomic short volatile lockprov_nodeid;         /**< Lock provider E/X cluster node id */
    _Atomic pid_t volatile lockprov_pid;            /**< Lock provider pid */
    char volatile lockprov_procname[NDRX_SG_PROCNAMEMAX+1];  /**< Lock provider name */
    char volatile sg_nodes[CONF_NDRX_NODEID_COUNT]; /**< Group nodes (full list us + them) */
    _Atomic int reason;                             /**< Reason code for unlock */     

} ndrx_sg_shm_t;

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/** Return the ptr to single group in shared memory */
extern NDRX_API ndrx_sg_shm_t *ndrx_sg_get(int singlegrp_no);

extern NDRX_API void ndrx_sg_load(ndrx_sg_shm_t * sg, ndrx_sg_shm_t * sg_shm);

/** Is given group locked? */
extern NDRX_API int ndrx_sg_is_locked(int singlegrp_no, char *reference_file, long flags);
extern NDRX_API int ndrx_sg_is_locked_int(int singlegrp_no, ndrx_sg_shm_t * sg, char *reference_file, long flags);

extern NDRX_API int ndrx_sg_do_lock(int singlegrp_no, short nodeid, int srvid, char *procname,
        time_t new_last_refresh, long new_sequence);
extern NDRX_API void ndrx_sg_unlock(ndrx_sg_shm_t * sg, int reason);

/** Return snapshoot of current locking */
extern NDRX_API void ndrx_sg_get_lock_snapshoot(int *lock_status_out, int *lock_status_out_len, long flags);

/** Reset shared memory block having the singleton gorup infos */
extern NDRX_API int ndrx_sg_init(void);
extern NDRX_API void ndrx_sg_reset(void);

extern NDRX_API int ndrx_sg_do_refresh(int singlegrp_no, ndrx_sg_shm_t * sg, 
    short nodeid, int srvid, time_t new_last_refresh, long new_sequence);

extern NDRX_API int ndrx_sg_is_valid(int singlegrp_no);
extern NDRX_API void ndrx_sg_flags_set(int singlegrp_no, unsigned short flags);
extern NDRX_API unsigned short ndrx_sg_flags_get(int singlegrp_no);
extern NDRX_API void ndrx_sg_nodes_set(int singlegrp_no, char *sg_nodes);

extern NDRX_API unsigned char ndrx_sg_bootflag_clt_get(int singlegrp_no);
extern NDRX_API void ndrx_sg_bootflag_clt_set(int singlegrp_no);
extern NDRX_API unsigned char ndrx_sg_bootflag_srv_get(int singlegrp_no);
extern NDRX_API void ndrx_sg_bootflag_srv_set(int singlegrp_no);

#if defined(__cplusplus)
}
#endif

#endif
/* vim: set ts=4 sw=4 et smartindent: */
