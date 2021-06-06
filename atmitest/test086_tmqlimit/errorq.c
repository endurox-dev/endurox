/**
 * @brief Error queue test cases
 * @file errorq.c
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
    int num_rsp;
} test_result_t;

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

#define MAX_TEST    100
/**
 * Perform error queue testing (enqueue, service fails)
 * - check error queue of requests
 * - check failurequeue for replies.
 * @param maxmsg
 * @return EXSUCCEED/EXFAIL
 */
expublic int basic_errorq(void)
{
    int ret = EXSUCCEED;
    TPQCTL qc;
    long i;
    long olen;
    char tmpbuf[1024];
    long num;
    UBFH *p_ub = NULL;
    test_result_t rsp[MAX_TEST];
    
    NDRX_LOG(log_error, "case basic_errorq");
    
    NDRX_ASSERT_TP_OUT((NULL!=(p_ub=(UBFH *)tpalloc("UBF", NULL, 1024))), "Failed to alloc");
    
    for (i=0; i<MAX_TEST; i++)
    {
        NDRX_ASSERT_UBF_OUT( (EXSUCCEED==Bchg(p_ub, T_LONG_FLD, 0, (char *)&i, 0L)), 
                "Failed to set test num");
        
        NDRX_ASSERT_UBF_OUT( (EXSUCCEED==Bchg(p_ub, T_STRING_FLD, 0, "REQ", 0L)), 
                "Failed to set test num");
        
        /* enq */
        memset(&qc, 0, sizeof(qc));
        qc.flags|=TPQFAILUREQ;
        NDRX_STRCPY_SAFE(qc.failurequeue, "ERRORRSP");
        
        NDRX_ASSERT_TP_OUT((EXSUCCEED==tpenqueue("MYSPACE", "FAILSVC", &qc, 
                (char *)p_ub, 0, 0L)), "Failed to enq");
    }
    
    /* This shall complete in this time.. */
    sleep(30);
    memset(rsp, 0, sizeof(rsp));
    
    /* read & match results, all stuff shall be in errorreq, errorrsp */
    
    for (i=0; i<MAX_TEST; i++)
    {
        memset(&qc, 0, sizeof(qc));
        
        tpfree((char *)p_ub);
        p_ub = NULL;
        
        NDRX_ASSERT_TP_OUT((EXFAIL==tpdequeue("MYSPACE", "FAILSVC", &qc, (char **)&p_ub, &olen, 0)), 
                "Must not be OK");
        
        /* verify req */
        NDRX_ASSERT_TP_OUT((EXSUCCEED==tpdequeue("MYSPACE", "ERRORREQ", &qc, (char **)&p_ub, &olen, 0)), 
                "Must be OK");
        
        NDRX_ASSERT_UBF_OUT((EXSUCCEED==Bget(p_ub, T_STRING_FLD, 0, tmpbuf, NULL)), 
                "Failed to get data fld");
        NDRX_ASSERT_VAL_OUT(0==strcmp(tmpbuf, "REQ"), "Request not found: %s", tmpbuf);
        NDRX_ASSERT_UBF_OUT((EXSUCCEED==Bget(p_ub, T_LONG_FLD, 0, (char *)&num, NULL)), 
                "Failed to get num");
        rsp[num].num_req++;
        
        
        tpfree((char *)p_ub);
        p_ub = NULL;
        tmpbuf[0]=EXEOS;
        
        /* verify rsp */
        NDRX_ASSERT_TP_OUT((EXSUCCEED==tpdequeue("MYSPACE", "ERRORRSP", &qc, (char **)&p_ub, &olen, 0)), 
                "Must be OK: %ld %s", qc.diagnostic, qc.diagmsg);
        
        NDRX_ASSERT_UBF_OUT((EXSUCCEED==Bget(p_ub, T_STRING_FLD, 0, tmpbuf, NULL)), 
                "Failed to get data fld");
        NDRX_ASSERT_VAL_OUT(0==strcmp(tmpbuf, "RSP"), "Request not found: %s", tmpbuf);
        NDRX_ASSERT_UBF_OUT((EXSUCCEED==Bget(p_ub, T_LONG_FLD, 0, (char *)&num, NULL)), 
                "Failed to get num");
        rsp[num].num_rsp++;
                
    }
    
    /* verify that all msgs as been set... */
    for (i=0; i<MAX_TEST; i++)
    {
        NDRX_ASSERT_VAL_OUT(rsp[i].num_req==1, "Invalid num_req at %ld: %d", 
                i, rsp[i].num_req);
        
        NDRX_ASSERT_VAL_OUT(rsp[i].num_rsp==1, "Invalid num_rsp at %ld: %d", 
                i, rsp[i].num_rsp);
    }
    
out:

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
