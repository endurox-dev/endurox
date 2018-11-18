/**
 * @brief Cryptokey by hostname
 *
 * @file cryptohost.c
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <nstdutil.h>
#include <expluginbase.h>
#include <sys_unix.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
exprivate char M_hostname[PATH_MAX];
exprivate int M_hostname_first = EXTRUE;
/*---------------------------Prototypes---------------------------------*/

/**
 * Initialize the plugin
 * @param provider_name plugin name/provider string
 * @param provider_name_bufsz provider string buffer size
 * @return FAIL (in case of error) or OR'ed function flags
 */
expublic long ndrx_plugin_init(char *provider_name, int provider_name_bufsz)
{
    snprintf(provider_name, provider_name_bufsz, "Enduro/X Crypthost");
    
    NDRX_LOG_EARLY(log_info, "Cryptohost plugin init...");
    
    /* we provide encryption key function */
    return NDRX_PLUGIN_FUNC_ENCKEY;
}

/**
 * Get the encryption key
 * @param keybuf
 * @param keybuf_bufsz
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_plugin_crypto_getkey(char *keybuf, int keybuf_bufsz)
{
    int ret = EXSUCCEED;
    
    /* No problem if we run from mutliple threads..
     * the hostname is save anyway so bytes in mem should be put in the same order
     */
    if (M_hostname_first)
    {
        if (EXSUCCEED!=ndrx_sys_get_hostname(M_hostname, sizeof(M_hostname)))
        {
            NDRX_LOG_EARLY(log_error, "Failed to get hostname!");
            EXFAIL_OUT(ret);
        }
        
        M_hostname_first = EXTRUE;
    }
    
    NDRX_STRNCPY(keybuf, M_hostname, keybuf_bufsz);
    
out:
    return ret;
}
/* vim: set ts=4 sw=4 et smartindent: */
