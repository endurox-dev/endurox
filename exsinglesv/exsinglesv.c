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
#include "exsinglesv.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define PROGSECTION "@singlesv"
#define MIN_SGREFRESH_CEOFFICIENT 3 /**< Minimum devider to use faults */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
expublic ndrx_exsinglesv_conf_t ndrx_G_exsinglesv_conf; /**< Configuration */
/*---------------------------Prototypes---------------------------------*/

/**
 * Periodic lock logic:
 *  - perform lock or unlock, alternately at every check cycle
 *  - in case if we are not locked, try to lock master lock (permanent lock)
 *  - in case if we are master locked, at every loop update the shared memory
 *  last_refresh
 *  - in case if maintenance mode is on, do not perform any locking 
 *  (if we are not locked). If locked, then original logic stays.
 *  - ping lock fails to lock or unlock, mark group as non locked, terminate the current
 *  proceess.
 * @return EXSUCCEED/EXFAIL. In case of failure, process will reboot.
 */
exprivate int ndrx_lock_chk(void)
{
    int ret = EXSUCCEED;

    if (!ndrx_G_exsinglesv_conf.locked1)
    {
        /* TODO: if in maintenace mode -> do not do any locking ... */

        /* try to lock file */
        ret=ndrx_exsinglesv_file_lock(NDRX_LOCK_FILE_1, 
            ndrx_G_exsinglesv_conf.lockfile_1);
        
        if (EXSUCCEED==ret)
        {
            /* mark as locked */
            ndrx_G_exsinglesv_conf.locked1=EXTRUE;

            /* TODO: load details to shared memory... */
        }
        else
        {
            goto out;
        }
    }

    /* lock/unlock 2. In case of failure, we shall unlock
     * the group...
     */
    if (!ndrx_G_exsinglesv_conf.locked2)
    {
        /* TODO: Lock */
    }
    else
    {
        /* TODO: unlock */
    }

out:
    return ret;
}

/**
 * Do initialization
 * Have a local MIB & shared MIB
 */
