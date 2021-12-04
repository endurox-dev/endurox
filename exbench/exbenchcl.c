/**
 * @brief Benchmark tool client
 *
 * @file exbenchcl.c
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include <ndebug.h>
#include <atmi.h>
#include <exthpool.h>

#include "atmi_int.h"
#include "expr.h"
#include <typed_buf.h>
#include <atmi_int.h>   /*  ndrx_tpchkunsol() */
#include <atmi_tls.h>   /* my_id */
#include <Exfields.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

/**
 * Read end of pipe
 */
#define NDRX_READ 0


/**
 * Write end of pipe
 */
#define NDRX_WRITE 1

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
exprivate int M_nr_threads=1;

/* if contains %d then it is replaced with thread number */
exprivate char M_svcnm[XATMI_EVENT_MAX+1]="EXBENCH";
exprivate int M_runtime=60;             /**< run for 60 sec default          */
exprivate char *M_sample_data=NULL;     /**< data to send                    */
exprivate BFLDID M_fld=BBADFLDID;       /**< field used for random data fill */
exprivate int M_rndsize=1024;           /**< test data size                  */
exprivate int M_doplot=EXFALSE;         /**< Do plot the benchmark restulst  */
exprivate int M_prio=NDRX_MSGPRIO_DEFAULT;  /**< Call priorioty              */
exprivate typed_buffer_descr_t * M_buftype = NULL;
exprivate threadpool M_threads; /**< thread pool */
exprivate MUTEX_LOCKDECL(M_wait_mutex);
exprivate int M_do_run = EXTRUE;
exprivate long M_msg_sent=0;    /**< Messages sent */
exprivate char *M_master_buf=NULL;  /**< This is master sample buffer       */
exprivate int M_msgsize = 0;        /**< effective message size */
exprivate int M_fork = EXFALSE;     /**< Use forking */
exprivate int M_fd[2]={EXFAIL, EXFAIL}; /**< pipe channel for forking clients to report stats */
exprivate int M_svcnum = 0;        /**< No multi-services used */
/** persistent queue mode, queue space */
exprivate char M_qspace[XATMI_SERVICE_NAME_LENGTH+1] = {EXEOS};
exprivate int M_autoq = EXFALSE;   /**< Use autoq testing                   */
exprivate int M_enqonly = EXFALSE;   /**< Persisten q, enqueue only         */
exprivate long M_numreq = EXFALSE;   /**< Number of requests                */

exprivate int M_tran = EXFALSE; /**< use distr tran */
exprivate int M_notify_mode = EXFALSE;  /**< shall notification be expected */
exprivate __thread long M_sent=0;               /** Current position of thread */
exprivate __thread long M_notif_sent_acq = EXFAIL;  /**< ACQ position          */
exprivate int M_event_mode=EXFALSE; /**< is event mode enabled?                */
/*---------------------------Prototypes---------------------------------*/

/**
 * Process the notification callback
 */
exprivate void notification_callback (char *data, long len, long flags)
{
    M_notif_sent_acq = M_sent;
}

