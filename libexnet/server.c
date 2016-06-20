/* 
** This implements server part of network library.
**
** @file server.c
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

#ifdef EX_USE_EPOLL

#include <sys/epoll.h>

#else

#include <poll.h>

#endif
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

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

/**
 * About to remove incoming connection.
 */
public void exnet_remove_incoming(exnetcon_t *net)
{
    close(net->sock);
    net->my_server->incomming_cons--;
    NDRX_LOG(log_debug, "Open connections decreased to: %d", 
            net->my_server->incomming_cons);
    free(net);
}

/**
 * Server got something on the line!
 * @param net
 * @return 
 */
public int exnetsvpollevent(int fd, uint32_t events, void *ptr1)
{
    int ret=SUCCEED;
    exnetcon_t *srv = (exnetcon_t *)ptr1;
    struct sockaddr_in clt_address;
    int client_fd;
    char * ip_ptr;
    exnetcon_t *client = NULL;
    char *fn = "exnetsvpollevent";
    socklen_t clilen = sizeof(clt_address);
    
    NDRX_LOG(log_debug, "%s - enter", fn);
    
    if ( (client_fd = accept(srv->sock, (struct sockaddr *)&clt_address, &clilen) ) < 0 )
    {
        NDRX_LOG(log_error, "Error calling accept()");
        FAIL_OUT(ret);
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
    if (NULL==(client = malloc(sizeof(exnetcon_t))))
    {
        NDRX_LOG(log_error, "Failed to allocate client structure! %s", 
                strerror(errno));
        FAIL_OUT(ret);
    }
    
    memcpy(client, srv, sizeof(exnetcon_t));
    
    /* Update the structure */
    client->is_server=FALSE;
    client->is_incoming = TRUE;
    client->sock = client_fd;
    client->my_server = srv;
    client->is_connected = TRUE;
    memcpy(&client->address, &clt_address, sizeof(clt_address));
    
    /* We could get IP address & port of the call save in client struct & dump do log. */
    
    if((ip_ptr = inet_ntoa(clt_address.sin_addr)) < 0 )
    {
        NDRX_LOG(log_error, "Failed to get client IP address! %s", 
                strerror(errno));
        FAIL_OUT(ret);
    }
    strcpy(client->addr, ip_ptr);
    /*Get port number*/
    client->port = ntohs(clt_address.sin_port);
    NDRX_LOG(log_warn, "Got call from: %s:%u", client->addr, client->port);
    
    /* Call custom callback, if there is such */
    if (NULL!=client->p_connected && SUCCEED!=client->p_connected(client))
    {
        NDRX_LOG(log_error, "Connected notification "
                "callback failed!");
        FAIL_OUT(ret);
    }
    
    if (SUCCEED!=exnet_configure_client_sock(client))
    {
        NDRX_LOG(log_error, "Failed to configure client socket");
        FAIL_OUT(ret);
    }
    
    /* Install poller extension? */
    if (SUCCEED!=tpext_addpollerfd(client->sock,
        POLL_FLAGS,
        client, exnet_poll_cb))
    {
        NDRX_LOG(log_error, "tpext_addpollerfd failed!");
        ret=FAIL;
        goto out;
    }
    
    /* Add incoming connection to list of connections. */
    exnet_add_con(client);
    
out:

    if (SUCCEED!=ret && client)
    {
        free(client);
    }

    NDRX_LOG(log_debug, "%s - return %d", fn, ret);
    
    return ret;
}

/**
 * Server enters int listening state
 * @param net
 * @return 
 */
public int exnet_bind(exnetcon_t *net)
{
	int ret=SUCCEED;
    char *fn = "exnet_bind";
    
    NDRX_LOG(log_debug, "%s - enter", fn);
    
    if ( (net->sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
    {
        NDRX_LOG(log_error, "Failed to create socket: %s",
                                strerror(errno));
        FAIL_OUT(ret);
    }
    
    /*  Bind our socket addresss to the 
	   listening socket, and call listen()  */
    if ( bind(net->sock, (struct sockaddr *) &net->address, sizeof(net->address)) < 0 )
    {
        NDRX_LOG(log_error, "Error calling bind(): %s", strerror(errno));
        FAIL_OUT(ret);
            
    }
    
    if ( listen(net->sock, net->backlog) < 0 ) 
    {
        NDRX_LOG(log_error, "Error calling listen(): %s", 
                strerror(errno));
        FAIL_OUT(ret);
    }
    
    /* Install poller extension? */
    if (SUCCEED!=tpext_addpollerfd(net->sock,
        POLL_FLAGS,
        net, exnetsvpollevent))
    {
        NDRX_LOG(log_error, "tpext_addpollerfd failed!");
        ret=FAIL;
        goto out;
    }
    
out:

    NDRX_LOG(log_debug, "%s - return %d", fn, ret);

	return ret;
}
