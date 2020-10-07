/**
 * @brief Basic test client
 *
 * @file atmiclt4.c
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
#define LOOP_NUM    10000
#define THREADS_NUM 5
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/

/*---------------------------Statics------------------------------------*/

exprivate int num1=2;
exprivate int num2=1;
exprivate int num3=0;
exprivate int M_err = EXFALSE;
/*---------------------------Prototypes---------------------------------*/

/**
 * Run test from thread...
 * @param arg
 * @return 
 */
void* thmain(void* arg)
{

    UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, 1024);
    int i;
    int ret=EXSUCCEED;

    CBadd(p_ub, T_DOUBLE_FLD, "5", 0, BFLD_STRING);
    Badd(p_ub, T_STRING_FLD, "THIS IS TEST FIELD 2", 0);
    Badd(p_ub, T_STRING_FLD, "THIS IS TEST FIELD 3", 0);
    
    /* Do it many times...! */
    for (i=0; i<LOOP_NUM; i++)
    {
        ret=tppost("EVX.TEST", (char*)p_ub, 0L, TPSIGRSTRT);
        NDRX_LOG(log_debug, "dispatched events: %d", ret);
        
        /* two servers process this */
        if (ret!=num1)
        {
            NDRX_LOG(log_error, "Applied event count is not %d (which is %d)", 
                    num1, ret);
            ret=EXFAIL;
            goto out;
        }
        else
        {
            NDRX_LOG(log_debug, "6 dispatches - OK");
        }
    }
    
out:
    if (EXFAIL==ret)
    {
        M_err=EXTRUE;
    }
        
    tpterm();
    return NULL;
}
/*
 * Do the test call to the server
 */
int main(int argc, char** argv) {

    UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, 1024);
    int i;
    int ret=EXSUCCEED;
    pthread_t th[THREADS_NUM];

    CBadd(p_ub, T_DOUBLE_FLD, "5", 0, BFLD_STRING);
    Badd(p_ub, T_STRING_FLD, "THIS IS TEST FIELD 2", 0);
    Badd(p_ub, T_STRING_FLD, "THIS IS TEST FIELD 3", 0);
    
    if (argc>1 && 'Y'==argv[1][0])
    {
        /* networked run */
        num1*=3;
        num2*=3;
    }
    
    for (i=0; i<THREADS_NUM; i++)
    {
        pthread_create(&th[i], NULL, thmain, NULL);
    }
    
    for (i=0; i<THREADS_NUM; i++)
    {
        void* val;
        pthread_join(th[i], &val);
    }
    
    if (M_err)
    {
        NDRX_LOG(log_error, "TESTERROR! some thread failed");
        EXFAIL_OUT(ret);
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
