/* 
** Test the tpconvert() function
**
** @file atmiclt21_tpconvert.c
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
    int ret=EXSUCCEED;
    TPTRANID t;
    char cnvstr[TPCONVMAXSTR];
    char cnvbin[TPCONVMAXSTR];
    
    if (EXSUCCEED!=tpopen())
    {
        NDRX_LOG(log_error, "TESTERROR: tpopen() fail: %d:[%s]", 
                                            tperrno, tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    /***************************************************************************/
    NDRX_LOG(log_debug, "Testing tpconvert()");
    /***************************************************************************/

    if (EXSUCCEED!=tpbegin(5, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: tpbegin() fail: %d:[%s]", 
                                            tperrno, tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    /*
     * Call service
     * - Service shall read the TPSVCINFO.cltid
     * - convert to string, must be equal to TPSVCINFO.cltid
     * - convert to binary, must be equal to TPSVCINFO.cltid
     * ... because we have always in string format
     */
    if (EXSUCCEED!=tpcall("TESTCLTID", NULL, 0L, (char **)&p_ub, &rsplen, TPNOTRAN))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to call TESTCLTID: %s - "
                "client id convert failed!", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    /* Testing of TID:
     * - suspend transaction
     * - convert transaction id to string, perform strpcy()
     * - convert id from string to bin
     * - resume transaction with convert bin tid
     */
    if (EXSUCCEED!=tpsuspend(&t, 0L))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to suspend transaction: %s!",
                tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    cnvstr[0] = EXEOS;
    cnvbin[0] = EXEOS;
    
    if (EXSUCCEED!=tpconvert(cnvstr, (char *)&t, TPCONVTRANID | TPTOSTRING))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to tpconvert() "
                "TPCONVTRANID | TPTOSTRING: %s!", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    /* convert back to bin */
    NDRX_LOG(log_debug, "Convert TPCONVTRANID from string to bin: [%s]",
            cnvstr);
    
    if (EXSUCCEED!=tpconvert(cnvstr, cnvbin, TPCONVTRANID))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to tpconvert() "
                "TPCONVTRANID from string: %s!",
                tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    /* resume transaction with convert bin */
    if (EXSUCCEED!=tpresume((TPTRANID *)cnvbin, 0L))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to resume transaction: %s!",
                tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    /*
     * Testing on XID:
     * - get value from TPTRANID. This is string value, convert to BIN
     * - BIN must not be equal to string
     * - convert BIN to back to string
     * - compare with TPTRANID, must be equal 
     */
    
    cnvstr[0] = EXEOS;
    cnvbin[0] = EXEOS;        
    
    /* t.tmxid is string,  convert to binary */
    if (EXSUCCEED!=tpconvert(t.tmxid, cnvbin, TPCONVXID))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to tpconvert() "
                "TPCONVXID from string: %s!",
                tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    /* thy must not match */
    if (0==strcmp(t.tmxid, cnvbin))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to tpconvert() TPCONVXID "
                "binary equal string [%s]!!!", cnvbin);
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=tpconvert(cnvstr, cnvbin, TPCONVXID | TPTOSTRING))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to tpconvert() TPCONVXID | "
                "TPTOSTRING failed: %s!", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    if (0!=strcmp(t.tmxid, cnvstr))
    {
        NDRX_LOG(log_error, "TESTERROR: TPCONVXID strings must match!!! "
                "tmxid: [%s] vs cnvstr [%s]", t.tmxid, cnvstr);
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=tpcommit(0))
    {
        NDRX_LOG(log_error, "TESTERROR: tpcommit()==%d fail: %d:[%s]", 
                                            ret, tperrno, tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
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
    
    NDRX_LOG(log_error, "Exiting with %d", ret);
            
    return ret;
}
