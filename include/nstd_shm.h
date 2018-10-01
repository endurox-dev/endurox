/**
 * @brief Enduro/X Standard library shared memory routines
 *
 * @file nstd_shm.h
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * AGPL or Mavimax's license for commercial use.
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
#ifndef NSTD_SHM_H__
#define	NSTD_SHM_H__

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define NDRX_SHM_PATH_MAX     128        /**< Max shared memory file name len           */
    
#define NDRX_SEM_SVC_OPS             0   /**< Semaphore for shared memory management    */
#define NDRX_SEM_SVC_GLOBAL_NUM      0   /**< Semaphore array for global svc managmenet */
#define NDRX_SEM_SV5LOCKS            1   /**< System V message queue lockings           */
    
#define NDRX_SEM_TYP_READ            0   /**< RW Lock - Read                */
#define NDRX_SEM_TYP_WRITE           1   /**< RW Lock - Write               */

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/**
 * Shared memory handler
 */
typedef struct
{
    char path[NDRX_SHM_PATH_MAX+1];     /**< name of shm                  */
    int fd;                             /**< opened fd                    */
    char *mem;                          /**< mapped memory                */
    int size;                           /**< size used by this shm block  */
} ndrx_shm_t;

/**
 * Semaphore handler
 */
typedef struct
{
    key_t key;              /**< Key for semaphore...                */
    int semid;
    short attached;         /**< Is attached?                        */
    int nrsems;             /**< Number of semaphores                */
    int maxreaders;         /**< Max number of readers for RW locks  */
} ndrx_sem_t;

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

extern NDRX_API int ndrx_shm_open(ndrx_shm_t *shm, int attach_on_exists);
extern NDRX_API int ndrx_shm_attach(ndrx_shm_t *shm);
extern NDRX_API int ndrx_shm_is_attached(ndrx_shm_t *shm);

extern NDRX_API int ndrx_sem_attach(ndrx_sem_t *sem);
extern NDRX_API int ndrxd_sem_open(ndrx_sem_t *sem, int attach_on_exists);
extern NDRX_API int ndrx_sem_lock(ndrx_sem_t *sem, const char *msg, int sem_num);
extern NDRX_API int ndrx_sem_unlock(ndrx_sem_t *sem, const   char *msg, int sem_num);
extern NDRX_API void ndrx_sem_remove(ndrx_sem_t *sem, int force);
extern NDRX_API int ndrx_sem_is_attached(ndrx_sem_t *sem);

extern NDRX_API int ndrx_sem_rwlock(ndrx_sem_t *sem, int sem_num, int typ);
extern NDRX_API int ndrx_sem_rwunlock(ndrx_sem_t *sem, int sem_num, int typ);

extern NDRX_API unsigned int ndrx_hash_fn( void *k );

#ifdef	__cplusplus
}
#endif

#endif	/* NSTD_CONTEXT_H */

/* vim: set ts=4 sw=4 et smartindent: */
