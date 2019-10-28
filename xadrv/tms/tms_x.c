/**
 * @brief Load the xa switch passed to _tmstartserver
 *
 * @file tms_x.c
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <atmi.h>
#include <atmi_int.h>
#include <sys_mqueue.h>

#define __USE_GNU
#include <dlfcn.h>

#include "atmi_shm.h"

#include <xa.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * API entry of loading the driver
 * @param symbol
 * @param descr
 * @return XA switch or null
 */
struct xa_switch_t *ndrx_get_xa_switch(void)
{
    /* lookup ndrx_G_tmsvrargs */
    
    struct tmsvrargs_t * sargs;
    
    if (NULL==(sargs = (struct tmsvrargs_t * )dlsym( RTLD_DEFAULT, "ndrx_G_tmsvrargs" )))
    {
        NDRX_LOG(log_error, "ndrx_G_tmsvrargs symbol not found or is NULL, check "
                "that you use libatmisrvinteg.so and that server is started with _tmstartserver(): %s", 
                dlerror());
        return NULL;
    }
    
    if (NULL==sargs->xa_switch)
    {
        NDRX_LOG(log_error, "ERROR! For tmsvrargs_t xa_switch is empty, "
                "passed to _tmstartserver()!");
        return NULL;
    }
    
    return sargs->xa_switch;
}

/* vim: set ts=4 sw=4 et smartindent: */
