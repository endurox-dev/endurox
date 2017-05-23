/* 
** Enduro/X Network Access library (client/server)
**
** @file exnet.h
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
#ifndef EXNET_H_
#define EXNET_H_

/*------------------------------Includes--------------------------------------*/
#include <stdint.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <ntimer.h>
/*------------------------------Externs---------------------------------------*/
extern int G_recv_tout;				/* Was there timeout on recieve? */
/*------------------------------Macros----------------------------------------*/
#define	EXNET_ADDR_LEN		32		/* Max len for address	   */
#define DATA_BUF_MAX		(64*1024)	/* Have a data buffer, 32K */

#define APPFLAGS_MASK			0x0001	/* Mask the content in prod mode */
#define APPFLAGS_TOUT_OK		0x0002	/* Timeout is OK		 */

/*------------------------------Enums-----------------------------------------*/
/*------------------------------Typedefs--------------------------------------*/

/**
 * Connection descriptor.
 * This structure is universal. It is suitable for Server and client connections.
 */
typedef struct exnetcon exnetcon_t;
struct exnetcon
{
    /* General config: */
    u_short port;                 /* user specified port number             */
    char addr[EXNET_ADDR_LEN];    /* will be a pointer to the address 	  */
    struct sockaddr_in address;   /* the libc network address data structure*/
    int sock;              /* file descriptor for the network socket */    
    int is_connected;     /* Connection state...                    */
    int is_server;          /* Are we server or client? */
    int is_incoming;        /* Is connection incoming? */
    
    /* Client properties */
    exnetcon_t *my_server;  /* Pointer to listener structure, used by server, 
                             * in case if this was incomming connection */
    int rcvtimeout;        /* Receive timeout                        */
    char d[DATA_BUF_MAX];         /* Data buffer                            */
    int  dl;                   /* Data left in databuffer                */
    int recv_tout;              /* Last system call error for receive	  */
    int len_pfx;           /* Length prefix                          */
    ndrx_timer_t rcv_timer;        /* Receive timer...  */
    ndrx_timer_t connect_time;    /* Time of connection in transit..... */
    int periodic_zero;          /* send zero length message in seconds */
    ndrx_timer_t last_zero;        /* Last time send zero length message */
    
    /* Server settings */
    int backlog;            /* Incomming connection queue len (backlog) */
    int max_cons;           /* Max number of connections we will handle */
    int incomming_cons;     /* Current number of incomming connections */

    /* Have some callbacks... */
    int (*p_process_msg)(exnetcon_t *net, char *buf, int len); /* Callback when msg recived  */
    int (*p_connected)(exnetcon_t *net); 	/* Callback on even when we are connected */
    int (*p_disconnected)(exnetcon_t *net); 	/* Callback on even when we are disconnected */
    
    /* Stuff for linked list... */
    exnetcon_t *next, *prev;
};

/*------------------------------Globals---------------------------------------*/
/*------------------------------Statics---------------------------------------*/
/*------------------------------Prototypes------------------------------------*/
extern int exnet_send_sync(exnetcon_t *net, char *buf, int len, int flags, int appflags);
extern int exnet_recv_sync(exnetcon_t *net, char *buf, int *len, int flags, int appflags);

/* <Callback functions will be invoked by ndrxd extensions> */
extern int exnet_b4_poll_cb(void);
extern int exnet_poll_cb(int fd, uint32_t events, void *ptr1);
extern int exnet_periodic(void);
/* </Callback functions will be invoked by ndrxd extensions> */

extern int exnet_install_cb(exnetcon_t *net, int (*p_process_msg)(exnetcon_t *net, char *buf, int len),
		int (*p_connected)(exnetcon_t *net), int (*p_disconnected)(exnetcon_t *net));
extern int exnet_configure(exnetcon_t *net, int rcvtimeout, char *addr, short port, 
        int len_pfx, int is_server, int backlog, int max_cons, int periodic_zero);
extern int exnet_is_connected(exnetcon_t *net);
extern int exnet_close_shut(exnetcon_t *net);
extern int exnet_set_timeout(exnetcon_t *net, int timeout);
extern void exnet_reset_struct(exnetcon_t *net);
extern int exnet_configure_client_sock(exnetcon_t *net);
extern int exnet_bind(exnetcon_t *net);

/* Connection tracking: */
extern void exnet_add_con(exnetcon_t *net);
extern void exnet_del_con(exnetcon_t *net);
extern exnetcon_t * extnet_get_con_head(void);
extern void exnet_remove_incoming(exnetcon_t *net);

#endif /* EXNET_H_ */
