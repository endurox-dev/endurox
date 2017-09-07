/* 
** XA transaction processing, tests
**
** @file atmiclt21.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, Mavimax, Ltd. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or Mavimax's license for commercial use.
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
** A commercial use license is available from Mavimax, Ltd
** contact@mavimax.com
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
    
    if (EXSUCCEED!=tpopen())
    {
        NDRX_LOG(log_error, "TESTERROR: tpopen() fail: %d:[%s]", 
                                            tperrno, tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }
    
    /***************************************************************************/
    NDRX_LOG(log_debug, "Testing normal tx processing - commit() ...");
    /***************************************************************************/
    for (i=0; i<100; i++)
    {

        if (EXSUCCEED!=tpbegin(5, 0))
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

        if (EXSUCCEED!=(ret=tpcommit(0)))
        {
            NDRX_LOG(log_error, "TESTERROR: tpcommit()==%d fail: %d:[%s]", 
                                                ret, tperrno, tpstrerror(tperrno));
            ret=EXFAIL;
            goto out;
        }
    }
    
    /***************************************************************************/
    NDRX_LOG(log_debug, "Testing normal tx processing - abort() ...");
    /***************************************************************************/
    for (i=0; i<100; i++)
    {
        if (EXSUCCEED!=tpbegin(5, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: tpbegin() fail: %d:[%s]", 
                                                tperrno, tpstrerror(tperrno));
            ret=EXFAIL;
            goto out;
        }

        Bchg(p_ub, T_STRING_FLD, 0, "TEST HELLO WORLD ABORT", 0L);

        /* Call Svc1 */
        if (EXFAIL == (ret=tpcall("RUNTX", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0)))
        {
            NDRX_LOG(log_error, "TX3SVC failed: %s", tpstrerror(tperrno));
            ret=EXFAIL;
            goto out;
        }

        if (EXSUCCEED!=(ret=tpabort(0)))
        {
            NDRX_LOG(log_error, "TESTERROR: tpabort()==%d fail: %d:[%s]", 
                                                ret, tperrno, tpstrerror(tperrno));
            ret=EXFAIL;
            goto out;
        }
    }
    
    /***************************************************************************/
    NDRX_LOG(log_debug, "Service returns fail");
    /***************************************************************************/
    for (i=0; i<100; i++)
    {
        if (EXSUCCEED!=tpbegin(5, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: tpbegin() fail: %d:[%s]", 
                                                tperrno, tpstrerror(tperrno));
            ret=EXFAIL;
            goto out;
        }

        Bchg(p_ub, T_STRING_FLD, 0, "TEST HELLO WORLD SVCFAIL", 0L);

        /* Call Svc1 */
        if (EXFAIL != (ret=tpcall("RUNTXFAIL", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0)))
        {
            NDRX_LOG(log_error, "RUNTXFAIL should return fail!");
        }

        ret=tpcommit(0);
        
        if (EXSUCCEED==ret || tperrno!=TPEABORT)
        {
            NDRX_LOG(log_error, "TESTERROR: abort()==%d fail: %d:[%s] - must be TPEABORT!", 
                                                ret, tperrno, tpstrerror(tperrno));
            ret=EXFAIL;
            goto out;
        }
        
        ret = EXSUCCEED;
    }    
    /***************************************************************************/
    NDRX_LOG(log_debug, "Transaction time-out (by tmsrv)");
    /***************************************************************************/
    for (i=0; i<5; i++)
    {
        if (EXSUCCEED!=tpbegin(2, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: tpbegin() fail: %d:[%s]", 
                                                tperrno, tpstrerror(tperrno));
            ret=EXFAIL;
            goto out;
        }

        Bchg(p_ub, T_STRING_FLD, 0, "TEST HELLO WORLD TOUT", 0L);

        /* Call Svc1 */
        if (EXFAIL == (ret=tpcall("RUNTX", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0)))
        {
            NDRX_LOG(log_error, "TESTERROR: RUNTX should not return fail!");
        }
        
        sleep(10);

        ret=tpcommit(0);
        
        if (EXSUCCEED==ret || tperrno!=TPEABORT)
        {
            NDRX_LOG(log_error, "TESTERROR: tpcommit()==%d fail: %d:[%s] - must be TPEABORT!", 
                                                ret, tperrno, tpstrerror(tperrno));
            ret=EXFAIL;
            goto out;
        }
        
        ret = EXSUCCEED;
    }    

    /***************************************************************************/
    NDRX_LOG(log_debug, "Call service, but not in tran mode - transaction must not be aborted!");
    /***************************************************************************/
    for (i=0; i<100; i++)
    {
        if (EXSUCCEED!=tpbegin(5, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: tpbegin() fail: %d:[%s]", 
                                                tperrno, tpstrerror(tperrno));
            ret=EXFAIL;
            goto out;
        }

        Bchg(p_ub, T_STRING_FLD, 0, "TEST HELLO WORLD SVCFAIL", 0L);

        /* Call Svc1 */
        if (EXFAIL != (ret=tpcall("NOTRANFAIL", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,TPNOTRAN)))
        {
            NDRX_LOG(log_error, "TESTERROR: NOTRANFAIL should return fail!");
        }

        ret=tpcommit(0);
        
        if (EXSUCCEED!=ret)
        {
            NDRX_LOG(log_error, "TESTERROR: tpcommit()==%d fail: %d:[%s] - must be SUCCEED!", 
                                                ret, tperrno, tpstrerror(tperrno));
            ret=EXFAIL;
            goto out;
        }
        
        ret = EXSUCCEED;
    }    

    /***************************************************************************/
    NDRX_LOG(log_debug, "Done...");
    /***************************************************************************/
    
    if (EXSUCCEED!=tpclose())
    {
        NDRX_LOG(log_error, "TESTERROR: tpclose() fail: %d:[%s]", 
                                            tperrno, tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }
    
out:
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
    
    NDRX_LOG(log_error, "Existing with %d", ret);
            
    return ret;
}

