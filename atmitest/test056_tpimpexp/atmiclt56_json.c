/* 
 * tpimport()/tpexport() function tests - client
 *
 * @file atmiclt56_json.c
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
#include <ubf.h>
#include <ndebug.h>
#include <test.fd.h>
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

expublic int test_impexp_json()
{
    int ret = EXSUCCEED;
    char type[16+1]={EXEOS};
    char subtype[XATMI_SUBTYPE_LEN]={EXEOS};
    long rsplen, olen;
    char json_json_in_b64[CARR_BUFFSIZE_B64+1];
    char json_json_out_b64[CARR_BUFFSIZE_B64+1];
    size_t len_b64;
    int i;
    char *obuf=NULL;
    char *data_test = 
                "{\"T_SHORT_FLD\":1765,"
                "\"T_LONG_FLD\":[3333111,2],"
                "\"T_CHAR_FLD\":\"A\","
                "\"T_FLOAT_FLD\":1,"
                "\"T_DOUBLE_FLD\":[1111.220000,333,444],"
                "\"T_STRING_FLD\":\"HELLO WORLD\","
                "\"T_CARRAY_FLD\":\"AAECA0hFTExPIEJJTkFSWQQFAA==\"}";

    char *json_json_in = 
        "{"
            "\"buftype\":\"JSON\","
            "\"version\":1,"
            "\"data\":"
                "{\"T_SHORT_FLD\":1765,"
                "\"T_LONG_FLD\":[3333111,2],"
                "\"T_CHAR_FLD\":\"A\","
                "\"T_FLOAT_FLD\":1,"
                "\"T_DOUBLE_FLD\":[1111.220000,333,444],"
                "\"T_STRING_FLD\":\"HELLO WORLD\","
                "\"T_CARRAY_FLD\":\"AAECA0hFTExPIEJJTkFSWQQFAA==\"}"
        "}";

    char json_json_out[1024];
    char *istrtemp=NULL;
    size_t bufsz = 0;

    NDRX_LOG(log_info, "JSON IN: [%s]", json_json_in);
        rsplen=0L;

    for (i=0; i<10000; i++)
    {
        if ( EXFAIL == tpimport(json_json_in, 
                                (long)strlen(json_json_in), 
                                (char **)&obuf, 
                                &rsplen, 
                                0L) )
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to import JSON!!!!");
            EXFAIL_OUT(ret);
        }
        NDRX_LOG(log_debug, "JSON imported. Return obuf=[%s] rsplen=[%ld]", 
                                                    obuf, rsplen);

        if (0!=strcmp(data_test, obuf))
        {
            NDRX_LOG(log_error, 
                 "TESTERROR: imported JSON not equal to obuf string ");
            EXFAIL_OUT(ret);
        }

        memset(json_json_out, 0, sizeof(json_json_out));
        olen = sizeof(json_json_out);

        if ( EXFAIL == tpexport(obuf, 
                                (long)strlen(obuf),
                                json_json_out, 
                                &olen, 
                                0L) )
        {
                NDRX_LOG(log_error, "TESTERROR: Failed to export JSON!!!!");
                EXFAIL_OUT(ret);
        }
        NDRX_LOG(log_debug, 
                "JSON exported. Return json_json_out=[%s] olen=[%ld]", 
                json_json_out, olen);

        if (0!=strcmp(json_json_in, json_json_out))
        {
            NDRX_LOG(log_error, 
                 "TESTERROR: Exported JSON not equal to incoming string ");
            EXFAIL_OUT(ret);
        }
    }

    /* testing with base64 flag*/
    NDRX_LOG(log_debug, "convert to b64");
    if (NULL==ndrx_base64_encode((unsigned char *)json_json_in, strlen(json_json_in), &len_b64, json_json_in_b64))
    {
            NDRX_LOG(log_error, "Failed to convert to b64!");
            EXFAIL_OUT(ret);
    }
    for (i=0; i<10000; i++)
    {
        rsplen=0L;
        if ( EXFAIL == tpimport(json_json_in_b64, 
                                (long)strlen(json_json_in_b64), 
                                (char **)&obuf, 
                                &rsplen, 
                                TPEX_STRING) )
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to import JSON CARRAY!!!!");
            EXFAIL_OUT(ret);
        }

        NDRX_LOG(log_debug, "JSON imported. Return obuf=[%s] rsplen=[%ld]", 
                                                    obuf, rsplen);

        if (0!=strcmp(data_test, obuf))
        {
            NDRX_LOG(log_error, 
                 "TESTERROR: imported JSON not equal to obuf string ");
            EXFAIL_OUT(ret);
        }

        memset(json_json_out_b64, 0, sizeof(json_json_out_b64));
        olen = sizeof(json_json_out_b64);

        if ( EXFAIL == tpexport(obuf, 
                                (long)strlen(obuf),
                                json_json_out_b64, 
                                &olen, 
                                TPEX_STRING) )
        {
                NDRX_LOG(log_error, "TESTERROR: Failed to export JSON!!!!");
                EXFAIL_OUT(ret);
        }
        NDRX_LOG(log_debug, 
                "JSON exported. Return json_json_out_b64=[%s] olen=[%ld]", 
                json_json_out_b64, olen);

        /* decode from b64 to check returned data */
        bufsz = strlen(json_json_out_b64);
        if (NULL==(istrtemp = NDRX_MALLOC(bufsz)))
        {
            NDRX_LOG(log_error, "Failed to allocate %ld bytes", strlen(json_json_out_b64));
            EXFAIL_OUT(ret);
        }

        if (NULL==ndrx_base64_decode(json_json_out_b64, strlen(json_json_out_b64), &bufsz, istrtemp))
        {
            NDRX_LOG(log_error, "Failed to decode CARRAY");
            EXFAIL_OUT(ret);
        }
        istrtemp[bufsz]=0;

        if (0!=strcmp(json_json_in, istrtemp))
        {
            NDRX_LOG(log_error, 
                 "TESTERROR: Exported JSON not equal to incoming JSON");
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
