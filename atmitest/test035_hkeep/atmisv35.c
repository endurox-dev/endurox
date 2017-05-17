/* 
** Conversational tests
** @file atmisv3.c
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

void CONVSV (TPSVCINFO *p_svc)
{
    int ret=SUCCEED;
    long revent;
    static double d = 55.66;
    int i;
    int cd;
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

    d+=1;

    for (i=0; i<100; i++)
    {
        sprintf(tmp, "SRV SND: %d", i);
        if (FAIL==Badd(p_ub, T_STRING_FLD, (char *)tmp, 0))
        {
            ret=FAIL;
            goto out;
        }

        if (p_svc->flags & TPSENDONLY)
        {
            NDRX_LOG(log_debug, "Doing some send!");
            /* Lets send some data back to client */
            if (FAIL==tpsend(p_svc->cd, (char *)p_ub, 0L, 0L, &revent))
            {
                NDRX_LOG(log_error, "Failed to send to client!");
            }
        }
	
	/* well we will now try to connect to other server... if we are conv2 */
	
	if (strcmp(p_svc->name, "CONVSV2"))
	{
		NDRX_LOG(log_info, "Try to connect to CONVSV!!!");
		if (FAIL==(cd=tpconnect("CONVSV", (char *)p_ub, 0L, TPRECVONLY)))
		{
			NDRX_LOG(log_error, "TESTERROR: connect failed!: %s",
						tpstrerror(tperrno));
			ret=FAIL;
			goto out;
		}
	}
	
	sleep(999);
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
        NDRX_LOG(log_debug, "Sent MSG OK!");
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
int NDRX_INTEGRA(tpsvrinit)(int argc, char **argv)
{
    int ret = SUCCEED;
    NDRX_LOG(log_debug, "tpsvrinit called");

    if (SUCCEED!=tpadvertise("CONVSV", CONVSV))
    {
        NDRX_LOG(log_error, "Failed to initialize CONVSV!");
        ret=FAIL;
    }
    
    
    return ret;
}

/**
 * Do de-initialization
 */
void NDRX_INTEGRA(tpsvrdone)(void)
{
    NDRX_LOG(log_debug, "tpsvrdone called");
}
