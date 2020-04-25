/**
 * @brief Daemon server example. Deamon will print to log some messages...
 *
 * @file atmisv75_dmn.c
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
#include "test75.h"

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

int M_stopping = EXFALSE;
/*---------------------------Prototypes---------------------------------*/

/**
 * Standard service entry, daemon thread
 */
void DMNSV (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;
    UBFH *p_ub = (UBFH *)p_svc->data;

    NDRX_LOG(log_debug, "%s got call", __func__);

    /* Just print the buffer */
    Bprint(p_ub);
    
    /* Check the buffer... */
    while(!M_stopping)
    {
        NDRX_LOG(log_debug, "Daemon running...");
        sleep(1);
    }
    
out:
    tpreturn(  ret==EXSUCCEED?TPSUCCESS:TPFAIL,
                0L,
                (char *)p_ub,
                0L,
                0L);
}

/**
 * Stop control service
 * @param p_svc
 */
void DMNSV_STOP (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;
    
    M_stopping=EXTRUE;
    
out:
    tpreturn(  ret==EXSUCCEED?TPSUCCESS:TPFAIL,
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
    UBFH *p_ub = NULL;
    NDRX_LOG(log_debug, "tpsvrinit called");

    if (EXSUCCEED!=tpadvertise("DMNSV", DMNSV))
    {
        NDRX_LOG(log_error, "TESTERROR Failed to initialise DMNSV!");
        EXFAIL_OUT(ret);
    }
    
    /* start the service, send some data */
    
    p_ub = (UBFH *)tpalloc("UBF", NULL, 1024);
    
    if (NULL==p_ub)
    {
        NDRX_LOG(log_error, "TESTERROR Failed to alloc");
        EXFAIL_OUT(ret);
    }
    
    Bchg(p_ub, BFLD_STRING, 0, "TEST DMN", 0L);
    
    if (EXSUCCEED!=tpacall("DMNSV", (char *)p_ub, 0, TPNOREPLY))
    {
        NDRX_LOG(log_error, "TESTERROR Failed to call DMNSV");
        EXFAIL_OUT(ret);
    }
    
    /* call twice... */
    if (EXSUCCEED!=tpacall("DMNSV", (char *)p_ub, 0, TPNOREPLY))
    {
        NDRX_LOG(log_error, "TESTERROR Failed to call DMNSV");
        EXFAIL_OUT(ret);
    }
    
out:
        
    if (NULL!=p_ub)
    {
        tpfree((char *)p_ub);
    }

    return ret;
}

/**
 * Do de-initialisation
 */
void tpsvrdone(void)
{
    NDRX_LOG(log_debug, "tpsvrdone called");
    
    M_stopping=EXTRUE;
}

/**
 * Do initialisation
 */
int tpsvrthrinit(int argc, char **argv)
{
    int ret = EXSUCCEED;
    
    /* will call few times, but nothing todo... just test */
    if (EXSUCCEED!=tpadvertise("DMNSV_STOP", DMNSV_STOP))
    {
        NDRX_LOG(log_error, "Failed to initialise DMNSV_STOP!");
        EXFAIL_OUT(ret);
    }
    
out:
    return ret;
}

/**
 * Do de-initialisation
 */
void tpsvrthrdone(void)
{
    NDRX_LOG(log_debug, "tpsvrthrdone called");
}

/* Auto generated system advertise table */
expublic struct tmdsptchtbl_t ndrx_G_tmdsptchtbl[] = {
    { NULL, NULL, NULL, 0, 0 }
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
