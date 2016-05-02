/* 
** TMQ test client.
**
** @file atmiclt3.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, ATR Baltic, SIA. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or ATR Baltic's license for commercial use.
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
** A commercial use license is available from ATR Baltic, SIA
** contact@atrbaltic.com
** -----------------------------------------------------------------------------
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>
#include <test.fd.h>
#include <ndrstandard.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/*
 * Do the test call to the server
 */
int main(int argc, char** argv) {

    int ret = SUCCEED;
    char testbuf_ref[10] = {0,1,2,3,4,5,6,7,8,9};
    long len;
    TPQCTL qc;
    int i;

    for (i=0; i<100000; i++)
    {
    char *buf = tpalloc("CARRAY", "", 10);
    
    /* alloc output buffer */
    if (NULL==buf)
    {
        NDRX_LOG(log_error, "TESTERROR: tpalloc() failed %s", 
                tpstrerror(tperrno));
        FAIL_OUT(ret);
    }
    
    /* enqueue the data buffer */
    memset(&qc, 0, sizeof(qc));
    if (SUCCEED!=tpenqueue("MYSPACE", "TEST1", &qc, testbuf_ref, 
            sizeof(testbuf_ref), TPNOTRAN))
    {
        NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                tpstrerror(tperrno), qc.diagnostic, qc.diagmsg);
        FAIL_OUT(ret);
    }
    
    /* dequeue the data buffer + allocate the output buf. */
    
    memset(&qc, 0, sizeof(qc));
    
    len = 10;
    if (SUCCEED!=tpdequeue("MYSPACE", "TEST1", &qc, &buf, 
            &len, TPNOTRAN))
    {
        NDRX_LOG(log_error, "TESTERROR: tpenqueue() failed %s diag: %d:%s", 
                tpstrerror(tperrno), qc.diagnostic, qc.diagmsg);
        FAIL_OUT(ret);
    }
    
    /* compare - should be equal */
    if (0!=memcmp(testbuf_ref, buf, len))
    {
        NDRX_LOG(log_error, "TESTERROR: Buffers not equal!");
        NDRX_DUMP(log_error, "original buffer", testbuf_ref, sizeof(testbuf_ref));
        NDRX_DUMP(log_error, "got form q", buf, len);
        FAIL_OUT(ret);
    }
    
    tpfree(buf);
    }
    
    if (SUCCEED!=tpterm())
    {
        NDRX_LOG(log_error, "tpterm failed with: %s", tpstrerror(tperrno));
        ret=FAIL;
        goto out;
    }
    
out:
    return ret;
}

