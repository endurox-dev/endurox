/**
 * @brief Ndrxd sanity scans / ulog entries - client
 *
 * @file atmiclt74.c
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
#include <signal.h>
#include "test74.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Run die...
 * @param ptr
 */
exprivate void die_thread_fun(void *ptr)
{
    tpinit(NULL);
    sleep(9999);
}

/**
 * Do the test call to the server
 */
int main(int argc, char** argv)
{
    int ret=EXSUCCEED;
    ndrx_stopwatch_t w;
    int i;
    int thread_die = EXFALSE;
    int main_thread_die = EXFALSE;
    struct sigaction sa;
    sa.sa_handler = SIG_IGN; //handle signal by ignoring
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGCHLD, &sa, 0) == -1) {
        perror(0);
        exit(1);
    }
    
    if (argc>1 && 0==strcmp(argv[1], "die"))
    {
        tpinit(NULL);
        /* Lets die */
        return -1;
    }
    
    if (argc>1 && 0==strcmp(argv[1], "thread_die"))
    {
        thread_die=EXTRUE;
    }
    
    if (argc>1 && 0==strcmp(argv[1], "main_die"))
    {
        main_thread_die=EXTRUE;
    }
    
    
    /* now try the test where  */
    
    /* run for some 2 minutes... */
    
    ndrx_stopwatch_reset(&w);
    
    while (ndrx_stopwatch_get_delta_sec(&w) < 120)
    {
#ifdef EX_OS_DARWIN
        /* seems macos has low system resources.. */
        for (i=0; i<25; i++)
#else
        for (i=0; i<100; i++)
#endif
        {
            /* run some fork... */
            if (0==fork())
            {
                if (thread_die || main_thread_die)
                {
                    pthread_t thread1;

                   /* run thread which will die... 
                    * The main thread is OK, but thread dies, thus we need some ULOG.
                    * Sure thing this will have no guarantee, but we can inspect by
                    * our selves..
                    */
                    pthread_attr_t pthread_custom_attr;
                    pthread_attr_init(&pthread_custom_attr);


                    /* create threads 1 and 2 */    
                    pthread_create (&thread1, &pthread_custom_attr, 
                            (void *) &die_thread_fun, NULL);
                }
                
                tpinit(NULL);
                usleep(700000);
                
                /* main thread unclean shutdown.. */
                if (!main_thread_die)
                {
                    tpterm();
                }
                
                return 0;
            }
            usleep(900);
        }
        sleep(1);
    }
    
out:
    
    fprintf(stderr, "Exit with %d\n", ret);

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
