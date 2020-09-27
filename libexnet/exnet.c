/**
 * @brief Enduro/X server net/socket client lib
 *   Network object needs to be synchronize otherwise unexpected core dumps might
 *   Locking:
 *   - main thread always have read lock
 *   - sender thread read lock only when doing send
 *   - when main thread wants some changes in net object, it waits for write lock
 *   - when scheduling the connection close, there is not need to write lock
 *     because that read thread sets only EXTRUE (close). The actual close and
 *     new socket open will be handled by write lock, thus it shall prevent from
 *     accidental setting back to EXTRUE.
 *   - the connection object once created will always live in DL list even with
 *   with disconnected status. Once new conn arrives, it will re-use the conn obj.
 *
 * @file exnet.c
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
#include <ndrx_config.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

#include <ndrstandard.h>

#if defined(EX_USE_EPOLL)

#include <sys/epoll.h>

#elif defined(EX_USE_KQUEUE)

#include <sys/event.h>

#endif

#include <poll.h>

#include <atmi.h>
#include <stdio.h>
#include <stdlib.h>
#include <exnet.h>
#include <ndebug.h>
#include <utlist.h>

#include <atmi_int.h>
#include <ndrx_config.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define DBUF_SZ	(NDRX_MSGSIZEMAX - net->dl)	/**< Buffer size to recv in 	*/

#if defined(EX_USE_EPOLL)

#define POLL_FLAGS (EPOLLIN | EPOLLHUP)

#elif defined(EX_USE_KQUEUE)

#define POLL_FLAGS EVFILT_READ

#else

#define POLL_FLAGS (POLLIN)

#endif

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

exprivate MUTEX_LOCKDECL(M_send_lock) 
exprivate MUTEX_LOCKDECL(M_recv_lock) 

/*---------------------------Prototypes---------------------------------*/
exprivate int close_socket(exnetcon_t *net);
exprivate int open_socket(exnetcon_t *net);
exprivate ssize_t recv_wrap (exnetcon_t *net, void *__buf, size_t __n, 
        int flags, int appflags);

exprivate int exnet_schedule_run(exnetcon_t *net);


/**
 * Get delta time when stopwatch was set
 * @param net network connection, used for locking
 * @param w stop watch to extract value from
 * @return stop watch reading
 */
expublic long exnet_stopwatch_get_delta_sec(exnetcon_t *net, ndrx_stopwatch_t *w)
{
    long ret;
    
    MUTEX_LOCK_V(net->flagslock);
    
    ret = ndrx_stopwatch_get_delta_sec(w);
    
    MUTEX_UNLOCK_V(net->flagslock);
    
    return ret;
}

/**
 * Reset the stopwatch in locked way
 * @param net network which contains the flags lock
 * @param w stopwatch to reset
 */
expublic void exnet_stopwatch_reset(exnetcon_t *net, ndrx_stopwatch_t *w)
{
    MUTEX_LOCK_V(net->flagslock);
    
    ndrx_stopwatch_reset(w);
    
    MUTEX_UNLOCK_V(net->flagslock);
}   

/**
 * Send single message, put length in front
 * We will send all stuff required, do that in loop!
 * @param hdr_buf pre-send some header, to avoid extra mem opy
 * @param hdr_len pre-send header lenght
 */
