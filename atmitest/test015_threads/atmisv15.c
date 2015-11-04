/* 
**
** @file atmisv15.c
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
#include <string.h>

/*
 * Service does not return anything...
 */
void NULLSV (TPSVCINFO *p_svc)
{
   tpreturn (TPSUCCESS, 0L, NULL, 0L, 0L);
}

/**
 * Service should receive NULL buffer.
 * Replies back some data
 * @param p_svc
 */
void RETSOMEDATA(TPSVCINFO *p_svc)
{
    int first=1;
    static UBFH *p_ub;

    if (NULL!=p_svc->data)
    {
        NDRX_LOG(log_error, "TESTERROR: RETSOMEDATA svc should got NULL call!");
        tpreturn (TPFAIL, 0L, NULL, 0L, 0L);
    }

    if (first)
    {
        if (NULL==(p_ub=(UBFH *)tpalloc("UBF", NULL, 1024)))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to allocate 1024 bytes: %s",
                                            tpstrerror(tperrno));
            tpreturn (TPFAIL, 0L, NULL, 0L, 0L);
        }

        if (SUCCEED!=Bchg(p_ub, T_STRING_2_FLD, 0, "RESPONSE DATA 1", 0))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to set s: T_STRING_2_FLD%s",
                                            Bstrerror(Berror));
            tpreturn (TPFAIL, 0L, NULL, 0L, 0L);
        }
    }

    /* Return OK */
    tpreturn (TPSUCCESS, 0L, (char *)p_ub, 0L, 0L);
}


/**
 * Service should receive NULL buffer.
 * Replies back some data
 * @param p_svc
 */
void ECHO(TPSVCINFO *p_svc)
{
    int first=1;

    UBFH *p_ub = (UBFH *)p_svc->data;
    
    /* Return OK */
    tpreturn (TPSUCCESS, 0L, (char *)p_ub, 0L, 0L);
}

/**
 * This service basically raises timeout!
 * @param p_svc
 */
void TIMEOUTSV (TPSVCINFO *p_svc)
{
    int ret=SUCCEED;
    UBFH *p_ub = (UBFH *)p_svc->data;

    /* Make some deadly sleep! */
    sleep(4);

    tpreturn(  ret==SUCCEED?TPSUCCESS:TPFAIL,
                0L,
                (char *)p_ub,
                0L,
                0L);
}

void TESTSVFN (TPSVCINFO *p_svc)
{
    int ret=SUCCEED;

    static double d = 55.66;

    UBFH *p_ub = (UBFH *)p_svc->data;

    NDRX_LOG(log_debug, "TESTSVFN got call");

    /* Just print the buffer 
    Bprint(p_ub);*/
    if (NULL==(p_ub = (UBFH *)tprealloc((char *)p_ub, 4096))) /* allocate some stuff for more data to put in  */
    {
        ret=FAIL;
        goto out;
    }

    /* d+=1; */

    if (FAIL==Badd(p_ub, T_DOUBLE_FLD, (char *)&d, 0))
    {
        ret=FAIL;
        goto out;
    }

out:
    tpreturn(  ret==SUCCEED?TPSUCCESS:TPFAIL,
                0L,
                (char *)p_ub,
                0L,
                0L);
}

int get_infos(UBFH **pp_ub, char *command)
{
    int ret=SUCCEED;
    char data_out[1035];
    FILE *fp;
    int len;

    if (NULL==(*pp_ub = (UBFH *)tprealloc((char *)*pp_ub, 4096))) /* allocate some stuff for more data to put in  */
    {
        ret=FAIL;
        goto out;
    }

    /* Open the command for reading. */
    fp = popen(command, "r");
    if (fp == NULL)
    {
	NDRX_LOG(log_error, "Failed to run command!");
	ret=FAIL;
        goto out;
    }

    /* Read the output a line at a time - output it. */
    while (fgets(data_out, sizeof(data_out)-1, fp) != NULL)
    {
    	data_out[strlen(data_out)-1] = 0; /*strip trailing stuff */
	len = strlen(data_out);

	if (len > 0 && 9==data_out[len-1])
	{
		data_out[len-1] = 0;
	}

        if (FAIL==Badd(*pp_ub, T_STRING_FLD, data_out, 0))
        {  
            ret=FAIL;
            goto out;
        }
    }

    /* close */
     pclose(fp);


out:
	return ret;
}

/**
 * UNIXINFO service
 */
void UNIXINFO (TPSVCINFO *p_svc)
{
    int ret=SUCCEED;

    UBFH *p_ub = (UBFH *)p_svc->data;

    NDRX_LOG(log_debug, "UNIXINFO got call");
    if (SUCCEED!=get_infos(&p_ub, "uname -a") ||
    	SUCCEED!=get_infos(&p_ub, "uptime"))
    {
    	ret=FAIL;
    }

out:
	/* Forward to info2 server!!! */
	if (SUCCEED==ret)
	{
	    tpforward(  "UNIX2",
	                (char *)p_ub,
	                0L,
	                0L);
	}
	else
	{
    tpreturn(  ret==SUCCEED?TPSUCCESS:TPFAIL,
                0L,
                (char *)p_ub,
                0L,
                0L);
		}
}

/**
 * UNIXINFO2 service
 */
void UNIX2 (TPSVCINFO *p_svc)
{
    int ret=SUCCEED;

    UBFH *p_ub = (UBFH *)p_svc->data;

    NDRX_LOG(log_debug, "UNIX2 got call");
    if (SUCCEED!=get_infos(&p_ub, "uname -a") ||
	SUCCEED!=get_infos(&p_ub, "uptime"))
    {
    	ret=FAIL;
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
    NDRX_LOG(log_debug, "tpsvrinit called");

    if (SUCCEED!=tpadvertise("TIMEOUTSV", TIMEOUTSV))
    {
        NDRX_LOG(log_error, "Failed to initialize TIMEOUTSV!");
    }
    else if (SUCCEED!=tpadvertise("TESTSV", TESTSVFN))
    {
        NDRX_LOG(log_error, "Failed to initialize TESTSV (first)!");
    }
    else if (SUCCEED!=tpadvertise("TESTSV", TESTSVFN))
    {
        NDRX_LOG(log_error, "Failed to initialize TESTSV (second)!");
    }
    else if (SUCCEED!=tpadvertise("NULLSV", NULLSV))
    {
        NDRX_LOG(log_error, "Failed to initialize NULLSV!");
    }
    else if (SUCCEED!=tpadvertise("ECHO", ECHO))
    {
        NDRX_LOG(log_error, "Failed to initialize ECHO!");
    }
    else if (SUCCEED!=tpadvertise("RETSOMEDATA", RETSOMEDATA))
    {
        NDRX_LOG(log_error, "Failed to initialize RETSOMEDATA!");
    }
    else if (SUCCEED!=tpadvertise("UNIXINFO", UNIXINFO))
    {
        NDRX_LOG(log_error, "Failed to initialize UNIXINFO!");
    }
    else if (SUCCEED!=tpadvertise("UNIX2", UNIX2))
    {
        NDRX_LOG(log_error, "Failed to initialize UNIX2!");
    }
}

/**
 * Do de-initialization
 */
void tpsvrdone(void)
{
    NDRX_LOG(log_debug, "tpsvrdone called");
}
