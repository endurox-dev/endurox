/**
 * @brief TMQ+TMSRV singleton group failovers - server
 *
 * @file atmisv104.c
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
#include "test104.h"

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
void QFWD1 (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;
    char testbuf[1024];
    UBFH *p_ub = (UBFH *)p_svc->data;
    TPQCTL ctl;
    memset(&ctl, 0, sizeof(ctl));

    if (EXSUCCEED!=tpenqueue("TESTSP", "Q2", &ctl, (char *)p_ub, 0, 0))
    {
        NDRX_LOG(log_error, "tpenqueue() to `QFWD2' failed %s diag: %d:%s",
                        tpstrerror(tperrno), ctl.diagnostic, ctl.diagmsg);
                EXFAIL_OUT(ret);
        EXFAIL_OUT(ret);
    }

out:
    tpreturn(  ret==EXSUCCEED?TPSUCCESS:TPFAIL,
                0L,
                (char *)p_ub,
                0L,
                0L);
}

void QFWD2 (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;
    char testbuf[1024];
    UBFH *p_ub = (UBFH *)p_svc->data;
    TPQCTL ctl;
    memset(&ctl, 0, sizeof(ctl));

    if (EXSUCCEED!=tpenqueue("TESTSP", "Q1", &ctl, (char *)p_ub, 0, 0))
    {
        /* DO NOT TRIGGER TESTERROR, as for auto-q might happen the crash */
        NDRX_LOG(log_error, "tpenqueue() to `QFWD2' failed %s diag: %d:%s",
                        tpstrerror(tperrno), ctl.diagnostic, ctl.diagmsg);
                EXFAIL_OUT(ret);
        EXFAIL_OUT(ret);
    }
        
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
int NDRX_INTEGRA(tpsvrinit)(int argc, char **argv)
{
    int ret = EXSUCCEED;
    NDRX_LOG(log_debug, "tpsvrinit called");

    if (EXSUCCEED!=tpadvertise("QFWD1", QFWD1))
    {
        NDRX_LOG(log_error, "Failed to advertise QFWD1!");
        EXFAIL_OUT(ret);
    }

    if (EXSUCCEED!=tpadvertise("QFWD2", QFWD1))
    {
        NDRX_LOG(log_error, "Failed to advertise QFWD2!");
        EXFAIL_OUT(ret);
    }

    if (EXSUCCEED!=tpopen())
    {
        NDRX_LOG(log_error, "Failed to open XA: %s", 
            tpstrerror(tperrno));
        EXFAIL_OUT(ret);   
    }

    /* Check tmsrv without -X */
    if (EXSUCCEED!=tpbegin(30, 0) ||
        EXSUCCEED!=tpcommit(0))
    {
        NDRX_LOG(log_error, "Transaction fail: %s", 
            tpstrerror(tperrno));
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
    NDRX_LOG(log_debug, "tpsvrdone called");
    tpclose();
}

/* vim: set ts=4 sw=4 et smartindent: */
