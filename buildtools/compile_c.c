/**
 * @brief Build C code for server, client or TMS
 *   
 * @file compile_c.c
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

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <atmi.h>

#include <ubf.h>
#include <ferror.h>
#include <fieldtable.h>
#include <fdatatype.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <errno.h>
#include "buildtools.h"
#include <typed_view.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

/**
 * Invoke C compiler.
 * @param buildmode this is build mode. Either server,client or tms, see COMPILE_ consts
 * @param verbose if set, print build command
 * @param cfile generate file name to build (where the main lives)
 * @param 
 */
expublic int ndrx_compile_c(int buildmode, int verbose, char *cfile, char *ofile, 
        char *firstfiles, char *lastfiles, ndrx_rm_def_t *p_rmdef)
{
    int ret=EXSUCCEED;
    char *env_cc, *env_cflags, *env_ndrx_home;
    char build_cmd[PATH_MAX+1] = {EXEOS};
    char default_compiler[PATH_MAX+1] = "cc";
    char *build_final_cmd;
    char ndrx_lib[PATH_MAX+1]={EXEOS};
    char ndrx_inc[PATH_MAX+1]={EXEOS};
    char tmp[PATH_MAX+1];
    char *saveptr1;
    char *p;
    env_cc = getenv("CC");
    
    if (NULL!=env_cc && env_cc[0]!=0)
    {
        build_final_cmd = env_cc;
    }
    else
    {
        NDRX_LOG(log_debug, "Use default cc");
        build_final_cmd = default_compiler;
    }
    
    env_cflags = getenv("CFLAGS");
    
    env_ndrx_home = getenv(CONF_NDRX_HOME);

    if (NULL!=env_ndrx_home && env_ndrx_home[0]!=0)
    {
        snprintf(ndrx_inc, sizeof(ndrx_inc), "-I%s/include", env_ndrx_home);
        
        /* check the lib64... */
        snprintf(ndrx_lib, sizeof(ndrx_lib), "-L%s/lib64", env_ndrx_home);
        
        if (!ndrx_file_exists(ndrx_lib))
        {
            snprintf(ndrx_lib, sizeof(ndrx_lib), "-L%s/lib", env_ndrx_home);
        }
    }
    
    NDRX_LOG(log_debug, "Build command set to: [%s]", build_final_cmd);
    
    NDRX_STRCPY_SAFE(build_cmd, build_final_cmd);
    
    if (NULL!=env_cflags)
    {
        NDRX_LOG(log_debug, "CFLAGS=[%s]", env_cflags);
        NDRX_STRCAT_S(build_cmd, sizeof(build_cmd), " ");
        NDRX_STRCAT_S(build_cmd, sizeof(build_cmd), env_cflags);
    }
    
    /* add outfile */
    NDRX_STRCAT_S(build_cmd, sizeof(build_cmd), " -o ");
    NDRX_STRCAT_S(build_cmd, sizeof(build_cmd), ofile);
    
    /* add C file to build */
    NDRX_STRCAT_S(build_cmd, sizeof(build_cmd), " ");
    NDRX_STRCAT_S(build_cmd, sizeof(build_cmd), cfile);
    
    if (EXEOS!=ndrx_inc[0])
    {
        NDRX_STRCAT_S(build_cmd, sizeof(build_cmd), " ");
        NDRX_STRCAT_S(build_cmd, sizeof(build_cmd), ndrx_inc);
    }
    
    if (EXEOS!=ndrx_lib[0])
    {
        NDRX_STRCAT_S(build_cmd, sizeof(build_cmd), " ");
        NDRX_STRCAT_S(build_cmd, sizeof(build_cmd), ndrx_lib);
    }
    
    if (EXEOS!=firstfiles[0])
    {
        NDRX_STRCAT_S(build_cmd, sizeof(build_cmd), " ");
        NDRX_STRCAT_S(build_cmd, sizeof(build_cmd), firstfiles);
    }
    
    /* Add RM libraries if any... */
    if (NULL!=p_rmdef && EXEOS!=p_rmdef->libnames[0])
    {
        NDRX_STRCAT_S(build_cmd, sizeof(build_cmd), " ");
        NDRX_STRCAT_S(build_cmd, sizeof(build_cmd), p_rmdef->libnames);
    }
    
    /* Add any Enduro/X libraries required... */
    if (COMPILE_SRV == buildmode)
    {   
        NDRX_STRCAT_S(build_cmd, sizeof(build_cmd), " -latmisrvinteg -latmi -lubf -lnstd");
    }
    else if (COMPILE_CLT == buildmode)
    {
        NDRX_STRCAT_S(build_cmd, sizeof(build_cmd), " -latmicltbld -latmi -lubf -lnstd");
    }
    else if (COMPILE_TMS == buildmode)
    {        
        NDRX_STRCAT_S(build_cmd, sizeof(build_cmd), 
                " -ltms -latmisrvinteg -latmi -lubf -lexuuid -lexthpool -lnstd");
    }
    
    /* add any platform specific library */
    NDRX_STRCPY_SAFE(tmp, NDRX_RT_LIB);
    
    p=strtok_r(tmp, ";", &saveptr1);
    
    while (NULL!=p)
    {
        /* "Crun" is Sun Studio compiler specific, thus exclude as user
         * might use GNU...
         */
        if (0!=strcmp(p, "Crun"))
        {
            NDRX_STRCAT_S(build_cmd, sizeof(build_cmd), " -l");
            NDRX_STRCAT_S(build_cmd, sizeof(build_cmd), p);
        }
        
        p=strtok_r(NULL, ";", &saveptr1);
    }
    
    NDRX_STRCAT_S(build_cmd, sizeof(build_cmd), " -lm -lc -lpthread");
    
    /* Add last files. */
    if (EXEOS!=lastfiles[0])
    {
        NDRX_STRCAT_S(build_cmd, sizeof(build_cmd), " ");
        NDRX_STRCAT_S(build_cmd, sizeof(build_cmd), lastfiles);
    }
    
    NDRX_LOG(log_debug, "build_cmd: [%s]", build_cmd);
    
    if (verbose)
    {
        fprintf(stderr, "%s\n", build_cmd);
    }
    
    /* exec shell ... */
    if (EXSUCCEED!=(ret=system(build_cmd)))
    {
        _Nset_error_fmt(NEEXEC, "Failed to execute compiler [%s]: %d", 
                build_cmd, ret);
        NDRX_LOG(log_error, "Failed to execute compiler [%s]: %d", 
                build_cmd, ret);
        ret = EXFAIL;
    }
    
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
