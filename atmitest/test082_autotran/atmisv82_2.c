/**
 * @brief Test auto-transaction functionality - server
 *
 * @file atmisv82.c
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
#include "test82.h"

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
void TESTSV2 (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;
    UBFH *p_ub = (UBFH *)p_svc->data;
    char testbuf[1024];
    TPQCTL qc;

    NDRX_LOG(log_debug, "%s got call", __func__);
    
    if (EXFAIL==Bget(p_ub, T_STRING_FLD, 0, testbuf, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to get T_STRING_FLD: %s", 
                 Bstrerror(Berror));
        ret=EXFAIL;
        goto out;
    }
    
    /* enqueue the data buffer */
    memset(&qc, 0, sizeof(qc));
    
    /* add msg 1 */
    if (EXSUCCEED!=tpenqueue("MYSPACE", "MSGQ", &qc, (char *)p_ub, 0, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                tpstrerror(tperrno), qc.diagnostic, qc.diagmsg);
        EXFAIL_OUT(ret);
    }
    
    if (0==strcmp(testbuf, "FAIL"))
    {
        ret=EXFAIL;
    }
    else if (0==strcmp(testbuf, "SLEEP"))
    {
        sleep(7);
    }
    
    /* perform dynamic advertise, so that next cases OK works too */
    
    if (EXSUCCEED!=tpunadvertise("TESTSV2"))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to unadvertise: %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=tpadvertise("TESTSV2", TESTSV2))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to advertise: %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
out:
    
    tpreturn(  EXSUCCEED==ret?TPSUCCESS:TPFAIL,
                0L,
                (char *)p_ub,
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
    
    if (EXSUCCEED!=tpopen())
    {
        NDRX_LOG(log_error, "Failed to tpopen: %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }

    if (EXSUCCEED!=tpadvertise("TESTSV2", TESTSV2))
    {
        NDRX_LOG(log_error, "Failed to initialise TESTSV2!");
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
    tpclose();
    
    NDRX_LOG(log_debug, "tpsvrdone called");
}

/* vim: set ts=4 sw=4 et smartindent: */
