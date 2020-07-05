/**
 * @brief Mixed tpcall with tpacall - client
 *
 * @file atmiclt78.c
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
#include <nstopwatch.h>
#include <fcntl.h>
#include <unistd.h>
#include <nstdutil.h>
#include <exassert.h>
#include "test78.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Do the test call to the server
 */
int main(int argc, char** argv)
{
    UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, 56000);
    UBFH *p_ub2 = (UBFH *)tpalloc("UBF", NULL, 56000);
    long rsplen;
    int i;
    int cd;
    long test_num=0;
    long test_num_sync, test_num_async, l;
    int ret=EXSUCCEED;
    
    for (i=0; i<100000; i++)
    {
        /* do the acall */
        test_num++;
        test_num_async = test_num;
        
        NDRX_ASSERT_UBF_OUT((EXSUCCEED==Bchg(p_ub, T_LONG_FLD, 0, (char *)&test_num_async, 0L)), 
                "Failed to set T_LONG_FLD to p_ub");
        
        NDRX_ASSERT_TP_OUT((EXFAIL != tpacall("TESTSV", (char *)p_ub, 0L, 0L)), 
                "Failed to tpacall TESTSV");
        
        test_num++;
        test_num_sync = test_num;
        NDRX_ASSERT_UBF_OUT((EXSUCCEED==Bchg(p_ub2, T_LONG_FLD, 0, (char *)&test_num_sync, 0L)), 
                "Failed to set T_LONG_FLD to p_ub2");
        
        /* now to the sync call */
        NDRX_ASSERT_TP_OUT((EXFAIL != tpcall("TESTSV", (char *)p_ub2, 0L, (char **)&p_ub2, &rsplen,0)), 
                "Failed to tpcall TESTSV");
        
        /* get echo field, */
        NDRX_ASSERT_UBF_OUT((EXSUCCEED==Bget(p_ub2, T_LONG_2_FLD, 0, (char *)&l, 0L)), 
                "Failed to set T_LONG_FLD to p_ub2");
        
        /* shall match the reply */
        NDRX_ASSERT_VAL_OUT((l==test_num_sync), "Reply does not match (1): %ld vs %ld", 
                l, test_num_sync);
        
        /* read the reply... */
        NDRX_ASSERT_TP_OUT((EXFAIL != tpgetrply(&cd, (char **)&p_ub, &rsplen, TPGETANY)), 
                "Failed tpgetrply failed");
        
        /* get reply value */
        NDRX_ASSERT_UBF_OUT((EXSUCCEED==Bget(p_ub, T_LONG_2_FLD, 0, (char *)&l, 0L)), 
                "Failed to set T_LONG_FLD to p_ub2");
        
        /* match the response */
        NDRX_ASSERT_VAL_OUT((l==test_num_async), "Reply does not match (2): %ld vs %ld",
                l, test_num_async);
    }
    
out:
    tpterm();
    fprintf(stderr, "Exit with %d\n", ret);

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
