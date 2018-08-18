/**
 * @brief XA transaction processing, cli tests (listing & commit & abort)
 *
 * @file atmiclt21-cli.c
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
 * Just open the transaction, and let do shell testing later
 */
int main(int argc, char** argv) {

    UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, 9216);
    long rsplen;
    int i=0;
    int ret=EXSUCCEED;
    TPTRANID tranid;
    
    if (EXSUCCEED!=tpopen())
    {
        NDRX_LOG(log_error, "TESTERROR: tpopen() fail: %d:[%s]", 
                                            tperrno, tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }
    
    /***************************************************************************/
    NDRX_LOG(log_debug, "Just opening a transactions (later will process cli by shell)");
    /***************************************************************************/
    for (i=0; i<100; i++)
    {

        if (EXSUCCEED!=tpbegin(60, 0))
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

        /* Suspend off from tran... */
        tpsuspend (&tranid, 0);

    }
    
out:
    /* So for some reason we do not  */
    tpclose();

    tpterm();
    
    NDRX_LOG(log_error, "Exiting with %d", ret);
            
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
