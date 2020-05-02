/**
 * @brief Another conv server, just to test that first service is able
 *  to open conversational traffic to other service (i.e. not just client
 *  oriented connection opening).
 *
 * @file atmisv75_conv2.c
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
#include "ubf_int.h"
#include <exassert.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
exprivate __thread int M_notifs = 0;
/*---------------------------Prototypes---------------------------------*/

/**
 * Set notification handler 
 * @return 
 */
void notification_callback (char *data, long len, long flags)
{
    M_notifs++;
}

/**
 * Conversational server
 */
void CONVSV2 (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;
    char *buf = p_svc->data;
    long revent, len;
    int i;
    NDRX_LOG(log_debug, "%s got call", __func__);
    M_notifs = 0;
    
    buf = tprealloc(buf, 1024);
    NDRX_ASSERT_TP_OUT((NULL!=buf), "Failed to realloc");
    
    /* client: connect 4x (4x threads):  Using string buffer ... */
    NDRX_ASSERT_VAL_OUT((strcmp(buf, "HELLO")==0), "Expected HELLO at connection point");
    
    /* we shall receive some stuff... */
    NDRX_ASSERT_TP_OUT((EXFAIL==tprecv(p_svc->cd, &buf, &len, 0, &revent)), "Expected failure");
    NDRX_ASSERT_TP_OUT( (TPEEVENT==tperrno),  "Expected TPEEVENT");
    NDRX_ASSERT_TP_OUT( (TPEV_SENDONLY==revent),  "Expected TPEV_SENDONLY");
    NDRX_ASSERT_VAL_OUT((strcmp(buf, "CLWAIT")==0), "Expected CLWAIT at connection point");
    
    /* Wait for notification... */
    tperrno = 0;
    while (EXFAIL!=tpchkunsol() && M_notifs < 9)
    {
        usleep(1000);
    }
    
    NDRX_ASSERT_TP_OUT( (0==tperrno), "Expected no error");

    /* send number back... */
    
    buf = tprealloc(buf, 1024);
    NDRX_ASSERT_TP_OUT((NULL!=buf), "Failed to realloc");
    sprintf(buf, "%d", M_notifs);
    
    NDRX_ASSERT_TP_OUT( (EXSUCCEED==tpsend(p_svc->cd, buf, 0, 0, &revent)), 
                    "Failed send nr of notifs...");
    /* OK finish off... */
out:
    tpreturn(  ret==EXSUCCEED?TPSUCCESS:TPFAIL,
                0L,
                (char *)buf,
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

    NDRX_ASSERT_TP_OUT((EXSUCCEED==tpadvertise("CONVSV2", CONVSV2)), 
            "Failed to advertise");
    
out:
        
    return ret;
}

/**
 * Do de-initialisation
 * After the server, thread pool is stopped
 */
void tpsvrdone(void)
{
    NDRX_LOG(log_debug, "tpsvrdone called");
}

/**
 * Do initialisation
 */
int tpsvrthrinit(int argc, char **argv)
{
    int ret = EXSUCCEED;
    
    NDRX_ASSERT_TP_OUT((NULL==tpsetunsol(notification_callback)), 
            "Invalid previous unsol handler");
    
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
