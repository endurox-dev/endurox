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
#include "ubf_int.h"
#include <exassert.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

int M_stopping1 = EXFALSE;
int M_stopping2 = EXFALSE;
/*---------------------------Prototypes---------------------------------*/

/**
 * Standard service entry, daemon thread
 */
void DMNSV1 (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;
    char tmp_buf[64];
    UBFH *p_ub = (UBFH *)p_svc->data;

    NDRX_LOG(log_debug, "%s got call", __func__);
    
    /* Just print the buffer */
    Bprint(p_ub);
    
    /* check that we got the data correctly */
    
    NDRX_ASSERT_UBF_OUT((EXSUCCEED==Bget(p_ub, T_STRING_2_FLD, 0, tmp_buf, NULL)),
            "Failed to get T_STRING_2_FLD");
    
    NDRX_ASSERT_VAL_OUT((0==strcmp(tmp_buf, "TEST_UBF")),
            "Invalid value");
    
    /* Check the buffer... */
    while(!M_stopping1)
    {
        NDRX_LOG(log_debug, "Daemon running...");
        sleep(1);
    }
    
    M_stopping1=EXFALSE;
    
out:
    tpreturn(  ret==EXSUCCEED?TPSUCCESS:TPFAIL,
                0L,
                (char *)p_ub,
                0L,
                0L);
}

/**
 * Standard service entry, daemon thread
 */
void DMNSV2 (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;

    NDRX_LOG(log_debug, "%s got call", __func__);

    /* Just print the buffer */
    if (NULL!=p_svc->data)
    {
        NDRX_LOG(log_error, "Expected NULL data");
        EXFAIL_OUT(ret);
    }
    
    /* Check the buffer... */
    while(!M_stopping2)
    {
        NDRX_LOG(log_debug, "Daemon running...");
        sleep(1);
    }
    
    M_stopping2=EXFALSE;
    
out:
    tpreturn(  ret==EXSUCCEED?TPSUCCESS:TPFAIL,
                0L,
                (char *)p_svc->data,
                0L,
                0L);
}

/**
 * Stop control service
 * @param p_svc
 */
void DMNSV_CTL (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;
    UBFH *p_ub = (UBFH *)p_svc->data;
    UBFH *p_ub_tmp = NULL;
    short dmn_no;
    char cmd[16];
    
    
    /* control which daemon to stop... Test busy flag of two daemons
     * if one stopped, the busy shall stay
     * if another stopped, the busy shall be removed
     */
    
    if (EXSUCCEED!=Bget(p_ub, T_STRING_FLD, 0, cmd, NULL))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to get command: %s", 
                Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=Bget(p_ub, T_SHORT_FLD, 0, (char *)&dmn_no, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to get command: %s", 
                Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    if (0==strcmp(cmd, "stop"))
    {
        switch (dmn_no)
        {
            case 1:
                M_stopping1=EXTRUE;
                break;
            case 2:
                M_stopping2=EXTRUE;
                break;
            default:
                NDRX_LOG(log_error, "Invalid dmn_no=%hd", dmn_no);
                break;
        }
    }
    else if (0==strcmp(cmd, "start"))
    {
        switch (dmn_no)
        {
            case 1:
                
                p_ub_tmp = (UBFH *)tpalloc("UBF", NULL, 1024);
                
                NDRX_ASSERT_TP_OUT((NULL!=p_ub_tmp), "Failed to alloc");
                NDRX_ASSERT_UBF_OUT((EXSUCCEED==Bchg(p_ub_tmp, T_STRING_2_FLD, 0, "TEST_UBF", 0)), 
                        "Failed to set");
                NDRX_ASSERT_TP_OUT((EXSUCCEED==tpacall("DMNSV1", (char *)p_ub_tmp, 
                        0, TPNOREPLY)), "Failed to call service with UBF");
                
                break;
            case 2:
                
                NDRX_ASSERT_TP_OUT((EXSUCCEED==tpacall("DMNSV2", (char *)p_ub_tmp, 
                        0, TPNOREPLY)), "Failed to call service with UBF");
                break;
            default:
                NDRX_LOG(log_error, "Invalid dmn_no=%hd", dmn_no);
                break;
        }
    }
    
out:
                
    if (NULL!=p_ub_tmp)
    {
        tpfree((char *)p_ub_tmp);
    }

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

    if (EXSUCCEED!=tpadvertise("DMNSV1", DMNSV1))
    {
        NDRX_LOG(log_error, "TESTERROR Failed to initialise DMNSV!");
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=tpadvertise("DMNSV2", DMNSV2))
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
    
    Bchg(p_ub, T_STRING_2_FLD, 0, "TEST_UBF", 0L);
    
    if (EXSUCCEED!=tpacall("DMNSV1", (char *)p_ub, 0, TPNOREPLY))
    {
        NDRX_LOG(log_error, "TESTERROR Failed to call DMNSV");
        EXFAIL_OUT(ret);
    }
    
    /* call twice... */
    if (EXSUCCEED!=tpacall("DMNSV2", NULL, 0, TPNOREPLY))
    {
        NDRX_LOG(log_error, "TESTERROR Failed to call DMNSV2");
        EXFAIL_OUT(ret);
    }
    
    /* call not advertised service, shall fail */
    if (EXSUCCEED==tpacall("DMNSV3", (char *)p_ub, 0, TPNOREPLY))
    {
        NDRX_LOG(log_error, "TESTERROR There shall be no DMNSV3!");
        EXFAIL_OUT(ret);
    }
    
    if (tperrno!=TPENOENT)
    {
        NDRX_LOG(log_error, "TESTERROR tperror expected %d but got %d!",
                TPENOENT, tperrno);
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
 * After the server, thread pool is stopped
 */
void tpsvrdone(void)
{
    NDRX_LOG(log_debug, "tpsvrdone called");
    
    M_stopping1=EXTRUE;
    M_stopping2=EXTRUE;
}

/**
 * Do initialisation
 */
int tpsvrthrinit(int argc, char **argv)
{
    int ret = EXSUCCEED;
    
    /* will call few times, but nothing todo... just test */
    if (EXSUCCEED!=tpadvertise("DMNSV_CTL", DMNSV_CTL))
    {
        NDRX_LOG(log_error, "Failed to initialise DMNSV_CTL!");
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
