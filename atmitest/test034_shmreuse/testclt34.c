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
	
	for (i=0; i<10000000; i++)
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
		sprintf(svcnm, "SVC%06ld", svcNr);
		
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

