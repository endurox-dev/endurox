/* 
** JSON buffer support (Recieve converted buffer)
**
** @file atmisv26.c
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
#include "test026.h"

/**
 * Receive some json & reply back with json, ok?
 */
void TEST26_UBF2JSON(TPSVCINFO *p_svc)
{
    int ret = SUCCEED;
    char *buf = p_svc->data;
    char type[16+1]={EOS};
    
    if (FAIL==tptypes(buf, type, NULL))
    {
        NDRX_LOG(log_error, "TESTERROR: TEST26_UBF2JSON cannot "
                "determine buffer type");
        FAIL_OUT(ret);
    }
    
    if (0!=strcmp(type, "JSON"))
    {
        NDRX_LOG(log_error, "TESTERROR: Buffer not JSON, but [%s]!", type);
        FAIL_OUT(ret);
    }
    
    if (0!=strcmp(buf, "{\"T_STRING_FLD\":\"HELLO FROM UBF\"}"))
    {
        NDRX_LOG(log_error, "TESTERROR: Invalid request!");
        FAIL_OUT(ret);
    }
    
    if (NULL== (buf = tprealloc(buf, 1024)))
    {
        NDRX_LOG(log_error, "TESTERROR: realloc fail!");
        FAIL_OUT(ret);
    }
    
    strcpy(buf, "{\"T_STRING_FLD\":[\"HELLO FROM UBF\", \"HELLO FROM JSON!\"], "
            "\"T_LONG_FLD\":1001}");
    
    NDRX_LOG(log_debug, "Sending buffer: [%s]", buf);
    
out:
    
    tpreturn(SUCCEED==ret?TPSUCCESS:TPFAIL, 0, buf, 0L, 0L);
    
}

/**
 * Test auto convert from JSON-to-UBF!
 */
void TEST26_JSON2UBF(TPSVCINFO *p_svc)
{
    int ret = SUCCEED;
    UBFH *buf = (UBFH *)p_svc->data;
    char tmp[1024];
    char type[16+1]={EOS};
    
    if (FAIL==tptypes((char *)buf, type, NULL))
    {
        NDRX_LOG(log_error, "TESTERROR: TEST26_JSON2UBF cannot "
                "determine buffer type");
        FAIL_OUT(ret);
    }
    
    if (0!=strcmp(type, "UBF"))
    {
        NDRX_LOG(log_error, "TESTERROR: Buffer not UBF buf: %s!", type);
        FAIL_OUT(ret);
    }
    
    if (SUCCEED!=Bget(buf, T_STRING_FLD, 0, tmp, 0L))
    {
        NDRX_LOG(log_error, "TESTERROR: T_STRING_FLD[0] not set!");
        FAIL_OUT(ret);
    }
    
    if (0!=strcmp(tmp, "HELLO UBF FROM JSON 1!"))
    {
        NDRX_LOG(log_error, "TESTERROR: Invalid request value [%s] in T_STRING_FLD[0]",
                tmp);
        FAIL_OUT(ret);
    }
    
    if (SUCCEED!=Bget(buf, T_STRING_FLD, 1, tmp, 0L))
    {
        NDRX_LOG(log_error, "TESTERROR: T_STRING_FLD[1] not set!");
        FAIL_OUT(ret);
    }
    
    if (0!=strcmp(tmp, "HELLO UBF FROM JSON 2!"))
    {
        NDRX_LOG(log_error, "TESTERROR: Invalid request value [%s] in T_STRING_FLD[1]",
                tmp);
        FAIL_OUT(ret);
    }
    
    if (SUCCEED!=Bchg(buf, T_STRING_FLD, 2, "HELLO FROM UBF!", 0L))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to set T_LONG_FLD!");
        FAIL_OUT(ret);
    }
    
out:
    
    tpreturn(SUCCEED==ret?TPSUCCESS:TPFAIL, 0, (char *)buf, 0L, 0L);
    
}

/*
 * Do initialization
 */
int NDRX_INTEGRA(tpsvrinit)(int argc, char **argv)
{
    int ret=SUCCEED;
    
    NDRX_LOG(log_debug, "tpsvrinit called");

    if (SUCCEED!=tpadvertise("TEST26_UBF2JSON", TEST26_UBF2JSON))
    {
        NDRX_LOG(log_error, "Failed to initialize TEST26_UBF2JSON!");
        ret=FAIL;
    }
    
    if (SUCCEED!=tpadvertise("TEST26_JSON2UBF", TEST26_JSON2UBF))
    {
        NDRX_LOG(log_error, "Failed to initialize TEST26_JSON2UBF!");
        ret=FAIL;
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

