/**
 * @brief Print singleton groups
 *
 * @file cmd_psg.c
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
#include <sys/param.h>

#include <ndrstandard.h>
#include <ndebug.h>

#include <ndrx.h>
#include <ndrxdcmn.h>
#include <atmi_int.h>
#include <gencall.h>
#include <errno.h>
#include <lcf.h>
#include <linenoise.h>

#include "nclopt.h"

#include <exhash.h>
#include <utlist.h>
#include <userlog.h>
#include <singlegrp.h>
#include <lcfint.h>


/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

/**
 * Decode flags
 */
exprivate char *decode_flags(unsigned short flags)
{
    static char buf[10];

    buf[0] = EXEOS;

    if (flags & NDRX_SG_NO_ORDER)
    {
        strcat(buf, "n");
    }

    if (flags & NDRX_SG_IN_USE)
    {
        strcat(buf, "I");
    }

    return buf;
}

/**
 * Print singleton groups
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
expublic int cmd_psg(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret = EXSUCCEED;
    int i;
    struct timespec ts;
    long long refresh;
    int print_all = EXFALSE;

    ncloptmap_t clopt[] =
    {
        {'a', BFLD_INT, (void *)&print_all, 0, 
                            NCLOPT_OPT|NCLOPT_TRUEBOOL, "Print all groups"},
        {0}
    };

    /* parse command line */
    if (nstd_parse_clopt(clopt, EXTRUE,  argc, argv, EXFALSE))
    {
        fprintf(stderr, XADMIN_INVALID_OPTIONS_MSG);
        EXFAIL_OUT(ret);
    }

    /* print header: */
    fprintf(stderr, "SGID LCKD MMON SBOOT CBOOT LPSRVID    LPPID LPPROCNM          REFRESH RSN FLAGS\n");
    fprintf(stderr, "---- ---- ---- ----- ----- ------- -------- ---------------- -------- --- -----\n");

    /* process groups: */
    for (i=1; i<ndrx_G_libnstd_cfg.pgmax; i++)
    {
        ndrx_sg_shm_t *p_shm=ndrx_sg_get(i);
        ndrx_sg_shm_t local;
        ndrx_sg_load(&local, p_shm);

        /* print only used groups by default */
        if (!( local.is_locked || (local.flags & NDRX_SG_IN_USE)) && !print_all )
        {
            continue;
        }

        if (local.is_locked)
        {
            ndrx_realtime_get(&ts);
            refresh = ((long long)ts.tv_sec-(long long)local.last_refresh)*1000;
        }
        else
        {
            refresh=-1;
        }

        fprintf(stdout, "%4d %-4.4s %-4.4s %-5.5s %-5.5s %7d %8d %-16.16s %8.8s %3d %-5.5s\n",
                i, 
                local.is_locked?"Y":"N",
                local.is_mmon?"Y":"N",
                local.is_srv_booted?"Y":"N",
                local.is_clt_booted?"Y":"N",
                local.lockprov_srvid,
                (int)local.lockprov_pid,
                local.lockprov_procname,
                ndrx_decode_msec((long)refresh, 0, 0, 2),
                local.reason,
                decode_flags(local.flags)
                );
    }

out:
    return ret;
}

/**
 * Put in maintenance mode
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
expublic int cmd_mmon(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret=EXSUCCEED;

    if (NULL!=ndrx_G_shmcfg)
    {
        ndrx_G_shmcfg->is_mmon=1;
        __sync_synchronize();
        printf("OK\n");
    }
    else
    {
        ret = EXFAIL;
    }

    return ret;
}

/**
 * Disbale maintenace mode
 */
expublic int cmd_mmoff(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret=EXSUCCEED;

    if (NULL!=ndrx_G_shmcfg)
    {
        ndrx_G_shmcfg->is_mmon=0;
        __sync_synchronize();
        printf("OK\n");
    }
    else
    {
        ret = EXFAIL;
    }

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
