/**
 * @brief Feature #139 mvitolin, 09/05/2017
 *   Call the server with GETNEXT which will unadvertise old service and advertise
 *   New name. This should exceed NDRX_SVCMAX and complete successfully.
 *
 * @file atmiclt34.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or Mavimax's license for commercial use.
 * -----------------------------------------------------------------------------
 * GPL license:
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
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

#include <ndebug.h>
#include <atmi.h>


#include <ubf.h>
#include <Exfields.h>
#include <test.fd.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

#ifndef SUCCEED
#define SUCCEED			0
#endif

#ifndef	FAIL
#define FAIL			-1
#endif

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/


/**
 * Initialise the application
 * @param argc	argument count
 * @param argv	argument values
 * @return SUCCEED/FAIL
 */
int init(int argc, char** argv)
{
	int ret = SUCCEED;

	TP_LOG(log_info, "Initialising...");

	if (SUCCEED!=tpinit(NULL))
	{
		TP_LOG(log_error, "Failed to Initialise: %s", 
			tpstrerror(tperrno));
		ret = FAIL;
		goto out;
	}

out:
	return ret;
}

/**
 * Terminate the application
 */
int uninit(int status)
{
	int ret = SUCCEED;
	
	TP_LOG(log_info, "Uninitialising...");
	
	ret = tpterm();
	
	return ret;
}

/**
 * Do processing (call some service)
 * @return SUCCEED/FAIL
 */
int process (void)
{
	int ret = SUCCEED;
	long i;
	UBFH *p_ub = NULL;
	long rsplen;
	BFLDLEN sz;
	char svcnm[MAXTIDENT+1];
	char svcnm_ret[MAXTIDENT+1];
	
	
	TP_LOG(log_info, "Processing...");


	/* Call sample server... */
	
	for (i=0; i<24000; i++)
	{
		if (NULL==(p_ub = (UBFH *)tpalloc("UBF", NULL, 1024)))
		{
			TP_LOG(log_error, "Failed to tpalloc: %s",  tpstrerror(tperrno));
			ret=FAIL;
			goto out;
		}

		if (SUCCEED!=Bchg(p_ub, T_LONG_FLD, 0, (char *)&i, 0L))
		{
			TP_LOG(log_error, "Failed to set T_LONG_FLD: %s",  
			      Bstrerror(Berror));
			ret=FAIL;
			goto out;
		}

		if (FAIL==tpcall("GETNEXT", (char *)p_ub, 0L, (char **)&p_ub, &rsplen, TPNOTIME))
		{
			TP_LOG(log_error, "TESTERROR: Failed to call GETNEXT: %s",
				tpstrerror(tperrno));
			ret=FAIL;
			goto out;
		}

		tplogprintubf(log_debug, "Got response from test server...", p_ub);
		
		/* Get the service number to advertise */
		sprintf(svcnm, "SVC%06ld", i);
		
		NDRX_LOG(log_info, "About to call service: [%s] - must be advertised",
				svcnm);

		if (FAIL==tpcall(svcnm, (char *)p_ub, 0L, (char **)&p_ub, &rsplen, TPNOTIME))
		{
			TP_LOG(log_error, "TESTERROR: Failed to call %s: %s",
				svcnm, tpstrerror(tperrno));
			ret=FAIL;
			goto out;
		}

		if (SUCCEED!=Bget(p_ub, T_STRING_2_FLD, 0, svcnm_ret, 0))
		{
			TP_LOG(log_error, "Failed to set T_STRING_2_FLD: %s",  
			      Bstrerror(Berror));
			ret=FAIL;
			goto out;
		}
		
		TP_LOG(log_info, "Response called [%s], returned [%s]", 
			 svcnm, svcnm_ret);
		
		if (0!=strcmp(svcnm, svcnm_ret))
		{
			TP_LOG(log_error, "TESTERROR!!!, expected service [%s], "
				"but got [%s]", 
				svcnm, svcnm_ret);
			ret=FAIL;
			goto out;
		}
	}
	
out:


	/* free up config data... */
	if (NULL!=p_ub)
	{
		tpfree((char *)p_ub);
	}
	
	return ret;
}

/**
 * Main entry of th program
 * @param argc	argument count
 * @param argv	argument values
 * @return SUCCEED/FAIL
 */
int main(int argc, char** argv)
{
	int ret = SUCCEED;

	if (SUCCEED!=init(argc, argv))
	{
		TP_LOG(log_error, "Failed to Initialize!");
		ret=FAIL;
		goto out;
	}
	
	
	if (SUCCEED!=process())
	{
		TP_LOG(log_error, "Process failed!");
		ret=FAIL;
		goto out;
	}
    
out:
	uninit(ret);

	return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
