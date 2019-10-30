/**
 * @brief Test UBF hooking functionality
 *
 * @file hookplugin.c
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <nstdutil.h>
#include <expluginbase.h>
#include <sys_unix.h>
#include <test.fd.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Initialize the plugin
 * @param provider_name plugin name/provider string
 * @param provider_name_bufsz provider string buffer size
 * @return FAIL (in case of error) or OR'ed function flags
 */
expublic long ndrx_plugin_init(char *provider_name, int provider_name_bufsz)
{
    snprintf(provider_name, provider_name_bufsz, "UBF Hooking test");
    
    NDRX_LOG_EARLY(log_info, "plugin init...");
    
    /* we provide encryption key function */
    return NDRX_PLUGIN_FUNC_TPLOGPRINTUBF_HOOK;
}

/**
 * Perform the log file hooking.
 * @param buffer buffer where output ubf buffer string is located (one row field\tvalue\n
 * @param datalen output buffer len incl EOS
 * @param dataptr1 RFU
 * @param do_write if set to EXTRUE output will be logged by core to output stream
 * @param outf output stream
 * @param fid field id
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_plugin_tplogprintubf_hook(char **buffer, long datalen, void *dataptr1,
        int *do_write, FILE * outf, int fid)
{
    char *p;
    
    if (T_STRING_6_FLD==fid || T_STRING_7_FLD==fid)
    {
        p = strchr(*buffer, '\t');
        
        p++;
        while (*p)
        {
            if (T_STRING_6_FLD==fid)
            {
                *p='X';
            }
            else
            {
                *p='Y';
            }
            
            p++;
        }
        
        p--;
        *p='\n';
    }
    
    *do_write = EXTRUE;
    
    return EXSUCCEED;
}
        

/* vim: set ts=4 sw=4 et smartindent: */
