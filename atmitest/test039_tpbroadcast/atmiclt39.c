/* 
** xadmin down tests
**
** @file atmiclt37.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, Mavimax, Ltd. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or Mavimax's license for commercial use.
** -----------------------------------------------------------------------------
** GPL license:
** 
** This program is free software; you can redistribute it and/or modify it under
** the terms of the GNU General Public License as published by the Free Software
** Foundation; either version 2 of the License, or (at your option) any later
** version.
**
** This program is distributed in the hope that it will be useful, but WITHOUT ANY
** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
** PARTICULAR PURPOSE. See the GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License along with
** this program; if not, write to the Free Software Foundation, Inc., 59 Temple
** Place, Suite 330, Boston, MA 02111-1307 USA
**
** -----------------------------------------------------------------------------
** A commercial use license is available from Mavimax, Ltd
** contact@mavimax.com
** -----------------------------------------------------------------------------
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <signal.h>

#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>
#include <test.fd.h>
#include <ndrstandard.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define MAX_CALLS       1000
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
private int M_shutdown = FALSE;
/*---------------------------Prototypes---------------------------------*/

/**
 * Set notification handler 
 * @return 
 */
void notification_callback (char *data, long len, long flags)
{
    UBFH *p_ub = (UBFH *)data;
    NDRX_LOG(log_info, "Got broadcast...");
    /* Dump UBF buffer... */
    ndrx_debug_dump_UBF(log_error, "notification_callback", p_ub);
}

/**
 * Do the broadcasting
 */
int run_broadcast(void)
{
    int ret = SUCCEED;
    UBFH *p_ub = NULL;
    char tmp[32];
    int i;
    
    if (SUCCEED!=tpinit(NULL))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to init!!!!");
        FAIL_OUT(ret);
    }
    
    if (NULL!=tpsetunsol(notification_callback))
    {
        NDRX_LOG(log_error, "TESTERRORR: Previous handler must be NULL!");
        FAIL_OUT(ret);
    }
      
    /* Allocate some buffer */
    if (NULL==(p_ub = (UBFH *)tpalloc("UBF", NULL, 1024)))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to allocate test buffer!");
        FAIL_OUT(ret);
    }
    
    /**
     * Send simple broadcast to all nodes and clients...
     */
    for (i=0; i<MAX_CALLS; i++)
    {
        /* Let servers to provide back replies... */
        /* Have a fixed buffer len, so that we do not need to realloc... */
        
        snprintf(tmp, sizeof(tmp), "AA%02ld%08d", tpgetnodeid(), i);
        
        if (SUCCEED!=Bchg(p_ub, T_STRING_FLD, 0, tmp, 0L))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to set T_STRING_FLD to [%s]: %s", 
                    tmp, Bstrerror(Berror));
            FAIL_OUT(ret);
        }
        
        if (i % 100)
        {
            usleep(500);
        }
        
        /* generic broadcast applied to all machines... */
        if (SUCCEED!=tpbroadcast(NULL, NULL, NULL, (char *)p_ub, 0L, 0L))
        {
            NDRX_LOG(log_error, "TESTERRROR: Failed to broadcast: %s", 
                    tpstrerror(tperrno));
            FAIL_OUT(ret);
        }
        
        if (FAIL==tpchkunsol())
        {
            NDRX_LOG(log_error, "TESTERROR: tpchkunsol() failed!");
            FAIL_OUT(ret);
        }
    }
    
    /* Send only to B like services */
    for (i=0; i<MAX_CALLS; i++)
    {
        /* Let servers to provide back replies... */
        /* Have a fixed buffer len, so that we do not need to realloc... */
        
        snprintf(tmp, sizeof(tmp), "BB%02ld%08d", tpgetnodeid(), i);
        
        if (SUCCEED!=Bchg(p_ub, T_STRING_FLD, 0, tmp, 0L))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to set T_STRING_FLD to [%s]: %s", 
                    tmp, Bstrerror(Berror));
            FAIL_OUT(ret);
        }
        
        if (i % 100)
        {
            usleep(500);
        }
        
        /* generic broadcast applied to all machines... */
        if (SUCCEED!=tpbroadcast(NULL, NULL, "^.*B.*", (char *)p_ub, 0L, TPREGEXMATCH))
        {
            NDRX_LOG(log_error, "TESTERRROR: Failed to broadcast: %s", 
                    tpstrerror(tperrno));
            FAIL_OUT(ret);
        }
        
        if (FAIL==tpchkunsol())
        {
            NDRX_LOG(log_error, "TESTERROR: tpchkunsol() failed!");
            FAIL_OUT(ret);
        }
    }
    
    /* Send only to 2nd node */
    for (i=0; i<MAX_CALLS; i++)
    {
        /* Let servers to provide back replies... */
        /* Have a fixed buffer len, so that we do not need to realloc... */
        
        snprintf(tmp, sizeof(tmp), "CC%02ld%08d", tpgetnodeid(), i);
        
        if (SUCCEED!=Bchg(p_ub, T_STRING_FLD, 0, tmp, 0L))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to set T_STRING_FLD to [%s]: %s", 
                    tmp, Bstrerror(Berror));
            FAIL_OUT(ret);
        }
        
        if (i % 100)
        {
            usleep(500);
        }
        
        /* generic broadcast applied to all machines... */
        if (SUCCEED!=tpbroadcast("2", NULL, NULL, (char *)p_ub, 0L, 0L))
        {
            NDRX_LOG(log_error, "TESTERRROR: Failed to broadcast: %s", 
                    tpstrerror(tperrno));
            FAIL_OUT(ret);
        }
        
        if (FAIL==tpchkunsol())
        {
            NDRX_LOG(log_error, "TESTERROR: tpchkunsol() failed!");
            FAIL_OUT(ret);
        }
        
    }
    
    /* Send to single client (matching the name) */
    for (i=0; i<MAX_CALLS; i++)
    {
        /* Let servers to provide back replies... */
        /* Have a fixed buffer len, so that we do not need to realloc... */
        
        snprintf(tmp, sizeof(tmp), "DD%02ld%08d", tpgetnodeid(), i);
        
        if (SUCCEED!=Bchg(p_ub, T_STRING_FLD, 0, tmp, 0L))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to set T_STRING_FLD to [%s]: %s", 
                    tmp, Bstrerror(Berror));
            FAIL_OUT(ret);
        }
        
        if (i % 100)
        {
            usleep(500);
        }
        
        /* generic broadcast applied to all machines... */
        if (SUCCEED!=tpbroadcast(NULL, NULL, "atmicltC39", (char *)p_ub, 0L, 0L))
        {
            NDRX_LOG(log_error, "TESTERRROR: Failed to broadcast: %s", 
                    tpstrerror(tperrno));
            FAIL_OUT(ret);
        }
        
        if (FAIL==tpchkunsol())
        {
            NDRX_LOG(log_error, "TESTERROR: tpchkunsol() failed!");
            FAIL_OUT(ret);
        }
    }
    
