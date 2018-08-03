/**
 * @brief TP Notify testing...
 *
 * @file atmisv38.c
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
#include <ubfutil.h>

void SVC38 (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;
    char tmp[20];
    char nodeid[5];
    UBFH *p_ub = (UBFH *)p_svc->data;
    
    
    ndrx_debug_dump_UBF(log_error, "SVC38 got request", p_ub);
    
    
    if (EXSUCCEED!=Bget(p_ub, T_STRING_FLD, 0, tmp, NULL))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to get T_STRING_FLD!");
        EXFAIL_OUT(ret);
    }
    
    snprintf(nodeid, sizeof(nodeid), "%02ld", tpgetnodeid());
    
    /* put the reply nodeid */
    tmp[2]=nodeid[0];
    tmp[3]=nodeid[1];
    
    /* Change the buffer & send notif */
    tmp[0]='B';
    tmp[1]='B';
    
    if (EXSUCCEED!=Bchg(p_ub, T_STRING_FLD, 0, tmp, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to setup T_STRING_FLD!");
        EXFAIL_OUT(ret);
    }
    
    /* OK we shall send notification to client, b */
    
    if (EXSUCCEED!=tpnotify(&p_svc->cltid, (char *)p_ub, 0L, 0L))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to tpnotify()!");
        EXFAIL_OUT(ret);
    }
    
    
    /* Set to CC */
    tmp[0]='C';
    tmp[1]='C';
    
    if (EXSUCCEED!=Bchg(p_ub, T_STRING_FLD, 0, tmp, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to setup T_STRING_FLD!");
        EXFAIL_OUT(ret);
    }
    
out:
    tpreturn(  ret==EXSUCCEED?TPSUCCESS:TPFAIL,
                0L,
                (char *)p_ub,
                0L,
                0L);
}

/*
 * Do initialization
 */
int NDRX_INTEGRA(tpsvrinit)(int argc, char **argv)
{
    int ret = EXSUCCEED;
    char svcnm[MAXTIDENT+1];
    NDRX_LOG(log_debug, "tpsvrinit called");

    
    snprintf(svcnm, sizeof(svcnm), "SVC38_%02ld", tpgetnodeid());
    /* Advertise our local service... */
    if (EXSUCCEED!=tpadvertise(svcnm, SVC38))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to initialize [%s]!", svcnm);
        ret=EXFAIL;
    }
    
    
    return ret;
}

/**
 * Do de-initialization
 */
void NDRX_INTEGRA(tpsvrdone)(void)
{
    NDRX_LOG(log_debug, "tpsvrdone called");
}
