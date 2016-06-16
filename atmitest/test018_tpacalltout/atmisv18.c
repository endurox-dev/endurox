/* 
**
** @file atmisv18.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, ATR Baltic, SIA. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or ATR Baltic's license for commercial use.
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
** A commercial use license is available from ATR Baltic, SIA
** contact@atrbaltic.com
** -----------------------------------------------------------------------------
*/

#include <unistd.h>     /* Symbolic Constants */
#include <sys/types.h>  /* Primitive System Data Types */ 
#include <errno.h>      /* Errors */
#include <stdio.h>      /* Input/Output */
#include <stdlib.h>     /* General Utilities */
#include <pthread.h>    /* POSIX Threads */


#include <ndebug.h>
#include <atmi.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <test.fd.h>
#include <string.h>




/* main server thread... 
 * NOTE: p_svc - this is local variable of enduro's main thread (on stack).
 * but p_svc->data - is auto buffer, will be freed when main thread returns.
 *                      Thus we need a copy of buffer for thread.
 */
void TESTSVFN (TPSVCINFO *p_svc)
{
    int ret=SUCCEED;
    UBFH *p_ub = (UBFH *)p_svc->data; /* this is auto-buffer */
   
    /* serve next.. */
    tpfree((char *)p_ub);
    
    NDRX_LOG(log_debug, "Got call + dumping it by continue()");
    
    tpcontinue();
}

/**
 * Just reply OK.
 * @param p_svc
 */
void ECHO(TPSVCINFO *p_svc)
{
    int first=1;

    UBFH *p_ub = (UBFH *)p_svc->data;
    
    sleep(5);
    
    /* Return OK */
    tpreturn (TPSUCCESS, 0L, (char *)p_ub, 0L, 0L);
}


/*
 * Do initialization
 */
int NDRX_INTEGRA(tpsvrinit)(int argc, char **argv)
{
    int ret = SUCCEED;
    NDRX_LOG(log_debug, "tpsvrinit called");

    if (SUCCEED!=tpadvertise("TESTSV", TESTSVFN))
    {
        NDRX_LOG(log_error, "Failed to initialize TESTSV (first)!");
        ret=FAIL;
    }
    else if (SUCCEED!=tpadvertise("ECHO", ECHO))
    {
        NDRX_LOG(log_error, "Failed to initialize ECHO!");
        ret=FAIL;
    }
    
    return ret;
}

/**
 * Do de-initialization
 */
void NDRX_INTEGRA(tpsvrdone)(void)
{
    NDRX_LOG(log_debug, "tpsvrdone called");
}
