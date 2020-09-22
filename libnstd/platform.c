/**
 * @brief System platform related utilities
 *
 * @file platform.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2019, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * AGPL (with Java and Go exceptions) or Mavimax's license for commercial use.
 * See LICENSE file for full text.
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
#include <ndrxdiag.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
exprivate long M_stack_size = EXFAIL;    /* Current stack size */

exprivate MUTEX_LOCKDECL(M_stack_size_lock);

/*---------------------------Prototypes---------------------------------*/

/**
 * Diagnostic for pthread_create
 * @param file source file
 * @param line source line
 * @param NDRX_DIAG_* code
 * @param msg module message
 * @param err errno after the pthread_create() failed.
 */
expublic void ndrx_platf_diag(char *file, long line, int code, int err, char *msg)
{
    switch (code)
    {
        case NDRX_DIAG_PTHREAD_CREATE:
            
            NDRX_LOG(log_always, "Failed to pthread_create() for %s (%d): %s, at %s:%ld", 
                    msg, errno, strerror(errno), line, file);
            userlog("Failed to pthread_create() for %s (%d): %s, at %s:%ld", 
                    msg, errno, strerror(errno), line, file);

            if (EAGAIN==err || EINVAL==err)
            {
#ifdef EX_OS_AIX
                NDRX_LOG(log_always, "Check thread specific resource "
                        "settings e.g. NDRX_THREADSTACKSIZE. For AIX ulimit -s "
                        "is setting global stack limit to all threads! Do limit "
                        "with NDRX_THREADSTACKSIZE", code);
                userlog("Check thread specific resource "
                        "settings e.g. NDRX_THREADSTACKSIZE. For AIX ulimit -s "
                        "is setting global stack limit to all threads! Do limit "
                        "with NDRX_THREADSTACKSIZE", code);
#else
                NDRX_LOG(log_always, "Check thread specific resource "
                        "settings e.g. NDRX_THREADSTACKSIZE", code);
                userlog("Check thread specific resource settings "
                        "e.g. NDRX_THREADSTACKSIZE", code);
#endif
            }
        break;
    }
}

/**
 * Return stack size configured for system
 * @return EXFAIL or stack size
 */
expublic long ndrx_platf_stack_get_size(void)
{
    struct rlimit limit;
    char *p;
    
    if (EXFAIL==M_stack_size)
    {
        /* lock */
        MUTEX_LOCK_V(M_stack_size_lock);
        
        if (EXFAIL==M_stack_size)
        {
            if (NULL!=(p=getenv(CONF_NDRX_THREADSTACKSIZE)))
            {
                M_stack_size=atoi(p)*1024;
            }
            
            /* if it was set to 0, or EXFAIL */
            if (M_stack_size <= 0)
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
            
            if (M_stack_size<0)
            {
                M_stack_size = NDRX_STACK_MAX;
                NDRX_LOG(log_warn, "Unlimited stack, setting to %ld bytes",
                        M_stack_size);
            }
        }
        MUTEX_UNLOCK_V(M_stack_size_lock);
    }
   
    return M_stack_size;
}

/**
 * Configure the stack size in the loop
 * seems like AIX RLIMITS might say one value but accepts another
 * Thus try in loop to reduce the stack until it is accepted
 * @param pthread_custom_attr ptr to pthread_attr_t attr to set
 */
expublic void ndrx_platf_stack_set(void *pthread_custom_attr)
{
    long ssize = ndrx_platf_stack_get_size();
    int ret;
    pthread_attr_t *pattr = (pthread_attr_t *)pthread_custom_attr;
    
    while (EXSUCCEED!=(ret = pthread_attr_setstacksize(pattr, ssize)) 
            && EINVAL==ret
            && ssize > 0)
    {
        ssize /= 2;
    }
    
    if (0==ssize)
    {
        userlog("Error ! failed to set stack value!");
    }
    
}

/* vim: set ts=4 sw=4 et smartindent: */
