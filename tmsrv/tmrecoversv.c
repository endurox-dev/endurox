/**
 * @brief Transaction recover server process. Used to clean up the transactions
 *  after the boot or periodically during the application runtime.
 *
 * @file tprecoversv.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * AGPL or Mavimax's license for commercial use.
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

/*---------------------------Includes-----------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ndrstandard.h>
#include <ndebug.h>
#include <atmi.h>
#include "tmrecover.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
exprivate int M_periodic = EXFALSE;    /**< by default single scan */
/*---------------------------Statics------------------------------------*/
/* Auto generated system advertise table */
expublic struct tmdsptchtbl_t ndrx_G_tmdsptchtbl[] = {
    { NULL, NULL, NULL, 0, 0 }
};
/*---------------------------Prototypes---------------------------------*/

/**
 * Transaction recover scan start
 * @return EXSUCCEED/EXFAIL
 */
exprivate int recover_scan(void)
{
    int ret = EXSUCCEED;
    
    NDRX_LOG(log_debug, "Recover scan started");
    
    /* recover transactions */
    ret = ndrx_tmrecover_do();
    
    /* set success if have 0 or rolled back.*/
    if (ret>=0)
    {
        if (!M_periodic)
        {
            /* remove our selves from periodic scanning as we are done
             */
            tpext_delperiodcb();
       }
       ret=EXSUCCEED;
    }
    
    return ret;
}

/**
 * Standard server init
 * @param argc
 * @param argv
 * @return 
 */
int tpsvrinit (int argc, char **argv)
{
    /* register periodic callback
     * - used to wait for bridges to establish 
     * - used for single & periodic recover scans.
     */
    int ret=EXSUCCEED;
    /* scan after 30 sec */
    int scan_time = 30;
    int c;
    
    /* Parse command line  */
    while ((c = getopt(argc, argv, "s:p")) != -1)
    {

	if (optarg)
        {
            NDRX_LOG(log_debug, "%c = [%s]", c, optarg);
        }
        else
        {
            NDRX_LOG(log_debug, "got %c", c);
        }

        switch(c)
        {
            case 's': 
                scan_time = atoi(optarg);
                NDRX_LOG(log_info, "Transaction scan time set to: %d", scan_time);
                break;
                /* status directory: */
            case 'p':
                M_periodic=EXTRUE;
                NDRX_LOG(log_info, "Periodic scan enabled");
                break;
            default:
                /*return FAIL;*/
                break;
        }
    }
    
    /* Register timer check (needed for time-out detection) */
    if (EXSUCCEED!=tpext_addperiodcb(scan_time, recover_scan))
    {
        NDRX_LOG(log_error, "tpext_addperiodcb failed: %s",
                        tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    
out:
    return ret;
}

/**
 * Standard server done
 */
void tpsvrdone(void)
{
    /* nothing todo */
}
/**
 * Main entry for tmsrv
 */
int main( int argc, char** argv )
{
    _tmbuilt_with_thread_option=0;
    struct tmsvrargs_t tmsvrargs =
    {
        &tmnull_switch,
        &ndrx_G_tmdsptchtbl[0],
        0,
        tpsvrinit,
        tpsvrdone,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    };
    
    return( _tmstartserver( argc, argv, &tmsvrargs ));
    
}

/* vim: set ts=4 sw=4 et smartindent: */
