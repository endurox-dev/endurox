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
expublic int test_impexp_view()
{
    int ret = EXSUCCEED;
    char type[16+1]={EXEOS};
    char subtype[XATMI_SUBTYPE_LEN]={EXEOS};
    long rsplen, olen;
    int i;
    char *obuf=NULL;
    char *json_view_in = 
        "{"
            "\"buftype\":\"VIEW\","
            "\"version\":1,"
            "\"subtype\":\"MYVIEW56\","
            "\"data\":"
            "{"
                "\"MYVIEW56\":"
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
    struct MYVIEW56 *v;

    NDRX_LOG(log_info, "JSON VIEW IN: [%s]", json_view_in);
    v = (struct MYVIEW56 *)tpalloc("VIEW", "MYVIEW56", sizeof(struct MYVIEW56));
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
        NDRX_LOG(log_error, 
                "VIEW exported. Return json_view_out=[%s] olen=[%ld]", 
                json_view_out, olen);

        if (0!=strcmp(json_view_in, json_view_out))
        {
            NDRX_LOG(log_error, 
                 "TESTERROR: Exported VIEW not equal to incoming VIEW ");
            EXFAIL_OUT(ret);
        }
    }
out:

    return ret;
}
