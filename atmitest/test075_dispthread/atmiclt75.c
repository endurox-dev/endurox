/**
 * @brief Test Enduro/X server dispatch threading - client
 *
 * @file atmiclt75.c
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
#include "test75.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define TEST_THREADS            6   /**< number of threads used         */
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
    long rsplen;
    int i, j;
    int ret=EXSUCCEED;
    int cd[TEST_THREADS];
    short cd_rsp[TEST_THREADS];
    
    for (i=0; i<10; i++)
    {
        UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, 56000);
        
        for (j=0; j<TEST_THREADS; j++)
        {
            if (EXFAIL==CBchg(p_ub, T_STRING_FLD, 0, VALUE_EXPECTED, 0, BFLD_STRING))
            {
                NDRX_LOG(log_debug, "Failed to set T_STRING_FLD[0]: %s", Bstrerror(Berror));
                EXFAIL_OUT(ret);
            }    

            if (EXFAIL == (cd[j]=tpacall("TESTSV", (char *)p_ub, 0L, 0)))
            {
                NDRX_LOG(log_error, "TESTSV %d %d failed: %s", i, j, tpstrerror(tperrno));
                EXFAIL_OUT(ret);
            }
        }
        
        memset(cd_rsp, 0, sizeof(cd_rsp));
        for (j=0; j<TEST_THREADS; j++)
        {
            short tmp;
            if (EXFAIL==tpgetrply(&cd[j], (char **)&p_ub, &rsplen, 0))
            {
                NDRX_LOG(log_error, "tpgetrply %d %d failed: %s", i, j, 
                        tpstrerror(tperrno));
                EXFAIL_OUT(ret);
            }
            
            /* get the reply thread id... */
            if (EXSUCCEED!=Bget(p_ub, T_SHORT_FLD, 0, (char *)&tmp, 0))
            {
                NDRX_LOG(log_error, "failed to get T_SHORT_FLD %d %d: %s", i, j, 
                        Bstrerror(Berror));
                EXFAIL_OUT(ret);
            }
            
            cd_rsp[tmp]++;
        }
        
        /* check that very response is unique */
        for (j=0; j<TEST_THREADS; j++)
        {
            if (1!=cd_rsp[j])
            {
                NDRX_LOG(log_error, "Slot %d invalid rsp %hd", j, cd_rsp[j]);
                EXFAIL_OUT(ret);
            }
        }
        
        
        tpfree((char *)p_ub);
    }
    
out:
    tpterm();
    fprintf(stderr, "Exit with %d\n", ret);

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
