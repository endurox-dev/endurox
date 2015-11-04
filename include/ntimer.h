/* 
** Timer handler
**
** @file ntimer.h
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

#ifndef NTIMER_H
#define	NTIMER_H

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
    /*struct timeval  timeval;*/
    struct timespec t;
} n_timer_t;
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
void n_timer_reset(n_timer_t *timer);
long n_timer_get_delta(n_timer_t *timer);
long n_timer_get_delta_sec(n_timer_t *timer);
char *decode_msec(long t, int slot, int level, int levels);
char *n_timer_decode(n_timer_t *timer, int slot);
long n_timer_diff(n_timer_t *t1, n_timer_t *t2);

void n_timer_minus(n_timer_t *timer, long msec);
void n_timer_plus(n_timer_t *timer, long msec);
#ifdef	__cplusplus
}
#endif

#endif	/* NTIMER_H */

