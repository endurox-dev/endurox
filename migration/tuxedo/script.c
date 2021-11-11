/**
 * @brief Run the script
 *
 * @file script.c
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
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <memory.h>
#include <errno.h>
#include <regex.h>
#include <nstdutil.h>

#include <exregex.h>
#include "tux.h"
#include "tux.tab.h"
#include "ndebug.h"
#include <sys_unix.h>
#include <atmi_int.h>

#include <pscript.h>
#include <psstdblob.h>
#include <psstdio.h>
#include <psstdsystem.h>
#include <psstdmath.h>
#include <psstdstring.h>
#include <psstdexutil.h>
#include <psstdaux.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
exprivate HPSCRIPTVM v;
/*---------------------------Prototypes---------------------------------*/

expublic int call_add_func(const char *func, char *arg)
{
    int ret = EXSUCCEED;
    PSInteger i=0;
    const PSChar *err;
    
    ps_pushroottable(v);
    ps_pushstring(v,func,-1);
    ps_get(v,-2);
    ps_pushroottable(v);
    ps_pushstring(v,arg,-1);
    
    if (PS_FAILED(ps_call(v,2,PSTrue,PSTrue)))
    {
        ps_getlasterror(v);
        if(PS_SUCCEEDED(ps_getstring(v,-1,&err)))
        {
            _Nset_error_fmt(NESYSTEM, "Failed to call script func %s: %s", func,
                    err);
        }
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_error, "OK");
    
    ps_pop(v,3); /* pops the roottable and the function*/
    
out:
    
    return ret;
     
}

expublic int tux_add_val(char *arg)
{
    int ret=EXSUCCEED;
    NDRX_LOG(log_debug, "Add value [%s]", arg);
    
    ret=call_add_func(__func__, arg);
    
    NDRX_FREE(arg);
    return ret;
}

expublic int tux_add_res_parm(char *arg)
{
    int ret=EXSUCCEED;
    NDRX_LOG(log_debug, "Add resource param [%s]", arg);
    
    ret=call_add_func(__func__, arg);
    NDRX_FREE(arg);
    return ret;
}

expublic int tux_add_sect_parm(char *arg)
{
    int ret=EXSUCCEED;
    NDRX_LOG(log_debug, "Add section param [%s]", arg);
    ret=call_add_func(__func__, arg);
    NDRX_FREE(arg);
    return ret;
}

expublic int tux_add_sect_keyw(char *arg)
{
    int ret=EXSUCCEED;
    NDRX_LOG(log_debug, "Add section keyword [%s]", arg);
    ret=call_add_func(__func__, arg);
    NDRX_FREE(arg);
    return ret;
}

expublic int tux_add_sect(char *arg)
{
    int ret=EXSUCCEED;
    NDRX_LOG(log_debug, "Add section [%s]", arg);
    ret=call_add_func(__func__, arg);
    NDRX_FREE(arg);
    return ret;
}

/**
 * Print function for pscript
 * @param v
 * @param s
 * @param ...
 */
expublic void printfunc(HPSCRIPTVM v,const PSChar *s,...)
{
    char buf[5000];
    char *p;
    
    buf[0]=EXEOS;
    
    va_list vl;
    va_start(vl, s);
    vsnprintf(buf, sizeof(buf), s, vl);
    va_end(vl);
    
    p=ndrx_str_lstrip_ptr(buf, "\n");
    ndrx_str_rstrip(p, "\n");
    NDRX_LOG(log_debug, "%s", p);
}

/**
 * Error function
 * @param v
 * @param s
 * @param ...
 */
expublic void errorfunc(HPSCRIPTVM v,const PSChar *s,...)
{
    char buf[5000];
    char *p;
    buf[0]=EXEOS;
    
    va_list vl;
    va_start(vl, s);
    vsnprintf(buf, sizeof(buf), s, vl);
    va_end(vl);
    
    p=ndrx_str_lstrip_ptr(buf, "\n");
    ndrx_str_rstrip(p, "\n");
    NDRX_LOG(log_error, "%s", p);
}


/**
 * Start the VM, firstly to fill up the structures
 * @param script_nm script name to load
 * @return 
 */
expublic int tux_init_vm(char *script_nm)
{
    int ret=EXSUCCEED;
    const PSChar *s;
    const PSChar *err;
    char *script = NULL;
    size_t len=0;
    v = ps_open(2048); /* creates a VM with initial stack size 1024 */
    
    ps_setprintfunc(v,printfunc,errorfunc);
    
    ps_pushroottable(v);
    
    /* register functions */
    psstd_register_bloblib(v);
    psstd_register_iolib(v);
    psstd_register_systemlib(v);
    psstd_register_mathlib(v);
    psstd_register_stringlib(v);
    psstd_register_exutillib(v);

    /* aux library
     * sets error handlers */
    psstd_seterrorhandlers(v);
    
    script=ndrx_file_read(script_nm, &len);
    
    if (NULL==script)
    {
        EXFAIL_OUT(ret);
    }
    
    /* Compile & load the script */
    if (PS_FAILED(ps_compilebuffer(v, script, len, script_nm, PSTrue)))
    {
        if(PS_SUCCEEDED(ps_getstring(v, -1, &err)))
        {
            _Nset_error_fmt(NESYSTEM, "Failed to compile script: %s", err);
            EXFAIL_OUT(ret);
        }
        EXFAIL_OUT(ret);
    }
    else
    {
       ps_pushroottable(v);
    }
    
    /* Load the function */
    if (PS_FAILED(ps_call(v,1,PSTrue, PSTrue)))
    {
        ps_getlasterror(v);
        if(PS_SUCCEEDED(ps_getstring(v,-1,&err)))
        {
            _Nset_error_fmt(NESYSTEM, "Failed load closure", err);
            EXFAIL_OUT(ret);
        }
    }
    
    NDRX_LOG(log_debug, "VM Script Loaded");
     
    
out:

    if (NULL!=script)
    {
        NDRX_FREE(script);
    }

    return ret;

}

/* vim: set ts=4 sw=4 et smartindent: */
