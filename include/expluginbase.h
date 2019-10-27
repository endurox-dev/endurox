/**
 * @brief Enduro/X Plugin Architecture
 *
 * @file expluginbase.h
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
#ifndef EXPLUGINBASE_H_
#define EXPLUGINBASE_H_
/*------------------------------Includes--------------------------------------*/
#include "ndrx_config.h"
#include <stdio.h>
/*------------------------------Externs---------------------------------------*/
/*------------------------------Macros----------------------------------------*/

#define NDRX_PLUGIN_PROVIDERSTR_BUFSZ        64


/**
 * Function: provides ndrx_plugin_crypto_getkey  
 */
#define NDRX_PLUGIN_FUNC_ENCKEY                     0x00000001
#define NDRX_PLUGIN_FUNC_TPLOGPRINTUBFMASK          0x00000002


/* symbols: */
#define NDRX_PLUGIN_INIT_SYMB                   "ndrx_plugin_init"

/** Cryptography key functionality available: */
#define NDRX_PLUGIN_CRYPTO_GETKEY_SYMB          "ndrx_plugin_crypto_getkey"

/** Load masking functionality expoed */
#define NDRX_PLUGIN_TPLOGPRINTUBFMASK_SYMB      "ndrx_plugin_tplogprintubf_mask"
/*------------------------------Enums-----------------------------------------*/
/*------------------------------Typedefs--------------------------------------*/

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

/**
 * Plugin for field printing to debug file of the UBF buffer
 * @param [in,out] buffer buffer which contains <field_name>\t<print_data>\n
 *  the buffer may be reallocated (the invoker does free)
 * @param [in] string length including EOS
 * @param [in] dataptr1 RFU
 * @param [in] do_write if set to TRUE, will print the given buffer in log file
 * @param [in] outf output file stream currently set
 * @param [in] fid UBF buffer field id
 * @return EXSUCCEED/EXFAIL
 */
typedef int (*ndrx_plugin_tplogprintubf_mask_t)(char **buffer, long datalen, void *dataptr1, 
        int *do_write, FILE * outf, int fid);


/* Have some global variable with pointer to callbacks */

struct ndrx_pluginbase {
    int plugins_loaded;
    
    /** pointer to get encryption key function */
    ndrx_plugin_crypto_getkey_t p_ndrx_crypto_getkey;
    /** provider string of crypto */
    char ndrx_crypto_getkey_provider[NDRX_PLUGIN_PROVIDERSTR_BUFSZ];
    
    /** UBF buffer dump to log file pre-processing function */
    ndrx_plugin_tplogprintubf_mask_t p_ndrx_tplogprintubf_mask;
    /** Provider string for the UBF dump pre-processing */
    char ndrx_tplogprintubf_mask_provider[NDRX_PLUGIN_PROVIDERSTR_BUFSZ];
};

typedef struct ndrx_pluginbase ndrx_pluginbase_t;

/*------------------------------Globals---------------------------------------*/
extern ndrx_pluginbase_t ndrx_G_plugins;
/*------------------------------Statics---------------------------------------*/
/*------------------------------Prototypes------------------------------------*/


extern NDRX_API int ndrx_plugins_load(void);

#endif /* EXPLUGINBASE_H_ */

/* vim: set ts=4 sw=4 et smartindent: */
