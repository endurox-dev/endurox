/* 
** System platform related utilities
**
** @file platform.c
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
