/**
 * @brief Basic test client
 *
 * @file atmiclt2.c
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
#include <math.h>

#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>
#include <test.fd.h>
#include <ndrstandard.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

/* async messages get stuck for a while in tpbridge, for RPI mem is low... */
#if EX_SIZEOF_LONG==4
#define TEST_ASYNC_LOOPS    20000
#else
#define TEST_ASYNC_LOOPS    200000
#endif

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/


/**
 * This will test that tpacall forward will not hang in TPNOREPLY mode.
 * i.e. at the at the second forwarded service reply shall be ignored,
 * and not delivered back to client.
 * What happened here: Bug #570 is that client receive unexpected replies
 * and client queue become full.
 * @return EXSUCCEED/EXFAIL.
 */
exprivate int tpacall_tpnoreply_forward_test(void)
{
    int ret = EXSUCCEED;
    int j;
    long rsplen;
    UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, 8192);
    
    for (j=0; j<TEST_ASYNC_LOOPS; j++)
    {
        Binit(p_ub, Bsizeof(p_ub));
        Badd(p_ub, T_STRING_FLD, "tpacall_tpnoreply_forward_test", 0);

        if (EXFAIL==tpacall("TEST2_1ST_AL", (char *)p_ub, 0, TPNOREPLY))
        {
            NDRX_LOG(log_error, "TESTERROR: failed to call TEST2_1ST_AL with TPNOREPLY: %s", 
                    tpstrerror(tperrno));
            
            EXFAIL_OUT(ret);
        }
    }

    /* wait for queue to finish ...*/
    if (EXFAIL == tpcall("TEST2_1ST_AL", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,TPNOTIME))
    {
        NDRX_LOG(log_error, "TEST2_1ST_AL failed: %s",
               tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }
out:
    if (NULL!=p_ub)
    {
        tpfree((char *)p_ub);
    }
    return ret;
}

/**
 * Test that tpforward fails, but for TPNOREPLY we shall not receive any reply
 * Bug #570 - currently client queue may fill up
 * @return EXSUCCEED/EXFAIL
 */
exprivate int tpacall_tpnoreply_forward_nodestsrv(void)
{
    int ret = EXSUCCEED;
    int j;
    UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, 8192);
    long rsplen;
    
    for (j=0; j<TEST_ASYNC_LOOPS; j++)
    {
        Binit(p_ub, Bsizeof(p_ub));
        Badd(p_ub, T_STRING_FLD, "tpacall_tpnoreply_forward_nodestsrv", 0);
        Badd(p_ub, T_STRING_10_FLD, "failure set", 0);

        if (EXFAIL==tpacall("TEST2_1ST_AL", (char *)p_ub, 0, TPNOREPLY))
        {
            NDRX_LOG(log_error, "TESTERROR: failed to call TEST2_1ST_AL with TPNOREPLY: %s", 
                    tpstrerror(tperrno));
            
            EXFAIL_OUT(ret);
        }
    }

    /* sync off */
    Bdel(p_ub, T_STRING_10_FLD, 0);
    if (EXFAIL == tpcall("TEST2_1ST_AL", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,TPNOTIME))
    {
        NDRX_LOG(log_error, "TEST2_1ST_AL failed: %s",
               tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }

out:
    if (NULL!=p_ub)
    {
        tpfree((char *)p_ub);
    }
    return ret;
}

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
    
    /* test Bug #570 */
    if (EXSUCCEED!=tpacall_tpnoreply_forward_test())
    {
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=tpacall_tpnoreply_forward_nodestsrv())
    {
        EXFAIL_OUT(ret);
    }

out:
    if (NULL!=p_ub)
    {
        tpfree((char *)p_ub);
    }
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
