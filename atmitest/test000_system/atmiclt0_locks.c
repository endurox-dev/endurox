/**
 * @brief System tests, Check locking performance. This is informative
 *  does not do any real testing.
 *
 * @file atmiclt0_locks.c
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

#include "test000.h"
#include "nstopwatch.h"

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

#define MAX_LOOP    10000000 /* some good number */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/

pthread_rwlock_t M_rwlock = PTHREAD_RWLOCK_INITIALIZER;
long M_rw_spent[2];
long M_mutex_spent[2];
MUTEX_LOCKDECL(M_mutex)
volatile long some_glob=0;

/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/


/**
 * This perform read lock on shared mutex
 * @param arg
 * @return 
 */
void* t1_lockedrw(void *arg)
{    
    long i;
    int ret;
    ndrx_stopwatch_t t;
    long idx = (long)arg;
    
    ndrx_stopwatch_reset(&t);
    
    for (i=0; i<MAX_LOOP; i++)
    {
        if ((ret=pthread_rwlock_rdlock(&M_rwlock)) != 0) 
        {
            fprintf(stderr,"can't acquire read lock (%ld): %d\n", (long)arg, ret);
            exit(-1);
        }
        
        some_glob++;
        
        if ((ret=pthread_rwlock_unlock(&M_rwlock)) != 0) 
        {
            fprintf(stderr,"can't acquire read lock (%ld): %d\n", (long)arg, ret);
            exit(-1);
        }
    }
    
    M_rw_spent[idx] = ndrx_stopwatch_get_delta(&t);
    
    return NULL;
}

/**
 * This perform read lock on shared mutex
 * @param arg
 * @return 
 */
void* t1_mutex(void *arg)
{    
    long i;
    ndrx_stopwatch_t t;
    long idx = (long)arg;
    ndrx_stopwatch_reset(&t);
    
    for (i=0; i<MAX_LOOP; i++)
    {
        MUTEX_LOCK_V(M_mutex);
        some_glob++;
        MUTEX_UNLOCK_V(M_mutex);
    }
    
    M_mutex_spent[idx] = ndrx_stopwatch_get_delta(&t);
    return NULL;
}

int main( int argc , char **argv )
{
    pthread_t pth1={0}, pth2={0};
    pthread_t pth3={0}, pth4={0};
    void *mb1=(void *)0, *mb2 =(void *)1;
    pthread_attr_t pthrat={0};
    pthread_attr_init(&pthrat);
    pthread_attr_setstacksize(&pthrat, 1<<20);
    unsigned int n_pth=0;
    long totrw;
    long totmut;
    int ret;
    
    fprintf(stderr, "Test RW lock speed...\n");
    
    if (0!=(ret=pthread_create( &pth1, &pthrat, t1_lockedrw, mb1)))
    {
        fprintf(stderr, "RW 1 fail create: %d\n", ret);
        return -1;
    }
    
    if (0!=(ret=pthread_create( &pth2, &pthrat, t1_lockedrw, mb2)))
    {
        fprintf(stderr, "RW 2 fail create: %d\n", ret);
        return -1;
    }
    
    pthread_join( pth1, NULL );
    pthread_join( pth2, NULL );
    
    
    fprintf(stderr, "Test Mutex lock speed...\n");
    n_pth=0;
    
    if (0!=(ret=pthread_create( &pth3, &pthrat, t1_mutex, mb1)))
    {
        fprintf(stderr, "Mut 1 fail create: %d\n", ret);
        return -1;
    }
    
    if (0!=(ret=pthread_create( &pth4, &pthrat, t1_mutex, mb2)))
    {
        fprintf(stderr, "Mut 2 fail create: %d\n", ret);
        return -1;
    }
    
    pthread_join( pth3, NULL );
    pthread_join( pth4, NULL );
    
    pthread_attr_destroy(&pthrat);

    totrw = M_rw_spent[0] + M_rw_spent[1];
    totmut = M_mutex_spent[0] + M_mutex_spent[1];
    
    fprintf(stderr,"Results %ld: rw1: %ld ms rw2: %ld ms, mut1: %ld ms, mut2: %ld ms\n", 
            some_glob, M_rw_spent[0], M_rw_spent[1], M_mutex_spent[0], M_mutex_spent[1]);
    
    fprintf(stderr,"Totals: rw: %ld vs mut %ld\n", 
            totrw, totmut);
    
    return 0;
}

/* vim: set ts=4 sw=4 et smartindent: */
