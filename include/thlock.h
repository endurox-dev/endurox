/**
 * @brief Thread synchronization routines. Mutexes & Spinlocks.
 *
 * @file thlock.h
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
#ifndef THLOCK_H
#define	THLOCK_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <pthread.h>
#include <userlog.h>

#include "ndrstandard.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/


/* *** PTHREAD MUTEX *** */
    
#define MUTEX_VAR(X)        pthread_mutex_t X
#define MUTEX_VAR_INIT(X)   do {\
                    if ( EXSUCCEED!=pthread_mutex_init ( &(X), NULL ) ) \
                    {\
                        userlog("Mutex init fail: %s", #X);\
                        exit ( 1 );\
                    }\
                } while (0)
/*
 Have custom error checks...
 */
#if NDRX_MUTEX_DEBUG

/**
 * Declare mutex with debug mode on
 * @param X mutex var name
 */
#define MUTEX_LOCKDECL(X) pthread_mutex_t X = PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP;

/**
 * Lock the mutex
 * @param X mutex var name
 */
#define MUTEX_LOCK_V(X) do {int ndrx_mut_ret; if (0!=(ndrx_mut_ret=pthread_mutex_lock(&X)))\
     {userlog("Mutex lock failed: %d/%s at %s:%u %s() - aborting", \
        ndrx_mut_ret, strerror(ndrx_mut_ret), __FILE__, __LINE__, __func__); abort();} } while (0)

/**
 * Unlock the mutex
 * @param X mutex var name
 */
#define MUTEX_UNLOCK_V(X) do {int ndrx_mut_ret; if (0!=(ndrx_mut_ret=pthread_mutex_unlock(&X)))\
     {userlog("Mutex unlock failed: %d/%s at %s:%u %s() - aborting", \
        ndrx_mut_ret, strerror(ndrx_mut_ret), __FILE__, __LINE__, __func__); abort();} } while (0)

#else 

/**
 * Declare static initalized mutex
 * @param X mutex variable name
 */
#define MUTEX_LOCKDECL(X) pthread_mutex_t X = PTHREAD_MUTEX_INITIALIZER;
    
/**
 * Lock the mutex
 * @param X mutex var name
 */
#define MUTEX_LOCK_V(X) pthread_mutex_lock(&X);
    
/**
 * Unlock the mutex
 * @param X mutex var name
 */
#define MUTEX_UNLOCK_V(X) pthread_mutex_unlock(&X);

#endif

    
#define MUTEX_TRYLOCK_V(X) pthread_mutex_trylock(&X)
    
/**
 * Destroy mutex var
 */
#define MUTEX_DESTROY_V(X) pthread_mutex_destroy(&X)

/**
 * Define static mutex and do lock / for scope
 */
#define MUTEX_LOCK \
		static MUTEX_LOCKDECL(__mutexlock);\
		MUTEX_LOCK_V(__mutexlock);

/**
 * Unlock the static mutex / for scope
 */
#define MUTEX_UNLOCK MUTEX_UNLOCK_V(__mutexlock);

/* *** NDRX LOCKS *** */
    
/**
 * Spinlock init
 */
#define NDRX_SPIN_INIT_V(X)   \
                do {\
                    if ( EXSUCCEED!=pthread_spin_init ( &(X), PTHREAD_PROCESS_PRIVATE ) ) \
                    {\
                        userlog("Spinlock init fail: %s", #X);\
                        exit ( 1 );\
                    }\
                }while(0)
/**
 * Spinlock declare variable
 * @param X name of declare. If needs static, just add before
 */
#define NDRX_SPIN_LOCKDECL(X) pthread_spinlock_t X
    
/**
 * Spinlock lock
 * This will ensure that scheduler yield would run
 * @param X name of variable of spinlock
 */
#define NDRX_SPIN_LOCK_V(X) \
    while (1)\
    {\
        int ndrx_try_counter; int ndrx_try_done=EXFALSE;\
        for (ndrx_try_counter=0; ndrx_try_counter < 1000; ndrx_try_counter++)\
        {\
            if (EXSUCCEED==pthread_spin_trylock(&X)) {ndrx_try_done=EXTRUE; break;}\
        }\
        if (ndrx_try_done) {break;}\
        sched_yield();\
    }

/**
 * Try lock
 * @param X lock var name
 * @return 0 - if locked, else error
 */
#define NDRX_SPIN_TRYLOCK_V(X) pthread_spin_trylock(&X)
    
/**
 * Spinlock unlock
 * @param X name of variable of spinlock
 */
#define NDRX_SPIN_UNLOCK_V(X) pthread_spin_unlock(&X)

/**
 * Destroy spinlock 
 * @param X spinlock var
 */
#define NDRX_SPIN_DESTROY_V(X) pthread_spin_destroy(&X)
    
/**
 * Delcare RW mutex with init
 * @param X rw lock variable name
 */ 
#define NDRX_RWLOCK_DECL(X) pthread_rwlock_t X = PTHREAD_RWLOCK_INITIALIZER;
    
/**
 * Read lock
 * @param X name of the lock variable
 */
#define NDRX_RWLOCK_RLOCK_V(X) \
    do {int ndrx_mut_ret; if (0!=(ndrx_mut_ret=pthread_rwlock_rdlock(&X)))\
     {userlog("pthread_rwlock_rdlock failed %d/%s at %s:%u %s() - aborting", \
        ndrx_mut_ret, strerror(ndrx_mut_ret), __FILE__, __LINE__, __func__); abort();} } while (0)
        
    
/**
 * Write lock
 * @param X name of the lock variable
 */
#define NDRX_RWLOCK_WLOCK_V(X) \
    do {int ndrx_mut_ret; if (0!=(ndrx_mut_ret=pthread_rwlock_wrlock(&X)))\
     {userlog("pthread_rwlock_wrlock failed %d/%s at %s:%u %s() - aborting", \
        ndrx_mut_ret, strerror(ndrx_mut_ret), __FILE__, __LINE__, __func__); abort();} } while (0)

/**
 * RW unlock
 * @param X name of rw lock variable
 */
#define NDRX_RWLOCK_UNLOCK_V(X) \
    do {int ndrx_mut_ret; if (0!=(ndrx_mut_ret=pthread_rwlock_unlock(&X)))\
     {userlog("pthread_rwlock_unlock failed %d/%s at %s:%u %s() - aborting", \
        ndrx_mut_ret, strerror(ndrx_mut_ret), __FILE__, __LINE__, __func__); abort();} } while (0)

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

#ifdef	__cplusplus
}
#endif

#endif	/* THLOCK_H */

/* vim: set ts=4 sw=4 et smartindent: */
