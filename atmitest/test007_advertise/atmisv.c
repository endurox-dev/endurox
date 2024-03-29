/**
 *
 * @file atmisv.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2023, Mavimax, Ltd. All Rights Reserved.
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


void TESTSVFN (TPSVCINFO *p_svc);

/**
 * Advertise the service.
 * @param p_svc
 */
void DOADV(TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;
    UBFH *p_ub = (UBFH *)p_svc->data;
	
    /* Advertise test service */
    ret=tpadvertise("TESTSVFN", TESTSVFN);

    tpreturn(  ret==EXSUCCEED?TPSUCCESS:TPFAIL,
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
    int ret=EXSUCCEED;
    UBFH *p_ub = (UBFH *)p_svc->data;
	
    
    /* Unadvertise test service */
    ret=tpunadvertise("TESTSVFN");

    tpreturn(  ret==EXSUCCEED?TPSUCCESS:TPFAIL,
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
    int ret=EXSUCCEED;

    char *str = "THIS IS TEST - OK!";

    UBFH *p_ub = (UBFH *)p_svc->data;

    NDRX_LOG(log_debug, "TESTSVFN got call");

    /* Just print the buffer */
    Bprint(p_ub);

    if (NULL==(p_ub = (UBFH *)tprealloc((char *)p_ub, 4096))) /* allocate some stuff for more data to put in  */
    {
        ret=EXFAIL;
        goto out;
    }

    if (EXFAIL==Bchg(p_ub, T_STRING_FLD, 0, (char *)str, 0))
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
    int ret = EXSUCCEED;
    NDRX_LOG(log_debug, "tpsvrinit called");

    if (EXSUCCEED!=tpadvertise("DOADV", DOADV))
    {
        NDRX_LOG(log_error, "Failed to initialize DOADV!");
        EXFAIL_OUT(ret);
    }
    
    /* OK continue as normal */
    
    if (EXSUCCEED!=tpadvertise("UNADV", UNADV))
    {
        NDRX_LOG(log_error, "Failed to initialize UNADV (first)!");
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_debug, "Checking advertise exceptions:");
    
    
        /* Do it twice..., same func all OK */
    if (EXSUCCEED!=tpadvertise("DOADV", DOADV))
    {
        NDRX_LOG(log_error, "Failed to initialize DOADV!");
        EXFAIL_OUT(ret);
    }
    
    /* Do it twice..., ptrs different -> match error */
    if (EXSUCCEED==tpadvertise("DOADV", UNADV))
    {
        NDRX_LOG(log_error, "TESTERROR: re-advertise different func must fail!");
        EXFAIL_OUT(ret);
    }
    
    if (TPEMATCH!=tperrno)
    {
        NDRX_LOG(log_error, "TESTERROR: Expected %d (TPEMATCH) but got %d err!",
                TPEMATCH, tperrno);
        EXFAIL_OUT(ret);
    }
    
    /* try empty */
    if (EXSUCCEED==tpadvertise("", UNADV))
    {
        NDRX_LOG(log_error, "TESTERROR: Empty advertise not allowed");
        EXFAIL_OUT(ret);
    }
    
    if (TPEINVAL!=tperrno)
    {
        NDRX_LOG(log_error, "TESTERROR: Expected %d (TPEINVAL) but got %d err!",
                TPEINVAL, tperrno);
        EXFAIL_OUT(ret);
    }
    
    /* try NULL */
    if (EXSUCCEED==tpadvertise(NULL, UNADV))
    {
        NDRX_LOG(log_error, "TESTERROR: NULL advertise not allowed");
        EXFAIL_OUT(ret);
    }
    
    if (TPEINVAL!=tperrno)
    {
        NDRX_LOG(log_error, "TESTERROR: Expected %d (TPEINVAL) but got %d err!",
                TPEINVAL, tperrno);
        EXFAIL_OUT(ret);
    }
    
    /* try too long name  */
    if (EXSUCCEED==tpadvertise("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAB", UNADV))
    {
        NDRX_LOG(log_error, "TESTERROR: Must fail with too long svcnm");
        EXFAIL_OUT(ret);
    }
    
    if (TPEINVAL!=tperrno)
    {
        NDRX_LOG(log_error, "TESTERROR: Expected %d (TPEINVAL) but got %d err!",
                TPEINVAL, tperrno);
        EXFAIL_OUT(ret);
    }
    
    
    /* try long name */
    if (EXSUCCEED!=tpadvertise("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", UNADV))
    {
        NDRX_LOG(log_error, "TESTERROR: Long name (30 symb) shall be OK!");
        EXFAIL_OUT(ret);
    }

    /* try NULL func  */
    if (EXSUCCEED==tpadvertise("AAAAAAAAAA", NULL))
    {
        NDRX_LOG(log_error, "TESTERROR: Null func adv shall fail");
        EXFAIL_OUT(ret);
    }
    
    if (TPEINVAL!=tperrno)
    {
        NDRX_LOG(log_error, "TESTERROR: Expected %d (TPEINVAL) but got %d err!",
                TPEINVAL, tperrno);
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
