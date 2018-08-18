/**
 * @brief Basic test client
 *
 * @file atmiclt4.c
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
    int i;
    int ret=EXSUCCEED;
    
    int num1=2;
    int num2=1;
    int num3=0;

    CBadd(p_ub, T_DOUBLE_FLD, "5", 0, BFLD_STRING);
    Badd(p_ub, T_STRING_FLD, "THIS IS TEST FIELD 2", 0);
    Badd(p_ub, T_STRING_FLD, "THIS IS TEST FIELD 3", 0);
    
    if (argc>1 && 'Y'==argv[1][0])
    {
        /* networked run */
        num1*=3;
        num2*=3;
    }
    
    /* Do it many times...! */
    for (i=0; i<1000; i++)
    {
        ret=tppost("EVX.TEST", (char*)p_ub, 0L, TPSIGRSTRT);
        NDRX_LOG(log_debug, "dispatched events: %d", ret);
        
        /* two servers process this */
        if (ret!=num1)
        {
            NDRX_LOG(log_error, "Applied event count is not 2 (which is %d)", ret);
            ret=EXFAIL;
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

    /* one server processes this - Support #279 */
    if (num2!=ret)
    {
        NDRX_LOG(log_error, "TESTERROR: First post of TEST2EV did not return 3 (%d) ",
                                    ret);
        ret=EXFAIL;
        goto out;
    }
    sleep(10); /* << because server may not complete the unsubscribe! */
    ret=tppost("TEST2EV", (char*)p_ub, 0L, TPSIGRSTRT);
    if (num3!=ret)
    {
        NDRX_LOG(log_error, "TESTERROR: Second post of TEST2EV did not return 0 (%d) ",
                                    ret);
        ret=EXFAIL;
        goto out;
    }

out:


    if (ret>=0)
    {
        ret=EXSUCCEED;
    }

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
