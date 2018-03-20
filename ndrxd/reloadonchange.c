/* 
** Reload services on change - support library.
** NOTE: Currently list will not clean up during the runtime.
** So if config changes tons of times with different bins, then this will grow,
** TODO: Bind to config loader to remove un-needed binaries.
** @file reloadonchange.c
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
#include <errno.h>
#include <memory.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <utlist.h>
#include <fcntl.h>

#include <ndrstandard.h>
#include <ndrxd.h>
#include <atmi_int.h>
#include <nstopwatch.h>

#include <ndebug.h>
#include <cmd_processor.h>
#include <signal.h>

#include "userlog.h"

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/* assume this is max time for reload cmd to hang in incoming ndrxd queue of commands*/
#define ROC_MAX_CYCLES_STAY_IN_RELOAD       5
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/**
 * Have a track of binaries found in system & their check values.
 */
typedef struct roc_exe_registry roc_exe_registry_t;
struct roc_exe_registry
{
    char binary_path[PATH_MAX+1];
    time_t mtime;
    unsigned sanity_cycle; /* sanity cycle */
    int reload_issued; /* is reload issued? If so do not issue any more checks  */
    
    EX_hash_handle hh;         /* makes this structure hashable */
};

/*---------------------------Globals------------------------------------*/

exprivate roc_exe_registry_t* M_binreg = NULL;
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Calculate checksum for binary if exists.
 * @param checksum object
 */
exprivate void roc_calc_tstamp(roc_exe_registry_t *bin, unsigned sanity_cycle)
{
    struct stat file_stat;
    
    if (EXSUCCEED==stat(bin->binary_path, &file_stat))
    {
        bin->mtime = file_stat.st_mtime;
        bin->sanity_cycle = sanity_cycle;   
    }
    
}

/**
 * Return the object to binary, if not found then crate one
 * @param path full path to binary
 * @return NULL (if failure of mem) or ptr to object
 */
exprivate roc_exe_registry_t *rco_get_binary(char *binary_path, unsigned sanity_cycle)
{
    roc_exe_registry_t *ret = NULL;
    
    EXHASH_FIND_STR( M_binreg, binary_path, ret);
    
    if (NULL==ret)
    {
        /* allocate binary */
        roc_exe_registry_t * ret = NDRX_CALLOC(1, sizeof(roc_exe_registry_t));

        if (NULL==ret)
        {
            userlog("malloc failed: %s", strerror(errno));
            goto out;
        }
        strcpy(ret->binary_path, binary_path);
        
        EXHASH_ADD_STR(M_binreg, binary_path, ret);
        
        /* calculate initial checksum */
        roc_calc_tstamp(ret, sanity_cycle);
    }
    
out:
    return ret;
}

/**
 * Return TRUE if reload is in progress.
 * This will try to fix things if issue is hanged for very long time 
 * (removed from config?)
 * @return 
 */
expublic int roc_is_reload_in_progress(unsigned sanity_cycle)
{
    roc_exe_registry_t *el = NULL;
    roc_exe_registry_t *elt = NULL;
    long diff;
    
    /* go over the hash and return TRUE, if in progress... */
    
    EXHASH_ITER(hh, M_binreg, el, elt)
    {
        if (el->reload_issued)
        {
            /* assume 5 cycles is max */
            if ((diff=labs((long)el->sanity_cycle-(long)sanity_cycle)) > ROC_MAX_CYCLES_STAY_IN_RELOAD)
            {
                NDRX_LOG(log_error, "Current cycle %u reload cycle %u - assume executed",
                        el->sanity_cycle, sanity_cycle);
                break;
                
            } 
            else
            {
                NDRX_LOG(log_error, "%s - enqueued for reload", el->binary_path);
                return EXTRUE;
            }
        }
    }
    
    return EXFALSE; 
}

/**
 * Check binary 
 * @param path
 * @param sanity_cycle
 * @return TRUE if needs to issue reload, FALSE if checksums not changed
 */
expublic int roc_check_binary(char *binary_path, unsigned sanity_cycle)
{
    int ret= EXFALSE;
    roc_exe_registry_t *bin = NULL;
    time_t old_mtime;
    
    /* check the table if reload needed then no calculation needed */
    if (roc_is_reload_in_progress(sanity_cycle))
    {
        ret=EXFALSE;
        goto out;
    }
    
    /* search for bin, if not exists, then get checksum & add */
    if (NULL==(bin=rco_get_binary(binary_path, sanity_cycle)))
    {
        NDRX_LOG(log_error, "Failed to get reload-on-change binary "
            "(%s) - memory issues", binary_path);
        ret=EXFALSE;
        goto out;
    }
    
    /* if exists, and cycle equals to current one, then no cksum */
    if (bin->sanity_cycle==sanity_cycle)
    {
        /* no cksum changed... as already calculated. */
        NDRX_LOG(log_debug, "Already checked at this cycle... (cached)");
        ret=EXFALSE;
        goto out;
    }
    
    /* if exists and cycle not equals, calc the checksum  */
    old_mtime = bin->mtime;
    
    roc_calc_tstamp(bin, sanity_cycle);
    
    if (old_mtime!=bin->mtime)
    {
        NDRX_LOG(log_warn, "Binary [%s] timestamp changed", binary_path);
        bin->reload_issued = EXTRUE; /* so will issue reload */
        
        ret = EXTRUE;
        goto out;
    }
    
out:

    NDRX_LOG(log_debug, "roc_check_binary [%s]: %s", binary_path, 
        ret?"issued reload":"reload not issued");

    return ret;
}

/**
 * Mark binary as reloaded.
 * @param path
 * @return 
 */
expublic void roc_mark_as_reloaded(char *binary_path, unsigned sanity_cycle)
{
    roc_exe_registry_t * bin = rco_get_binary(binary_path, sanity_cycle);
    
    if (NULL==bin)
    {
        NDRX_LOG(log_error, "Binary [%s] not found in hash - mem error!",
                binary_path)
    }
    else
    {
        NDRX_LOG(log_error, "Marking binary [%s]/%d as reloaded!",
                binary_path, bin->reload_issued);
                
        bin->reload_issued = EXFALSE;
    }
}
