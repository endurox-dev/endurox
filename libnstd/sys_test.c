/**
 * @brief System testing routines / entry points and tests
 *
 * @file sys_test.c
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
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <userlog.h>
#include <sys_test.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
expublic int ndrx_G_systest_enabled = EXFALSE;    /**< Is system tests enbld ?*/
/*---------------------------Statics------------------------------------*/

exprivate char *M_modebuf = NULL;       /**< Mode buffer of system tests */

/*---------------------------Prototypes---------------------------------*/

/**
 * Prepare system tests keywords
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_systest_init(void)
{
    int ret = EXSUCCEED;
    char *param;
    int len;
    
    param = getenv(CONF_NDRX_TESTMODE);
    
    if (NULL!=param)
    {
        len = strlen(param)+3;/* +,,EOS*/
        
        M_modebuf = NDRX_MALLOC(len); 
        
        if (NULL==M_modebuf)
        {
            userlog("Failed to malloc %d bytes: %s", len, strerror(errno));
            EXFAIL_OUT(ret);
        }
        
        snprintf(M_modebuf, len, ",%s,", param);
        ndrx_str_strip(M_modebuf, " \t");
        ndrx_G_systest_enabled = EXTRUE;
        
        NDRX_LOG_EARLY(log_debug, "Test mode enabled: [%s]", M_modebuf);
    }
    else
    {
        NDRX_LOG_EARLY(log_debug, "sys_test off");
    }
    
out:
    return ret;
}

/**
 * Test is particular test case enabled
 * @param mode test mode string
 * @return EXTRUE/EXFALSE
 */
expublic int ndrx_systest_case(char *mode)
{
    if (M_modebuf && 0!=strstr(M_modebuf, mode))
    {
        return EXTRUE;
    }
    
    return EXFALSE;
}


/* vim: set ts=4 sw=4 et smartindent: */
