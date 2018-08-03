/**
 * @brief Test Environment in ndrxconfig.xml - server
 *
 * @file atmisv55.c
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
#include "test55.h"

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

    NDRX_LOG(log_debug, "%s got call", __func__);

    /* Just print the buffer */
    Bprint(p_ub);
    
    if (EXFAIL==Bget(p_ub, T_STRING_FLD, 0, testbuf, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to get T_STRING_FLD: %s", 
                 Bstrerror(Berror));
        ret=EXFAIL;
        goto out;
    }
    
    if (0!=strcmp(testbuf, VALUE_EXPECTED))
    {
        NDRX_LOG(log_error, "TESTERROR: Expected: [%s] got [%s]",
            VALUE_EXPECTED, testbuf);
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

/**
 * Do initialisation
 */
int NDRX_INTEGRA(tpsvrinit)(int argc, char **argv)
{
    int ret = EXSUCCEED;
    char *p;
    NDRX_LOG(log_debug, "tpsvrinit called");
      
    /* test environment variables... */
    
    NDRX_LOG(log_debug, "[%s] = [%s]", "HELLO1", getenv("HELLO1"));
    NDRX_LOG(log_debug, "[%s] = [%s]", "HELLO2", getenv("HELLO2"));
    NDRX_LOG(log_debug, "[%s] = [%s]", "HELLO3", getenv("HELLO3"));
    NDRX_LOG(log_debug, "[%s] = [%s]", "HELLO4", getenv("HELLO4"));
    NDRX_LOG(log_debug, "[%s] = [%s]", "HELLO5", getenv("HELLO5"));
    NDRX_LOG(log_debug, "[%s] = [%s]", "HELLO6", getenv("HELLO6"));
    NDRX_LOG(log_debug, "[%s] = [%s]", "HELLO7", getenv("HELLO7"));
    NDRX_LOG(log_debug, "[%s] = [%s]", "HELLO10", getenv("HELLO10"));
    NDRX_LOG(log_debug, "[%s] = [%s]", "HELLO11", getenv("HELLO11"));
    
    if (10==tpgetsrvid())
    {
        if (0!=strcmp((p=getenv("HELLO1")), "THIS IS 1"))
        {
            NDRX_LOG(log_error, "TESTERROR ! Expected [THIS IS 1] got [%s]", p);
            EXFAIL_OUT(ret);
        }
        
        if (0!=strcmp((p=getenv("HELLO2")), "THIS IS 2"))
        {
            NDRX_LOG(log_error, "TESTERROR ! Expected [THIS IS 2] got [%s]", p);
            EXFAIL_OUT(ret);
        }
        
        if (0!=strcmp((p=getenv("HELLO3")), "THIS IS 3"))
        {
            NDRX_LOG(log_error, "TESTERROR ! Expected [THIS IS 3] got [%s]", p);
            EXFAIL_OUT(ret);
        }
        
        if (NULL!=(p=getenv("HELLO4")))
        {
            NDRX_LOG(log_error, "TESTERROR ! Expected NULL for HELLO4 but got [%s]", p);
            EXFAIL_OUT(ret);
        }
        
        if (0!=strcmp((p=getenv("HELLO5")), "THIS IS 5"))
        {
            NDRX_LOG(log_error, "TESTERROR ! Expected [THIS IS 5] got [%s]", p);
            EXFAIL_OUT(ret);
        }

        if (NULL!=(p=getenv("HELLO6")))
        {
            NDRX_LOG(log_error, "TESTERROR ! Expected NULL for HELLO6 but got [%s]", p);
            EXFAIL_OUT(ret);
        }
        
        if (NULL!=(p=getenv("HELLO7")))
        {
            NDRX_LOG(log_error, "TESTERROR ! Expected NULL for HELLO7 but got [%s]", p);
            EXFAIL_OUT(ret);
        }        
    }
    else if (11==tpgetsrvid())
    {
        if (0!=strcmp((p=getenv("HELLO6")), "THIS IS 6"))
        {
            NDRX_LOG(log_error, "TESTERROR ! Expected [THIS IS 6] got [%s]", p);
            EXFAIL_OUT(ret);
        }
        
        if (0!=strcmp((p=getenv("HELLO7")), "THIS IS 7"))
        {
            NDRX_LOG(log_error, "TESTERROR ! Expected [THIS IS 7] got [%s]", p);
            EXFAIL_OUT(ret);
        }
        
        if (0!=strcmp((p=getenv("HELLO5")), "THIS IS 5"))
        {
            NDRX_LOG(log_error, "TESTERROR ! Expected [THIS IS 5] got [%s]", p);
            EXFAIL_OUT(ret);
        }
        
        if (NULL!=(p=getenv("HELLO1")))
        {
            NDRX_LOG(log_error, "TESTERROR ! Expected NULL for HELLO1 but got [%s]", p);
            EXFAIL_OUT(ret);
        }
        
        if (NULL!=(p=getenv("HELLO2")))
        {
            NDRX_LOG(log_error, "TESTERROR ! Expected NULL for HELLO2 but got [%s]", p);
            EXFAIL_OUT(ret);
        }
        
        if (NULL!=(p=getenv("HELLO3")))
        {
            NDRX_LOG(log_error, "TESTERROR ! Expected NULL for HELLO3 but got [%s]", p);
            EXFAIL_OUT(ret);
        }
        
        if (NULL!=(p=getenv("HELLO4")))
        {
            NDRX_LOG(log_error, "TESTERROR ! Expected NULL for HELLO4 but got [%s]", p);
            EXFAIL_OUT(ret);
        }
    }
    
    if (EXSUCCEED!=tpadvertise("TESTSV", TESTSV))
    {
        NDRX_LOG(log_error, "Failed to initialise TESTSV!");
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

