/* 
 * @brief xa_start command fails. This could be problems with connections to DB.
 *   thus if RECON flags are set, it must succeed. If flags are not set, the
 *   test case must fail.
 *
 * @file atmiclt21-startfail.c
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
#include <unistd.h>

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

    UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, 9216);
    long rsplen;
    int i=0;
    int ret=EXSUCCEED;
    int nr_fails = 0;
    if (EXSUCCEED!=tpopen())
    {
        NDRX_LOG(log_error, "TESTERROR: tpopen() fail: %d:[%s]", 
                                            tperrno, tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }
    
    /***************************************************************************/
    NDRX_LOG(log_debug, "Testing Bug #160 - startfails");
    /***************************************************************************/

    for (i=0; i<1000; i++)
    {
        ret=tpbegin(20, 0);
        
        if (argc>1) /* we have "fail" flag */
        {
            if (EXSUCCEED!=ret)
            {
                nr_fails++;
            }
            else
            {
                /* abort tran... do not want whole loop */
                tpabort(0L);
            }
            ret = EXSUCCEED;
            continue; /* ok to fail */
        }
        else if (EXSUCCEED!=ret)
        {
            NDRX_LOG(log_error, "TESTERROR: tpbegin() fail: %d:[%s]", 
                                                tperrno, tpstrerror(tperrno));
            ret=EXFAIL;
            goto out;
        }
        
        Bchg(p_ub, T_STRING_FLD, 0, "TEST HELLO WORLD COMMIT", 0L);

        /* Call Svc1 */
        if (EXFAIL == (ret=tpcall("RUNTX", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0)))
        {
            NDRX_LOG(log_error, "TX3SVC failed: %s", tpstrerror(tperrno));
            ret=EXFAIL;
            goto out;
        }

        if (EXSUCCEED!=tpcommit(0))
        {
            NDRX_LOG(log_error, "TESTERROR: tpcommit()==%d fail: %d:[%s]", 
                                        ret, tperrno, tpstrerror(tperrno));
            ret=EXFAIL;
            goto out;
        }
    }
   
    /***************************************************************************/
    NDRX_LOG(log_debug, "Done...");
    /***************************************************************************/
    
    if (EXSUCCEED!=tpclose())
    {
        NDRX_LOG(log_error, "tpclose() fail: %d:[%s]", 
                                    tperrno, tpstrerror(tperrno));
    }
    
out:

    if (argc>1 && nr_fails <= 100)
    {
        NDRX_LOG(log_error, "TESTERROR! flags not present and nr_fails %d must be > 100!",
                nr_fails);
        ret=EXFAIL;
    }
                
    if (EXSUCCEED!=ret)
    {
        /* atleast try... */
        if (EXSUCCEED!=tpabort(0))
        {
            NDRX_LOG(log_error, "TESTERROR: tpabort() fail: %d:[%s]", 
                                                tperrno, tpstrerror(tperrno));
        }
        tpclose();
    }

    tpterm();
    
    NDRX_LOG(log_error, "Exiting with %d", ret);
            
    return ret;
}

