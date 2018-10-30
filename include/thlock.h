/**
 * @brief Thread synchronization routines. Mutexes & Spinlocks.
 *
 * @file thlock.h
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
    
#define MUTEX_VAR(X)        pthread_mutex_t X
#define MUTEX_VAR_INIT(X)   pthread_mutex_init(&X, NULL)
    
#define MUTEX_LOCKDECL(X) static pthread_mutex_t X = PTHREAD_MUTEX_INITIALIZER;

#define MUTEX_LOCK_V(X) pthread_mutex_lock(&X);

#define MUTEX_UNLOCK_V(X) pthread_mutex_unlock(&X);

#define MUTEX_LOCK \
		MUTEX_LOCKDECL(__mutexlock);\
		MUTEX_LOCK_V(__mutexlock);

#define MUTEX_UNLOCK MUTEX_UNLOCK_V(__mutexlock);

/* *** EX_SPIN LOCKS *** */
#define EX_SPIN_VAR(X)        pthread_spinlock_t X
#define EX_SPIN_VAR_INIT(X)   if ( EXSUCCEED!=pthread_spin_init ( &X, 0 ) ) \
                    {\
                        userlog("Spinlock %s init fail: %s", #X, strerror(errno));\
                        exit ( 1 );\
                    }
    
#define EX_SPIN_LOCKDECL(X) static pthread_spinlock_t X;\
        MUTEX_LOCKDECL(X##__initlock__);\
        static int X##__first__ = EXTRUE;

#define EX_SPIN_LOCK_V(X)\
    if (X##__first__)\
    {\
        MUTEX_LOCK_V(X##__initlock__);\
        if (X##__first__)\
        {\
            EX_SPIN_VAR_INIT(X);\
            X##__first__=EXFALSE;\
        }\
        MUTEX_UNLOCK_V(X##__initlock__);\
    }\
    pthread_spin_lock(&X);

#define EX_SPIN_UNLOCK_V(X) pthread_spin_unlock(&X);

#define EX_SPIN_LOCK \
		EX_SPIN_LOCKDECL(__spinlock);\
		EX_SPIN_LOCK_V(__spinlock);

#define EX_SPIN_UNLOCK EX_SPIN_UNLOCK_V(__spinlock);

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
