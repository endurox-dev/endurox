/**
 * @brief Conversational tests
 *
 * @file atmisv35.c
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

#include <stdio.h>
#include <stdlib.h>
#include <ndebug.h>
#include <atmi.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <test.fd.h>
#include <string.h>

void CONVSV (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;
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
        ret=EXFAIL;
        goto out;
    }

    d+=1;

    for (i=0; i<100; i++)
    {
        sprintf(tmp, "SRV SND: %d", i);
        if (EXFAIL==Badd(p_ub, T_STRING_FLD, (char *)tmp, 0))
        {
            ret=EXFAIL;
            goto out;
        }

        if (p_svc->flags & TPSENDONLY)
        {
            NDRX_LOG(log_debug, "Doing some send!");
            /* Lets send some data back to client */
            if (EXFAIL==tpsend(p_svc->cd, (char *)p_ub, 0L, 0L, &revent))
            {
                NDRX_LOG(log_error, "Failed to send to client!");
            }
        }
	
	/* well we will now try to connect to other server... if we are conv2 */
	
	if (strcmp(p_svc->name, "CONVSV2"))
	{
		NDRX_LOG(log_info, "Try to connect to CONVSV!!!");
		if (EXFAIL==(cd=tpconnect("CONVSV", (char *)p_ub, 0L, TPRECVONLY)))
		{
			NDRX_LOG(log_error, "TESTERROR: connect failed!: %s",
						tpstrerror(tperrno));
			ret=EXFAIL;
			goto out;
		}
	}
	
	sleep(999);
    }

    /* Now we will become as listeners, OK? */
    if (EXFAIL==tpsend(p_svc->cd, (char *)p_ub, 0L, TPRECVONLY, &revent))
    {
        NDRX_LOG(log_error, "Failed to send to client!");
        goto out;
    }

    /* now wait for messages to come in! */
    while (EXSUCCEED==tprecv(p_svc->cd, (char **)&p_ub, 0L, 0L, &revent))
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
            ret=EXFAIL;
        }
    }

out:
    tpreturn(  ret==EXSUCCEED?TPSUCCESS:TPFAIL,
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
    int ret = EXSUCCEED;
    NDRX_LOG(log_debug, "tpsvrinit called");

    if (EXSUCCEED!=tpadvertise("CONVSV", CONVSV))
    {
        NDRX_LOG(log_error, "Failed to initialize CONVSV!");
        ret=EXFAIL;
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
/* vim: set ts=4 sw=4 et smartindent: */
