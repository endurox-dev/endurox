/* 
** Typed VIEW testing
**
** @file atmiclt40.c
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
#include <math.h>       /* fabs */

#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>
#include <test.fd.h>
#include <ndrstandard.h>
#include "test040.h"
#include "tperror.h"

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Test json2view and vice versa
 * @return 
 */
int test_view2json(void)
{
    int ret = EXSUCCEED;
    struct MYVIEW1 v1;
    char msg[ATMI_MSG_MAX_SIZE+1];
    char *abuf = NULL;
    char view[NDRX_VIEW_NAME_LEN+1];
    char *testbuf = "{\"MYVIEW2\":{\"tshort1\":1,\"tlong1\":2,\"tchar1\":\"A\",\""
    "tfloat1\":1,\"tdouble1\":21,\"tstring1\":\"ABC\",\""
    "tcarray1\":\"SEVMTE8AAAAAAA==\"}}";

    char *t3buf_null = "{\"MYVIEW3\":{\"tshort1\":0,\"tshort2\":0,\"tshort3\":0}}";
    char *t3buf_nnull = "{\"MYVIEW3\":{}}";
    
    struct MYVIEW2 *v2;
    struct MYVIEW3 v3;
    
    init_MYVIEW1(&v1);

    if (EXSUCCEED!=tpviewtojson((char *)&v1, "MYVIEW1", msg, sizeof(msg), 
                             BVACCESS_NOTNULL))
    {
        NDRX_LOG(log_error, "TESTERROR: tpviewtojson() failed: %s", 
                 tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }

    memset(&v1, 0, sizeof(v1));

    NDRX_LOG(log_debug, "Got json: [%s]", msg);

    if (NULL==(abuf=tpjsontoview(view, msg)))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to get view from JSON: %s", 
                 tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }

    /* test structure... */
    if (EXSUCCEED!=validate_MYVIEW1((struct MYVIEW1 *)abuf))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to validate view recovery!");
        EXFAIL_OUT(ret);
    }


    tpfree(abuf);
    abuf = NULL;

    /* test full convert... */
    init_MYVIEW1(&v1);

    msg[0] = EXEOS;
    if (EXSUCCEED!=tpviewtojson((char *)&v1, "MYVIEW1", msg, sizeof(msg), 0))
    {
        NDRX_LOG(log_error, "TESTERROR: tpviewtojson() failed: %s", 
                 tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }

    NDRX_LOG(log_debug, "Got json2: [%s]", msg);

    if (NULL==(abuf=tpjsontoview(view, msg)))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to get view from JSON: %s", 
                 tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }

    /* test structure... */
    if (EXSUCCEED!=validate_MYVIEW1((struct MYVIEW1 *)abuf))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to validate view recovery!");
        EXFAIL_OUT(ret);
    }
    
    tpfree(abuf);
    abuf=NULL;


    /* test manual convert */

    if (NULL==(abuf=tpjsontoview(view, testbuf)))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to get view from JSON: %s", 
                 tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }

    v2 = (struct MYVIEW2 *)abuf;

    if (1!=v2->tshort1)
    {
        NDRX_LOG(log_error, "TESTERROR: tshort1 got %hd expected 1", 
                 v2->tshort1);
        EXFAIL_OUT(ret);
    }

    if (2!=v2->tlong1)
    {
        NDRX_LOG(log_error, "TESTERROR: tlong1 got %hd expected 2", 
                 v2->tlong1);
        EXFAIL_OUT(ret);
    }

    if ('A'!=v2->tchar1)
    {
        NDRX_LOG(log_error, "TESTERROR: tchar1 got %c expected A", 
                 v2->tchar1);
        EXFAIL_OUT(ret);
    }

    if (fabs(v2->tfloat1 - 1.0f) > 0.1)
    {
        NDRX_LOG(log_error, "TESTERROR: tfloat1 got %f expected 1", 
                 v2->tfloat1);
        EXFAIL_OUT(ret);
    }

    if ((v2->tdouble1 - 21.0f) > 0.1)
    {
        NDRX_LOG(log_error, "TESTERROR: tdouble1 got %lf expected 21", 
                 v2->tdouble1);
        EXFAIL_OUT(ret);
    }

    if (0!=strcmp(v2->tstring1, "ABC"))
    {
        NDRX_LOG(log_error, "TESTERROR: tstring1 got [%s] expected ABC", 
                 v2->tdouble1);
        EXFAIL_OUT(ret);
    }

    if (0!=strncmp(v2->tcarray1, "HELLO", 5))
    {
        NDRX_LOG(log_error, "TESTERROR: tstring1 got [%10s] expected "
                "[HELLO]", 
                 v2->tdouble1); 
        EXFAIL_OUT(ret);
    }


    msg[0] = EXEOS;

    if (EXSUCCEED!=tpviewtojson((char *)v2, "MYVIEW2", msg, sizeof(msg), 0))
    {
        NDRX_LOG(log_error, "TESTERROR: tpviewtojson() failed: %s", 
                 tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }

    NDRX_LOG(log_debug, "v2 json: [%s]", msg);

    if (0!=strcmp(msg, testbuf))
    {
        NDRX_LOG(log_error, "TESTERROR: Built json: [%s] expected [%s]", 
                 msg, testbuf);
        EXFAIL_OUT(ret);
    }
    
    /* test error cases */
    tpfree(abuf);
    abuf = NULL;

    
    if (NULL!=(abuf=tpjsontoview(view, "HELLO WORLD")))
    {
        NDRX_LOG(log_error, "TESTERROR: Failure must occur - invalid json");
        EXFAIL_OUT(ret);
    }
    
    if (TPEINVAL!=tperrno)
    {
        NDRX_LOG(log_error, "TESTERROR: invalid error on json parse: "
                "expected TPEINVAL, got: %s",
                tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    if (NULL!=(abuf=tpjsontoview(view, "{\"NONEXIST\":{}}")))
    {
        NDRX_LOG(log_error, "TESTERROR: Failure must occur - invalid json");
        EXFAIL_OUT(ret);
    }
    
    if (TPEINVAL!=tperrno)
    {
        NDRX_LOG(log_error, "TESTERROR: invalid error on json parse: "
                "expected TPEINVAL, got: %s",
                tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    /* Test case for NULL field... */
    
    if (EXSUCCEED!=Bvsinit((char *)&v3, "MYVIEW3"))
    {
        NDRX_LOG(log_error, "TESTERROR: MYVIEW3 failed to init: %s", 
                Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=tpviewtojson((char *)&v3, "MYVIEW3", msg, sizeof(msg), 0))
    {
        NDRX_LOG(log_error, "TESTERROR: tpviewtojson() failed: %s", 
                 tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }

    NDRX_LOG(log_debug, "v3 json: [%s]", msg);

    if (0!=strcmp(msg, t3buf_null))
    {
        NDRX_LOG(log_error, "TESTERROR: Built json: [%s] expected [%s]", 
                 msg, t3buf_null);
        EXFAIL_OUT(ret);
    }
    
    
    if (EXSUCCEED!=tpviewtojson((char *)&v3, "MYVIEW3", msg, sizeof(msg), 
            BVACCESS_NOTNULL))
    {
        NDRX_LOG(log_error, "TESTERROR: tpviewtojson() failed: %s", 
                 tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }

    NDRX_LOG(log_debug, "v3 json: [%s]", msg);

    if (0!=strcmp(msg, t3buf_nnull))
    {
        NDRX_LOG(log_error, "TESTERROR: Built json: [%s] expected [%s]", 
                 msg, t3buf_nnull);
        EXFAIL_OUT(ret);
    }
 
    if (NULL==(abuf=tpjsontoview(view, msg)))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to parse empty json: %s", 
                Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    tpfree(abuf);
    abuf = NULL;
    
	
out:
    if (NULL!=abuf)
    {
        tpfree(abuf);
    }
    
    return ret;
}

/**
 * Call JSON service (TEST40_V2JSON) with VIEW buffer
 * @return 
 */
int test_x_view2json(void)
{
    int ret = EXSUCCEED;
    struct MYVIEW3 *v3 = NULL;
    long rsplen;
    
    v3 = (struct MYVIEW3 *)tpalloc("VIEW", "MYVIEW3", 0);
    
    if (NULL==v3)
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to allocate MYVIEW3: %s", 
                tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    v3->tshort1 = 1;
    v3->tshort2 = 2;
    v3->tshort3 = 3;
    
    if (EXSUCCEED!=tpcall("TEST40_V2JSON", (char *) v3, 0, 
            (char **)&v3, &rsplen, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to call TEST40_V2JSON");
        EXFAIL_OUT(ret);
    }
    
    if (4!=v3->tshort1)
    {
        NDRX_LOG(log_error, "TESTERROR: tshort1 must be 4 but must be %hd",
                v3->tshort1);
        EXFAIL_OUT(ret);
    }
    
    if (5!=v3->tshort2)
    {
        NDRX_LOG(log_error, "TESTERROR: tshort2 must be 5 but must be %hd",
                v3->tshort2);
        EXFAIL_OUT(ret);
    }
    
    if (6!=v3->tshort3)
    {
        NDRX_LOG(log_error, "TESTERROR: tshort3 must be 6 but must be %hd",
                v3->tshort3);
        EXFAIL_OUT(ret);
    }
    
out:
    
    if (NULL!=v3)
    {
        tpfree((char *)v3);
    }

    return ret;
}

/**
 * Call VIEW service with JSON (TEST40_JSON2V)
 * @return 
 */
int test_x_json2view(void)
{
    int ret = EXSUCCEED;
    long rsplen;
    char *buf;
    
    char *input = "{\"MYVIEW3\":{\"tshort1\":6,\"tshort2\":7,\"tshort3\":8}}";
    char *output = "{\"MYVIEW3\":{\"tshort1\":9,\"tshort2\":1,\"tshort3\":2}}";
    
    buf = tpalloc("JSON", NULL, 1024);
    
    if (NULL==buf)
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to allocate MYVIEW3: %s", 
                tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    strcpy(buf, input);
    
    if (EXSUCCEED!=tpcall("TEST40_JSON2V", buf, 0, &buf, &rsplen, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to call TEST40_JSON2V: %s",
                tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    if (0!=strcmp(buf, output))
    {
        NDRX_LOG(log_error, "TESTERROR: Received [%s] expected [%s]", 
                buf, output);
        EXFAIL_OUT(ret);
    }
    
out:
    
    if (NULL!=buf)
    {
        tpfree(buf);
    }

    return ret;
}

/*
 * Do the test call to the server
 */
int main(int argc, char** argv) {

    long rsplen;
    int i, j;
    int ret=EXSUCCEED;
    
    struct MYVIEW1 *v1;
    struct MYVIEW3 *v3;
    
    if (EXSUCCEED!=tpinit(NULL))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to init: %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    for (i=0; i<1000; i++)
    {
        if (EXSUCCEED!=test_x_json2view())
        {
            NDRX_LOG(log_error, "TESTERROR: test_x_json2view() fail!");
            EXFAIL_OUT(ret);
        }
    }
    
    
    for (i=0; i<1000; i++)
    {
        if (EXSUCCEED!=test_x_view2json())
        {
            NDRX_LOG(log_error, "TESTERROR: test_x_view2json() fail!");
            EXFAIL_OUT(ret);
        }
    }
    
    /* view ops with json */
    
    for (i=0; i<10000; i++)
    {
        if (EXSUCCEED!=test_view2json())
        {
            NDRX_LOG(log_error, "TESTERROR: JSON2VIEW failed!");
            EXFAIL_OUT(ret);
        }
    }

    /* View ops with services */
    for (j=0; j<1000; j++)
    {
        
        v1= (struct MYVIEW1 *) tpalloc("VIEW", "MYVIEW1", sizeof(struct MYVIEW1));
        if (NULL==v1)
        {
            NDRX_LOG(log_error, "TESTERROR: failed to alloc VIEW buffer!");
            EXFAIL_OUT(ret);
        }

        init_MYVIEW1(v1);
        
        NDRX_DUMP(log_debug, "VIEW1 request...", v1, sizeof(struct MYVIEW1));

        if (EXSUCCEED!=tpcall("TEST40_VIEW", (char *)v1, 0, (char **)&v1, &rsplen, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: failed to call TEST40_VIEW");
            EXFAIL_OUT(ret);
        }
        
        v3 = (struct MYVIEW3 *)v1;
        if (EXSUCCEED!=validate_MYVIEW3((v3)))
        {
            NDRX_LOG(log_error, "Failed to validate V3!");
            EXFAIL_OUT(ret);
        }
        
        tpfree((char *)v3);
    }
    
out:

    if (ret>=0)
        ret=EXSUCCEED;

    return ret;
}

