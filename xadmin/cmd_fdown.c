/**
 * @brief `down' command implementation
 *
 * @file cmd_fdown.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * AGPL or Mavimax's license for commercial use.
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
#include <nclopt.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Force shutdown all app domain & resource cleanup.
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
expublic int cmd_fdown(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret=EXSUCCEED;
    short confirm = EXFALSE;
    short user_res = EXFALSE;
    
    ncloptmap_t clopt[] =
    {
        {'y', BFLD_SHORT, (void *)&confirm, 0, 
                                NCLOPT_OPT|NCLOPT_TRUEBOOL, "Confirm"},
        {'r', BFLD_SHORT, (void *)&user_res, 0, 
                                NCLOPT_OPT|NCLOPT_TRUEBOOL, "Delete user resources "
                    "(System V queues and semaphores matched by current username)"},
        {0}
    };
    /* parse command line */
    if (nstd_parse_clopt(clopt, EXTRUE,  argc, argv, EXFALSE))
    {
        fprintf(stderr, XADMIN_INVALID_OPTIONS_MSG);
        EXFAIL_OUT(ret);
    }
    
    if (!chk_confirm("ARE YOU SURE YOU WANT TO FORCIBLY SHUTDOWN (KILL) "
                    "THE APP SESSION?", confirm))
    {
        EXFAIL_OUT(ret);
    }
        
    ndrx_down_sys(G_config.qprefix, G_config.qpath, EXFALSE, user_res);
    /* second loop with TRUE... */
    ndrx_down_sys(G_config.qprefix, G_config.qpath, EXTRUE, user_res);
    
    
out:
    return ret;
}

/**
 * Down all user resources
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
expublic int cmd_udown(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret=EXSUCCEED;
    short confirm = EXFALSE;
    
    ncloptmap_t clopt[] =
    {
        {'y', BFLD_SHORT, (void *)&confirm, 0, 
                                NCLOPT_OPT|NCLOPT_TRUEBOOL, "Confirm"},
        {0}
    };
    /* parse command line */
    if (nstd_parse_clopt(clopt, EXTRUE,  argc, argv, EXFALSE))
    {
        fprintf(stderr, XADMIN_INVALID_OPTIONS_MSG);
        EXFAIL_OUT(ret);
    }
    
    if (!chk_confirm("ARE YOU SURE YOU WANT TO FORCIBLY SHUTDOWN (KILL) "
                    "THE APP SESSION?", confirm))
    {
        EXFAIL_OUT(ret);
    }
    
    ndrx_down_userres();
    
out:
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