expublic int exnet_send_sync(exnetcon_t *net, char *hdr_buf, int hdr_len, 
        char *buf, int len, int flags, int appflags)
{
    int ret=EXSUCCEED;
    int allow_size = DATA_BUF_MAX;
    int sent = 0;
    /* let to have 128 header size */
    char d[NET_LEN_PFX_LEN+128];	/* Data buffer, len             */
    int size_to_send, len_whdr, hdr_snd_size = 0;
    int tmp_s;
    int err;
    int retry;
    ndrx_stopwatch_t w;

    /* check the sizes are that supported? */
    len_whdr = hdr_len + len;
    if (len_whdr>allow_size)
    {
        NDRX_LOG(log_error, "Buffer too large for sending! "
                        "requested: %d, allowed: %d", len_whdr, allow_size);
        EXFAIL_OUT(ret);
    }

    /* !!! Prepare the buffer 
     * do we really need a copy???!
     * maybe just send 4 bytes and the other bytes...   
     
    memcpy(d+net->len_pfx, buf, len);
    * - moved to two step sending...
    */
    
    if (4==net->len_pfx)
    {
        /* Install the length prefix. */
        d[0] = (len_whdr >> 24) & 0xff;
        d[1] = (len_whdr >> 16) & 0xff;
        d[2] = (len_whdr >> 8) & 0xff;
        d[3] = (len_whdr) & 0xff;
        hdr_snd_size=net->len_pfx;
    }
    
    if (NULL!=hdr_buf)
    {
        /* add hdr to msg */
        memcpy(d+net->len_pfx, hdr_buf, hdr_len);
        hdr_snd_size+=hdr_len;
    }

    size_to_send = len+hdr_snd_size;

    /* Do sending in loop... */
    MUTEX_LOCK_V(net->sendlock);
    do
    {
        err = 0;
        NDRX_LOG(log_debug, "Sending, len: %d, total msg: %d", 
                size_to_send-sent, size_to_send);
        
        if (!(appflags & APPFLAGS_MASK))
        {
            if (sent < hdr_snd_size)
            {
                NDRX_DUMP(log_debug, "Sending, msg (msg len pfx)", 
                        d+sent, hdr_snd_size-sent);
            }
            else
            {
                NDRX_DUMP(log_debug, "Sending, msg ", buf+sent-hdr_snd_size, 
                            size_to_send-sent);
            }
        }
        else
        {
            NDRX_LOG(log_debug, "*** MSG DUMP IS MASKED ***");
        }
        
        ndrx_stopwatch_reset(&w);
        
        do
        {
            err = 0;
            retry = EXFALSE;
            
            if (sent<hdr_snd_size)
            {
                tmp_s = send(net->sock, d+sent, hdr_snd_size-sent, flags);
            }
            else
            {
                /* WARNING ! THIS MIGHT GENERATE SIGPIPE */
                tmp_s = send(net->sock, buf+sent-hdr_snd_size, 
                        size_to_send-sent, flags);
            }
            
            if (EXFAIL==tmp_s)
            {
                err = errno;
            }
            else
            {
                /* something is sent..., thus reset sent stopwatch */
                exnet_stopwatch_reset(net, &net->last_snd);
            }
            
            if (EAGAIN==err || EWOULDBLOCK==err)
            {
                int spent = ndrx_stopwatch_get_delta_sec(&w);
		int rcvtim = net->rcvtimeout - spent;
		struct pollfd ufd;

                spent = ndrx_stopwatch_get_delta_sec(&w);
		memset(&ufd, 0, sizeof ufd);

                NDRX_LOG(log_warn, "Socket full: %s - retry, "
                        "time spent: %d, max: %d - POLLOUT (rcvtim=%d) sent: %d tot: %d",
                        strerror(err), spent, net->rcvtimeout, rcvtim, sent, size_to_send);

		ufd.fd = net->sock; /* poll the read fd after the write fd is closed */
		ufd.events = POLLOUT;

		if (rcvtim < 1 || poll(&ufd, 1, rcvtim * 1000) < 0 || ufd.revents & POLLERR)
		{
                    NDRX_LOG(log_error, "ERROR! Failed to send, socket full: %s "
                            "time spent: %d, max: %d short: %hd rcvtim: %d", 
                        strerror(err), spent, net->rcvtimeout, ufd.revents, rcvtim);
                    
                    userlog("ERROR! Failed to send, socket full: %s "
                            "time spent: %d, max: %d short: %hd rcvtim: %d",
                        strerror(err), spent, net->rcvtimeout, ufd.revents, rcvtim);
                    
                    net->schedule_close = EXTRUE;
                    ret=EXFAIL;
                    goto out_unlock;
                }
                
                retry = EXTRUE;
                
            }
        }
        while (retry);

        if (EXFAIL==tmp_s)
        {
            NDRX_LOG(log_error, "send failure: %s",
                            strerror(err));
            /* close_socket(net); */
            
            NDRX_LOG(log_error, "Scheduling connection close...");
            net->schedule_close = EXTRUE;
            ret=EXFAIL;
            break;
        }
        else
        {
            NDRX_LOG(log_debug, "Sent %d bytes", tmp_s);
                        /* We should have sent something */
            sent+=tmp_s;
        }

        if (sent < size_to_send)
        {
            NDRX_LOG(log_debug, "partial submission: total: %d, sent: %d, left "
                    "for sending: %d - continue", size_to_send, sent, 
                    size_to_send - sent);
        }
        
    } 
    while (EXSUCCEED==ret && sent < size_to_send);

out_unlock:
    MUTEX_UNLOCK_V(net->sendlock);

out:
    
    return ret;
}

/**
 * Internal version of receive.
 * On error, it will do disconnect!
 * Receive is done by main thread only. Thus no we can close connection here directly.
 */
exprivate  ssize_t recv_wrap (exnetcon_t *net, void *__buf, size_t __n, int flags, int appflags)
{
    ssize_t ret;

    ret = recv (net->sock, __buf, __n, flags);

    if (0==ret)
    {
        NDRX_LOG(log_error, "Disconnect received - schedule close!");
        
        net->schedule_close = EXTRUE;
        ret=EXFAIL;
        goto out;
    }
    else if (EXFAIL==ret)
    {
        if (EAGAIN==errno || EWOULDBLOCK==errno)
        {
            NDRX_LOG(log_info, "Still no data (waiting...)");
        }
        else
        {
            NDRX_LOG(log_error, "recv failure: %s - schedule close", 
                    strerror(errno));
            
            net->schedule_close = EXTRUE;
        }

        ret=EXFAIL;
        goto out;
    }
    else
    {
        /* msg received */
        exnet_stopwatch_reset(net, &net->last_rcv);
    }
    
out:
    return ret;
}

/**
 * Get the length of message currently buffered.
 * We operate it little endian mode
 */
exprivate int get_full_len(exnetcon_t *net)
{
    int  pfx_len, msg_len;

    /* 4 bytes... */
    pfx_len = ( 
                (net->d[0] & 0xff) << 24
              | (net->d[1] & 0xff) << 16
              | (net->d[2] & 0xff) << 8
              | (net->d[3] & 0xff)
            );

    msg_len = pfx_len+net->len_pfx;
    NDRX_LOG(log_debug, "pfx_len=%d msg_len=%d", pfx_len, msg_len);
    
    /* TODO: if we got invalid len - close connection */

    return msg_len;
}

