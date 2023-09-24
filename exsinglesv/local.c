/**
 * @brief Local request handler (for transactiona lock status)
 *
 * @file local.c
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
#include <string.h>
#include <errno.h>
#include <regex.h>
#include <utlist.h>
#include <unistd.h>
#include <signal.h>

#include <ndebug.h>
#include <atmi.h>
#include <atmi_int.h>
#include <typed_buf.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <ubfutil.h>
#include <cconfig.h>
#include "exsinglesv.h"
#include <Exfields.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
/**
 * Local checks, may call remote server
 * For getting current status & clock time.
 * Allow clock drift for one check interval only.
 * @param p_svc - data & len used only... (to be in future)
 */
void SGLOC (TPSVCINFO *p_svc)
{
    UBFH *p_ub = (UBFH *)p_svc->data;
    int ret=EXSUCCEED;
    char cmd = EXEOS;
    int cd;
    int int_diag = 0;
    char lock_status;
    ndrx_locksm_ctx_t lockst;
    long lookup_grpno;

    tplogprintubf(log_info, "SGLOC request buffer:", p_ub);

    /* check local group, is it locked or not ... */
    if (EXSUCCEED!=Bget(p_ub, EX_PROCGRP_NO, 0, (char *)&lookup_grpno, 0L))
    {
        TP_LOG(log_error, "Failed to get EX_PROCGRP_NO: %s", Bstrerror(Berror));
        ndrx_exsinglesv_set_error_fmt(p_ub, TPEINVAL, "Failed to get EX_PROCGRP_NO: %s",
            Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }

    if (ndrx_G_exsinglesv_conf.procgrp_lp_no!=lookup_grpno)
    {
        TP_LOG(log_error, "Invalid EX_PROCGRP_NO server %ld, requested %ld",
            ndrx_G_exsinglesv_conf.procgrp_lp_no, lookup_grpno);
        ndrx_exsinglesv_set_error_fmt(p_ub, TPEINVAL, 
            "Invalid EX_PROCGRP_NO server %ld, requested %ld",
            ndrx_G_exsinglesv_conf.procgrp_lp_no, lookup_grpno);
        EXFAIL_OUT(ret);
    }

    ret=ndrx_exsinglesv_sg_is_locked(&lockst, EXFALSE);

    if (EXFAIL==ret)
    {
        TP_LOG(log_error, "Failed to check lock status");
        ndrx_exsinglesv_set_error_fmt(p_ub, TPESYSTEM, "Failed to check lock status");
        EXFAIL_OUT(ret);
    }

    lock_status=(char)ret;
    ret=EXSUCCEED;

    if (EXSUCCEED!=Bchg(p_ub, EX_LCKSTATUS, 0, &lock_status, 0))
    {
        TP_LOG(log_error, "Failed to set EX_LCKSTATUS: %s", Bstrerror(Berror));
        ndrx_exsinglesv_set_error_fmt(p_ub, TPESYSTEM, "Failed to set EX_LCKSTATUS: %s",
            Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }

    if (EXSUCCEED!=Bchg(p_ub, EX_SEQUENCE, 0, (char *)&lockst.local.sequence, 0))
    {
        TP_LOG(log_error, "Failed to set EX_SEQUENCE: %s", Bstrerror(Berror));
        ndrx_exsinglesv_set_error_fmt(p_ub, TPESYSTEM, "Failed to set EX_SEQUENCE: %s",
            Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }

    /* approve the buffer */
    ndrx_exsinglesv_set_error_fmt(p_ub, TPMINVAL, "Succeed");

out:
    tplogprintubf(log_info, "SGLOC response buffer:", p_ub);

    tpreturn(  ret==EXSUCCEED?TPSUCCESS:TPFAIL,
                0L,
                (char *)p_ub,
                0L,
                0L);
}

/* vim: set ts=4 sw=4 et smartindent: */
