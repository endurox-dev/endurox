/**
 * @brief Functions for tracking open connections.
 *   Contains doubly linked list.
 *   So we will track all incoming & outgoing connections, right?
 *
 * @file contrack.c
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

#include <utlist.h>

#include "exhash.h"
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
expublic void exnet_add_con(exnetcon_t *net)
{
    /* Add stuff to linked list */
    DL_APPEND(M_netlist, net);
}

/**
 * Delete connection from linked list. The node will not be deleted by it self.
 * Outside process shall do delete.
 * @param 
 */
expublic void exnet_del_con(exnetcon_t *net)
{
    /* delete stuff from linked list */
    DL_DELETE(M_netlist, net);   
}

/**
 * Get the head of the linked list.
 */
expublic exnetcon_t * extnet_get_con_head(void)
{
    return M_netlist;
}

/**
 * Find a disconnected connection object to fill the connection details in
 * This assumes that we hold read lock on the object.
 * @return NULL (not dicon objs) or some obj
 */
expublic exnetcon_t *exnet_find_free_conn(void)
{
    exnetcon_t *net;
    
    DL_FOREACH(M_netlist, net)
    {
        if (!net->is_connected && !net->is_server)
        {
            return net;
        }
    }
    
    return NULL;
}/* vim: set ts=4 sw=4 et smartindent: */
/* vim: set ts=4 sw=4 et smartindent: */
