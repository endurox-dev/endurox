/* 
** Enduro/X server net/socket client lib
** Network object needs to be synchronize otherwise unexpected core dumps might
** Locking:
** - main thread always have read lock
** - sender thread read lock only when doing send
** - when main thread wants some changes in net object, it waits for write lock
** - the connection object once created will always live in DL list even with
** with disconnected status. Once new conn arrives, it will re-use the conn obj.
**
** @file exnet.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, Mavimax, Ltd. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or Mavimax's license for commercial use.
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
** A commercial use license is available from Mavimax, Ltd
** contact@mavimax.com
** -----------------------------------------------------------------------------
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
#define DBUF_SZ	(NDRX_MSGSIZEMAX - net->dl)	/* Buffer size to recv in 	*/

#if defined(EX_USE_EPOLL)

#define POLL_FLAGS (EPOLLET | EPOLLIN | EPOLLHUP)

#elif defined(EX_USE_KQUEUE)

#define POLL_FLAGS EVFILT_READ

#else

#define POLL_FLAGS POLLIN

#endif

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

MUTEX_LOCKDECL(M_send_lock) 
MUTEX_LOCKDECL(M_recv_lock) 

/*---------------------------Prototypes---------------------------------*/
exprivate int close_socket(exnetcon_t *net);
exprivate int open_socket(exnetcon_t *net);
exprivate ssize_t recv_wrap (exnetcon_t *net, void *__buf, size_t __n, 
        int flags, int appflags);

exprivate int exnet_schedule_run(exnetcon_t *net);

/**
 * Send single message, put length in front
 * We will send all stuff required, do that in loop!
 */
