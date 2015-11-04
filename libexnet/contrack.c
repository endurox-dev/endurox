/* 
** Functions for tracking open connections.
** Contains doubly linked list.
** So we will track all incoming & outgoing connections, right?
**
** @file contrack.c
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
#include <sys/epoll.h>
#include <atmi.h>

#include <stdio.h>
#include <stdlib.h>

#include <exnet.h>
#include <ndrstandard.h>
#include <ndebug.h>

#include <utlist.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/* Doubly linked list with connections */
exnetcon_t *M_netlist = NULL;
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Add connection from linked list
 * @param 
 */
public void exnet_add_con(exnetcon_t *net)
{
    /* Add stuff to linked list */
    DL_APPEND(M_netlist, net);
}

/**
 * Delete connection from linked list. The node will not be deleted by it self.
 * Outside process shall do delete.
 * @param 
 */
public void exnet_del_con(exnetcon_t *net)
{
    /* delete stuff from linked list */
    DL_DELETE(M_netlist, net);   
}

/**
 * Get the head of the linked list.
 */
public exnetcon_t * extnet_get_con_head(void)
{
    return M_netlist;
}


