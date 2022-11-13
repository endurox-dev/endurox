/**
 * @brief Server stdout test - server
 *
 * @file atmisv100.c
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

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/


/**
 * Adervice service 
 */
void WRITELOG(TPSVCINFO *svcinfo)
{
    int ret=EXSUCCEED;
    char stdoutbuf[100]="";
    char stderrbuf[100]="";
    UBFH *p_ub = (UBFH *)svcinfo->data;
    
    if (EXSUCCEED!=Bget(p_ub, T_STRING_FLD, 0, stdoutbuf, 0L)||
        EXSUCCEED!=Bget(p_ub, T_STRING_2_FLD, 0, stderrbuf, 0L))
    {
        NDRX_LOG(log_error, "TESTERROR: Faild to get T_STRING_FLD or T_STRING_2_FLD");
        EXFAIL_OUT(ret);
    }

    fprintf(stdout, "%s\n", stdoutbuf);
    fflush(stdout);

    fprintf(stderr, "%s\n", stderrbuf);
    fflush(stderr);

out:
    tpreturn(  ret==EXSUCCEED?TPSUCCESS:TPFAIL,
                0L,
                svcinfo->data,
                0L,
                0L);

}

/**
 * Do initialisation
 */
int NDRX_INTEGRA(tpsvrinit)(int argc, char **argv)
{
    int ret = EXSUCCEED;
    char svc[MAXTIDENT+1];
    NDRX_LOG(log_debug, "tpsvrinit called");

    fprintf(stdout, "Hello world from stdout\n");
    fflush(stdout);

    fprintf(stderr, "Hello world from stderr\n");
    fflush(stderr);

    snprintf(svc, sizeof(svc), "WRITELOG_%d", tpgetsrvid());

    if (EXSUCCEED!=tpadvertise(svc, WRITELOG))
    {
        NDRX_LOG(log_error, "failed to advertise [%s]: %s",
            svc, tpstrerror(tperrno));
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
