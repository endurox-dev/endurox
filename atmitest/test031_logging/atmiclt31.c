/* 
** Basic test client
**
** @file atmiclt31.c
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

#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>
#include <test.fd.h>
#include <ndrstandard.h>
#include <nstdutil.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/


void do_thread_work1 ( void *ptr )
{
    if (SUCCEED!=tplogconfig(LOG_FACILITY_TP_THREAD, FAIL, "file=./clt-tp-th1.log tp=5", "TEST", NULL))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to configure user TP logger: %s", 
                Nstrerror(Nerror));
    }
    else
    {
        sleep(1);
        tplog(5, "hello from thread 1");
    }
}

void do_thread_work2 ( void *ptr )
{
    if (SUCCEED!=tplogconfig(LOG_FACILITY_TP_THREAD, FAIL, "file=./clt-tp-th2.log tp=5", "TEST", NULL))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to configure user TP logger: %s", 
                Nstrerror(Nerror));
    }
    else
    {
        sleep(1);
        tplog(5, "hello from thread 2");
    }
}

/*
 * Do the test call to the server
 */
int main(int argc, char** argv)
{
    int ret = SUCCEED;
    UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, 8192);
    pthread_t thread1, thread2;  /* thread variables */
    
    if (SUCCEED==p_ub)
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to allocate p_ub: %s", tpstrerror(tperrno));
        FAIL_OUT(ret);
    }
    
    /* 
     * Enduro/X logging 
     */
    if (SUCCEED!=tplogconfig(LOG_FACILITY_NDRX|LOG_FACILITY_UBF, 
            log_debug, NULL, "TEST", "./clt-endurox.log"))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to configure Enduro/X logger: %s", 
                Nstrerror(Nerror));
        FAIL_OUT(ret);
    }
    
    /*
     * User TP log 
     */
    if (SUCCEED!=tplogconfig(LOG_FACILITY_TP, FAIL, "file=./clt-tp.log tp=6", "TEST", NULL))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to configure user TP logger: %s", 
                Nstrerror(Nerror));
        FAIL_OUT(ret);
    }
    
    /* write some stuff in user log */
    tplog(log_error, "Hello from tp!");
    
    /* write some stuff in user Enduro/X log */
    NDRX_LOG(log_error, "Hello from NDRX!");
    
    tplog(6, "hello from level 6");
    
    /* run off two threads... (each will have it's own log file) */
    pthread_create (&thread1, NULL, (void *) &do_thread_work1, NULL);
    pthread_create (&thread2, NULL, (void *) &do_thread_work2, NULL);
    
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    
    tplog(6, "hello from main thread");

out:
    return ret;
}

