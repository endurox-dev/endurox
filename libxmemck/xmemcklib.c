/* 
** Memory checking library
**
** @file xmemcklib.c
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
/*---------------------------Includes-----------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <ndrstandard.h>
#include <nstdutil.h>
#include <nstd_tls.h>
#include <string.h>
#include "thlock.h"
#include "userlog.h"
#include "ndebug.h"
#include <xmemck.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Add configuration entry to the xm lib. Each entry will epply to the
 * list of the processes
 * @param mask
 * @param mem_limit
 * @param percent_diff_allow
 * @param dlft_mask
 * @param interval_start_prcnt
 * @param interval_stop_prcnt
 * @param flags
 * @return 
 */
public int ndrx_memck_add_mask(char *mask, 
        long mem_limit, 
        int percent_diff_allow, 
        char *dlft_mask,
        int interval_start_prcnt, 
        int interval_stop_prcnt,
        long flags)
{
    int ret = SUCCEED;
    
out:    
    return ret;
}

/**
 * Remove process by mask
 * @param mask
 * @return 
 */
public int ndrx_memck_rm(char *mask)
{
    int ret = SUCCEED;
    
out:    
    return ret;
}

/**
 * Return statistics blocks, linked list...
 * @return 
 */
public void* ndrx_memck_getstats(void)
{
    return NULL;
}


/**
 * Report status callback of the process
 * status should contain:
 * - ok, or leaky.
 * The status block should be the same as the stats, some linked list of all 
 * attribs..
 * @return 
 */
public int ndrx_memck_set_status_cb(void)
{
    int ret = SUCCEED;
    
out:    
    return ret;
}
    
/**
 * Set the callback func to be invoked when process needs to be killed (too leaky!)
 * @return 
 */
public int ndrx_memck_set_kill_cb(void)
{
    int ret = SUCCEED;
    
out:    
    return ret;
}


/**
 * Reset statistics for process
 * @param mask
 * @return 
 */
public int ndrx_memck_reset(char *mask)
{
    int ret = SUCCEED;
    
out:    
    return ret;
}

/**
 * Reset statistics for the PID
 * @param pid
 * @return 
 */
public int ndrx_memck_reset_pid(pid_t pid)    
{
    int ret = SUCCEED;
    
out:    
    return ret;
}



/**
 * Run the one 
 * @return 
 */
public int ndrx_memck_tick(void)
{
    int ret = SUCCEED;
    
    /* List all processes */
    
    /* Check them against monitoring table */
    
    
    /* Check them against monitoring config */
    
    /* Add process statistics, if found in table, the add func shall create new
     * entry or append existing entry */
    
out:    
    return ret;
}

