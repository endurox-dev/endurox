/**
 * @brief Load the xa switch passed to _tmstartserver or return null switch
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
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <atmi.h>
#include <atmi_int.h>
#include <sys_mqueue.h>

#include <dlfcn.h>

#include "atmi_shm.h"

#include <xa.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
typedef  struct xa_switch_t * (*p_func_t)(void);

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
    struct xa_switch_t *ret = NULL;
    
    p_func_t f;
    
    if (NULL==(f = (p_func_t)dlsym( RTLD_DEFAULT, "ndrx_xa_builtin_get" )))
    {
        NDRX_LOG(log_warn, "ndrx_xa_builtin_get symbol not found");
        goto out;
    }
    /* get the switch */
    ret = f();
    
    if (NULL==ret)
    {
        NDRX_LOG(log_error, "Null result!");
        return NULL;
    }
    
    NDRX_LOG(log_debug, "XA Switch [%s] resolved", ret->name);
out:
    
    if (NULL==ret)
    {
        NDRX_LOG(log_info, "Using NULL switch");
        ret = &tmnull_switch;
    }
    
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
