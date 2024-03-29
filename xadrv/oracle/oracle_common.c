/**
 * @brief Load oracle drivers - static version
 *
 * @file oracle_common.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2023, Mavimax, Ltd. All Rights Reserved.
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

#include "atmi_shm.h"

#include <xa.h>
#include <dlfcn.h>
#include "oracle_common.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define NDRX_ORA_TPGETCONN_MAGIC                0x1fca8e4c /**< magic   */
#define NDRX_ORA_TPGETCONN_VERSION              1   /**< have xaoSvcCtx */
#define NDRX_ORA_XAOSVCCTX_SYMBOL "xaoSvcCtx"
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/**
 * tpgetconn() struct values
 */
typedef struct
{
    unsigned int magic; /**< Magic number                               */
    long version;       /**< record version                             */
    void *xaoSvcCtx;    /**< xaoSvcCtx handle,                          */
    
} ndrx_ora_tpgetconn_t;

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
exprivate ndrx_ora_tpgetconn_t M_details;
/*---------------------------Prototypes---------------------------------*/

/**
 * Get connection detail
 * @return ptr to ndrx_ora_tpgetconn_t;
 */
void *xa_getconn_detail(void)
{
    return (void *)&M_details;
}

/*
 * The function is exported and dynamically retrieved after the module was
 * fetched
 */
struct xa_switch_t *ndrx_get_xa_switch_int(char *symbol, char *descr)
{
    struct xa_switch_t * sw = NULL;
    void *handle = NULL;
    int ret = EXSUCCEED;
    NDRX_LOG(log_debug, "%s", descr);
    
    memset(&M_details, 0, sizeof(M_details));
    
    M_details.magic = NDRX_ORA_TPGETCONN_MAGIC;
    M_details.version = NDRX_ORA_TPGETCONN_VERSION;
            
    sw = (struct xa_switch_t * )dlsym( RTLD_DEFAULT, symbol );
    if( sw == NULL )
    {
        NDRX_LOG(log_debug, "%s symbol not found in "
                "process address space - loading .so!", symbol);
        
        /* Loading the symbol... */
        handle = dlopen (G_atmi_env.xa_rmlib, RTLD_NOW);
        if (!handle)
        {
            NDRX_LOG(log_error, "Failed to load XA Resource Manager lib [%s]: %s", 
                G_atmi_env.xa_rmlib, dlerror());
            EXFAIL_OUT(ret);
        }
        
        /* resolve symbol now... */
        if (NULL==(sw = (struct xa_switch_t * )dlsym( handle, symbol )))
        {
            NDRX_LOG(log_error, "Oracle XA switch `%s' handler "
                    "not found!", symbol);
            EXFAIL_OUT(ret);
        }
        
        M_details.xaoSvcCtx = dlsym( handle, NDRX_ORA_XAOSVCCTX_SYMBOL );
        
    }
    else
    {
        /* try to load */
        M_details.xaoSvcCtx = dlsym( RTLD_DEFAULT, NDRX_ORA_XAOSVCCTX_SYMBOL );
    }
    
    NDRX_LOG(log_debug, "xaoSvcCtx=%p", M_details.xaoSvcCtx);
    
    /* set callback handle */
    ndrx_xa_setgetconnn(xa_getconn_detail);
    
out:
    if (EXSUCCEED!=ret && NULL!=handle)
    {
        /* close the handle */
        dlclose(handle);
    }

    
    return sw;
}

/* vim: set ts=4 sw=4 et smartindent: */
