/**
 * @brief Basic OS elements
 *
 * @file sys_primitives.h
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
#ifndef SYS_PRIMITIVES_H
#define	SYS_PRIMITIVES_H

#ifdef	__cplusplus
extern "C" {
#endif
/*---------------------------Includes-----------------------------------*/
#include <ndrx_config.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
#ifdef EX_OS_DARWIN
/* Darwin does not have spinlock... */
typedef int pthread_spinlock_t;
#endif
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

#ifdef EX_OS_DARWIN
extern NDRX_API int pthread_spin_init(pthread_spinlock_t *lock, int pshared);
extern NDRX_API int pthread_spin_destroy(pthread_spinlock_t *lock);
extern NDRX_API int pthread_spin_lock(pthread_spinlock_t *lock);
extern NDRX_API int pthread_spin_trylock(pthread_spinlock_t *lock);
extern NDRX_API int pthread_spin_unlock(pthread_spinlock_t *lock);
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* SYS_DARWIN_H */

/* vim: set ts=4 sw=4 et smartindent: */
