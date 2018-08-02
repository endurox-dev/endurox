/* 
 * @brief Test server connection to Oracle DB from C - client
 *
 * @file atmiclt47.c
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
#include <fcntl.h>
#include <unistd.h>
#include <nstdutil.h>
#include "test47.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/


/**
 * Check the balance
 * @param accnum account number to return balance
 * @return on error -1, 0 on succeed.
 */
int check_balance(char *accnum, long *balance)
{
    int ret=EXSUCCEED;
    UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, 56000);
    long rsplen;
    
    if (EXFAIL==CBchg(p_ub, T_STRING_FLD, 0, accnum, 0, BFLD_STRING))
    {
        NDRX_LOG(log_debug, "Failed to set T_STRING_FLD[0]: %s", Bstrerror(Berror));
        ret=EXFAIL;
        goto out;
    }

    if (EXFAIL == tpcall("BALANCE", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0))
    {
        NDRX_LOG(log_error, "BALANCE failed: %s", tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }
    
    if (EXFAIL==Bget(p_ub, T_LONG_FLD, 0, (char *)balance, 0))
    {
        NDRX_LOG(log_debug, "Failed to set T_LONG_FLD[0]: %s", Bstrerror(Berror));
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
    UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, 56000);
    long rsplen;
    long l;
    char *rsp;
    char tmp[51];
    long bal;
    int ret=EXSUCCEED;
    
    if (EXSUCCEED!=tpinit(NULL))
    {
        NDRX_LOG(log_debug, "Failed to init!: %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    /* will work with transactions */
    if (EXSUCCEED!=tpopen())
    {
        NDRX_LOG(log_debug, "Failed to tpopen!: %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    /* delete accounts no global transactional */
    if (EXFAIL == tpcall("ACCCLEAN", NULL, 0L, &rsp, &rsplen,0))
    {
        NDRX_LOG(log_error, "ACCCLEAN failed: %s", tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }
    
    for (l=0; l<100; l++)
    {
        
        
        snprintf(tmp, sizeof(tmp), "ACC%03ld", l);
        
        /* make an account, but abort... */
        
        if (EXSUCCEED != tpbegin(60, 0))
        {
            NDRX_LOG(log_error, "tpbegin failed: %s", tpstrerror(tperrno));
            ret=EXFAIL;
            goto out;
        }
        
        if (EXFAIL==CBchg(p_ub, T_STRING_FLD, 0, tmp, 0, BFLD_STRING))
        {
            NDRX_LOG(log_debug, "Failed to set T_STRING_FLD[0]: %s", Bstrerror(Berror));
            ret=EXFAIL;
            goto out;
        }    
        
        if (EXFAIL==CBchg(p_ub, T_LONG_FLD, 0, (char *)&l, 0, BFLD_LONG))
        {
            NDRX_LOG(log_debug, "Failed to set T_LONG_FLD[0]: %s", Bstrerror(Berror));
            ret=EXFAIL;
            goto out;
        }    
        
        /* TPTRANSUSPEND must be present, otherwise other RMID must be defined
         * as two binaries cannot operate on the same transaction
         */
        if (EXFAIL == tpcall("ACCOPEN", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,TPTRANSUSPEND))
        {
            NDRX_LOG(log_error, "ACCOPEN failed: %s", tpstrerror(tperrno));
            ret=EXFAIL;
            goto out;
        }
        
        if (EXSUCCEED != tpabort(0))
        {
            NDRX_LOG(log_error, "tpabort failed: %s", tpstrerror(tperrno));
            ret=EXFAIL;
            goto out;
        }
        
        if (EXSUCCEED==check_balance(tmp, &bal))
        {
            NDRX_LOG(log_error, "Account [%s] found after abort! error!", tmp);
            ret=EXFAIL;
            goto out;
        }
        
        /* start new tran */
        if (EXSUCCEED != tpbegin(60, 0))
        {
            NDRX_LOG(log_error, "tpbegin failed: %s", tpstrerror(tperrno));
            ret=EXFAIL;
            goto out;
        }
        
        if (EXFAIL == tpcall("ACCOPEN", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,TPTRANSUSPEND))
        {
            NDRX_LOG(log_error, "ACCOPEN failed: %s", tpstrerror(tperrno));
            ret=EXFAIL;
            goto out;
        }
        
        if (EXSUCCEED != tpcommit(0))
        {
            NDRX_LOG(log_error, "tpcommit failed: %s", tpstrerror(tperrno));
            ret=EXFAIL;
            goto out;
        }
        
        if (EXSUCCEED!=check_balance(tmp, &bal))
        {
            NDRX_LOG(log_error, "Account [%s] NOT found!", tmp);
            ret=EXFAIL;
            goto out;
        }
        
        if (bal!=l)
        {
            NDRX_LOG(log_error, "Invalid balance expected: %ld, but got: %ld", 
                    l, bal);
            ret=EXFAIL;
            goto out;
        }
    }
    
out:

    if (EXSUCCEED!=ret)
    {
        /* Abort anyway... */
        tpabort(0);
    }

    if (NULL!=p_ub)
    {
        tpfree((char *)p_ub);
    }

    tpclose();
    tpterm();
    fprintf(stderr, "Exit with %d\n", ret);

    return ret;
}
