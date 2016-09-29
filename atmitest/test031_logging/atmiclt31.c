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
#include <unistd.h>

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
    tplogclosethread();
    tplog(5, "Thread 1 logs to main");
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
    tplogclosethread();
    tplog(5, "Thread 2 logs to main");
}

/**
 * Test the request file processing....
 * @return 
 */
int test_request_file(void)
{
    int ret = SUCCEED;
    UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, 8192);
    int i;
    long rsplen;
    char testfname[PATH_MAX+1];
    char testfname_should_be[PATH_MAX+1];

    if (SUCCEED==p_ub)
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to allocate p_ub: %s", tpstrerror(tperrno));
        FAIL_OUT(ret);
    }
    
    for (i=0; i<1000; i++)
    {   
        /* let SETREQFILE service to set the filename... 
         * It shall close the previous file (thus if we have a problem there
         * will be memory leak of file descriptors...
         */
        if (SUCCEED!=tplogsetreqfile((char **)&p_ub, NULL, "SETREQFILE"))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to set request file:%s", 
                    tpstrerror(tperrno));
            FAIL_OUT(ret);
        }
        
        tplog(log_debug, "Hello from atmicl31!");
        
        /* test tploggetbufreqfile() must match the name we know */
        testfname[0] = EOS;
        
        if (SUCCEED!=tploggetbufreqfile((char *)p_ub, testfname, sizeof(testfname)))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to get current logger: %s", 
                    tpstrerror(tperrno));
            FAIL_OUT(ret);
        }
        
        sprintf(testfname_should_be, "./logs/request_%d.log", i+1);
        
        TP_LOG(log_debug, "Request file should be [%s] got [%s]", 
                testfname_should_be, testfname);
        
        if (0!=strcmp(testfname_should_be, testfname))
        {
            TP_LOG(log_error, "TESTERROR: Request file should be [%s] but got [%s]!!!", 
                testfname_should_be, testfname);
            FAIL_OUT(ret);
        }

        /* Test tploggetreqfile() */
        
        if (!tploggetreqfile(testfname, sizeof(testfname)))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to get current request log file: %s", 
                    tpstrerror(tperrno));
            FAIL_OUT(ret);
        }
        
        if (0!=strcmp(testfname_should_be, testfname))
        {
            TP_LOG(log_error, "TESTERROR: Request file should be [%s] but got [%s]!!!", 
                testfname_should_be, testfname);
            FAIL_OUT(ret);
        }
        
        /* Add some data to buffer */
        if (0==(i % 100))
        {
            if (SUCCEED!=Badd(p_ub, T_STRING_FLD, "HELLO WORLD!", 0L))
            {
                NDRX_LOG(log_error, "TESTERROR: Failed to add T_STRING_FLD:%s", 
                    Bstrerror(Berror));
                FAIL_OUT(ret);
            }
        }
        
        /* About to call service */
        tplog(log_warn, "Calling TEST31_1ST!");
        if (FAIL == tpcall("TEST31_1ST", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0))
        {
            NDRX_LOG(log_error, "TEST31_1ST failed: %s", tpstrerror(tperrno));
            FAIL_OUT(ret);
        }
        
        tplog(log_warn, "back from TEST31_1ST call!");     
        
        tplogprintubf(log_info, "Buffer before cleanup", p_ub);
        
        /* delete the request file from buffer */
        if (SUCCEED!=tplogdelbufreqfile((char *)p_ub))
        {
            NDRX_LOG(log_error, "tplogdelbufreqfile() failed: %s", tpstrerror(tperrno));
            FAIL_OUT(ret);
        }
        
        tplogprintubf(log_info, "Buffer after cleanup", p_ub);
    }
    
out:
    /* close the logger
     * Now sutff should go to endurox and tp
     */
    tplogclosereqfile();

    return ret;
    
}

/*
 * Do the test call to the server
 */
int main(int argc, char** argv)
{
    int ret = SUCCEED;
    
    pthread_t thread1, thread2;  /* thread variables */
    
    /* 
     * Enduro/X logging 
     */
    if (SUCCEED!=tplogconfig(LOG_FACILITY_NDRX|LOG_FACILITY_UBF, 
            FAIL, "ndrx=5 ubf=0", "TEST", "./clt-endurox.log"))
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
    
    if (SUCCEED!=test_request_file())
    {
        tplog(5, "TESTERROR: test_request_file() failed!");
    }
    
out:

    tplog(1, "Finishing off");

    return ret;
}

