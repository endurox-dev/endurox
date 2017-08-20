/* 
** Typed VIEW tests
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
void TEST40_VIEW(TPSVCINFO *p_svc)
{
    int ret = EXSUCCEED;
    char *buf = p_svc->data;
    char type[16+1]={EXEOS};
    char subtype[XATMI_SUBTYPE_LEN]={EXEOS};
    
    NDRX_LOG(log_debug, "Into: %s", __func__);
    
    if (EXFAIL==tptypes(buf, type, subtype))
    {
        NDRX_LOG(log_error, "TESTERROR: TEST40_VIEW cannot "
                "determine buffer type");
        EXFAIL_OUT(ret);
    }
    
    if (0!=strcmp(type, "VIEW"))
    {
        NDRX_LOG(log_error, "TESTERROR: Buffer not VIEW!");
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_debug, "%s VIEW ok", __func__);
    
    if (0!=strcmp(subtype, "MYVIEW1"))
    {
        NDRX_LOG(log_error, "TESTERROR: sub-type not MYVIEW1, but [%s]", 
                subtype);
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_debug, "%s MYVIEW1 ok", __func__);
        
    NDRX_LOG(log_debug, "%s About to compare data block..", __func__);
    
    NDRX_LOG(log_error, "v1 OK, now BUF")
    if (EXSUCCEED!=validate_MYVIEW1((struct MYVIEW1 *)buf))
    {
        NDRX_LOG(log_error, "Invalid data recovered from FB!");
        EXFAIL_OUT(ret);
    }
    NDRX_LOG(log_error, "BUF OK")
    
    /* Return different block back */
    if (NULL==(buf = tpalloc("VIEW", "MYVIEW3", sizeof(struct MYVIEW3))))
    {
        NDRX_LOG(log_error, "Failed VIEW buffer: %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    /* Setup the values */
    init_MYVIEW3((struct MYVIEW3 *)buf);
    
    
out:
    
    tpreturn(EXSUCCEED==ret?TPSUCCESS:TPFAIL, 0, buf, 0L, 0L);
    
}

/**
 * Do initialization
 */
int NDRX_INTEGRA(tpsvrinit)(int argc, char **argv)
{
    int ret=EXSUCCEED;
    
    NDRX_LOG(log_debug, "tpsvrinit called");


    if (EXSUCCEED!=tpadvertise("TEST40_VIEW", TEST40_VIEW))
    {
        NDRX_LOG(log_error, "Failed to initialize TEST40_VIEW!");
        ret=EXFAIL;
    }

    if (EXSUCCEED!=ret)
    {
        goto out;
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

