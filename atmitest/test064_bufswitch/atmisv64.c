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

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * NUll request, provides some new buffer back
 * @param p_svc request infos
 */
void NULLREQ (TPSVCINFO *p_svc)
{
    if (NULL!=p_svc->data)
    {
        /* TODO: Test error */
    }
}

/**
 * Provides null response back. No matter of request buffer...
 * @param p_svc request infos
 */
void NULLRSP(TPSVCINFO *p_svc)
{
    
}

/**
 * JSON responder
 * @param p_svc
 */
void JSONRSP(TPSVCINFO *p_svc)
{
    
}

/**
 * String responder
 * @param p_svc
 */
void STRINGRSP(TPSVCINFO *p_svc)
{
    
}

/**
 * Carray responder
 * @param p_svc
 */
void CARRAYRSP(TPSVCINFO *p_svc)
{
    
}

/**
 * VIEW responder
 * @param p_svc
 */
void VIEWRSP(TPSVCINFO *p_svc)
{
    
}

/**
 * UBF Responder
 * @param p_svc
 */
void UBFRSP(TPSVCINFO *p_svc)
{
    
}

#if 0
/**
 * Standard service entry
 */
void TESTSV (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;
    char testbuf[1024];
    UBFH *p_ub = (UBFH *)p_svc->data;

    NDRX_LOG(log_debug, "%s got call", __func__);

    /* Just print the buffer */
    Bprint(p_ub);
    
    if (EXFAIL==Bget(p_ub, T_STRING_FLD, 0, testbuf, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to get T_STRING_FLD: %s", 
                 Bstrerror(Berror));
        ret=EXFAIL;
        goto out;
    }
    
    if (0!=strcmp(testbuf, VALUE_EXPECTED))
    {
        NDRX_LOG(log_error, "TESTERROR: Expected: [%s] got [%s]",
            VALUE_EXPECTED, testbuf);
        ret=EXFAIL;
        goto out;
    }
        
    
out:
    tpreturn(  ret==EXSUCCEED?TPSUCCESS:TPFAIL,
                0L,
                (char *)p_ub,
                0L,
                0L);
}

#endif

/**
 * Do initialisation
 */
int NDRX_INTEGRA(tpsvrinit)(int argc, char **argv)
{
    int ret = EXSUCCEED;
    
    NDRX_LOG(log_debug, "tpsvrinit called");

    if (EXSUCCEED!=tpadvertise("NULLREQ", NULLREQ))
    {
        NDRX_LOG(log_error, "Failed to initialise NULLREQ!");
        EXFAIL_OUT(ret);
    }
    
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

