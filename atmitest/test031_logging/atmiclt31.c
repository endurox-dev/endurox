/**
 * @brief Basic test client
 *
 * @file atmiclt31.c
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
#include <unistd.h>

#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>
#include <ndebugcmn.h>
#include <test.fd.h>
#include <ndrstandard.h>
#include <nstdutil.h>
#include <exassert.h>
#include <nstd_int.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/


void do_thread_work1 ( void *ptr )
{
    /* 
     * Enduro/X logging 
     * But can be done at thread level
     */
    if (EXSUCCEED!=tplogconfig(LOG_FACILITY_NDRX_THREAD|LOG_FACILITY_UBF_THREAD, 
            EXFAIL, "ndrx=5 ubf=0", "TEST", "./clt-endurox.log"))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to configure Enduro/X logger: %s", 
                Nstrerror(Nerror));
        return;
    }
    
    /* write some stuff in user Enduro/X log */
    NDRX_LOG(log_error, "Hello from NDRX!");
    
    if (EXSUCCEED!=tplogconfig(LOG_FACILITY_TP_THREAD, EXFAIL, "file=./clt-tp-th1.log tp=5", "TEST", NULL))
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
    if (EXSUCCEED!=tplogconfig(LOG_FACILITY_TP_THREAD, EXFAIL, "file=./clt-tp-th2.log tp=5", "TEST", NULL))
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
    int ret = EXSUCCEED;
    UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, 8192);
    int i;
    long rsplen;
    char testfname[PATH_MAX+1];
    char testfname_should_be[PATH_MAX+1];

    if (EXSUCCEED==p_ub)
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to allocate p_ub: %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    for (i=0; i<1000; i++)
    {   
        /* let SETREQFILE service to set the filename... 
         * It shall close the previous file (thus if we have a problem there
         * will be memory leak of file descriptors...
         */
        if (EXSUCCEED!=tplogsetreqfile((char **)&p_ub, NULL, "SETREQFILE"))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to set request file:%s", 
                    tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
        
        tplog(log_debug, "Hello from atmicl31!");
        
        /* test tploggetbufreqfile() must match the name we know */
        testfname[0] = EXEOS;
        
        if (EXSUCCEED!=tploggetbufreqfile((char *)p_ub, testfname, sizeof(testfname)))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to get current logger: %s", 
                    tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
        
        snprintf(testfname_should_be, sizeof(testfname_should_be), 
                "./logs/request_%d.log", i+1);
        
        TP_LOG(log_debug, "Request file should be [%s] got [%s]", 
                testfname_should_be, testfname);
        
        if (0!=strcmp(testfname_should_be, testfname))
        {
            TP_LOG(log_error, "TESTERROR: Request file should be [%s] but got [%s]!!!", 
                testfname_should_be, testfname);
            EXFAIL_OUT(ret);
        }

        /* Test tploggetreqfile() */
        
        if (!tploggetreqfile(testfname, sizeof(testfname)))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to get current request log file: %s", 
                    tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
        
        if (0!=strcmp(testfname_should_be, testfname))
        {
            TP_LOG(log_error, "TESTERROR: Request file should be [%s] but got [%s]!!!", 
                testfname_should_be, testfname);
            EXFAIL_OUT(ret);
        }
        
        /* Add some data to buffer */
        if (0==(i % 100))
        {
            if (EXSUCCEED!=Badd(p_ub, T_STRING_FLD, "HELLO WORLD!", 0L))
            {
                NDRX_LOG(log_error, "TESTERROR: Failed to add T_STRING_FLD:%s", 
                    Bstrerror(Berror));
                EXFAIL_OUT(ret);
            }
        }
        
        /* About to call service */
        tplog(log_warn, "Calling TEST31_1ST!");
        if (EXFAIL == tpcall("TEST31_1ST", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0))
        {
            NDRX_LOG(log_error, "TEST31_1ST failed: %s", tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
        
        tplog(log_warn, "back from TEST31_1ST call!");     
        
        NDRX_LOG(log_always, "THIS IS NDRX IN REQLOG!");
        UBF_LOG(0, "THIS IS UBF IN REQLOG!");
        
        tplogprintubf(log_info, "Buffer before cleanup", p_ub);
        
        /* delete the request file from buffer */
        if (EXSUCCEED!=tplogdelbufreqfile((char *)p_ub))
        {
            NDRX_LOG(log_error, "tplogdelbufreqfile() failed: %s", tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
        
        tplogprintubf(log_info, "Buffer after cleanup", p_ub);
    }
    
out:
    /* close the logger
     * Now sutff should go to endurox and tp
     */
    tplogclosereqfile();

    NDRX_LOG(log_always, "THIS IS NDRX IN PROCLOG!");
    UBF_LOG(0, "THIS IS UBF IN PROCLOG!");
    TP_LOG(0, "THIS IS TP IN PROCLOG!");
        

    return ret;
    
}

/*
 * Do the test call to the server
 */
int main(int argc, char** argv)
{
    int ret = EXSUCCEED;
    int i;
    int sinks, refs;
    pthread_t thread1, thread2;  /* thread variables */
    
#if 0
    - this does not work at process level, due to fact that logger sinks cannot be swapped
    - instead this can be done on thread logger bases
            
    /* 
     * Enduro/X logging 
     * ()
     */
    if (EXSUCCEED!=tplogconfig(LOG_FACILITY_NDRX|LOG_FACILITY_UBF, 
            EXFAIL, "ndrx=5 ubf=0", "TEST", "./clt-endurox.log"))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to configure Enduro/X logger: %s", 
                Nstrerror(Nerror));
        EXFAIL_OUT(ret);
    }
#endif
    /*
     * User TP log 
     */
    if (EXSUCCEED!=tplogconfig(LOG_FACILITY_TP, EXFAIL, "file=./clt-tp.log tp=6", "TEST", NULL))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to configure user TP logger: %s", 
                Nstrerror(Nerror));
        EXFAIL_OUT(ret);
    }
    
    /* write some stuff in user log */
    tplog(log_error, "Hello from tp!");
    