expublic int exnet_send_sync(exnetcon_t *net, char *buf, int len, int flags, int appflags)
{
    int ret=EXSUCCEED;
    int allow_size = DATA_BUF_MAX-net->len_pfx;
    int sent = 0;
    char d[NET_LEN_PFX_LEN];	/* Data buffer, len     		   */
    int size_to_send;
    int tmp_s;
    int err;
    int retry;
    ndrx_stopwatch_t w;

    /* check the sizes are that supported? */
    if (len>allow_size)
    {
        NDRX_LOG(log_error, "Buffer too large for sending! "
                        "requested: %d, allowed: %d", len, allow_size);
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
        d[0] = (len >> 24) & 0xff;
        d[1] = (len >> 16) & 0xff;
        d[2] = (len >> 8) & 0xff;
        d[3] = (len) & 0xff;
    }

    size_to_send = len+net->len_pfx;

    /* Do sending in loop... */
    MUTEX_LOCK_V(net->sendlock);
    do
    {
        NDRX_LOG(log_debug, "Sending, len: %d, total msg: %d", 
                size_to_send-sent, size_to_send);
        
        if (!(appflags & APPFLAGS_MASK))
        {
            if (sent < net->len_pfx)
            {
                NDRX_DUMP(log_debug, "Sending, msg (msg len pfx)", 
                        d+sent, net->len_pfx-sent);
            }
            else
            {
                NDRX_DUMP(log_debug, "Sending, msg ", buf+sent-net->len_pfx, 
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
            
            if (sent<net->len_pfx)
            {
                tmp_s = send(net->sock, d+sent, net->len_pfx-sent, flags);
            }
            else
            {
                tmp_s = send(net->sock, buf+sent-net->len_pfx, 
                        size_to_send-sent, flags);
            }
            
            if (EXFAIL==tmp_s)
            {
                err = errno;
            }
            
            if (EAGAIN==err || EWOULDBLOCK==err)
            {
                int spent = ndrx_stopwatch_get_delta_sec(&w);
                NDRX_LOG(log_warn, "Socket full: %s - retry, time spent: %d, max: %d", 
                        strerror(err), spent, net->rcvtimeout);
                usleep(100000); /* sleep 0.1 sec on retry... */

                if (spent>=net->rcvtimeout)
                {
                    NDRX_LOG(log_error, "ERROR! Failed to send, socket full: %s "
                            "time spent: %d, max: %d", 
                        strerror(err), spent, net->rcvtimeout);
                    
                    userlog("ERROR! Failed to send, socket full: %s "
                            "time spent: %d, max: %d", 
                        strerror(err), spent, net->rcvtimeout);
                    
                    break;
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
        
    } while (EXSUCCEED==ret && sent < size_to_send);
    
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
        
        /*
        close_socket(net);
         */
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
            /* close_socket(net); */
            net->schedule_close = EXTRUE;
        }

        ret=EXFAIL;
        goto out;
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
 *  Cut the message from buffer & return it to caller!
 */
exprivate int cut_out_msg(exnetcon_t *net, int full_msg, char *buf, int *len, int appflags)
{
    int ret=EXSUCCEED;
    int len_with_out_pfx = full_msg-net->len_pfx;
    /* 1. check the sizes 			*/
    NDRX_LOG(log_debug, "Msg len with out pfx: %d  (full_msg: %d - pfx: %d), userbuf: %d",
                    len_with_out_pfx, full_msg, net->len_pfx, *len);
    if (*len < len_with_out_pfx)
    {
        NDRX_LOG(log_error, "User buffer to small: %d < %d",
                        *len, len_with_out_pfx);
        ret=EXFAIL;
        goto out;
    }

    /* 2. copy data to user buffer 		*/
    NDRX_LOG(log_debug, "Got message, len: %d", full_msg);

    if (!(appflags & APPFLAGS_MASK))
    {
        NDRX_DUMP(log_debug, "Got message: ", net->d, full_msg);
    }

    memcpy(buf, net->d+net->len_pfx, len_with_out_pfx);
    *len = len_with_out_pfx;

    /* 3. reduce the internal buffer 	*/
    memmove(net->d, net->d+full_msg, net->dl - full_msg);
    net->dl -= full_msg;

    NDRX_LOG(log_info, "net->dl = %d after cut", net->dl);
    
    if (0==*len)
    {
        NDRX_LOG(log_debug, "zero length message - ignore!");
        ret=EXFAIL;
    }

out:
    return ret;
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
 */
expublic int exnet_recv_sync(exnetcon_t *net, char *buf, int *len, int flags, int appflags)
{
    int ret=EXSUCCEED;
    int got_len;
    int full_msg;

    if (0==net->dl)
    {
        /* This is new message */
        ndrx_stopwatch_reset(&net->rcv_timer);
    }
    
    /* Lock the stuff... */
    MUTEX_LOCK_V(net->rcvlock)
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
                /* close_socket(net); */
                net->schedule_close = EXTRUE;
                ret=EXFAIL;
                break;
            }
            else if (full_msg > NDRX_MSGSIZEMAX)
            {
                NDRX_LOG(log_error, "ERROR ! received len %d > max buf %ld! - "
                        "closing socket!", full_msg, NDRX_MSGSIZEMAX);
                userlog("ERROR ! received len %d > max buf %ld! - "
                        "closing socket!", full_msg, NDRX_MSGSIZEMAX);
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
                ret=cut_out_msg(net, full_msg, buf, len, appflags);
                
                MUTEX_UNLOCK_V(net->rcvlock)
                return ret;
            }
        }

        NDRX_LOG(log_debug, "Data needs to be received, dl=%d", net->dl);
        if (EXFAIL==(got_len=recv_wrap(net, net->d+net->dl, DBUF_SZ, flags, appflags)))
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
    MUTEX_UNLOCK_V(net->rcvlock)
    
    /* We should fail anyway, because no message received, yet! */
    ret=EXFAIL;
out:

    /* If message is not complete & there is timeout condition, 
     * then close conn & fail
     */
    if (!net->schedule_close && 
            ndrx_stopwatch_get_delta_sec(&net->rcv_timer) >=net->rcvtimeout )
    {
        NDRX_LOG(log_error, "This is time-out => schedule close socket !");
        net->schedule_close = EXTRUE;
    }

    return ret;
}

/**
 * Run stuff before going to poll.
 * We might have a buffered message to process!
 */
expublic int exnet_b4_poll_cb(void)
{
    int ret=EXSUCCEED;
    char buf[DATA_BUF_MAX];
    int len = DATA_BUF_MAX;
    exnetcon_t *head = extnet_get_con_head();
    exnetcon_t *net, *tmp;
    
    DL_FOREACH_SAFE(head, net, tmp)
    {
        if (exnet_schedule_run(net))
        {
            continue;
        }
        
        if (net->dl>0)
        {
            NDRX_LOG(6, "exnet_b4_poll_cb - dl: %d", net->dl);
            if (EXSUCCEED == exnet_recv_sync(net, buf, &len, 0, 0))
            {
                /* We got the message - do the callback op */
                ret = net->p_process_msg(net, buf, len);
            }
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
    char buf[DATA_BUF_MAX];
    int buflen = DATA_BUF_MAX;
    
    /* check schedule... */
    if (exnet_schedule_run(net))
    {
        goto out;
    }

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
        net->is_connected = EXTRUE;
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
        /*close_socket(net);*/
        
        exnet_rwlock_mainth_write(net);
        net->schedule_close = EXTRUE;
        exnet_rwlock_mainth_read(net);
        
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
            /* close_socket(net); */
        
            exnet_rwlock_mainth_write(net);
            net->schedule_close = EXTRUE;
            exnet_rwlock_mainth_read(net);
            
            /* Do not send fail to NDRX */
            goto out;
        }
    }
    else if (net->is_connected)
    {
        /* We are connected, send zero length message, ok */
        if (net->periodic_zero && 
                ndrx_stopwatch_get_delta_sec(&net->last_zero) > net->periodic_zero)
        {
            NDRX_LOG(log_debug, "About to issue zero length "
                    "message on fd %d", net->sock);
            if (EXSUCCEED!=exnet_send_sync(net, NULL, 0, 0, 0))
            {
                NDRX_LOG(log_debug, "Failed to send zero length message!");
            }
            else
            {
                ndrx_stopwatch_reset(&net->last_zero);
            }
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
        while(EXSUCCEED == exnet_recv_sync(net, buf, &buflen, 0, 0))
        {
            /* We got the message - do the callback op */
            ret = net->p_process_msg(net, buf, buflen);
            buflen = DATA_BUF_MAX;
            /* NDRX_LOG(6, "events & EPOLLIN => loop call"); */
            /*on raspi seems we stuck here... thus if nothing to do, then terminate... */
            if (0 == net->dl)
            {
                NDRX_LOG(6, "events & EPOLLIN => dl=0, terminate loop");
                break;
            }
        }
    }

out:
    return EXSUCCEED;
}

/**
 * Close socket
 */
exprivate int close_socket(exnetcon_t *net)
{
    int ret=EXSUCCEED;

    NDRX_LOG(log_warn, "Closing socket...");
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

        /* Close it */
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
    
    return ret;
}

/**
 * Configure socket
 * @param net
 * @return 
 */
expublic int exnet_configure_client_sock(exnetcon_t *net)
{
    int ret=EXSUCCEED;
    struct timeval tv;
    int flag = 1;
    int result;
    
    /* We want to poll the stuff */
    if (EXFAIL==fcntl(net->sock, F_SETFL, O_NONBLOCK))
    {
        NDRX_LOG(log_error, "Failed set socket non blocking!: %s",
                                strerror(errno));
        ret=EXFAIL;
        goto out;
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
        ret=EXFAIL;
        goto out;
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
 * Open socket
 */
exprivate int open_socket(exnetcon_t *net)
{
    int ret=EXSUCCEED;
    /* Try to connect! */
    net->is_connected=EXFALSE;

    net->sock = socket(AF_INET, SOCK_STREAM, 0);

    /* Create socket for listening */
    if (EXFAIL==net->sock)
    {
        NDRX_LOG(log_error, "Failed to create socket: %s",
                                strerror(errno));
        ret=EXFAIL;
        goto out;
    }
    
    /* Configure socket */
    if (EXSUCCEED!=exnet_configure_client_sock(net))
    {
        EXFAIL_OUT(ret);
    }

    NDRX_LOG(log_debug, "Trying to connect to: %s:%d", net->addr, net->port);
    if (EXSUCCEED!=connect(net->sock, (struct sockaddr *)&net->address, sizeof(net->address)))
    {
        NDRX_LOG(log_error, "connect() failed for fd=%d: %d/%s",
                        net->sock, errno, strerror(errno));
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
 * @return EXTRUE -> Connection removed, EXFALSE -> connetion not removed
 */
exprivate int exnet_schedule_run(exnetcon_t *net)
{
    int is_incoming;
    
    if (net->schedule_close)
    {
        NDRX_LOG(log_warn, "Connection close is scheduled - closing fd %d", 
                net->sock);
        is_incoming=net->is_incoming;
        
        exnet_rwlock_mainth_write(net);
        close_socket(net);
        exnet_rwlock_mainth_read(net);

        /* if incoming, continue.. */
        if (is_incoming)
        {
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
                /* Server should bind at this point */
                ret = exnet_bind(net);               
            }
            else
            {
                /* Client should open socket at this point. */
                ret=open_socket(net);
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
 * @p_process_msg 	callback on message received.
 * @p_connected	called when we are connected
 * @p_disconnected	called when we are disconnected
 */
expublic int exnet_install_cb(exnetcon_t *net, int (*p_process_msg)(exnetcon_t *net, char *buf, int len),
		int (*p_connected)(exnetcon_t *net), int (*p_disconnected)(exnetcon_t *net))
{
    int ret=EXSUCCEED;

    net->p_process_msg = p_process_msg;
    net->p_connected = p_connected;
    net->p_disconnected = p_disconnected;

out:
    return ret;
}

/**
 * Configure the library
 */
expublic int exnet_configure(exnetcon_t *net, int rcvtimeout, char *addr, short port, 
        int len_pfx, int is_server, int backlog, int max_cons, int periodic_zero)
{
    int ret=EXSUCCEED;

    net->port = port;
    NDRX_STRCPY_SAFE(net->addr, addr);

    net->address.sin_family = AF_INET;
    net->address.sin_addr.s_addr = inet_addr(net->addr); /* assign the address */
    net->address.sin_port = htons(net->port);          /* translate int2port num */
    net->len_pfx = len_pfx;
    net->rcvtimeout = rcvtimeout;
    /* server settings: */
    net->backlog = backlog;
    net->max_cons = max_cons;
    net->periodic_zero = periodic_zero;
    
    if (!is_server)
    {
        NDRX_LOG(log_error, "EXNET: client for: %s:%u", net->addr, net->port);
    }
    else
    {
        net->is_server = EXTRUE;
        NDRX_LOG(log_error, "EXNET: server for: %s:%u", net->addr, net->port);
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
    if (EXFAIL!=net->sock)
    {
        if (EXSUCCEED!=shutdown(net->sock, SHUT_RDWR))
        {
            NDRX_LOG(log_info, "Failed to shutdown socket: %s",
                            strerror(errno));
        }
        
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
    net->sock = EXFAIL;              /* file descriptor for the network socket */    
    net->rcvtimeout = EXFAIL;         /* Receive timeout                        */
    net->len_pfx = EXFAIL;           /* Length prefix                          */    
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
    
    net->d = NDRX_MALLOC(DATA_BUF_MAX);
    
    if (NULL==net->d)
    {
        int err = errno;
        
        userlog("Failed to allocate client structure! %s", 
                strerror(err));
        
        NDRX_LOG(log_error, "Failed to allocate data block for client! %s", 
                strerror(err));
        
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=pthread_rwlock_init(&(net->rwlock), NULL))
    {
        NDRX_LOG(log_error, "Failed to init rwlock: %s", strerror(errno));
        EXFAIL_OUT(ret);
    }
    
    MUTEX_VAR_INIT(net->sendlock);
    MUTEX_VAR_INIT(net->rcvlock);
    
    /* acquire read lock */
    if (EXSUCCEED!=pthread_rwlock_rdlock(&(net->rwlock)))
    {
        userlog("Failed to acquire read lock!");
        NDRX_LOG(log_error, "Failed to acquire read lock - exiting... !");
        exit(EXFAIL);
    }
            
out:
    return ret;
}
