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
#include <dlfcn.h>

#include <ndrstandard.h>
#include <expluginbase.h>
#include <excrypto.h>

#include "ndebug.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/* same as xatmi.h" CONF_NDRX_PLUGINS */
#define NDRX_PLUGINS_ENV "NDRX_PLUGINS"
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
 * Load single plugin
 * @param name shared library name
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_plugins_loadone(char *fname)
{
    int ret = EXSUCCEED;
    void *handle;
    ndrx_plugin_init_t *init;
    long flags;
    char provider[NDRX_PLUGIN_PROVIDERSTR_BUFSZ];
    
    handle = dlopen(fname, RTLD_LOCAL | RTLD_NODELETE);
    
    if (NULL==handle)
    {
        NDRX_LOG_EARLY(log_error, "Failed to load [%s]: %s", fname, dlerror());
        EXFAIL_OUT(ret);
    }
    
    /* init the plugin */
    init = (ndrx_plugin_init_t *)dlsym(handle, NDRX_PLUGIN_INIT_SYMB);
    
    if (NULL==init)
    {
        NDRX_LOG_EARLY(log_error, "Invalid plugin [%s] - symbol [%s] not found: %s",
                    fname, NDRX_PLUGIN_INIT_SYMB, dlerror());
        userlog("Invalid plugin [%s] - symbol [%s] not found: %s",
                    fname, NDRX_PLUGIN_INIT_SYMB, dlerror());
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG_EARLY("About to call init: %p", init);
    
    flags = init(provider, sizeof(provider));
    
    if (EXFAIL==flags)
    {
        NDRX_LOG_EARLY(log_error, "Invalid plugin [%s] init failed1", fname);
        userlog("Invalid plugin [%s] init failed1", fname);
        EXFAIL_OUT(ret);
    }
    
    /* now check which functionality plugin supports... */
    
    if (flags & NDRX_PLUGIN_FUNC_ENCKEY)
    {
        
    }
    
out:
    if (NULL!=handle)
    {
        dlclose(handle);
    }
    return ret;
}

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
    char *plugins_env = getenv(NDRX_PLUGINS_ENV);
    char *plugins = NULL;
    char *p;
    char *fname;
    char *save_ptr;
    
    if (NULL==plugins)
    {
        NDRX_LOG_EARLY(log_debug, "%s: no plugins defined by %s", 
                __func__, NDRX_PLUGINS_ENV);
        goto out;
    }
    
    /* Get the env and interate it over... */
    
    plugins = strdup(plugins_env);
    
    NDRX_LOG_EARLY(log_debug, "%s: loading plugins.... [%s]", __func__, plugins);
    
    p = strtok_r (plugins, ";", &save_ptr);
    
    while (NULL!=p)
    {
        /* trim down ltrim/rtrim the name */
        
        fname = ndrx_str_lstrip_ptr(p, " \t");
        ndrx_str_rstrip(fname, " \t");
        
        NDRX_LOG_EARLY(log_info, "About to load: [%s]", fname);
        
        /* TODO: Resolve symbols... */
        
        
        
        strtok_r (NULL, ";", &save_ptr);
    }
    
out:
    return ret;
}

