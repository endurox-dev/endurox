/**
 * @brief Both sites max send, avoid deadlock of full sockets - server
 *
 * @file atmisv72.c
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
#include "test72.h"

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
exprivate long M_count = 0; /**< number of calls received, 
                             * platform shall ensure the serialization */
pthread_spinlock_t M_count_lock;
/*---------------------------Prototypes---------------------------------*/

/**
 * Standard service entry
 */
void TESTSV (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;
    UBFH *p_ub = (UBFH *)p_svc->data;

    NDRX_LOG(log_debug, "%s got call", __func__);
    
    p_ub = (UBFH *)tprealloc((char *)p_ub, 1024);

    if (EXFAIL==Bchg(p_ub, T_LONG_FLD, 0, (char *)&M_count, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to get T_LONG_FLD: %s", 
                 Bstrerror(Berror));
        ret=EXFAIL;
        goto out;
    }
    pthread_spin_lock(&M_count_lock);
    M_count++;
    pthread_spin_unlock(&M_count_lock);
    
out:
    tpreturn(  ret==EXSUCCEED?TPSUCCESS:TPFAIL,
                0L,
                (char *)p_ub,
                0L,
                0L);
}

/**
 * Query number message processed...
 */
void GETINFOS (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;
    long cnt;
    UBFH *p_ub = (UBFH *)p_svc->data;

    NDRX_LOG(log_debug, "%s got call", __func__);
    
    p_ub = (UBFH *)tprealloc((char *)p_ub, 1024);
    
    pthread_spin_lock(&M_count_lock);
    cnt = M_count;
    pthread_spin_unlock(&M_count_lock);

    if (EXFAIL==Bchg(p_ub, T_LONG_FLD, 0, (char *)&cnt, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to get T_LONG_FLD: %s", 
                 Bstrerror(Berror));
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
    NDRX_LOG(log_debug, "tpsvrinit called");

    pthread_spin_init(&M_count_lock, PTHREAD_PROCESS_PRIVATE);
    
    if (EXSUCCEED!=tpadvertise("TESTSV", TESTSV))
    {
        NDRX_LOG(log_error, "Failed to initialise TESTSV!");
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=tpadvertise("GETINFOS", GETINFOS))
    {
        NDRX_LOG(log_error, "Failed to initialise GETINFOS!");
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
