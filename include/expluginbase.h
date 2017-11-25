/* 
** Enduro/X Plugin Architecture
**
** @file expluginbase.h
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
#ifndef EXPLUGINBASE_H_
#define EXPLUGINBASE_H_

/*------------------------------Includes--------------------------------------*/
/*------------------------------Externs---------------------------------------*/
/*------------------------------Macros----------------------------------------*/

/* Define some mode flags:
 * 
 */

/*------------------------------Enums-----------------------------------------*/
/*------------------------------Typedefs--------------------------------------*/

/* Have some global variable with pointer to callbacks */

struct ndrx_pluginbase {
    int plugins_loaded;
    /* pointer to get encryption key function
     * @param key_out  buffer to store key in
     * @param key_out_bufsz buffer size of 'key_out'
     */
    int (*p_ndrx_crypto_getkey) (char *key_out, long key_out_bufsz);
};

typedef struct ndrx_pluginbase ndrx_pluginbase_t;

/*------------------------------Globals---------------------------------------*/
extern ndrx_pluginbase_t ndrx_G_plugins;
/*------------------------------Statics---------------------------------------*/
/*------------------------------Prototypes------------------------------------*/

#endif /* EXPROTO_H_ */
