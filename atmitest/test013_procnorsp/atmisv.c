/**
 *
 * @file atmisv.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * AGPL or Mavimax's license for commercial use.
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

/* Stuff for sockets testing: */
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <ndebug.h>
#include <atmi.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <test.fd.h>


#define LONG_SLEEP          3600

exprivate int M_long_shut = 0;


/*
 * This is test service!
 */
void TESTSVFN (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;
    UBFH *p_ub = (UBFH *)p_svc->data;
    
    NDRX_LOG(log_debug, "TESTSVFN got call");

out:
    tpreturn(  ret==EXSUCCEED?TPSUCCESS:TPFAIL,
                0L,
                (char *)p_ub,
                0L,
                0L);
}
/*
 * Do initialization
 */
int NDRX_INTEGRA(tpsvrinit)(int argc, char **argv)
{
 
    FILE *f;
    char str[100]={EXEOS,EXEOS};
    NDRX_LOG(log_debug, "tpsvrinit called");
    
    if (argc<10)
    {
        NDRX_LOG(log_error, "tpsvrinit: 10th arg should be test case name!");
    }
    else
    {
        NDRX_LOG(log_debug, "argv[9]=[%s]", argv[9]);
    }
    
    if (NULL==(f=NDRX_FOPEN("case_type", "r")))
    {
        NDRX_LOG(log_error, "Failed to open case_type: %s", strerror(errno));
        exit(EXFAIL);
    }
    
    if (NULL==fgets (str, sizeof(str) , f))
    {
        NDRX_LOG(log_error, "Failed to read case_type: %s", strerror(errno));
        exit(EXFAIL);
    }
    
    NDRX_FCLOSE(f);
    
    if ('1' == str[0])
    {
        if (0==strcmp(argv[9], "x__long_start"))
        {
            NDRX_LOG(log_error, "Doing long sleep for start");
            sleep(LONG_SLEEP);
        }
        else if (0==strcmp(argv[9], "x__long_stop"))
        {
            NDRX_LOG(log_error, "Will do long sleep in shutdown!");
            M_long_shut = EXTRUE;
        }
    }
    else
    {
        NDRX_LOG(log_error, "Normal process");
    }
        
    if (EXSUCCEED!=tpadvertise("TESTSVFN", TESTSVFN))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to initialize TESTSV (first)!");
    }
    
    return EXSUCCEED;
}

/**
 * Do de-initialization
 */
void NDRX_INTEGRA(tpsvrdone)(void)
{
    /* Will sleep for long time here */
    if (M_long_shut)
    {
        sleep(LONG_SLEEP);
    }
    
    NDRX_LOG(log_debug, "tpsvrdone called");
}
/* vim: set ts=4 sw=4 et smartindent: */
