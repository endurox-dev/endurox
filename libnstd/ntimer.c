/* 
** Basic timer implementation.
**
** @file ntimer.c
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

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
char *decode_msec(long t, int slot, int level, int levels)
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
        decode_msec(next_t, slot, level, levels);
    
    return text[slot];
}

/**
 * Decode timer.
 * @param timer
 * @param slot
 * @return 
 */
char *n_timer_decode(n_timer_t *timer, int slot)
{
    static char *na="N/A";
    
    if (NULL==timer || 0==timer->t.tv_sec)
    {
        return na;
    }
    
    return decode_msec(n_timer_get_delta(timer), slot, 0, 2);
}

/**
 * Reset timer
 * @param timer
 */
void n_timer_reset(n_timer_t *timer)
{
    clock_gettime(CLOCK_MONOTONIC, &timer->t);
}

/**
 * return diff in milliseconds of two timers.
 * @param timer1
 * @param timer2
 * @return diff in milliseconds
 */
long n_timer_diff(n_timer_t *t1, n_timer_t *t2)
{
    long t1r = t1->t.tv_sec*1000 + t1->t.tv_nsec/1000000; /* Convert to milliseconds */
    long t2r = t2->t.tv_sec*1000 + t2->t.tv_nsec/1000000; /* Convert to milliseconds */
    
    return (t1r-t2r);
}

/**
 * Get time spent in milliseconds
 * @param timer
 * @return time spent in milliseconds
 */
long n_timer_get_delta(n_timer_t *timer)
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
long n_timer_get_delta_sec(n_timer_t *timer)
{
    return (n_timer_get_delta(timer)/1000);
}

/**
 * Timer plus.
 * TODO: What to do if msec is <0 ?
 * @param timer
 * @param msec
 * @return 
 */
void n_timer_plus(n_timer_t *timer, long msec)
{
    if (msec < 0)
    {
        n_timer_minus(timer, msec * -1);
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
void n_timer_minus(n_timer_t *timer, long msec)
{
    if (msec < 0)
    {
        n_timer_plus(timer, msec * -1);
    }
    else
    {
        long over = msec % 1000; /* Real milli seconds */
        long nsec_tot = over*1000000;

        timer->t.tv_sec-= msec/1000;

        if (timer->t.tv_nsec - nsec_tot < 0)
        {
            timer->t.tv_sec--;
            nsec_tot-= 1000000000;
        }

        timer->t.tv_nsec-=nsec_tot;
    }
}

