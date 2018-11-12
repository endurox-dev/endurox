/* 
** tpimport()/tpexport() function tests - client
**
** @file atmiclt56.c
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>

#include <atmi.h>
#include <atmi_int.h>
#include <ndebug.h>
#include <ndrstandard.h>
#include <nstopwatch.h>
#include <fcntl.h>
#include <unistd.h>
#include <nstdutil.h>
#include "test56.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * test case to import/export STRING
 * @return EXSUCCEED/EXFAIL
 */
expublic int test_impexp_string()
{
    int ret = EXSUCCEED;
    int i;
    long rsplen,olen;
    char *obuf;
    char *json_string_in = 
        "{"
            "\"buftype\":\"STRING\","
            "\"version\":1,"
            "\"data\":\"HELLO WORLD\""
        "}";
    char json_string_out[1024];

    NDRX_LOG(log_info, "JSON STRING IN: [%s]", json_string_in);

    if (NULL==(obuf = tpalloc("STRING", NULL, 256)))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to alloc STRING buffer!");
        EXFAIL_OUT(ret);
    }

    for (i=0; i<10000; i++)
    {
        rsplen=0L;
        if ( EXFAIL == tpimport(json_string_in, 
                                (long)strlen(json_string_in), 
                                (char **)&obuf, 
                                &rsplen, 
                                0L) )
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to import JSON STRING!!!!");
            EXFAIL_OUT(ret);
        }
        NDRX_LOG(log_error, 
                 "JSON STRING imported. Return obuf=[%s] rsplen=[%ld]", 
                 obuf, rsplen);

        /* Check if imported sting match with original string */
        if ( 0!=strcmp("HELLO WORLD", obuf) )
        {
            NDRX_LOG(log_error, 
                 "TESTERROR: imported string not equal to obuf string ");
            EXFAIL_OUT(ret);
        }

        memset(json_string_out, 0, sizeof(json_string_out));
        olen = sizeof(json_string_out);
        if ( EXFAIL == tpexport(obuf, 
                                (long)strlen(obuf), 
                                json_string_out, 
                                &olen, 
                                0L) )
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to export JSON STRING!!!!");
            EXFAIL_OUT(ret);
        }
        NDRX_LOG(log_error, 
                 "JSON STRING exported. Return json_string_out=[%s] olen=[%ld]", 
                 json_string_out, olen);

        if (0!=strcmp(json_string_in, json_string_out))
        {
            NDRX_LOG(log_error, 
                 "TESTERROR: Exported JSON not equal to incoming string ");
            EXFAIL_OUT(ret);
        }

    }

out:

    return ret;
}
