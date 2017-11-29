/* 
** Plugin base
**
** @file expluginbase.c
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
#include <ndrx_config.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <ndrstandard.h>
#include <expluginbase.h>
#include <excrypto.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/

/**
 * Plugin directory:
 * How about sync here? Threads must be sure that plugins are loaded
 * maybe we need some read/write locks?
 * Or normally expect that tpinit() must took a place and only then plugins
 * are valid!
 * Some of them might be pre-initialized like standard crypto key
 */
expublic ndrx_pluginbase_t ndrx_G_plugins = {

    /* Is plugins loaded? */
    .plugins_loaded = EXFALSE,
    
    /* Standard encryption encryption key function: */
    .p_ndrx_crypto_getkey = ndrx_crypto_getkey_std,
    .ndrx_crypto_getkey_provider = "built in"
            
    };
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Load the plugins from the NDRX_PLUGINS - semicolon separated list of
 * dynamic link libraries. Once we load the lib, we call `long ndrx_init()'
 * This shall in case of success it must return the flags of which functionality
 * the library provides. Thus then we search for globals in the library and
 * send our pointer to it.
 * 
 * In case of failure the init shall return -1. We will print the warning to the
 * ULOG and skip the plugin.
 * 
 * @return 
 */
expublic int ndrx_plugins_load(void)
{
    int ret = EXSUCCEED;
    
    /* Get the env and interate it over... */
    
    
out:
    return ret;
}
