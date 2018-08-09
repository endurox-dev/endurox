/**
 * @brief Simple thread server caller.
 *
 * @file atmiclt17.c
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
#include <math.h>

#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>
#include <test.fd.h>
#include <ndrstandard.h>
#include <nstopwatch.h>
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

    UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, 9217);
    long rsplen;
    int i;
    int ret=EXSUCCEED;
    double d;
    int j;
    int calls = 0;
    ndrx_stopwatch_t timer;
    double cps;

    ndrx_stopwatch_reset(&timer);
    for (j = 0; j<100; j++)
    {
        UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, 9217);

    double dv = 55.66;

    ndrx_stopwatch_t timer;
    int call_num = MAX_ASYNC_CALLS *4;
    Badd(p_ub, T_STRING_FLD, "THIS IS TEST FIELD 1", 0);
    Badd(p_ub, T_STRING_FLD, "THIS IS TEST FIELD 2", 0);
    Badd(p_ub, T_STRING_FLD, "THIS IS TEST FIELD 3", 0);
    Badd(p_ub, T_DOUBLE_FLD, (char *)&dv, 0);

    for (i=1; i<100; i++)
    {
        calls++;
        dv+=1;
        
        if (EXFAIL == tpcall("TESTSV", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0))
        {
            NDRX_LOG(log_error, "TESTERROR TESTSV failed: %s", tpstrerror(tperrno));
            ret=EXFAIL;
            goto out;
        }

        /* Verify the data */
        if (EXFAIL==Bget(p_ub, T_DOUBLE_FLD, i, (char *)&d, 0))
        {
            NDRX_LOG(log_error, "TESTERROR Failed to get T_DOUBLE_FLD[%d]", i);
            ret=EXFAIL;
            goto out;
        }

        if (fabs(dv - d) > 0.00001)
        {
            NDRX_LOG(log_error, "TESTERROR %lf!=%lf =>  FAIL", dv, d);
            ret=EXFAIL;
            goto out;
        }
        
        /* print the output 
        Bfprint(p_ub, stderr);
        */
    }
           tpfree((char *)p_ub);
    }
   
    cps = (double)(calls)/(double)ndrx_stopwatch_get_delta_sec(&timer);
    printf("Performance - Nr calls: %d calls per sec: %lf\n", calls, cps);
    
out:
    tpterm();
    NDRX_LOG(log_error, "RESULT: %d", ret);
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
