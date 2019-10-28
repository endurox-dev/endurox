/**
 * @brief Test memory limit restarts - server
 *
 * @file atmisv62.c
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
#include <stdio.h>
#include <stdlib.h>
#include <ndebug.h>
#include <atmi.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <test.fd.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "test62.h"

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Take some 50MB of RSS. This actually makes a use of memory..
 */
void RSSALLOC (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;
    static char *buf = NULL;
    int i, chk=0;
    
    if (NULL==buf)
    {
        buf = NDRX_MALLOC(RSS_BLOCK);
        
        if (NULL==buf)
        {
            NDRX_LOG(log_error, "TESTERROR ! Failed to alloc %d bytes: %s", 
                    RSS_BLOCK, strerror(errno));
            EXFAIL_OUT(ret);
        }
    }
    
    /* store some values there and trick out the kernel not to
     * figure out our strategy */
    for (i=0; i<RSS_BLOCK; i++)
    {
        buf[i] = i/3;
    }
    
    for (i=0; i<RSS_BLOCK; i++)
    {
        chk+=(char)buf[i];
    }
    
    /* avoid optimizations ... */
    printf("chk %d\n", chk);

out:
    tpreturn(  ret==EXSUCCEED?TPSUCCESS:TPFAIL,
                0L,
                (char *)p_svc->data,
                0L,
                0L);
}

/**
 * Take some 1GB of Virtual memory, just malloc, no other actions...
 */
void VSZALLOC (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;
    static char *buf = NULL;
    
    if (NULL==buf)
    {
        buf = NDRX_MALLOC(VSZ_BLOCK);
        
        if (NULL==buf)
        {
            NDRX_LOG(log_error, "TESTERROR ! Failed to alloc %d bytes: %s", 
                    VSZ_BLOCK, strerror(errno));
            EXFAIL_OUT(ret);
        }
    }
    
out:
    tpreturn(  ret==EXSUCCEED?TPSUCCESS:TPFAIL,
                0L,
                (char *)p_svc->data,
                0L,
                0L);
}

/**
 * Do initialisation
 */
int NDRX_INTEGRA(tpsvrinit)(int argc, char **argv)
{
    int ret = EXSUCCEED;
    NDRX_LOG(log_debug, "tpsvrinit called");

    if (EXSUCCEED!=tpadvertise("RSSALLOC", RSSALLOC))
    {
        NDRX_LOG(log_error, "Failed to initialise RSSALLOC!");
        ret = EXFAIL;
    }
    else if (EXSUCCEED!=tpadvertise("VSZALLOC", VSZALLOC))
    {
        NDRX_LOG(log_error, "Failed to initialise VSZALLOC!");
        ret = EXFAIL;
    }
    
    return EXSUCCEED;
}

/**
 * Do de-initialisation
 */
void NDRX_INTEGRA(tpsvrdone)(void)
{
    NDRX_LOG(log_debug, "tpsvrdone called");
}

/* vim: set ts=4 sw=4 et smartindent: */
