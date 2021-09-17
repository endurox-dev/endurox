/**
 * @brief Test of tpsblktime(), tpgblktime()
 *
 * @file atmiclt51.c
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
#include "test51.h"
#include <exassert.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Validate timeout response for dead server
 * @param tout expected value
 * @return EXSUCCEED/EXFAIL
 */
int chk_tpcall(int tout)
{
    /* validate the tout response */
    ndrx_stopwatch_t w;
    UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, 56000);
    long rsplen, delta;
    int ret = EXSUCCEED;
    
    ndrx_stopwatch_reset(&w);
    
    if (EXSUCCEED == tpcall("SLEEPSV", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0))
    {
        NDRX_LOG(log_error, "TESTERROR!  Call succeed but must fail!");
        ret=EXFAIL;
        goto out;
    }
    
    /* the time shall be more or less as in boundries */
    delta = ndrx_stopwatch_get_delta_sec(&w);
    
    if (delta <tout-2 || delta > tout+2)
    {
        NDRX_LOG(log_error, "TESTERROR! Expected +-2 of %d got %ld",
                tout, delta);
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
 * Fill up the queue.
 * Active the block service
 * @param msgs number of messages to load
 * @return EXSUCCEED/EXFAIL
 */
int full_load(int msgs)
{
    int i;
    int ret = EXSUCCEED;
    
    for (i=0; i<msgs; i++)
    {
        if (EXSUCCEED!=tpacall("SLEEPSV", NULL, 0, TPNOREPLY))
        {
            EXFAIL_OUT(ret);
        }
    }
    
out:
    return ret;    
}

/**
 * Basic call tests
 * @return 
 */
int call_tests(void)
{
    UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, 56000);
    long rsplen;
    int cd;
    int ret=EXSUCCEED;
    int err;
    
    if (EXSUCCEED!=tpsblktime(1, TPBLK_NEXT))
    {
        NDRX_LOG(log_debug, "TESTERROR: Failed to set timeout to 1: %s", 
                tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    if (EXFAIL==CBchg(p_ub, T_STRING_FLD, 0, VALUE_EXPECTED, 0, BFLD_STRING))
    {
        NDRX_LOG(log_debug, "Failed to set T_STRING_FLD[0]: %s", Bstrerror(Berror));
        ret=EXFAIL;
        goto out;
    }    

    /* get standard timeout due to 1 sec... */
    if (EXSUCCEED == tpcall("TESTSV", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0))
    {
        NDRX_LOG(log_error, "TESTERROR!  Call succeed but must fail!");
        ret=EXFAIL;
        goto out;
    }
    
    err = tperrno;
    
    if (tperrno!=TPETIME)
    {
        NDRX_LOG(log_error, "TESTERROR! Expected TPETIME, but got: %d!", err);
        ret=EXFAIL;
        goto out;
    }

    /* invoke now with full wait first 2 sec + 3 sec, we get 6-7 sec 
     * thus receive normal response...
     */
    if (EXSUCCEED!=tpsblktime(6, TPBLK_NEXT))
    {
        NDRX_LOG(log_debug, "TESTERROR: Failed to set timeout to 6: %s", 
                tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }

    /* now wait fully for response, the first call shall be dropped.. */
    if (EXFAIL == tpcall("TESTSV", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0))
    {
        NDRX_LOG(log_error, "TESTERROR!  Second call failed: %s", 
                tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }
    
    /* also this will make assumtion that if we receive message
     * we do not check was it actually expired by previous timeout settings
     * this applies on waiting on reply queue and not actual msg age.
     * call age is checked by dest server.
     */ 
    if (EXSUCCEED!=tpsblktime(2, TPBLK_ALL))
    {
        NDRX_LOG(log_debug, "TESTERROR: Failed to set timeout to 2: %s", 
                tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }

    /* shall test the case when message sits in queue for too long by custom
     * timeout setting, thus it must be dropped, thus lets do three async calls
     * for atleast one we shall get the dropped message in server log.
     * We will receive only one message... as issued in the same time
     */
    
    if (EXFAIL == tpacall("TESTSV", (char *)p_ub, 0L, 0))
    {
        NDRX_LOG(log_error, "TESTERROR! tpacall failed (1): %s", 
                tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }
    
    if (EXFAIL == tpacall("TESTSV", (char *)p_ub, 0L, 0))
    {
        NDRX_LOG(log_error, "TESTERROR! tpacall failed (2): %s", 
                tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }
    
    if (EXFAIL == tpacall("TESTSV", (char *)p_ub, 0L, 0))
    {
        NDRX_LOG(log_error, "TESTERROR! tpacall failed (3): %s", 
                tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }

    /* wait 5 sec for each... */
    if (EXSUCCEED!=tpsblktime(5, TPBLK_ALL))
    {
        NDRX_LOG(log_debug, "TESTERROR: Failed to set timeout to 5: %s", 
                tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    /* wait for reply
     * this one shall come back, as first processed in 3 sec.
     * The others must be dropped due to expired.. as the server requires wait for 3 sec
     * but we marked packets to live only for 2 sec
     */
    if (EXSUCCEED!=tpgetrply(&cd, (char **)&p_ub, &rsplen, TPGETANY))
    {
        NDRX_LOG(log_error, "TESTERROR! tpgetrply (1) failed: %s", 
                tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }
    
    /*
    if (EXSUCCEED!=tptoutset(1))
    {
        NDRX_LOG(log_debug, "TESTERROR: Failed to set timeout to 1 (for tpgetrply): %s", 
                tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    */
    
    /* these shall be zapped by dest server due to expired */
    /* the other shall fail as total 4+4 > 4 tout setting */
    if (EXSUCCEED==tpgetrply(&cd, (char **)&p_ub, &rsplen, TPGETANY))
    {
        NDRX_LOG(log_error, "TESTERROR! tpgetrply (2) SUCCEED but must fail!");
        ret=EXFAIL;
        goto out;
    }
    
    err = tperrno;
    if (tperrno!=TPETIME)
    {
        NDRX_LOG(log_error, "TESTERROR! Expected TPETIME (2), but got: %d!", err);
        ret=EXFAIL;
        goto out;
    }
    
    if (EXSUCCEED==tpgetrply(&cd, (char **)&p_ub, &rsplen, TPGETANY))
    {
        NDRX_LOG(log_error, "TESTERROR! tpgetrply (3) SUCCEED but must fail!");
        ret=EXFAIL;
        goto out;
    }
    
    err = tperrno;
    if (tperrno!=TPETIME)
    {
        NDRX_LOG(log_error, "TESTERROR! Expected TPETIME (3), but got: %d!", err);
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
 * Do the test call to the server
 */
int main(int argc, char** argv)
{
    int ret=EXSUCCEED;
    int tret;
    TPCONTEXT_T context, ctxt2;

    NDRX_ASSERT_TP_OUT(EXSUCCEED==tpinit(NULL), "failed to init ctxt 1");
    NDRX_ASSERT_TP_OUT(EXSUCCEED==tptoutset(11), "Failed to set global timeout");
    
    /* read default values*/
    NDRX_ASSERT_TP_OUT(0==(tret=tpgblktime(TPBLK_ALL)), "Failed to get TPBLK_ALL %d", tret);
    NDRX_ASSERT_TP_OUT(0==(tret=tpgblktime(TPBLK_NEXT)), "Failed to get TPBLK_NEXT %d", tret);
    NDRX_ASSERT_TP_OUT(11==(tret=tpgblktime(0)), "Failed to get 0 blktime %d", tret);

    /* set + reset */
    NDRX_ASSERT_TP_OUT(EXSUCCEED==tpsblktime(3,TPBLK_ALL), "Failed to set TPBLK_ALL");
    NDRX_ASSERT_TP_OUT(EXSUCCEED==tpsblktime(4,TPBLK_NEXT), "Failed to set TPBLK_NEXT)");

    NDRX_ASSERT_TP_OUT(3==(tret=tpgblktime(TPBLK_ALL)), "Failed to get TPBLK_ALL %d", tret);
    NDRX_ASSERT_TP_OUT(4==(tret=tpgblktime(TPBLK_NEXT)), "Failed to get TPBLK_NEXT %d", tret);
   
    NDRX_ASSERT_TP_OUT(EXSUCCEED==tpsblktime(0,TPBLK_ALL), "Failed to set TPBLK_ALL");
    NDRX_ASSERT_TP_OUT(EXSUCCEED==tpsblktime(0,TPBLK_NEXT), "Failed to set TPBLK_NEXT)");

    NDRX_ASSERT_TP_OUT(0==(tret=tpgblktime(TPBLK_ALL)), "Failed to get TPBLK_ALL %d", tret);
    NDRX_ASSERT_TP_OUT(0==(tret=tpgblktime(TPBLK_NEXT)), "Failed to get TPBLK_NEXT %d", tret);
 
    
    /* set next call thread specific tout ... */
    NDRX_ASSERT_TP_OUT(EXSUCCEED==tpsblktime(3,TPBLK_ALL), "Failed to set TPBLK_ALL");
    NDRX_ASSERT_TP_OUT(EXSUCCEED==tpsblktime(4,TPBLK_NEXT), "Failed to set TPBLK_NEXT)");

    NDRX_ASSERT_TP_OUT(3==(tret=tpgblktime(TPBLK_ALL)), "Failed to get TPBLK_ALL %d", tret);
    NDRX_ASSERT_TP_OUT(4==(tret=tpgblktime(TPBLK_NEXT)), "Failed to get TPBLK_NEXT %d", tret);
    NDRX_ASSERT_TP_OUT(11==(tret=tpgblktime(0)), "Failed to get 0 blktime %d", tret);

    /* save ctx 1 */
    NDRX_ASSERT_TP_OUT(EXFAIL!=tpgetctxt(&context, 0), "Failed to get context");

    /* check new context 2 */
    NDRX_ASSERT_TP_OUT(EXSUCCEED==tpinit(NULL), "failed to init ctxt 2");
    NDRX_ASSERT_TP_OUT(0==(tret=tpgblktime(TPBLK_ALL)), "Failed to get TPBLK_ALL %d / ctxt2", tret);
    NDRX_ASSERT_TP_OUT(0==(tret=tpgblktime(TPBLK_NEXT)), "Failed to get TPBLK_NEXT %d / ctxt2", tret);
    NDRX_ASSERT_TP_OUT(11==(tret=tpgblktime(0)), "Failed to get 0 blktime %d / ctxt2", tret);
    NDRX_ASSERT_TP_OUT(EXSUCCEED==tpterm(), "failed to term ctxt 2");

    /* free ctxt 2*/
    NDRX_ASSERT_TP_OUT(EXFAIL!=tpgetctxt(&ctxt2, 0), "Failed to get context 2");
    tpfreectxt(ctxt2);

    /* restore org context */
    NDRX_ASSERT_TP_OUT(EXFAIL!=tpsetctxt(context, 0), "Failed to set context");
    NDRX_ASSERT_TP_OUT(3==(tret=tpgblktime(TPBLK_ALL)), "Failed to get TPBLK_ALL %d", tret);
    NDRX_ASSERT_TP_OUT(4==(tret=tpgblktime(TPBLK_NEXT)), "Failed to get TPBLK_NEXT %d", tret);
    NDRX_ASSERT_TP_OUT(11==(tret=tpgblktime(0)), "Failed to get 0 blktime %d", tret);

    /* validate errors: */
    NDRX_ASSERT_TP_OUT(EXFAIL==tpsblktime(-1,TPBLK_ALL) && TPEINVAL==tperrno, "Expected TPEINVAL");
    NDRX_ASSERT_TP_OUT(EXFAIL==tpsblktime(-1,9999) && TPEINVAL==tperrno, "Expected TPEINVAL");
    NDRX_ASSERT_TP_OUT(EXFAIL==tpgblktime(9999) && TPEINVAL==tperrno, "Expected TPEINVAL");

    

    NDRX_LOG(log_debug, "running of the case");
    
    
    NDRX_ASSERT_TP_OUT(EXSUCCEED==tpsblktime(99,TPBLK_ALL), "Failed to set TPBLK_ALL");
    NDRX_ASSERT_TP_OUT(EXSUCCEED==tpsblktime(99,TPBLK_NEXT), "Failed to set TPBLK_NEXT)");
    
            
    NDRX_ASSERT_VAL_OUT(EXSUCCEED==call_tests(), "Call tests failed");
    

    /* block the service */
    
    NDRX_ASSERT_VAL_OUT(EXSUCCEED==full_load(1), "Failed to active blocked service");
    
    NDRX_ASSERT_TP_OUT(EXSUCCEED==tpsblktime(5,TPBLK_ALL), "Failed to set TPBLK_ALL");
    NDRX_ASSERT_TP_OUT(EXSUCCEED==tpsblktime(2,TPBLK_NEXT), "Failed to set TPBLK_NEXT");
    NDRX_ASSERT_VAL_OUT(EXSUCCEED==chk_tpcall(2), "tout failed");
    /* next is reset */
    NDRX_ASSERT_VAL_OUT(EXSUCCEED==chk_tpcall(5), "tout failed");
   
    NDRX_ASSERT_TP_OUT(EXSUCCEED==tpsblktime(0,TPBLK_ALL), "Failed to reset TPBLK_ALL");
    NDRX_ASSERT_TP_OUT(EXSUCCEED==tptoutset(9), "Failed to default tout");
    NDRX_ASSERT_VAL_OUT(EXSUCCEED==chk_tpcall(9), "tout failed (default)");
   
    
#ifdef EX_USE_EPOLL
    
    /* block the dest service / we get full Q */
    while (EXSUCCEED==full_load(1)){};
    
    /* now lets test caller send timtouts */
    NDRX_ASSERT_TP_OUT(EXSUCCEED==tpsblktime(5,TPBLK_ALL), "Failed to set TPBLK_ALL");
    NDRX_ASSERT_TP_OUT(EXSUCCEED==tpsblktime(2,TPBLK_NEXT), "Failed to set TPBLK_NEXT");
    NDRX_ASSERT_VAL_OUT(EXSUCCEED==chk_tpcall(2), "tout failed");
    /* next is reset */
    NDRX_ASSERT_VAL_OUT(EXSUCCEED==chk_tpcall(5), "tout failed");
   
    NDRX_ASSERT_TP_OUT(EXSUCCEED==tpsblktime(0,TPBLK_ALL), "Failed to reset TPBLK_ALL");
    NDRX_ASSERT_TP_OUT(EXSUCCEED==tptoutset(9), "Failed to default tout");
    NDRX_ASSERT_VAL_OUT(EXSUCCEED==chk_tpcall(9), "tout failed (default)");
    
#endif
    
out:
    tpterm();
    fprintf(stderr, "Exit with %d\n", ret);

    return ret;
}
/* vim: set ts=4 sw=4 et smartindent: */
