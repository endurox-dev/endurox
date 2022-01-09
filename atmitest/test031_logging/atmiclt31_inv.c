/**
 * @brief Test case verifies that failed tplogsetreqfile() redirects logger
 *  to stderr, and that it can correctly use it.
 *
 * @file atmiclt31_inv.c
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
#include <unistd.h>

#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>
#include <ndebugcmn.h>
#include <test.fd.h>
#include <ndrstandard.h>
#include <nstdutil.h>
#include <exassert.h>
#include <nstd_int.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/


/**
 * Bug #755 the bug did close stderr,
 * on Aix that caused segmentation fault.
 */
void do_thread_work ( void *ptr )
{
    int i;
    
    for (i=0; i<10000; i++)
    {
        NDRX_LOG(log_error, "HELLO");
        tplogsetreqfile(NULL, "/no/such/file/right", NULL);
        NDRX_LOG(log_error, "HELLO2");
        tplogclosereqfile();
    }
    
    /* tplogsetreqfile() does full client init */
    tpterm();
    
}

/*
 * Do the test call to the server
 */
int main(int argc, char** argv)
{
    int ret = EXSUCCEED;
    pthread_t thread1, thread2;  /* thread variables */
    
    /* run off two threads... (each will have it's own log file) */
    pthread_create (&thread1, NULL, (void *) &do_thread_work, NULL);
    pthread_create (&thread2, NULL, (void *) &do_thread_work, NULL);
    
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    
    
out:

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
