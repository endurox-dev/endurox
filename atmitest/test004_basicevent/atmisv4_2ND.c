/**
 *
 * @file atmisv4_2ND.c
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

void TEST4_2ND (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;

    static double d = 11.66;

    UBFH *p_ub = (UBFH *)p_svc->data;

    NDRX_LOG(log_debug, "TESTSVFN got call");

    /* Just print the buffer */
    Bprint(p_ub);
    if (NULL==(p_ub = (UBFH *)tprealloc((char *)p_ub, 4096))) /* allocate some stuff for more data to put in  */
    {
        ret=EXFAIL;
        goto out;
    }

    d+=1;

    if (EXFAIL==Badd(p_ub, T_DOUBLE_2_FLD, (char *)&d, 0))
    {
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
    int ret=EXSUCCEED;
    TPEVCTL evctl;

    NDRX_LOG(log_debug, "tpsvrinit called");
    memset(&evctl, 0, sizeof(evctl));

    if (EXSUCCEED!=tpadvertise("TEST4_2ND", TEST4_2ND))
    {
        NDRX_LOG(log_error, "Failed to initialize TEST2_2ND (first)!");
        ret=EXFAIL;
        goto out;
    }
    else if (EXSUCCEED!=tpadvertise("TEST4_2ND_AL", TEST4_2ND))
    {
        NDRX_LOG(log_error, "Failed to initialize TEST4_2ND_AL (alias)!");
        ret=EXFAIL;
        goto out;
    }


    NDRX_STRCPY_SAFE(evctl.name1, "TEST4_2ND");
    evctl.flags|=TPEVSERVICE;
    /* Subscribe to event server */
    if (EXFAIL==tpsubscribe("EV..TEST", "1==1 && T_DOUBLE_FLD==5", &evctl, 0L))
    {
        NDRX_LOG(log_error, "Failed to subscribe TEST4_1ST "
                                        "to EV..TEST event failed");
        ret=EXFAIL;
    }
    
out:
    return ret;
}

/**
 * Do de-initialization
 */
void NDRX_INTEGRA(tpsvrdone)(void)
{
    NDRX_LOG(log_debug, "tpsvrdone called");
}

/* vim: set ts=4 sw=4 et smartindent: */
