/**
 * @brief Test service / function aliasing
 *
 * @file atmisv84_2.c
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
#include <thlock.h>
#include "exassert.h"

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Standard service entry, function
 */
void TESTFN (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;
    UBFH *p_ub = (UBFH *)p_svc->data;
    
    /* ensure the size */
    if (NULL==(p_ub = (UBFH *)tprealloc((char *)p_ub, 1024)))
    {
        NDRX_LOG(log_error, "Failed to realloc: %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }

    NDRX_LOG(log_debug, "%s got call", __func__);
    
    /* Just print the buffer */
    Bprint(p_ub);
    
    if (EXFAIL==Bchg(p_ub, T_STRING_FLD, 0, p_svc->name, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to set T_STRING_FLD to [%s]: %s", 
                 p_svc->name, Bstrerror(Berror));
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
 * Multi-init from tpacall from thread init
 * @param p_svc
 */
void MULTI_INIT (TPSVCINFO *p_svc)
{
    tpreturn(  TPSUCCESS,
                0L,
                (char *)p_svc->data,
                0L,
                0L);
}
/**
 * Do initialisation
 */
int tpsvrinit(int argc, char **argv)
{
    int ret = EXSUCCEED;
    NDRX_LOG(log_debug, "tpsvrinit called");
out:
    return ret;
}

/**
 * Do de-initialisation
 */
void tpsvrdone(void)
{
    NDRX_LOG(log_debug, "tpsvrdone called");
}

/* Auto generated system advertise table */
expublic struct tmdsptchtbl_t ndrx_G_tmdsptchtbl[] = {
    { "", "TESTFN", TESTFN, 0, 0 }
    , { NULL, NULL, NULL, 0, 0 }
};
/*---------------------------Prototypes---------------------------------*/

/**
 * Main entry for tmsrv
 */
int main( int argc, char** argv )
{
    _tmbuilt_with_thread_option=EXTRUE;
    struct tmsvrargs_t tmsvrargs =
    {
        &tmnull_switch,
        &ndrx_G_tmdsptchtbl[0],
        0,
        tpsvrinit,
        tpsvrdone,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        tpsvrthrinit,
        tpsvrthrdone
    };
    
    return( _tmstartserver( argc, argv, &tmsvrargs ));
    
}


/* vim: set ts=4 sw=4 et smartindent: */
