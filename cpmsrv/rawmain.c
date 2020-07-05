/**
 * @brief Integration mode, mask signals first
 *
 * @file rawmain.c
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
#include <signal.h>
#include <ndebug.h>
#include <memory.h>
#include <atmi.h>
/*---------------------------Externs------------------------------------*/
extern void NDRX_INTEGRA(tpsvrdone)(void);
extern int NDRX_INTEGRA(tpsvrinit) (int argc, char **argv);
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/* dummy, otherwise sigwait() does not return anything... */
exprivate void sig_hand(int sig) {}

/*
 * Forward the call to NDRX
 */
int main(int argc, char** argv) 
{
    sigset_t blockMask;
    struct sigaction sa; /* Seem on AIX signal might slip.. */

    /* block sigchld first as some IPC mechanisms like systemv start
     * threads very early...
     */
    sigemptyset(&blockMask);
    sigaddset(&blockMask, SIGCHLD);
    
    /*
    if (pthread_sigmask(SIG_BLOCK, &blockMask, NULL) == -1)
        */
    if (sigprocmask(SIG_BLOCK, &blockMask, NULL) == -1)
    {
        NDRX_LOG(log_always, "%s: sigprocmask failed: %s", __func__, 
                strerror(errno));
        return EXFAIL;
    }
    
    /* if handler is not set, the sigwait() does not return any results.. */
    sa.sa_handler = sig_hand;
    sigemptyset (&sa.sa_mask);
    sa.sa_flags = SA_RESTART; /* restart system calls please... */
    sigaction (SIGCHLD, &sa, 0);
    
    return ndrx_main_integra(argc, argv, 
            NDRX_INTEGRA(tpsvrinit), NDRX_INTEGRA(tpsvrdone), 0);
}
/* vim: set ts=4 sw=4 et smartindent: */
