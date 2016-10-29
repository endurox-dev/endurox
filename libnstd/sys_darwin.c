/* 
** Darwin Abstraction Layer (DAL)
**
** @file sys_darwin.c
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

/*---------------------------Includes-----------------------------------*/
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <stdarg.h>
#include <ctype.h>
#include <memory.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>
#include <pthread.h>
#include <string.h>
#include <mach/mach_time.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <nstdutil.h>
#include <limits.h>
#include <sys_unix.h>
#include <utlist.h>


/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define MT_NANO (+1.0E-9)
#define MT_GIGA UINT64_C(1000000000)
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/* TODO create a list of timers,*/
static double mt_timebase = 0.0;
static uint64_t mt_timestart = 0;
/*---------------------------Prototypes---------------------------------*/

/**
 * Return list of message queues (actually it is list of named pipes
 * as work around for missing posix queue listing functions.
 */
public string_list_t* ndrx_sys_mqueue_list_make(char *qpath, int *return_status)
{
    return ndrx_sys_folder_list(qpath, return_status);
}



/* TODO be more careful in a multithreaded environement */
public int clock_gettime(clockid_t clk_id, struct timespec *tp)
{
    kern_return_t retval = KERN_SUCCESS;
    if( clk_id == TIMER_ABSTIME)
    {
        if (!mt_timestart) { // only one timer, initilized on the first call to the TIMER
            mach_timebase_info_data_t tb = { 0 };
            mach_timebase_info(&tb);
            mt_timebase = tb.numer;
            mt_timebase /= tb.denom;
            mt_timestart = mach_absolute_time();
        }

        double diff = (mach_absolute_time() - mt_timestart) * mt_timebase;
        tp->tv_sec = diff * MT_NANO;
        tp->tv_nsec = diff - (tp->tv_sec * MT_GIGA);
    }
    else // other clk_ids are mapped to the coresponding mach clock_service
    {
        clock_serv_t cclock;
        mach_timespec_t mts;

        host_get_clock_service(mach_host_self(), clk_id, &cclock);
        retval = clock_get_time(cclock, &mts);
        mach_port_deallocate(mach_task_self(), cclock);

        tp->tv_sec = mts.tv_sec;
        tp->tv_nsec = mts.tv_nsec;
    }

    return retval;
}

