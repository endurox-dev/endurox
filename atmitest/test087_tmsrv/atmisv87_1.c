/**
 * @brief TMSRV State Driving verification - server
 *
 * @file atmisv87_1.c
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
#include <stdio.h>
#include <stdlib.h>
#include <ndebug.h>
#include <atmi.h>
#include <ndrstandard.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Standard service entry
 */
void TESTSV1 (TPSVCINFO *p_svc)
{
    tpforward("TESTSV2", NULL, 0, 0);
}

/**
 * Standard service entry, Error server.
 */
void TESTSVE1 (TPSVCINFO *p_svc)
{
    char *odata=NULL;
    long olen;
    
    /* shall generate error */
    tpcall("TESTSVE2", NULL, 0, &odata, &olen, 0);
    
    /* shall keep the error */
    tpforward("TESTSV2", NULL, 0, 0);
}

/**
 * Standard service entry, Error server, Return
 */
void TESTSVE1_RET (TPSVCINFO *p_svc)
{
    char *odata=NULL;
    long olen;
    
    /* shall generate error */
    tpcall("TESTSVE2", NULL, 0, &odata, &olen, 0);
    
    tpreturn(TPSUCCESS, 0, NULL, 0, 0);
}


/**
 * Participant tests of the transaction APIs.
 */
void TEST1_PART (TPSVCINFO *p_svc)
{
    int ret = EXSUCCEED;
    
    if (EXSUCCEED==tpabort(0))
    {
        NDRX_LOG(log_error, "TESTERROR: Must not abort!");
        EXFAIL_OUT(ret);
    }
    
    if (TPEPROTO!=tperrno)
    {
        NDRX_LOG(log_error, "TESTERROR: Must be TPEPROTO, got %d!", tperrno);
        EXFAIL_OUT(ret);
    }
    
    if (!tpgetlev())
    {
        NDRX_LOG(log_error, "TESTERROR: Process shall stay in transaction, but is not");
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED==tpcommit(0))
    {
        NDRX_LOG(log_error, "TESTERROR: Must not commit!");
        EXFAIL_OUT(ret);
    }
    
    if (TPEPROTO!=tperrno)
    {
        NDRX_LOG(log_error, "TESTERROR: Must be TPEPROTO, got %d!", tperrno);
        EXFAIL_OUT(ret);
    }
    
    if (!tpgetlev())
    {
        NDRX_LOG(log_error, "TESTERROR: Process shall stay in transaction, but is not");
        EXFAIL_OUT(ret);
    }
    
    
out:
    
    if (EXSUCCEED==ret)
    {
        tpreturn(TPSUCCESS, 0, NULL, 0, 0);
    }
    else
    {
        tpreturn(TPFAIL, 0, NULL, 0, 0); 
    }
}


/**
 * Our transaction still attached
 */
void TESTSVE1_TRANRET (TPSVCINFO *p_svc)
{
    int ret = EXSUCCEED;
    
    if (EXSUCCEED!=tpbegin(40, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to begin transaction: %s",
                tpstrerror(tperrno));
        userlog("TESTERROR: Failed to begin transaction: %s",
                tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }    
out:
    if (EXSUCCEED==ret)
    {
        tpreturn(TPSUCCESS, 0, NULL, 0, 0);
    }
    else
    {
        tpreturn(TPFAIL, 0, NULL, 0, 0);
    }
}

/**
 * Open tran, forward
 */
void TESTSVE1_TRANFWD (TPSVCINFO *p_svc)
{
    if (EXSUCCEED!=tpbegin(40, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to begin transaction: %s",
                tpstrerror(tperrno));
        userlog("TESTERROR: Failed to begin transaction: %s",
                tpstrerror(tperrno));
    }    
    
    tpforward("TESTSV2", NULL, 0, 0);
}

/**
 * Standard service entry, Error server, Return
 */
void TESTSVE1_NORET (TPSVCINFO *p_svc)
{
    char *odata=NULL;
    long olen;
}


int tpsvrinit (int argc, char **argv)
{
    return tpopen();
}

void tpsvrdone(void)
{
    tpclose();
}


/* vim: set ts=4 sw=4 et smartindent: */
