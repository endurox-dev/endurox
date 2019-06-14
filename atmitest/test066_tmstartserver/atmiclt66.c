/**
 * @brief Test for _tmstartserver() - client
 *
 * @file atmiclt66.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * AGPL or Mavimax's license for commercial use.
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
#include "test66.h"
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
    int i;
    int ret=EXSUCCEED;
    char *p;
    
    if (EXFAIL==CBchg(p_ub, T_STRING_FLD, 0, VALUE_EXPECTED, 0, BFLD_STRING))
    {
        NDRX_LOG(log_debug, "Failed to set T_STRING_FLD[0]: %s", Bstrerror(Berror));
        ret=EXFAIL;
        goto out;
    }    

    if (EXFAIL == tpcall("TESTSV", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0))
    {
        NDRX_LOG(log_error, "TESTSV failed: %s", tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }
    
    if (EXFAIL == tpcall("SYSADV1", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0))
    {
        NDRX_LOG(log_error, "SYSADV1 failed: %s", tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }
    
    if (EXFAIL==CBchg(p_ub, T_STRING_FLD, 0, VALUE_EXPECTED_2, 0, BFLD_STRING))
    {
        NDRX_LOG(log_debug, "Failed to set T_STRING_FLD[0]: %s", Bstrerror(Berror));
        ret=EXFAIL;
        goto out;
    }    

    if (EXFAIL == tpcall("SYSADV2", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0))
    {
        NDRX_LOG(log_error, "SYSADV2 failed: %s", tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }
    
    
    if (EXFAIL == tpcall("NEWS2", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0))
    {
        NDRX_LOG(log_error, "SYSADV2 failed: %s", tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }
    
    if (0!=strcmp((p=Bfind(p_ub, T_STRING_FLD, 0, 0L)), "somefunc"))
    {
        NDRX_LOG(log_error, "sumefunc failed: got [%s]", p);
        EXFAIL_OUT(ret);
    }
    
    /* ensure that NEWSVC is missing */
    if (EXSUCCEED == tpcall("NEWSVC", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0))
    {
        NDRX_LOG(log_error, "NEWSVC CALL OK! But must be TPENOENT");
        ret=EXFAIL;
        goto out;
    }
    
    if (TPENOENT!=tperrno)
    {
        NDRX_LOG(log_error, "Expected %d but got %d", TPENOENT, tperrno);
        ret=EXFAIL;
        goto out;   
    }
    
out:
    tpterm();
    fprintf(stderr, "Exit with %d\n", ret);

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */

