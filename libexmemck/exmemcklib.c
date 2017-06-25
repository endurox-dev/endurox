/* 
** Memory checking library
**
** @file exmemcklib.c
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
#include <exmemck.h>
#include <errno.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/

exmemck_config_t *M_config = NULL; /* global config */
exmemck_process_t *M_proc = NULL; /* global process list */

/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Get config
 * @param mask
 * @param autocreate create entry automatically...
 * @return found/allocated config block or NULL
 */
private exmemck_config_t * get_config(char *mask, int autocreate, int *p_ret, 
        int *p_is_new)
{
    exmemck_config_t * ret;
    
    EXHASH_FIND_STR(M_config, mask, ret);
    
    if (NULL==ret && autocreate)
    {
        /* Allocate the block */
        
        if (SUCCEED!=(ret=NDRX_CALLOC(1, sizeof(exmemck_config_t))))
        {
            int err = errno;
            NDRX_LOG(log_error, "Failed to allocate xmemck_config_t: %s", 
                    strerror(err));
            
            userlog("Failed to allocate xmemck_config_t: %s", 
                    strerror(err));
            FAIL_OUT((*p_ret));
        }
        
        NDRX_STRCPY_SAFE(ret->mask, mask);
        
        EXHASH_ADD_STR(M_config, mask, ret);
        
        *p_is_new = TRUE;
        
    }
out:

    return ret;
}

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
    int is_new = FALSE;
    
    exmemck_config_t * cfg, *dflt;
    
    NDRX_LOG(log_debug, "%s: enter, mask: [%s]", __func__, mask);
    
    cfg = get_config(mask, TRUE, &ret, &is_new);
    
    if (NULL==cfg || SUCCEED!=ret)
    {
        NDRX_LOG(log_error, "%s: failed to get config for mask [%s]", 
                __func__, mask);
        FAIL_OUT(ret);
    }
    
    /* search for defaults */
    if (is_new && NULL!=dlft_mask)
    {
        NDRX_LOG(log_debug, "Making init for defaults: [%s]", dlft_mask);
        
        /* ret will succeed here always! */
        dflt = get_config(dlft_mask, FALSE, &ret, NULL);
        
        if (NULL!=dflt)
        {
            /* Got defaults */
            
            NDRX_LOG(log_debug, "Got defaults...");
            cfg->flags = dflt->flags;
            cfg->interval_start_prcnt = dflt->interval_start_prcnt;
            cfg->interval_stop_prcnt = dflt->interval_stop_prcnt;
            cfg->mem_limit = dflt->mem_limit;
            cfg->percent_diff_allow = dflt->percent_diff_allow;
        }
    }
    
    
    if (flags>FAIL)
    {
        cfg->flags = flags;
    }
    
    if (interval_start_prcnt>FAIL)
    {
        cfg->interval_start_prcnt = interval_start_prcnt;
    }
    
    if (interval_stop_prcnt >FAIL)
    {
        cfg->interval_stop_prcnt = interval_stop_prcnt;
    }
    
    if (mem_limit >FAIL)
    {
        cfg->mem_limit = mem_limit;
    }
    
    if (percent_diff_allow >FAIL)
    {
        cfg->percent_diff_allow = percent_diff_allow;
    }
   
out:    
    return ret;
}

/**
 * Remove process by mask. Also will remove any monitored processes from the list
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
    
    /* If process is not running anymore, just report its stats via callback
     * and remove from the memory
     */
    
out:    
    return ret;
}

