/* 
** Cache sanity daemon - this will remove expired records from db
**
** @file tpcached.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, Mavimax, Ltd. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or Mavimax's license for commercial use.
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
** A commercial use license is available from Mavimax, Ltd
** contact@mavimax.com
** -----------------------------------------------------------------------------
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include <atmi.h>
#include <atmi_int.h>
#include <ndrstandard.h>
#include <Exfields.h>
#include <ubf.h>
#include <ubf_int.h>
#include <ndebug.h>
#include <getopt.h>
#include <atmi_cache.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/    
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/


exprivate int M_sleep = 5;          /* perform actions at every X seconds */

/* for this mask, sigint, sigterm, check sigs periodically */
exprivate int M_shutdown = EXFALSE;  /* do we have shutdown requested?    */

exprivate sigset_t  M_mask;

/*---------------------------Prototypes---------------------------------*/

/**
 * Perform init (read -i command line argument - interval)
 * @return 
 */
expublic int init(int argc, char** argv)
{
    int ret = EXSUCCEED;
    signed char c;
    pthread_t thread;
    sigset_t set;
    int s;
    sigset_t    mask;
    
    /* Parse command line  */
    while ((c = getopt(argc, argv, "i:")) != -1)
    {
        NDRX_LOG(log_debug, "%c = [%s]", c, optarg);
        switch(c)
        {
            case 'i': 
                M_sleep = atoi(optarg);
                break;
            default:
                /*return FAIL;*/
                break;
        }
    }

    NDRX_LOG(log_debug, "Periodic sleep: %d secs", M_sleep);
    
    
    /* mask signal */
    sigemptyset(&M_mask);
    sigaddset(&M_mask, SIGINT);
    sigaddset(&M_mask, SIGTERM);

    if (EXSUCCEED!=sigprocmask(SIG_SETMASK, &M_mask, NULL))
    {
        NDRX_LOG(log_error, "Failed to set SIG_SETMASK: %s", strerror(errno));
        EXFAIL_OUT(ret);
    }

out:
    return ret;
}

/**
 * Main entry point for `tpcached' utility
 */
expublic int main(int argc, char** argv)
{

    int ret=EXSUCCEED;

    struct timespec timeout;
    siginfo_t info;
    int result = 0;

    /* local init */
    
    if (EXSUCCEED!=init(argc, argv))
    {
        NDRX_LOG(log_error, "Failed to init!");
        EXFAIL_OUT(ret);
    }
    
    /* ATMI init */
    
    if (EXSUCCEED!=tpinit(NULL))
    {
        NDRX_LOG(log_error, "Failed to init: %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    /* TODO:
     * loop over all databases
     * if database is limited (i.e. limit > 0), then do following:
     * - Read keys or (header with out data) into memory (linear mem)
     * and perform corresponding qsort
     * then remove records which we have at tail of the array.
     * - sleep configured time
     */
    
    timeout.tv_sec = M_sleep;
    timeout.tv_nsec = 0;


    while (!M_shutdown)
    {
        /* Get the DBs
         */

        result = sigtimedwait( &M_mask, &info, &timeout );

        if (result > 0)
        {
            if (SIGINT == result || SIGTERM == result)
            {
                NDRX_LOG(log_warn, "Signal received: %d - shutting down", result);
                M_shutdown = EXTRUE;
                break;
            }
            else
            {
                NDRX_LOG(log_warn, "Signal received: %d - ignore", result);
            }
        }
        else if (EXFAIL==result)
        {
            NDRX_LOG(log_error, "sigtimedwait failed: %s", strerror(errno));
            EXFAIL_OUT(ret);
        }

        /* TODO: interval process */
	
	
	
	
        
    }
    
out:
    /* un-initialize */
    tpterm();
    return ret;
}

