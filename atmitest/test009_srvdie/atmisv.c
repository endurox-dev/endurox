/**
 *
 * @file atmisv.c
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

/* Stuff for sockets testing: */
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <ndebug.h>
#include <atmi.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <test.fd.h>

/*
 * This is test service!
 */
void TESTSVFN (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;

    char *str = "THIS IS TEST - OK!";

    UBFH *p_ub = (UBFH *)p_svc->data;

    NDRX_LOG(log_debug, "TESTSVFN got call");
    
    sleep(2); /* <<< So that clients enqueue theyr calls! */
    exit (1); /* We have crashed! */

out:
    tpreturn(  ret==EXSUCCEED?TPSUCCESS:TPFAIL,
                0L,
                (char *)p_ub,
                0L,
                0L);
}

/**
 * Perform exit of server after few seconds..
 * @param p_svc
 */
void EXITSVC(TPSVCINFO *p_svc)
{
    
    sleep(4);
    
    /* lets die */
    exit(1);
}

/*
 * Do initialization
 */
int NDRX_INTEGRA(tpsvrinit)(int argc, char **argv)
{
    int ret = EXSUCCEED;
    NDRX_LOG(log_debug, "tpsvrinit called");
        
    if (EXSUCCEED!=tpadvertise("TESTSVFN", TESTSVFN))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to initialize TESTSV (first)!");
        EXFAIL_OUT(ret);
    }
    else if (EXSUCCEED!=tpadvertise("EXITSVC", EXITSVC))
    {
        NDRX_LOG(log_error, "Failed to initialize EXITSVC!");
        EXFAIL_OUT(ret);
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
