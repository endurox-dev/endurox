/* 
 * @brief Cache boot server, for full startup will reset databases which are marked
 *   for startup clean up (bootreset flag)
 *
 * @file tpcachebtsv.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or ATR Baltic's license for commercial use.
 * -----------------------------------------------------------------------------
 * GPL license:
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * -----------------------------------------------------------------------------
 * A commercial use license is available from Mavimax, Ltd
 * contact@mavimax.com
 * -----------------------------------------------------------------------------
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <regex.h>
#include <utlist.h>

#include <ndebug.h>
#include <atmi.h>
#include <sys_unix.h>
#include <atmi_int.h>
#include <typed_buf.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <Exfields.h>
#include <atmi_shm.h>
#include <exregex.h>
#include "tpcachesv.h"
#include <atmi_cache.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Standard server init
 */
int NDRX_INTEGRA(tpsvrinit)(int argc, char **argv)
{
    int ret=EXSUCCEED;
    char *env;
    long l;
    
    
    /* pull in init.. */
    if (EXSUCCEED!=tpinit(NULL))
    {
        NDRX_LOG(log_error, "Failed to pre-init server");
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_info, "Closing caches (1)...");
    ndrx_cache_uninit();
           
    /* get the env */
    
    env = getenv(CONF_NDRX_SYSFLAGS);
    
    if (NULL!=env)
    {
        l = atol(env);
        
        NDRX_LOG(log_debug, "sysflags=%ld", l);
        
        if (l & NDRX_PRC_SYSFLAGS_FULLSTART)
        {
            NDRX_LOG(log_info, "Fullstart - About to reset caches (if bootreset set)");
            
            
            NDRX_LOG(log_info, "Closing caches open (standard)");
            ndrx_cache_uninit();
            
            NDRX_LOG(log_debug, "init boot mode...");
            /* otherwise on restart we do not need cache handle... */
            if (EXSUCCEED!=ndrx_cache_init(NDRX_TPCACH_INIT_BOOT))
            {
                NDRX_LOG(log_error, "Cache init failed");
                EXFAIL_OUT(ret);
            }
        }
        else
        {
            NDRX_LOG(log_info, "not full startup, no cache reset");
        }
    }

    NDRX_LOG(log_info, "Closing caches (2)...");
    ndrx_cache_uninit();
out:

    return ret;
}

void NDRX_INTEGRA(tpsvrdone) (void)
{
    NDRX_LOG(log_debug, "%s called", __func__);
}
