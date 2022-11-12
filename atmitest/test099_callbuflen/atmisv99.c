/**
 * @brief Test TPSVCINFO.len - server
 *
 * @file atmisv99.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2019, Mavimax, Ltd. All Rights Reserved.
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
#include <string.h>
#include <unistd.h>
#include "test99.h"
#include <exassert.h>
#include <test_view.h>

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
    long len;
    char type[16]="";
    char subtype[16]="";

    NDRX_ASSERT_TP_OUT(tptypes(p_svc->data, type, subtype)>=0, 
        "Failed to get buffer info %p", p_svc->data);

    /* match the buffer size with tptypes... */
    if (0==strcmp(type, "UBF"))
    {
        NDRX_ASSERT_VAL_OUT(p_svc->len==1024, "TPSVCINFO.len [%ld] != 1024", p_svc->len);
    }
    else if (0==strcmp(type, "STRING"))
    {
        NDRX_ASSERT_VAL_OUT(p_svc->len==6, "TPSVCINFO.len [%ld] != 6", p_svc->len);
    }
    else if (0==strcmp(type, "CARRAY"))
    {
        NDRX_ASSERT_VAL_OUT(p_svc->len==5, "TPSVCINFO.len [%ld] != 5", p_svc->len);
    }
    else if (0==strcmp(type, "JSON"))
    {
        NDRX_ASSERT_VAL_OUT(p_svc->len==3, "TPSVCINFO.len [%ld] != 3", p_svc->len);
    }
    else if (0==strcmp(type, "VIEW"))
    {
        NDRX_ASSERT_VAL_OUT(p_svc->len==sizeof(struct UBTESTVIEW1), 
            "TPSVCINFO.len [%ld] != %ld", p_svc->len, sizeof(struct UBTESTVIEW1));
    }
    else if (0==strcmp(type, "NULL"))
    {
        NDRX_ASSERT_VAL_OUT(p_svc->len==0, "TPSVCINFO.len [%ld] != 0", p_svc->len);
    }
    else
    {
        NDRX_ASSERT_VAL_OUT(EXFALSE, "Unexpected buffer type [%s]", type);
    }
    
out:
    tpreturn(  ret==EXSUCCEED?TPSUCCESS:TPFAIL,
                0L,
                p_svc->data,
                0L,
                0L);
}

/**
 * Do initialisation
 */
int NDRX_INTEGRA(tpsvrinit)(int argc, char **argv)
{
    int ret = EXSUCCEED;
    NDRX_LOG(log_debug, "tpsvrinit called");

    if (EXSUCCEED!=tpadvertise("TESTSV", TESTSV))
    {
        NDRX_LOG(log_error, "Failed to initialise TESTSV!");
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

/* vim: set ts=4 sw=4 et smartindent: */
