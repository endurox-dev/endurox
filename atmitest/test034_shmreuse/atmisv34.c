/**
 * @brief Feature #139 mvitolin, 09/05/2017
 *   Test the shared memory service registry reuse - when unique service names
 *   in app session overflows the NDRX_SVCMAX
 *
 * @file atmisv34.c
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
#include <ndrstandard.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

#ifndef EXSUCCEED
#define EXSUCCEED			0
#endif

#ifndef	EXFAIL
#define EXFAIL			-1
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
    int ret = EXSUCCEED;
    UBFH *p_ub = (UBFH *)p_svc->data;

    if (NULL==(p_ub = (UBFH *)tprealloc((char *)p_ub, 4096)))
    {
        EXFAIL_OUT(ret);
    }

    if (EXFAIL==Badd(p_ub, T_STRING_2_FLD, p_svc->name, 0L))
    {
        EXFAIL_OUT(ret);
    }
	
 out:

    tpreturn(  ret==EXSUCCEED?TPSUCCESS:TPFAIL,
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
    int ret = EXSUCCEED;
    long svcNr;
    static int first = EXTRUE;
    UBFH *p_ub = (UBFH *)p_svc->data;
    char svcnm[MAXTIDENT+1];
    char svcnm_prev[MAXTIDENT+1];

    tplogprintubf(log_info, "Got request", p_ub);


    if (EXFAIL==Bget(p_ub, T_LONG_FLD, 0, (char *)&svcNr, 0L))
    {
        EXFAIL_OUT(ret);
    }

    /* Get the service number to advertise */
    sprintf(svcnm, "SVC%06ld", svcNr);
    sprintf(svcnm_prev, "SVC%06ld", svcNr-1);
    
    NDRX_LOG(log_info, "Got service name: [%s], prev: [%s]", svcnm, svcnm_prev);
    
    if (first)
    {
        first = EXFALSE;
    }
    else
    {
        /* Unadvertise service... */
        if (EXSUCCEED!=tpunadvertise(svcnm_prev))
        {
            NDRX_LOG(log_error, "TESTERROR! Failed to unadvertise [%s]: %s", 
                     svcnm_prev, tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
    }

    if (EXSUCCEED!=tpadvertise(svcnm, DYNFUNC))
    {
        NDRX_LOG(log_error, "TESTERROR! Failed to advertise [%s]: %s", 
                 svcnm, tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }

out:

    tpreturn(  ret==EXSUCCEED?TPSUCCESS:TPFAIL,
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
    int ret = EXSUCCEED;

    TP_LOG(log_info, "Initialising...");

    if (EXSUCCEED!=tpinit(NULL))
    {
        TP_LOG(log_error, "Failed to Initialise: %s", 
                tpstrerror(tperrno));
        ret = EXFAIL;
        goto out;
    }

    /* Advertise our service */
    if (EXSUCCEED!=tpadvertise("GETNEXT", GETNEXT))
    {
        TP_LOG(log_error, "Failed to initialise GETNEXT!");
        ret=EXFAIL;
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

