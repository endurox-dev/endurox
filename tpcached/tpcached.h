/**
 * @brief Daemon support routines
 *
 * @file tpcached.h
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

#ifndef TPCACHED_H
#define	TPCACHED_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <cconfig.h>
#include <ndrx_config.h>
#include <exdb.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/    
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
    
/* we need a linked list of objects to remove */

typedef struct ndrx_tpcached_msglist ndrx_tpcached_msglist_t;
struct ndrx_tpcached_msglist
{
    int is_full;                /* is this full message? Or only key? */
    
    /* have a full key */
    EDB_val keydb;              /* stored key                         */
    
    /* have a full data */
    EDB_val val;                /* stored value (if is_full is TRUE)  */
    
    ndrx_tpcached_msglist_t *next;
};

/*---------------------------Prototypes---------------------------------*/

extern int ndrx_tpcached_add_msg(ndrx_tpcached_msglist_t ** list,
        EDB_val *keydb, EDB_val *val);
extern int ndrx_tpcached_kill_list(ndrx_tpcache_db_t *db, 
        ndrx_tpcached_msglist_t ** list);
extern void ndrx_tpcached_free_list(ndrx_tpcached_msglist_t ** list);


#ifdef	__cplusplus
}
#endif

#endif	/* TPCACHED_H */

/* vim: set ts=4 sw=4 et smartindent: */
