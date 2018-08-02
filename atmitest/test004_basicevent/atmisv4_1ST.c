/* 
 *
 * @file atmisv4_1ST.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or Mavimax's license for commercial use.
 * -----------------------------------------------------------------------------
 * GPL license:
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
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

long M_subs_to_unsibscribe = -1;

void TEST4_1ST (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;

    static double d = 55.66;

    UBFH *p_ub = (UBFH *)p_svc->data;

    NDRX_LOG(log_debug, "TEST4_1ST got call");

    /* Just print the buffer */
    Bprint(p_ub);
    
    /* allocate some stuff for more data to put in  */
    if (NULL==(p_ub = (UBFH *)tprealloc((char *)p_ub, 4096)))
    {
        ret=EXFAIL;
        goto out;
    }

    d+=1;

    if (EXFAIL==Badd(p_ub, T_DOUBLE_FLD, (char *)&d, 0))
    {
        ret=EXFAIL;
        goto out;
    }
out:

    tpreturn(0, 0, NULL, 0L, 0L);

}

/**
 * This service does automatically unsubscribe.
 * @param p_svc
 */
void TEST4_1ST_2(TPSVCINFO *p_svc)
{
    long ret;
    
    NDRX_LOG(log_debug, "TEST4_1ST_2 - Called");
    
    if (1!=(ret=tpunsubscribe(M_subs_to_unsibscribe, 0L)))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to unsubscribe, should "
                                "be 1, but got %ld", ret);
    }

    if (3!=(ret=tpunsubscribe(-1, 0L)))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to unsubscribe to all events should "
                                "be 3, but got %ld", ret);
    }

    tpreturn(0, 0, NULL, 0L, 0L);
}

/*
 * Do initialization
 */
int NDRX_INTEGRA(tpsvrinit)(int argc, char **argv)
{
    int ret=EXSUCCEED;
    
    NDRX_LOG(log_debug, "tpsvrinit called");
    TPEVCTL evctl;

    memset(&evctl, 0, sizeof(evctl));

    if (EXSUCCEED!=tpadvertise("TEST4_1ST", TEST4_1ST))
    {
        NDRX_LOG(log_error, "Failed to initialize TEST4_1ST (first)!");
        ret=EXFAIL;
    }
    else if (EXSUCCEED!=tpadvertise("TEST4_1ST_AL", TEST4_1ST))
    {
        NDRX_LOG(log_error, "Failed to initialize TEST4_1ST_AL (alias)!");
        ret=EXFAIL;
    }
    else if (EXSUCCEED!=tpadvertise("TEST4_1ST_2", TEST4_1ST_2))
    {
        NDRX_LOG(log_error, "Failed to initialize TEST4_1ST_2!");
        ret=EXFAIL;
    }
    
    if (EXSUCCEED!=ret)
    {
        goto out;
    }

    NDRX_STRCPY_SAFE(evctl.name1, "TEST4_1ST");
    evctl.flags|=TPEVSERVICE;

    /* Subscribe to event server */
    if (EXFAIL==tpsubscribe("EV..TEST", NULL, &evctl, 0L))
    {
        NDRX_LOG(log_error, "Failed to subscribe TEST4_1ST "
                                        "to EV..TEST event failed");
        ret=EXFAIL;
    }

    NDRX_STRCPY_SAFE(evctl.name1, "TEST4_1ST_2");
    /* Subscribe to event server */
    if (EXFAIL==(M_subs_to_unsibscribe=tpsubscribe("TEST2EV", NULL, &evctl, 0L)))
    {
        NDRX_LOG(log_error, "Failed to subscribe TEST4_1ST "
                                        "to EV..TEST event failed");
        ret=EXFAIL;
    }

    NDRX_STRCPY_SAFE(evctl.name1, "TEST4_1ST_2");
    /* Subscribe to event server */
    if (EXFAIL==(M_subs_to_unsibscribe=tpsubscribe("TEST2EV2", NULL, &evctl, 0L)))
    {
        NDRX_LOG(log_error, "Failed to subscribe TEST4_1ST "
                                        "to EV..TEST event failed");
        ret=EXFAIL;
    }

    NDRX_STRCPY_SAFE(evctl.name1, "TEST4_1ST_2");
    /* Subscribe to event server */
    if (EXFAIL==(M_subs_to_unsibscribe=tpsubscribe("TEST2EV3", NULL, &evctl, 0L)))
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

