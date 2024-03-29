/**
 * @brief Enqeueu & 
 *
 * @file atmiclt104.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2023, Mavimax, Ltd. All Rights Reserved.
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

#include <exassert.h>
#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>
#include <test.fd.h>
#include <ndrstandard.h>
#include <nstopwatch.h>
#include <fcntl.h>
#include <unistd.h>
#include <nstdutil.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
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
    UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, 56000);
    long rsplen;
    int i;
    int ret=EXSUCCEED;
    TPQCTL qctl;
    short num;
    long len;

    if (argc < 3 || 0==strcmp(argv[1], "deq") && argc < 4)
    {
        fprintf(stderr, "Usage: %s {enq <msg_no> | deq <msg_tot> <broken_txn_flag>}\n", argv[0]);
        exit(EXFAIL);
    }

    NDRX_ASSERT_TP_OUT(EXSUCCEED==tpopen(), "Failed to topen()");
    NDRX_ASSERT_TP_OUT(EXSUCCEED==tpbegin(30, 0), "Failed to tpbegin()");

    num = atoi(argv[2]);

    if (0==strcmp(argv[1], "enq"))
    {
        if (EXSUCCEED!=Bchg(p_ub, T_SHORT_FLD, 0, (char *)&num, 0L))
        {
            fprintf(stderr, "Failed to set T_SHORT_FLD\n");
            EXFAIL_OUT(ret);
        }

        /* enqueue buffer */
        memset(&qctl, 0, sizeof(qctl));

        if (EXSUCCEED!=tpenqueue("TESTSP", "Q1", &qctl, (char *)p_ub, 0, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: tpenqueue() to `Q1' failed %s diag: %d:%s",
                            tpstrerror(tperrno), qctl.diagnostic, qctl.diagmsg);
                    EXFAIL_OUT(ret);
            EXFAIL_OUT(ret);
        }

        NDRX_ASSERT_TP_OUT(EXSUCCEED==tpcommit(0), "Failed to tpcommit()");

    }
    else if (0==strcmp(argv[1], "deq"))
    {
        /* read number of message and match slots, check for duplicates */
        short messages[num];
        short val;
        int j;
        memset(messages, 0, sizeof(messages));

        /* read from q1 or q2, if no msg present, then generate error */
        memset(&qctl, 0, sizeof(qctl));
        while (EXSUCCEED==tpdequeue("TESTSP", "Q1", &qctl, (char **)&p_ub, &len, 0))
        {
            if (EXSUCCEED!=Bget(p_ub, T_SHORT_FLD, 0, (char *)&val, NULL))
            {
                NDRX_LOG(log_error, "TESTERROR: Failed to get T_SHORT_FLD: %s", Bstrerror(Berror));
                EXFAIL_OUT(ret);
            }

            if (val<0 || val>=num)
            {
                NDRX_LOG(log_error, "TESTERROR: Message %d out of range in Q1", val);
                EXFAIL_OUT(ret);
            }

            if (messages[val]!=0)
            {
                NDRX_LOG(log_error, "TESTERROR: Duplicate message %d in Q1", val);
                EXFAIL_OUT(ret);
            }
            NDRX_LOG(log_error, "GOT val=%hd", val);
            messages[val] = 1;
            memset(&qctl, 0, sizeof(qctl));
        }

        if (TPEDIAGNOSTIC!=tperrno)
        {
            NDRX_LOG(log_error, "TESTRROR: Expected TPEDIAGNOSTIC, got %s", tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }

        NDRX_LOG(log_error, "tpdequeue failed %s diag: %d:%s",
                    tpstrerror(tperrno), qctl.diagnostic, qctl.diagmsg);

        for (j=0; j<num; j++)
        {
            if (messages[j]!=1)
            {
                NDRX_LOG(log_error, "Message %d not found in Q", j);
                NDRX_ASSERT_TP_OUT(EXSUCCEED==tpabort(0), "Failed to tpcommit()");
                EXFAIL_OUT(ret);
            }
        }
        
        /* commit if needed... */
        if ('Y'==argv[3][0])
        {
            /* jsut terminate, with transaction in progress... */
            exit(0);
        }

        NDRX_ASSERT_TP_OUT(EXSUCCEED==tpcommit(0), "Failed to tpcommit()");
    }
    else
    {
        NDRX_LOG(log_error, "Unknown command %s", argv[1]);
        exit(EXFAIL);
    }
    
out:
    tpterm();
    fprintf(stderr, "Exit with %d\n", ret);

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
