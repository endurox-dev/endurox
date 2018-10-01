/**
 * @brief This implements server part of network library.
 *
 * @file server.c
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

/*---------------------------Includes-----------------------------------*/
#include <ndrx_config.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <atmi.h>

#include <stdio.h>
#include <stdlib.h>

#include <exnet.h>
#include <ndrstandard.h>
#include <ndebug.h>
#include <userlog.h>

#if defined(EX_USE_EPOLL)

#include <sys/epoll.h>

#elif defined(EX_USE_KQUEUE)

#include <sys/event.h>

#else

#include <poll.h>

#endif
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

#if defined(EX_USE_EPOLL)

#define POLL_FLAGS (EPOLLET | EPOLLIN | EPOLLHUP)

#elif defined (EX_USE_KQUEUE)

#define POLL_FLAGS EVFILT_READ

#else

#define POLL_FLAGS POLLIN

#endif


/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * About to remove incoming connefction.
 */
expublic void exnet_remove_incoming(exnetcon_t *net)
{
    /* close(net->sock); Bug #233 - socket was already closed, core dumps on freebsd next */
    net->my_server->incomming_cons--;
    NDRX_LOG(log_debug, "Open connections decreased to: %d", 
            net->my_server->incomming_cons); 
    NDRX_FREE(net->d); /* remove network buffer */
    NDRX_FREE(net);
}

/**
 * Server got something on the line! Executed by main thread
 * @param net
 * @return 
 */
expublic int exnetsvpollevent(int fd, uint32_t events, void *ptr1)
{
    int ret=EXSUCCEED;
    exnetcon_t *srv = (exnetcon_t *)ptr1;
    struct sockaddr_in clt_address;
    int client_fd;
    char * ip_ptr;
    exnetcon_t *client = NULL;
    char *fn = "exnetsvpollevent";
    socklen_t addr_len = sizeof(clt_address);
    int is_new_con;
    
    /* COMMON SETUP between new connection and existing... */
#define CLT_COMMON_SETUP client->sock = client_fd;\
    client->schedule_close = EXFALSE;\
    client->is_connected = EXTRUE;\
    \
    /* We could get IP address & port of the call save in client struct \
     * & dump do log. \
     */\
    \
    if((ip_ptr = inet_ntoa(clt_address.sin_addr)) < 0 )\
    {\
        NDRX_LOG(log_error, "Failed to get client IP address! %s", \
                strerror(errno));\
        ret=EXFAIL;\
    }\
    \
    NDRX_STRCPY_SAFE(client->addr, ip_ptr);\
    /*Get port number*/\
    client->port = ntohs(clt_address.sin_port);\
    NDRX_LOG(log_warn, "Got call from: %s:%u", client->addr, client->port);\
    \
    if (EXSUCCEED!=exnet_configure_client_sock(client))\
    {\
        NDRX_LOG(log_error, "Failed to configure client socket");\
        ret=EXFAIL;\
    }
    /* COMMON SETUP between new connection and existing..., END */
    
    NDRX_LOG(log_debug, "%s - enter", fn);
    
    
    if ( (client_fd = accept(srv->sock, (struct sockaddr *)&clt_address, &addr_len) ) < 0 )
    {
        NDRX_LOG(log_error, "Error calling accept()");
        EXFAIL_OUT(ret);
    }

    NDRX_LOG(log_error, "New incoming connection: fd=%d", client_fd);

    if (srv->incomming_cons+1 > srv->max_cons)
    {
        NDRX_LOG(log_error, "Too many open connections: %d/%d - closing down fd %d", 
                srv->incomming_cons+1, srv->max_cons, client_fd);
        close(client_fd);
        /*Nothing to do than go out...*/
        goto out;
    }
    srv->incomming_cons++;
    NDRX_LOG(log_debug, "Open connections increased to: %d", 
            srv->incomming_cons);
    
    /* Allocate client connection structure! 
     * We could take a copy of server structure + do some modifications to it */
    
    /* Search for connection existence... - basically needs to find free connection */
    if (NULL!=(client=exnet_find_free_conn()))
    {
        NDRX_LOG(log_info, "Reusing connection object");
        is_new_con = EXFALSE;
    }
    else
    {
        NDRX_LOG(log_info, "Creating new connection object");
        is_new_con = EXTRUE;
    }
    
    if (is_new_con)
    {
        if (NULL==(client = NDRX_MALLOC(sizeof(exnetcon_t))))
        {
            int err = errno;

            userlog("Failed to allocate client structure! %s", 
                    strerror(err));

            NDRX_LOG(log_error, "Failed to allocate client structure! %s", 
                    strerror(err));
            EXFAIL_OUT(ret);
        }

        memcpy(client, srv, sizeof(exnetcon_t));

        /* Update the structure */
        client->is_server=EXFALSE;
        client->is_incoming = EXTRUE;
        client->my_server = srv;
        client->is_connected = EXTRUE;
        
        memcpy(&client->address, &clt_address, sizeof(clt_address));
        
        
        CLT_COMMON_SETUP;
        
        /* trap an error */
        if (EXSUCCEED!=ret)
        {
            goto out;
        }

        if (EXSUCCEED!=exnet_net_init(client))
        {
            NDRX_LOG(log_error, "Failed to init client!");
            EXFAIL_OUT(ret);
        }
    }
    else
    {
        exnet_rwlock_mainth_write(client);
        
        CLT_COMMON_SETUP;
        
        exnet_rwlock_mainth_read(client);
        
        /* trap an error */
        if (EXSUCCEED!=ret)
        {
            goto out;
        }
    }
    
    
    /* Call custom callback, if there is such */
    if (NULL!=client->p_connected && EXSUCCEED!=client->p_connected(client))
    {
        NDRX_LOG(log_error, "Connected notification "
                "callback failed!");
        EXFAIL_OUT(ret);
    }
    
    /* Install poller extension? */
    if (EXSUCCEED!=tpext_addpollerfd(client->sock,
        POLL_FLAGS,
        client, exnet_poll_cb))
    {
        NDRX_LOG(log_error, "tpext_addpollerfd failed!");
        ret=EXFAIL;
        goto out;
    }
    
    if (is_new_con)
    {
        NDRX_LOG(log_info, "Adding connection to hashlist");
        /* Add incoming connection to list of connections. */
        exnet_add_con(client);
    }
    
out:

    if (EXSUCCEED!=ret && client)
    {
        if (NULL!=client->d)
        {
            NDRX_FREE(client->d);
        }
        
        NDRX_FREE(client);
    }

    NDRX_LOG(log_debug, "%s - return %d", fn, ret);
    
    return ret;
}

