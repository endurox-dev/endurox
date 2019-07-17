/**
 * @brief Administrative server
 *
 * @file tpadmsv.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * AGPL or Mavimax's license for commercial use.
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
#include <string.h>
#include <errno.h>
#include <regex.h>
#include <utlist.h>
#include <unistd.h>
#include <signal.h>

#include <ndebug.h>
#include <atmi.h>
#include <atmi_int.h>
#include <typed_buf.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <Exfields.h>
#include <Excompat.h>
#include <ubfutil.h>

#include "tpadmsv.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

expublic char ndrx_G_svcnm2[MAXTIDENT+1]; /** our service name, per instance. */

/**
 * Nothing to do here..
 * 
 * @param p_svc
 */
void MIB (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;
    char clazz[MAXTIDENT+1];
    char op[MAXTIDENT+1];
    BFLDLEN len;
    UBFH *p_ub = (UBFH *)p_svc->data;
    
    
    ndrx_debug_dump_UBF(log_info, "Request buffer:", p_ub);
    
    /* Get request class: 
     * In case if we get cursor from other node, then we shall forward there
     * the current request.
     */
    len = sizeof(clazz);
    if (EXSUCCEED!=Bget(TA_CLASS, 0, clazz, &len))
    {
        NDRX_LOG(log_error, "Failed to get TA_CLASS: %s", Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    len = sizeof(op);
    if (EXSUCCEED!=Bget(TA_OPERATION, 0, op, &len))
    {
        NDRX_LOG(log_error, "Failed to get TA_OPERATION: %s", Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    /* TODO: Needs functions for error handling:
     * with following arguments:
     * TA_ERROR code, TA_BADFLD (optional only for INVAL), TA_STATUS message.
     * The function shall receive UBF buffer and allow to process the format
     * string which is loaded into TA_STATUS.
     * 
     * We need a function to check is buffer approved, if so then we can set
     * the reject. Reject on reject is not possible (first is more significant).
     */

    
out:
    tpreturn(  ret==EXSUCCEED?TPSUCCESS:TPFAIL,
                0,
                (char *)p_svc->data,
                0L,
                0L);
}

/**
 * Do initialization
 * Have a local MIB & shared MIB
 */
int NDRX_INTEGRA(tpsvrinit)(int argc, char **argv)
{
    int ret=EXSUCCEED;
    
    snprintf(ndrx_G_svcnm2, sizeof(ndrx_G_svcnm2), NDRX_SVC_TMIBNODE, 
            tpgetnodeid(), tpgetsrvid());

    if (EXSUCCEED!=tpadvertise(NDRX_SVC_TMIB, MIB))
    {
        NDRX_LOG(log_error, "Failed to initialize [%s]!", NDRX_SVC_TMIB);
        EXFAIL_OUT(ret);
    }
    else if (EXSUCCEED!=tpadvertise(ndrx_G_svcnm2, MIB))
    {
        NDRX_LOG(log_error, "Failed to initialize [%s]!", ndrx_G_svcnm2);
        EXFAIL_OUT(ret);
    }

out:
    return ret;
}

void NDRX_INTEGRA(tpsvrdone)(void)
{
    /* just for build... */
}

/* vim: set ts=4 sw=4 et smartindent: */
