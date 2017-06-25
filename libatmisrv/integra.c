/* 
 * Integration library with different platforms.
** Currently provides linked entry points of tpsvrinit() and tpsvrdone(), 
** but does call the registered callbacks (currently needed for golang linking)
** 
** @file integra.c
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
#include <stdio.h>
#include <stdlib.h>
#include <ndrstandard.h>
#include <ndebug.h>
#include <utlist.h>
#include <string.h>
#include <unistd.h>

#include "srv_int.h"
#include <atmi_int.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
int (*G_tpsvrinit__)(int, char **) = NULL;
void (*G_tpsvrdone__)(void) = NULL;
/* No jump please (default for integra) */
expublic long G_libatmisrv_flags     =   ATMI_SRVLIB_NOLONGJUMP; 
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Do initialization
 */
int tpsvrinit(int argc, char **argv)
{
    int ret = EXSUCCEED;
    NDRX_LOG(log_debug, "tpsvrinit() called");
    if (NULL!=G_tpsvrinit__)
    {
        if (EXSUCCEED!=(ret = G_tpsvrinit__(argc, argv)))
        {
            NDRX_LOG(log_error, "G_tpsvrinit__() failed");
            goto out;
        }
        else
        {
            NDRX_LOG(log_debug, "G_tpsvrinit__() ok");
        }
    }
    else
    {
        NDRX_LOG(log_error, "G_tpsvrinit__ == NULL => FAIL!");
        EXFAIL_OUT(ret);
    }
    
out:
    return ret;
}

/**
 * Do de-initialization
 */
void tpsvrdone(void)
{
    NDRX_LOG(log_debug, "tpsvrdone() called");
    
    if (NULL!=G_tpsvrdone__)
    {
        G_tpsvrdone__();
    }
    else
    {
        NDRX_LOG(log_warn, "G_tpsvrdone__ null, not calling");
    }
}


/**
 * Forward the call to NDRX
 */
int ndrx_main_integra(int argc, char** argv, int (*in_tpsvrinit)(int, char **), 
            void (*in_tpsvrdone)(void), long flags) 
{

    G_tpsvrinit__ =  in_tpsvrinit;
    G_tpsvrdone__ =  in_tpsvrdone;
    G_libatmisrv_flags = flags;

    return ndrx_main(argc, argv);
}