/**
 * Receive single message with prefixed length.
 * We will check peek the length bytes, and then
 * will request to receive full message
 * If we do poll on incoming messages, we shall receive from the same thread
 * because, otherwise it will alert for messages all the time or we need to
 * check some advanced flags..
 * On error we can close connection directly, because receive is done by main 
 * thread.
 * @return EXSUCCEED full msg received, EXFAIL no full msg 
 */
expublic int exnet_recv_sync(exnetcon_t *net, char **buf, int *len, int flags, int appflags)
{
    int ret=EXSUCCEED;
    int got_len;
    int full_msg;
    int download_size;
    
    /* Lock the stuff... */
    MUTEX_LOCK_V(net->rcvlock);
    
    if (0==net->dl)
    {
        /* This is new message */
        ndrx_stopwatch_reset(&net->rcv_timer);
    }
    
    if (NULL==net->dlsysbuf)
    {
        size_t tmp_buf_len;
        /* alloc the buffer... */
        NDRX_SYSBUF_MALLOC_OUT(net->dlsysbuf, tmp_buf_len, ret);
    }
    
    while (EXSUCCEED==ret)
    {
        /* Either we will timeout, or return by cut_out_msg! */
        if (net->dl>=net->len_pfx)
        {
            full_msg = get_full_len(net);
            
            if (full_msg < 0)
            {
                NDRX_LOG(log_error, "ERROR ! received len %d < 0 - closing socket!", 
                        full_msg);
                userlog("ERROR ! received len %d < 0! - schedule close socket!", full_msg);
                net->schedule_close = EXTRUE;
                ret=EXFAIL;
                break;
            }
            else if (full_msg > DATA_BUF_MAX)
            {
                NDRX_LOG(log_error, "ERROR ! received len %d > max buf %ld! - "
                        "closing socket!", full_msg, DATA_BUF_MAX);
                userlog("ERROR ! received len %d > max buf %ld! - "
                        "closing socket!", full_msg, DATA_BUF_MAX);
                net->schedule_close = EXTRUE;
                ret=EXFAIL;
                break;
            }

            NDRX_LOG(log_debug, "Data buffered - "
                    "buf: [%d], full: %d",
                    net->dl, full_msg);
            if (net->dl >= full_msg)
            {
                /* Copy msg out there & cut the buffer */
                /*
                ret=cut_out_msg(net, full_msg, buf, len, appflags);
                 * 
                 */
                /* return the msg */
                *len = full_msg-net->len_pfx;
                net->dl -= full_msg;
                *buf=net->dlsysbuf;
                net->dlsysbuf=NULL;
                /* should be 0! */
                
                if (0!=net->dl)
                {
                    NDRX_LOG(log_error, "Expected left over 0 but got %d", net->dl);
                    userlog("Expected left over 0 but got %d", net->dl);
                    abort();
                }
                
                if (0==*len)
                {
                    NDRX_LOG(log_debug, "zero length message - ignore!");
                    ret=EXFAIL;
                }
                else if (!(appflags & APPFLAGS_MASK))
                {
                    NDRX_DUMP(log_debug, "Got message: ", *buf, *len);
                }
                
                MUTEX_UNLOCK_V(net->rcvlock);
                return ret;
            }
        }

        NDRX_LOG(log_debug, "Data needs to be received, dl=%d", net->dl);
        
        /* well, lets receive only the message bytes,
         * and try to not receive any buffered stuff,
         * that would cause lots of memmove for large downloaded messages...
         */
        
        if (net->dl < net->len_pfx)
        {
            /* read some length + msg bytes... */
            download_size = net->len_pfx - net->dl;
        }
        else
        {
            /* OK we have full_msg, thus calculate the download size... */
            download_size = full_msg - net->dl;
        }
        
        /* we shall not get the  download_size < 0, this means data is buffered
         * but for some reason we are not processing them...
         * also download_size shall bigger than DBUF_SZ
         */
        
        if (download_size < 0)
        {
            NDRX_LOG(log_error, "ERROR ! Expected download size < 0 (%d)", download_size);
            userlog("ERROR ! Expected download size < 0 (%d)", download_size);
            net->schedule_close = EXTRUE;
            ret=EXFAIL;
            break;
        }
        else if (download_size > DBUF_SZ)
        {
            NDRX_LOG(log_error, "ERROR ! Expected download size bigger "
                    "than buffer left: %d > %d", download_size, (DBUF_SZ));
            userlog("ERROR ! Expected download size bigger "
                    "than buffer left: %d > %d", download_size, (DBUF_SZ));
            
            /* write lock? maybe not, as schedule_close will be set to EXFALSE
             * only after connection rest. Thus some race condition on setting
             * to EXTRUE, wont cause a problem.
             */
            net->schedule_close = EXTRUE;
            ret=EXFAIL;
            break;
        }
        
        /* 
         * select target buffer, either prefix receiving buf
         * our data receiving buf from the caller - to avoid extra memcopy....
         */
        if (net->dl < net->len_pfx)
        {
            if (EXFAIL==(got_len=recv_wrap(net, net->d+net->dl, download_size, 
                    flags, appflags)))
            {
                /* NDRX_LOG(log_error, "Failed to get data");*/
                ret=EXFAIL;
            }
            else
            {
                if (!(appflags&APPFLAGS_MASK))
                {
                    NDRX_DUMP(log_debug, "Got packet: ",
                            net->d+net->dl, got_len);
                }
                net->dl+=got_len;
            }
        }
        else
        {
            if (EXFAIL==(got_len=recv_wrap(net, net->dlsysbuf+net->dl-net->len_pfx, 
                    download_size, flags, appflags)))
            {
                /* NDRX_LOG(log_error, "Failed to get data");*/
                ret=EXFAIL;
            }
            else
            {
                if (!(appflags&APPFLAGS_MASK))
                {
                    NDRX_DUMP(log_debug, "Got packet: ",
                            net->dlsysbuf+net->dl-net->len_pfx, got_len);
                }
                net->dl+=got_len;
            }
        }
        
    }
    
    /* If message is not complete & there is timeout condition, 
     * then close conn & fail
     */
    /* well shouldn't this be write locked? */
    if (!net->schedule_close && 
            ndrx_stopwatch_get_delta_sec(&net->rcv_timer) >=net->rcvtimeout )
    {
        NDRX_LOG(log_error, "This is time-out => schedule close socket !");
        net->schedule_close = EXTRUE;
    }

out:    
    MUTEX_UNLOCK_V(net->rcvlock);
    
    /* We should fail anyway, because no message received, yet! */
    ret=EXFAIL;

    return ret;
}

