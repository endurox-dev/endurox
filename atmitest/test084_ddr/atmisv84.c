/**
 * @brief DDR functionality tests - server
 *
 * @file atmisv84.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2023, Mavimax, Ltd. All Rights Reserved.
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
#include "test84.h"
#include "exsha1.h"

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Standard service entry
 */
void TESTSV (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;
    UBFH *p_ub = (UBFH *)p_svc->data;
    
    if (NULL==p_ub)
    {
        /* it's OK */
        goto out;
    }
    
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


void DYNSV(TPSVCINFO *p_svc)
{
    tpreturn(  TPSUCCESS,
                0L,
                (char *)p_svc->data,
                0L,
                0L);
}

void DYNADV(TPSVCINFO *p_svc)
{
    UBFH *p_ub = (UBFH *)p_svc->data;
    int ret = EXSUCCEED;
    char svcnm[XATMI_SERVICE_NAME_LENGTH+1];
    BFLDLEN len;
    
    len = sizeof(svcnm);
    
    if (EXFAIL==Bget(p_ub, T_STRING_2_FLD, 0, svcnm, &len))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to get T_STRING_2_FLD: %s", Bstrerror(Berror));
        ret=EXFAIL;
        goto out;
    }
    
    if (0==strcmp(p_svc->name, "DADV"))
    {
        if (EXSUCCEED!=tpadvertise(svcnm, DYNSV))
        {
            NDRX_LOG(log_error, "Failed to advertise: %s", tpstrerror(tperrno));
            ret=EXFAIL;
            goto out;
        }
    }
    else if (0==strcmp(p_svc->name, "DUNA"))
    {
        if (EXSUCCEED!=tpunadvertise(svcnm))
        {
            NDRX_LOG(log_error, "Failed to unadvertise: %s", tpstrerror(tperrno));
            ret=EXFAIL;
            goto out;
        }
    }
    
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
 * Forward service
 * @param p_svc
 */
void FWDSV (TPSVCINFO *p_svc)
{
    tpforward("TESTSV", p_svc->data, p_svc->len, 0);
}

/**
 * Do initialisation
 */
int NDRX_INTEGRA(tpsvrinit)(int argc, char **argv)
{
    int ret = EXSUCCEED;
    NDRX_LOG(log_debug, "tpsvrinit called");

    if (EXSUCCEED!=tpadvertise("TESTSV", TESTSV))
    {
        NDRX_LOG(log_error, "Failed to initialise TESTSV!");
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=tpadvertise("FWDSV", FWDSV))
    {
        NDRX_LOG(log_error, "Failed to initialise FWDSV!");
        EXFAIL_OUT(ret);
    }
    
    /* Check that "UNASV" is not present by psc */
    if (EXSUCCEED!=tpadvertise("UNASV", TESTSV))
    {
        NDRX_LOG(log_error, "Failed to initialise FWDSV!");
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=tpadvertise("DYNADV", DYNADV))
    {
        NDRX_LOG(log_error, "Failed to initialise DYNADV!");
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=tpunadvertise("UNASV"))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to unadvertise!");
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
