/* 
** Thread synchronization routines. Mutexes & Spinlocks.
**
** @file thlock.h
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, ATR Baltic, SIA. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or ATR Baltic's license for commercial use.
** -----------------------------------------------------------------------------
** GPL license:
** 
** This program is free software; you can redistribute it and/or modify it under
** the terms of the GNU General Public License as published by the Free Software
** Foundation; either version 2 of the License, or (at your option) any later
** version.
**
** This program is distributed in the hope that it will be useful, but WITHOUT ANY
** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
** PARTICULAR PURPOSE. See the GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License along with
** this program; if not, write to the Free Software Foundation, Inc., 59 Temple
** Place, Suite 330, Boston, MA 02111-1307 USA
**
** -----------------------------------------------------------------------------
** A commercial use license is available from ATR Baltic, SIA
** contact@atrbaltic.com
** -----------------------------------------------------------------------------
*/
#ifndef THLOCK_H
#define	THLOCK_H


#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <pthread.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

/* *** PTHREAD MUTEX *** */
#define MUTEX_LOCKDECL(X) static pthread_mutex_t X = PTHREAD_MUTEX_INITIALIZER; 

#define MUTEX_LOCK_V(X) pthread_mutex_lock(&X);

#define MUTEX_UNLOCK_V(X) pthread_mutex_unlock(&X);

#define MUTEX_LOCK \
		MUTEX_LOCKDECL(__mutexlock);\
		MUTEX_LOCK_V(__mutexlock);

#define MUTEX_UNLOCK MUTEX_UNLOCK_V(__mutexlock);

/* *** SPIN LOCKS *** */

/* data type used for external controlled locking. */
#define SPIN_LOCKDECL(X) static volatile int X = 0;

/**
 * Create spinlock
 * @param X - volatile int, locking variable
 * Must be first thing of the block!
 */
#define SPIN_LOCK_V(X) \
	int __count = 0;\
	while(!__sync_bool_compare_and_swap(&X, 0, 1))\
	{\
		__count++;\
		if (0 == __count % 2)\
		{\
			sched_yield();\
		}\
	}\
	
/**
 * Unlock spin.
 * @param X - volatile int, locking variable
 * Must be defined at the end of the locking block.
 */
#define SPIN_UNLOCK_V(X)\
	__sync_synchronize();\
	X = 0;\

/* Use default __spinlock variable */
#define SPIN_LOCK \
		static volatile int __spinlock = 0;\
		SPIN_LOCK_V(__spinlock);

/* Use default __spinlock variable */
#define SPIN_UNLOCK SPIN_UNLOCK_V(__spinlock);
	

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

#ifdef	__cplusplus
}
#endif

#endif	/* THLOCK_H */

