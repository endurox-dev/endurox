/* 
 * @brief Create and application instance
 *
 * @file cmd_provision.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or Mavimax's license for commercial use.
 * -----------------------------------------------------------------------------
 * GPL license:
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
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
#include <errno.h>

#include "nclopt.h"

#ifndef NDRX_DISABLEPSCRIPT
#include <pscript.h>
#include <psstdblob.h>
#include <psstdio.h>
#include <psstdsystem.h>
#include <psstdmath.h>
#include <psstdstring.h>
#include <psstdexutil.h>
#include <psstdaux.h>
#endif


#ifndef NDRX_DISABLEPSCRIPT
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/* Have some access to resources */
extern const char ndrx_G_resource_provision[];

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Run the wizard for application via pscrip
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
expublic int cmd_provision(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret=EXSUCCEED;
    const PSChar *s;
    HPSCRIPTVM v;
    PSInteger res;
    int i;
    const PSChar *err;
        
    v = ps_open(1024); /* creates a VM with initial stack size 1024 */
    
    ps_setprintfunc(v,printfunc,errorfunc);
    
    ps_pushroottable(v);
    
    /* register functions */
    psstd_register_bloblib(v);
    psstd_register_iolib(v);
    psstd_register_systemlib(v);
    psstd_register_mathlib(v);
    psstd_register_stringlib(v);
    psstd_register_exutillib(v);

    register_getExfields(v);
    /* aux library
     * sets error handlers */
    psstd_seterrorhandlers(v);
    
    /* Load any parameters in different table */
    
    ps_pushstring(v, "args", -1); /* 2 */
    ps_newtable(v); /* 3 */
    
    if (EXSUCCEED!=add_defaults_from_config(v, "provision"))
    {
        EXFAIL_OUT(ret);
    }
    
    if (argc>=2)
    {
        for (i=1; i<argc; i++)
        {
            /* load defaults */
            if (0==strcmp(argv[i], "-d"))
            {
                PSBool isDefaulted = EXTRUE;
                ps_pushstring(v, "isDefaulted", -1); /* 4 */
                ps_pushbool(v, isDefaulted);
                ps_newslot(v, -3, PSFalse );/* 3 */
            }
            else if (0==strncmp(argv[i], "-v", 2) 
                    && 0!=strcmp(argv[i], "-v") )
            {
                /* pass in value definition (as string) 
                 * format <key>=<value>
                 */
                if (EXSUCCEED!=load_value(v, argv[i]+2))
                {
                    fprintf(stderr, "Invalid value\n");
                    EXFAIL_OUT(ret);
                }
            }
            else if (0==strncmp(argv[i], "-v", 2))
            {
                /* next value is key */
                if (i+1<argc)
                {
                    i++;
                    if (EXSUCCEED!=load_value(v, argv[i]))
                    {
                        fprintf(stderr, "Invalid value\n");
                        EXFAIL_OUT(ret);
                    }
                }
                else
                {
                    fprintf(stderr, "Invalid command line missing value at end.\n");
                    EXFAIL_OUT(ret);
                }
            }
            
        }
    }
    
    /* Load the command line arguments to the script */
    
    ps_newslot(v, -3, PSFalse );/*1*/


    
    /* do some stuff with pscript here */
    if (PS_FAILED(ps_compilebuffer(v, ndrx_G_resource_provision, 
                strlen(ndrx_G_resource_provision), "provision.ps", PSTrue)))
    {
        fprintf(stderr, "Failed to compile...\n");

        if(PS_SUCCEEDED(ps_getstring(v,-1,&err)))
        {
            fprintf(stderr, _SC("Error [%s]\n"),err);
            return EXFAIL;
        }
    }
    else
    {
        ps_pushroottable(v);
    }

    /* Load the script & run globals... */
    if (PS_FAILED(ps_call(v,1,PSTrue, PSTrue)))
    {
        fprintf(stderr, "Failed to load script\n");
        ps_getlasterror(v);
        if(PS_SUCCEEDED(ps_getstring(v,-1,&err)))
        {
            printf(_SC("Error [%s]\n"),err);
            EXFAIL_OUT(ret);
        }
    }
    
    /* pop the result out of the stack */
    ps_getinteger(v,-1,&res);
    
    if (res>=0)
    {
        ret = EXSUCCEED;
    }
    else
    {
        ret = EXFAIL;
    }
    
    ps_pop(v,3); /* pops the roottable and the function */

    ps_close(v);
    
out:
    return ret;
}

#else
/**
 * Not compiled on this platform
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
expublic int cmd_provision(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    fprintf(stderr, "Pscript not compiled for this platform!\n");
    return EXFAIL;
}

#endif
