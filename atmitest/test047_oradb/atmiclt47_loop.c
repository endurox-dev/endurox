/**
 * @brief loop test for RAC failover/RECON testing
 *
 * @file atmiclt47_loop.c
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

    if (EXFAIL == tpcall("BALANCE", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,TPNOABORT))
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
    char *rsp=NULL;
    char tmp[51];
    long bal;
    int ret=EXSUCCEED;
    int upper1=100, upper2=300;
    char *p;
    ndrx_stopwatch_t w_loop;
    ndrx_stopwatch_t w_rsp;
    long max_rsp=-1;

    if (NULL!=(p=getenv(CONF_NDRX_XA_FLAGS)) && NULL!=strstr(p, "BTIGHT"))
    {
        upper1=5;
        upper2=205;
    }
    
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
    
    ndrx_stopwatch_reset(&w_loop);
    ndrx_stopwatch_reset(&w_rsp);
    
    /* run for 13 min */
    while (1)
    {
again:
        /* terminate our loop... */
        if (ndrx_stopwatch_get_delta_sec(&w_loop) >= 800)
        {
            break;
        }
        /* if response is longer than than... go bad..*/
        if ((l=ndrx_stopwatch_get_delta_sec(&w_rsp)) > MAX_RSP)
        {
            NDRX_LOG(log_error, "TESTERROR: Expected OK response in %d sec got %ld", MAX_RSP, l);
            EXFAIL_OUT(ret);
        }
        
        /* delete accounts no global transactional */
        while (EXFAIL == tpcall("ACCCLEAN", NULL, 0L, &rsp, &rsplen,0))
        {
            NDRX_LOG(log_error, "ACCCLEAN failed: %s", tpstrerror(tperrno));
            sleep(1);
        }

        while (EXSUCCEED != tpbegin(MAX_RSP, 0))
        {
            NDRX_LOG(log_error, "tpbegin failed: %s", tpstrerror(tperrno));
            sleep(1);
        }

        /* Check multi commit... */
        for (l=200; l<upper2; l++)
        {
            snprintf(tmp, sizeof(tmp), "ACC%03ld", l);
        
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
            if (EXFAIL == tpcall("ACCOPEN", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0))
            {
                NDRX_LOG(log_error, "ACCOPEN failed: %s", tpstrerror(tperrno));
                /* abort the transaction, or mark for rollback... */
                if (EXSUCCEED!=tpabort(0))
                {
                    NDRX_LOG(log_error, "tpabort failed: %s", tpstrerror(tperrno));
                }
                /* restart the test.. */
                goto again;
            }
            /* we shall have some positive response! */
            ndrx_stopwatch_reset(&w_rsp);
        }
    
        /* at this point transaction must be marked for rollback */
        if (EXSUCCEED != tpcommit(0))
        {
            NDRX_LOG(log_error, "tpcommit failed: %s", tpstrerror(tperrno));
            tpabort(0);
        }
    }

    
out:

    /* if response is longer than than... go bad..*/
    if ((l=ndrx_stopwatch_get_delta_sec(&w_rsp)) > MAX_RSP)
    {
        NDRX_LOG(log_error, "TESTERROR: Expected OK response in %d sec got %ld", MAX_RSP, l);
        ret=EXFAIL;
    }

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
    
    /* return an error marking */
    if (EXSUCCEED!=ret)
    {
        NDRX_LOG(log_error, "TESTERROR: Exit with %d", ret);
    }
    
    fprintf(stderr, "Exit with %d\n", ret);

    return ret;
}
/* vim: set ts=4 sw=4 et smartindent: */
