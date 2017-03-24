/* 
** XA transaction processing, tests, test case for #105
**
** @file atmiclt21_105.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, ATR Baltic, SIA. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or ATR Baltic's license for commercial use.
** -----------------------------------------------------------------------------
** GPL license:
** 
** This program is free software; you can redistribute it and/or modify it under
** the terms of the GNU General Public License as published by the Free Software
** Foundation; either version 2 of the License, or (at your option) any later
** version.
**
** This program is distributed in the hope that it will be useful, but WITHOUT ANY
** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
** PARTICULAR PURPOSE. See the GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License along with
** this program; if not, write to the Free Software Foundation, Inc., 59 Temple
** Place, Suite 330, Boston, MA 02111-1307 USA
**
** -----------------------------------------------------------------------------
** A commercial use license is available from ATR Baltic, SIA
** contact@atrbaltic.com
** -----------------------------------------------------------------------------
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
#include <ntimer.h>
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
    int ret=SUCCEED;
    
    if (SUCCEED!=tpopen())
    {
        NDRX_LOG(log_error, "TESTERROR: tpopen() fail: %d:[%s]", 
                                            tperrno, tpstrerror(tperrno));
        ret=FAIL;
        goto out;
    }
    
    /***************************************************************************/
    NDRX_LOG(log_debug, "Testing Bug #105");
    /***************************************************************************/

    if (SUCCEED!=tpbegin(5, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: tpbegin() fail: %d:[%s]", 
                                            tperrno, tpstrerror(tperrno));
        ret=FAIL;
        goto out;
    }

    Bchg(p_ub, T_STRING_FLD, 0, "TEST HELLO WORLD COMMIT", 0L);

    /* Call Svc1 */
    if (FAIL == (ret=tpcall("RUNTX", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0)))
    {
        NDRX_LOG(log_error, "TX3SVC failed: %s", tpstrerror(tperrno));
        ret=FAIL;
        goto out;
    }

    if (SUCCEED==tpcommit(0))
    {
        NDRX_LOG(log_error, "TESTERROR: tpcommit()==%d fail: %d:[%s]", 
                                            ret, tperrno, tpstrerror(tperrno));
        ret=FAIL;
        goto out;
    }
    
    if (TPETIME!=tperrno)
    {
        NDRX_LOG(log_error, "TESTERROR: Expected TPETIME, Got tpcommit()==%d fail: %d:[%s]", 
                                            ret, tperrno, tpstrerror(tperrno));
        
        ret=FAIL;
        goto out;
    }
   
    /***************************************************************************/
    NDRX_LOG(log_debug, "Done...");
    /***************************************************************************/
    
    if (SUCCEED!=tpclose())
    {
        NDRX_LOG(log_error, "TESTERROR: tpclose() fail: %d:[%s]", 
                                            tperrno, tpstrerror(tperrno));
        ret=FAIL;
        goto out;
    }
    
out:
    if (SUCCEED!=ret)
    {
        /* atleast try... */
        if (SUCCEED!=tpabort(0))
        {
            NDRX_LOG(log_error, "TESTERROR: tpabort() fail: %d:[%s]", 
                                                tperrno, tpstrerror(tperrno));
        }
        tpclose();
    }

    tpterm();
    
    NDRX_LOG(log_error, "Existing with %d", ret);
            
    return ret;
}

