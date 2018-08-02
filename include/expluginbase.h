/* 
 * @brief Enduro/X Plugin Architecture
 *
 * @file expluginbase.h
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or ATR Baltic's license for commercial use.
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
#ifndef EXPLUGINBASE_H_
#define EXPLUGINBASE_H_

#include "ndrx_config.h"


/*------------------------------Includes--------------------------------------*/
/*------------------------------Externs---------------------------------------*/
/*------------------------------Macros----------------------------------------*/

#define NDRX_PLUGIN_PROVIDERSTR_BUFSZ        64


/**
 * Function: provides ndrx_plugin_crypto_getkey  
 */
#define NDRX_PLUGIN_FUNC_ENCKEY             0x00000001


/* symbols: */
#define NDRX_PLUGIN_INIT_SYMB               "ndrx_plugin_init"
#define NDRX_PLUGIN_CRYPTO_GETKEY_SYMB      "ndrx_plugin_crypto_getkey"
/*------------------------------Enums-----------------------------------------*/
/*------------------------------Typedefs--------------------------------------*/

double (*cosine)(double);


/**
 * Init plugin function
 */
typedef long (*ndrx_plugin_init_t)(char *provider_name, int provider_name_bufsz);

/**
 * Plugin encryption key func:
 * @param keybuf NUL terminated key
 * @param keybuf_bufsz
 */
typedef int (*ndrx_plugin_crypto_getkey_t)(char *keybuf, int keybuf_bufsz);


/* Have some global variable with pointer to callbacks */

struct ndrx_pluginbase {
    int plugins_loaded;
    /* pointer to get encryption key function */
    ndrx_plugin_crypto_getkey_t p_ndrx_crypto_getkey;
    char ndrx_crypto_getkey_provider[NDRX_PLUGIN_PROVIDERSTR_BUFSZ];
};

typedef struct ndrx_pluginbase ndrx_pluginbase_t;

/*------------------------------Globals---------------------------------------*/
extern ndrx_pluginbase_t ndrx_G_plugins;
/*------------------------------Statics---------------------------------------*/
/*------------------------------Prototypes------------------------------------*/


extern NDRX_API int ndrx_plugins_load(void);

#endif /* EXPROTO_H_ */
