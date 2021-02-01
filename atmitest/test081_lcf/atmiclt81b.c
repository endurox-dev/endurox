/**
 * @brief Housekeeping testing
 *
 * @file atmiclt35.c
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

#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>
#include <test.fd.h>
#include <ndrstandard.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/*
 * Do the test call to the server
 */
void do_thread_work ( void *ptr )
{
    char tmp[511];
    int i=0;
    
    /* we will configure to lower log level... with two messages one at lower another at higher*/
    snprintf(tmp, sizeof(tmp), "ndrx=4 file=./%s", (char *)ptr);
    
    if (EXSUCCEED!=tplogconfig(LOG_FACILITY_NDRX_THREAD, EXFAIL, tmp, "", ""))
    {
        NDRX_LOG(log_error, "TESTERROR! Failed to configure logger");
        exit(1);
    }
    
    do
    {
        i++;
        NDRX_LOG(log_debug, "HELLO LEV 5 - step %d", i);
        NDRX_LOG(log_info, "HELLO LEV 4 - step %d", i);
        usleep(1000);
    } while (1);
}

/*
 * Do the test call to the server
 */
int main(int argc, char** argv) 
{
    int i=0;
    pthread_t thread1, thread2;
    void *arg1 = "CLT1.log";
    void *arg2 = "CLT2.log";
    
    tpinit(NULL); /* pull off init from main thread too... */
    
    /* create threads 1 and 2 */    
    pthread_create (&thread1, NULL, (void *) &do_thread_work, arg1);
    pthread_create (&thread2, NULL, (void *) &do_thread_work, arg2);

    /* also main thread is doing writes  -> this will init form cpm... */
    do
    {
        i++;
        NDRX_LOG(log_debug, "HELLO LEV 5 - step %d", i);
        NDRX_LOG(log_info, "HELLO LEV 4 - step %d", i);
        usleep(1000);
    } while (1);
    
    exit(0);
}

/* vim: set ts=4 sw=4 et smartindent: */
