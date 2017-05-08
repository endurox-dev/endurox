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
#include <ndrstandard.h>
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
 * Dynamic function
 */
void DYNFUNC(TPSVCINFO *p_svc)
{
    int ret = SUCCEED;
    UBFH *p_ub = (UBFH *)p_svc->data;

    if (NULL==(p_ub = (UBFH *)tprealloc((char *)p_ub, 4096)))
    {
        FAIL_OUT(ret);
    }

    if (FAIL==Badd(p_ub, T_STRING_2_FLD, p_svc->name, 0L))
    {
        FAIL_OUT(ret);
    }
	
 out:

    tpreturn(  ret==SUCCEED?TPSUCCESS:TPFAIL,
            0L,
            (char *)p_ub,
            0L,
            0L);
}

/**
 * Service entry
 * @return SUCCEED/FAIL
 */
void GETNEXT (TPSVCINFO *p_svc)
{
    int ret = SUCCEED;
    long svcNr;
    static int first = TRUE;
    UBFH *p_ub = (UBFH *)p_svc->data;
    char svcnm[MAXTIDENT+1];
    char svcnm_prev[MAXTIDENT+1];

    tplogprintubf(log_info, "Got request", p_ub);


    if (FAIL==Bget(p_ub, T_LONG_FLD, 0, (char *)&svcNr, 0L))
    {
        FAIL_OUT(ret);
    }

    /* Get the service number to advertise */
    sprintf(svcnm, "SVC%06ld", svcNr);
    sprintf(svcnm_prev, "SVC%06ld", svcNr-1);
    
    NDRX_LOG(log_info, "Got service name: [%s], prev: [%s]", svcnm, svcnm_prev);
    
    if (first)
    {
        first = FALSE;
    }
    else
    {
        /* Unadvertise service... */
        if (SUCCEED!=tpunadvertise(svcnm_prev))
        {
            NDRX_LOG(log_error, "TESTERROR! Failed to unadvertise [%s]: %s", 
                     svcnm_prev, tpstrerror(tperrno));
            FAIL_OUT(ret);
        }
    }

    if (SUCCEED!=tpadvertise(svcnm, DYNFUNC))
    {
        NDRX_LOG(log_error, "TESTERROR! Failed to advertise [%s]: %s", 
                 svcnm, tpstrerror(tperrno));
        FAIL_OUT(ret);
    }

out:

    tpreturn(  ret==SUCCEED?TPSUCCESS:TPFAIL,
            0L,
            (char *)p_ub,
            0L,
            0L);
}

/**
 * Initialize the application
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

    /* Advertise our service */
    if (SUCCEED!=tpadvertise("GETNEXT", GETNEXT))
    {
        TP_LOG(log_error, "Failed to initialise GETNEXT!");
        ret=FAIL;
        goto out;
    }
	
out:

	
    return ret;
}

/**
 * Terminate the application
 */
void uninit(void)
{
    TP_LOG(log_info, "Uninitialising...");
}

/**
 * Server program main entry
 * @param argc	argument count
 * @param argv	argument values
 * @return SUCCEED/FAIL
 */
int main(int argc, char** argv)
{
    /* Launch the Enduro/x thread */
    return ndrx_main_integra(argc, argv, init, uninit, 0);
}