/**
 * Run stuff before going to poll.
 * We might have a buffered message to process!
 */
expublic int exnet_b4_poll_cb(void)
{
    int ret=EXSUCCEED;
    exnetcon_t *head = extnet_get_con_head();
    exnetcon_t *net, *tmp;
    
    DL_FOREACH_SAFE(head, net, tmp)
    {
        if (exnet_schedule_run(net))
        {
            continue;
        }
    }

out:
    return ret;
}

/**
 * Poll have detected activity on FD
 */
expublic int exnet_poll_cb(int fd, uint32_t events, void *ptr1)
{
    int ret;
    int so_error=0;
    socklen_t len = sizeof so_error;
    exnetcon_t *net = (exnetcon_t *)ptr1;   /* Get the connection ptr... */
    /* char buf[DATA_BUF_MAX]; */
    int buflen = 0;
    char *buf = NULL;
    
    /* check schedule... */
    if (exnet_schedule_run(net))
    {
        goto out;
    }
    
    /* sysbuf alloc  - moved to recv...
    NDRX_SYSBUF_MALLOC_OUT(buf, NULL, ret);
     * */

    /* Receive the event of the socket */
    if (EXSUCCEED!=getsockopt(net->sock, SOL_SOCKET, SO_ERROR, &so_error, &len))
    {
        NDRX_LOG(log_error, "Failed go get getsockopt: %s",
                        strerror(errno));
        ret=EXFAIL;
        goto out;
    }

    if (0==so_error && !net->is_connected

#ifndef  EX_USE_KQUEUE
	&& events /* for Kqueue looks like no event is ok! */
#endif
	)
    {

        /* RW Lock! */
        
        exnet_rwlock_mainth_write(net);
        EXNET_CONNECTED(net);
        exnet_rwlock_mainth_read(net);
        
        NDRX_LOG(log_warn, "Connection is now open!");

        /* Call custom callback, if there is such */
        if (NULL!=net->p_connected && EXSUCCEED!=net->p_connected(net))
        {
            NDRX_LOG(log_error, "Connected notification "
                            "callback failed!");
            ret=EXFAIL;
            goto out;
        }

    }

    if (0==so_error && !net->is_connected && 
            ndrx_stopwatch_get_delta_sec(&net->connect_time) > net->rcvtimeout)
    {
        NDRX_LOG(log_error, "Cannot establish connection to server in "
                "time: %ld secs", ndrx_stopwatch_get_delta_sec(&net->connect_time));
        
        net->schedule_close = EXTRUE;
        
        goto out;
    }
    else if (0!=so_error)
    {
        if (!net->is_connected)
        {
            NDRX_LOG(log_error, "Failed to connect to server: %s",
                            strerror(so_error));
        }
        else
        {
            NDRX_LOG(log_error, "Socket client failed: %s",
                            strerror(so_error));
        }

        if (EINPROGRESS!=errno)
        {
            net->schedule_close = EXTRUE;
            
            /* Do not send fail to NDRX */
            goto out;
        }
    }
    else if (net->is_connected)
    {
        long rcvt;
        
        
        /* TODO: This shall be moved to main thread
         * as sending is not part of the receiving...
         * and shall be periodic
         */
        
        /* We are connected, send zero length message, ok 
         * Firstly: sending shall be done by worker thread
         * Secondly: send only in case if there was no data sent over the socket
         * - We need timer for incoming data, in case if data not received
         *  for twice of "periodic_zero" period, the connection shall be reset.
         */
        if (net->periodic_zero && 
                exnet_stopwatch_get_delta_sec(net, &net->last_snd) > net->periodic_zero)
        {
            NDRX_LOG(log_info, "About to issue zero length "
                    "message on fd %d", net->sock);

            /* Submit the job for sending to sender thread,
             * so that we do not lock-up the dispatching thread..
             */
            net->p_snd_zero_len(net);
        }
        
        if (net->recv_activity_timeout && 
                (rcvt=exnet_stopwatch_get_delta_sec(net, &net->last_rcv)) > net->recv_activity_timeout)
        {
            NDRX_LOG(log_error, "No data received in %ld sec (max with out data: %d) "
                    "reset soc/fd=%d", rcvt, net->recv_activity_timeout, net->sock);
            userlog("No data received in %ld sec (max with out data: %d) "
                    "reset soc/fd=%d", rcvt, net->recv_activity_timeout, net->sock);
            net->schedule_close = EXTRUE;
        }
    }

    /* Hmm try to receive something? */
#if defined(EX_USE_EPOLL)
    
    if (events & EPOLLIN)
        
#elif defined(EX_USE_KQUEUE)
        
    if (1) /* Process all events... of Kqueue */
        
#else
    if (events & POLLIN)
#endif
    {
        /* NDRX_LOG(6, "events & EPOLLIN => call exnet_recv_sync()"); */
/*        while(EXSUCCEED == exnet_recv_sync(net, buf, &buflen, 0, 0))*/
        if(EXSUCCEED == exnet_recv_sync(net, &buf, &buflen, 0, 0))
        {
            /* We got the message - do the callback op */
            net->p_process_msg(net, &buf, buflen);
        }
    }

out:

    /* remove the buffer if haven't already zapped by threads */
    if (NULL!=buf)
    {
        NDRX_SYSBUF_FREE(buf);
    }

    return EXSUCCEED;
}

