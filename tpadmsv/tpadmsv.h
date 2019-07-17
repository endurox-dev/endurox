/**
 *
 * @file brdcstsv.h
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

#ifndef BRDCSTSV_H
#define	BRDCSTSV_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/**
 * Admin cursors open
 */
struct ndrx_adm_cursors
{
    char cursorid[32];          /**< Cached cursor id                       */
    char *memblock;             /**< Memory block                           */
    long items_tot;             /**< Total number of items                  */
    int curspos;                /**< Current cursor position                */
    long itemsize;              /**< Block size                             */
    
    ndrx_stopwatch_t w;         /**< Stopwatch when the cursor was open     */
    EX_hash_handle hh;          /**< makes this structure hashable          */
};

/**
 * Cursors type
 */
typedef struct ndrx_adm_cursors ndrx_adm_cursors_t;

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Open new cursor
 * @param clazz
 * @param data data to load into cursor
 * @return 
 */
expublic ndrx_adm_cursors_t* ndrx_adm_curs_new(char *clazz, ndrx_adm_cursors_t *data)
{
    return NULL;
}

/**
 * This shall find the cursor, and perform the buffer mappings according to tables
 *  and structures.
 *  The buffer shall be received and filled accordingly. If fetched till the end
 *  the data item shall be deleted.
 * @return 
 */
expublic int ndrx_adm_curs_fetch()
{
    
}

#ifdef	__cplusplus
}
#endif

#endif	/* BRDCSTSV_H */

/* vim: set ts=4 sw=4 et smartindent: */
