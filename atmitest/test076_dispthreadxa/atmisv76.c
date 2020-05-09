/**
 * @brief Dispatch thread xa on cluster, null switch, buildtools - server
 *  We need: OKTX->OKTX2, FAILTX->FAILTX2, NOTX->NOTX2. The atmi client from threads shall call
 *  these services number of times
 *
 * @file atmisv76.c
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
#include <exassert.h>
#include "test76.h"

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Add transaction flag
 * @param p_ub buffer where to add
 * @return EXSCUCEED/EXFAIL
 */
exprivate int add_tx_flag(UBFH *p_ub)
{
    int ret = EXSUCCEED;
    
    if (tpgetlev())
    {
        NDRX_ASSERT_UBF_OUT((EXSUCCEED==Badd(p_ub, T_STRING_FLD, "INTX", 0)), 
                "Failed to add T_STRING_FLD");
    }
    else
    {
        NDRX_ASSERT_UBF_OUT((EXSUCCEED==Badd(p_ub, T_STRING_FLD, "NOTX", 0)), 
                "Failed to add T_STRING_FLD");
    }
    
out:
    return ret;    
}

/**
 * Second service returns OK or fail
 */
void SVC2 (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;
    UBFH *p_ub = (UBFH *)p_svc->data;
    
    NDRX_LOG(log_debug, "%s got call", __func__);
    
    /* realloc the buffer */
    p_ub = (UBFH *)tprealloc((char *)p_ub, 1024);
    
    NDRX_ASSERT_TP_OUT((NULL!=p_ub), "Failed to realloc");
    
    /* add transaction flag */
    if (EXSUCCEED!=add_tx_flag(p_ub))
    {
        EXFAIL_OUT(ret);
    }
    
    /* check the service is it failure or not? */
    
    if (NULL!=strstr(p_svc->name, "FAIL"))
    {
        ret=EXFAIL;
    }
    
out:
    
    tpreturn(  EXSUCCEED==ret?TPSUCCESS:TPFAIL,
                    0L,
                    (char *)p_ub,
                    0L,
                    0L);
}

/**
 * Standard service entry.
 * Forward to OK returned. Add to buffer infos that we are in transaction
 */
void SVC1 (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;
    UBFH *p_ub = (UBFH *)p_svc->data;
    char svcnm2[MAXTIDENT+1];
    long rsplen;
    
    NDRX_LOG(log_debug, "%s got call", __func__);
    
    /* realloc the buffer */
    
    p_ub = (UBFH *)tprealloc((char *)p_ub, 1024);
    
    NDRX_ASSERT_TP_OUT((NULL!=p_ub), "Failed to realloc");
    
    /* add transaction flag */
    if (EXSUCCEED!=add_tx_flag(p_ub))
    {
        EXFAIL_OUT(ret);
    }
    
    /* call target service, ignore result we will check buffer later
     * and also transaction shall be marked for failure if failed 
     */
    
    snprintf(svcnm2, sizeof(svcnm2), "%s2", p_svc->name);
    tpcall(svcnm2, (char *)p_ub, 0, (char **)&p_ub, &rsplen, 0);
    
out:

    tpreturn(  EXSUCCEED==ret?TPSUCCESS:TPFAIL,
                0L,
                (char *)p_ub,
                0L,
                0L);
}

/* system advertise table */
expublic struct tmdsptchtbl_t ndrx_G_tmdsptchtbl[] = {
    { "TXOK", "SVC1", SVC1, 0, 0 }
    ,{ "TXFAIL", "SVC1", SVC1, 0, 0 }
    ,{ "NOTX", "SVC1", SVC1, 0, 0 }
    ,{ "TXOK2", "SVC2", SVC2, 0, 0 }
    ,{ "TXFAIL2", "SVC2", SVC2, 0, 0 }
    ,{ "NOTX2", "SVC2", SVC2, 0, 0 }
    ,{ NULL, NULL, NULL, 0, 0 }
};

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
