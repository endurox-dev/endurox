/**
 * @brief UBF Field table database tests - server
 *
 * @file atmisv50.c
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
#include <unistd.h>
#include "test50.h"

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Standard service entry
 */
void TESTSV (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;
    char testbuf[1024];
    UBFH *p_ub = (UBFH *)p_svc->data;
    static long l=0;
    long ll;
    short s;
    char c;
    NDRX_LOG(log_debug, "%s got call", __func__);

    /* Just print the buffer */
    Bprint(p_ub);
    
    /* test string */
    if (EXSUCCEED!=Bget(p_ub, Bfldid("T_HELLO_STR"), 0, testbuf, 0L))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to get T_HELLO_STR: %s", 
                 Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    if (0!=strcmp(testbuf, "HELLO WORLD"))
    {
        NDRX_LOG(log_error, "TESTERROR: expected [HELLO WORLD] got [%s]",
                testbuf);
        EXFAIL_OUT(ret);
    }
    
    /* test short */
    if (EXSUCCEED!=Bget(p_ub, Bfldid("T_HELLO_SHORT"), 0, (char *)&s, 0L))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to get T_HELLO_SHORT: %s", 
                 Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    if (56!=s)
    {
        NDRX_LOG(log_error, "TESTERROR: expected [56] got [%hd]", s);
        EXFAIL_OUT(ret);
    }
    
    /* test char */
    if (EXSUCCEED!=Bget(p_ub, Bfldid("T_HELLO_CHAR"), 0, (char *)&c, 0L))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to get T_HELLO_CHAR: %s", 
                 Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    if ('A'!=c)
    {
        NDRX_LOG(log_error, "TESTERROR: expected [A] got [%c]", c);
        EXFAIL_OUT(ret);
    }
    
    /* test long */
    if (EXSUCCEED!=Bget(p_ub, Bfldid("T_HELLO_LONG"), 0, (char *)&ll, 0L))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to get T_HELLO_LONG: %s", 
                 Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    if (ll!=l)
    {
        NDRX_LOG(log_error, "TESTERROR: expected [%ld] got [%ld]", 
                l, ll);
        EXFAIL_OUT(ret);
    }
    
    /* test other string (fd) */
    if (EXSUCCEED!=Bget(p_ub, Bfldid("T_STRING_FLD"), 0, testbuf, 0L))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to get T_STRING_FLD: %s", 
                 Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    if (0!=strcmp(testbuf, "ABC"))
    {
        NDRX_LOG(log_error, "TESTERROR: expected [ABC] got [%s]",
                testbuf);
        EXFAIL_OUT(ret);
    }
    
    /* test field names */
    if (0!=strcmp(Bfname(Bfldid("T_HELLO_STR")), "T_HELLO_STR"))
    {
        NDRX_LOG(log_error, "TESTERROR: Reverse testing T_HELLO_STR failed");
        EXFAIL_OUT(ret);
    }
    
    if (0!=strcmp(Bfname(Bfldid("T_HELLO_CHAR")), "T_HELLO_CHAR"))
    {
        NDRX_LOG(log_error, "TESTERROR: Reverse testing T_HELLO_CHAR failed");
        EXFAIL_OUT(ret);
    }
    
    if (0!=strcmp(Bfname(Bfldid("T_HELLO_LONG")), "T_HELLO_LONG"))
    {
        NDRX_LOG(log_error, "TESTERROR: Reverse testing T_HELLO_LONG failed");
        EXFAIL_OUT(ret);
    }
    
    if (0!=strcmp(Bfname(Bfldid("T_STRING_FLD")), "T_STRING_FLD"))
    {
        NDRX_LOG(log_error, "TESTERROR: Reverse testing T_STRING_FLD failed");
        EXFAIL_OUT(ret);
    }
    
out:
    l++;
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
    NDRX_LOG(log_debug, "tpsvrinit called");

    if (EXSUCCEED!=tpadvertise("TESTSV", TESTSV))
    {
        NDRX_LOG(log_error, "Failed to initialize TESTSV!");
    }
    
    return EXSUCCEED;
}

/**
 * Do de-initialization
 */
void NDRX_INTEGRA(tpsvrdone)(void)
{
    NDRX_LOG(log_debug, "tpsvrdone called");
}