/**
 * Server enters int listening state
 * @param net
 * @return 
 */
expublic int exnet_bind(exnetcon_t *net)
{
	int ret=EXSUCCEED;
    char *fn = "exnet_bind";
    int enable = EXTRUE;
    
    NDRX_LOG(log_debug, "%s - enter", fn);
    
    if ( (net->sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
    {
        NDRX_LOG(log_error, "Failed to create socket: %s",
                                strerror(errno));
        EXFAIL_OUT(ret);
    }

    if (setsockopt(net->sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    {
        NDRX_LOG(log_error, "Failed to set SO_REUSEADDR: %s", strerror(errno));
        EXFAIL_OUT(ret);
    }
    
    /*  Bind our socket addresss to the 
	   listening socket, and call listen()  */
    if ( bind(net->sock, (struct sockaddr *) &net->address, sizeof(net->address)) < 0 )
    {
        NDRX_LOG(log_error, "Error calling bind(): %s", strerror(errno));
        EXFAIL_OUT(ret);
            
    }

    if ( listen(net->sock, net->backlog) < 0 ) 
    {
        NDRX_LOG(log_error, "Error calling listen(): %s", 
                strerror(errno));
        EXFAIL_OUT(ret);
    }
    
    /* Install poller extension? */
    if (EXSUCCEED!=tpext_addpollerfd(net->sock,
        POLL_FLAGS,
        net, exnetsvpollevent))
    {
        NDRX_LOG(log_error, "tpext_addpollerfd failed!");
        ret=EXFAIL;
        goto out;
    }
    
out:

    NDRX_LOG(log_debug, "%s - return %d", fn, ret);

    return ret;
}
/* vim: set ts=4 sw=4 et smartindent: */
