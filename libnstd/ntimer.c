/* 
** Basic stop-watch implementation.
**
** @file ntimer.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, Mavimax, Ltd. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or Mavimax's license for commercial use.
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
** A commercial use license is available from Mavimax, Ltd
** contact@mavimax.com
** -----------------------------------------------------------------------------
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include <ndrx_config.h>
#include <sys_unix.h>
#include <ndrstandard.h>
#include <ntimer.h>
#include "ndebug.h"
#include <time.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Decode time in human readable form.
 * Return only major units.
 * @param t
 * @param slot
 * @return 
 */
public char *ndrx_decode_msec(long t, int slot, int level, int levels)
{
    static char text[20][128];
    char tmp[128];
    long next_t=0;
#define DEC_SECOND      ((long)1000)
#define DEC_MINUTE      ((long)1000*60)
#define DEC_HOUR        ((long)1000*60*60)
#define DEC_DAY         ((long)1000*60*60*24)
#define DEC_WEEK        ((long)1000*60*60*24*7)
#define DEC_MONTH       ((long long)1000*60*60*24*7*4)
#define DEC_YEAR        ((long long)1000*60*60*24*365)
#define DEC_MILLENIUM     ((long long)1000*60*60*24*365*1000)
    
    level++;
    
    if ((double)t/DEC_SECOND < 1.0) /* Less that second */
    {
        sprintf(tmp, "%ldms", t);
    }
    else if ((double)t/DEC_MINUTE < 1.0) /* less than minute */
    {
        sprintf(tmp, "%lds", t/DEC_SECOND);
        
        if (level<levels)
            next_t = t%DEC_SECOND;
    }
    else if ((double)t/DEC_HOUR < 1.0) /* less that minute */
    {
        sprintf(tmp, "%ldm", t/DEC_MINUTE);
        
        if (level<levels)
            next_t = t%DEC_MINUTE;
    }
    else if ((double)t/DEC_DAY < 1.0) /* less than hour */
    {
        sprintf(tmp, "%ldh", t/DEC_HOUR);
        
        if (level<levels)
            next_t = t%DEC_HOUR;
    }
    /* Days */
    else if ((double)t/DEC_WEEK < 1.0) /* Less that week */
    {
        sprintf(tmp, "%ldd", t/DEC_DAY);
        
        if (level<levels)
            next_t = t%DEC_DAY;
    }
    else if ((double)t/DEC_MONTH < 1.0) /* less than month */
    {
        sprintf(tmp, "%ldw", t/DEC_WEEK);
        
        if (level<levels)
            next_t = t%DEC_WEEK;
    }
    else if ((double)t/DEC_YEAR < 1.0) /* less than year */
    {
        sprintf(tmp, "%lldM", t/DEC_MONTH);
        
        if (level<levels)
            next_t = t%DEC_MONTH;
    }
    else if ((double)t/DEC_MILLENIUM < 1.0) /* less than 1000 years */
    {
        sprintf(tmp, "%lldY", t/DEC_YEAR);
        
        if (level<levels)
            next_t = t%DEC_YEAR;
    }
    
    if (level==1)
        strcpy(text[slot], tmp);
    else
        strcat(text[slot], tmp);
    
    if (next_t)
        ndrx_decode_msec(next_t, slot, level, levels);
    
    return text[slot];
}

/**
 * Decode timer.
 * @param timer
 * @param slot
 * @return 
 */
public char *ndrx_stopwatch_decode(ndrx_stopwatch_t *timer, int slot)
{
    static char *na="N/A";
    
    if (NULL==timer || 0==timer->t.tv_sec)
    {
        return na;
    }
    
    return ndrx_decode_msec(ndrx_stopwatch_get_delta(timer), slot, 0, 2);
}

/**
 * Reset timer
 * @param timer
 */
public void ndrx_stopwatch_reset(ndrx_stopwatch_t *timer)
{
    clock_gettime(CLOCK_MONOTONIC, &timer->t);
}

/**
 * return diff in milliseconds of two timers.
 * @param timer1
 * @param timer2
 * @return diff in milliseconds
 */
public long long ndrx_stopwatch_diff(ndrx_stopwatch_t *t1, ndrx_stopwatch_t *t2)
{
    long long t1r = ((long long)t1->t.tv_sec)*1000 + t1->t.tv_nsec/1000000; /* Convert to milliseconds */
    long long t2r = ((long long)t2->t.tv_sec)*1000 + t2->t.tv_nsec/1000000; /* Convert to milliseconds */
    long long ret = (t1r-t2r);
/* DEBUG:    NDRX_LOG(log_error, "t1r (%d) = %lld t2r=%lld ret=%lld", t1->t.tv_sec, t1r, t2r, ret); */
    return ret;
}

/**
 * Get time spent in milliseconds
 * @param timer
 * @return time spent in milliseconds
 */
public long ndrx_stopwatch_get_delta(ndrx_stopwatch_t *timer)
{
    struct timespec t;
    long ret;

    clock_gettime(CLOCK_MONOTONIC, &t);
    
    /* calculate delta */
    ret = (t.tv_sec - timer->t.tv_sec)*1000 /* Convert to milliseconds */ +
               (t.tv_nsec - timer->t.tv_nsec)/1000000; /* Convert to milliseconds */

    return ret;
}

/**
 * Get time spent in seconds
 * @param timer
 * @return time spent in seconds
 */
public long ndrx_stopwatch_get_delta_sec(ndrx_stopwatch_t *timer)
{
    return (ndrx_stopwatch_get_delta(timer)/1000);
}

/**
 * Timer plus.
 * TODO: What to do if msec is <0 ?
 * @param timer
 * @param msec
 * @return 
 */
public void ndrx_stopwatch_plus(ndrx_stopwatch_t *timer, long long msec)
{
    if (msec < 0)
    {
        ndrx_stopwatch_minus(timer, msec * -1);
    }
    else 
    {
        long over = msec % 1000; /* Real milli seconds */
        long nsec_tot = over*1000000;
        timer->t.tv_sec+= msec/1000;

        if (timer->t.tv_nsec + nsec_tot> 1000000000)
        {
            timer->t.tv_sec++;
            nsec_tot-= 1000000000;
        }

        timer->t.tv_nsec+=nsec_tot;
    }
}

/**
 * Reduce the time value...
 * @param timer
 * @param msec
 * @return 
 */
public void ndrx_stopwatch_minus(ndrx_stopwatch_t *timer, long long msec)
{
    if (msec < 0)
    {
        ndrx_stopwatch_plus(timer, msec * -1);
    }
    else
    {
        long long over = msec % 1000; /* Real milli seconds */
        long long nsec_tot = over*1000000;

        timer->t.tv_sec-= msec/1000;

        if (timer->t.tv_nsec - nsec_tot < 0)
        {
            timer->t.tv_sec--;
            nsec_tot-= 1000000000;
        }

        timer->t.tv_nsec-=nsec_tot;
    }
}

