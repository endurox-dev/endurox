/* 
** EnduroX server net/socket client lib
**
** @file exnet.c
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

#ifdef EX_USE_EPOLL

#include <sys/epoll.h>

#endif

#include <poll.h>

#include <atmi.h>
#include <stdio.h>
#include <stdlib.h>
#include <exnet.h>
#include <ndebug.h>
#include <utlist.h>

#include <atmi_int.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define DBUF_SZ	(sizeof(net->d) - net->dl)	/* Buffer size to recv in 	*/

#ifdef EX_USE_EPOLL

#define POLL_FLAGS (EPOLLET | EPOLLIN | EPOLLHUP)

#else

#define POLL_FLAGS POLLIN

#endif

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
private int close_socket(exnetcon_t *net);
private int open_socket(exnetcon_t *net);
private ssize_t recv_wrap (exnetcon_t *net, void *__buf, size_t __n, int flags, int appflags);

/**
 * Send single message, put length in front
 * We will send all stuff required, do that in loop!
 */
public int exnet_send_sync(exnetcon_t *net, char *buf, int len, int flags, int appflags)
{
    int ret=SUCCEED;
    int allow_size = DATA_BUF_MAX-net->len_pfx;
    int sent = 0;
    char d[DATA_BUF_MAX];	/* Data buffer				   */
    int size_to_send;
    int tmp_s;

    /* check the sizes are that supported? */
    if (len>allow_size)
    {
        NDRX_LOG(log_error, "Buffer too large for sending! "
                        "requested: %d, allowed: %d", len, allow_size);
        ret=FAIL;
        goto out;
    }

    /* Prepare the buffer */
    memcpy(d+net->len_pfx, buf, len);

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
    do
    {
        NDRX_LOG(log_debug, "Sending, len: %d", size_to_send-sent);
        if (!(appflags & APPFLAGS_MASK))
        {
            NDRX_DUMP(log_debug, "Sending, msg ", d+sent, size_to_send-sent);
        }
        else
        {
            NDRX_LOG(log_debug, "*** MSG DUMP IS MASKED ***");
        }
        tmp_s = send(net->sock, d+sent, size_to_send-sent, flags);

        if (FAIL==tmp_s)
        {
            NDRX_LOG(log_error, "send failure: %s",
                            strerror(errno));
            close_socket(net);
            ret=FAIL;
            goto out;
        }
        else
        {
            NDRX_LOG(log_debug, "Sent %d bytes", tmp_s);
                        /* We should have sent something */
            sent+=tmp_s;
        }

    } while (sent < size_to_send);

out:
	return ret;
}

/**
 * Internal version of receive.
 * On error, it will do disconnect!
 */
private  ssize_t recv_wrap (exnetcon_t *net, void *__buf, size_t __n, int flags, int appflags)
{
    ssize_t ret;

    /* Reset recv err */
    net->recv_tout =0;

    ret = recv (net->sock, __buf, __n, flags);

    if (0==ret)
    {
        NDRX_LOG(log_error, "Disconnect received!");
        close_socket(net);
        ret=FAIL;
        goto out;
    }
    else if (FAIL==ret)
    {
        if (EAGAIN==errno || EWOULDBLOCK==errno)
        {
                NDRX_LOG(log_error, "Still no data (waiting...)");
        }
        else
        {
            NDRX_LOG(log_error, "recv failure: %s", strerror(errno));
            close_socket(net);
        }

        ret=FAIL;
        goto out;
    }
    else
    {
        
    }
out:
	return ret;
}

/**
 * Get the length of message currently buffered.
 * We operate it little endian mode
 */
private int get_full_len(exnetcon_t *net)
{
    int  pfx_len, msg_len;

    /* 5 bytes... */
    pfx_len = ( 
                (net->d[0] & 0xff) << 24
              | (net->d[1] & 0xff) << 16
              | (net->d[2] & 0xff) << 8
              | (net->d[3] & 0xff)
            );

    msg_len = pfx_len+net->len_pfx;
    NDRX_LOG(log_debug, "pfx_len=%d msg_len=%d", pfx_len, msg_len);

    return msg_len;
}

/**
 *  Cut the message from buffer & return it to caller!
 */
