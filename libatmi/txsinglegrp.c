/**
 * @brief Sintleton group transaction support
 *
 * @file txsinglegrp.c
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
#include <ndrx_config.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <sys/sem.h>

#include <lcfint.h>
#include <atmi.h>
#include <atmi_shm.h>
#include <ndrstandard.h>
#include <ndebug.h>
#include <ndrxdcmn.h>
#include <userlog.h>
#include <singlegrp.h>
#include <Exfields.h>
#include <ubfutil.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Extended lock verificaiton, if enabled on.
 * This sends query to EXSINGLESV which would
 * contact with other cluster members or shared disk to check
 * time skew.
 * @param grpno group number
 * @return -1 on FAIL, 0 not locked, 1 locked
 */
expublic int ndrx_tpsgislocked(int grpno, long flags)
{
    int ret;
    int grpno_lookup;
    ndrx_sg_shm_t *p_shm, local;
    UBFH *p_ub=NULL;

    NDRX_LOG(log_debug, "Checking if group %d is locked", grpno);

    /*
     * get the flags from the sintleton group...
     */
    if (0==grpno)
    {
        grpno_lookup = G_atmi_env.procgrp_no;
    }

    p_shm = ndrx_sg_get(grpno_lookup);

    if (NULL==p_shm)
    {
        /* set error */
        ndrx_TPset_error_fmt(TPEINVAL,  "Process group not found in shared memory %d",
                grpno_lookup);
        EXFAIL_OUT(ret);
    }

    ndrx_sg_load(&local, p_shm);
    
    /* check is group singleton? */
    if (local.flags & NDRX_SG_SINGLETON)
    {
        if ( (local.flags & NDRX_SG_VERIFY) && (flags & TPACK) )
        {
            long tmp, rsplen;
            char svcnm[MAXTIDENT+1]={EXEOS};
            /* call server for results (local server) */
            snprintf(svcnm, sizeof(svcnm), NDRX_SVC_SGLOC, tpgetnodeid(), grpno_lookup);

            p_ub = (UBFH *)tpalloc("UBF", NULL, 1024);

            if (NULL==p_ub)
            {
                NDRX_LOG(log_error, "Buffer alloc fail");
                EXFAIL_OUT(ret);
            }

            tmp = grpno_lookup;
            if (EXSUCCEED!=Bchg(p_ub, EX_COMMAND, 0, NDRX_SGCMD_VERIFY, 0L)
                || EXSUCCEED!=Bchg(p_ub, EX_PROCGRP_NO, 0, (char *)&tmp, 0L))
            {
                NDRX_LOG(log_error, "Failed to setup request buffer: %s", Bstrerror(Berror));
                ndrx_TPset_error_fmt(TPESYSTEM,  "Failed to setup request buffer: %s", Bstrerror(Berror));
                EXFAIL_OUT(ret);
            }

            ndrx_debug_dump_UBF(log_info, "Request buffer:", p_ub);
            NDRX_LOG(log_debug, "Calling [%s]", svcnm);

            ret = tpcall(svcnm, (char*)p_ub, 0L, (char **)&p_ub, &rsplen, TPNOTRAN);

            if (EXSUCCEED!=ret)
            {
                NDRX_LOG(log_error, "Failed to call [%s]: %s", 
                    svcnm, tpstrerror(tperrno));

                /* if service failed for some reason...  */
                if (TPESVCFAIL==tperrno)
                {
                    char errmsg[MAX_TP_ERROR_LEN+1];
                    BFLDLEN errmsglen;
                    long err;
                    ndrx_debug_dump_UBF(log_error, "reply buffer:", p_ub);
                    /* try to read the error message/code */
                    if (EXSUCCEED==Bget(p_ub, EX_TPERRNO, 0, (char *)&err, 0L)
                        && EXSUCCEED==Bget(p_ub, EX_TPSTRERROR, 0, (char *)errmsg, &errmsglen))
                    {
                        /* set returned error code */
                        ndrx_TPunset_error();
                        /* set error */
                        ndrx_TPset_error_msg(err, errmsg);
                    }
                }
                /* done with error... */
                EXFAIL_OUT(ret);
            }
            else
            {
                short lock_stat;

                ndrx_debug_dump_UBF(log_info, "reply buffer:", p_ub);

                /* read the locked flag... */
                if (EXSUCCEED!=Bget(p_ub, EX_LCKSTATUS, 0, (char *)&lock_stat, 0L))
                {
                    NDRX_LOG(log_error, "Missing EX_LCKSTATUS: %s", Bstrerror(Berror));
                    ndrx_TPset_error_fmt(TPESYSTEM,  "Missing EX_LCKSTATUS: %s", Bstrerror(Berror));
                    EXFAIL_OUT(ret);
                }

                ret=lock_stat;
            }
        }
        else
        {
            /* read directly from shm */
            ret=ndrx_sg_is_locked_int(grpno_lookup, p_shm, NULL, 0);

            if (EXFAIL==ret)
            {
                NDRX_LOG(log_error, "Local group %d check failed", grpno_lookup);
            }
        }
    }
    else
    {
        /* not singleton group */
        ndrx_TPset_error_fmt(TPEPROTO,  "Process group %d is not singleton",
                grpno_lookup);
        EXFAIL_OUT(ret);
    }

out:
    if (NULL!=p_ub)
    {
        tpfree((char *)p_ub);
    }
    NDRX_LOG(log_info, "lock check grpno=%d ret = %d", grpno_lookup, ret);
    return ret;
}


/* vim: set ts=4 sw=4 et smartindent: */
