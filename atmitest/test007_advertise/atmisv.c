/* 
**
** @file atmisv.c
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


void TESTSVFN (TPSVCINFO *p_svc);

/**
 * Advertise the service.
 * @param p_svc
 */
void DOADV(TPSVCINFO *p_svc)
{
    int ret=SUCCEED;
    UBFH *p_ub = (UBFH *)p_svc->data;
	
    /* Advertise test service */
    ret=tpadvertise("TESTSVFN", TESTSVFN);

    tpreturn(  ret==SUCCEED?TPSUCCESS:TPFAIL,
                0L,
                (char *)p_ub,
                0L,
                0L);
}

/**
 * Undvertise the service!
 * @param p_svc
 */
void UNADV (TPSVCINFO *p_svc)
{
    int ret=SUCCEED;
    UBFH *p_ub = (UBFH *)p_svc->data;
	
    
    /* Unadvertise test service */
    ret=tpunadvertise("TESTSVFN");

    tpreturn(  ret==SUCCEED?TPSUCCESS:TPFAIL,
                0L,
                (char *)p_ub,
                0L,
                0L);
}

/*
 * This is test service!
 */
void TESTSVFN (TPSVCINFO *p_svc)
{
    int ret=SUCCEED;

    char *str = "THIS IS TEST - OK!";

    UBFH *p_ub = (UBFH *)p_svc->data;

    NDRX_LOG(log_debug, "TESTSVFN got call");

    /* Just print the buffer */
    Bprint(p_ub);
    if (NULL==(p_ub = (UBFH *)tprealloc((char *)p_ub, 4096))) /* allocate some stuff for more data to put in  */
    {
        ret=FAIL;
        goto out;
    }

    if (FAIL==Bchg(p_ub, T_STRING_FLD, 0, (char *)str, 0))
    {
        ret=FAIL;
        goto out;
    }

out:
    tpreturn(  ret==SUCCEED?TPSUCCESS:TPFAIL,
                0L,
                (char *)p_ub,
                0L,
                0L);
}

/*
 * Do initialization
 */
int tpsvrinit(int argc, char **argv)
{
    int ret = SUCCEED;
    NDRX_LOG(log_debug, "tpsvrinit called");

    if (SUCCEED!=tpadvertise("DOADV", DOADV))
    {
        NDRX_LOG(log_error, "Failed to initialize DOADV!");
        ret=FAIL;
    }
    else if (SUCCEED!=tpadvertise("UNADV", UNADV))
    {
        NDRX_LOG(log_error, "Failed to initialize UNADV (first)!");
        ret=FAIL;
    }
    
    return ret;
}

/**
 * Do de-initialization
 */
void tpsvrdone(void)
{
    NDRX_LOG(log_debug, "tpsvrdone called");
}
