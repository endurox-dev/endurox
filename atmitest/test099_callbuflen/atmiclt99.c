/**
 * @brief Test TPSVCINFO.len - client
 *
 * @file atmiclt99.c
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
#include "test99.h"
#include <test_view.h>
#include <exassert.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
typedef struct
{
    char *ptr;
    long len;
} test_buffer_t;
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Do the test call to the server
 */
int main(int argc, char** argv)
{

    char *p_ub = tpalloc("UBF", NULL, 56000);
    char *str = tpalloc("STRING", NULL, 56000);
    char *carr = tpalloc("CARRAY", NULL, 56000);
    char *json = tpalloc("JSON", NULL, 56000);
    char *view = tpalloc("VIEW", "UBTESTVIEW1", sizeof(struct UBTESTVIEW1));
    char *ret_buf = NULL;
    long rsplen, revent;
    int cd, i;
    int ret=EXSUCCEED;

    NDRX_ASSERT_VAL_OUT(NULL!=p_ub, "p_ub failed to alloc");
    NDRX_ASSERT_VAL_OUT(NULL!=str, "str failed to alloc");
    NDRX_ASSERT_VAL_OUT(NULL!=json, "json failed to alloc");
    NDRX_ASSERT_VAL_OUT(NULL!=view, "view failed to alloc");
    
    test_buffer_t bufs[] = {{p_ub, 0}, {str, 0}, {carr, 5}, {json, 0}, {view, 0}, {NULL, 0}};
    char *p;

    strcpy(str, "hello");
    strcpy(carr, "world");
    strcpy(json, "{}");

    for (i=0; i<N_DIM(bufs); i++)
    {
        if (EXFAIL == tpcall("TESTSV", bufs[i].ptr, bufs[i].len, (char **)&ret_buf, &rsplen,0))
        {
            NDRX_LOG(log_error, "TESTSV failed (%d): %s", i, tpstrerror(tperrno));
            ret=EXFAIL;
            goto out;
        }
    }

    /* test conv: */
    for (i=0; i<N_DIM(bufs); i++)
    {

        if (EXFAIL==(cd = tpconnect("TESTSV", bufs[i].ptr, bufs[i].len, TPRECVONLY)))
        {
            NDRX_LOG(log_error, "TESTSV tpconnect failed (%d): %s", i, tpstrerror(tperrno));
            ret=EXFAIL;
            goto out;
        }

        revent=0;
        if (EXFAIL==tprecv(cd, (char **)&p_ub, &rsplen, 0, &revent))
        {
            if (TPEEVENT!=tperrno || TPEV_SVCSUCC!=revent)
            {
                NDRX_LOG(log_error, "TESTSV tprecv failed (%d): %s %ld", i, tpstrerror(tperrno), revent);
                ret=EXFAIL;
                goto out;
            }
        }
    }

out:
    tpterm();
    fprintf(stderr, "Exit with %d\n", ret);

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