/**
 * Close socket
 * This shall be called in write lock mode!
 */
exprivate int close_socket(exnetcon_t *net)
{
    int ret=EXSUCCEED;

    NDRX_LOG(log_warn, "Closing socket %d...", net->sock);
    net->dl = 0; /* Reset buffered bytes */
    
    net->is_connected=EXFALSE; /* mark disconnected. */
    
    if (EXFAIL!=net->sock)
    {
        /* Remove from polling structures */
        if (EXSUCCEED!=tpext_delpollerfd(net->sock))
        {
            NDRX_LOG(log_error, "Failed to remove polling extension: %s",
                                tpstrerror(tperrno));
        }

        /* shutdown & Close it */
        if (EXSUCCEED!=shutdown(net->sock, SHUT_RDWR))
        {
            NDRX_LOG(log_error, "Failed to shutdown socket: %s",
                                    strerror(errno));
        }

        if (EXSUCCEED!=close(net->sock))
        {
            NDRX_LOG(log_error, "Failed to close socket: %s",
                                    strerror(errno));
            ret=EXFAIL;
            goto out;
        }
    }

out:
    net->sock=EXFAIL;
    net->schedule_close = EXFALSE;
    
    if (NULL!=net->p_disconnected && EXSUCCEED!=net->p_disconnected(net))
    {
        NDRX_LOG(log_error, "Disconnected notification "
                        "callback failed!");
        ret=EXFAIL;
    }
#if 0
    /* Remove it from linked list, if it is incoming connection. */
    if (net->is_incoming)
    {
        exnet_del_con(net);
        /* If this was incoming, then do some server side work + do free as it did malloc! */
        exnet_remove_incoming(net);
    }
#endif
    
    if (net->is_incoming)
    {
        exnet_remove_incoming(net);
    }
    
    return ret;
}

/**
 * Configure socket / set standard options
 * @param net client/server handler (including listener)
 * @return EXSUCCEED/EXFAIL
 */