#if 0
    /* write some stuff in user Enduro/X log */
    NDRX_LOG(log_error, "Hello from NDRX!");
#endif
    tplog(6, "hello from level 6");
    
    /* run off two threads... (each will have it's own log file) */
    pthread_create (&thread1, NULL, (void *) &do_thread_work1, NULL);
    pthread_create (&thread2, NULL, (void *) &do_thread_work2, NULL);
    
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    
    tplog(6, "hello from main thread");
    
    if (EXSUCCEED!=test_request_file())
    {
        tplog(5, "TESTERROR: test_request_file() failed!");
    }
    
#define NO_DIR_LOG "./non_exist/folder/is/missing/test.log"
    
    /* we do not have permissions to root */
#define MKFAIL_DIR_LOG "./non_write/folder/is/missing/test.log"
    /* open new log file in non existing folder */
    
    if (EXSUCCEED!=tplogconfig(LOG_FACILITY_TP, EXFAIL, NULL, 
            "TEST", NO_DIR_LOG))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to open log to missing folder: %s", 
                Nstrerror(Nerror));
        EXFAIL_OUT(ret);
    }

    tplog(1, "Hello to missing folder");

    /* check that file exists */
    if (ndrx_file_exists(NO_DIR_LOG))
    {
        NDRX_LOG(log_error, "TESTERROR! [%s] must be missing!", NO_DIR_LOG);
        EXFAIL_OUT(ret);
    }

    if (EXSUCCEED!=tplogconfig(LOG_FACILITY_TP, EXFAIL, "tp=6 mkdir=y",
            "TEST", NO_DIR_LOG))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to open log to missing folder: %s", 
                Nstrerror(Nerror));
        EXFAIL_OUT(ret);
    }

    tplog(1, "Hello to missing folder");

    /* check that file exists */
    if (!ndrx_file_exists(NO_DIR_LOG))
    {
        NDRX_LOG(log_error, "TESTERROR! [%s] must be present!", NO_DIR_LOG);
        EXFAIL_OUT(ret);
    }

    /* check that fails to open log file */
    if (EXSUCCEED!=tplogconfig(LOG_FACILITY_TP, EXFAIL, "tp=6 mkdir=y",
            "TEST", MKFAIL_DIR_LOG))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to open log to non-write folder: %s [%s]", 
                Nstrerror(Nerror), MKFAIL_DIR_LOG);
        EXFAIL_OUT(ret);
    }

    if (ndrx_file_exists(MKFAIL_DIR_LOG))
    {
        NDRX_LOG(log_error, "TESTERROR! [%s] must be missing!", MKFAIL_DIR_LOG);
        EXFAIL_OUT(ret);
    }
    
    tplogconfig(LOG_FACILITY_TP, EXFAIL, "file=./clt-tp.log tp=6", "TEST", NULL);
    
    
    /* Ensure only specific number loggers has left:
     * if we set all to 1 x file.
     * 
     *  */
    
    ndrx_debug_refcount(&sinks, &refs);
    
    /* stdout and process file */
    NDRX_ASSERT_TP_OUT((sinks==2), "There shall be only 2 process level sink, but got: %d", sinks);
    
    /* there shall be only 4 refs*/
    NDRX_ASSERT_TP_OUT((refs==4), "There shall be only 4 refs (ubf, stdout, tp, ndrx): got %d", refs);
    
out:

    tplog(1, "Finishing off");

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
