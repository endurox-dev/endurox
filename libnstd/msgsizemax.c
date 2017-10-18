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

#include <ndrstandard.h>
#include <ndebug.h>
#include <nstdutil.h>
#include <sys_unix.h>
#include <userlog.h>
#include <cconfig.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
exprivate int M_maxmsgsize_loaded = EXFALSE; /* Is config loaded? */
exprivate long M_maxmsgsize = EXFAIL; /* Max message size */
/*---------------------------Prototypes---------------------------------*/

/**
 * Return configured max message size.
 * This will try to get value from environment, if not possible then NDRX_ATMI_MSG_MAX_SIZE
 * is used.
 * @return max message size
 */
expublic long ndrx_msgsizemax (void)
{
    char *esize;
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
        
        M_maxmsgsize_loaded = EXTRUE;
    }
    
    return M_maxmsgsize;
}