expublic int exnet_configure_setopts(exnetcon_t *net)
{
    int ret=EXSUCCEED;
    struct timeval tv;
    int flag = 1;
    int result;
    struct linger l;
    int enable = EXTRUE;

    /* We want to poll the stuff */
    if (EXFAIL==fcntl(net->sock, F_SETFL, O_NONBLOCK))
    {
        NDRX_LOG(log_error, "Failed set socket non blocking!: %s",
                                strerror(errno));
        EXFAIL_OUT(ret);
    }
    
    if (setsockopt(net->sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    {
        NDRX_LOG(log_error, "Failed to set SO_REUSEADDR: %s", strerror(errno));
        EXFAIL_OUT(ret);
    }
    

    if (EXFAIL==(result = setsockopt(net->sock,            /* socket affected */
                            IPPROTO_TCP,     /* set option at TCP level */
                            TCP_NODELAY,     /* name of option */
                            (char *) &flag,  /* the cast is historical
                                                    cruft */
                            sizeof(int))))    /* length of option value */
    {

        NDRX_LOG(log_error, "Failed set socket non blocking!: %s",
                                strerror(errno));
        EXFAIL_OUT(ret);
    }
    
    /* set no linger */
    l.l_onoff=0;
    l.l_linger=0;
    if(setsockopt(net->sock, SOL_SOCKET, SO_LINGER, &l, sizeof(l)) < 0)
    {
        NDRX_LOG(log_error, "Failed to set SO_LINGER: %s", strerror(errno));
        EXFAIL_OUT(ret);
    }
    

/* this is not workin on solaris 10... */
#if EX_OS_SUNOS && EX_LSB_RELEASE_VER_MAJOR == 5 && EX_LSB_RELEASE_VER_MINOR == 10
    NDRX_LOG(log_warn, "Solaris 10 - SO_RCVTIMEO not suppported.");
#else
    memset(&tv, 0, sizeof(tv));
    tv.tv_sec = net->rcvtimeout;
    NDRX_LOG(log_debug, "Setting SO_RCVTIMEO=%d", tv.tv_sec);
    if (EXSUCCEED!=setsockopt(net->sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval)))
    {
        NDRX_LOG(log_error, "setsockopt() failed for fd=%d: %s",
                        net->sock, strerror(errno));
        ret=EXFAIL;
        goto out;
    }
#endif
    
out:
    return ret;
}

/**
 * Extract port number
 * @param sa socket address 
 * @return port number
 */
expublic in_port_t exnet_get_port(struct sockaddr *sa)
{
    if (AF_INET == sa->sa_family)
    {
        return ntohs(((struct sockaddr_in*)sa)->sin_port);
    }
    else
    {
        return ntohs(((struct sockaddr_in6*)sa)->sin6_port);
    }
}

/**
 * Open socket
 */
exprivate int open_socket(exnetcon_t *net)
{
    int ret=EXSUCCEED;
    char ip[(INET6_ADDRSTRLEN)*2];
    
    /* Try to connect! */
    net->is_connected=EXFALSE;

    net->sock = socket(net->addr_cur->ai_family, 
            SOCK_STREAM, net->addr_cur->ai_protocol);

    /* Create socket for listening */
    if (EXFAIL==net->sock)
    {
        NDRX_LOG(log_error, "Failed to create socket: %s",
                                strerror(errno));
        ret=EXFAIL;
        goto out;
    }
    
    /* Configure socket */
    if (EXSUCCEED!=exnet_configure_setopts(net))
    {
        EXFAIL_OUT(ret);
    }

    if (NULL!=inet_ntop (net->addr_cur->ai_family, 
            &((struct sockaddr_in *)net->addr_cur->ai_addr)->sin_addr, ip, sizeof(ip)))
    {
        NDRX_LOG(log_info,"Trying to connect to IPv%d address: %s port: %d", 
                net->addr_cur->ai_family == PF_INET6 ? 6 : 4, ip, 
                (int)exnet_get_port(net->addr_cur->ai_addr));
    }
    else
    {
        NDRX_LOG(log_error, "Failed to extract address info: %s", strerror(errno));
    }

    if (EXSUCCEED!=connect(net->sock, net->addr_cur->ai_addr, 
            net->addr_cur->ai_addrlen))
    {
        NDRX_LOG(log_error, "connect() failed for fd=%d: %d/%s",
                        net->sock, errno, strerror(errno));
        
        if (ENETUNREACH==errno)
        {
            NDRX_LOG(log_error, "Try later to connect -> next ip");
            close(net->sock);
            net->sock=EXFAIL;
            goto out;
        }
        if (EINPROGRESS!=errno)
        {
            ret=EXFAIL;
            goto out;
        }
    }
    /* Take the time on what we try to connect. */
    ndrx_stopwatch_reset(&net->connect_time);

    /* Add stuff for polling */
    if (EXSUCCEED!=tpext_addpollerfd(net->sock,
            POLL_FLAGS,
            (void *)net, exnet_poll_cb))
    {
        NDRX_LOG(log_error, "tpext_addpollerfd failed!");
        ret=EXFAIL;
        goto out;
    }
    
out:
    return ret;
}

/**
 * Close connection
 * @param net network object
 * @return EXTRUE -> Connection removed, EXFALSE -> connection not removed
 */
exprivate int exnet_schedule_run(exnetcon_t *net)
{
    int is_incoming;
    
    if (net->schedule_close)
    {
        NDRX_LOG(log_warn, "Connection close is scheduled - "
                "closing fd %d is_incoming %d", 
                net->sock, net->is_incoming);
        is_incoming=net->is_incoming;
        
        exnet_rwlock_mainth_write(net);
        close_socket(net);
        exnet_rwlock_mainth_read(net);

        /* if incoming, continue.. */
        if (is_incoming)
        {
            /* remove connection 
            DL_DELETE(M_netlist, net);
            NDRX_FREE(net);*/
            return EXTRUE;
        }
    }
    
    return EXFALSE;
}

/**
 * Open socket is one is closed/FAIL.
 * This is periodic callback to the library from
 * NDRX polling extension
 */
expublic int exnet_periodic(void)
{
    int ret=EXSUCCEED;
    exnetcon_t *head = extnet_get_con_head();
    exnetcon_t *net, *tmp;
   
    DL_FOREACH_SAFE(head, net, tmp)
    {
        /* Check if close is scheduled... */
        if (exnet_schedule_run(net))
        {
            continue;
        }
        
        /* Only on connections... */
        if (EXFAIL==net->sock)
        {
            if (net->is_server)
            {
                /* if bind failed / try next address... */
                if (EXSUCCEED!=exnet_addr_next(net))
                {
                    NDRX_LOG(log_error, "Failed to resolve next binding address!");
                    EXFAIL_OUT(ret);
                }
                
                /* Server should bind at this point */
                ret = exnet_bind(net);
                
            }
            else if (!net->is_incoming)
            {
                if (EXSUCCEED!=exnet_addr_next(net))
                {
                    NDRX_LOG(log_error, "Failed to resolve next connect address!");
                    EXFAIL_OUT(ret);
                }
                /* Client should open socket at this point. */
                ret = open_socket(net);
            }
        }
        else if (!net->is_server)
        {
            /* Check connection.... */
            ret=exnet_poll_cb(net->sock, 0, (void *)net);
        }
    }

out:
    return ret;
}

/**
 * Set incoming message processing callback
 * @param p_process_msg 	callback on message received.
 * @param p_connected	called when we are connected
 * @param p_disconnected	called when we are disconnected
 * @param p_send_zero_len callback func for zero length msg sending
 *  (normally via thread pool)
 * @return EXSUCCEED 
*/
expublic int exnet_install_cb(exnetcon_t *net, int (*p_process_msg)(exnetcon_t *net, char **buf, int len),
		int (*p_connected)(exnetcon_t *net), int (*p_disconnected)(exnetcon_t *net),
                int (*p_snd_zero_len)(exnetcon_t *net))
{
    int ret=EXSUCCEED;

    net->p_process_msg = p_process_msg;
    net->p_connected = p_connected;
    net->p_disconnected = p_disconnected;
    net->p_snd_zero_len = p_snd_zero_len;

out:
    return ret;
}
/**
 * Destroy connection handler (free up resources)
 * This assumes that connection is closed
 * @param net handler to destroy/free up
 */
expublic void exnet_unconfigure(exnetcon_t *net)
{
    if (NULL!=net->addrinfos)
    {
        freeaddrinfo(net->addrinfos);
        net->addrinfos=NULL;
    }
    
    net->addr_cur=NULL;
}

/**
 * Get list of addresses...
 * @param net network config
 * @return EXSUCCEED/EXFAIL
 */
expublic int exnet_addr_get(exnetcon_t *net)
{
    int ret=EXSUCCEED;
    struct addrinfo hints;
    struct addrinfo *iter;
    char ip[(INET6_ADDRSTRLEN)*2];
    
    /* delete dynamic data... */
    exnet_unconfigure(net);
    
    if (!net->is_server)
    {
        NDRX_LOG(log_error, "EXNET: client for: %s:%s", net->addr, net->port);
    }
    else
    {
        NDRX_LOG(log_error, "EXNET: server for: %s:%s", net->addr, net->port);
    }
    
    /* Resolve address... */
    memset(&hints, 0, sizeof(struct addrinfo));
    
    if (net->is_ipv6)
    {
        hints.ai_family = AF_INET6;    /* Default Allow IPv4 */
    }
    else
    {
        hints.ai_family = AF_INET;    /* Default Allow IPv4 */
    }
    
    hints.ai_socktype = SOCK_STREAM; /* TCP Socket socket */
    
    if (net->is_server)
    {
        hints.ai_flags = AI_PASSIVE;
    }
    else
    {
        hints.ai_flags = 0;
    }
    
    /* allow numeric IP */
    if (net->is_numeric)
    {
        hints.ai_flags|=AI_NUMERICHOST;
    }
    
    hints.ai_protocol = 0; /* Any protocol */

    ret = getaddrinfo(net->addr, net->port, &hints, &net->addrinfos);
    
    if (ret != EXSUCCEED)
    {
        NDRX_LOG(log_error, "Failed to resolve -i addr: getaddrinfo(): %s", 
                gai_strerror(ret));
        EXFAIL_OUT(ret);
    }
    
    /* print all resolved addresses... */
    for(iter=net->addrinfos; iter != NULL;iter=iter->ai_next)
    {
        if (NULL!=inet_ntop (iter->ai_family, 
            &((struct sockaddr_in *)iter->ai_addr)->sin_addr, ip, sizeof(ip)))
        {
            NDRX_LOG(log_info,"Resolved: IPv%d address: %s port: %d", 
                    iter->ai_family == PF_INET6 ? 6 : 4, ip, 
                    (int)exnet_get_port(iter->ai_addr));
        }
        else
        {
            NDRX_LOG(log_error, "Failed to get addr info: %s", strerror(errno));
        }
    }
    
out:
    return ret;
}

/**
 * Select next address..
 * If we are at the end of address selection,
 * query addresses again...
 * 
 * @param net exnet_configure()
 * @return 
 */
expublic int exnet_addr_next(exnetcon_t *net)
{
    int ret = EXSUCCEED;
    
    if (NULL==net->addr_cur)
    {
        net->addr_cur=net->addrinfos;
    }
    else
    {
        net->addr_cur=net->addr_cur->ai_next;
    }
    
    if (NULL==net->addr_cur)
    {
        NDRX_LOG(log_warn, "Reload addresses");
        
        ret=exnet_addr_get(net);
        
        if (EXSUCCEED!=ret)
        {
            NDRX_LOG(log_error, "Failed to resolve bind/connect addresses!");
            EXFAIL_OUT(ret);
        }
        
        net->addr_cur=net->addrinfos;
    }
    
    if (NULL==net->addr_cur)
    {
        NDRX_LOG(log_error, "NULL Address found");
        EXFAIL_OUT(ret);
    }
    
out:
    NDRX_LOG(log_error, "exnet_addr_next returns %d", ret);

    return ret;
}

/**
 * Configure the library
 * @param net values for the socket must be pre-loaded from caller.
 * @return EXSUCCEED/EXFAIL
 */
expublic int exnet_configure(exnetcon_t *net)
{
    int ret = EXSUCCEED;
    
    if (EXSUCCEED!=exnet_addr_get(net))
    {
        EXFAIL_OUT(ret);
    }
    
    /* Add stuff to the list of connections */
    exnet_add_con(net);
    
out:
    return ret;
}

/**
 * Make read lock
 * @param net network object
 */
expublic void exnet_rwlock_read(exnetcon_t *net)
{
    if (EXSUCCEED!=pthread_rwlock_rdlock(&(net->rwlock)))
    {
        int err = errno;
        
        NDRX_LOG(log_error, "Failed to read lock: %s - exiting", strerror(err));
        userlog("Failed to read lock: %s  - exiting", strerror(err));
        exit(EXFAIL);
    }
}

/**
 * Make write lock
 * @param net network object
 */
expublic void exnet_rwlock_write(exnetcon_t *net)
{
    if (EXSUCCEED!=pthread_rwlock_rdlock(&net->rwlock))
    {
        int err = errno;
        
        NDRX_LOG(log_error, "Failed to write lock: %s - exiting", strerror(err));
        userlog("Failed to write lock: %s  - exiting", strerror(err));
        exit(EXFAIL);
    }
}

/**
 * Unlock the network object
 * @param net
 */
expublic void exnet_rwlock_unlock(exnetcon_t *net)
{
    if (EXSUCCEED!=pthread_rwlock_unlock(&net->rwlock))
    {
        int err = errno;
        
        NDRX_LOG(log_error, "Failed to unlock rwlock: %s - exiting", strerror(err));
        userlog("Failed to unlock rwlock: %s  - exiting", strerror(err));
        exit(EXFAIL);
    }
}

/**
 * Main thread about to write to net
 * This assumes that main thread already have read lock
 * @param net
 */
expublic void exnet_rwlock_mainth_write(exnetcon_t *net)
{
    exnet_rwlock_unlock(net);
    exnet_rwlock_write(net);
}

/**
 * Main thread switching back to read mode
 * @param net
 */
expublic void exnet_rwlock_mainth_read(exnetcon_t *net)
{
    exnet_rwlock_unlock(net);
    exnet_rwlock_read(net); /* lock back to read */
}

/**
 * Check are we connected?
 */
expublic int exnet_is_connected(exnetcon_t *net)
{
    return net->is_connected;
}

/**
 * Gently close the connection
 */
expublic int exnet_close_shut(exnetcon_t *net)
{
    if (net->lock_init)
    {
        exnet_rwlock_mainth_write(net);
        close_socket(net);
        exnet_rwlock_mainth_read(net);
    }

    return EXSUCCEED;
}

/**
 * Set custom timeout
 * @timeout timeout in seconds
 */
expublic int exnet_set_timeout(exnetcon_t *net, int timeout)
{
    int ret=EXSUCCEED;
    struct timeval tv;

    memset(&tv, 0, sizeof(tv));
    tv.tv_sec = timeout;

    if (EXSUCCEED!=setsockopt(net->sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval)))
    {
        NDRX_LOG(log_error, "setsockopt() failed for fd=%d: %s",
                        net->sock, strerror(errno));
        EXFAIL_OUT(ret);
    }

out:
    return ret;
}

