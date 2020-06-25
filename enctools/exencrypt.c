/**
 * @brief Encrypt string
 *
 * @file exencrypt.c
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
#include <memory.h>
#include <sys/param.h>
#include <unistd.h>
#include <ctype.h>


#include <ndrstandard.h>
#include <ndebug.h>
#include <excrypto.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Main Entry point..
 */
int main(int argc, char** argv)
{
    int ret = EXSUCCEED;
    char encbuf[PATH_MAX+1];
    char temp_buf[PATH_MAX+1];
    char *manual_argv[2] = {argv[0], temp_buf};
    char **enc_ptr = argv;
    int i;
    long len;
    
    /* pull in plugin loader.. */
    if (argc <= 1)
    {
        /*fprintf(stderr, "usage: %s <string_to_encrypt>\n", argv[0]);
        EXFAIL_OUT(ret);
         */
        if (EXSUCCEED!=ndrx_get_password("data to encrypt (e.g. password)", 
                temp_buf, sizeof(temp_buf)))
        {
            EXFAIL_OUT(ret);
        }
        else
        {
            enc_ptr = manual_argv;
            argc=2;
        }
    }
    
    for (i=1; i<argc; i++)
    {
        /* Pull-in plugins, by debug... */
        NDRX_LOG(6, "Encrypting [%s]", enc_ptr[i]);
        
        len=sizeof(encbuf);
        if (EXSUCCEED!=ndrx_crypto_enc_string(enc_ptr[i], encbuf, &len))
        {
            NDRX_LOG(log_error, "Failed to encrypt string: %s", Nstrerror(Nerror));
            fprintf(stderr, "Failed to encrypt string: %s\n", Nstrerror(Nerror));
            EXFAIL_OUT(ret);
        }

        fprintf(stdout, "%s\n", encbuf);
    }
    
out:
    NDRX_LOG(log_debug, "program terminates with: %s", 
        EXSUCCEED==ret?"SUCCEED":"FAIL");
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
