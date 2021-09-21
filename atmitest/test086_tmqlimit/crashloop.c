/**
 * @brief This is ultra high survival test case. We kill the tmsrv/tmqueue
 *  periodically, and also periodically disk fails to log the prepared status.
 *  Test performs self looping i.e. try to forward to non existing service
 *  and put msg back on queue - the same queue is errorq.
 * @file crashloop.c
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
#include <unistd.h>

#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>
#include <test.fd.h>
#include <ndrstandard.h>
#include <ubfutil.h>
#include <nstopwatch.h>
#include <nstdutil.h>
#include <exassert.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/** verify matching req/replies. */
typedef struct 
{
    int num_req;
} test_result_t;

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

#define MAX_TEST    100

/**
 * Crashloop test
 * @param qname queue name for crash loop
 * @return EXSUCCEED/EXFAIL
 */
expublic int basic_crashloop(char *qname)
{
    int ret = EXSUCCEED;
    TPQCTL qc;
    long i;
    long olen;
    char tmpbuf[1024];
    long num;
    UBFH *p_ub = NULL;
    test_result_t rsp[MAX_TEST];
    char cmd[PATH_MAX+1];
    /* run test for 10 min... */
    ndrx_stopwatch_t w;
    
    NDRX_LOG(log_error, "case basic_crashloop");
    
    memset(rsp, 0, sizeof(rsp));
    
    NDRX_ASSERT_TP_OUT((NULL!=(p_ub=(UBFH *)tpalloc("UBF", NULL, 1024))), "Failed to alloc");
    
    for (i=0; i<MAX_TEST; i++)
    {
        NDRX_ASSERT_UBF_OUT( (EXSUCCEED==Bchg(p_ub, T_LONG_FLD, 0, (char *)&i, 0L)), 
                "Failed to set test num");
        
        NDRX_ASSERT_UBF_OUT( (EXSUCCEED==Bchg(p_ub, T_STRING_FLD, 0, "REQ", 0L)), 
                "Failed to set test str");
        
        /* enq */
        memset(&qc, 0, sizeof(qc));
        
        NDRX_ASSERT_TP_OUT((EXSUCCEED==tpenqueue("MYSPACE", qname, &qc, 
                (char *)p_ub, 0, 0L)), "Failed to enq");
    }
    
    
    ndrx_stopwatch_reset(&w);
    
    do
    {
        /* logging fails:*/
        NDRX_ASSERT_VAL_OUT(EXSUCCEED==system("xadmin lcf tcrash -n -A 150 -a"), "system() failed");
        sleep(2);
        
        /* crash the tmsrv: */
        NDRX_ASSERT_VAL_OUT(EXSUCCEED==system("xadmin killall tmqueue tmsrv"), "system() failed");
        sleep(10);
        
        /* restore disk operations */
        NDRX_ASSERT_VAL_OUT(EXSUCCEED==system("xadmin lcf tcrash -n -A 0 -a"), "system() failed");
        sleep(10);
        
        /* crash tmsrv again: */
        NDRX_ASSERT_VAL_OUT(EXSUCCEED==system("xadmin killall tmqueue tmsrv"), "system() failed");
        sleep(10);
        /* run test for 5 min... */
    } while (ndrx_stopwatch_get_delta_sec(&w) < 300);
    
    /* disable autoQ so that we can read results,  restart q if any parked msgs.. */
    NDRX_ASSERT_VAL_OUT(EXSUCCEED==system("xadmin killall tmqueue"), "system() failed");

    /* let tmqueue to boot back: */
    sleep(10);
    snprintf(cmd, sizeof(cmd), "xadmin mqch -n1 -i 100 -q%s,autoq=n", qname);
    NDRX_LOG(log_error, "%s", cmd);
    NDRX_ASSERT_VAL_OUT(EXSUCCEED==system(cmd), "system() failed");
    
    NDRX_LOG(log_error, "xadmin mqlc");
    NDRX_ASSERT_VAL_OUT(EXSUCCEED==system("xadmin mqlc"), "system() failed");
    
    /* Wait messages to settle down */
    sleep(60);
    
    /* print some statistics.. */
    NDRX_LOG(log_error, "xadmin pt");
    NDRX_ASSERT_VAL_OUT(EXSUCCEED==system("xadmin pt"), "system() failed");
    NDRX_LOG(log_error, "xadmin mqlq");
    NDRX_ASSERT_VAL_OUT(EXSUCCEED==system("xadmin mqlq"), "system() failed");
    
    /* read & match results, all msgs should be in place */
    
    for (i=0; i<MAX_TEST; i++)
    {
        memset(&qc, 0, sizeof(qc));
        
        tpfree((char *)p_ub);
        p_ub = NULL;
        
        /* verify req */
        NDRX_ASSERT_TP_OUT((EXSUCCEED==tpdequeue("MYSPACE", qname, &qc, (char **)&p_ub, &olen, 0)), 
                "Must be OK at loop %ld", i);
        
        NDRX_ASSERT_UBF_OUT((EXSUCCEED==Bget(p_ub, T_STRING_FLD, 0, tmpbuf, NULL)), 
                "Failed to get data fld");
        NDRX_ASSERT_VAL_OUT(0==strcmp(tmpbuf, "REQ"), "Request not found: %s", tmpbuf);
        NDRX_ASSERT_UBF_OUT((EXSUCCEED==Bget(p_ub, T_LONG_FLD, 0, (char *)&num, NULL)), 
                "Failed to get num");
        rsp[num].num_req++;
                
    }
    
    /* no any extra msgs... */
    memset(&qc, 0, sizeof(qc));
    NDRX_ASSERT_TP_OUT((EXFAIL==tpdequeue("MYSPACE", qname, &qc, (char **)&p_ub, &olen, 0)), 
                "Must fail!");
    NDRX_ASSERT_VAL_OUT((QMENOMSG==qc.diagnostic), "Invalid diagnostics code: %ld", qc.diagnostic);
    
    /* verify that all msgs as been set... */
    for (i=0; i<MAX_TEST; i++)
    {
        NDRX_ASSERT_VAL_OUT(rsp[i].num_req==1, "Invalid num_req at %ld: %d", 
                i, rsp[i].num_req);
    }
    
out:

    NDRX_LOG(log_error, "xadmin pt (exit)");
    NDRX_ASSERT_VAL_OUT(EXSUCCEED==system("xadmin pt"), "system() failed");
    if (EXSUCCEED!=tpterm())
    {
        NDRX_LOG(log_error, "tpterm failed with: %s", tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }

    if (NULL!=p_ub)
    {
        tpfree((char *)p_ub);
    }

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