out:

    if (NULL!=p_ub)
    {
        tpfree((char *)p_ub);
    }
    return ret;
    
}

/**
 * Run listener
 */
int bc_listen(void)
{
    int ret = SUCCEED;
    
    if (NULL!=tpsetunsol(notification_callback))
    {
        NDRX_LOG(log_error, "TESTERRORR: Previous handler must be NULL!");
        FAIL_OUT(ret);
    }
    
    while (!M_shutdown)
    {
        int applied;
        while (FAIL!=(applied=tpchkunsol()))
        {
            if (applied > 0)
            {
                NDRX_LOG(log_debug, "Applied: %d", applied);
            }
            /* Have some sleep */
            usleep(100000);
        }
        
        if (FAIL==applied)
        {
            NDRX_LOG(log_error, "TESTERROR: failed to call tpchkunsol(): %s", 
                    tpstrerror(tperrno));
            FAIL_OUT(ret);
        }
    }
    
out:
    return ret;    
}

void sighandler(int signum)
{
    M_shutdown = TRUE;
}

/**
 * Do the test call to the server
 */
int main(int argc, char** argv) 
{
    int ret = SUCCEED;
    TPINIT init;
    
    if (argc<2)
    {
        NDRX_LOG(log_error, "usage: %s <broadcast|listen|mutted|>")
        FAIL_OUT(ret);
    }

    signal(SIGINT, sighandler);
    signal(SIGTERM, sighandler);
    
    memset(&init, 0, sizeof(init));
    
    
    if (0==strcmp(argv[1], "broadcast"))
    {
        NDRX_LOG(log_error, "Running: broadcast");
        ret = run_broadcast();
    }
    else if (0==strcmp(argv[1], "listen"))
    {
        NDRX_LOG(log_error, "Running: listen");
        /* no flags.. */
        if (SUCCEED!=tpinit(&init))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to init!!!!");
            FAIL_OUT(ret);
        }

        ret = bc_listen();
    }
    else if (0==strcmp(argv[1], "mutted"))
    {
        NDRX_LOG(log_error, "Running: mutted");
        init.flags|=TPU_IGN;
        if (SUCCEED!=tpinit(&init))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to init!!!!");
            FAIL_OUT(ret);
        }
        
        ret = bc_listen();
    }
    
out:

    if (SUCCEED!=ret)
    {
        NDRX_LOG(log_error, "TESTERROR: main() finishing with error %d!", ret);
    }

    return ret;
    
}


