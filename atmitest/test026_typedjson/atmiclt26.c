/**
 * @brief Typed JSON testing
 *
 * @file atmiclt26.c
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
#include "ubfutil.h"

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * test case for #189 - fails to parse json
 * @return EXSUCCEED/EXFAIL
 */
int test_tpjsontoubf(void)
{
    int ret = EXSUCCEED;
    
    /* Bug #189 */
    char *data = "{\n"
                  "        \"T_STRING_FLD\":\"L\"\n"
                  "}";
    
    UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, 1024);
    
    if (NULL==p_ub)
    {
        NDRX_LOG(log_error, "TESTERROR! Failed to allocate p_ub: %s", 
                tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_info, "Testing Bug #189 tpjsontoubf() - failes to parse JSON");
    
    if (EXSUCCEED!=tpjsontoubf(p_ub, data))
    {
        NDRX_LOG(log_error, "TESTERROR! Failed to parse [%s]: %s", 
                data, tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    ndrx_debug_dump_UBF(log_debug, "parsed ubf buffer", p_ub);
    
    
out:
    tpfree((char *)p_ub);
    return ret;
}

/**
 * test case for Bug #796, tpubftojson() damages the log files
*/
int test_tpjsontoubf_long(void)
{
    int ret = EXSUCCEED;
    long lmax = LONG_MAX;
    long lmin = LONG_MIN;

#ifdef SYS64BIT
    long lmax2 = 109832567849012365;
#endif
    char tmp[1024];

    UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, 1024);
    UBFH *p_ub2 = (UBFH *)tpalloc("UBF", NULL, 1024);
    
    if (NULL==p_ub || NULL==p_ub2)
    {
        NDRX_LOG(log_error, "TESTERROR! Failed to allocate p_ub or p_ub2: %s", 
                tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }

    if (EXSUCCEED!=Bchg(p_ub, T_LONG_FLD, 0, (char *)&lmax, 0L))
    {
        NDRX_LOG(log_error, "TESTERROR! Failed to set T_LONG_FLD: %s", 
                Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }

    if (EXSUCCEED!=Bchg(p_ub, T_LONG_2_FLD, 0, (char *)&lmin, 0L))
    {
        NDRX_LOG(log_error, "TESTERROR! Failed to set T_LONG_2_FLD: %s", 
                Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }

#ifdef SYS64BIT
    if (EXSUCCEED!=Bchg(p_ub, T_LONG_3_FLD, 0, (char *)&lmax2, 0L))
    {
        NDRX_LOG(log_error, "TESTERROR! Failed to set T_LONG_3_FLD: %s", 
                Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    /* test array */
    if (EXSUCCEED!=Bchg(p_ub, T_LONG_3_FLD, 1, (char *)&lmax2, 0L))
    {
        NDRX_LOG(log_error, "TESTERROR! Failed to set T_LONG_3_FLD[1]: %s",
                Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
#endif

    if (EXSUCCEED!=Bchg(p_ub, T_CHAR_FLD, 0, (char *)"B", 0L))
    {
        NDRX_LOG(log_error, "TESTERROR! Failed to set T_CHAR_FLD: %s",
                Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }

    if (EXSUCCEED!=tpubftojson(p_ub, tmp, sizeof(tmp)))
    {
        NDRX_LOG(log_error, "TESTERROR! Failed to convert to json: %s", 
                tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }

    NDRX_LOG(log_debug, "Got json: [%s]", tmp);

    if (EXSUCCEED!=tpjsontoubf(p_ub2, tmp))
    {
        NDRX_LOG(log_error, "TESTERROR! Failed to parse [%s]: %s", 
                tmp, tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }

    if (EXSUCCEED!=Bcmp(p_ub, p_ub2))
    {
        NDRX_LOG(log_error, "TESTERROR! Values are not equal!");
        
        ndrx_debug_dump_UBF(log_debug, "Org buffer", p_ub);
        ndrx_debug_dump_UBF(log_debug, "Parsed buffer", p_ub2);
        EXFAIL_OUT(ret);
    }
    
out:
    if (NULL!=p_ub)
    {
        tpfree((char *)p_ub);
    }

    if (NULL!=p_ub2)
    {
        tpfree((char *)p_ub2);
    }
    
    return ret;
}
/*
 * Do the test call to the server
 */
int main(int argc, char** argv) {

    long rsplen;
    int i;
    int ret=EXSUCCEED;
    long l;
    char tmp[128];
    
    if (EXSUCCEED!=test_tpjsontoubf())
    {
        EXFAIL_OUT(ret);
    }

    if (EXSUCCEED!=test_tpjsontoubf_long())
    {
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_info, "Testing UBF -> JSON auto conv...");
    for (i=0; i<10000; i++)
    {
        UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, 1024);
        UBFH *p_ub2=NULL;

        if (NULL==p_ub)
        {
            NDRX_LOG(log_error, "TESTERROR: failed to alloc UBF buffer!");
            EXFAIL_OUT(ret);
        }

        p_ub2 = (UBFH *)tpalloc("UBF", NULL, 1024);
        if (NULL==p_ub)
        {
            NDRX_LOG(log_error, "TESTERROR: failed to alloc UBF buffer!");
            EXFAIL_OUT(ret);
        }

        if (EXSUCCEED!=Bchg(p_ub, T_STRING_FLD, 0, "HELLO FROM UBF", 0L))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to set T_STRING_FLD");
            ret=EXFAIL;
            goto out;
        }

        if (EXSUCCEED!=Bchg(p_ub2, T_STRING_FLD, 0, "HELLO FROM INNER", 0L))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to set T_STRING_FLD");
            ret=EXFAIL;
            goto out;
        }

        if (EXSUCCEED!=Bchg(p_ub, T_PTR_FLD, 0, (char *)&p_ub2, 0L))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to set T_PTR_FLD");
            ret=EXFAIL;
            goto out;
        }

        if (EXFAIL==tpcall("TEST26_UBF2JSON", (char *)p_ub, 0L, (char **)&p_ub, &rsplen, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: TEST26_UBF2JSON failed: %s", 
                    tpstrerror(tperrno));
            ret=EXFAIL;
            goto out;
        }

        NDRX_LOG(log_info, "Got response:");
        Bprint(p_ub);

        if (EXSUCCEED!=Bget(p_ub, T_STRING_FLD, 1, tmp, 0L))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to set T_STRING_FLD");
            ret=EXFAIL;
            goto out;
        }

        if (0!=strcmp(tmp, "HELLO FROM JSON!"))
        {
            NDRX_LOG(log_error, "TESTERROR: Invalid response [%s]!", tmp);
            ret=EXFAIL;
            goto out;
        }

        if (EXSUCCEED!=Bget(p_ub, T_PTR_FLD, 0, (char *)&p_ub2, 0L))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to get T_PTR_FLD");
            ret=EXFAIL;
            goto out;
        }

        if (EXSUCCEED!=Bget(p_ub2, T_STRING_FLD, 0, tmp, 0L))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to get T_STRING_FLD");
            ret=EXFAIL;
            goto out;
        }

        if (0!=strcmp(tmp, "HELLO FROM INNER"))
        {
            NDRX_LOG(log_error, "TESTERROR: Invalid response [%s]!", tmp);
            ret=EXFAIL;
            goto out;
        }

        if (EXSUCCEED!=Bget(p_ub, T_LONG_FLD, 0, (char *)&l, 0L))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to set T_STRING_FLD");
            ret=EXFAIL;
            goto out;
        }

        if (1001!=l)
        {
            NDRX_LOG(log_error, "TESTERROR: Invalid response [%ld]!", l);
            ret=EXFAIL;
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
            EXFAIL_OUT(ret);
        }
        
        strcpy(json, "{\"T_STRING_FLD\":[\"HELLO UBF FROM JSON 1!\", \"HELLO UBF FROM JSON 2!\"]}");

        if (EXFAIL==tpcall("TEST26_JSON2UBF", (char *)json, 0L, (char **)&json, &rsplen, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: TEST26_UBF2JSON failed: %s", 
                    tpstrerror(tperrno));
            ret=EXFAIL;
            goto out;
        }

        NDRX_LOG(log_info, "Got response: [%s]", json);
        
        if (0!=strcmp(json, "{\"T_STRING_FLD\":[\"HELLO UBF FROM JSON 1!\",\"HELLO UBF FROM JSON 2!\",\"HELLO FROM UBF!\"]}"))
        {
            NDRX_LOG(log_error, "TESTERROR: Invalid response [%s]", json);
            ret=EXFAIL;
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
            EXFAIL_OUT(ret);
        }
        
        strcpy(json, "Some ][ very bad json{");

        if (EXSUCCEED==tpcall("TEST26_JSON2UBF", (char *)json, 0L, (char **)&json, &rsplen, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: TEST26_UBF2JSON must FAIL!");
            ret=EXFAIL;
            goto out;
        }
        
        if (tperrno!=TPESVCERR)
        {
            NDRX_LOG(log_error, "TESTERROR: Invalid error code [%d]", tperrno);
            ret=EXFAIL;
            goto out;
        }

        tpfree((char *)json);
    }
    
    
    
out:

    if (ret>=0)
        ret=EXSUCCEED;

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
