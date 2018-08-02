/* 
 * @brief Typed CARRAY tests
 *
 * @file atmisv23.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or ATR Baltic's license for commercial use.
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
#include "test023.h"

long M_subs_to_unsibscribe = -1;

/**
 * Receive some carray & reply back with carray, ok?
 */
void TEST23_CARRAY(TPSVCINFO *p_svc)
{
    int ret = EXSUCCEED;
    char *buf = p_svc->data;
    char type[16+1]={EXEOS};
    int i;
    
    if (EXFAIL==tptypes(buf, type, NULL))
    {
        NDRX_LOG(log_error, "TESTERROR: TEST23_CARRAY cannot "
                "determine buffer type");
        EXFAIL_OUT(ret);
    }
    
    if (0!=strcmp(type, "CARRAY"))
    {
        NDRX_LOG(log_error, "TESTERROR: Buffer not CARRAY!");
        EXFAIL_OUT(ret);
    }
    
    for (i=0; i<TEST_REQ_SIZE; i++)
    {
        if (((char)buf[i]) != ((char)i))
        {
            NDRX_LOG(log_error, "TESTERROR: Buffer pos %d not equal to %d!",
                            i, i);
            EXFAIL_OUT(ret);
        }
    }
    
    if (NULL== (buf = tprealloc(buf, TEST_REPLY_SIZE)))
    {
        NDRX_LOG(log_error, "TESTERROR: TEST23_CARRAY failed "
                "to realloc the buffer");
    }
    
    /* fill all the buffer with  */
    for (i=0; i<TEST_REPLY_SIZE; i++)
    {
        buf[i]=i%256;
    }
    
    NDRX_LOG(log_debug, "Sending buffer: [%s]", buf);
    
out:
    
    tpreturn(EXSUCCEED==ret?TPSUCCESS:TPFAIL, 0, buf, TEST_REPLY_SIZE, 0L);
    
}

/**
 * This should match the regex of caller (extra filter test func)
 */
void TEST23 (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;
    
out:

    tpreturn(0, 0, NULL, 0L, 0L);

}

/**
 * This shall not match the regex of caller (extra filter test func)
 */
void TEST23_2(TPSVCINFO *p_svc)
{
    long ret;
    
    NDRX_LOG(log_debug, "TEST23_2 - Called");
    
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

    if (EXSUCCEED!=tpadvertise("TEST23_CARRAY", TEST23_CARRAY))
    {
        NDRX_LOG(log_error, "Failed to initialize TEST23_CARRAY!");
        ret=EXFAIL;
    }
    else if (EXSUCCEED!=tpadvertise("TEST23", TEST23))
    {
        NDRX_LOG(log_error, "Failed to initialize TEST23 (first)!");
        ret=EXFAIL;
    }
    else if (EXSUCCEED!=tpadvertise("TEST23_2", TEST23_2))
    {
        NDRX_LOG(log_error, "Failed to initialize TEST23_2!");
        ret=EXFAIL;
    }
    
    if (EXSUCCEED!=ret)
    {
        goto out;
    }

    strcpy(evctl.name1, "TEST23");
    evctl.flags|=TPEVSERVICE;

    /* Subscribe to event server */
    if (EXFAIL==tpsubscribe("TEST23EV", NULL, &evctl, 0L))
    {
        NDRX_LOG(log_error, "Failed to subscribe TEST23 "
                                        "to EV..TEST event failed");
        ret=EXFAIL;
    }

#if 0
    strcpy(evctl.name1, "TEST23_2");
    /* Subscribe to event server */
    if (EXFAIL==(M_subs_to_unsibscribe=tpsubscribe("TEST23EV", "Hello (.*) Pluto", &evctl, 0L)))
    {
        NDRX_LOG(log_error, "Failed to subscribe TEST23 "
                                        "to EV..TEST event failed");
        ret=EXFAIL;
    }
#endif
    
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

