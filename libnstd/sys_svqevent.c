/**
 * @brief System V Queue Time-out and event handling
 *
 * @file sys_svqevent.c
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

/*---------------------------Includes-----------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <unistd.h>
#include <stdarg.h>
#include <ctype.h>
#include <memory.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>
#include <pthread.h>
#include <string.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <nstdutil.h>
#include <limits.h>

#include <sys_unix.h>
#include <sys/epoll.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/

/* have a thread handler for tout monitoring thread! */

/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Wakup signal handling
 * @param sig
 */
exprivate void ndrx_svq_signal_action(int sig)
{
    /* nothing todo, just ignore */
    return;
}

/**
 * Thread used for monitoring the pipe of incoming timeout requests
 * doing polling for the given time period and sending the events
 * to queue threads.
 * @param arg
 * @return 
 */
exprivate void * ndrx_svq_timeout_thread(void* arg)
{
    /* we shall receive unnamed pipe
     * in thread.
     * 
     * During the operations, via pipe we might receive following things:
     * 1) request for timeout monitornig.
     * 2) request for adding particular FD in monitoring sub-set
     * 3) request for deleting particular FD from monitoring sub-set
     */
}

/**
 * Setup basics for Event handling for System V queues
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_svq_event_init(void)
{
    int err;
    int ret = EXSUCCEED;
   /* 
    * Signal handling 
    */
    struct sigaction act;
    
    NDRX_LOG(log_debug, "About to init System V Eventing sub-system");
    memset(&act, 0, sizeof(act));
    /* define the signal action */

    /* make "act" an empty signal set */
    sigemptyset(&act.sa_mask);

    act.sa_handler = ndrx_svq_signal_action;
    act.sa_flags = SA_NODEFER;

    if (EXSUCCEED!=sigaction(NDRX_SVQ_SIG, &act, 0))
    {
        err = errno;
        NDRX_LOG(log_error, "Failed to init sigaction: %s" ,strerror(err));
        EXFAIL_OUT(ret);
    }
    
    /* At this moment we need to boostrap a timeout monitoring thread.. 
     * the stack size of this thread could be small one because it
     * will mostly work with dynamically allocated memory..
     */
    
out:
    return ret;
}

/**
 * Event send/receive includes timeout processing and other events
 * defined in  NDRX_SVQ_EV*
 * @param mqd queue descriptor (already validated)
 * @param ptr data ptr 
 * @param maxlen either max on rcv, or size to send including mtype
 * @param tout timeout in seconds for timed send, receive
 * @param p_evt See NDRX_SVQ_EV_*. Initially set to NDRX_SVQ_EV_NONE.
 *  if set to something other than NDRX_SVQ_EV_NONE, then there is
 *  event instead of normal data
 * @param p_ptr allocate data buffer for NDRX_SVQ_EV_DATA event.
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_svq_event_sndrcv(mqd_t mqd, char *ptr, size_t maxlen, 
        int tout, int *p_evt, char **p_ptr)
{
    int ret = EXSUCCEED;
    
	
	memset(&ts, 0, sizeof(ts));
	
	/*signal(SIG, signal_action);*/
	
	
	printf("Blocking SIGQUIT in thread...\n");


	/*
	if ((msg_queue_key = ftok (sync->cfg.pathname, sync->cfg.projid)) == -1) 
	{
		perror ("ftok");
		exit (1);
	}
	*/
	
	if ((sync->qid = msgget (IPC_PRIVATE, IPC_CREAT | sync->cfg.permissions)) == -1) 
	{
		perror ("msgget");
		exit (1);
	}

	printf ("Server: Hello, World!\n");

	while (3!=sync->shut1) 
	{
		
		/* read an incoming message
			ok we may get signal here?? */
		int ret;
		int queued;
		int err;
		volatile int loop;
		
		/* Also we shall here check the queue? */
		
		/* Lock here before process... */
		pthread_mutex_lock(&(sync->rcvlockb4));
		
		/* TODO maybe we need pre-read lock...? as it is flushing the queue
			* but we assume that it will still process the queue...! */
		/* lock queue  */
		pthread_mutex_lock(&(sync->qlock));
		
		/* process incoming events... */
		server_process_q(sync);
		
		if (sync->shuts_got)
		{
			pthread_mutex_unlock(&(sync->qlock));
			/* we terminate here... */
			break;
		}

		/* Chain the lockings, so that Q lock would wait until both
			are locked
			*/
		
		
		/* at this moment we can try to wakup our friends???
			* or we will get deadlock?
			*/
		
		/* here is no interrupt, as pthread locks are imune to signals */
		pthread_mutex_lock(&(sync->rcvlock));
		
		/* so that we can wait on Q... */
		
		/* unlock queue  */
		pthread_mutex_unlock(&(sync->qlock));
		
		
		
		
		/* So we will do following by tout thread.
			* Lock Q, put msgs inside unlock
			* if rcvlockb4 && rcvlock are locked, then send kill signals
			* to process...
			* send until both are unlocked or stamp is changed.
			*/
		
		
		/* enable signal */
			
/*		pthread_sigmask(SIG_UNBLOCK, &set, NULL); */
		ret=msgrcv (sync->qid, &message, sizeof (struct message_text), 0, 0);		
		err=errno;
		
		
		
		/* mask signal */
		
		pthread_mutex_unlock(&(sync->rcvlock));
		pthread_mutex_unlock(&(sync->rcvlockb4));
				
#if 0
		/* try condition? 
			* on each message perform wake?
			* so...
			* and have some border that others dot not get the signal
			* if stamp value is changed, then we have waken up the thread
			*/
		//sync->stamp++;
		
		//So do this way:
		//put event in Q.
		// put the border lock 
		// if we get locked rcvlockb4 by ours, then we can forget this one as the message will be processed. Unlock the locks and border
		// if we get one lock and another not, then repeat until we get both non locked
		// if both are non locked, then send signal
		// then signal shall be sent until "stamp" is changed
		//unlock the border
#endif
			
		pthread_mutex_lock(&(sync->border));
		pthread_mutex_unlock(&(sync->border));
			
#if 0
		//Do not let to slip any async events...
		pthread_sigmask(SIG_BLOCK, &set, NULL);
	
		//clear any signals, if async delivery made...
		while (sigtimedwait(&set, NULL, &ts) > 0)
		{
			if (errno == EINTR) {
				continue;
			}
		}
#endif
			
		/* lock queue  */
		pthread_mutex_lock(&(sync->qlock));	
		
		/* process incoming events... */
		server_process_q(sync);
		
		if (sync->shuts_got)
		{
			pthread_mutex_unlock(&(sync->qlock));
			/* we terminate here... */
			break;
		}
		
		/* unlock queue  */
		pthread_mutex_unlock(&(sync->qlock));
		/* and process messages... ok? */
		
		if (-1==ret)
		{
			/*fprintf(stderr, "RET MINUS\n");*/
			
			
			if (err==EINTR)
			{
				/* ok, this is something, maybe timeout */
			/*	fprintf(stderr, "INTR\n");*/
			}
			else
			{
				fprintf(stderr, "msgrcv: %s\n", strerror(err));
				exit (1);
			}
		}
		else
		{
			msgs++;
		}
	}
    
out:
    return ret;
}


/* vim: set ts=4 sw=4 et smartindent: */
