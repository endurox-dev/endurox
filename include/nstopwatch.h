/**
 * @brief Timer handler
 *
 * @file nstopwatch.h
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

#ifndef NSTOPWATCH_H
#define	NSTOPWATCH_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <time.h>

#include "ndebug.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/* Dump the contents of N timer. */
#define N_TIMER_DUMP(L, T, D) NDRX_LOG(L, "%s sec=%ld nsec=%ld", T, \
                                D.t.tv_sec, D.t.tv_nsec);
    
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/**
 * Timer struct
 */
typedef struct
{
    struct timespec t;
} ndrx_stopwatch_t;
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
extern NDRX_API void ndrx_stopwatch_reset(ndrx_stopwatch_t *timer);
extern NDRX_API long ndrx_stopwatch_get_delta(ndrx_stopwatch_t *timer);
extern NDRX_API long ndrx_stopwatch_get_delta_sec(ndrx_stopwatch_t *timer);
extern NDRX_API char *ndrx_decode_msec(long t, int slot, int level, int levels);
extern NDRX_API char *ndrx_stopwatch_decode(ndrx_stopwatch_t *timer, int slot);
extern NDRX_API long long ndrx_stopwatch_diff(ndrx_stopwatch_t *t1, ndrx_stopwatch_t *t2);
extern NDRX_API void ndrx_stopwatch_minus(ndrx_stopwatch_t *timer, long long msec);
extern NDRX_API void ndrx_stopwatch_plus(ndrx_stopwatch_t *timer, long long msec);
#ifdef	__cplusplus
}
#endif

#endif	/* NSTOPWATCH_H */

/* vim: set ts=4 sw=4 et smartindent: */
