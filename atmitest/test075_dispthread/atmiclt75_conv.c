/**
 * @brief Test Enduro/X server dispatch threading - client
 *
 * @file atmiclt75_conv.c
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
#include "test75.h"
#include "exassert.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define TEST_THREADS            4   /**< number of threads used         */
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
    int i, j;
    int ret=EXSUCCEED;
    int cd[TEST_THREADS];
    long revent, len;
    for (i=0; i<100000; i++)
    {
        char *buf[TEST_THREADS];
        
        for (j=0; j<TEST_THREADS; j++)
        {
            buf[j]= tpalloc("STRING", NULL, 1024);
            NDRX_ASSERT_TP_OUT( (NULL!=buf[j]), "tpalloc failed %d", j);
        }
        
        /* connect to the server */
        for (j=0; j<TEST_THREADS; j++)
        {
            strcpy(buf[j], "HELLO");
            cd[j]=tpconnect("CONVSV1", buf[j], 0, TPSENDONLY);
            NDRX_ASSERT_TP_OUT( (EXFAIL!=cd[j]), "Failed to connect to CONVSV1!");
        }
        
        /* give control  */
        for (j=0; j<TEST_THREADS; j++)
        {
            strcpy(buf[j], "CLWAIT");
            revent=0;
            
            NDRX_ASSERT_TP_OUT( (EXSUCCEED==tpsend(cd[j], buf[j], 0, TPRECVONLY, &revent)), 
                    "Failed send CLWAIT");
        }
        
        /* now do broadcast... */
        for (j=0; j<9; j++)
        {
            NDRX_ASSERT_UBF_OUT((0==tpbroadcast("", "", "atmi.sv75_conv", NULL, 0, 0)), 
                    "Failed to broadcast");
        }
        
        /* now receive data... */
        for (j=0; j<TEST_THREADS; j++)
        {
            revent=0;
            NDRX_ASSERT_TP_OUT( (EXSUCCEED==tprecv(cd[j], &buf[j], &len, 0, &revent)), 
                    "Failed to get counts");
            NDRX_ASSERT_VAL_OUT( (0==revent),  "Invalid revent");
            NDRX_ASSERT_VAL_OUT( (0==strcmp(buf[j], "9")),  
                    "Invalid count received but got: %s", buf[j]);
        }
        
        /* expect to finish the conv */
        for (j=0; j<TEST_THREADS; j++)
        {
            revent=0;
            NDRX_ASSERT_TP_OUT( (EXFAIL==tprecv(cd[j], &buf[j], &len, 0, &revent)), 
                    "Failed to recv");
            
            NDRX_ASSERT_TP_OUT( (TPEEVENT==tperrno),  "Expected TPEEVENT");
            NDRX_ASSERT_TP_OUT( (TPEV_SVCSUCC==revent),  "Expected TPEV_SVCSUCC");
        }
        
        for (j=0; j<TEST_THREADS; j++)
        {
            if (NULL!=buf[j])
            {
                tpfree(buf[j]);
            }
        }
    }
        
out:
    tpterm();
    fprintf(stderr, "Exit with %d\n", ret);

    return ret;
    
}

/* vim: set ts=4 sw=4 et smartindent: */
