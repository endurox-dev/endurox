/* 
** Basic test client
**
** @file convclt21.c
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

    UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, 1024);
    long rsplen;
    int i, j;
    int ret=SUCCEED;
    int cd;
    long revent;
    char tmp[126];

    if (SUCCEED!=tpopen())
    {
        NDRX_LOG(log_error, "TESTERROR: tpopen() fail: %d:[%s]", 
                                            tperrno, tpstrerror(tperrno));
        ret=FAIL;
        goto out;
    }

for (j=0; j<1000; j++)
{
    if (SUCCEED!=tpbegin(5, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: tpbegin() fail: %d:[%s]", 
                                            tperrno, tpstrerror(tperrno));
        ret=FAIL;
        goto out;
    }

    if (FAIL==(cd=tpconnect("CONVSV", (char *)p_ub, 0L, TPRECVONLY)))
    {
            NDRX_LOG(log_error, "TESTSV connect failed!: %s",
                                    tpstrerror(tperrno));
            ret=FAIL;
            goto out;
    }

    /* Recieve the stuff back */
    NDRX_LOG(log_debug, "About to tprecv!");

    while (SUCCEED==tprecv(cd, (char **)&p_ub, 0L, 0L, &revent))
    {
        NDRX_LOG(log_debug, "MSG RECEIVED OK!");
    }
    

    /* If we have event, we would like to become recievers if so */
    if (TPEEVENT==tperrno)
    {
        if (TPEV_SENDONLY==revent)
        {
            int i=0;
            /* Start the sending stuff now! */
            for (i=0; i<1 && SUCCEED==ret; i++)
            {
        	sprintf(tmp, "CLT: %d\n", i);
	        Bchg(p_ub, T_STRING_FLD, 0, tmp, 0L);
                ret=tpsend(cd, (char *)p_ub, 0L, 0L, &revent);
            }
        }
    }

    /* Now give the control to the server, so that he could finish up */
    if (FAIL==tpsend(cd, NULL, 0L, TPRECVONLY, &revent))
    {
        NDRX_LOG(log_debug, "Failed to give server control!!");
        ret=FAIL;
        goto out;
    }

    NDRX_LOG(log_debug, "Get response from tprecv!");
    Bfprint(p_ub, stderr);

    /* Wait for return from server */
    ret=tprecv(cd, (char **)&p_ub, 0L, 0L, &revent);
    NDRX_LOG(log_error, "tprecv failed with revent=%ld", revent);

    if (FAIL==ret && TPEEVENT==tperrno && TPEV_SVCSUCC==revent)
    {
        NDRX_LOG(log_error, "Service finished with TPEV_SVCSUCC!");
	if (SUCCEED!=(ret=tpcommit(0)))
        {
            NDRX_LOG(log_error, "TESTERROR: tpcommit()==%d fail: %d:[%s]", 
                                                ret, tperrno, tpstrerror(tperrno));
            ret=FAIL;
            goto out;
        }
        ret=SUCCEED;
    }
    else
    {
	if (SUCCEED!=(ret=tpabort(0)))
        {
            NDRX_LOG(log_error, "TESTERROR: tpabort()==%d fail: %d:[%s]", 
                                                ret, tperrno, tpstrerror(tperrno));
            ret=FAIL;
            goto out;
        }
    }
}
    
out:

    if (SUCCEED!=tpclose())
    {
        NDRX_LOG(log_error, "TESTERROR: tpclose() fail: %d:[%s]", 
                                            tperrno, tpstrerror(tperrno));
        ret=FAIL;
        goto out;
    }

    if (SUCCEED!=tpterm())
    {
        NDRX_LOG(log_error, "tpterm failed with: %s", tpstrerror(tperrno));
        ret=FAIL;
        goto out;
    }
    
    return ret;
}

