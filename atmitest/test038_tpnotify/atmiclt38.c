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

#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>
#include <test.fd.h>
#include <ndrstandard.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define MAX_CALLS       10000
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
int M_replies_got = 0;
int M_notifs_got = 0;
/*---------------------------Prototypes---------------------------------*/

/**
 * Set notification handler 
 * @return 
 */
void notification_callback (char *data, long len, long flags)
{
    UBFH *p_ub = (UBFH *)data;
    NDRX_LOG(log_info, "Got notification...");
    /* Dump UBF buffer... */
    ndrx_debug_dump_UBF(log_error, "notification_callback", p_ub);
    M_notifs_got++;
}

/**
 * Read the replies from ATMI subsystem.
 * @return SUCCEED/FAIL
 */
int handle_replies(UBFH **pp_ub)
{
    int ret = SUCCEED;
    long len;
    int cd;
    
    /* Read the replies... */
    while (SUCCEED==tpgetrply(&cd, (char **)pp_ub, &len, TPGETANY | TPNOBLOCK) 
            && 0!=cd)
    {
        ndrx_debug_dump_UBF(log_error, "Got reply", *pp_ub);
        M_replies_got++;
    }
    
out:
    return ret;
}

/**
 * Do the test call to the server
 */
int main(int argc, char** argv) 
{
    int ret = SUCCEED;
    int i;
    char tmp[20];
    UBFH *p_ub = NULL;
    void (*prev_handler) (char *data, long len, long flags);
    
    if (SUCCEED!=tpinit(NULL))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to init!!!!");
        FAIL_OUT(ret);
    }
    
    if (NULL!=(prev_handler = tpsetunsol(notification_callback)))
    {
        NDRX_LOG(log_error, "TESTERRORR: Previous handler must be NULL!");
        FAIL_OUT(ret);
    }
    
    if (notification_callback!= tpsetunsol(notification_callback))
    {
        NDRX_LOG(log_error, "TESTERRORR: Previous handler must be "
                "notification_callback()!");
        FAIL_OUT(ret);
    }
    
    /* Allocate some buffer */
    if (NULL==(p_ub = (UBFH *)tpalloc("UBF", NULL, 1024)))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to allocate test buffer!");
        FAIL_OUT(ret);
    }
    
    /* So we will call lots of local and remote services,
     * The service must send back notification.
     */
    for (i=0; i<MAX_CALLS; i++)
    {
        /* Let servers to provide back replies... */
        /* Have a fixed buffer len, so that we do not need to realloc... */
        
        if (SUCCEED!=handle_replies(&p_ub))
        {
            NDRX_LOG(log_error, "handle_replies() failed");
            FAIL_OUT(ret);
        }
        
        snprintf(tmp, sizeof(tmp), "AA%02ld%08d", tpgetnodeid(), i);
        
        if (SUCCEED!=Bchg(p_ub, T_STRING_FLD, 0, tmp, 0L))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to set T_STRING_FLD to [%s]: %s", 
                    tmp, Bstrerror(Berror));
            FAIL_OUT(ret);
        }
        
        /* Do Some A calls... */
        
        if (i % 100)
        {
            usleep(500);
        }
        /* Local service 
        if (tpacall())
         * */
        
        /* Remote service & local services */
        if (tpacall("SVC38_01", (char *)p_ub, 0L, 0L)<=0)
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to call [SVC38_01]: %s",
                    tpstrerror(tperrno));
            FAIL_OUT(ret);
        }
        
        if (tpacall("SVC38_02", (char *)p_ub, 0L, 0L)<=0)
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to call [SVC38_02]: %s",
                    tpstrerror(tperrno));
            FAIL_OUT(ret);
        }
        
    }
    
    /* Let all replies come in... */
    sleep(5);
    if (SUCCEED!=handle_replies(&p_ub))
    {
        NDRX_LOG(log_error, "TESTERROR: handle_replies() failed");
        FAIL_OUT(ret);
    }
    
    /* Reply from both domains */
    if (MAX_CALLS*2!=M_notifs_got)
    {
        NDRX_LOG(log_error, "TESTERROR: M_notifs_got = %d, expected: %d", 
                M_notifs_got, MAX_CALLS*2);
        FAIL_OUT(ret);
    }
    
    /* Reply from both domains */
    if (MAX_CALLS*2!=M_replies_got)
    {
        NDRX_LOG(log_error, "TESTERROR: M_replies_got = %d, expected: %d", 
                M_replies_got, MAX_CALLS*2);
        FAIL_OUT(ret);
    }
    
out:

    if (NULL!=p_ub)
    {
        tpfree((char *)p_ub);
    }

    return ret;
    
}