/* need to synchronize function for starting the sending... */
expublic void thread_process(void *ptr, int *p_finish_off)
{
    /*  thread number */
    long thnum = (long)ptr;
    char svcnm[XATMI_SERVICE_NAME_LENGTH+1];
    char *buf = tpalloc(M_buftype->type, NULL, M_rndsize*2);
    char *rcv_buf;
    long rcvlen;
    
    TPQCTL qc;
    ndrx_stopwatch_t w;
    
    if (EXSUCCEED!=tpinit(NULL))
    {
        NDRX_LOG(log_error, "Failed to init: %s", tpstrerror(tperrno));
        userlog("Failed to init: %s", tpstrerror(tperrno));
        exit(-1);
    }
    
    
    if (M_tran && EXSUCCEED!=tpopen())
    {
        NDRX_LOG(log_error, "Failed to tpopen(): %s", 
                tpstrerror(tperrno));
        exit(-1);
    }

    if (NULL==buf)
    {
        NDRX_LOG(log_error, "Failed to alloc send buf: %s", 
                tpstrerror(tperrno));
        exit(-1);
    }
    
    /* In case if notify Q -> append the buffer with myid */
    memcpy(buf, M_master_buf, M_rndsize*2);
    
    if (M_notify_mode)
    {
        /* load client id so that server may provide notification */
        if (EXSUCCEED!=Bchg((UBFH *)buf, EX_CLTID, 0, G_atmi_tls->G_atmi_conf.my_id, 0))
        {
            NDRX_LOG(log_error, "Failed to set EX_CLTID: %s", Bstrerror(Berror));
            exit(-1);
        }
        
        /* set callback... */
        tpsetunsol (notification_callback);
        
    }
    
    /* Service by thread */
    
    if (M_svcnum > 0)
    {
        snprintf(svcnm, sizeof(svcnm), "%s%03d", M_svcnm, (int)thnum % M_svcnum);
    }
    else
    {
        /* just use name directly.. */
        NDRX_STRCPY_SAFE(svcnm, M_svcnm);
    }
    
    /* re-lock.. */
    MUTEX_LOCK_V(M_wait_mutex);
    MUTEX_UNLOCK_V(M_wait_mutex);

    ndrx_stopwatch_reset(&w);
    
    while ((!M_fork && M_do_run) || (M_fork && ndrx_stopwatch_get_delta_sec(&w) < M_runtime))
    {
        if (M_prio!=NDRX_MSGPRIO_DEFAULT)
        {
            tpsprio(M_prio, TPABSOLUTE);
        }
                
        /* start tran, if M_tran */
        if (M_tran && EXSUCCEED!=tpbegin(M_tran, 0))
        {
            NDRX_LOG(log_error, "tpbegin() failed: %s",
                tpstrerror(tperrno));
            exit(-1);
        }

        if (M_qspace[0])
        {
            /* Run enq to SVCNM */
            memset(&qc, 0, sizeof(qc));
            if (EXSUCCEED!=tpenqueue(M_qspace, svcnm, &qc, buf, 0, 0))
            {
                NDRX_LOG(log_error, "tpenqueue() failed %s diag: %d:%s", 
                        tpstrerror(tperrno), qc.diagnostic, qc.diagmsg);
                exit(-1);
            }

            if (!M_autoq && !M_enqonly)
            {

                /* restart the tran if doing deq */
                if (M_tran && EXSUCCEED!=tpcommit(0))
                {
                    NDRX_LOG(log_error, "tpcommit() failed: %s",
                        tpstrerror(tperrno));
                    exit(-1);
                }

                if (M_tran && EXSUCCEED!=tpbegin(M_tran, 0))
                {
                    NDRX_LOG(log_error, "tpbegin() failed: %s",
                        tpstrerror(tperrno));
                    exit(-1);
                }

                /* Run deq from SVCNM */
                memset(&qc, 0, sizeof(qc));
                rcv_buf=NULL;
                if (EXSUCCEED!=tpdequeue(M_qspace, svcnm, &qc, (char **)&rcv_buf, &rcvlen, 0))
                {
                    NDRX_LOG(log_error, "tpdequeue() failed %s diag: %d:%s", 
                            tpstrerror(tperrno), qc.diagnostic, qc.diagmsg);
                    exit(-1);
                }
            }

        }
        else
        {
            rcv_buf=NULL;
            if (M_event_mode)
            {
                if (tppost(svcnm, buf, 0, 0)<=0)
                {
                    NDRX_LOG(log_error, "Failed to post event [%s]: %s", 
                            svcnm, tpstrerror(tperrno));
                    exit(-1);
                }
            }
            else if (EXFAIL==tpcall(svcnm, buf, 0, &rcv_buf, &rcvlen, 0))
            {
                NDRX_LOG(log_error, "Failed to call [%s]: %s", 
                        svcnm, tpstrerror(tperrno));
                exit(-1);
            }
        }

        if (M_tran && EXSUCCEED!=tpcommit(0))
        {
            NDRX_LOG(log_error, "tpcommit() 2 failed: %s",
                tpstrerror(tperrno));
            exit(-1);
        }
        
        if (NULL!=rcv_buf)
        {
            tpfree(rcv_buf);
        }
        
        /* if set, wait for notification back! 
         * either we got the callback already during the call...
         * Or we wait for callback here
         * We need some sequencing.
         * Will use sent as seq...
         */
        if (M_notify_mode)
        {
            while (M_notif_sent_acq<M_sent)
            {
                if (EXFAIL==ndrx_tpchkunsol(0))
                {
                    if (tperrno!=TPETIME)
                    {
                        NDRX_LOG(log_error, "Failed to wait for notif: %s", 
                                tpstrerror(tperrno));
                        exit(-1);
                    }
                }
            }
        }
        
        M_sent++;
        /* enqueue number of messages only... */
        if (M_numreq && M_sent >= M_numreq)
        {
            break;
        }
    }
    
    /* publish results... */
    MUTEX_LOCK_V(M_wait_mutex);
    M_msg_sent+=M_sent;
    MUTEX_UNLOCK_V(M_wait_mutex);

    /* Wait on queue to finish ... 
     * Additionally, if we have several servers running
     * then @TMQ servers running, then we shall peek
     * them all for the stats, if all queues are empty,
     * only then terminate
     */
    if (M_qspace[0] && M_autoq)
    {
        int done = EXFALSE;
        do
        {
            memset(&qc, 0, sizeof(qc));
            qc.flags|=TPQPEEK;
            rcv_buf=NULL;
            if (EXSUCCEED!=tpdequeue(M_qspace, svcnm, &qc, (char **)&rcv_buf, &rcvlen, 0))
            {
                if (tperrno==TPEDIAGNOSTIC && QMENOMSG==qc.diagnostic)
                {
                    done=EXTRUE;
                }
                else
                {
                    NDRX_LOG(log_error, "tpdequeue() failed %s diag: %d:%s", 
                            tpstrerror(tperrno), qc.diagnostic, qc.diagmsg);
                    exit(-1);
                }
            }
            else
            {
                if (NULL!=rcv_buf)
                {
                    tpfree(rcv_buf);
                }
                sleep(1);
            }
            
        } while (!done);
    }
    
out:
    
    *p_finish_off=EXTRUE;

    /* free up ... */
    if (NULL!=buf)
    {
        tpfree(buf);
    }
    
    if (M_tran)
    {
        tpclose();
    }    

    /* release resources */
    tpterm();

    return;
    
}

