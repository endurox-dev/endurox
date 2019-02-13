/**
 * @brief Buffer type switch + null - server
 *
 * @file atmisv64.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * AGPL or Mavimax's license for commercial use.
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
#include "test64.h"
#include <tpadm.h>
#include "t64.h"

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Provides null response back. No matter of request buffer...
 * @param p_svc request infos
 */
void NULLRSP(TPSVCINFO *p_svc)
{
    tpreturn(TPSUCCESS, 0, NULL, 0, 0L);
}

/**
 * JSON responder
 * @param p_svc
 */
void JSONRSP(TPSVCINFO *p_svc)
{
    char *data = NULL;
    int ret = TPSUCCESS;
    
    if (NULL==(data = tpalloc("JSON", NULL, 100)))
    {
        NDRX_LOG(log_error, "TESTERROR ! Failed to alloc JSON data!");
        ret = TPFAIL;
        goto out;
    }
    
    strcpy(data, "{}");
    
out:
    
    tpreturn(ret, 0, data, 0, 0L);
}

/**
 * String responder
 * @param p_svc
 */
void STRINGRSP(TPSVCINFO *p_svc)
{
    char *data = NULL;
    int ret = TPSUCCESS;
    
    if (NULL==(data = tpalloc("STRING", NULL, 100)))
    {
        NDRX_LOG(log_error, "TESTERROR ! Failed to alloc STRING data!");
        ret = TPFAIL;
        goto out;
    }
    
    strcpy(data, "WORLD");
    
out:
    
    tpreturn(ret, 0, data, 0, 0L);
}

/**
 * Carray responder
 * @param p_svc
 */
void CARRAYRSP(TPSVCINFO *p_svc)
{
    char *data = NULL;
    int ret = TPSUCCESS;
    
    if (NULL==(data = tpalloc("CARRAY", NULL, 100)))
    {
        NDRX_LOG(log_error, "TESTERROR ! Failed to alloc CARRAY data!");
        ret = TPFAIL;
        goto out;
    }
    
    strcpy(data, "SPACE");
    
out:
    
    tpreturn(ret, 0, data, strlen(data), 0L);
}

/**
 * VIEW responder
 * @param p_svc
 */
void VIEWRSP(TPSVCINFO *p_svc)
{
    int ret = TPSUCCESS;   
    struct MYVIEW2 *v = NULL;
    
    
    if (NULL==(v = (struct MYVIEW2 *)tpalloc("VIEW", "MYVIEW2", 
            sizeof(struct MYVIEW2))))
    {
        NDRX_LOG(log_error, "TESTERROR ! Failed to alloc MYVIEW2 data!");
        ret = TPFAIL;
        goto out;
    }
    
    memset(&v, 0, sizeof(v));
    
    NDRX_STRCPY_SAFE(v->tstring1, "TEST 55");
    
out:    
    tpreturn(ret, 0, (char *)v, 0, 0L);    
}

/**
 * UBF Responder
 * @param p_svc
 */
void UBFRSP(TPSVCINFO *p_svc)
{
    UBFH *data = NULL;
    int ret = TPSUCCESS;
    
    if (NULL==(data = (UBFH *)tpalloc("UBF", NULL, 1024)))
    {
        NDRX_LOG(log_error, "TESTERROR ! Failed to alloc UBF data!");
        ret = TPFAIL;
        goto out;
    }
    
    if (EXSUCCEED!=Bchg(data, T_STRING_FLD, 1, "HELLO WORLD", 0L))
    {
        NDRX_LOG(log_error, "TESTERROR ! Failed to set T_STRING_FLD: %s", 
                Bstrerror(Berror));
        ret = TPFAIL;
        goto out;
    }
    
out:
    
    tpreturn(ret, 0, (char *)data, 0, 0L);
}

/**
 * Do initialisation
 */
int NDRX_INTEGRA(tpsvrinit)(int argc, char **argv)
{
    int ret = EXSUCCEED;
    
    NDRX_LOG(log_debug, "tpsvrinit called");
    
    if (EXSUCCEED!=tpadvertise("NULLRSP", NULLRSP))
    {
        NDRX_LOG(log_error, "Failed to initialise NULLRSP!");
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=tpadvertise("JSONRSP", JSONRSP))
    {
        NDRX_LOG(log_error, "Failed to initialise JSONRSP!");
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=tpadvertise("STRINGRSP", STRINGRSP))
    {
        NDRX_LOG(log_error, "Failed to initialise STRINGRSP!");
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=tpadvertise("CARRAYRSP", CARRAYRSP))
    {
        NDRX_LOG(log_error, "Failed to initialise CARRAYRSP!");
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=tpadvertise("VIEWRSP", VIEWRSP))
    {
        NDRX_LOG(log_error, "Failed to initialise VIEWRSP!");
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=tpadvertise("UBFRSP", UBFRSP))
    {
        NDRX_LOG(log_error, "Failed to initialise UBFRSP!");
        EXFAIL_OUT(ret);
    }
    
out:    
    return ret;
}

/**
 * Do de-initialisation
 */
void NDRX_INTEGRA(tpsvrdone)(void)
{
    ndrx_growlist_t list;
    NDRX_LOG(log_debug, "tpsvrdone called");
    
    /* count buffer allocated.. shall be 0!!!! */
    
    if (EXSUCCEED!=ndrx_buffer_list(&list))
    {
        NDRX_LOG(log_error, "TESTERROR! Failed to build buffer list!");
    }
    else
    {
        if (EXFAIL < list.maxindexused)
        {
            NDRX_LOG(log_error, "TESTERROR! Max buffer index: %d, shall be -1 (as all free)!",
                    list.maxindexused);
        }
        
        ndrx_growlist_free(&list);   
    }
    
}

/* vim: set ts=4 sw=4 et smartindent: */

