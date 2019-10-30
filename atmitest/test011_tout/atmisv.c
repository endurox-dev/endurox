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
    static int counter = 0;
    char buf[100];
    UBFH *p_ub = (UBFH *)p_svc->data;
    static char last_buf[100] = {EXEOS};
    
    counter++;
    
    if (EXSUCCEED!=Bget(p_ub, T_STRING_FLD, 0, buf, 0L))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to get T_STRING_FLD!!");
        ret=EXFAIL;
        goto out;
    }
    /* Reset counter on non-tout */
    if (0!=strcmp(buf, last_buf))
    {
        counter = 1;
        strcpy(last_buf, buf);
    }

    NDRX_LOG(log_debug, "TESTSVFN %s @%d@", buf, counter);
    
    sleep(6);

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
        
    if (EXSUCCEED!=tpadvertise("TESTSVFN", TESTSVFN))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to initialize TESTSV (first)!");
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
