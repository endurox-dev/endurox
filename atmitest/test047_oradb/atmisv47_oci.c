/**
 * @brief Oracle OCI Sample database server.
 *  basic sample to demonstrate OCI operations
 *
 * @file atmisv47_oci.c
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

#include <stdio.h>
#include <stdlib.h>

#include <ndebug.h>
#include <atmi.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <ubfutil.h>
#include <test.fd.h>
#include <string.h>
#include "test47.h"
#include <oci.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
exprivate OCIEnv        *M_oci_env;
exprivate OCISvcCtx     *M_oci_svc_ctx;

/*---------------------------Prototypes---------------------------------*/

/**
 *  Perform insert using the XA transaction
 */
exprivate int do_insert(char *accnum, long balance)
{
    int           ret;
    OCIStmt       *stmt_h=NULL;
    OCIError      *err_h=NULL;
    char stmt[1024];
    
    snprintf(stmt, sizeof(stmt), "INSERT INTO accounts(accnum, balance) VALUES('%s', %ld)",
        accnum, balance);

    if (OCI_SUCCESS != OCIHandleAlloc( (dvoid *)M_oci_env, (dvoid **)&err_h,
                                     OCI_HTYPE_ERROR, (size_t)0, (dvoid **)0))
    {
        NDRX_LOG(log_error, "Unable to allocate error handle");
        EXFAIL_OUT(ret);
    }

    if (OCI_SUCCESS != OCIHandleAlloc( (dvoid *)M_oci_env, (dvoid **)&stmt_h,
                             OCI_HTYPE_STMT, (size_t)0, (dvoid **)0))
    {
        NDRX_LOG(log_error, "Unable to allocate statement handle");
        EXFAIL_OUT(ret);
    }
    
    if (OCI_SUCCESS != OCIStmtPrepare(stmt_h, err_h, stmt,
                                        (ub4) strlen(stmt),
                                        (ub4) OCI_NTV_SYNTAX,
                                        (ub4) OCI_DEFAULT))
    {
        NDRX_LOG(log_error, "Failed to prepare stmt");
        EXFAIL_OUT(ret);
    }

    ret = OCIStmtExecute(M_oci_svc_ctx, stmt_h, err_h,
                            (ub4)1, (ub4)0,
                            (CONST OCISnapshot *)NULL,
                            (OCISnapshot *)NULL, OCI_DEFAULT);

    if (OCI_SUCCESS != ret && OCI_SUCCESS_WITH_INFO != ret)
    {
        fprintf(stderr, "Failed to execute ret: %d", ret);
        EXFAIL_OUT(ret);
    }

    ret=EXSUCCEED;
    
out:
    if (NULL!=stmt_h)
    {
        OCIHandleFree((dvoid *)stmt_h, (ub4)OCI_HTYPE_STMT);
    }

    if (NULL!=err_h)
    {
        OCIHandleFree((dvoid *)err_h, (ub4)OCI_HTYPE_ERROR);
    }

    return ret;
}

/**
 * This service will make an account
 */
void ACCOPEN_OCI (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;
    UBFH *p_ub = (UBFH *)p_svc->data;
    int tran_started=EXFALSE;
    long h_balance;
    char h_accnum[1024];
    TPTRANID trn;
    
    NDRX_LOG(log_debug, "%s got call", __func__);

    if (!tpgetlev())
    {
        /* start a transaction */
        if (EXSUCCEED!=tpbegin(60, 0))
        {
            NDRX_LOG(log_error, "Failed to start transaction: %s", 
                    tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
        tran_started=EXTRUE;
    }
    
    /* Just print the buffer */
    ndrx_debug_dump_UBF(log_debug, "ACCOPEN_OCI got call", p_ub);
    
    if (EXSUCCEED!=tpsuspend (&trn, 0))
    {
        NDRX_LOG(log_error, "failed to suspend: %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }

    if (EXSUCCEED!=tpresume (&trn, 0))
    {
        NDRX_LOG(log_error, "failed to resume: %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }

    if (EXFAIL==Bget(p_ub, T_STRING_FLD, 0, h_accnum, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to get T_STRING_FLD: %s", 
                 Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    if (EXFAIL==Bget(p_ub, T_LONG_FLD, 0, (char *)&h_balance, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to get h_balance: %s", 
                 Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_info, "Opening account: [%s] balance: [%ld]", 
            h_accnum, h_balance);
    
    if (EXSUCCEED!=do_insert(h_accnum, h_balance))
    {
        EXFAIL_OUT(ret);
    }

    NDRX_LOG(log_debug, "OK");
    
out:
        
    if (tran_started)
    {
        if (EXSUCCEED==ret)
        {
            if (EXSUCCEED!=tpcommit(0))
            {
                NDRX_LOG(log_error, "Failed to commit: %s", tpstrerror(tperrno));
                ret=EXFAIL;
            }
        }
        
        if (EXFAIL==ret)
        {
            if (EXSUCCEED!=tpabort(0))
            {
                NDRX_LOG(log_error, "Failed to abort: %s", tpstrerror(tperrno));
            }
        }
    }

    tpreturn(  ret==EXSUCCEED?TPSUCCESS:TPFAIL,
                0L,
                (char *)p_ub,
                0L,
                0L);
}

/**
 * Do initialization
 */
int NDRX_INTEGRA(tpsvrinit)(int argc, char **argv)
{
    int ret = EXSUCCEED;
    char *p;
    NDRX_LOG(log_debug, "tpsvrinit called");
    
    if (EXSUCCEED!=tpopen())
    {
        NDRX_LOG(log_error, "Failed to open DB!");
        EXFAIL_OUT(ret);
    }

    /* Get the XA env: */
    if (NULL == (M_oci_env = xaoEnv(NULL)))
    {
        NDRX_LOG(log_error, "xaoEnv returned a NULL pointer\n");
        EXFAIL_OUT(ret);
    }

    if (NULL == (M_oci_svc_ctx = xaoSvcCtx(NULL)))
    {
        NDRX_LOG(log_error, "xaoSvcCtx returned a NULL pointer\n");
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=tpadvertise("ACCOPEN_OCI", ACCOPEN_OCI))
    {
        NDRX_LOG(log_error, "Failed to initialize ACCOPEN_OCI!");
        EXFAIL_OUT(ret);
    }
    
    
out:
    return ret;
}

/**
 * Do de-initialization
 */
void NDRX_INTEGRA(tpsvrdone)(void)
{
    NDRX_LOG(log_debug, "tpsvrdone called");
    
    tpclose();
    
}

/* vim: set ts=4 sw=4 et smartindent: */
