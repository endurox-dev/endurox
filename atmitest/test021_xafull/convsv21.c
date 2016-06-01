/* 
**
** @file convsv21.c
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

#include <stdio.h>
#include <stdlib.h>
#include <ndebug.h>
#include <atmi.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <test.fd.h>


extern int __write_to_tx_file(char *buf);

void CONVSV (TPSVCINFO *p_svc)
{
    int ret=SUCCEED;
    long revent;
    int i;
    UBFH *p_ub = (UBFH *)p_svc->data;
    char tmp[128];
    NDRX_LOG(log_debug, "CONVSV got call");

    /* Just print the buffer */
    Bprint(p_ub);

    if (NULL==(p_ub = (UBFH *)tprealloc((char *)p_ub, 6000))) /* allocate some stuff for more data to put in  */
    {
        ret=FAIL;
        goto out;
    }

    for (i=0; i<1; i++)
    {
        sprintf(tmp, "SRV SND: %d", i);
        Bchg(p_ub, T_STRING_FLD, 0, (char *)tmp, 0);

        if (p_svc->flags & TPSENDONLY)
        {
            NDRX_LOG(log_debug, "Doing some send!");
            /* Lets send some data back to client */
            if (FAIL==tpsend(p_svc->cd, (char *)p_ub, 0L, 0L, &revent))
            {
                NDRX_LOG(log_error, "Failed to send to client!");
            }
        }
    }

    /* Now we will become as listeners, OK? */
    if (FAIL==tpsend(p_svc->cd, (char *)p_ub, 0L, TPRECVONLY, &revent))
    {
        NDRX_LOG(log_error, "Failed to send to client!");
        goto out;
    }

    /* now wait for messages to come in! */
    while (SUCCEED==tprecv(p_svc->cd, (char **)&p_ub, 0L, 0L, &revent))
    {
	Bget(p_ub, T_STRING_FLD, 0, tmp, 0L);
	/* write off transaction */
	__write_to_tx_file(tmp);
	
        NDRX_LOG(log_debug, "Recevied MSG OK!");
    }

    /* Dump the buffer */
    Bfprint(p_ub, stderr);

    if (TPEEVENT==tperrno)
    {
        if (TPEV_SENDONLY==revent)
        {
            NDRX_LOG(log_debug, "We are senders - FINISH UP!");
        }
        else
        {
            NDRX_LOG(log_debug, "Did not get TPEV_SENDONLY!!!");
            ret=FAIL;
        }
    }

out:
    tpreturn(  ret==SUCCEED?TPSUCCESS:TPFAIL,
                0L,
                (char *)p_ub,
                0L,
                0L);
}

/*
 * Do initialization
 */
int tpsvrinit(int argc, char **argv)
{
    int ret = SUCCEED;
    NDRX_LOG(log_debug, "tpsvrinit called");

    if (SUCCEED!=tpopen())
    {
        NDRX_LOG(log_error, "TESTERROR: tpopen() fail: %d:[%s]", 
                                            tperrno, tpstrerror(tperrno));
        ret=FAIL;
        goto out;
    }

    if (SUCCEED!=tpadvertise("CONVSV", CONVSV))
    {
        NDRX_LOG(log_error, "Failed to initialize CONVSV!");
    }
out:
    return ret;
}

/**
 * Do de-initialization
 */
void tpsvrdone(void)
{
    NDRX_LOG(log_debug, "tpsvrdone called");

    if (SUCCEED!=tpclose())
    {
        NDRX_LOG(log_error, "TESTERROR: tpclose() fail: %d:[%s]", 
                                            tperrno, tpstrerror(tperrno));
    }

}

