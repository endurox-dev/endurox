/* 
 * @brief Test of tptoutset()/get() - client
 *
 * @file atmiclt51.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2018, Mavimax, Ltd. All Rights Reserved.
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
#include <fcntl.h>
#include <unistd.h>
#include <nstdutil.h>
#include "test51.h"
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
    long rsplen;
    int cd;
    int ret=EXSUCCEED;
    int err;

    if (EXSUCCEED!=tpinit(NULL))
    {
        NDRX_LOG(log_debug, "TESTERROR: Failed to init: %s", 
                tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_debug, "running of the case");
    
    
    if (EXSUCCEED!=tptoutset(1))
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
    
    /* invoke now with full wait */
    
    if (EXSUCCEED!=tptoutset(4))
    {
        NDRX_LOG(log_debug, "TESTERROR: Failed to set timeout to 5: %s", 
                tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }

    if (EXFAIL == tpcall("TESTSV", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0))
    {
        NDRX_LOG(log_error, "TESTERROR!  Second call failed: %s", 
                tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
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
    
    /* wait for reply */
    if (EXSUCCEED!=tpgetrply(&cd, (char **)&p_ub, &rsplen, TPGETANY))
    {
        NDRX_LOG(log_error, "TESTERROR! tpgetrply (1) failed: %s", 
                tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }
    
    if (EXSUCCEED!=tptoutset(1))
    {
        NDRX_LOG(log_debug, "TESTERROR: Failed to set timeout to 2 (for tpgetrply): %s", 
                tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
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
    tpterm();
    fprintf(stderr, "Exit with %d\n", ret);

    return ret;
}
