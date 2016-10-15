/* 
** Test server
**
** @file atmisv32FIRST.c
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

#include <stdio.h>
#include <stdlib.h>
#include <ndebug.h>
#include <atmi.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <test.fd.h>
#include <ndebug.h>

void TEST32_1ST (TPSVCINFO *p_svc)
{
    int ret=SUCCEED;
    UBFH *p_ub = (UBFH *)p_svc->data;
    
    /* TODO: Get the value from buffer, increment it
     * and push back to buffer. Do this by O API.
     */
    
out:

    tpreturn(TPFAIL,
            0L,
            (char *)p_ub,
            0L,
            0L);

}

/**
 * Do initialization
 */
int NDRX_INTEGRA(tpsvrinit)(int argc, char **argv)
{
    NDRX_LOG(log_debug, "tpsvrinit called");
    
    if (SUCCEED!=tpadvertise("TEST32_1ST", TEST32_1ST))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to initialize TEST32_1ST (first)!");
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

