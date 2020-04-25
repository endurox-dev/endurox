/**
 * @brief System tests, firstly Thread Local Storage (TLS) must work
 *
 * @file atmiclt0.c
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
#include <pthread.h>
#include <unistd.h>
#include <ndrstandard.h>
#include <errno.h>
#include <sys/time.h>

#include <ndebug.h>
#include <sys_unix.h>

#include "test000.h"

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/* for osx tests: */
exprivate pthread_mutex_t M_mut = PTHREAD_MUTEX_INITIALIZER;
exprivate pthread_cond_t M_cond = PTHREAD_COND_INITIALIZER;
/*---------------------------Prototypes---------------------------------*/

__thread int M_field;

void *M_ptrs[3] = {NULL, NULL, NULL};

void* t1(void *arg)
{    
    M_ptrs[1] = &M_field;
    sleep(1);
    return NULL;
}

void* t2(void *arg)
{   
    M_ptrs[2] = &M_field;
    sleep(1);
    return NULL;
}

#ifdef EX_OS_DARWIN

void tim(void)
{
 time_t timer;
    char buffer[26];
    struct tm* tm_info;

    timer = time(NULL);
    tm_info = localtime(&timer);

    strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
	fprintf(stderr, "%s\n", buffer);

}

/**
 * Check the busy field..
 * ptrs shall match..
 */
void* osx_chk_thread(void *arg)
{
    ndrx_osx_pthread_cond_t *p_cond;
    p_cond = (ndrx_osx_pthread_cond_t *)&M_cond;

    /* wait on thread */
    MUTEX_LOCK_V(M_mut);

    /*let other thread to set the fields.. inside of pthread_cond_wait() */
    sleep(1);
    if ((char *)p_cond->busy != (char *)&M_mut)
    {
        /* check for https://github.com/apple/darwin-libpthread/blob/master/src/pthread_cond.c updates
         * and fix for given OSX version, needs to add some macros...
         */
        fprintf(stderr, "Cannot access busy field of Darwin pthread library for cond var: %p vs %p\n",
                p_cond->busy, &M_mut);
        exit(-1);
    }
    else
    {
        fprintf(stderr, "Cond var: %p vs %p OK\n", p_cond->busy, &M_mut);
    }
    
    pthread_cond_signal( &M_cond ); 
    MUTEX_UNLOCK_V( M_mut );

    return NULL;
}

/**
 * Check that we have access to cond variable internals...
 * and it still works as in 08/02/2020.
 * @return EXSUCCED / EXFAIL
 */
int osx_chk_cond_work_around(void)
{
    int ret = EXSUCCEED;
    ndrx_osx_pthread_cond_t *p_cond;
    
    pthread_t valth={0};
    p_cond = (ndrx_osx_pthread_cond_t *)&M_cond;

    MUTEX_LOCK_V(M_mut);

    if( (ret=pthread_create( &valth, NULL, osx_chk_thread, NULL)) != 0)
    {
        fprintf(stderr, "Failed to create thread: %d", ret);
        return EXFAIL;
    }

    p_cond->busy = NULL;
    ret = pthread_cond_wait(&M_cond, &M_mut);
    fprintf(stderr, "pthread_cond_wait 2: ret=%d (%s) mut=%p\n", ret, strerror(ret), &M_mut);

    if (EXSUCCEED!=ret)
    {
        fprintf(stderr, "ret=%d Expected 0\n", ret);
        return EXFAIL;
    }

    MUTEX_UNLOCK_V(M_mut);
    
    pthread_join( valth, NULL );
    
    ret = EXSUCCEED;
    
out:
    return ret;    

}
#endif

int main( int argc , char **argv )
{
    pthread_t pth1={0}, pth2={0};
    void *mb1=0, *mb2 =0;
    pthread_attr_t pthrat={0};
    pthread_attr_init(&pthrat);
    pthread_attr_setstacksize(&pthrat, 1<<20);
    unsigned int n_pth=0;
    
    
#ifdef EX_OS_DARWIN
    
    if (EXSUCCEED!=osx_chk_cond_work_around())
    {
        fprintf(stderr, "OSX PTHREAD_PROCESS_SHARED Cond variable work-a-round does not work!\n");
        return EXFAIL;
    }
    
#endif

    M_ptrs[0] = &M_field;

    if( pthread_create( &pth1, &pthrat, t1,&mb1) == 0)
        n_pth += 1;
    if( pthread_create( &pth2, &pthrat, t2,&mb2) == 0)
        n_pth += 1;
    if( n_pth > 0 )
        pthread_join( pth1, &mb1 );
    if( n_pth > 1 )
        pthread_join( pth2, &mb2 );
    pthread_attr_destroy(&pthrat);

    fprintf(stderr,"main : %p %p %p\n", M_ptrs[0], M_ptrs[1], M_ptrs[2]);

    if (M_ptrs[0] == M_ptrs[1] || M_ptrs[0] == M_ptrs[2] ||
            M_ptrs[1] == M_ptrs[2])
    {
        fprintf(stderr, "TESTERROR: Thread Local Storage not working!\n");
        return -1;
    }
    else
    {
        fprintf(stderr, "Thread Local Storage OK!\n");
        return 0;
    }
}


/* vim: set ts=4 sw=4 et smartindent: */
