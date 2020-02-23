/**
 * @brief Both sites max send, avoid deadlock of full sockets - client
 *
 * @file atmiclt72.c
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

#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>
#include <test.fd.h>
#include <ndrstandard.h>
#include <nstopwatch.h>
#include <fcntl.h>
#include <unistd.h>
#include <nstdutil.h>
#include "test72.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Do the test call to the server
 */
int main(int argc, char** argv)
{

    UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, 56000);
    long rsplen;
    int i;
    int ret=EXSUCCEED;
    long sent=0;
    long sentread;
    ndrx_stopwatch_t w;
    
    /* send msg for 5 min.... */
    
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <Service_name>\n", argv[0]);
        EXFAIL_OUT(ret);
    }
    
    ndrx_stopwatch_reset(&w);
    
    /* have shorter test on RPI/32bit sys */
#if EX_SIZEOF_LONG==4
    while (ndrx_stopwatch_get_delta_sec(&w) < 5)
#else
    while (ndrx_stopwatch_get_delta_sec(&w) < 20)
#endif
    {
        if (EXFAIL==CBchg(p_ub, T_STRING_FLD, 0, VALUE_EXPECTED, 0, BFLD_STRING))
        {
            NDRX_LOG(log_debug, "Failed to set T_STRING_FLD[0]: %s", Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }    

        /* do the sync call... */
        if (EXFAIL == tpacall(argv[1], (char *)p_ub, 0L, TPNOREPLY))
        {
            NDRX_LOG(log_error, "%s failed: %s", argv[1], tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
        
        sent++;
    }
    
    /* wait for leftover from queue, if service was unable to cope with the traffic */
    fprintf(stderr, "Waiting 30 for queues to flush at bridges...\n");
    sleep(60);
    
    if (EXFAIL == tpcall(argv[1], (char *)p_ub, 0L, (char **)&p_ub, &rsplen, 0))
    {
        NDRX_LOG(log_error, "%s failed: %s", argv[1], tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    /* read the value */
    if (EXSUCCEED!=Bget(p_ub, T_LONG_FLD, 0, (char *)&sentread, 0L))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to get T_LONG_FLD: %s", Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    if (sentread!=sent)
    {
        NDRX_LOG(log_error, "TESTERROR: sent: %d but server have seen: %d", 
                sent, sentread);
        EXFAIL_OUT(ret);
    }
    
out:
    tpterm();
    fprintf(stderr, "Exit with %d\n", ret);

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
