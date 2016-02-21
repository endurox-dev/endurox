/* 
** Typed JSON testing
**
** @file atmiclt26.c
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>
#include <test.fd.h>
#include <ndrstandard.h>
#include "test026.h"

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/*
 * Do the test call to the server
 */
int main(int argc, char** argv) {

    long rsplen;
    int i;
    int ret=SUCCEED;
    long l;
    char tmp[128];
    
    NDRX_LOG(log_info, "Testing UBF -> JSON auto conv...");
    for (i=0; i<10000; i++)
    {
        UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, 1024);

        if (NULL==p_ub)
        {
            NDRX_LOG(log_error, "TESTERROR: failed to alloc UBF buffer!");
            FAIL_OUT(ret);
        }

        if (SUCCEED!=Bchg(p_ub, T_STRING_FLD, 0, "HELLO FROM UBF", 0L))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to set T_STRING_FLD");
            ret=FAIL;
            goto out;
        }

        if (FAIL==tpcall("TEST26_UBF2JSON", (char *)p_ub, 0L, (char **)&p_ub, &rsplen, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: TEST26_UBF2JSON failed: %s", 
                    tpstrerror(tperrno));
            ret=FAIL;
            goto out;
        }

        NDRX_LOG(log_info, "Got response:");
        Bprint(p_ub);

        if (SUCCEED!=Bget(p_ub, T_STRING_FLD, 1, tmp, 0L))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to set T_STRING_FLD");
            ret=FAIL;
            goto out;
        }

        if (0!=strcmp(tmp, "HELLO FROM JSON!"))
        {
            NDRX_LOG(log_error, "TESTERROR: Invalid response [%s]!", tmp);
            ret=FAIL;
            goto out;
        }

        if (SUCCEED!=Bget(p_ub, T_LONG_FLD, 0, (char *)&l, 0L))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to set T_STRING_FLD");
            ret=FAIL;
            goto out;
        }

        if (1001!=l)
        {
            NDRX_LOG(log_error, "TESTERROR: Invalid response [%ld]!", l);
            ret=FAIL;
            goto out;
        }

        tpfree((char *)p_ub);
    }
    
    NDRX_LOG(log_info, "Testing JSON -> UBF auto conv...");
    for (i=0; i<10000; i++)
    {
        char *json = tpalloc("JSON", NULL, 1024);

        if (NULL==json)
        {
            NDRX_LOG(log_error, "TESTERROR: failed to alloc JSON buffer!");
            FAIL_OUT(ret);
        }
        
        strcpy(json, "{\"T_STRING_FLD\":[\"HELLO UBF FROM JSON 1!\", \"HELLO UBF FROM JSON 2!\"]}");

        if (FAIL==tpcall("TEST26_JSON2UBF", (char *)json, 0L, (char **)&json, &rsplen, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: TEST26_UBF2JSON failed: %s", 
                    tpstrerror(tperrno));
            ret=FAIL;
            goto out;
        }

        NDRX_LOG(log_info, "Got response: [%s]", json);
        
        if (0!=strcmp(json, "{\"T_STRING_FLD\":[\"HELLO UBF FROM JSON 1!\",\"HELLO UBF FROM JSON 2!\",\"HELLO FROM UBF!\"]}"))
        {
            NDRX_LOG(log_error, "TESTERROR: Invalid response [%s]", json);
            ret=FAIL;
            goto out;
        }
        tpfree((char *)json);
    }
    
    NDRX_LOG(log_info, "Testing invalid json request - shall got SVCERR!");
    for (i=0; i<10000; i++)
    {
        char *json = tpalloc("JSON", NULL, 1024);

        if (NULL==json)
        {
            NDRX_LOG(log_error, "TESTERROR: failed to alloc JSON buffer!");
            FAIL_OUT(ret);
        }
        
        strcpy(json, "Some ][ very bad json{");

        if (SUCCEED==tpcall("TEST26_JSON2UBF", (char *)json, 0L, (char **)&json, &rsplen, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: TEST26_UBF2JSON must FAIL!");
            ret=FAIL;
            goto out;
        }
        
        if (tperrno!=TPESVCERR)
        {
            NDRX_LOG(log_error, "TESTERROR: Invalid error code [%d]", tperrno);
            ret=FAIL;
            goto out;
        }

        tpfree((char *)json);
    }
    
out:

    if (ret>=0)
        ret=SUCCEED;

    return ret;
}

