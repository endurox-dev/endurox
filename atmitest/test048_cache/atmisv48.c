/* 
** TP Cache tests - server
**
** @file atmisv48.c
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

#include <stdio.h>
#include <stdlib.h>
#include <ndebug.h>
#include <atmi.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <test.fd.h>
#include <string.h>
#include <unistd.h>
#include <ubfutil.h>
#include "test48.h"

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
void TESTSV01 (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;
    char testbuf[1024];
    UBFH *p_ub = (UBFH *)p_svc->data;
    char tstamp[TSTAMP_BUFSZ];
    NDRX_LOG(log_debug, "%s got call", __func__);

    
    if (NULL==(p_ub =(UBFH *)tprealloc((char *)p_ub, Bused(p_ub)+1024)))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to reallocate incoming buffer: %s",
                tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    /* Just print the buffer */
    Bprint(p_ub);
    
    if (EXFAIL==Bget(p_ub, T_STRING_FLD, 0, testbuf, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to get T_STRING_FLD: %s", 
                 Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    if (0!=strcmp(testbuf, VALUE_EXPECTED))
    {
        NDRX_LOG(log_error, "TESTERROR: Expected: [%s] got [%s]",
            VALUE_EXPECTED, testbuf);
        ret=EXFAIL;
        goto out;
    }
    
    if (EXSUCCEED!=Bchg(p_ub, T_STRING_FLD, 1, VALUE_EXPECTED_RET, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to set T_STRING_FLD[1]: %s", 
                 Bstrerror(Berror));
        ret=EXFAIL;
        goto out;
    }
    
    /* get stamp */
    
    test048_stamp_get(tstamp, sizeof(tstamp));
    
    if (EXSUCCEED!=Bchg(p_ub, T_STRING_5_FLD, 0, tstamp, 0))
    {
        NDRX_LOG(log_error, "Failed to set T_STRING_5_FLD: %s", Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
out:
    tpreturn(  ret==EXSUCCEED?TPSUCCESS:TPFAIL,
                0L,
                (char *)p_ub,
                0L,
                0L);
}

/**
 * Standard service entry
 */
void BENCH48 (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;
    
    /* do nothing... */
out:
    tpreturn(  TPSUCCESS,
                0L,
                p_svc->data,
                0L,
                0L);
}

/**
 * Standard service entry
 */
void OKSVC (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;
    UBFH *p_ub = (UBFH *)p_svc->data;
    
    long t, tusec;
    
    NDRX_LOG(log_debug, "%s got call", __func__);
 
    if (NULL==(p_ub =(UBFH *)tprealloc((char *)p_ub, Bused(p_ub)+1024)))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to reallocate incoming buffer: %s",
                tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    /* Just print the buffer */
    ndrx_debug_dump_UBF(log_debug, "Received buffer", p_ub);
    
    ndrx_utc_tstamp2(&t, &tusec);
    
    if (EXSUCCEED!=Bchg(p_ub, T_LONG_2_FLD, 0, (char *)&t, 0L) ||
            EXSUCCEED!=Bchg(p_ub, T_LONG_2_FLD, 1, (char *)&tusec, 0L))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to set T_LONG_2_FLD fields!",
            Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    
    ndrx_debug_dump_UBF(log_debug, "Response buffer", p_ub);
    
out:
    tpreturn(  ret==EXSUCCEED?TPSUCCESS:TPFAIL,
                0L,
                (char *)p_ub,
                0L,
                0L);
}

/**
 * Standard service entry, fail
 */
void FAILSVC (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;
    int ret_gen_succ=EXFALSE;
    UBFH *p_ub = (UBFH *)p_svc->data;
    long l;
    long t, tusec;
    
    NDRX_LOG(log_debug, "%s got call", __func__);
 
    if (NULL==(p_ub =(UBFH *)tprealloc((char *)p_ub, Bused(p_ub)+1024)))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to reallocate incoming buffer: %s",
                tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    /* Just print the buffer */
    ndrx_debug_dump_UBF(log_debug, "Received buffer", p_ub);
    
    ndrx_utc_tstamp2(&t, &tusec);
    
    if (EXSUCCEED!=Bchg(p_ub, T_LONG_2_FLD, 0, (char *)&t, 0L) ||
            EXSUCCEED!=Bchg(p_ub, T_LONG_2_FLD, 1, (char *)&tusec, 0L))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to set T_LONG_2_FLD fields!",
            Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    /* make succeed if value 1 */
            
    if (EXSUCCEED==Bget(p_ub, T_LONG_3_FLD, 0, (char *)&l, 0L))
    {
        if (1==l)
        {
            ret_gen_succ=EXTRUE;
        }
    }
    
    ndrx_debug_dump_UBF(log_debug, "Response buffer", p_ub);
    
out:
        
    tpreturn(  ret_gen_succ?TPSUCCESS:TPFAIL,
                0L,
                (char *)p_ub,
                0L,
                0L);
}

/*
 * Do initialization
 */
int NDRX_INTEGRA(tpsvrinit)(int argc, char **argv)
{
    int ret = EXSUCCEED;
    NDRX_LOG(log_debug, "tpsvrinit called");

    if (EXSUCCEED!=tpadvertise("TESTSV01", TESTSV01))
    {
        NDRX_LOG(log_error, "Failed to initialize TESTSV01!");
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=tpadvertise("OKSVC", OKSVC))
    {
        NDRX_LOG(log_error, "Failed to initialize OKSVC!");
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=tpadvertise("FAILSVC", FAILSVC))
    {
        NDRX_LOG(log_error, "Failed to initialize FAILSVC!");
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=tpadvertise("BENCH48", BENCH48))
    {
        NDRX_LOG(log_error, "Failed to initialize BENCH48!");
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
}

