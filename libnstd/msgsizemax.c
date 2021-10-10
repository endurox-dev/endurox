/**
 * @brief Max message size determination functions
 *
 * @file msgsizemax.c
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
#include <ndrx_config.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
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
exprivate int volatile M_maxmsgsize_loaded = EXFALSE; /**< Is config loaded? */
exprivate long volatile M_maxmsgsize = EXFAIL; /**< Max message size */
exprivate MUTEX_LOCKDECL(M_maxmsgsize_loaded_lock);
/*---------------------------Prototypes---------------------------------*/

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
    
    if (NDRX_UNLIKELY(!M_maxmsgsize_loaded))
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

            /* round the max msg size to modulus of 16, to be aligned. */
            
            M_maxmsgsize = M_maxmsgsize + 16 - M_maxmsgsize % 16;
            /* avoid write re-ordering ! */
            __sync_synchronize();
            M_maxmsgsize_loaded=EXTRUE;
        }
        MUTEX_UNLOCK_V(M_maxmsgsize_loaded_lock);
    }
    
    return M_maxmsgsize;
}

/* vim: set ts=4 sw=4 et smartindent: */