/**
 * Reset network structure
 * @return 
 */
expublic void exnet_reset_struct(exnetcon_t *net)
{
    memset(net, 0, sizeof(*net));
    net->sock = EXFAIL;             /**< file descriptor for the network socket */    
    net->rcvtimeout = EXFAIL;       /**< Receive timeout                        */
    net->len_pfx = EXFAIL;          /**< Length prefix                          */    
    net->is_server = EXFAIL;        /**< server mode                            */
    net->rcvtimeout = ndrx_get_G_atmi_env()->time_out;
}

/**
 * Initialize network struct
 * Main thread acquires read lock by default
 * @param net networks struct to init
 * @return EXSUCCEED/EXFAIL
 */
expublic int exnet_net_init(exnetcon_t *net)
{
    int ret = EXSUCCEED;
    int err;
    
    memset(&(net->rwlock), 0, sizeof(net->rwlock));

    if (EXSUCCEED!=(err=pthread_rwlock_init(&(net->rwlock), NULL)))
    {
        NDRX_LOG(log_error, "Failed to init rwlock: %s", strerror(err));
        userlog("Failed to init rwlock: %s", strerror(err));
        EXFAIL_OUT(ret);
    }
    
    MUTEX_VAR_INIT(net->sendlock);
    MUTEX_VAR_INIT(net->rcvlock);
    MUTEX_VAR_INIT(net->flagslock);
    
    /* acquire read lock */
    if (EXSUCCEED!=(err=pthread_rwlock_rdlock(&(net->rwlock))))
    {
        userlog("Failed to acquire read lock: %s", strerror(err));
        NDRX_LOG(log_error, "Failed to acquire read lock - exiting...: %s",
                                                   strerror(err));
        exit(EXFAIL);
    }
    net->lock_init=EXTRUE;
            
out:
    return ret;
}
/* vim: set ts=4 sw=4 et smartindent: */
