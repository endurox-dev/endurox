/**
 * @brief test big messages over 64kb - server
 *
 * @file atmisv41.c
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
#include "test41.h"

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Standard service entry
 */
void TESTSV (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;
    char bufferreq[TEST_MSGSIZE];
    int i;
    BFLDLEN retlen;
    UBFH *p_ub = (UBFH *)p_svc->data;

    NDRX_LOG(log_debug, "%s got call", __func__);
    
     /* test the response... */
    retlen = TEST_MSGSIZE;
    if (EXFAIL==Bget(p_ub, T_CARRAY_FLD, 0, bufferreq, &retlen))
    {
        NDRX_LOG(log_debug, "Failed to get T_CARRAY_FLD[0]: %s", Bstrerror(Berror));
        ret=EXFAIL;
        goto out;
    }

    if (retlen != TEST_MSGSIZE)
    {
        NDRX_LOG(log_error, "Invalid message size received, expected: %d, got: %d", 
                (int)retlen, (int)TEST_MSGSIZE);
        ret=EXFAIL;
        goto out;
    }

    for (i=0; i<TEST_MSGSIZE; i++)
    {
        char c = (char) ((i+2) & 0xff);
        if (bufferreq[i] != c)
        {
            NDRX_LOG(log_error, "TESTERROR and index %d: expected %x but got %x",
                        i, (int)bufferreq[i], (int)c);
            ret=EXFAIL;
            goto out;
        }
    }
    
    for (i=0; i<TEST_MSGSIZE; i++)
    {
        bufferreq[i] = (char) ((i+3) & 0xff);
    }

    if (EXFAIL==Bchg(p_ub, T_CARRAY_FLD, 0, bufferreq, TEST_MSGSIZE))
    {
        NDRX_LOG(log_debug, "Failed to set T_CARRAY_FLD[0]: %s", Bstrerror(Berror));
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

/*
 * Do initialization
 */
int NDRX_INTEGRA(tpsvrinit)(int argc, char **argv)
{
    NDRX_LOG(log_debug, "tpsvrinit called");

    if (EXSUCCEED!=tpadvertise("TESTSV", TESTSV))
    {
        NDRX_LOG(log_error, "Failed to initialize TESTSV!");
    }
    
    return EXSUCCEED;
}

/**
 * Do de-initialization
 */
void NDRX_INTEGRA(tpsvrdone)(void)
{
    NDRX_LOG(log_debug, "tpsvrdone called");
}

/* vim: set ts=4 sw=4 et smartindent: */
