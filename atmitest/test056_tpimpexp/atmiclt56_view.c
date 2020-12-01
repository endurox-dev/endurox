/**
 * @brief tpimport()/tpexport() function tests - client
 *
 * @file atmiclt56_view.c
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
#include <math.h>

#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>
#include <test.fd.h>
#include <ndrstandard.h>
#include <nstopwatch.h>
#include <fcntl.h>
#include <unistd.h>
#include <nstdutil.h>
#include <ubfutil.h>
#include <atmi_int.h>
#include "test56.h"
#include <extest.h>
#include <exbase64.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
/**
 * 
 * @return 
 */
expublic int test_impexp_view(void)
{
    int ret = EXSUCCEED;
    long rsplen, olen;
    char json_view_in_b64[CARR_BUFFSIZE_B64+1];
    char json_view_out_b64[CARR_BUFFSIZE_B64+1];
    size_t len_b64;
    int i;
    char *json_view_in = 
        "{"
            "\"buftype\":\"VIEW\","
            "\"version\":1,"
            "\"subtype\":\"UBTESTVIEW2\","
            "\"data\":"
            "{"
                "\"UBTESTVIEW2\":"
                "{"
                    "\"tshort1\":1,"
                    "\"tlong1\":2,"
                    "\"tchar1\":\"A\","
                    "\"tfloat1\":1,"
                    "\"tdouble1\":21,"
                    "\"tstring1\":\"ABC\","
                    "\"tcarray1\":\"SEVMTE8AAAAAAA==\""
                "}"
            "}"
        "}";
    char json_view_out[1024];
    struct UBTESTVIEW2 *v;
    char *istrtemp=NULL;
    size_t bufsz = 0;

    NDRX_LOG(log_info, "JSON VIEW IN: [%s]", json_view_in);
    v = (struct UBTESTVIEW2 *)tpalloc("VIEW", "UBTESTVIEW2", sizeof(struct UBTESTVIEW2));
    if (NULL == v)
    {
        NDRX_LOG(log_error, "Failed to allocate VIEW %ld bytes: %s",
                 NDRX_MSGSIZEMAX, tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    for (i=0; i<10000; i++)
    {
        rsplen=0L;
        if ( EXFAIL == tpimport(json_view_in, 
                                (long)strlen(json_view_in), 
                                (char **)&v, 
                                &rsplen, 
                                0L) )
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to import VIEW!!!!");
            EXFAIL_OUT(ret);
        }
        if (1 != v->tshort1)
        {
            NDRX_LOG(log_error, "TESTERROR: tshort1 got %ld expected 5555", 
                 v->tshort1);
            EXFAIL_OUT(ret);
        }
        if (2!=v->tlong1)
        {
            NDRX_LOG(log_error, "TESTERROR: tlong1 got %hd expected 2", 
                     v->tlong1);
            EXFAIL_OUT(ret);
        }
        if ('A'!=v->tchar1)
        {
            NDRX_LOG(log_error, "TESTERROR: tchar1 got %c expected A", 
                     v->tchar1);
            EXFAIL_OUT(ret);
        }
        if (fabs(v->tfloat1 - 1.0f) > 0.1)
        {
            NDRX_LOG(log_error, "TESTERROR: tfloat1 got %f expected 1", 
                     v->tfloat1);
            EXFAIL_OUT(ret);
        }
        if ((v->tdouble1 - 21.0f) > 0.1)
        {
            NDRX_LOG(log_error, "TESTERROR: tdouble1 got %lf expected 21", 
                     v->tdouble1);
            EXFAIL_OUT(ret);
        }
        if (0!=strcmp(v->tstring1, "ABC"))
        {
            NDRX_LOG(log_error, "TESTERROR: tstring1 got [%s] expected ABC", 
                     v->tdouble1);
            EXFAIL_OUT(ret);
        }

        memset(json_view_out, 0, sizeof(json_view_out));
        olen = sizeof(json_view_out);

        if ( EXFAIL == tpexport((char*)v, 
                                0L, 
                                json_view_out, 
                                &olen, 
                                0L) )
        {
                NDRX_LOG(log_error, "TESTERROR: Failed to export JSON VIEW!!!!");
                EXFAIL_OUT(ret);
        }
        NDRX_LOG(log_debug, 
                "VIEW exported. Return json_view_out=[%s] olen=[%ld]", 
                json_view_out, olen);

        if (0!=strcmp(json_view_in, json_view_out))
        {
            NDRX_LOG(log_error, 
                 "TESTERROR: Exported VIEW not equal to incoming VIEW ");
            EXFAIL_OUT(ret);
        }
    }

    /* testing with base64 flag*/
    NDRX_LOG(log_debug, "convert to b64");
    len_b64 = sizeof(json_view_in_b64);
    if (NULL==ndrx_base64_encode((unsigned char *)json_view_in, strlen(json_view_in), 
            &len_b64, json_view_in_b64))
    {
            NDRX_LOG(log_error, "Failed to convert to b64!");
            EXFAIL_OUT(ret);
    }

    for (i=0; i<10000; i++)
    {
        rsplen=0L;
        if ( EXFAIL == tpimport(json_view_in_b64, 
                                (long)strlen(json_view_in_b64), 
                                (char **)&v, 
                                &rsplen, 
                                TPEX_STRING) )
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to import VIEW!!!!");
            EXFAIL_OUT(ret);
        }
        if (1 != v->tshort1)
        {
            NDRX_LOG(log_error, "TESTERROR: tshort1 got %ld expected 5555", 
                 v->tshort1);
            EXFAIL_OUT(ret);
        }
        if (2!=v->tlong1)
        {
            NDRX_LOG(log_error, "TESTERROR: tlong1 got %hd expected 2", 
                     v->tlong1);
            EXFAIL_OUT(ret);
        }
        if ('A'!=v->tchar1)
        {
            NDRX_LOG(log_error, "TESTERROR: tchar1 got %c expected A", 
                     v->tchar1);
            EXFAIL_OUT(ret);
        }
        if (fabs(v->tfloat1 - 1.0f) > 0.1)
        {
            NDRX_LOG(log_error, "TESTERROR: tfloat1 got %f expected 1", 
                     v->tfloat1);
            EXFAIL_OUT(ret);
        }
        if ((v->tdouble1 - 21.0f) > 0.1)
        {
            NDRX_LOG(log_error, "TESTERROR: tdouble1 got %lf expected 21", 
                     v->tdouble1);
            EXFAIL_OUT(ret);
        }
        if (0!=strcmp(v->tstring1, "ABC"))
        {
            NDRX_LOG(log_error, "TESTERROR: tstring1 got [%s] expected ABC", 
                     v->tdouble1);
            EXFAIL_OUT(ret);
        }

        memset(json_view_out_b64, 0, sizeof(json_view_out_b64));
        olen = sizeof(json_view_out_b64);

        if ( EXFAIL == tpexport((char*)v, 
                                0L, 
                                json_view_out_b64, 
                                &olen, 
                                TPEX_STRING) )
        {
                NDRX_LOG(log_error, "TESTERROR: Failed to export JSON VIEW!!!!");
                EXFAIL_OUT(ret);
        }

        /* decode from b64 to check returned data */
        bufsz = strlen(json_view_out_b64);
        if (NULL==(istrtemp = NDRX_MALLOC(bufsz)))
        {
            NDRX_LOG(log_error, "Failed to allocate %ld bytes", strlen(json_view_out_b64));
            EXFAIL_OUT(ret);
        }

        if (NULL==ndrx_base64_decode(json_view_out_b64, strlen(json_view_out_b64), 
                &bufsz, istrtemp))
        {
            NDRX_LOG(log_error, "Failed to decode CARRAY");
            EXFAIL_OUT(ret);
        }
        istrtemp[bufsz]=0;

        if (0!=strcmp(json_view_in, istrtemp))
        {
            NDRX_LOG(log_error, 
                 "TESTERROR: Exported VIEW not equal to incoming VIEW ");
            EXFAIL_OUT(ret);
        }
        if (NULL!=istrtemp)
        {
            NDRX_FREE(istrtemp);
            istrtemp=NULL;
        }
   }

out:

    if (NULL!=istrtemp)
    {
        NDRX_FREE(istrtemp);
        istrtemp=NULL;
    }

return ret;
}
/* vim: set ts=4 sw=4 et smartindent: */
