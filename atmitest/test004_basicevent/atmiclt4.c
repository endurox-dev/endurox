/* 
** Basic test client
**
** @file atmiclt4.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, ATR Baltic, SIA. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or ATR Baltic's license for commercial use.
** -----------------------------------------------------------------------------
** GPL license:
** 
** This program is free software; you can redistribute it and/or modify it under
** the terms of the GNU General Public License as published by the Free Software
** Foundation; either version 2 of the License, or (at your option) any later
** version.
**
** This program is distributed in the hope that it will be useful, but WITHOUT ANY
** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
** PARTICULAR PURPOSE. See the GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License along with
** this program; if not, write to the Free Software Foundation, Inc., 59 Temple
** Place, Suite 330, Boston, MA 02111-1307 USA
**
** -----------------------------------------------------------------------------
** A commercial use license is available from ATR Baltic, SIA
** contact@atrbaltic.com
** -----------------------------------------------------------------------------
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>

#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>
#include <test.fd.h>
#include <ndrstandard.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/*
 * Do the test call to the server
 */
int main(int argc, char** argv) {

    UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, 1024);
    long rsplen;
    int i;
    int ret=SUCCEED;

    
    CBadd(p_ub, T_DOUBLE_FLD, "5", 0, BFLD_STRING);
    Badd(p_ub, T_STRING_FLD, "THIS IS TEST FIELD 2", 0);
    Badd(p_ub, T_STRING_FLD, "THIS IS TEST FIELD 3", 0);

    /* Do it many times...! */
    for (i=0; i<9900; i++)
    {
        ret=tppost("EVX.TEST", (char*)p_ub, 0L, TPSIGRSTRT);
        NDRX_LOG(log_debug, "dispatched events: %d", ret);
        if (ret!=6)
        {
            NDRX_LOG(log_error, "Applied event count is not 6 (which is %d)", ret);
            ret=FAIL;
            goto out;
        }
        else
        {
            NDRX_LOG(log_debug, "6 dispatches - OK");
        }
        
        if (i % 1000 == 0)
        {
            /* let services to flush the stuff... */
            NDRX_LOG(log_debug, "Dispatched %d - sleeping", i);
            sleep(10);
        }
    }

    ret=tppost("TEST2EV", (char*)p_ub, 0L, TPSIGRSTRT);

    if (3!=ret)
    {
        NDRX_LOG(log_error, "TESTERROR: First post of TEST2EV did not return 3 (%d) ",
                                    ret);
        ret=FAIL;
        goto out;
    }
    sleep(10); /* << because server may not complete the unsubscribe! */
    ret=tppost("TEST2EV", (char*)p_ub, 0L, TPSIGRSTRT);
    if (0!=ret)
    {
        NDRX_LOG(log_error, "TESTERROR: Second post of TEST2EV did not return 0 (%d) ",
                                    ret);
        ret=FAIL;
        goto out;
    }

out:


    if (ret>=0)
        ret=SUCCEED;


    return ret;
}

