/**
 * @brief Test buildserver, buildclient and buildtms - server
 *
 * @file atmisv71_4.c
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
#include <ubf.h>
#include <test.fd.h>
#include <string.h>
#include <unistd.h>
#include "test71.h"

/*---------------------------Externs------------------------------------*/
extern int __write_to_tx_file(char *buf);
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * This will give some echo back...
 */
void ECHOSV (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;
    char testbuf[1024];
    UBFH *p_ub = (UBFH *)p_svc->data;

    NDRX_LOG(log_debug, "%s got call", __func__);
    
    /* Allocate some stuff... */
    if (NULL==(p_ub = (UBFH *)tprealloc((char *)p_ub, 4096)))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to allocate 1024 bytes: %s",
                                        tpstrerror(tperrno));
        tpreturn (TPFAIL, 0L, NULL, 0L, 0L);
    }
    
    if (EXFAIL==Bget(p_ub, T_STRING_FLD, 0, testbuf, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to get T_STRING_FLD: %s", 
                 Bstrerror(Berror));
        ret=EXFAIL;
        goto out;
    }
    /* echo back the field */
    if (EXFAIL==Bchg(p_ub, T_STRING_2_FLD, 0, testbuf, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to get T_STRING_2_FLD: %s", 
                 Bstrerror(Berror));
        ret=EXFAIL;
        goto out;
    }
    
out:
    tpreturn(  ret==EXSUCCEED?TPSUCCESS:TPFAIL,
                0L,
                (char *)p_ub,
                0L,
                0L);
}

/**
 * Another service func..
 */
void ECHO2SV (TPSVCINFO *p_svc)
{
    return ECHOSV(p_svc);
}

/**
 * Another service func..
 */
void SERV (TPSVCINFO *p_svc)
{
    return ECHOSV(p_svc);
}


/* vim: set ts=4 sw=4 et smartindent: */
