/* 
** Second server, will use debug defaults..
**
** @file atmisv31SECOND.c
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

void TEST31_2ND (TPSVCINFO *p_svc)
{
    int ret=SUCCEED;
    UBFH *p_ub = (UBFH *)p_svc->data;

    tplogprintubf(log_debug, "TEST31_2ND got request", p_ub);
    
    /* Just print the buffer */
    tplogsetreqfile((char **)&p_ub, NULL, NULL);
    
    tplog(log_debug, "Hello from TEST31_2ND!");
    
    
out:
    tplogclosereqfile();
    tplog(log_warn, "Returning... (logging from main)");
    tpreturn(  ret==SUCCEED?TPSUCCESS:TPFAIL,
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

    if (SUCCEED!=tpadvertise("TEST31_2ND", TEST31_2ND))
    {
        NDRX_LOG(log_error, "Failed to initialize TEST31_2ND (first)!");
    }

    return SUCCEED;
}

/**
 * Do de-initialization
 */
void NDRX_INTEGRA(tpsvrdone)(void)
{
    NDRX_LOG(log_debug, "tpsvrdone called");
}

