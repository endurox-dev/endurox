/* 
**
** @file atmisv21.c
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
#include <atmi_int.h>


extern int __write_to_tx_file(char *buf);


void NOTRANFAIL(TPSVCINFO *p_svc)
{
    UBFH *p_ub = (UBFH *)p_svc->data;

    if (tpgetlev())
    {
        NDRX_LOG(log_error, "TESTERROR: SHOULD NOT BE IN TRNASACTION!");
    }
    
    tpreturn (TPFAIL, 0L, (char *)p_ub, 0L, 0L);
}

void RUNTXFAIL(TPSVCINFO *p_svc)
{
    int first=1;
    UBFH *p_ub = (UBFH *)p_svc->data;
    char buf[1024];

    p_ub = (UBFH *)tprealloc ((char *)p_ub, Bsizeof (p_ub) + 4096);

    if (SUCCEED!=Bget(p_ub, T_STRING_FLD, 0, buf, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to get T_STRING_FLD: %s",
                                          Bstrerror(Berror));
        tpreturn (TPFAIL, 0L, NULL, 0L, 0L);
    }
    
    if (SUCCEED!=__write_to_tx_file(buf))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to call __write_to_tx_file()");
        tpreturn (TPFAIL, 0L, NULL, 0L, 0L);
    }
    
    /* Return FAIL */
    tpreturn (TPFAIL, 0L, (char *)p_ub, 0L, 0L);
}

void RUNTX(TPSVCINFO *p_svc)
{
    int first=1;
    UBFH *p_ub = (UBFH *)p_svc->data;
    char buf[1024];

    p_ub = (UBFH *)tprealloc ((char *)p_ub, Bsizeof (p_ub) + 4096);

    if (SUCCEED!=Bget(p_ub, T_STRING_FLD, 0, buf, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to get T_STRING_FLD: %s",
                                          Bstrerror(Berror));
        tpreturn (TPFAIL, 0L, NULL, 0L, 0L);
    }
    
    if (SUCCEED!=__write_to_tx_file(buf))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to call __write_to_tx_file()");
        tpreturn (TPFAIL, 0L, NULL, 0L, 0L);
    }
    
    /* Return OK */
    tpreturn (TPSUCCESS, 0L, (char *)p_ub, 0L, 0L);
}

/**
 * Do initialization
 */
int NDRX_INTEGRA(tpsvrinit)(int argc, char **argv)
{
    char svcnm[16];
    int i;
    int ret = SUCCEED;
    NDRX_LOG(log_debug, "tpsvrinit called");
    
    
    if (SUCCEED!=tpopen())
    {
        NDRX_LOG(log_error, "TESTERROR: tpopen() fail: %d:[%s]", 
                                            tperrno, tpstrerror(tperrno));
        ret=FAIL;
        goto out;
    }

    if (SUCCEED!=tpadvertise("RUNTX", RUNTX))
    {
        NDRX_LOG(log_error, "Failed to initialize RUNTX!");
    }
    
    if (SUCCEED!=tpadvertise("RUNTXFAIL", RUNTXFAIL))
    {
        NDRX_LOG(log_error, "Failed to initialize RUNTXFAIL!");
    }

    if (SUCCEED!=tpadvertise("NOTRANFAIL", NOTRANFAIL))
    {
        NDRX_LOG(log_error, "Failed to initialize NOTRANFAIL!");
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
    if (SUCCEED!=tpclose())
    {
        NDRX_LOG(log_error, "TESTERROR: tpclose() fail: %d:[%s]", 
                                            tperrno, tpstrerror(tperrno));
    }
}

