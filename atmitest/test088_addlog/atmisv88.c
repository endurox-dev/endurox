/**
 * @brief Thread logging and check for stderr/stdout server logrotate - server
 *
 * @file atmisv88.c
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
#include "test88.h"

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
    char testbuf[1024];
    static int test_num = 0;
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

    test_num++;

    NDRX_LOG(log_error, "HELLO WORLD SV");
    fprintf(stderr, "HELLO STDERR %d\n", test_num);
    fprintf(stdout, "HELLO STDOUT %d\n", test_num);
    
    if (0!=strcmp(testbuf, VALUE_EXPECTED))
    {
        NDRX_LOG(log_error, "TESTERROR: Expected: [%s] got [%s]",
            VALUE_EXPECTED, testbuf);
        ret=EXFAIL;
        goto out;
    }

    fflush(stderr);
    fflush(stdout);
        
out:
    tpreturn(  ret==EXSUCCEED?TPSUCCESS:TPFAIL,
                0L,
                (char *)p_ub,
                0L,
                0L);
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
    { "TESTSV", "TESTSV", TESTSV, 0, 0 }
    , { NULL, NULL, NULL, 0, 0 }
};
/*---------------------------Prototypes---------------------------------*/

/**
 * Main entry for tmsrv
 */
int main( int argc, char** argv )
{
    _tmbuilt_with_thread_option=EXTRUE;
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
        tpsvrthrinit,
        tpsvrthrdone
    };
    
    return( _tmstartserver( argc, argv, &tmsvrargs ));
    
}


/* vim: set ts=4 sw=4 et smartindent: */
