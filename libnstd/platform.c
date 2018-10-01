/**
 * @brief System platform related utilities
 *
 * @file platform.c
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

/*---------------------------Includes-----------------------------------*/
#include <ndrstandard.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <ctype.h>
#include <pthread.h>
#include <nstd_tls.h>

#include "nstdutil.h"
#include "ndebug.h"
#include "userlog.h"
#include <errno.h>
#include <sys/resource.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
exprivate long M_stack_size = EXFAIL;    /* Current stack size */

MUTEX_LOCKDECL(M_stack_size_lock);

/*---------------------------Prototypes---------------------------------*/

/**
 * Return stack size configured for system
 * @return EXFAIL or stack size
 */
expublic long ndrx_platf_stack_get_size(void)
{
    struct rlimit limit;
    
    if (EXFAIL==M_stack_size)
    {
        /* lock */
        MUTEX_LOCK_V(M_stack_size_lock);
        
        if (EXFAIL==M_stack_size)
        {
            if (EXSUCCEED!=getrlimit (RLIMIT_STACK, &limit))
            {
                int err = errno;
                NDRX_LOG(log_error, "Failed to get stack size: %s", strerror(err));
                userlog("Failed to get stack size: %s", strerror(err));
            }
            else
            {
                M_stack_size=limit.rlim_cur;
                
                NDRX_LOG(log_info, "Current stack size: %ld, max: %ld", 
                        M_stack_size,  (long)limit.rlim_max);
            }
        }
        MUTEX_UNLOCK_V(M_stack_size_lock);
    }
   
    return M_stack_size;
}
/* vim: set ts=4 sw=4 et smartindent: */
