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
    int ret = EXSUCCEED;

    if (!chk_confirm_clopt("ARE YOU SURE YOU WANT TO FORCIBLY SHUTDOWN (KILL) "
                    "THE APP SESSION?", argc, argv))
    {
        return EXFAIL;
    }
    else
    {
        /* quit automatically, as all resources are being removed! */
        *p_have_next = EXFALSE;
        
        ndrx_down_sys(G_config.qprefix, G_config.qpath, EXFALSE);
        ndrx_down_sys(G_config.qprefix, G_config.qpath, EXTRUE); /* second loop with TRUE... */
    }
    
out:
    return ret;
}
/* vim: set ts=4 sw=4 et smartindent: */
