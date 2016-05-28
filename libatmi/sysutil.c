/* 
** System utilities
**
** @file sysutil.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, ATR Baltic, SIA. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or ATR Baltic's license for commercial use.
** -----------------------------------------------------------------------------
** GPL license:
** 
** This program is free software; you can redistribute it and/or modify it under
** the terms of the GNU General Public License as published by the Free Software
** Foundation; either version 2 of the License, or (at your option) any later
** version.
**
** This program is distributed in the hope that it will be useful, but WITHOUT ANY
** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
** PARTICULAR PURPOSE. See the GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License along with
** this program; if not, write to the Free Software Foundation, Inc., 59 Temple
** Place, Suite 330, Boston, MA 02111-1307 USA
**
** -----------------------------------------------------------------------------
** A commercial use license is available from ATR Baltic, SIA
** contact@atrbaltic.com
** -----------------------------------------------------------------------------
*/
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <dlfcn.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <ndrxdcmn.h>
#include <userlog.h>
#include <ubf.h>
#include <ubfutil.h>
#include <sys_unix.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

/**
 * Check is server running
 */
public int ex_chk_server(char *procname, short srvid)
{
    int ret = FALSE;
    char test_string3[NDRX_MAX_KEY_SIZE+4];
    char test_string4[64];
    string_list_t * list;
     
    sprintf(test_string3, "-k %s", G_atmi_env.rnd_key);
    sprintf(test_string4, "-i %hd", srvid);
    
    list =  ex_sys_ps_list(ex_sys_get_cur_username(), procname, test_string3, test_string4);
    
    if (NULL!=list)
    {
        NDRX_LOG(log_debug, "process %s -i %hd running ok", procname, srvid);
        ret = TRUE;
    }
    else
    {
        NDRX_LOG(log_debug, "process %s -i %hd not running...", procname, srvid);
    }
    
    
    ex_string_list_free(list);
   
    return ret;
}


/**
 * Check is `ndrxd' daemon running
 */
public int ex_chk_ndrxd(void)
{
    int ret = FALSE;
    char test_string3[NDRX_MAX_KEY_SIZE+4];
    string_list_t * list;
     
    sprintf(test_string3, "-k %s", G_atmi_env.rnd_key);
    
    list =  ex_sys_ps_list(ex_sys_get_cur_username(), "ndrxd", test_string3, "");
    
    if (NULL!=list)
    {
        NDRX_LOG(log_debug, "process `ndrxd' running ok");
        ret = TRUE;
    }
    else
    {
        NDRX_LOG(log_debug, "process `ndrxd' not running...");
    }
    
    
    ex_string_list_free(list);
    
    return ret;
}

/**
 * Kill the system running (the xadmin dies last...)
 */
public int ex_down_sys(void)
{
    int ret = SUCCEED;
    return ret;
}


