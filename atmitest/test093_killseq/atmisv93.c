/**
 * @brief Test kill signal sequences - server
 *
 * @file atmisv93.c
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
#include <stdio.h>
#include <stdlib.h>
#include <ndebug.h>
#include <ndrstandard.h>
#include <atmi.h>
#include <sys/signal.h>
#include <ubf.h>
#include <test.fd.h>
#include <string.h>
#include <unistd.h>
#include "test93.h"

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

/* number of signal readings: */
int M_signal_2=0;
int M_signal_14=0;
int M_signal_15=0;
int M_is_stock = EXFALSE;
/*---------------------------Prototypes---------------------------------*/

/**
 * register the signal sequences...
 */
exprivate void sighandler(int signo)
{
    switch (signo)
    {
        case 2:
            M_signal_2++;
            break;
        case 14:
            M_signal_14++;
            break;
        case 15:
            M_signal_15++;
            break;
    }
}

/**
 * Do initialisation
 */
int NDRX_INTEGRA(tpsvrinit)(int argc, char **argv)
{
    int ret = EXSUCCEED;
    struct sigaction sigact;
    
    NDRX_LOG(log_debug, "tpsvrinit called [%s]", argv[argc-2]);
    
    sigact.sa_handler = sighandler;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;
    sigaction(2, &sigact, NULL);
    sigaction(14, &sigact, NULL);
    sigaction(15, &sigact, NULL);
    
    if (0==strcmp(argv[argc-2], "stock"))
    {
        NDRX_LOG(log_debug, "Stock mode");
        M_is_stock=EXTRUE;
    }

out:
    return ret;
}

/**
 * Do de-initialisation
 */
void NDRX_INTEGRA(tpsvrdone)(void)
{
    NDRX_LOG(log_debug, "tpsvrdone called");
    
    if (M_is_stock)
    {
        while (0==M_signal_2 || 0==M_signal_15)
        {
            NDRX_LOG(log_debug, "Waiting for signals M_signal_2=%d M_signal_14=%d M_signal_15=%d",
                    M_signal_2, M_signal_14, M_signal_15);
            sleep(1);
        }
        
        if (1==M_signal_2 && 0==M_signal_14 && 1==M_signal_15)
        {
            NDRX_LOG(log_debug, "STOCK SIGNALS OK");
        }
    }
    else
    {
        while (0==M_signal_2 || 0==M_signal_14 || 0==M_signal_15)
        {
            NDRX_LOG(log_debug, "Waiting for signals M_signal_2=%d M_signal_14=%d M_signal_15=%d",
                    M_signal_2, M_signal_14, M_signal_15);
            sleep(1);
        }

        NDRX_LOG(log_debug, "ALL SIGNALS OK");
    }
}

/* vim: set ts=4 sw=4 et smartindent: */
