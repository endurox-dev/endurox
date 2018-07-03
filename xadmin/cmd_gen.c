/* 
** Generate application sources
**
** @file cmd_gen.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, Mavimax, Ltd. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or Mavimax's license for commercial use.
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
** A commercial use license is available from Mavimax, Ltd
** contact@mavimax.com
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

#include <exhash.h>
#include <utlist.h>
#include <userlog.h>

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

extern const char ndrx_G_resource_gen_go_server[];
extern const char ndrx_G_resource_gen_go_client[];
extern const char ndrx_G_resource_gen_c_client[];
extern const char ndrx_G_resource_gen_c_server[];
extern const char ndrx_G_resource_gen_ubf_tab[];
extern const char ndrx_G_resource_gen_test_local[];

/*---------------------------Macros-------------------------------------*/
/* #define GEN_DEBUG 1 */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
typedef struct gen_hash gen_hash_t;
struct gen_hash
{
    char command[PATH_MAX+1];
    
    char *stock; /* allocated in memory */
    
    char *fname; /* allocated on disk */
    
    EX_hash_handle hh; /* makes this structure hashable        */
};

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

exprivate gen_hash_t *M_gen_res = NULL; /* gen resources */

/*---------------------------Prototypes---------------------------------*/

/**
 * Get the sub-command
 * @param sub_command
 * @return 
 */
exprivate gen_hash_t * gen_get(char *command)
{
    gen_hash_t *ret = NULL;
    
    EXHASH_FIND_STR( M_gen_res, command, ret);
    
    return ret;
}

/**
 * Add static command
 * @param ptr
 * @param file
 * @param command
 * @return 
 */
exprivate int reg_cmd(char *command, const char *stock, const char *fname)
{
    gen_hash_t *gen;
    int ret = EXSUCCEED;
    
    if (NULL==(gen = NDRX_MALLOC(sizeof(gen_hash_t))))
    {
        int err = errno;
        NDRX_LOG(log_error, "Failed to alloc gen_hash_t: %s", strerror(err));
        userlog("Failed to alloc gen_hash_t: %s", strerror(err));
        EXFAIL_OUT(ret);
    }
    
    NDRX_STRNCPY(gen->command, command, sizeof(gen->command));
    gen->command[sizeof(gen->command)-1] = EXEOS;
    
    if (stock)
    {
        if (NULL==(gen->stock = strdup(stock)))
        {
            int err = errno;
            NDRX_LOG(log_error, "Failed to alloc: %s", strerror(err));
            userlog("Failed to alloc: %s", strerror(err));
            EXFAIL_OUT(ret);
        }
    }
    
    if (fname)
    {
        if (NULL==(gen->fname = strdup(fname)))
        {
            int err = errno;
            NDRX_LOG(log_error, "Failed to alloc: %s", strerror(err));
            userlog("Failed to alloc: %s", strerror(err));
            EXFAIL_OUT(ret);
        }
    }
    
#ifdef GEN_DEBUG
    fprintf(stderr, "Adding command [%s]", gen->command);
#endif
    
    /* add stuff to hash */
    EXHASH_ADD_STR( M_gen_res, command, gen );
    
out:
    
    if (EXSUCCEED!=ret)
    {
        if (gen)
        {
            if (gen->stock)
            {
                free(gen->stock);
            }
            
            if (gen->fname)
            {
                free(gen->fname);
            }
            
            free(gen);
        }
    }
    return ret;
}

/**
 * This will load the generation scripts into hash
 * Hash shall contain the ptr to symbol in memory
 * Or path to file (path loaded from config)
 * @return SUCCEED/FAIL
 */
expublic int cmd_gen_load_scripts(void)
{
    int ret = EXSUCCEED;
    ndrx_inicfg_section_keyval_t *val;
    string_list_t* flist, *elt= NULL;
    int return_status = EXSUCCEED;
    
    char tmp[PATH_MAX+1];
    char path[PATH_MAX+1];
    char *start;
    char *p;
    int i;
    
    
    /* 1. List strings in memory, start with "gen_001" */
    if (EXSUCCEED!=reg_cmd("go server", ndrx_G_resource_gen_go_server, NULL)
        || EXSUCCEED!=reg_cmd("go client", ndrx_G_resource_gen_go_client, NULL)
        || EXSUCCEED!=reg_cmd("c server", ndrx_G_resource_gen_c_server, NULL)
        || EXSUCCEED!=reg_cmd("c client", ndrx_G_resource_gen_c_client, NULL)
        || EXSUCCEED!=reg_cmd("ubf tab", ndrx_G_resource_gen_ubf_tab, NULL)
        || EXSUCCEED!=reg_cmd("test local", ndrx_G_resource_gen_test_local, NULL)
       )
    {
        EXFAIL_OUT(ret);
    }
    
    /* 2. List strings on disk, if have such config string */
    if (NULL!=(val=ndrx_keyval_hash_get(G_xadmin_config, "gen scripts")))
    {
        if (NULL!=(flist = ndrx_sys_folder_list(val->val, &return_status)))
        {
            int len, replaced;
            /* Loop over the files & load the into memory 
             * If name terminates with .pscript, then load it.
             */
            
            LL_FOREACH(flist,elt)
            {
                len = strlen(elt->qname);
                
                if (len>8 && 0==strncmp(elt->qname+(len-8), ".pscript", 8)
                        /* name shall match gen_<lang>_<type>.pscript */
                    )
                {
                    NDRX_STRNCPY(tmp, elt->qname, sizeof(tmp));
                    tmp[sizeof(tmp)-1] = EXEOS;
                    
                    
                    start = strstr(tmp, "gen_");
                    
                    if (NULL==start)
                    {
                        /* not ours invalid name.. */
                        continue; 
                    }
                    
                    start+=4;
                    
                    p = strrchr(tmp, '.');
                    
                    *p = EXEOS; /* do not care the suffix */
                    
                    /* replace _ with space*/   
                    len = strlen(start);
                    replaced = 0;
                    for (i=0; i<len; i++)
                    {
                        if ('_' == start[i])
                        {
                            replaced++;
                            start[i]=' ';
                        }
                    }
                    
                    if (!replaced)
                    {
                        /* not ours invalid name.. */
                        continue;
                    }
                    
                    if (len==replaced)
                    {
                        /* not ours invalid name.. */
                        continue;
                    }
                    
                    /* ok it is ours.. */
                    snprintf(path, sizeof(path), "%s%s", val->val, elt->qname);
                    
#ifdef GEN_DEBUG
                    fprintf(stdout, "Adding resource: cmd [%s] file [%s]\n", 
                            start, path);
#endif
                    
                    if (EXSUCCEED!=reg_cmd(start, NULL, path))
                    {
                        EXFAIL_OUT(ret);
                    }

                }
            }
           
            ndrx_string_list_free(flist);
        }
    }
    
out:
    
    return ret;
}

