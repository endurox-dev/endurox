/* 
** Typed STRING tests
**
** @file atmisv40.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, Mavimax, Ltd. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or Mavimax's license for commercial use.
** -----------------------------------------------------------------------------
** GPL license:
** 
** This program is free software; you can redistribute it and/or modify it under
** the terms of the GNU General Public License as published by the Free Software
** Foundation; either version 2 of the License, or (at your option) any later
** version.
**
** This program is distributed in the hope that it will be useful, but WITHOUT ANY
** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
** PARTICULAR PURPOSE. See the GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License along with
** this program; if not, write to the Free Software Foundation, Inc., 59 Temple
** Place, Suite 330, Boston, MA 02111-1307 USA
**
** -----------------------------------------------------------------------------
** A commercial use license is available from Mavimax, Ltd
** contact@mavimax.com
** -----------------------------------------------------------------------------
*/

#include <stdio.h>
#include <stdlib.h>
#include <ndebug.h>
#include <atmi.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <test.fd.h>
#include <string.h>
#include "test040.h"

long M_subs_to_unsibscribe = -1;

/**
 * Receive some string & reply back with string, ok?
 */
void TEST40_STRING(TPSVCINFO *p_svc)
{
    int ret = EXSUCCEED;
    char *buf = p_svc->data;
    char type[16+1]={EXEOS};
    int i;
    
    if (EXFAIL==tptypes(buf, type, NULL))
    {
        NDRX_LOG(log_error, "TESTERROR: TEST40_STRING cannot "
                "determine buffer type");
        EXFAIL_OUT(ret);
    }
    
    if (0!=strcmp(type, "STRING"))
    {
        NDRX_LOG(log_error, "TESTERROR: Buffer not STRING!");
        EXFAIL_OUT(ret);
    }
    
    if (0!=strcmp(buf, "HELLO WORLD"))
    {
        NDRX_LOG(log_error, "TESTERROR: Incoming string not \"HELLO WORLD\"");
        EXFAIL_OUT(ret);
    }
    
    if (NULL== (buf = tprealloc(buf, TEST_REPLY_SIZE+1)))
    {
        NDRX_LOG(log_error, "TESTERROR: TEST40_STRING failed "
                "to realloc the buffer");
    }
    
    /* fill all the buffer with  */
    for (i=0; i<TEST_REPLY_SIZE; i++)
    {
        buf[i]=i%255+1;
    }
    
    buf[TEST_REPLY_SIZE] = EXEOS;
    
    NDRX_LOG(log_debug, "Sending buffer: [%s]", buf);
    
out:
    
    tpreturn(EXSUCCEED==ret?TPSUCCESS:TPFAIL, 0, buf, 0L, 0L);
    
}

/**
 * This should match the regex of caller (extra filter test func)
 */
void TEST40 (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;
    
out:

    tpreturn(0, 0, NULL, 0L, 0L);

}

/**
 * This shall not match the regex of caller (extra filter test func)
 */
void TEST40_2(TPSVCINFO *p_svc)
{
    long ret;
    
    NDRX_LOG(log_debug, "TEST40_2 - Called");
    
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

    if (EXSUCCEED!=tpadvertise("TEST40_STRING", TEST40_STRING))
    {
        NDRX_LOG(log_error, "Failed to initialize TEST40_STRING!");
        ret=EXFAIL;
    }
    else if (EXSUCCEED!=tpadvertise("TEST40", TEST40))
    {
        NDRX_LOG(log_error, "Failed to initialize TEST40 (first)!");
        ret=EXFAIL;
    }
    else if (EXSUCCEED!=tpadvertise("TEST40_2", TEST40_2))
    {
        NDRX_LOG(log_error, "Failed to initialize TEST40_2!");
        ret=EXFAIL;
    }
    
    if (EXSUCCEED!=ret)
    {
        goto out;
    }

    strcpy(evctl.name1, "TEST40");
    evctl.flags|=TPEVSERVICE;

    /* Subscribe to event server */
    if (EXFAIL==tpsubscribe("TEST40EV", "Hello (.*) Mars", &evctl, 0L))
    {
        NDRX_LOG(log_error, "Failed to subscribe TEST40 "
                                        "to TEST40EV event failed");
        ret=EXFAIL;
    }

    strcpy(evctl.name1, "TEST40_2");
    /* Subscribe to event server */
    if (EXFAIL==(M_subs_to_unsibscribe=tpsubscribe("TEST40EV", "Hello (.*) Pluto", &evctl, 0L)))
    {
        NDRX_LOG(log_error, "Failed to subscribe TEST40 "
                                        "to TEST40EV event failed");
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

