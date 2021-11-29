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
#include "ubb.h"
#include "ubb.tab.h"
#include "ddr.tab.h"
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
#include "buildtools.h"
/*---------------------------Externs------------------------------------*/
extern void ddr_scan_string (char *yy_str  );
extern int ddrlex_destroy  (void);
extern int ndrx_G_ddrcolumn;

extern const char ndrx_G_resource_tmloadcf_bytecode[];
extern const size_t ndrx_G_resource_tmloadcf_bytecode_len;

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
    
    ps_pop(v,3); /* pops the roottable and the function*/
    
out:
    
    return ret;
     
}

expublic int ubb_add_val(char *arg)
{
    int ret=EXSUCCEED;
    NDRX_LOG(log_debug, "Add value [%s]", arg);
    
    ret=call_add_func(__func__, arg);
    
    NDRX_FREE(arg);
    return ret;
}

expublic int ubb_add_res_parm(char *arg)
{
    int ret=EXSUCCEED;
    NDRX_LOG(log_debug, "Add resource param [%s]", arg);
    
    ret=call_add_func(__func__, arg);
    NDRX_FREE(arg);
    return ret;
}

expublic int ubb_add_sect_parm(char *arg)
{
    int ret=EXSUCCEED;
    NDRX_LOG(log_debug, "Add section param [%s]", arg);
    ret=call_add_func(__func__, arg);
    NDRX_FREE(arg);
    return ret;
}

expublic int ubb_add_sect_keyw(char *arg)
{
    int ret=EXSUCCEED;
    NDRX_LOG(log_debug, "Add section keyword [%s]", arg);
    ret=call_add_func(__func__, arg);
    NDRX_FREE(arg);
    return ret;
}

expublic int ubb_add_sect(char *arg)
{
    int ret=EXSUCCEED;
    NDRX_LOG(log_debug, "Add section [%s]", arg);
    ret=call_add_func(__func__, arg);
    NDRX_FREE(arg);
    return ret;
}

/**
 * This is callback from script
 * When DDR is requested to be parsed.
 * @param v
 * @return 1 - ok, 0 - fail
 */
static PSInteger ubb_ddr_parse(HPSCRIPTVM v)
{
    int ret = EXSUCCEED;
    const PSChar *s=NULL;
    char err[256];
    
    memset(&ndrx_G_ddrp, 0, sizeof(ndrx_G_ddrp));
    /* init the string builder */
    ndrx_growlist_init(&ndrx_G_ddrp.stringbuffer, 200, sizeof(char));
    
    if(PS_SUCCEEDED(ps_getstring(v,2,&s)))
    {
        NDRX_LOG(log_error, "PARSE RANGE [%s]", s);
        /* start to parse... */
        ndrx_G_ddrcolumn=0;
                
        ndrx_G_ddrp.parsebuf=(char *)s;
        
        ddr_scan_string((char *)s);

        if (EXSUCCEED!=ddrparse() || EXSUCCEED!=ndrx_G_ddrp.error)
        {
            NDRX_LOG(log_error, "Failed to parse tux config");

            /* free parsers... */
            ddrlex_destroy();

            /* well if we hav */
            EXFAIL_OUT(ret);
        }
        ddrlex_destroy();
    }    
    else
    {
        EXFAIL_OUT(ret);
    }
    
out:
    
    /* free up string buffer */
    ndrx_growlist_free(&ndrx_G_ddrp.stringbuffer);
    
    if (EXSUCCEED!=ret)
    {
        if (EXEOS!=ndrx_G_ddrp.errbuf[0])
        {
            return ps_throwerror(v, ndrx_G_ddrp.errbuf);
        }
        else
        {
            snprintf(err, sizeof(err), "Failed to parse DDR expression [%.20s] near line %d",
                    (char *)s, ndrx_G_ubbline);
            return ps_throwerror(v, err);
        }
    }

    return ret;
}

/**
 * Mark group as routed
 * @param seq
 * @param grp
 * @param is_mallocd
 * @return 
 */
expublic int ndrx_ddr_add_group(ndrx_routcritseq_dl_t * seq, char *grp, int is_mallocd)
{
    int ret;
    
    ret = call_add_func("ubb_mark_group_routed", grp);
    
    NDRX_FREE(grp);
    NDRX_FREE(seq);
    
    return ret;
}

/**
 * Dummy not used
 * @param range
 * @param is_negative
 * @param dealloc
 * @return 
 */
