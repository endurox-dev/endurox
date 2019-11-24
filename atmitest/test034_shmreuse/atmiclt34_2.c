/**
 * @brief Check shared memory directly, using system headers
 *
 * @file atmiclt34_2.c
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
#include <atmi_shm.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
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
    int ret = EXSUCCEED;

    TP_LOG(log_info, "Initialising...");

    if (EXSUCCEED!=tpinit(NULL))
    {
        TP_LOG(log_error, "Failed to Initialise: %s", 
                tpstrerror(tperrno));
        ret = EXFAIL;
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
    int ret = EXSUCCEED;

    TP_LOG(log_info, "Uninitialising...");

    ret = tpterm();

    return ret;
}

/**
 * Load some keys with one free, the first one
 * @param mem memory to init
 * @param mark_free position to leave un-inited -> free
 * @param mark_was_used position to mark 
 */
exprivate int init_one_free(int max_svcs, int mark_free)
{
    int ret = EXSUCCEED;
    int i;
    char svcnm[MAXTIDENT+1];
    
    /* reset service memory...  */
    memset(G_svcinfo.mem, 0, G_svcinfo.size);
    
    for (i=0; i<max_svcs; i++)
    {
        snprintf(svcnm, sizeof(svcnm), "SVC%d", i);
        /* lets install some service stuff there... */
        if (EXSUCCEED!=ndrx_shm_install_svc(svcnm, 0, 1))
        {
            NDRX_LOG(log_error, "Failed to install service [%s]! / prep", 
                    svcnm);
            EXFAIL_OUT(ret);
        }
    }
    
    /* reset the cell.. */
    NDRX_LOG(log_error, "Free pos: %d", mark_free);
    memset(G_svcinfo.mem+mark_free*(SHM_SVCINFO_SIZEOF), 0, SHM_SVCINFO_SIZEOF);
    
out:
    return ret;
}



/**
 * This assumes that no other processes are looking into shared memory
 * This means that ndrxd must be in idle mode, with sanity checks disabled.
 * i.e. we run $ xadmin appconfig sanity 99999
 * before this test.
 * @return SUCCEED/FAIL
 */
int process (void)
{
    int ret = EXSUCCEED;
    int i;
    char svcnm[MAXTIDENT+1];
    int max_svcs;
    int run_max=10000;
    int last;
    /* let attach to shm... */
    
    /* service:
     * 
     * expublic int _ndrx_shm_get_svc(char *svc, int *pos, int doing_install, int *p_install_cmd);
     * 
     */
    

    
    /* fill with services */
    max_svcs = G_max_svcs;
    
    
    for (i=max_svcs; i<run_max; i++)
    {
        if (EXSUCCEED!=init_one_free(max_svcs, 1))
        {
            EXFAIL_OUT(ret);
        }
        
        /* try to install now single position, we shall get one... */
        snprintf(svcnm, sizeof(svcnm), "XXX%d", i);
        
        /* lets install some service stuff there... */
        if (EXSUCCEED!=ndrx_shm_install_svc(svcnm, 0, 1))
        {
            NDRX_LOG(log_error, "Failed to install service [%s]! / test-one", 
                    svcnm);
            EXFAIL_OUT(ret);
        }
        
        /* try to install again when full, should fail. */
        snprintf(svcnm, sizeof(svcnm), "XXX%d", i+1);
        if (EXSUCCEED==ndrx_shm_install_svc(svcnm, 0, 1))
        {
            NDRX_LOG(log_error, "Got succeed! but SHM is full on [%s]!",  svcnm);
            EXFAIL_OUT(ret);
        }
        
        /* remove from shm...and install again..  */
        snprintf(svcnm, sizeof(svcnm), "XXX%d", i);
        ndrxd_shm_uninstall_svc(svcnm, &last, 1);
        
        /* try to install again.. should succeed. */
        if (EXSUCCEED!=ndrx_shm_install_svc(svcnm, 0, 1))
        {
            NDRX_LOG(log_error, "Failed to install service [%s]! / test-one /2 ", 
                    svcnm);
            EXFAIL_OUT(ret);
        }
        
        
    }
    
out:

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
    int ret = EXSUCCEED;

    
    if (EXSUCCEED!=init(argc, argv))
    {
        TP_LOG(log_error, "Failed to Initialize!");
        EXFAIL_OUT(ret);
    }

    if (EXSUCCEED!=process())
    {
        TP_LOG(log_error, "Process failed!");
        EXFAIL_OUT(ret);
    }
    
out:
    uninit(ret);

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
