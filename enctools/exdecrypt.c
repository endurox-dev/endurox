/**
 * @brief Decrypt string
 *
 * @file exdecrypt.c
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
    char clrbuf[PATH_MAX];
    int i;
    
    /* pull in plugin loader.. */
    if (argc <= 1)
    {
        fprintf(stderr, "usage: %s <string_to_encrypt>\n", argv[0]);
        EXFAIL_OUT(ret);
    }
    
    for (i=1; i<argc; i++)
    {
        NDRX_LOG(log_debug, "Decrypting [%s]", argv[i]);

        if (EXSUCCEED!=ndrx_crypto_dec_string(argv[i], clrbuf, sizeof(clrbuf)))
        {
            NDRX_LOG(log_error, "Failed to decrypt string: %s", Nstrerror(Nerror));
            fprintf(stderr, "Failed to decrypt string: %s\n", Nstrerror(Nerror));
            EXFAIL_OUT(ret);
        }

        fprintf(stdout, "%s\n", clrbuf);
    }
    
out:
    NDRX_LOG(log_debug, "program terminates with: %s", 
        EXSUCCEED==ret?"SUCCEED":"FAIL");
    return ret;
}
/* vim: set ts=4 sw=4 et smartindent: */