expublic char *ndrx_ddr_new_rangeval(char *range, int is_negative, int dealloc)
{   
    char *ret=NULL;
    int len;
    
    if (!is_negative)
    {
        ret=NDRX_STRDUP(range);
    }
    else
    {
        NDRX_ASPRINTF(&ret, &len, "-%s", range);
    }
    
    if (dealloc)
    {
        NDRX_FREE(range);
    }
    
    return ret;

}

/**
 * Dummy not used.
 * @param range_min
 * @param range_max
 * @return 
 */
expublic ndrx_routcritseq_dl_t * ndrx_ddr_new_rangeexpr(char *range_min, char *range_max)
{   
    if (NULL!=range_min)
    {
        NDRX_FREE(range_min);
    }
    
    if (range_max!=range_min && NULL!=range_max)
    {
        NDRX_FREE(range_max);
    }
    
    return (ndrx_routcritseq_dl_t *)(NDRX_MALLOC(1));
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
 * Get RM switch
 * @param [script] RM name
 * @return XASwitch struct name, or "" empty string
 */
static PSInteger ubb_get_rmswitch(HPSCRIPTVM v)
{
    const PSChar *str;
    
    ndrx_rm_def_t sw;
    ps_getstring(v,2,&str);
    
    if (EXTRUE==ndrx_get_rm_name((char *)str, &sw))
    {
        ps_pushstring(v, sw.structname, -1);
    }
    else
    {
        ps_pushstring(v, "", -1);
    }
    
    return 1;

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
 * @param n_opt check only
 * @param y_opt auto confirm overwrite
 * @param l_opt LMDID only which shall be generated.
 * @param a_opt assign new SRVIDs
 * @return EXSUCCEED/EXFAIL
 */
expublic int init_vm(char *script_nm,
        char *n_opt, char *y_opt, char *l_opt, char *a_opt)
{
    int ret=EXSUCCEED;
    const PSChar *s;
    const PSChar *err;
    char *script = NULL;
    size_t len=0;
    v = ps_open(2048); /* creates a VM with initial stack size 1024 */
    
    ps_setprintfunc(v,printfunc,errorfunc);
    
    ps_pushroottable(v);
    
    /* Load settings into root table */
    
    ps_pushstring(v,"M_n_opt",-1);
    ps_pushstring(v,n_opt,-1);
    ps_newslot(v,-3,PSFalse);
    
    ps_pushstring(v,"M_y_opt",-1);
    ps_pushstring(v,y_opt,-1);
    ps_newslot(v,-3,PSFalse);
    
    ps_pushstring(v,"M_l_opt",-1);
    ps_pushstring(v,l_opt,-1);
    ps_newslot(v,-3,PSFalse);
    
    ps_pushstring(v,"M_a_opt",-1);
    ps_pushstring(v,a_opt,-1);
    ps_newslot(v,-3,PSFalse);
    
    /* register functions */
    psstd_register_bloblib(v);
    psstd_register_iolib(v);
    psstd_register_systemlib(v);
    psstd_register_mathlib(v);
    psstd_register_stringlib(v);
    psstd_register_exutillib(v);

    /* register new func for DDR parsing. */
    ps_pushstring(v,"ubb_ddr_parse",-1);
    ps_newclosure(v,ubb_ddr_parse,0);
    ps_setparamscheck(v,2,".s");
    ps_setnativeclosurename(v,-1,"ubb_ddr_parse");
    ps_newslot(v,-3,PSFalse);
    
    /* Switch resolver */
    ps_pushstring(v,"ubb_get_rmswitch",-1);
    ps_newclosure(v,ubb_get_rmswitch,0);
    ps_setparamscheck(v,2,".s");
    ps_setnativeclosurename(v,-1,"ubb_ddr_parse");
    ps_newslot(v,-3,PSFalse);
    
    /* aux library
     * sets error handlers */
    psstd_seterrorhandlers(v);
    
    if (EXEOS!=script_nm[0])
    {
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
    }
    else
    {
        PSMemReader reader;
        
        memset(&reader, 0, sizeof(reader));
        
        reader.memptr = (char *)ndrx_G_resource_tmloadcf_bytecode;
        reader.size = ndrx_G_resource_tmloadcf_bytecode_len;
        
        if (PS_FAILED(psstd_loadmem(v, &reader)))
        {
            if(PS_SUCCEEDED(ps_getstring(v, -1, &err)))
            {
                _Nset_error_fmt(NESYSTEM, "Failed to load bytecode: %s", err);
                EXFAIL_OUT(ret);
            }
            else
            {
                _Nset_error_msg(NESYSTEM, "Failed to load bytecode: no error specfied");
                EXFAIL_OUT(ret);
            }
        }
    }
    
    ps_pushroottable(v);
    
    
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
