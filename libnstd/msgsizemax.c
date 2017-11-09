/* 
** Max message size determination functions
**
** @file msgsizemax.c
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
#include <ndrx_config.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <nstdutil.h>
#include <sys_unix.h>
#include <userlog.h>
#include <cconfig.h>
#include <sys/resource.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
exprivate int M_maxmsgsize_loaded = EXFALSE; /* Is config loaded? */
exprivate long M_maxmsgsize = EXFAIL; /* Max message size */
exprivate long M_stack_estim = EXFAIL; /* Estimated stack size */
MUTEX_LOCKDECL(M_maxmsgsize_loaded_lock);
/*---------------------------Prototypes---------------------------------*/

/**
 * Test the stack and have a function name which will tell
 * what to user do.
 */
exprivate int ndrx_please_increase_stack(void)
{
    volatile char buf[M_stack_estim];
    
    buf[M_stack_estim-1] = EXSUCCEED;
}

/**
 * Return configured max message size.
 * This will try to get value from environment, if not possible then NDRX_ATMI_MSG_MAX_SIZE
 * is used.
 * TODO: Have some sync here... no need to execute twice..
 * @return max message size
 */
expublic long ndrx_msgsizemax (void)
{
    char *esize;
    struct rlimit rl;
    
    if (!M_maxmsgsize_loaded)
    {
        MUTEX_LOCK_V(M_maxmsgsize_loaded_lock);
        
        if (!M_maxmsgsize_loaded)
        {
            /* this is thread safe function
             * it will not be a problem if this block is twice executed.
             */
            ndrx_cconfig_load();

            esize = getenv(CONF_NDRX_MSGSIZEMAX);

            if (NULL!=esize)
            {
                M_maxmsgsize = atol(esize);

                if (M_maxmsgsize < NDRX_ATMI_MSG_MAX_SIZE)
                {
                    M_maxmsgsize = NDRX_ATMI_MSG_MAX_SIZE;
                }
            }
            else
            {
                M_maxmsgsize = NDRX_ATMI_MSG_MAX_SIZE;
            }

            /* Estimate stack */
            M_stack_estim = M_maxmsgsize * NDRX_STACK_MSG_FACTOR;

            /* Check rlimit */
            if (EXSUCCEED==getrlimit(RLIMIT_STACK, &rl))
            {
                if (RLIM_INFINITY!=rl.rlim_cur && rl.rlim_cur < M_stack_estim)
                {
                    rl.rlim_cur = M_stack_estim;

                    /* Ignore the result, because it is assumed that 
                     * we cannot perform any logging  */
                    if (EXSUCCEED!=setrlimit(RLIMIT_STACK, &rl))
                    {
                        userlog("setrlimit(RLIMIT_STACK, ...) rlim_cur=%ld failed: %s",
                                    (long)M_stack_estim, strerror(errno));

                        userlog("LIMITS ERROR ! Please set stack (ulimit -s) size "
                                    "to: %ld bytes or %ld kb (calculated by: "
                                    "NDRX_MSGSIZEMAX(%ld)*NDRX_STACK_MSG_FACTOR(%d))\n", 
                                    (long)M_stack_estim, (long)(M_stack_estim/1024), 
                                    (long)M_maxmsgsize, NDRX_STACK_MSG_FACTOR);

                        fprintf(stderr, "LIMITS ERROR ! Please set stack (ulimit -s) size "
                                    "to: %ld bytes or %ld kb (calculated by: "
                                    "NDRX_MSGSIZEMAX(%ld)*NDRX_STACK_MSG_FACTOR(%d))\n", 
                                    (long)M_stack_estim, (long)(M_stack_estim/1024), 
                                    (long)M_maxmsgsize, NDRX_STACK_MSG_FACTOR);

                        fprintf(stderr, "Process is terminating with error...\n");

                        exit(EXFAIL);
                    }
                }
            }
            else
            {
                userlog("getrlimit(RLIMIT_STACK, ...) failed: %s", strerror(errno));
            }

            /*test the stack */
            ndrx_please_increase_stack();

            M_maxmsgsize_loaded = EXTRUE;
        }
        MUTEX_UNLOCK_V(M_maxmsgsize_loaded_lock);
    }
    
    return M_maxmsgsize;
}

