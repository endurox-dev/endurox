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
int start_daemon_recover(void);

/**
 * Nothing to do here..
 * 
 * @param p_svc
 */
void MIB (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;
    
    UBFH *p_ub = (UBFH *)p_svc->data;
    
    
    ndrx_debug_dump_UBF(log_info, "Request buffer:", p_ub);
    
    /* TODO: Get request class: */
    
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
    char svcnm2[MAXTIDENT+1];
    
    snprintf(svcnm2, sizeof(svcnm2), NDRX_SVC_TMIBNODE, tpgetnodeid());

    if (EXSUCCEED!=tpadvertise(NDRX_SVC_TMIB, MIB))
    {
        NDRX_LOG(log_error, "Failed to initialize [%s]!", NDRX_SVC_TMIB);
        EXFAIL_OUT(ret);
    }
    else if (EXSUCCEED!=tpadvertise(svcnm2, MIB))
    {
        NDRX_LOG(log_error, "Failed to initialize [%s]!", svcnm2);
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
