/**
 * @brief Test buildserver, buildclient and buildtms - client
 *  This will run with default switch.
 *  - tms will be built with TestSw
 *  - atmisv71 will be built with TestSw too.
 *
 * @file atmiclt71_1.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2023, Mavimax, Ltd. All Rights Reserved.
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
#include "test71.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*---------------------------Externs------------------------------------*/
extern int run_tran_services(void);
extern int run_echo_services(void);
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Do the test call to the server
 */
int run_loop(int argc, char** argv)
{
    int ret = EXSUCCEED;
    
    if (EXSUCCEED!=run_tran_services())
    {
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=run_echo_services())
    {
        EXFAIL_OUT(ret);
    }
    
out:
    tpterm();
    fprintf(stderr, "Exit with %d\n", ret);

    return ret;
}

#ifdef __cplusplus
} // extern "C"
#endif
/* vim: set ts=4 sw=4 et smartindent: */