private int cut_out_msg(exnetcon_t *net, int full_msg, char *buf, int *len, int appflags)
{
    int ret=SUCCEED;
    int len_with_out_pfx = full_msg-net->len_pfx;
    /* 1. check the sizes 			*/
    NDRX_LOG(log_debug, "Msg len with out pfx: %d, userbuf: %d",
                    len_with_out_pfx, *len);
    if (*len < len_with_out_pfx)
    {
            NDRX_LOG(log_error, "User buffer to small: %d < %d",
                            *len, len_with_out_pfx);
            ret=FAIL;
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
        ret=FAIL;
    }

out:
    return ret;
}

/**
 * Receive single message with prefixed length.
 * We will check peek the lenght bytes, and then
 * will request to receive full message
 */
public int exnet_recv_sync(exnetcon_t *net, char *buf, int *len, int flags, int appflags)
{
    int ret=SUCCEED;
    int got_len;
    int full_msg;

    if (0==net->dl)
    {
        /* This is new message */
        net->recv_tout = 0; /* Just in case reset... */
        ndrx_timer_reset(&net->rcv_timer);
    }
    
    while (SUCCEED==ret)
    {
        /* Either we will timeout, or return by cut_out_msg! */
        if (net->dl>=net->len_pfx)
        {
            full_msg = get_full_len(net);

            NDRX_LOG(log_debug, "Data buffered - "
                    "buf: [%d], full: %d",
                    net->dl, full_msg);
            if (net->dl >= full_msg)
            {
                NDRX_LOG(log_debug,
                    "Full msg in buffer");
                /* Copy msg out there & cut the buffer */
                return cut_out_msg(net, full_msg, buf, len, appflags);
            }
        }

        NDRX_LOG(log_debug, "Data needs to be received, dl=%d", net->dl);
        if (FAIL==(got_len=recv_wrap(net, net->d+net->dl, DBUF_SZ, flags, appflags)))
        {
            /* NDRX_LOG(log_error, "Failed to get data");*/
            ret=FAIL;
            goto out;
        }
        else
        {
            if (!(appflags&APPFLAGS_MASK))
            {
                NDRX_DUMP(log_debug, "Got packet: ",
                        net->d+net->dl, got_len);
            }
        }
        net->dl+=got_len;
    }
    
    /* We should fail anyway, because no message received, yet! */
    ret=FAIL;
out:

    /* If message is not complete & there is timeout condition, 
     * then close conn & fail
     */
    if (ndrx_timer_get_delta_sec(&net->rcv_timer) >=net->rcvtimeout )
    {
        NDRX_LOG(log_error, "This is time-out => closing socket !");
        close_socket(net);
    }

	return ret;
}

/**
 * Run stuff before going to poll.
 * We might have a buffered message to process!
 */
