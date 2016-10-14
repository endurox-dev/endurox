/* 
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

    if (SUCCEED!=tplogsetreqfile((char **)&p_ub, NULL, NULL))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to set request file! :%s", tpstrerror(tperrno));
        FAIL_OUT(ret);
    }
    
    /* Just print the buffer */
    tplogprintubf(log_debug, "TEST32_1ST request buffer", p_ub);
    
    tplog(log_warn, "Hello from TEST32_1ST!");
    

out:

    if (SUCCEED==ret)
    {
        tplog(log_warn, "Request ok, forwarding to 2ND service");
        tplogclosereqfile();
        tpforward(  "TEST32_2ND",
                    (char *)p_ub,
                    0L,
                    0L);
    }
    else
    {
        tplog(log_warn, "Request FAIL, returning...");
        tplogclosereqfile();
        tpreturn(TPFAIL,
                0L,
                (char *)p_ub,
                0L,
                0L);
    }
}

/**
 * Service for setting request file for new request...
 * @param p_svc
 */
void SETREQFILE(TPSVCINFO *p_svc)
{
    int ret = SUCCEED;
    UBFH *p_ub = (UBFH *)p_svc->data;
    char filename[256];
    static int req_no = 0;
    
    req_no++;
    
    sprintf(filename, "./logs/request_%d.log", req_no);
    
    /* allocate bit space for new file name to be set */
    if (NULL==(p_ub = (UBFH *)tprealloc((char *)p_ub, 4000)))
    {
        NDRX_LOG(log_error, "TESTERROR: realloc failed: %s", 
                filename, tpstrerror(tperrno));
        FAIL_OUT(ret);
    }
    
    if (SUCCEED!=tplogsetreqfile((char **)&p_ub, filename, NULL))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to set request file to [%s]:%s", 
                filename, tpstrerror(tperrno));
        FAIL_OUT(ret);
    }
    
    tplog(log_debug, "Hello from SETREQFILE!");
    
    tplogclosereqfile();
    
    tplog(log_debug, "SETREQFILE closed req file - oapi in main");
    
out:
    tpreturn(  ret==SUCCEED?TPSUCCESS:TPFAIL,
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
    
    /* configure main logger */
    if (SUCCEED!=tplogconfig(LOG_FACILITY_NDRX|LOG_FACILITY_UBF|LOG_FACILITY_TP, 
            FAIL, "file=./1sv.log tp=5 ndrx=5 ubf=0", "1SRV", NULL))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to configure logger!");
    }

    if (SUCCEED!=tpadvertise("TEST32_1ST", TEST32_1ST))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to initialize TEST32_1ST (first)!");
    }
    else if (SUCCEED!=tpadvertise("SETREQFILE", SETREQFILE))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to initialize SETREQFILE!");
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

