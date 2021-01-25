/**
 * @brief API Part of the LCF
 *
 * @file lcf_api.c
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

/*---------------------------Includes-----------------------------------*/
#include <ndrstandard.h>
#include <lcfint.h>
#include <ndebug.h>
#include <nstd_shm.h>
#include <ndebugcmn.h>
#include <sys_unix.h>
#include <nerror.h>
#include <lcf.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

#define API_ENTRY {_Nunset_error();}

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Register xadmin lcf command
 * Code cannot be internal one
 * @param xcmd filled structure of the command
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_lcf_xadmin_add(ndrx_lcf_reg_xadmin_t *xcmd)
{
    int ret = EXSUCCEED;
    
    API_ENTRY;
    
    if (NULL==xcmd)
    {
        _Nset_error_fmt(NEINVAL, "xcmd cannot be NULL");
        NDRX_LOG_EARLY(log_error, "ERROR: xcmd cannot be NULL");
        EXFAIL_OUT(ret);
    }
    
    if (EXEOS==xcmd->cmdstr[0])
    {
        _Nset_error_fmt(NEINVAL, "xcmd->cmdstr cannot be empty");
        NDRX_LOG_EARLY(log_error, "ERROR: xcmd->cmdstr cannot be NULL");
        EXFAIL_OUT(ret);
    }
    
    if (! (xcmd->command>=NDRX_LCF_CMD_MIN_CUST && xcmd->command<=NDRX_LCF_CMD_MAX_CUST))
    {
        _Nset_error_fmt(NEINVAL, "xcmd->command code out of the range: min %d max %d got %d",
                NDRX_LCF_CMD_MIN_CUST, NDRX_LCF_CMD_MAX_CUST, xcmd->command);
        NDRX_LOG_EARLY(log_error, "xcmd->command code out of the range: min %d max %d got %d",
                NDRX_LCF_CMD_MIN_CUST, NDRX_LCF_CMD_MAX_CUST, xcmd->command);
        EXFAIL_OUT(ret);
    }
    
    ret=ndrx_lcf_xadmin_add(xcmd);
    
out:
    return ret;
    
}

/**
 * Delete command by string
 * @param cmdsr registered command code (only user codes)
 */
expublic int ndrx_lcf_xadmin_del(char *cmdstr)
{
    int ret = EXSUCCEED;
    API_ENTRY;
    
    if (NULL==cmdstr || EXEOS==cmdstr[0])
    {
        _Nset_error_fmt(NEINVAL, "cmdstr cannot be NULL or empty");
        NDRX_LOG_EARLY(log_error, "ERROR: cmdstr cannot be NULL or empty");
        EXFAIL_OUT(ret);
    }
    
    ret = ndrx_lcf_xadmin_delstr_int(cmdstr);
    
out:
    return ret;
}

/**
 * Register callback function
 * @param cfunc callback def with command code
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_lcf_func_add(ndrx_lcf_reg_func_t *cfunc)
{
    /* TODO: */
}

/**
 * Delete callback function by command code
 * @param command
 * @return EXSUCCEED/EXFAIL(was not found)
 */
expublic int ndrx_lcf_func_del(int command)
{
    /* TODO: */
}

/* vim: set ts=4 sw=4 et smartindent: */
