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
#include <tpadm.h>

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
 * MIB Service
 * @param p_svc
 */
void MIB (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;
    char clazz[MAXTIDENT+1];
    char op[MAXTIDENT+1];
    char cursorid[MAXTIDENT+1];
    ndrx_adm_cursors_t cursnew; /* New cursor */
    BFLDLEN len;
    UBFH *p_ub = (UBFH *)p_svc->data;
    /* get the incoming buffer new size: */
    long bufsz = NDRX_MIN(TPADM_BUFFER_MINSZ, NDRX_MSGSIZEMAX);
    
    /* Allocate some buffer size  */
    
    ndrx_debug_dump_UBF(log_info, "Request buffer:", p_ub);
    
    /* Realloc to size: */
    p_ub = (UBFH *)tprealloc (p_svc->data, bufsz);
        
    if (NULL==p_ub)
    {
        NDRX_LOG(log_error, "Failed realloc UBF to %d bytes: %s", 
                bufsz, tpstrerror(tperrno));
        p_ub = (UBFH *)p_svc->data;
        EXFAIL_OUT(ret);
    }
    
    /* Get request class: 
     * In case if we get cursor from other node, then we shall forward there
     * the current request.
     */
    len = sizeof(clazz);
    if (EXSUCCEED!=Bget(p_ub, TA_CLASS, 0, clazz, &len))
    {
        NDRX_LOG(log_error, "Failed to get TA_CLASS: %s", Bstrerror(Berror));
        
        ndrx_adm_error_set(p_ub, TAEREQUIRED, TA_CLASS, 
                "Failed to get TA_CLASS: %s", Bstrerror(Berror));
        
        EXFAIL_OUT(ret);
    }
    
    len = sizeof(op);
    if (EXSUCCEED!=Bget(p_ub, TA_OPERATION, 0, op, &len))
    {
        NDRX_LOG(log_error, "Failed to get TA_OPERATION: %s", Bstrerror(Berror));
        
        ndrx_adm_error_set(p_ub, TAEREQUIRED, TA_OPERATION, 
                "Failed to get TA_OPERATION: %s", Bstrerror(Berror));
        
        EXFAIL_OUT(ret);
    }
    
    /* get cursor if required */
    if (0==strcmp(NDRX_TA_GETNEXT, op))
    {
        len = sizeof(cursorid);
        if (EXSUCCEED!=Bget(p_ub, TA_CURSOR, 0, cursorid, &len))
        {
            NDRX_LOG(log_error, "Failed to get TA_CURSOR: %s", Bstrerror(Berror));

            ndrx_adm_error_set(p_ub, TAEREQUIRED, TA_CURSOR, 
                        "Failed to get TA_CURSOR: %s", Bstrerror(Berror));

            EXFAIL_OUT(ret);
        }
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

    
    if (0==strcmp(NDRX_TA_GET, op))
    {
        memset(&cursnew, 0, sizeof(cursnew));
        
        /* run the selector... */
        if (0==strcmp(clazz, NDRX_TA_CLASS_CLIENT))
        {
            if (EXSUCCEED!=ndrx_adm_client_get(&cursnew))
            {
                NDRX_LOG(log_error, "Failed to open client cursor");

                ndrx_adm_error_set(p_ub, TAESYSTEM, BBADFLDID, 
                            "Failed to open client cursor");
                EXFAIL_OUT(ret);
            }
        }
        else
        {
            NDRX_LOG(log_error, "Unsupported class [%s]", clazz);

            ndrx_adm_error_set(p_ub, TAESUPPORT, BBADFLDID, 
                        "Unsupported class [%s]", clazz);
            EXFAIL_OUT(ret);
        }
        
    }
    
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