int NDRX_INTEGRA(tpsvrinit)(int argc, char **argv)
{
    int ret=EXSUCCEED;
    ndrx_inicfg_t *cfg = NULL;
    ndrx_inicfg_section_keyval_t *params = NULL;
    ndrx_inicfg_section_keyval_t *el, *elt;
    char *p;
    int ndrx_sgrefresh;

    /* Only singleton server sections needed */
    char *sections[] = {"@singlesv",
                    NULL};

    memset(&ndrx_G_exsinglesv_conf, 0, sizeof(ndrx_G_exsinglesv_conf));

    if (EXSUCCEED!=ndrx_cconfig_load_sections(&cfg, sections))
    {
        NDRX_LOG(log_error, "Failed to load configuration");
        EXFAIL_OUT(ret);
    }

    /* Load params by using ndrx_inicfg_get_subsect() */
    if (EXSUCCEED!=ndrx_cconfig_get_cf(cfg, PROGSECTION, &params))
    {
        NDRX_LOG(log_error, "Failed to load configuration section [%s]", PROGSECTION);
        EXFAIL_OUT(ret);
    }

    /* Iterate over params */
    EXHASH_ITER(hh, params, el, elt)
    {
        NDRX_LOG(log_info, "Param: [%s]=[%s]", el->key, el->val);

        /* read the params such as:
        [@exsinglegrp/<CCTAG>]
        singlegrp=4
        lockfile_1=/some/file.1
        lockfile_2=/some/file2
        exec_on_bootlocked=/some/script/to/exec.sh
        exec_on_locked=/some/script/to/exec.sh
        interval=3
        to the global vars
        */

       if (0==strcmp(el->key, "singlegrp"))
       {
            ndrx_G_exsinglesv_conf.singlegrp = atoi(el->val);
       }
       else if (0==strcmp(el->key, "lockfile_1"))
       {
            NDRX_STRCPY_SAFE(ndrx_G_exsinglesv_conf.lockfile_1, el->val);
       }
       else if (0==strcmp(el->key, "lockfile_2"))
       {
            NDRX_STRCPY_SAFE(ndrx_G_exsinglesv_conf.lockfile_2, el->val);
       }
       else if (0==strcmp(el->key, "exec_on_bootlocked"))
       {
            NDRX_STRCPY_SAFE(ndrx_G_exsinglesv_conf.exec_on_bootlocked, el->val);
       }
       else if (0==strcmp(el->key, "exec_on_locked"))
       {
            NDRX_STRCPY_SAFE(ndrx_G_exsinglesv_conf.exec_on_locked, el->val);
       }
       else if (0==strcmp(el->key, "chkinterval"))
       {
            ndrx_G_exsinglesv_conf.chkinterval = atoi(el->val);
       }
       else
       {
            NDRX_LOG(log_debug, "Unknown parameter [%s]", el->key);
            EXFAIL_OUT(ret);
       }
    }

    /* check key flags... */
    if (0>=ndrx_G_exsinglesv_conf.singlegrp)
    {
        NDRX_LOG(log_error, "Invalid singlegrp value [%d]", 
            ndrx_G_exsinglesv_conf.singlegrp);
        EXFAIL_OUT(ret);
    }

    if (EXEOS==ndrx_G_exsinglesv_conf.lockfile_1[0])
    {
        NDRX_LOG(log_error, "Invalid lockfile_1");
        EXFAIL_OUT(ret);
    }

    if (EXEOS==ndrx_G_exsinglesv_conf.lockfile_2[0])
    {
        NDRX_LOG(log_error, "Invalid lockfile_2");
        EXFAIL_OUT(ret);
    }

    if (0==strcmp(ndrx_G_exsinglesv_conf.lockfile_1, ndrx_G_exsinglesv_conf.lockfile_2))
    {
        NDRX_LOG(log_error, "lockfile_1 and lockfile_2 shall be different");
        EXFAIL_OUT(ret);
    }

    p = getenv(CONF_NDRX_SGREFRESH);
    if (NULL==p || (ndrx_sgrefresh=atoi(p))<=0)
    {
        NDRX_LOG(log_error, "Environment variable [%s] is not set or having invalid value, "
            "expecting >0", CONF_NDRX_SGREFRESH);
        userlog("Environment variable [%s] is not set or having invalid value, "
            "expecting >0", CONF_NDRX_SGREFRESH);
        EXFAIL_OUT(ret);
    }

    if (0>=ndrx_G_exsinglesv_conf.chkinterval)
    {
        /* 
         * get default from NDRX_SGREFRESH
         */
        char *p = getenv(CONF_NDRX_SGREFRESH);

        if (NULL!=p)
        {
            ndrx_G_exsinglesv_conf.chkinterval = atoi(p)/MIN_SGREFRESH_CEOFFICIENT;
        }

        /* generate error */
        if (ndrx_G_exsinglesv_conf.chkinterval<=0)
        {
            NDRX_LOG(log_error, "Invalid value for %s env setting. "
                "To use defaults, it shall be atleast %d", 
                CONF_NDRX_SGREFRESH, MIN_SGREFRESH_CEOFFICIENT);
            userlog("Invalid value for %s env setting. "
                "To use defaults, it shall be atleast %d", 
                CONF_NDRX_SGREFRESH, MIN_SGREFRESH_CEOFFICIENT);
            EXFAIL_OUT(ret);
        }
    }

    /* Dump the configuration to the log file */
    NDRX_LOG(log_info, "singlegrp=%d", ndrx_G_exsinglesv_conf.singlegrp);
    NDRX_LOG(log_info, "lockfile_1=[%s]", ndrx_G_exsinglesv_conf.lockfile_1);
    NDRX_LOG(log_info, "lockfile_2=[%s]", ndrx_G_exsinglesv_conf.lockfile_2);
    NDRX_LOG(log_info, "exec_on_bootlocked=[%s]", ndrx_G_exsinglesv_conf.exec_on_bootlocked);
    NDRX_LOG(log_info, "exec_on_locked=[%s]", ndrx_G_exsinglesv_conf.exec_on_locked);
    NDRX_LOG(log_info, "chkinterval=%d", ndrx_G_exsinglesv_conf.chkinterval);

    if (ndrx_G_exsinglesv_conf.chkinterval*MIN_SGREFRESH_CEOFFICIENT>ndrx_sgrefresh)
    {
        NDRX_LOG(log_warn, "WARNING: `%s' (%d) shall be at least %d times "
                "bigger than 'chkinterval' (%d)"
                CONF_NDRX_SGREFRESH, ndrx_sgrefresh, MIN_SGREFRESH_CEOFFICIENT,
                ndrx_G_exsinglesv_conf.chkinterval);
        userlog("WARNING: `%s' (%d) shall be at least %d times "
                "bigger than 'chkinterval' (%d)"
                CONF_NDRX_SGREFRESH, ndrx_sgrefresh, MIN_SGREFRESH_CEOFFICIENT,
                ndrx_G_exsinglesv_conf.chkinterval);
    }
    
    /* Register timer check.... */
    if (EXSUCCEED==ret &&
            EXSUCCEED!=tpext_addperiodcb(ndrx_G_exsinglesv_conf.chkinterval, 
            ndrx_lock_chk))
    {
        NDRX_LOG(log_error, "tpext_addperiodcb failed: %s",
                        tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }

    /* TODO: report flags and singlegrp to the ndrxd */

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
    /* unlock the group from SHM and unlock / close the lockfiles */
}

/* vim: set ts=4 sw=4 et smartindent: */
