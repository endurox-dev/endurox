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
    long allow_flags;
    API_ENTRY;
    
    if (NULL==xcmd)
    {
        _Nset_error_fmt(NEINVAL, "xcmd cannot be NULL");
        NDRX_LOG_EARLY(log_error, "ERROR: xcmd cannot be NULL");
        EXFAIL_OUT(ret);
    }
    
    if (xcmd->version < NDRX_LCF_XCMD_VERSION)
    {
        _Nset_error_fmt(NEVERSION, "Invalid argument - version minimum: %d got: %d",
                NDRX_LCF_XCMD_VERSION, xcmd->version);
        NDRX_LOG_EARLY(log_error, "Invalid argument - version minimum: %d got: %d",
                NDRX_LCF_XCMD_VERSION, xcmd->version);
        EXFAIL_OUT(ret);
    }
    
    if (EXEOS==xcmd->cmdstr[0])
    {
        _Nset_error_fmt(NEINVAL, "xcmd->cmdstr cannot be empty");
        NDRX_LOG_EARLY(log_error, "ERROR: xcmd->cmdstr cannot be NULL");
        EXFAIL_OUT(ret);
    }
    
    if (EXTRUE!=ndrx_str_valid_cid(xcmd->cmdstr, NDRX_LCF_ADMINCMD_MAX))
    {
        _Nset_error_fmt(NEINVAL, "xcmd->cmdstr has invalid characters or empty val");
        NDRX_LOG_EARLY(log_error, "xcmd->cmdstr has invalid characters or empty val");
        EXFAIL_OUT(ret);
    }
    
    /* check flags */
    allow_flags = NDRX_LCF_FLAG_ALL
            |NDRX_LCF_FLAG_ARGA
            |NDRX_LCF_FLAG_ARGB
            |NDRX_LCF_FLAG_DOSTARTUP
            |NDRX_LCF_FLAG_DOSTARTUPEXP;
    
    allow_flags = ~allow_flags;
    
    if (xcmd->dltflags & allow_flags)
    {
        _Nset_error_fmt(NEINVAL, "Invalid flags given: 0x%lx", xcmd->dltflags & allow_flags);
        NDRX_LOG_EARLY(log_error, "Invalid flags given: 0x%lx", xcmd->dltflags & allow_flags);
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
    
    ret=ndrx_lcf_xadmin_add_int(xcmd);
    
out:
    return ret;
    
}


/**
 * Register callback function. Uses may only add the functions.
 * No chance to delete to avoid any issues with threads performing the lookups
 * @param cfunc callback def with command code
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_lcf_func_add(ndrx_lcf_reg_func_t *cfunc)
{
    int ret = EXSUCCEED;
    API_ENTRY;

    if (NULL==cfunc)
    {
        _Nset_error_fmt(NEINVAL, "cfunc cannot be NULL");
        NDRX_LOG_EARLY(log_error, "cfunc cannot be NULL");
        EXFAIL_OUT(ret);
    }
    
    if (cfunc->version < NDRX_LCF_CCMD_VERSION)
    {
        _Nset_error_fmt(NEVERSION, "Invalid argument version minimum: %d got: %d",
                NDRX_LCF_CCMD_VERSION, cfunc->version);
        NDRX_LOG_EARLY(log_error, "Invalid argument version minimum: %d got: %d",
                NDRX_LCF_CCMD_VERSION, cfunc->version);
        EXFAIL_OUT(ret);
    }
    
    if (NULL==cfunc->pf_callback)
    {
        _Nset_error_fmt(NEINVAL, "pf_callback cannot be NULL");
        NDRX_LOG_EARLY(log_error, "pf_callback cannot be NULL");
        EXFAIL_OUT(ret);
    }
    
    if (NULL==cfunc->cmdstr)
    {
        _Nset_error_fmt(NEINVAL, "cmdstr cannot be NULL");
        NDRX_LOG_EARLY(log_error, "cmdstr cannot be NULL");
        EXFAIL_OUT(ret);
    }
    
    if (EXTRUE!=ndrx_str_valid_cid(cfunc->cmdstr, NDRX_LCF_ADMINCMD_MAX))
    {
        _Nset_error_fmt(NEINVAL, "xcmd->cmdstr has invalid characters or empty val");
        NDRX_LOG_EARLY(log_error, "xcmd->cmdstr has invalid characters or empty val");
        EXFAIL_OUT(ret);
    }
        
    ret = ndrx_lcf_func_add_int(cfunc);
    
out:
    return ret;
    
}

/**
 * Public API function for publishing LCF command
 * @param slot slot number where to publish. Note
 * @param cmd
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_lcf_publish(int slot, ndrx_lcf_command_t *cmd)
{
    int ret = EXSUCCEED;
    API_ENTRY;
    /* pull in the debug init, as LCF must be open */
    NDRX_DBG_INIT_ENTRY;
    
    if (cmd->version < NDRX_LCF_LCMD_VERSION)
    {
        _Nset_error_fmt(NEVERSION, "Invalid argument version minimum: %d got: %d",
                NDRX_LCF_CCMD_VERSION, cmd->version);
        NDRX_LOG_EARLY(log_error, "Invalid argument version minimum: %d got: %d",
                NDRX_LCF_CCMD_VERSION, cmd->version);
        EXFAIL_OUT(ret);
    }
    
    if (EXEOS==cmd->cmdstr[0])
    {
        _Nset_error_msg(NEINVAL, "cmd->cmdstr is empty");
        NDRX_LOG_EARLY(log_error, "cmd->cmdstr is empty");
        EXFAIL_OUT(ret);
    }
    
    /* check that buffers are OK, maybe this process will die
     * but at least others will be safe and sound
     */
    if (strlen(cmd->cmdstr) > NDRX_LCF_ADMINCMD_MAX)
    {
        _Nset_error_msg(NEINVAL, "cmd->cmdstr invalid length");
        NDRX_LOG_EARLY(log_error, "cmd->cmdstr invalid length");
        EXFAIL_OUT(ret);
    }
    
    if (strlen(cmd->arg_a) > PATH_MAX)
    {
        _Nset_error_msg(NEINVAL, "cmd->arg_a invalid length");
        NDRX_LOG_EARLY(log_error, "cmd->arg_a invalid length");
        EXFAIL_OUT(ret);
    }
    
    if (strlen(cmd->arg_b) > NAME_MAX)
    {
        _Nset_error_msg(NEINVAL, "cmd->arg_b invalid length");
        NDRX_LOG_EARLY(log_error, "cmd->arg_b invalid length");
        EXFAIL_OUT(ret);
    }
    
    if (strlen(cmd->procid) > NAME_MAX)
    {
        _Nset_error_msg(NEINVAL, "cmd->procid invalid length");
        NDRX_LOG_EARLY(log_error, "cmd->procid invalid length");
        EXFAIL_OUT(ret);
    }
    
    if (strlen(cmd->fbackmsg) > NDRX_LCF_FEEDBACK_BUF-1)
    {
        _Nset_error_msg(NEINVAL, "cmd->fbackmsg invalid length");
        NDRX_LOG_EARLY(log_error, "cmd->fbackmsg invalid length");
        EXFAIL_OUT(ret);
    }
    
    ret = ndrx_lcf_publish_int(slot, cmd);
    
out:
    return ret;
}


/* vim: set ts=4 sw=4 et smartindent: */
