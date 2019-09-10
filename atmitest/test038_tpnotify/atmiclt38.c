/**
 * @brief xadmin down tests
 *
 * @file atmiclt38.c
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
#include <ubfutil.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define MAX_CALLS       10000
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
int M_replies_got = 0;
int M_notifs_got = 0;
int M_calls_made = 0;
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
int handle_replies(UBFH **pp_ub, int num)
{
    int ret = EXSUCCEED;
    long len;
    int cd;
    int i;
    
    /* Process notifs... */
    if (0==num)
    {
        while (EXSUCCEED==tpgetrply(&cd, (char **)pp_ub, &len, TPGETANY | TPNOBLOCK) 
                && 0!=cd)
        {
            ndrx_debug_dump_UBF(log_error, "Got reply", *pp_ub);
            M_replies_got++;
        }
    }
    else
    {
        for (i=0; i<num; i++)
	    {
            if (EXSUCCEED!=tpgetrply(&cd, (char **)pp_ub, &len, TPGETANY))
            {
                    NDRX_LOG(log_error, "TESTERROR! Failed to get rply!");
                    EXFAIL_OUT(ret);
            }
            ndrx_debug_dump_UBF(log_error, "Got reply", *pp_ub);
            M_replies_got++;
        }
    }
    
out:
    return ret;
}

/**
 * Do the test call to the server
 */
int main(int argc, char** argv) 
{
    int ret = EXSUCCEED;
    int i;
    char tmp[20];
    UBFH *p_ub = NULL;
    void (*prev_handler) (char *data, long len, long flags);
    
    if (EXSUCCEED!=tpinit(NULL))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to init!!!!");
        EXFAIL_OUT(ret);
    }
    
    if (NULL!=(prev_handler = tpsetunsol(notification_callback)))
    {
        NDRX_LOG(log_error, "TESTERRORR: Previous handler must be NULL!");
        EXFAIL_OUT(ret);
    }
    
    if (notification_callback!= tpsetunsol(notification_callback))
    {
        NDRX_LOG(log_error, "TESTERRORR: Previous handler must be "
                "notification_callback()!");
        EXFAIL_OUT(ret);
    }
    
    /* Allocate some buffer */
    if (NULL==(p_ub = (UBFH *)tpalloc("UBF", NULL, 1024)))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to allocate test buffer!");
        EXFAIL_OUT(ret);
    }
    
    /* So we will call lots of local and remote services,
     * The service must send back notification.
     */
    for (i=1; i<MAX_CALLS+1; i++)
    {
        /* Let servers to provide back replies... */
        /* Have a fixed buffer len, so that we do not need to realloc... */
        
        snprintf(tmp, sizeof(tmp), "AA%02ld%08d", tpgetnodeid(), i);
        
restart:
        if (EXSUCCEED!=Bchg(p_ub, T_STRING_FLD, 0, tmp, 0L))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to set T_STRING_FLD to [%s]: %s", 
                    tmp, Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
        
        /* Do Some A calls... */
        
        /* Remote service & local services */
        /* maybe async call, with retry? 
         * as we get deadlock here: atmisv38 does tpnotify -> tpbrdcstsv does send to us
         * but we try to send to atmisv38. Our Q gets full and we are stuck..
         */

        if (tpacall("SVC38_01", (char *)p_ub, 0L, TPNOBLOCK)<=0)
        {
            if (TPEBLOCK==tperrno || TPELIMIT==tperrno)
            {
                NDRX_LOG(log_error, "Additional handle_replies %d", tperrno);
                if (EXSUCCEED!=handle_replies(&p_ub, 0))
                {
                    NDRX_LOG(log_error, "handle_replies() failed");
                    EXFAIL_OUT(ret);
                }
                /* also restart the value */
                goto restart;
            }
            else
            {
                NDRX_LOG(log_error, "TESTERROR: Failed to call [SVC38_01]: %s",
                    tpstrerror(tperrno));
                EXFAIL_OUT(ret);
            }
        }

        M_calls_made++;

restart2:
        if (tpacall("SVC38_02", (char *)p_ub, 0L, 0L)<=0)
        {
            /* try to receive something if we got limit... */
            if (TPELIMIT==tperrno)
            {
                NDRX_LOG(log_error, "Additional handle_replies %d", tperrno);
                if (EXSUCCEED!=handle_replies(&p_ub, 0))
                {
                    NDRX_LOG(log_error, "handle_replies() failed");
                    EXFAIL_OUT(ret);
                }
                goto restart2;
            }
            else
            {
                NDRX_LOG(log_error, "TESTERROR: Failed to call [SVC38_02]: %s",
                        tpstrerror(tperrno));
                EXFAIL_OUT(ret);
            }
        }
        M_calls_made++;
        if (0==i%10)
        {	
            if (EXSUCCEED!=handle_replies(&p_ub, 0))
            {
                NDRX_LOG(log_error, "handle_replies() failed");
                EXFAIL_OUT(ret);
            }
        }
    }
    
    i=0; /* try for 30 sec... */
    while (i<1000000 && (M_replies_got < M_calls_made || M_notifs_got < M_calls_made))
    {
        /* Let all replies come in... */
        NDRX_LOG(log_warn, "Waiting for replies...");
        usleep(1000); /* 0.01 sec */
        if (EXSUCCEED!=handle_replies(&p_ub,  0))
        {
             NDRX_LOG(log_error, "TESTERROR: handle_replies() failed");
             EXFAIL_OUT(ret);
        }
	
        i++;
    }
    
    /* Reply from both domains */
    if (M_calls_made!=M_notifs_got)
    {
        NDRX_LOG(log_error, "TESTERROR: M_notifs_got = %d, expected: %d", 
                M_notifs_got, M_calls_made);
        EXFAIL_OUT(ret);
    }
    
    /* Reply from both domains */
    if (M_calls_made!=M_replies_got)
    {
        NDRX_LOG(log_error, "TESTERROR: M_replies_got = %d, expected: %d", 
                M_replies_got, M_calls_made);
        EXFAIL_OUT(ret);
    }
    
out:

    if (NULL!=p_ub)
    {
        tpfree((char *)p_ub);
    }

    return ret;
    
}


/* vim: set ts=4 sw=4 et smartindent: */
