/**
 * @brief Basic test client
 *
 * @file atmiclt005.c
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

#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>
#include <test.fd.h>
#include <ndrstandard.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * This basically tests the normal case when all have been finished OK!
 * @return
 */
int test_normal_case(void)
{
  UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, 1024);
    int ret=EXSUCCEED;
    int cd;
    long revent;
    int recv_continue = 1;
    int tp_errno;
    int rcv_count = 0;
    if (EXFAIL == (cd = tpconnect("CONVSV",
                                    NULL,
                                    0,
                                    TPNOTRAN | /* untl XA   */
                                    TPSENDONLY)))
    {
        NDRX_LOG(log_error, "TESTERROR: connect error %d", tperrno );
        ret = EXFAIL;
        goto out;
    }
    NDRX_LOG(log_debug, "Connected OK, cd = %d", cd );

    /* Now send configuration paramters */

    if (EXFAIL==Bchg(p_ub, T_STRING_2_FLD, 0, "TERMINAL_T", 0L))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to set T_STRING_2_FLD: %d",
                                        Berror );
        ret = EXFAIL;
        goto out;
    }

    /* Send the configuration stuff to the server */
    if (EXFAIL == tpsend(cd, (char *)p_ub, 0L, TPRECVONLY, &revent))
    {
        tp_errno = tperrno;
        if (TPEEVENT == tp_errno)
        {
                NDRX_LOG(log_error,
                                 "TESTERROR: Unexpected conv event %lx", revent );
        }
        else
        {
                NDRX_LOG(log_error, "send error %d", tp_errno );
        }
        ret = EXFAIL;
        goto out;
    }

    while (recv_continue)
    {
        recv_continue=0;
        if (EXFAIL == tprecv(cd,
                            (char **)&p_ub,
                            0L,
                            0L,
                            &revent))
        {
            ret = EXFAIL;
            tp_errno = tperrno;
            if (TPEEVENT == tp_errno)
            {
                    if (TPEV_SENDONLY == revent)
                    {
                            if (EXFAIL == tpsend(cd, NULL,
                                             0L, TPRECVONLY, &revent))
                            {
                                    NDRX_LOG(log_error,
                                             "TESTERROR: Send failed %d", tperrno);
                                    ret=EXFAIL;
                                    goto out;
                            }
                            else
                            {
                                    recv_continue=1;
                                    ret = EXSUCCEED;
                            }
                    }
                    else if (TPEV_SVCSUCC == revent)
                            ret = EXSUCCEED;
                    else
                    {
                            NDRX_LOG(log_error,
                                     "TESTERROR: Unexpected conv event %lx", revent );
                            ret=EXFAIL;
                            goto out;
                    }
            }
            else
            {
                    NDRX_LOG(log_error, "TESTERROR: recv error %d", tp_errno  );
                    ret = EXFAIL;
                    goto out;
            }
        }
        else
        {
            Bfprint(p_ub, stderr);
            rcv_count++;
            recv_continue=1;
        }
    }

    if (100!=rcv_count)
    {
        NDRX_LOG(log_error, "TESTERROR: Did not receive 100x config details, but %d!!!", rcv_count);
        ret=EXFAIL;
        goto out;
    }

out:
    return ret;
}

/**
 * TODO: Test the case when client aborts!
 * We will see how the actually server acts in this case??!?!
 */
int test_clt_abort_case(void)
{
    int ret=EXSUCCEED;
    
out:
    return ret;
}
/*
 * Do the test call to the server
 */
int main(int argc, char** argv) {
    int ret=EXSUCCEED;

    if (EXFAIL==test_normal_case())
    {
        ret=EXFAIL;
        goto out;
    }
    
out:
    tpterm();
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
