/**
 * @brief tpbroadcast tests
 *
 * @file atmiclt39.c
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
#include <signal.h>

#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>
#include <test.fd.h>
#include <ndrstandard.h>
#include <ubfutil.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define MAX_CALLS       1000
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
exprivate int M_shutdown = EXFALSE;
/*---------------------------Prototypes---------------------------------*/

int test_tpchkunsol_ret_num(void);

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
    int ret = EXSUCCEED;
    UBFH *p_ub = NULL;
    char tmp[32];
    int i;
    
    if (EXSUCCEED!=tpinit(NULL))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to init!!!!");
        EXFAIL_OUT(ret);
    }
    
    if (NULL!=tpsetunsol(notification_callback))
    {
        NDRX_LOG(log_error, "TESTERRORR: Previous handler must be NULL!");
        EXFAIL_OUT(ret);
    }
      
    /* Allocate some buffer */
    if (NULL==(p_ub = (UBFH *)tpalloc("UBF", NULL, 1024)))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to allocate test buffer!");
        EXFAIL_OUT(ret);
    }
    
    /**
     * Send simple broadcast to all nodes and clients...
     */
    NDRX_LOG(log_info, ">>> Send simple broadcast to all nodes and clients...");
    for (i=0; i<MAX_CALLS; i++)
    {
        /* Let servers to provide back replies... */
        /* Have a fixed buffer len, so that we do not need to realloc... */
        
        snprintf(tmp, sizeof(tmp), "AA%02ld%08d", tpgetnodeid(), i);
        
        if (EXSUCCEED!=Bchg(p_ub, T_STRING_FLD, 0, tmp, 0L))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to set T_STRING_FLD to [%s]: %s", 
                    tmp, Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
        
        if (i % 100)
        {
            usleep(500);
        }
        
        /* generic broadcast applied to all machines... */
        /* seems like we are doing broadcasts to tpbridge threads too..
         * thus will send only to atmi matched clients... */
        if (EXSUCCEED!=tpbroadcast(NULL, NULL, "atmi", (char *)p_ub, 0L, TPREGEXMATCH))
        {
            NDRX_LOG(log_error, "TESTERRROR: Failed to broadcast: %s", 
                    tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
        
        if (EXFAIL==tpchkunsol())
        {
            NDRX_LOG(log_error, "TESTERROR: tpchkunsol() failed!");
            EXFAIL_OUT(ret);
        }
    }
    
    /* Send only to B like services */
    NDRX_LOG(log_info, ">>> Send only to B like services");
    for (i=0; i<MAX_CALLS; i++)
    {
        /* Let servers to provide back replies... */
        /* Have a fixed buffer len, so that we do not need to realloc... */
        
        snprintf(tmp, sizeof(tmp), "BB%02ld%08d", tpgetnodeid(), i);
        
        if (EXSUCCEED!=Bchg(p_ub, T_STRING_FLD, 0, tmp, 0L))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to set T_STRING_FLD to [%s]: %s", 
                    tmp, Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
        
        if (i % 100)
        {
            usleep(500);
        }
        
        /* generic broadcast applied to all machines... */
        if (EXSUCCEED!=tpbroadcast(NULL, NULL, "^.*B.*", (char *)p_ub, 0L, TPREGEXMATCH))
        {
            NDRX_LOG(log_error, "TESTERRROR: Failed to broadcast: %s", 
                    tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
        
        if (EXFAIL==tpchkunsol())
        {
            NDRX_LOG(log_error, "TESTERROR: tpchkunsol() failed!");
            EXFAIL_OUT(ret);
        }
    }
    
    /* Send only to 2nd node */
    NDRX_LOG(log_info, ">>> Send only to 2nd node");
    for (i=0; i<MAX_CALLS; i++)
    {
        /* Let servers to provide back replies... */
        /* Have a fixed buffer len, so that we do not need to realloc... */
        
        snprintf(tmp, sizeof(tmp), "CC%02ld%08d", tpgetnodeid(), i);
        
        if (EXSUCCEED!=Bchg(p_ub, T_STRING_FLD, 0, tmp, 0L))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to set T_STRING_FLD to [%s]: %s", 
                    tmp, Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
        
        if (i % 100)
        {
            usleep(500);
        }
        
        /* generic broadcast applied to all machines... */
        if (EXSUCCEED!=tpbroadcast("2", NULL, "atmi", (char *)p_ub, 0L, TPREGEXMATCH))
        {
            NDRX_LOG(log_error, "TESTERRROR: Failed to broadcast: %s", 
                    tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
        
        if (EXFAIL==tpchkunsol())
        {
            NDRX_LOG(log_error, "TESTERROR: tpchkunsol() failed!");
            EXFAIL_OUT(ret);
        }
        
    }
    
    /* Send to single client (matching the name) */
    NDRX_LOG(log_info, ">>> Send to single client (matching the name)");
    for (i=0; i<MAX_CALLS; i++)
    {
        /* Let servers to provide back replies... */
        /* Have a fixed buffer len, so that we do not need to realloc... */
        
        snprintf(tmp, sizeof(tmp), "DD%02ld%08d", tpgetnodeid(), i);
        
        if (EXSUCCEED!=Bchg(p_ub, T_STRING_FLD, 0, tmp, 0L))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to set T_STRING_FLD to [%s]: %s", 
                    tmp, Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
        
        if (i % 100)
        {
            usleep(500);
        }
        
        /* generic broadcast applied to all machines... */
        if (EXSUCCEED!=tpbroadcast(NULL, NULL, "atmicltC39", (char *)p_ub, 0L, 0L))
        {
            NDRX_LOG(log_error, "TESTERRROR: Failed to broadcast: %s", 
                    tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
        
        if (EXFAIL==tpchkunsol())
        {
            NDRX_LOG(log_error, "TESTERROR: tpchkunsol() failed!");
            EXFAIL_OUT(ret);
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
    int ret = EXSUCCEED;
    
    if (NULL!=tpsetunsol(notification_callback))
    {
        NDRX_LOG(log_error, "TESTERRORR: Previous handler must be NULL!");
        EXFAIL_OUT(ret);
    }
    
    while (!M_shutdown)
    {
        int applied;
        while (EXFAIL!=(applied=tpchkunsol()))
        {
            if (applied > 0)
            {
                NDRX_LOG(log_debug, "Applied: %d", applied);
            }
            /* Have some sleep */
            usleep(100000);
        }
        
        if (EXFAIL==applied)
        {
            NDRX_LOG(log_error, "TESTERROR: failed to call tpchkunsol(): %s", 
                    tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
    }
    
out:
    return ret;    
}
/**
 * Check the number of events received
 * Bug #269
 * @return EXSUCCEED/EXFAIL
 */
int test_tpchkunsol_ret_num(void)
{
    int ret = EXSUCCEED;
    UBFH *p_ub = NULL;
    char tmp[32];
    int i;
    int cnt;
    
    if (NULL!=tpsetunsol(notification_callback))
    {
        NDRX_LOG(log_error, "TESTERRORR: Previous handler must be NULL!");
        EXFAIL_OUT(ret);
    }
      
    /* Allocate some buffer */
    if (NULL==(p_ub = (UBFH *)tpalloc("UBF", NULL, 1024)))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to allocate test buffer!");
        EXFAIL_OUT(ret);
    }
#define NUMCALLS    7
    
    for (i=0; i<NUMCALLS; i++)
    {
        /* Let servers to provide back replies... */
        /* Have a fixed buffer len, so that we do not need to realloc... */
        
        snprintf(tmp, sizeof(tmp), "AA%02ld%08d", tpgetnodeid(), i);
        
        if (EXSUCCEED!=Bchg(p_ub, T_STRING_FLD, 0, tmp, 0L))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to set T_STRING_FLD to [%s]: %s", 
                    tmp, Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
        
        if (EXSUCCEED!=tpbroadcast(NULL, NULL, "atmicltA39", (char *)p_ub, 0L, TPREGEXMATCH))
        {
            NDRX_LOG(log_error, "TESTERRROR: Failed to broadcast: %s", 
                    tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
        
    }
    
    sleep(10);
    
    if (NUMCALLS!=(cnt=tpchkunsol()))
    {
        NDRX_LOG(log_error, "TESTERROR: Expected numcalls: %d but got %d!!!",
                NUMCALLS, cnt);
        EXFAIL_OUT(ret);
    }
    
out:
    return ret;    
}

void sighandler(int signum)
{
    M_shutdown = EXTRUE;
}

/**
 * Do the test call to the server
 */
int main(int argc, char** argv) 
{
    int ret = EXSUCCEED;
    TPINIT init;
    
    if (argc<2)
    {
        NDRX_LOG(log_error, "usage: %s <broadcast|listen|mutted|>")
        EXFAIL_OUT(ret);
    }

    signal(SIGINT, sighandler);
    signal(SIGTERM, sighandler);
    
    memset(&init, 0, sizeof(init));
    
    if (0==strcmp(argv[1], "retnum"))
    {
        NDRX_LOG(log_error, "Running: broadcast");
        ret = test_tpchkunsol_ret_num();
    }
    else if (0==strcmp(argv[1], "broadcast"))
    {
        NDRX_LOG(log_error, "Running: broadcast");
        ret = run_broadcast();
    }
    else if (0==strcmp(argv[1], "listen"))
    {
        NDRX_LOG(log_error, "Running: listen");
        /* no flags.. */
        if (EXSUCCEED!=tpinit(&init))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to init!!!!");
            EXFAIL_OUT(ret);
        }

        ret = bc_listen();
    }
    else if (0==strcmp(argv[1], "mutted"))
    {
        NDRX_LOG(log_error, "Running: mutted");
        init.flags|=TPU_IGN;
        if (EXSUCCEED!=tpinit(&init))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to init!!!!");
            EXFAIL_OUT(ret);
        }
        
        ret = bc_listen();
    }
    
out:

    if (EXSUCCEED!=ret)
    {
        NDRX_LOG(log_error, "TESTERROR: main() finishing with error %d!", ret);
    }

    tpterm();

    return ret;
    
}


