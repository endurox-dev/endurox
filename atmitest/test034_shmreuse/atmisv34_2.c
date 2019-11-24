/**
 * @brief Testing Bug #475 boot time advertise limits
 *
 * @file atmisv34_2.c
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
#include <ndrxdcmn.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

#ifndef EXSUCCEED
#define EXSUCCEED			0
#endif

#ifndef	EXFAIL
#define EXFAIL			-1
#endif

#define USED_SERVICES           2   /**< used by bootet server */
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
 * Initialize the application
 * Check static limits.
 * Shell test the environment when we have less SHM.
 * Currently it wont stop from booting as SHM limits are found only after
 * the init phase.
 * Currently it will silently ignore the system SHM limits and let binary to boot.
 * ndrxd and psc will see the queues. But thy will not be visible to shared
 * memory. Thus ndrxd psvc will not show them.
 * @param argc	argument count
 * @param argv	argument values
 * @return SUCCEED/FAIL
 */
int init(int argc, char** argv)
{
    int ret = EXSUCCEED;
    int i;
    char svcnm[MAXTIDENT+1];
    
    TP_LOG(log_info, "Initialising...");

    /* try to hit the limit? On 50 we shall get the system error
     * Once we try to boot with more than have in shm
     * we shall get some failure too..
     */
    
    /* check the test case... now... */
    NDRX_LOG(log_error, "Advertise max per service");
    for (i=0; i<MAX_SVC_PER_SVR-2 /* for adjust for admin/reply */ ; i++)
    {
        snprintf(svcnm, sizeof(svcnm), "ZZZ%06d", i);
        if (EXSUCCEED!=tpadvertise(svcnm, DYNFUNC))
        {
            NDRX_LOG(log_error, "TESTERROR! Failed to advertise [%s]: %s", 
                     svcnm, tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
    }
    
    NDRX_LOG(log_error, "Check limit, shall fail:");
    /* Check for next failure.. */
    snprintf(svcnm, sizeof(svcnm), "ZZZ%06d", i+1);
    if (EXSUCCEED==tpadvertise(svcnm, DYNFUNC))
    {
        NDRX_LOG(log_error, "TESTERROR! Must fail to advertise but got OK!!! [%s]: %s", 
                 svcnm, tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    /* check error */
    if (TPELIMIT!=tperrno)
    {
        NDRX_LOG(log_error, "TESTERROR! must be TPELIMIT, but got %d: %s", 
                 tperrno, tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_error, "Static unadvertise: ");
    /* remove all, check for leaks... */
    for (i=0; i<MAX_SVC_PER_SVR-2 /* for adjust for admin/reply */ ; i++)
    {
        snprintf(svcnm, sizeof(svcnm), "ZZZ%06d", i);
        if (EXSUCCEED!=tpunadvertise(svcnm))
        {
            NDRX_LOG(log_error, "TESTERROR! Failed to unadvertise [%s]: %s", 
                     svcnm, tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
    }
    
    /* ok do it again... */
    for (i=0; i<MAX_SVC_PER_SVR-2 /* for adjust for admin/reply */ ; i++)
    {
         snprintf(svcnm, sizeof(svcnm), "ZZZ%06d", i);
        if (EXSUCCEED!=tpadvertise(svcnm, DYNFUNC))
        {
            NDRX_LOG(log_error, "TESTERROR! Failed to advertise [%s]: %s", 
                     svcnm, tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
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

/* vim: set ts=4 sw=4 et smartindent: */
