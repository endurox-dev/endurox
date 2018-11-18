/**
 * @brief Basic test client
 *
 * @file atmiclt2.c
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

    UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, 8192);
    long rsplen;
    int i,j;
    long cnt = 0;
    int ret=EXSUCCEED;
    double d, d2;
    double dv = 55.66;
    double dv2 = 11.66;
    
for (j=0; j<100000; j++)
{
    Binit(p_ub, Bsizeof(p_ub));

    Badd(p_ub, T_STRING_FLD, "THIS IS TEST FIELD 1", 0);
    Badd(p_ub, T_STRING_FLD, "THIS IS TEST FIELD 2", 0);
    Badd(p_ub, T_STRING_FLD, "THIS IS TEST FIELD 3", 0);

    for (i=0; i<100; i++)
    {
        cnt++;
        dv+=1;
        dv2+=1;
        
        if (EXFAIL == tpcall("TEST2_1ST_AL", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0))
        {
            NDRX_LOG(log_error, "TEST2_1ST_AL failed: %s",
                    tpstrerror(tperrno));
            ret=EXFAIL;
            goto out;
        }

        /* Verify the data */
        if (EXFAIL==Bget(p_ub, T_DOUBLE_FLD, i, (char *)&d, 0))
        {
            NDRX_LOG(log_debug, "Failed to get T_DOUBLE_FLD[%d]", i);
            ret=EXFAIL;
            goto out;
        }

        if (EXFAIL==Bget(p_ub, T_DOUBLE_2_FLD, i, (char *)&d2, 0))
        {
            NDRX_LOG(log_debug, "Failed to get T_DOUBLE_2_FLD[%d]", i);
            ret=EXFAIL;
            goto out;
        }

        if (fabs(dv-d) > 0.00001)
        {
            NDRX_LOG(log_debug, "T_DOUBLE_FLD: %lf!=%lf =>  FAIL", dv, d);
            ret=EXFAIL;
            goto out;
        }

        if (fabs(dv2 - d2) > 0.00001)
        {
            NDRX_LOG(log_debug, "T_DOUBLE_2_FLD: %lf!=%lf =>  FAIL", dv, d);
            ret=EXFAIL;
            goto out;
        }
        
        /* print the output */
        Bfprint(p_ub, stderr);
    }

    NDRX_LOG(log_debug, "CURRENT CNT: %ld", cnt);
    if (argc<=1)
    {
        break;
    }
}

out:
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
