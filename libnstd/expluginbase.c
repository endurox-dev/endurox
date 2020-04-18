/**
 * @brief Plugin base
 *
 * @file expluginbase.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2019, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * AGPL (with Java and Go exceptions) or Mavimax's license for commercial use.
 * See LICENSE file for full text.
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
#include <ndrx_config.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

#include <ndrstandard.h>
#include <expluginbase.h>
#include <excrypto.h>
#include <inttypes.h>

#include "ndebug.h"
#include "userlog.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/** Offset definition for the function */
#define OFSZ(e)   EXOFFSET(ndrx_pluginbase_t,p_ndrx_##e), EXOFFSET(ndrx_pluginbase_t,ndrx_##e##_provider)
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/**
 * Plugin loader mapping
 */
struct plugin_loader_map
{
    char *symb;
    int func_off;
    int provider_off;
    long flags;
};
typedef struct plugin_loader_map plugin_loader_map_t;

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
    .plugins_loaded = EXFALSE
    
    /* Standard encryption encryption key function: */
    ,.p_ndrx_crypto_getkey = ndrx_crypto_getkey_std
    ,.ndrx_crypto_getkey_provider = "built in"
    
    /* UBF log print hooking */
    ,.p_ndrx_tplogprintubf_hook = NULL
    ,.ndrx_tplogprintubf_hook_provider = "none"
    };

/*---------------------------Statics------------------------------------*/

/**
 * Mapping driver
 */
exprivate plugin_loader_map_t M_map_driver[] =
{  
     {NDRX_PLUGIN_CRYPTO_GETKEY_SYMB,      OFSZ(crypto_getkey),        NDRX_PLUGIN_FUNC_ENCKEY}
    ,{NDRX_PLUGIN_TPLOGPRINTUBF_HOOK_SYMB, OFSZ(tplogprintubf_hook),   NDRX_PLUGIN_FUNC_TPLOGPRINTUBF_HOOK}
    ,{NULL}
};

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
    ndrx_plugin_init_t init;
    ndrx_plugin_crypto_getkey_t crypto;
    long flags;
    char provider[NDRX_PLUGIN_PROVIDERSTR_BUFSZ];
    
    plugin_loader_map_t *p = M_map_driver;
    
    
    handle = dlopen(fname, RTLD_LOCAL | RTLD_LAZY);
    
    if (NULL==handle)
    {
        NDRX_LOG_EARLY(log_error, "Failed to load [%s]: %s", fname, dlerror());
        EXFAIL_OUT(ret);
    }
    
    /* init the plugin */
    init = (ndrx_plugin_init_t)dlsym(handle, NDRX_PLUGIN_INIT_SYMB);
    
    if (NULL==init)
    {
        NDRX_LOG_EARLY(log_error, "Invalid plugin [%s] - symbol [%s] not found: %s",
                    fname, NDRX_PLUGIN_INIT_SYMB, dlerror());   
        userlog("Invalid plugin [%s] - symbol [%s] not found: %s",
                    fname, NDRX_PLUGIN_INIT_SYMB, dlerror());
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG_EARLY(log_debug, "About to call init: %p", init);
    
    flags = init(provider, sizeof(provider));
    
    if (EXFAIL==flags)
    {
        NDRX_LOG_EARLY(log_error, "Invalid plugin [%s] init failed1", fname);
        userlog("Invalid plugin [%s] init failed1", fname);
        EXFAIL_OUT(ret);
    }
    
    /* now check which functionality plugin supports... */
    
    NDRX_LOG_EARLY(log_info, "[%s] flags %lx", fname, flags);
    
    while (NULL!=p->symb)
    {
        if (flags & p->flags)
        {
            void *fptr = dlsym(handle, p->symb);
            void **func_ptr = (void *)(((char *)&ndrx_G_plugins) + p->func_off);
            char *prov_ptr = ((char *)&ndrx_G_plugins) + p->provider_off;
            
            if (NULL==fptr)
            {
                NDRX_LOG_EARLY(log_error, "Invalid plugin [%s] - symbol [%s] not "
                        "found (flags " PRIx64 "): %s",
                        fname, p->flags, flags, dlerror());
                userlog("Invalid plugin [%s] - symbol [%s] not "
                        "found (flags " PRIx64 "): %s",
                        fname, p->flags, flags, dlerror());
                EXFAIL_OUT(ret);
            }

            NDRX_LOG_EARLY(log_info, "Plugin [%s] provides [%s] function",
                    provider, p->symb);

            *func_ptr = fptr;
            NDRX_STRCPY_SAFE_DST(prov_ptr, provider, NDRX_PLUGIN_PROVIDERSTR_BUFSZ);
            
        }
        
        p++;
    }
    
out:
    if (EXSUCCEED!=ret && NULL!=handle)
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
    char *plugins_env = getenv(CONF_NDRX_PLUGINS);
    char *plugins = NULL;
    char *p;
    char *fname;
    char *save_ptr;
    
    if (NULL==plugins_env)
    {
        NDRX_LOG_EARLY(log_info, "No plugins defined by %s env variable", 
                CONF_NDRX_PLUGINS);
        /* nothing to do... */
        goto out;
    }
    
    /* Get the env and iterate it over... */
    
    plugins = NDRX_STRDUP(plugins_env);
    
    NDRX_LOG_EARLY(log_debug, "%s: loading plugins.... [%s]", __func__, plugins);
    
    p = strtok_r (plugins, ";", &save_ptr);
    
    while (NULL!=p)
    {
        /* trim down ltrim/rtrim the name */
        
        fname = ndrx_str_lstrip_ptr(p, " \t");
        ndrx_str_rstrip(fname, " \t");
        
        NDRX_LOG_EARLY(log_info, "About to load: [%s]", fname);
        
        /* Resolve symbols... */
        if (EXSUCCEED!=ndrx_plugins_loadone(fname))
        {
            userlog("Failed to load [%s] plugin...", fname);
        }
        
        p = strtok_r (NULL, ";", &save_ptr);
    }
    
out:
    if (NULL!=plugins)
    {
        NDRX_FREE(plugins);
    }
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
