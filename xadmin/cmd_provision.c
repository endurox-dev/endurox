/* 
** Create and application instance
**
** @file cmd_provision.c
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
#endif


/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/* Have some access to resources */

extern const char G_resource_provision[];
extern const char G_resource_Exfields[];

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

#ifndef NDRX_DISABLEPSCRIPT

void printfunc(HPSCRIPTVM v,const PSChar *s,...)
{
        va_list vl;
        va_start(vl, s);
        vfprintf(stdout, s, vl);
        va_end(vl);
}


void errorfunc(HPSCRIPTVM v,const PSChar *s,...)
{
        va_list vl;
        va_start(vl, s);
        vfprintf(stderr, s, vl);
        va_end(vl);
}

/**
 *Read the line from terminal
 *@return line read string
 */
static PSInteger _xadmin_getExfields(HPSCRIPTVM v)
{
    
    ps_pushstring(v,G_resource_Exfields,-1);

    return 1;
}

/**
 * Provide the Exfields function to root table.
 */
private int register_getExfields(HPSCRIPTVM v)
{
    
    ps_pushstring(v,"getExfields",-1);
    ps_newclosure(v,_xadmin_getExfields,0);
    ps_setparamscheck(v,1,".s");
    ps_setnativeclosurename(v,-1,"getExfields");
    ps_newslot(v,-3,PSFalse);

    return 1;
}

/**
 * Load key:value string into VM table
 * @param v
 * @param key_val_string
 * @return 
 */
private int load_value(HPSCRIPTVM v, char *key_val_string)
{
    int ret = SUCCEED;
    char *p;
    int len = strlen(key_val_string);
    
    p=strchr(key_val_string, '=');
    
    if (!len)
    {
        fprintf(stderr, "Empty value string!\n");
        FAIL_OUT(ret);
    }
    
    if (NULL==p)
    {
        fprintf(stderr, "Missing '=' in value [%s]!\n", key_val_string);
        FAIL_OUT(ret);
    }
    
    if (p==key_val_string)
    {
        fprintf(stderr, "Missing value [%s]!\n", key_val_string);
        FAIL_OUT(ret);
    }
    
    if (p==key_val_string)
    {
        fprintf(stderr, "Missing value [%s]!\n", key_val_string);
        FAIL_OUT(ret);
    }
    
    *p = EOS;
    p++;
    
    /* fprintf(stderr, "Setting value: %s\n", p); */
    ps_pushstring(v, key_val_string, -1); /* 4 */
    ps_pushstring(v, p, -1);/* 5 */
    ps_newslot(v, -3, PSFalse );/* 3 */
    
out:
    return ret;
}

/**
 * Add defaults from config file
 * @return 
 */
private int add_defaults_from_config(HPSCRIPTVM v)
{
    int ret = SUCCEED;
    ndrx_inicfg_section_keyval_t *val;
    char *ptr = NULL;
    char *p;
    /* get the value if have one */
    if (NULL!=(val=ndrx_keyval_hash_get(G_xadmin_config, "provision")))
    {
        userlog("Got config defaults for provision: [%s]", val->val);
        
        if (NULL==(ptr = strdup(val->val)))
        {
            userlog("Malloc failed: %s", strerror(errno));
            FAIL_OUT(ret);
        }
        
        /* OK token the string and do override */
        
        /* Now split the stuff */
        p = strtok (ptr, ARG_DEILIM);
        while (p != NULL)
        {
            if (0==strcmp(p, "-d"))
            {
                PSBool isDefaulted = TRUE;
                ps_pushstring(v, "isDefaulted", -1); /* 4 */
                ps_pushbool(v, isDefaulted);
                ps_newslot(v, -3, PSFalse );/* 3 */
            }
            else if (0==strncmp(p, "-v", 2) 
                    && 0!=strcmp(p, "-v"))
            {
                if (strlen(p) < 4)
                {
                    userlog("Invalid default settings for provision command [%s]", 
                            val->val);
                    FAIL_OUT(ret);
                }
                
                /* pass in value definition (as string) 
                 * format <key>=<value>
                 */
                if (SUCCEED!=load_value(v, p+2))
                {
                    userlog("Invalid value\n");
                    FAIL_OUT(ret);
                }
            }
            else if (0==strncmp(p, "-v", 2))
            {
                p = strtok (NULL, ARG_DEILIM);
                
                /* next value is key */
                if (NULL!=p)
                {
                    if (SUCCEED!=load_value(v, p))
                    {
                        userlog("Invalid value on provision defaults [%s]", 
                                val->val);
                        FAIL_OUT(ret);
                    }
                }
                else
                {
                    userlog("Invalid command line missing value at end [%s]", 
                            val->val);
                    FAIL_OUT(ret);
                }
            }
            
            p = strtok (NULL, ARG_DEILIM);
        }
    }
    
out:

    if (NULL!=ptr)
    {
        free(ptr);
    }

    if (SUCCEED!=ret)
    {
        fprintf(stderr, "Failed to process defaults - invalid config [%s], see ULOG\n", 
                G_xadmin_config_file);
    }

    return ret;
}
    
/**
 * Run the wizzard for application via pscrip
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
public int cmd_provision(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret=SUCCEED;
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
    
    if (SUCCEED!=add_defaults_from_config(v))
    {
        FAIL_OUT(ret);
    }
    
    if (argc>=2)
    {
        for (i=1; i<argc; i++)
        {
            /* load defaults */
            if (0==strcmp(argv[i], "-d"))
            {
                PSBool isDefaulted = TRUE;
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
                if (SUCCEED!=load_value(v, argv[i]+2))
                {
                    fprintf(stderr, "Invalid value\n");
                    FAIL_OUT(ret);
                }
            }
            else if (0==strncmp(argv[i], "-v", 2))
            {
                /* next value is key */
                if (i+1<argc)
                {
                    i++;
                    if (SUCCEED!=load_value(v, argv[i]))
                    {
                        fprintf(stderr, "Invalid value\n");
                        FAIL_OUT(ret);
                    }
                }
                else
                {
                    fprintf(stderr, "Invalid command line missing value at end.\n");
                    FAIL_OUT(ret);
                }
            }
            
        }
    }
    
    /* Load the command line arguments to the script */
    
    ps_newslot(v, -3, PSFalse );/*1*/


    
    /* do some stuff with pscript here */
    if (PS_FAILED(ps_compilebuffer(v, G_resource_provision, 
                strlen(G_resource_provision), "provision.ps", PSTrue)))
    {
        fprintf(stderr, "Failed to compile...\n");

        if(PS_SUCCEEDED(ps_getstring(v,-1,&err)))
        {
            fprintf(stderr, _SC("Error [%s]\n"),err);
            return FAIL;
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
            FAIL_OUT(ret);
        }
    }
    
    /* pop the result out of the stack */
    ps_getinteger(v,-1,&res);
    
    if (res>=0)
    {
        ret = SUCCEED;
    }
    else
    {
        ret = FAIL;
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
public int cmd_provision(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    fprintf(stderr, "Pscript not compiled for this platform!\n");
    return FAIL;
}

#endif
