/**
 * @brief Get resource manager name
 *
 * @file getrm.c
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

/*---------------------------Includes-----------------------------------*/

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <atmi.h>
#include <string.h>

#include <ubf.h>
#include <ferror.h>
#include <fieldtable.h>
#include <fdatatype.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <errno.h>
#include "buildtools.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Parse resource manager file,
 * format: res_mgr_logical_name:xa_switch_name:build_libraries_last_names
 * 
 * @param buf parsed buffer from udataobj/RM text file
 * @param p_rmdef resource manager define, shall be set to 0 (memset)
 * @return EXUSCCEED/EXFAIL
 */
exprivate int parse_rm_string(char *buf, ndrx_rm_def_t *p_rmdef)
{
    int ret = EXSUCCEED;
    char *saveptr1 = NULL, *p;
    int i;
    char *args[3] = {NULL, NULL, NULL};
    
    p = strtok_r (buf, ":", &saveptr1);
    
    NDRX_LOG(log_debug, "Parsing define: [%s]", buf);
    
    for (i=0; i<3 && NULL!=p; i++, p = strtok_r (NULL, ":", &saveptr1))
    {
        args[i]=p;
    }
    
    if (i<2)
    {
        _Nset_error_fmt(NEFORMAT, "Invalid resource manager define: [%s]", buf);
        EXFAIL_OUT(ret);
    }
    
    NDRX_STRCPY_SAFE(p_rmdef->rmname, args[0]);
    NDRX_STRCPY_SAFE(p_rmdef->structname, args[1]);
    
    if (args[2])
    {
        NDRX_STRCPY_SAFE(p_rmdef->libnames, args[2]);
    }
    
    NDRX_LOG(log_debug, "Got rmname=[%s] structname=[%s] libnames[%s]", 
            p_rmdef->rmname, p_rmdef->structname, p_rmdef->libnames);
    
out:
    
    return ret;
}

/**
 * Get resource manager and last files for xa_switch_name by rm_name
 * 
 * @param rm_name resource manager name to search for
 * @param p_rmdef Resource manager define struct
 * @return EXFAIL (failed to process), EXTRUE if found, EXSUCCEED if not found but ok
 */
expublic int ndrx_get_rm_name(char *rm_name, ndrx_rm_def_t *p_rmdef)
{
    int ret = EXSUCCEED;
    FILE *fp = NULL;
    char *config;
    char *tmp;
    char ndrx_home_rmfile[NDRX_BPATH_MAX]={EXEOS};
    char buf[NDRX_BPATH_MAX+1];
    
    config=getenv(CONF_NDRX_RMFILE);
    
    if (NULL!=config)
    {
        NDRX_LOG(log_info , "RM file - using [%s]", CONF_NDRX_RMFILE);
    }
    else if (NULL!=(tmp=getenv(CONF_NDRX_HOME)))
    {
        NDRX_LOG(log_info , "RM file try from home [%s]", tmp);
        
        snprintf(ndrx_home_rmfile, sizeof(ndrx_home_rmfile), 
                                "%s/udataobj/RM", tmp);
        config = ndrx_home_rmfile;
    }
    else
    {
        NDRX_LOG(log_error, "Resource Manager file not set (no %s or %s)!", 
                CONF_NDRX_RMFILE, CONF_NDRX_HOME);
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_debug, "Try to open: [%s]", config);

    if (NULL==(fp=NDRX_FOPEN(config, "r")))
    {
        int err = errno;
        NDRX_LOG(log_error, "Failed to open RM file [%s]: %s", config, strerror(err));
        
        _Nset_error_fmt(NENOENT, "Failed to open RM file [%s]: %s", 
                config, strerror(err));
        
        EXFAIL_OUT(ret);
    }
    
     /* process line by line */
    while (NULL!=fgets(buf, sizeof(buf), fp))
    {
        char *stripped;
        
        ndrx_str_rstrip(buf," \t\n\r");
        stripped = ndrx_str_lstrip_ptr(buf," \t\n\r");
        
        if (EXEOS==stripped[0] || '#'==stripped[0])
        {
            /* skip comments */
            continue;
        }
        
        /* try to parse rm switch */
        if (EXSUCCEED!=parse_rm_string(stripped, p_rmdef))
        {
            EXFAIL_OUT(ret);
        }
        
        if (0==strcmp(p_rmdef->rmname, rm_name))
        {
            NDRX_LOG(log_info, "rm_name=[%s] found in file [%s] on line [%s]", 
                    rm_name, config, buf);
            ret=EXTRUE;
            break;
        }
    }
    
out:
    if (NULL!=fp)
    {
        NDRX_FCLOSE(fp);
        fp = NULL;
    }

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
