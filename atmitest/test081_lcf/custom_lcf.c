/**
 * @brief Custom LCF Command
 *
 * @file custom_lcf.c
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <nstdutil.h>
#include <expluginbase.h>
#include <sys_unix.h>
#include <test.fd.h>
#include <lcf.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Depending on args will provided OK or NOK or Feedback
 */
exprivate int custom_command(ndrx_lcf_command_t *cmd, long *p_flags)
{
    NDRX_LOG(log_error, "**************** %s *********************", cmd->arg_a);
    
    /* set feedback */
    cmd->fbackcode=987;
    NDRX_STRCPY_SAFE(cmd->fbackmsg, cmd->arg_b);
    *p_flags|=(NDRX_LCF_FLAG_FBACK_CODE|NDRX_LCF_FLAG_FBACK_MSG);
    
    if (0==strcmp(cmd->arg_b, "FAIL"))
    {
        cmd->fbackcode=741;
        return -1;
    }
    
    return 0;
}

/**
 * Initialize the plugin
 * @param provider_name plugin name/provider string
 * @param provider_name_bufsz provider string buffer size
 * @return FAIL (in case of error) or OR'ed function flags
 */
expublic long ndrx_plugin_init(char *provider_name, int provider_name_bufsz)
{
    int ret = EXSUCCEED;
    ndrx_lcf_reg_func_t cfunc;
    ndrx_lcf_reg_xadmin_t xfunc;
    
    memset(&cfunc, 0, sizeof(cfunc));
    
    NDRX_STRCPY_SAFE(cfunc.cmdstr, "customz");
    cfunc.command=1001;
    cfunc.version=NDRX_LCF_CCMD_VERSION;
    cfunc.pf_callback=custom_command;
    
    if (EXSUCCEED!=ndrx_lcf_func_add(&cfunc))
    {
        NDRX_LOG_EARLY(log_error, "TESTERROR: Failed to add func: %s", Nstrerror(Nerror));
        EXFAIL_OUT(ret);
    }
    
    memset(&xfunc, 0, sizeof(xfunc));
    NDRX_STRCPY_SAFE(xfunc.cmdstr, "customz");
    xfunc.command=1001;
    xfunc.version = cfunc.version=NDRX_LCF_XCMD_VERSION;
    NDRX_STRCPY_SAFE(xfunc.helpstr, "Test command");
    xfunc.dltflags=(NDRX_LCF_FLAG_DOSTARTUPEXP | NDRX_LCF_FLAG_ARGA | NDRX_LCF_FLAG_ARGB);
    xfunc.dfltslot=3;
    if (EXSUCCEED!=ndrx_lcf_xadmin_add(&xfunc))
    {
        NDRX_LOG_EARLY(log_error, "TESTERROR: Failed to add func: %s", Nstrerror(Nerror));
        EXFAIL_OUT(ret);
    }
    
out:
    /* No functions provided */
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
