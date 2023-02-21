/**
 * @brief Test -B flag for server opt - server
 *
 * @file atmisv92.c
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
#include <stdio.h>
#include <stdlib.h>
#include <ndebug.h>
#include <atmi.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <test.fd.h>
#include <string.h>
#include <unistd.h>
#include "test92.h"

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
    UBFH *p_ub = (UBFH *)p_svc->data;
    
out:
    tpreturn(  TPSUCCESS,
                0L,
                (char *)p_ub,
                0L,
                0L);
}

/**
 * Another service, only advertised from buildserver
 */
void TESTSVX (TPSVCINFO *p_svc)
{
    TESTSV(p_svc);
}
/**
 * Do initialisation
 */
int tpsvrinit(int argc, char **argv)
{
    int ret = EXSUCCEED;
    NDRX_LOG(log_debug, "tpsvrinit called");

    if (EXSUCCEED!=tpadvertise("TESTSV", TESTSV))
    {
        NDRX_LOG(log_error, "Failed to initialise TESTSV!");
        EXFAIL_OUT(ret);
    }
out:
    return ret;
}

/**
 * Do de-initialisation
 */
void tpsvrdone(void)
{
    NDRX_LOG(log_debug, "tpsvrdone called");
}


/* Auto generated system advertise table */
expublic struct tmdsptchtbl_t ndrx_G_tmdsptchtbl[] = {
    { "BUILTSVC", "TESTSVX", TESTSVX, 0, 0 }
    , { NULL, NULL, NULL, 0, 0 }
};

/**
 * Server program main entry, multi-threaded support
 * @param argc  argument count
 * @param argv  argument values
 * @return SUCCEED/FAIL
 */
int main( int argc, char** argv )
{
    _tmbuilt_with_thread_option=EXFALSE;
    struct tmsvrargs_t tmsvrargs =
    {
        &tmnull_switch,
        &ndrx_G_tmdsptchtbl[0],
        0,
        tpsvrinit,
        tpsvrdone,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    };

    return( _tmstartserver( argc, argv, &tmsvrargs ));

}

/* vim: set ts=4 sw=4 et smartindent: */