/**
 * Print usage
 * @param bin binary name
 */
expublic void usage(char *bin)
{
    fprintf(stderr, "Usage: %s [options] -B buffer_type \n", bin);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -p <prio>        Call priority\n");
    fprintf(stderr, "  -P               Plot results, default false. Needs NDRX_BENCH_FILE and NDRX_BENCH_CONFIGNAME\n");
    fprintf(stderr, "  -n <threadsnr>   Number of threads, default 1\n");
    fprintf(stderr, "  -s <svcnm>       Service to call\n");
    fprintf(stderr, "  -t <time>        Number of seconds to run\n");
    fprintf(stderr, "  -f <fldnm>       Ubf field name to fill with random data\n");
    fprintf(stderr, "  -b <data>        Initial UBF data. In tpjsontoubf() format.\n");
    fprintf(stderr, "  -S <size>        Random data size, default 1024\n");
    fprintf(stderr, "  -F               Use forking instead of threading\n");
    fprintf(stderr, "  -N <svcnum>      Number of services\n");
    fprintf(stderr, "  -Q <qspname>     Persistent queue mode. Queue space name (thread enq+deq)\n");
    fprintf(stderr, "  -A               Auto queue testing (forwarding)\n");
    fprintf(stderr, "  -E               Persist only\n");
    fprintf(stderr, "  -R <msgnum>      Number of requests (time or nr first to stop)\n");
    fprintf(stderr, "  -T <tout_sec>    Initiate global transaction for XATMI calls\n");
    fprintf(stderr, "  -I               Wait for client notification\n");
    fprintf(stderr, "  -e               Post the event, -s serves as event\n");
}

