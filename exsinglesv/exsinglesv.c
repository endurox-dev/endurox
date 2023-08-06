/**
 * @brief Singleton group lock provider
 *
 * @file exsinglesv.c
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
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Periodic lock checks
 * In case if we are locked, perform ping lock
 * If we are not locked try to lock master lock
 * @return EXSUCCEED/EXFAIL. In case of failure, process will reboot.
 */
exprivate int ndrx_lock_chk(void)
{
    return EXSUCCEED;
}

/**
 * Do initialization
 * Have a local MIB & shared MIB
 */
int NDRX_INTEGRA(tpsvrinit)(int argc, char **argv)
{
    int ret=EXSUCCEED;
    ndrx_inicfg_t *cfg = NULL;

    /* Only singleton server sections needed */
    char *sections[] = {"@singlesv",
                    NULL};

    if (EXSUCCEED!=ndrx_cconfig_load_sections(&cfg, sections))
    {
        NDRX_LOG(log_error, "Failed to load configuration");
        EXFAIL_OUT(ret);
    }

    /* TODO: read following paramsters:
     * - singlegrp
     * - lockfile_1
     * - lockfile_2
     * - exec_on_bootlocked
     * - exec_on_locked
     * - chkinterval
     */
    
    /* Register timer check.... 
    if (EXSUCCEED==ret &&
            EXSUCCEED!=tpext_addperiodcb(ndrx_G_adm_config.scantime, 
            ndrx_adm_curs_housekeep))
    {
        
        NDRX_LOG(log_error, "tpext_addperiodcb failed: %s",
                        tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    */

   /* perform first check
    * so that on boot, the first locked node would boot without any interruptions */
   if (EXSUCCEED!=ndrx_lock_chk())
   {
        NDRX_LOG(log_error, "Failed to perform lock check");
        EXFAIL_OUT(ret);
   }

out:

    if (NULL!=cfg)
    {
        ndrx_inicfg_free(cfg);
    }

    return ret;
}

void NDRX_INTEGRA(tpsvrdone)(void)
{
    /* just for build... */
}

/* vim: set ts=4 sw=4 et smartindent: */
