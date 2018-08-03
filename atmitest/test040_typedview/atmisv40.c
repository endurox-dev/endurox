/**
 * @brief Typed VIEW tests
 *
 * @file atmisv40.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or Mavimax's license for commercial use.
 * -----------------------------------------------------------------------------
 * GPL license:
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
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
 * Test View to JSON convert
 * @param p_svc
 */
void TEST40_V2JSON(TPSVCINFO *p_svc)
{
    int ret = EXSUCCEED;
    char *buf = p_svc->data;
    char type[16+1]={EXEOS};
    char subtype[XATMI_SUBTYPE_LEN]={EXEOS};
    char *input = "{\"MYVIEW3\":{\"tshort1\":1,\"tshort2\":2,\"tshort3\":3}}";
    char *output = "{\"MYVIEW3\":{\"tshort1\":4,\"tshort2\":5,\"tshort3\":6}}";
    
    NDRX_LOG(log_debug, "Into: %s", __func__);
    
    if (EXFAIL==tptypes(buf, type, subtype))
    {
        NDRX_LOG(log_error, "TESTERROR: TEST40_VIEW cannot "
                "determine buffer type");
        EXFAIL_OUT(ret);
    }
    
    if (0!=strcmp(type, "JSON"))
    {
        NDRX_LOG(log_error, "TESTERROR: Buffer not JSON!");
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_debug, "%s JSON received ok: [%s]", __func__, buf);
    
    /* compare.. */
    if (0!=strcmp(input, buf))
    {
        NDRX_LOG(log_error, "TESTERROR: Input of V2J not matched, expected: "
                "[%s] got [%s]", input, buf);
        EXFAIL_OUT(ret);
    }
    
    /* size not changed... */
    strcpy(buf, output);
    
out:
    
    tpreturn(EXSUCCEED==ret?TPSUCCESS:TPFAIL, 0, buf, 0L, 0L);
}


/**
 * Process view recieved from json buffer
 */
void TEST40_JSON2V(TPSVCINFO *p_svc)
{
    int ret = EXSUCCEED;
    char *buf = p_svc->data;
    char type[16+1]={EXEOS};
    char subtype[XATMI_SUBTYPE_LEN]={EXEOS};
    struct MYVIEW3 *v3 = (struct MYVIEW3 *)buf;
    
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
    
    if (0!=strcmp(subtype, "MYVIEW3"))
    {
        NDRX_LOG(log_error, "TESTERROR: sub-type not MYVIEW3, but [%s]", 
                subtype);
        EXFAIL_OUT(ret);
    }
         
    NDRX_LOG(log_debug, "%s About to compare data block..", __func__);

    if (6!=v3->tshort1)
    {
        NDRX_LOG(log_error, "TESTERROR: tshort1 must be 6 but must be %hd",
                v3->tshort1);
        EXFAIL_OUT(ret);
    }
    
    if (7!=v3->tshort2)
    {
        NDRX_LOG(log_error, "TESTERROR: tshort2 must be 7 but must be %hd",
                v3->tshort2);
        EXFAIL_OUT(ret);
    }
    
    if (8!=v3->tshort3)
    {
        NDRX_LOG(log_error, "TESTERROR: tshort3 must be 8 but must be %hd",
                v3->tshort3);
        EXFAIL_OUT(ret);
    }
    
    v3->tshort1 = 9;
    v3->tshort2 = 1;
    v3->tshort3 = 2;

out:
    
    tpreturn(EXSUCCEED==ret?TPSUCCESS:TPFAIL, 0, buf, 0L, 0L);
}



/**
 * Do initialisation
 */
int NDRX_INTEGRA(tpsvrinit)(int argc, char **argv)
{
    int ret=EXSUCCEED;
    
    NDRX_LOG(log_debug, "tpsvrinit called");


    if (EXSUCCEED!=tpadvertise("TEST40_VIEW", TEST40_VIEW))
    {
        NDRX_LOG(log_error, "Failed to initialise TEST40_VIEW!");
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=tpadvertise("TEST40_V2JSON", TEST40_V2JSON))
    {
        NDRX_LOG(log_error, "Failed to initialise TEST40_V2JSON!");
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=tpadvertise("TEST40_JSON2V", TEST40_JSON2V))
    {
        NDRX_LOG(log_error, "Failed to initialise TEST40_JSON2V!");
        EXFAIL_OUT(ret);
    }

out:
    return ret;
}

/**
 * Do de-initialisation
 */
void NDRX_INTEGRA(tpsvrdone)(void)
{
    NDRX_LOG(log_debug, "tpsvrdone called");
}