/**
 * Benchmark tool entry
 */
expublic int main( int argc, char** argv )
{
    int ret = EXSUCCEED;
    int c;
    long i;
    char *rnd_block = NULL;
    double spent;
    double tps;
    ndrx_stopwatch_t w;
    char run_ver[512];
    int parent=EXTRUE;
    /* parse args: 
     * -n <number_of_threads> 
     * -s <service_to_call> 
     * -t <time_in_sec> 
     * -b <UBF buffer base> 
     * -f <random_data_field for UBF> 
     * -S <random_data_size>
     * -P - do plot
     * -p <call_priority>
     * -F - use forking mode
     * -N <number_of_services_modulus>
     */
    
    /* Currently Only UBF is supported
     * TODO: in future releases use tpimport() to teterminte the buffer format
     */
    M_buftype = ndrx_get_buffer_descr("UBF", NULL);

    while ((c = getopt (argc, argv, "n:s:t:b:S:p:Pf:FN:Q:AER:T:Ie")) != -1)
    {
        switch (c)
        {
            case 'e':
                M_event_mode=EXTRUE;
                break;
            case 'A':
                M_autoq = EXTRUE;
                break;
            case 'R':
                M_numreq = atol(optarg);
                break;
            case 'E':
                M_enqonly = EXTRUE;
                break;
            case 'Q':
                NDRX_STRCPY_SAFE(M_qspace, optarg);
                break;
            case 'N':
                M_svcnum = atoi(optarg);
                break;
            case 'F':
                M_fork = EXTRUE;
                break;
            case 'p':
                M_prio = atoi(optarg);
                break;
            case 'P':
                M_doplot = EXTRUE;
                break;
            case 'n':
                /* this will allocate thread pool... */
                M_nr_threads=atoi(optarg);
                break;
            case 's':
                /* service to call */
                NDRX_STRCPY_SAFE(M_svcnm, optarg);
                break;
            case 't':
                M_runtime = atoi(optarg);
                break;
            case 'b':
                
                M_sample_data = NDRX_STRDUP(optarg);
                
                if (NULL==M_sample_data)
                {
                    NDRX_LOG(log_error, "Failed to copy data buffer: %s", strerror(errno));
                    EXFAIL_OUT(ret);
                }
                
                break;
            case 'f':
                
                M_fld = Bfldid(optarg);
                
                if (BBADFLDID==M_fld)
                {
                    NDRX_LOG(log_error, "Failed to resolve field id [%s]: %s", 
                            optarg, Bstrerror(Berror));
                    EXFAIL_OUT(ret);
                }
                break;
            case 'S':
                M_rndsize = atoi(optarg);
                break;
            case 'T':
                M_tran = atoi(optarg);
                break;
            case 'I':
                M_notify_mode = EXTRUE;
                break;
            default:
                NDRX_LOG(log_error, "Unknown option %c", c);
                usage(argv[0]);
                EXFAIL_OUT(ret);
                break;
        }
    }
    
    
    /*** print details: ****/
    
    NDRX_LOG(log_info, "M_svcnm=[%s]", M_svcnm);
    NDRX_LOG(log_info, "M_runtime=%d", M_runtime);
    NDRX_LOG(log_info, "M_sample_data=[%s]", (M_sample_data?M_sample_data:"NULL"));
    NDRX_LOG(log_info, "M_fld=%ld", M_fld);
    NDRX_LOG(log_info, "M_rndsize=%d", M_rndsize);
    NDRX_LOG(log_info, "M_doplot=%d", M_doplot);
    NDRX_LOG(log_info, "M_prio=%d", M_prio);
    NDRX_LOG(log_info, "M_buftype=%p", M_buftype);
    NDRX_LOG(log_info, "M_nr_threads=%d", M_nr_threads);
    NDRX_LOG(log_info, "M_fork=%d", M_fork);
    NDRX_LOG(log_info, "M_svcnum=%d", M_svcnum);
    NDRX_LOG(log_info, "M_qspace=[%s]", M_qspace);
    NDRX_LOG(log_info, "M_autoq=[%d]", M_autoq);
    NDRX_LOG(log_info, "M_enqonly=[%d]", M_autoq);
    NDRX_LOG(log_info, "M_numreq=[%ld]", M_numreq);
    NDRX_LOG(log_info, "M_tran=[%d]", M_tran);
    NDRX_LOG(log_info, "M_notify_mode=[%d]", M_notify_mode);
    NDRX_LOG(log_info, "M_event_mode=[%d]", M_event_mode);
    
    /* allocate the buffer & fill with random data */
    
    if (NULL==M_buftype)
    {
        NDRX_LOG(log_error, "Invalid buffer specified (or not specified)");
        usage(argv[0]);
        EXFAIL_OUT(ret);
    }
    
    M_master_buf = tpalloc(M_buftype->type, NULL, 1024 + M_rndsize*2);
    
    if (NULL==M_master_buf)
    {
        NDRX_LOG(log_error, "Failed to allocate send buffer: %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    /* parse data in ... */
    
    if (0==strcmp(M_buftype->type, "UBF"))
    {
        if (NULL!=M_sample_data)
        {
            if (EXSUCCEED!=tpjsontoubf((UBFH *)M_master_buf, M_sample_data))
            {
                NDRX_LOG(log_error, "Failed to parse call data: %s", 
                        tpstrerror(tperrno));
                EXFAIL_OUT(ret);
            }
        }
        
        /* load random data */
        rnd_block = NDRX_MALLOC(M_rndsize);
        
        if (NULL==rnd_block)
        {
            NDRX_LOG(log_error, "Failed to malloc random block: %s", 
                    strerror(errno));
            EXFAIL_OUT(ret);
        }
       
        /* carray block */
        if (BBADFLDID!=M_fld)
        {
            NDRX_LOG(log_debug, "Adding random block to %s of %d", Bfname(M_fld), M_rndsize);
            if (EXSUCCEED!=Bchg((UBFH *)M_master_buf, M_fld, 0, rnd_block, M_rndsize))
            {
                NDRX_LOG(log_error, "Failed to add random block: %s", 
                        Bstrerror(Berror));
                EXFAIL_OUT(ret);
            }
        }
        
        M_msgsize=Bused((UBFH *)M_master_buf);
    }
    else if (M_notify_mode)
    {
        NDRX_LOG(log_error, "Notify (-I) mode only supported on UBF buffers!");
        EXFAIL_OUT(ret);
    }
    
    if (!getenv("NDRX_BENCH_FILE"))
    {
        setenv("NDRX_BENCH_FILE", "test.out", EXTRUE);
    }
    
    if (!getenv("NDRX_BENCH_CONFIGNAME"))
    {
        snprintf(run_ver, sizeof(run_ver), "test");
        setenv("NDRX_BENCH_CONFIGNAME", run_ver, EXTRUE);
    }
    
    if (M_fork)
    {
        signal(SIGCHLD, SIG_IGN); /* ignore childs, we will wait on pipe */
        
        NDRX_LOG(log_debug, "Fork mode");

        if (EXSUCCEED!=pipe ( M_fd ))
        {
            NDRX_LOG(log_error, "Failed to pipe: %s", strerror(errno));
            EXFAIL_OUT(ret);
        }
        
        /* just in anty case... */
        tpterm();
        
        for (i=0; i<M_nr_threads; i++)
        {
            if ( fork () == 0 ) /* Child Writer */
            {
                int finish=EXFALSE;
                parent=EXFALSE;
                
                sleep(1);
                
                ndrx_stopwatch_reset(&w);
                
                /* mark the run-time... */
                thread_process((void *)i, &finish);
                
                spent=ndrx_stopwatch_get_delta(&w);

                tps = ((double)M_msg_sent / (spent / 1000));
                NDRX_LOG(log_debug, "Spent: %ld ms msgs: %ld tps: %lf", 
                    spent, M_msg_sent, tps);
                
                if (sizeof(tps)!=write (M_fd[NDRX_WRITE], (char *)&tps, sizeof(tps)))
                {
                    NDRX_LOG(log_error, "Failed to write to pipe: %s", 
                            strerror(errno));
                    userlog("Failed to write to pipe: %s", 
                            strerror(errno));
                    EXFAIL_OUT(ret);
                }
                
                /* proc is terminating.. */
                goto out;
                
            }
        }
       
        /* read the results */
        for (i=0; i<M_nr_threads; i++)
        {
            double tpsproc=0;
            if (sizeof(tpsproc)!=read ( M_fd[NDRX_READ], (char *)&tpsproc, sizeof(tpsproc)))
            {
                NDRX_LOG(log_error, "Failed to read result %d: %s", strerror(errno));
                EXFAIL_OUT(ret);
            }
            /* update totals */
            tps+=tpsproc;
        }
    }
    else
    {
        
        NDRX_LOG(log_debug, "Thread mode");
        M_threads = ndrx_thpool_init(M_nr_threads,  &ret, NULL, NULL, 0, NULL);

        if (EXSUCCEED!=ret)
        {
            NDRX_LOG(log_error, "Thread pool init failure");
            EXFAIL_OUT(ret);
        }

        /* sync to master */
        MUTEX_LOCK_V(M_wait_mutex);

        for (i=0; i<M_nr_threads; i++)
        {
            /* thread nr is set as ptr */
            ndrx_thpool_add_work(M_threads, (void*)thread_process, (void *)i);
        }

        /* let threads to prepare */
        sleep(2);

        MUTEX_UNLOCK_V(M_wait_mutex);
        ndrx_stopwatch_reset(&w);

        /* let it run... */
        while (ndrx_stopwatch_get_delta_sec(&w) < M_runtime)
        {
            /* If all threads have made exit, assume it was
             * request based run
             */
            sleep(1);
            
            if (M_nr_threads==ndrx_thpool_nr_not_working(M_threads))
            {
                NDRX_LOG(log_debug, "All threads did exit.");
                break;
            }
        }
        M_do_run=EXFALSE;

        /* wait.. */
        ndrx_thpool_wait(M_threads);
        spent=ndrx_stopwatch_get_delta(&w);

        tps = ((double)M_msg_sent / (spent / 1000));

        NDRX_LOG(log_debug, "Spent: %ld ms msgs: %ld tps: %lf", 
                spent, M_msg_sent, tps);

        ndrx_thpool_destroy(M_threads);
    }
    
    /* write the stats... */
    if (M_doplot)
    {
        ndrx_bench_write_stats(M_msgsize, tps);
    }
    
out:
    if (EXFAIL!=M_fd[NDRX_READ])
    {
        close(M_fd[NDRX_READ]);
    }

    if (EXFAIL!=M_fd[NDRX_WRITE])
    {
        close(M_fd[NDRX_WRITE]);
    }

    if (NULL!=M_master_buf)
    {
        tpfree((char *)M_master_buf);
    }

    if (NULL!=M_sample_data)
    {
        NDRX_FREE(M_sample_data);
    }

    if (NULL!=rnd_block)
    {
        NDRX_FREE(rnd_block);
    }

    return ret;
    
}

/* vim: set ts=4 sw=4 et smartindent: */
