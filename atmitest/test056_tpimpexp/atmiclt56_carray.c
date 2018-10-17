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
#include <ubfutil.h>
#include <exbase64.h>
#include "test56.h"
#include "t56.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define CARR_BUFFSIZE       NDRX_MSGSIZEMAX
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * test case to import/export STRING
 * @return EXSUCCEED/EXFAIL
 */
int test_impexp_carray()
{
    int ret = EXSUCCEED;
    int i;
    long rsplen,olen,ilen;
    char *obuf;
    char buf_bin[CARR_BUFFSIZE+1];
    size_t st_len;
    char *json_carray_in = 
        "{"
            "\"buftype\":\"CARRAY\","
            "\"version\":1,"
//            "\"data\":\"AAECA0hFTExPIEJJTkFSWQQFAA==\""
            "\"data\":\"SEVMTE8gV09STEQgQ0FSUkFZ\""
        "}";
    char json_carray_out[1024];

    NDRX_LOG(log_info, "JSON CARRAY IN: [%s]", json_carray_in);

    if (NULL==(obuf = tpalloc("CARRAY", NULL, 128)))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to alloc CARRAY buffer!");
        EXFAIL_OUT(ret);
    }

    for (i=0; i<10000; i++)
    {
        rsplen=0L;
        if ( EXFAIL == tpimport(json_carray_in, 
                                (long)strlen(json_carray_in), 
                                (char **)&obuf, 
                                &rsplen, 
                                0L) )
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to import JSON CARRAY!!!!");
            EXFAIL_OUT(ret);
        }

        NDRX_DUMP(log_error, "Imported CARRAY", obuf, rsplen);

        memset(json_carray_out, 0, sizeof(json_carray_out));
        olen = sizeof(json_carray_out);
        ilen = rsplen;
        if ( EXFAIL == tpexport(obuf, 
                                ilen, 
                                json_carray_out, 
                                &olen, 
                                0L) )
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to export JSON CARRAY!!!!");
            EXFAIL_OUT(ret);
        }

        NDRX_LOG(log_error, 
                 "CARRAY exported. Return json_carray_out=[%s] olen=[%ld]", 
                 json_carray_out, olen);

        if (0!=strcmp(json_carray_in, json_carray_out))
        {
            NDRX_LOG(log_error, 
                 "TESTERROR: Exported JSON not equal to incoming carray");
            EXFAIL_OUT(ret);
        }
    }

out:

    return ret;
}
