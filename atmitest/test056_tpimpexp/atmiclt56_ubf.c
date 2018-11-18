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
#include <ubf.h>
#include <ndebug.h>
#include <test.fd.h>
#include <ndrstandard.h>
#include <nstopwatch.h>
#include <fcntl.h>
#include <unistd.h>
#include <nstdutil.h>
#include <ubfutil.h>
#include "test56.h"
#include "t56.h"
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
expublic int test_impexp_ubf()
{
    int ret = EXSUCCEED;
    int i;
    long rsplen,olen;
    char *obuf;
    char *json_ubf_in = 
        "{"
            "\"buftype\":\"UBF\","
            "\"version\":1,"
            "\"data\":"
            "{"
                "\"T_SHORT_FLD\":1765,"
                "\"T_LONG_FLD\":[3333111,2],"
                "\"T_CHAR_FLD\":\"A\","
                "\"T_FLOAT_FLD\":1,"
                "\"T_DOUBLE_FLD\":[1111.220000,333,444],"
                "\"T_STRING_FLD\":\"HELLO WORLD\","
                "\"T_CARRAY_FLD\":\"AAECA0hFTExPIEJJTkFSWQQFAA==\""
            "}"
        "}";
    char json_ubf_out[1024];

    NDRX_LOG(log_info, "JSON UBF IN: [%s]", json_ubf_in);

    if (NULL == (obuf = tpalloc("UBF", NULL, NDRX_MSGSIZEMAX)))
    {
        NDRX_LOG(log_error, "Failed to allocate UBFH %ld bytes: %s",
                 NDRX_MSGSIZEMAX, tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }

    for (i=0; i<10000; i++)
    {
        rsplen=0L;
        if ( EXFAIL == tpimport(json_ubf_in, 
                                (long)strlen(json_ubf_in), 
                                (char **)&obuf, 
                                &rsplen, 
                                0L) )
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to import JSON UBF!!!!");
            EXFAIL_OUT(ret);
        }
        ndrx_debug_dump_UBF(log_debug, "JSON UBF imported. Return obuf", (UBFH *)obuf);

        /* TODO Check imported field values */

        memset(json_ubf_out, 0, sizeof(json_ubf_out));
        olen = sizeof(json_ubf_out);
        if ( EXFAIL == tpexport(obuf, 
                                (long)strlen(obuf), 
                                json_ubf_out, 
                                &olen, 
                                0L) )
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to export JSON UBF!!!!");
            EXFAIL_OUT(ret);
        }
        NDRX_LOG(log_error, 
                 "JSON UBF exported. Return json_ubf_out=[%s] olen=[%ld]", 
                 json_ubf_out, olen);

        if (0!=strcmp(json_ubf_in, json_ubf_out))
        {
            NDRX_LOG(log_error, 
                 "TESTERROR: Exported UBF not equal to incoming string ");
            EXFAIL_OUT(ret);
        }
    }

out:

    return ret;
}