/**
 * This will list available commands for help callback.
 * @return 
 */
expublic int cmd_gen_help(void)
{
    gen_hash_t *val = NULL, *val_tmp = NULL;
    
    EXHASH_ITER(hh, M_gen_res, val, val_tmp)
    {
        fprintf(stdout, "\t\t%s\n", val->command);
    }
    
    fprintf(stdout, "\n");
    
    return EXSUCCEED;
}

/**
 * File feed for reader from file
 * @param file
 * @return 
 */
PSInteger file_lexfeedASCII(PSUserPointer file)
{
    int ret;
    char c;
    if( ( ret=fread(&c,sizeof(c),1,(FILE *)file )>0) )
    {
        return c;
    }
    return 0;
}

/**
 * Generate application sources
 * We will dynamically lookup the exported symbols, list them and offer these
 * for commands for generation
 * 
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
expublic int cmd_gen(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret=EXSUCCEED;
    const PSChar *s;
    HPSCRIPTVM v = NULL;
    PSInteger res;
    int i;
    const PSChar *err;
    char tmp[PATH_MAX+1];
    gen_hash_t *gen;
        
    FILE *f = NULL;
    
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
    
    
    if (argc>=3)
    {
        snprintf(tmp, sizeof(tmp), "%s %s", argv[1], argv[2]);
        
        /* lookup the command */
        if (NULL==(gen = gen_get(tmp)))
        {
            fprintf(stderr, "Unknown gen sub-command: [%s]\n", tmp);
            EXFAIL_OUT(ret);
        }
    }
    else 
    {
        fprintf(stderr, "Invalid arguments, format: gen <language> <type>\n");
        EXFAIL_OUT(ret);
    }
    
    /* make full command for defaults lookup */
    snprintf(tmp, sizeof(tmp), "gen %s %s", argv[1], argv[2]);
    
    if (EXSUCCEED!=add_defaults_from_config(v, tmp))
    {
        EXFAIL_OUT(ret);
    }
    
    if (argc>3)
    {
        for (i=3; i<argc; i++)
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
    if (gen->stock)
    {
        if (PS_FAILED(ps_compilebuffer(v, gen->stock, 
                    strlen(gen->stock), "gen.ps", PSTrue)))
        {
            fprintf(stderr, "Failed to compile stock script\n");

            if(PS_SUCCEEDED(ps_getstring(v,-1,&err)))
            {
                fprintf(stderr, _SC("Error [%s]\n"),err);
                EXFAIL_OUT(ret);
            }
        }
        else
        {
            ps_pushroottable(v);
        }
    }
    else
    {
        /* it is file read & compile */
        FILE *f=fopen(gen->fname,"rb");
        
        if(f)
        {
            if (PS_FAILED(ps_compile(v,file_lexfeedASCII,f,gen->fname,1)))
            {
               fprintf(stderr, "Failed to compile stock script\n");

               if(PS_SUCCEEDED(ps_getstring(v,-1,&err)))
               {
                   fprintf(stderr, _SC("Error [%s]\n"),err);
                   EXFAIL_OUT(ret);
               }
            }
            else
            {
                ps_pushroottable(v);
            } 
        }
        else
        {
            fprintf(stderr, "Failed to read script file: [%s]:%s", 
                    gen->fname, strerror(errno));
            EXFAIL_OUT(ret);
        }
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

out:
    if (v)
    {
        ps_close(v);
    }

    if (f)
    {
        fclose(f);
    }
    

    return ret;
}

#else
/**
 * Pscript not supported
 * @return FAIL
 */
expublic int cmd_gen_help(void)
{
    fprintf(stderr, "Pscript not compiled for this platform!\n");
    return EXFAIL;
}

/**
 * Not compiled on this platform
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return FAIL
 */
expublic int cmd_gen(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    fprintf(stderr, "Pscript not compiled for this platform!\n");
    return EXFAIL;
}

/**
 * Dummy for non pscript systems
 * @return SUCCEED
 */
expublic int cmd_gen_load_scripts(void)
{
    return EXSUCCEED;
}

#endif