public int exnet_b4_poll_cb(void)
{
    int ret=SUCCEED;
    char buf[DATA_BUF_MAX];
    int len = DATA_BUF_MAX;
    exnetcon_t *head = extnet_get_con_head();
    exnetcon_t *net;
    
    DL_FOREACH(head, net)
    {
        if (net->dl>0)
        {
            NDRX_LOG(6, "exnet_b4_poll_cb - dl: %d", net->dl);
            if (SUCCEED == exnet_recv_sync(net, buf, &len, 0, 0))
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
public int exnet_poll_cb(int fd, uint32_t events, void *ptr1)
{
    int ret;
    int so_error=0;
    socklen_t len = sizeof so_error;
    exnetcon_t *net = (exnetcon_t *)ptr1;   /* Get the connection ptr... */
    char buf[DATA_BUF_MAX];
    int buflen = DATA_BUF_MAX;

    /* Receive the event of the socket */
    if (SUCCEED!=getsockopt(net->sock, SOL_SOCKET, SO_ERROR, &so_error, &len))
    {
        NDRX_LOG(log_error, "Failed go get getsockopt: %s",
                        strerror(errno));
        ret=FAIL;
        goto out;
    }

    if (0==so_error && !net->is_connected && events)
    {
        int arg;
        net->is_connected = TRUE;
        NDRX_LOG(log_warn, "Connection is now open!");

        /* Call custom callback, if there is such */
        if (NULL!=net->p_connected && SUCCEED!=net->p_connected(net))
        {
            NDRX_LOG(log_error, "Connected notification "
                            "callback failed!");
            ret=FAIL;
            goto out;
        }

    }

    if (0==so_error && !net->is_connected && 
            ndrx_timer_get_delta_sec(&net->connect_time) > net->rcvtimeout)
    {
        NDRX_LOG(log_error, "Cannot establish connection to server in "
                "time: %ld secs", ndrx_timer_get_delta_sec(&net->connect_time));
        close_socket(net);
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
            close_socket(net);
            /* Do not send fail to NDRX */
            goto out;
        }
    }
    else
    {
        /* We are connected, send zero lenght message, ok */
        if (net->periodic_zero && 
                ndrx_timer_get_delta_sec(&net->last_zero) > net->periodic_zero)
        {
            NDRX_LOG(log_debug, "About to issue zero length "
                    "message on fd %d", net->sock);
            if (SUCCEED!=exnet_send_sync(net, NULL, 0, 0, 0))
            {
                NDRX_LOG(log_debug, "Failed to send zero length message!");
            }
            else
            {
                ndrx_timer_reset(&net->last_zero);
            }
        }
    }

    /* Hmm try to receive something? */
#ifdef EX_USE_EPOLL
    if (events & EPOLLIN)
#else
    if (events & POLLIN)
#endif
    {
        /* NDRX_LOG(6, "events & EPOLLIN => call exnet_recv_sync()"); */
        while(SUCCEED == exnet_recv_sync(net, buf, &buflen, 0, 0))
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
	return SUCCEED;
}

/**
 * Close socket
 */
private int close_socket(exnetcon_t *net)
{
    int ret=SUCCEED;

    NDRX_LOG(log_warn, "Closing socket...");
    net->dl = 0; /* Reset buffered bytes */
    if (FAIL!=net->sock)
    {
        net->is_connected=FALSE;

        /* Remove from polling structures */
        if (SUCCEED!=tpext_delpollerfd(net->sock))
        {
            NDRX_LOG(log_error, "Failed to remove polling extension: %s",
                                tpstrerror(tperrno));
        }

        /* Close it */
        if (SUCCEED!=close(net->sock))
        {
            NDRX_LOG(log_error, "Failed to close socket: %s",
                                    strerror(errno));
            ret=FAIL;
            goto out;
        }
    }

out:
    net->sock=FAIL;

    if (NULL!=net->p_disconnected && SUCCEED!=net->p_disconnected(net))
    {
            NDRX_LOG(log_error, "Disconnected notification "
                            "callback failed!");
            ret=FAIL;
    }

    /* Remove it from linked list, if it is incoming connection. */
    if (net->is_incoming)
    {
        exnet_del_con(net);
        /* If this was incoming, then do some server side work + do free as it did malloc! */
        exnet_remove_incoming(net);
    }

    return ret;
}

/**
 * Configure socket
 * @param net
 * @return 
 */
public int exnet_configure_client_sock(exnetcon_t *net)
{
    int ret=SUCCEED;
    struct timeval tv;
    int flag = 1;
    int result;
    
    /* We want to poll the stuff */
    if (FAIL==fcntl(net->sock, F_SETFL, O_NONBLOCK))
    {
            NDRX_LOG(log_error, "Failed set socket non blocking!: %s",
                                    strerror(errno));
            ret=FAIL;
            goto out;
    }

    if (FAIL==(result = setsockopt(net->sock,            /* socket affected */
                            IPPROTO_TCP,     /* set option at TCP level */
                            TCP_NODELAY,     /* name of option */
                            (char *) &flag,  /* the cast is historical
                                                    cruft */
                            sizeof(int))))    /* length of option value */
    {

        NDRX_LOG(log_error, "Failed set socket non blocking!: %s",
                                strerror(errno));
        ret=FAIL;
        goto out;
    }

    memset(&tv, 0, sizeof(tv));
    tv.tv_sec = net->rcvtimeout;
    NDRX_LOG(log_debug, "Setting SO_RCVTIMEO=%d", tv.tv_sec);
    if (SUCCEED!=setsockopt(net->sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval)))
    {
            NDRX_LOG(log_error, "setsockopt() failed for fd=%d: %s",
                            net->sock, strerror(errno));
            ret=FAIL;
            goto out;
    }
    
out:
    return ret;
}

/**
 * Open socket
 */
private int open_socket(exnetcon_t *net)
{
    int ret=SUCCEED;
    /* Try to connect! */
    net->is_connected=FALSE;

    net->sock = socket(AF_INET, SOCK_STREAM, 0);

    /* Create socket for listening */
    if (FAIL==net->sock)
    {
        NDRX_LOG(log_error, "Failed to create socket: %s",
                                strerror(errno));
        ret=FAIL;
        goto out;
    }
    
    /* Configure socket */
    if (SUCCEED!=exnet_configure_client_sock(net))
    {
        FAIL_OUT(ret);
    }

    NDRX_LOG(log_debug, "Trying to connect to: %s:%d", net->addr, net->port);
    if (SUCCEED!=connect(net->sock, (struct sockaddr *)&net->address, sizeof(net->address)))
    {
        NDRX_LOG(log_error, "connect() failed for fd=%d: %d/%s",
                        net->sock, errno, strerror(errno));
        if (EINPROGRESS!=errno)
        {
                ret=FAIL;
                goto out;
        }
    }
    /* Take the time on what we try to connect. */
    ndrx_timer_reset(&net->connect_time);

    /* Add stuff for polling */
    if (SUCCEED!=tpext_addpollerfd(net->sock,
            POLL_FLAGS,
            (void *)net, exnet_poll_cb))
    {
        NDRX_LOG(log_error, "tpext_addpollerfd failed!");
        ret=FAIL;
        goto out;
    }
    
out:
    return ret;
}

/**
 * Open socket is one is closed/FAIL.
 * This is periodic callback to the library from
 * NDRX polling extension
 */
public int exnet_periodic(void)
{
    int ret=SUCCEED;
    exnetcon_t *head = extnet_get_con_head();
    exnetcon_t *net;

    DL_FOREACH(head, net)
    {
        /* Only on connections... */
        if (FAIL==net->sock)
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
public int exnet_install_cb(exnetcon_t *net, int (*p_process_msg)(exnetcon_t *net, char *buf, int len),
		int (*p_connected)(exnetcon_t *net), int (*p_disconnected)(exnetcon_t *net))
{
    int ret=SUCCEED;

    net->p_process_msg = p_process_msg;
    net->p_connected = p_connected;
    net->p_disconnected = p_disconnected;

out:
    return ret;
}

/**
 * Configure the library
 */
public int exnet_configure(exnetcon_t *net, int rcvtimeout, char *addr, short port, 
        int len_pfx, int is_server, int backlog, int max_cons, int periodic_zero)
{
    int ret=SUCCEED;

    net->port = port;
    strcpy(net->addr, addr);

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
        net->is_server = TRUE;
        NDRX_LOG(log_error, "EXNET: server for: %s:%u", net->addr, net->port);
    }
    
    /* Add stuff to the list of connections */
    exnet_add_con(net);
    
out:
    return ret;
}

/**
 * Check are we connected?
 */
public int exnet_is_connected(exnetcon_t *net)
{
	return net->is_connected;
}

/**
 * Gently close the connection
 */
public int exnet_close_shut(exnetcon_t *net)
{
    if (FAIL!=net->sock)
    {
        if (SUCCEED!=shutdown(net->sock, SHUT_RDWR))
        {
            NDRX_LOG(log_info, "Failed to shutdown socket: %s",
                            strerror(errno));
        }
        close_socket(net);
    }

    return SUCCEED;
}

/**
 * Set custom timeout
 * @timeout timeout in seconds
 */
public int exnet_set_timeout(exnetcon_t *net, int timeout)
{
    int ret=SUCCEED;
    struct timeval tv;

    memset(&tv, 0, sizeof(tv));
    tv.tv_sec = timeout;

    if (SUCCEED!=setsockopt(net->sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval)))
    {
        NDRX_LOG(log_error, "setsockopt() failed for fd=%d: %s",
                        net->sock, strerror(errno));
        FAIL_OUT(ret);
    }

out:
    return ret;
}

/**
 * Reset network structure
 * @return 
 */
public void exnet_reset_struct(exnetcon_t *net)
{
    memset(net, 0, sizeof(*net));
    net->sock = FAIL;              /* file descriptor for the network socket */    
    net->rcvtimeout = FAIL;         /* Receive timeout                        */
    net->len_pfx = FAIL;           /* Length prefix                          */    
}

