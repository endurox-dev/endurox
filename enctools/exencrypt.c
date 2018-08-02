/* 
 * @brief Encrypt string
 *
 * @file exencrypt.c
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
    char encbuf[PATH_MAX];
    int i;
    
    /* pull in plugin loader.. */
    if (argc <= 1)
    {
        fprintf(stderr, "usage: %s <string_to_encrypt>\n", argv[0]);
        EXFAIL_OUT(ret);
    }
    
    for (i=1; i<argc; i++)
    {
        /* Pull-in plugins, by debug... */
        NDRX_LOG(6, "Encrypting [%s]", argv[i]);
        
        if (EXSUCCEED!=ndrx_crypto_enc_string(argv[i], encbuf, sizeof(encbuf)))
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

