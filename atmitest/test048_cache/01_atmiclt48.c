/* 
 * @brief TP Cache tests - client
 *
 * @file 01_atmiclt48.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or ATR Baltic's license for commercial use.
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
#include <fcntl.h>
#include <unistd.h>
#include <nstdutil.h>
#include <ubfutil.h>
#include "test48.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Do the test call to the server
 * So some testing for cache could be for time based fields
 * If time stamp not changed, it is from cache
 * If changed then new data
 */
int main(int argc, char** argv)
{

    UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, 56000);
    long rsplen;
    short i;
    short i_res, i_res_prev = EXFAIL;
    int ret=EXSUCCEED;
    char testbuf[1024];
    char tstamp[TSTAMP_BUFSZ];
    char tstamp_prev[TSTAMP_BUFSZ];
    BFLDID emtpy [] = {BBADFLDID};

    for (i=0; i<200; i++)
    {
        if (EXSUCCEED!=Bproj(p_ub, emtpy))
        {
            NDRX_LOG(log_debug, "Failed to reset buffer: %s", Bstrerror(Berror));
            ret=EXFAIL;
            goto out;
        }

        if (EXFAIL==CBchg(p_ub, T_STRING_FLD, 0, VALUE_EXPECTED, 0, BFLD_STRING))
        {
            NDRX_LOG(log_debug, "Failed to set T_STRING_FLD[0]: %s", Bstrerror(Berror));
            ret=EXFAIL;
            goto out;
        } 

        if (EXFAIL==CBchg(p_ub, T_STRING_3_FLD, 0, "HELLO", 0, BFLD_STRING))
        {
            NDRX_LOG(log_debug, "Failed to set T_STRING_3_FLD[0]: %s", Bstrerror(Berror));
            ret=EXFAIL;
            goto out;
        }

        i_res = i / 5;
        
        if (EXFAIL==CBchg(p_ub, T_STRING_2_FLD, 1, (char *)&i_res, 0, BFLD_SHORT))
        {
            NDRX_LOG(log_debug, "Failed to set T_STRING_2_FLD[1]: %s", Bstrerror(Berror));
            ret=EXFAIL;
            goto out;
        }

        if (EXFAIL==CBchg(p_ub, T_LONG_FLD, 0, "4", 0, BFLD_STRING))
        {
            NDRX_LOG(log_debug, "Failed to set T_LONG_FLD[0]: %s", Bstrerror(Berror));
            ret=EXFAIL;
            goto out;
        }

        if (EXFAIL==CBchg(p_ub, T_SHORT_FLD, 0, "3", 0, BFLD_STRING))
        {
            NDRX_LOG(log_debug, "Failed to set T_SHORT_FLD[0]: %s", Bstrerror(Berror));
            ret=EXFAIL;
            goto out;
        } 

        if (EXFAIL == tpcall("TESTSV01", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0))
        {
            NDRX_LOG(log_error, "TESTSV01 failed: %s", tpstrerror(tperrno));
            ret=EXFAIL;
            goto out;
        }
        
        /* Dump the reply */
        
        ndrx_debug_dump_UBF(log_debug, "Got data from service", p_ub);
        
        
        /* validate response... */
        
        if (EXFAIL==Bget(p_ub, T_STRING_FLD, 1, testbuf, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to get T_STRING_FLD: %s", 
                     Bstrerror(Berror));
            ret=EXFAIL;
            goto out;
        }

        if (0!=strcmp(testbuf, VALUE_EXPECTED_RET))
        {
            NDRX_LOG(log_error, "TESTERROR: Expected: [%s] got [%s]",
                VALUE_EXPECTED_RET, testbuf);
            ret=EXFAIL;
            goto out;
        }
        
        if (EXFAIL==Bget(p_ub, T_STRING_5_FLD, 0, tstamp, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to get T_STRING_5_FLD - tstamp: %s", 
                     Bstrerror(Berror));
            ret=EXFAIL;
            goto out;
        }
        
        if (EXFAIL==i_res_prev)
        {
            /* first get cache prev */
            i_res_prev = i_res;
            NDRX_STRCPY_SAFE(tstamp_prev, tstamp);
        }
        else if (i_res_prev != i_res)
        {
            NDRX_STRCPY_SAFE(tstamp_prev, tstamp);
            i_res_prev = i_res;
        }
        else
        {
            /* compare with cache prev, should be the same as cached */
            
            if (0!=strcmp(tstamp, tstamp_prev))
            {
                NDRX_LOG(log_error, "TESTERROR ! i_res = %d, tstamp [%d] != tstamp_prev [%d]",
                        tstamp, tstamp_prev);
                EXFAIL_OUT(ret);
            }
        }
    }
    
out:
    tpterm();
    fprintf(stderr, "Exit with %d\n", ret);

    return ret;
}
