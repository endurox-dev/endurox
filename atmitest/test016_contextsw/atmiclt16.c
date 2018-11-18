/**
 * @brief Test the full Enduro/X multi-contexting.
 *
 * @file atmiclt16.c
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

#include <unistd.h>     /* Symbolic Constants */
#include <sys/types.h>  /* Primitive System Data Types */ 
#include <errno.h>      /* Errors */
#include <stdio.h>      /* Input/Output */
#include <stdlib.h>     /* General Utilities */
#include <pthread.h>    /* POSIX Threads */

#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>
#include <test.fd.h>
#include <ndrstandard.h>
#include <nstopwatch.h>
#include <nstdutil.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/    
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

TPCONTEXT_T M_ctx[2];

/*
 * Thread function for doing ATMI tests...
 */
void do_thread_work ( void *ptr )
{
    UBFH *p_ub;
    long slot = (long)ptr;
    long rsplen;
    int ret=EXSUCCEED;
    TPCONTEXT_T test_ctx;
    
    if (EXSUCCEED!=tpsetctxt(M_ctx[slot], 0))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to tpsetctxt(): %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    if (NULL==(p_ub = (UBFH *)tpalloc("UBF", NULL, 9216)))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to tpalloc(): %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    if (EXFAIL == tpcall("TESTSV", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0))
    {
        NDRX_LOG(log_error, "TESTERROR: TESTSV failed: %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
out:
    
    if (NULL!=p_ub)
    {
        tpfree((char *)p_ub);
    }

    /* disassoc current thread from context */
    if (TPMULTICONTEXTS!=tpgetctxt(&M_ctx[slot], 0))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to tpgetctxt(): %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }

    /* it must be NULL context afterwards */
    if (TPNULLCONTEXT!=(ret=tpgetctxt(&test_ctx, 0)))
    {
        NDRX_LOG(log_error, "TESTERROR: tpgetctxt() must be in NULLCONTEXT, "
                "but got: %d", ret);
        EXFAIL_OUT(ret);
    }
    
    return;
}

/*
 * Do the test call to the server
 */
int main(int argc, char** argv) 
{
    int i;
    pthread_t thread1;
    int ret = EXSUCCEED;
    
    for (i =0; i< 2; i++)
    {
        if (EXSUCCEED!=tpinit(NULL))
        {
            NDRX_LOG(log_error, "TESTERROR: tpinit fail: %s", tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }

        /* get the first context */
        if (TPMULTICONTEXTS!=tpgetctxt(&M_ctx[i], 0))
        {
            NDRX_LOG(log_error, "TESTERROR: tpgetctxt fail: %s", tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
    }
    
    for (i=0; i<1000; i++)
    {
        int slot = i%2;
        pthread_create (&thread1, NULL, (void *) &do_thread_work, (void *)((long)slot));
        pthread_join(thread1, NULL);
    }
    
    
out:
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
