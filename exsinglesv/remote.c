/**
 * @brief Remote requests
 *
 * @file remote.c
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
#include <assert.h>

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
#include <singlegrp.h>
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
void ndrx_exsingle_remote (void *ptr, int *p_finish_off)
{
    /* Ok we should not handle the commands
     * TPBEGIN...
     */
    int ret=EXSUCCEED;
    ndrx_thread_server_t *thread_data = (ndrx_thread_server_t *)ptr;
    char cmd = EXEOS;
    int cd;
    int int_diag = 0;
    ndrx_sg_shm_t *p_shm, local;
    long procgrp_no;
    long lck_time;
    /**************************************************************************/
    /*                        THREAD CONTEXT RESTORE                          */
    /**************************************************************************/
    UBFH *p_ub = (UBFH *)thread_data->buffer;

    /* restore context. */
    if (EXSUCCEED!=tpsrvsetctxdata(thread_data->context_data, SYS_SRV_THREAD))
    {
        userlog("tmqueue: Failed to set context");
        TP_LOG(log_error, "Failed to set context");
        exit(1);
    }

    ndrx_debug_dump_UBF(log_info, "ndrx_exsingle_local buffer:", p_ub);

    /* just get the group + lock status (our local one)
     * and return entry...
     */
    p_shm = ndrx_sg_get(ndrx_G_exsinglesv_conf.procgrp_lp_no);
    assert(NULL!=p_shm);
    
    /* load local */
    ndrx_sg_load(&local, p_shm);

    procgrp_no=ndrx_G_exsinglesv_conf.procgrp_lp_no;

    lck_time=local.last_refresh;
    if (EXSUCCEED!=Bchg(p_ub, EX_TSTAMP, 0, (char *)&lck_time, 0)
        || EXSUCCEED!=Bchg(p_ub, EX_LCKSTATUS, 0, (char *)&local.is_locked, 0)
        || EXSUCCEED!=Bchg(p_ub, EX_PROCGRP_NO, 0, (char *)procgrp_no, 0))
    {
        TP_LOG(log_error, "Failed to set EX_TSTAMP");
        ndrx_exsinglesv_set_error_fmt(p_ub, TPESYSTEM, "Failed to set UBF values: %s",
            Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }

    /* approve the buffer */
    ndrx_exsinglesv_set_error_fmt(p_ub, TPMINVAL, "Succeed");
    
out:
    /* again read the command code and perform the action:
     * if local -> get local, group, if locked call remote stuff
     *  - including reading the disk...
     * if get current local group, load the status to buffer and reply.
     */
    tpreturn(  ret==EXSUCCEED?TPSUCCESS:TPFAIL,
                0L,
                (char *)p_ub,
                0L,
                0L);
}


/* vim: set ts=4 sw=4 et smartindent: */
