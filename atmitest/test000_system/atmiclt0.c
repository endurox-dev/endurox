/**
 * @brief System tests, firstly Thread Local Storage (TLS) must work
 *
 * @file atmiclt0.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or Mavimax's license for commercial use.
 * -----------------------------------------------------------------------------
 * GPL license:
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
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

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
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

int main( int argc , char **argv )
{
    pthread_t pth1={0}, pth2={0};
    void *mb1=0, *mb2 =0;
    pthread_attr_t pthrat={0};
    pthread_attr_init(&pthrat);
    pthread_attr_setstacksize(&pthrat, 1<<20);
    unsigned int n_pth=0;

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


