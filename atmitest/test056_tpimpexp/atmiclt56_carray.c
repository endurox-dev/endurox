/* 
 * tpimport()/tpexport() function tests - client
 *
 * @file atmiclt56_carray.c
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
int test_impexp_carray()
{
    int ret = EXSUCCEED;
    int i;
    long rsplen,olen,ilen;
    char *obuf;
    char buf_bin[CARR_BUFFSIZE+1];
    char json_carray_in_b64[CARR_BUFFSIZE_B64+1];
    char json_carray_out_b64[CARR_BUFFSIZE_B64+1];
    size_t st_len, len_b64;
    char *json_carray_in = 
        "{"
            "\"buftype\":\"CARRAY\","
            "\"version\":1,"
            "\"data\":\"SEVMTE8gV09STEQgQ0FSUkFZ\""
        "}";
    char json_carray_out[1024];
    char *istrtemp=NULL;
    size_t bufsz = 0;

    NDRX_LOG(log_debug, "JSON CARRAY IN: [%s]", json_carray_in);

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

        NDRX_DUMP(log_debug, "Imported CARRAY", obuf, rsplen);

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

        NDRX_LOG(log_debug, 
                 "CARRAY exported. Return json_carray_out=[%s] olen=[%ld]", 
                 json_carray_out, olen);

        if (0!=strcmp(json_carray_in, json_carray_out))
        {
            
            NDRX_LOG(log_error, 
                 "TESTERROR: Exported JSON not equal to incoming carray");
            EXFAIL_OUT(ret);
        }
    }

    /* testing with base64 flag*/
    NDRX_LOG(log_debug, "convert to b64");
    if (NULL==ndrx_base64_encode((unsigned char *)json_carray_in, strlen(json_carray_in), &len_b64, json_carray_in_b64))
    {
            NDRX_LOG(log_error, "Failed to convert to b64!");
            EXFAIL_OUT(ret);
    }
    for (i=0; i<10000; i++)
    {
        rsplen=0L;
        if ( EXFAIL == tpimport(json_carray_in_b64, 
                                (long)len_b64, 
                                (char **)&obuf, 
                                &rsplen, 
                                TPEX_STRING) )
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to import JSON CARRAY!!!!");
            EXFAIL_OUT(ret);
        }

        NDRX_DUMP(log_debug, "Imported CARRAY [%s][%ld]", obuf, rsplen);

        memset(json_carray_out_b64, 0, sizeof(json_carray_out_b64));
        olen = sizeof(json_carray_out_b64);
        ilen = rsplen;
        if ( EXFAIL == tpexport(obuf, 
                                ilen, 
                                json_carray_out_b64, 
                                &olen, 
                                TPEX_STRING) )
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to export JSON CARRAY!!!!");
            EXFAIL_OUT(ret);
        }

        NDRX_LOG(log_debug, 
                 "CARRAY exported. Return json_carray_out_b64=[%s] olen=[%ld]", 
                 json_carray_out_b64, olen);

        /* decode from b64 to check returned data */
        bufsz = strlen(json_carray_out_b64);
        if (NULL==(istrtemp = NDRX_MALLOC(bufsz)))
        {
            NDRX_LOG(log_error, "Failed to allocate %ld bytes", strlen(json_carray_out_b64));
            EXFAIL_OUT(ret);
        }

        if (NULL==ndrx_base64_decode(json_carray_out_b64, strlen(json_carray_out_b64), &bufsz, istrtemp))
        {
            NDRX_LOG(log_error, "Failed to decode CARRAY");
            EXFAIL_OUT(ret);
        }
        istrtemp[bufsz]=0;

        if (0!=strcmp(json_carray_in, istrtemp))
        {
            NDRX_LOG(log_error, 
                "TESTERROR: \n"
                "json_carray_in=[%s]\n"
                "      istrtemp=[%s]", 
                json_carray_in, istrtemp);
            NDRX_LOG(log_error, 
                 "TESTERROR: Exported JSON not equal to incoming carray");
            EXFAIL_OUT(ret);
        }
    }

out:

    if (NULL!=istrtemp)
    {
        NDRX_FREE(istrtemp);
    }

    return ret;
}
