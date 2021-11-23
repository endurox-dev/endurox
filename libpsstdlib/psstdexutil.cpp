/**
 * @brief Enduro/X standard utilities
 *
 * @file psstdexutil.cpp
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
#define __USE_POSIX_IMPLICITLY
#include <ndrstandard.h>
#include <pscript.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <psstdexutil.h>
#include <ndebug.h>
#include <nstdutil.h>
#include <userlog.h>
#include <string.h>
#include <ndrx_config.h>
#include <sys/stat.h>
#include <errno.h>
#include <netdb.h>


#define _XOPEN_SOURCE_EXTENDED 1
#include <libgen.h>

extern "C" const char ndrx_G_resource_WizardBase[];

//Read the line from terminal
//@return line read string
static PSInteger _exutil_getline(HPSCRIPTVM v)
{
    char ln[PATH_MAX+1];
    
    ndrx_fgets_stdin_strip(ln, sizeof(ln));
    
    ps_pushstring(v,ln,-1);

    return 1;
}

//Return the compiled operating system name
static PSInteger _exutil_getosname(HPSCRIPTVM v)
{
    ps_pushstring(v,NDRX_BUILD_OS_NAME,-1);
    
    return 1;
}

//Return the compiled operating system name
static PSInteger _exutil_getpoller(HPSCRIPTVM v)
{
    ps_pushstring(v,EX_POLLER_STR,-1);
    
    return 1;
}

//Return wizard base script
static PSInteger _exutil_getwizardbase(HPSCRIPTVM v)
{
    ps_pushstring(v,ndrx_G_resource_WizardBase,-1);
    
    return 1;
}


//Get the current working dir
//@return   current working dir
static PSInteger _exutil_getcwd(HPSCRIPTVM v)
{
    char* ret;
    char buff[PATH_MAX + 1];
    
    ret = getcwd( buff, sizeof(buff) );
     
    ps_pushstring(v,ret,-1);
    
    
    return 1;
}

//Check that file exists on disk
//@return   true exists, false does not exists
static PSInteger _exutil_fileexists(HPSCRIPTVM v)
{
    const PSChar *s;
    PSBool b;
    
    if(PS_SUCCEEDED(ps_getstring(v,2,&s))) {
        
        if (ndrx_file_exists((char *)s))
        {
            b = PSTrue;
        }
        else
        {
            b = PSFalse;
        }
        
        ps_pushbool(v, b);
    }
    
    return 1;
}

//Write user log message
//@param msg    Message
static PSInteger _exutil_userlog(HPSCRIPTVM v)
{
    const PSChar *s;
    if(PS_SUCCEEDED(ps_getstring(v,2,&s))){
        
        userlog_const (s);
        
        return 1;
    }
    return 0;
}

static PSInteger _exutil_chmod(HPSCRIPTVM v)
{
    const PSChar *file;
    const PSChar *mode;
    char err[256];
    
    if(PS_SUCCEEDED(ps_getstring(v,2,&file)) &&
        PS_SUCCEEDED(ps_getstring(v,3,&mode))) {
        
        int mod;
        sscanf(mode, "%o", &mod);
        
        if (EXSUCCEED!=chmod(file, mod))
        {
            snprintf(err, sizeof(err), "chmod failed: %d:%s", 
                    errno, strerror(errno));
            return ps_throwerror(v,err);
        }
        
        
        return 1;
    }
    return 0;
}

//mkdir, change to 755 mode. 
//Allow recursive mkdir. In that case we shall return an array
//of directories created.
//@param[script] dir    Diretory to create
static PSInteger _exutil_mkdir(HPSCRIPTVM v)
{
    const PSChar *s;
    char err[256];
    struct stat sb;
    
    if(PS_SUCCEEDED(ps_getstring(v,2,&s)))
    {
        /* Check folder for existance and create if missing */
        if (stat(s, &sb) != 0 || !S_ISDIR(sb.st_mode))
        {
            if (EXSUCCEED!=mkdir(s, 0777))
            {
                snprintf(err, sizeof(err), "mkdir [%s] failed: %d:%s", 
                        s, errno, strerror(errno));
                return ps_throwerror(v,err);
            }
        }
        
        return 1;
    }
    return 0;
}

/**
 * Remove directory
 * @param v
 * @return 
 */
static PSInteger _exutil_rmdir(HPSCRIPTVM v)
{
    const PSChar *s;
    char err[256];
    
    if(PS_SUCCEEDED(ps_getstring(v,2,&s)))
    {
        if (EXSUCCEED!=rmdir(s))
        {
            snprintf(err, sizeof(err), "rmdir [%s] failed: %d:%s", 
                    s, errno, strerror(errno));
            return ps_throwerror(v,err);
        }
        
        return 1;
    }
    return 0;
}

/**
 * Remove file
 * @param v
 * @return 
 */
static PSInteger _exutil_unlink(HPSCRIPTVM v)
{
    const PSChar *s;
    char err[256];
    
    if(PS_SUCCEEDED(ps_getstring(v,2,&s)))
    {
        if (EXSUCCEED!=unlink(s))
        {
            snprintf(err, sizeof(err), "unlink [%s] failed: %d:%s", 
                    s, errno, strerror(errno));
            return ps_throwerror(v,err);
        }
        
        return 1;
    }
    return 0;
}

/**
 * Return filename in path
 * @param v
 * @return 
 */
static PSInteger _exutil_basename(HPSCRIPTVM v)
{
    const PSChar *str;
    PSInteger memsize;
    PSChar * stemp;
    
    ps_getstring(v,2,&str);
    memsize = (ps_getsize(v,2)+1)*sizeof(PSChar);
    stemp = ps_getscratchpad(v,memsize);
    memcpy(stemp, str, memsize);
    
    ps_pushstring(v,basename(stemp),-1);
    
    return 1;
}

/**
 * return path to file
 * @param v
 * @return 
 */
static PSInteger _exutil_dirname(HPSCRIPTVM v)
{
    const PSChar *str;
    PSInteger memsize;
    PSChar * stemp;
    
    ps_getstring(v,2,&str);
    memsize = (ps_getsize(v,2)+1)*sizeof(PSChar);
    stemp = ps_getscratchpad(v,memsize);
    memcpy(stemp, str, memsize);
    
    ps_pushstring(v,dirname(stemp),-1);
    
    return 1;
}


/**
 * netdb entry, resolve service by name
 * Not thread safe version (but at least c is cross platform)
 * @param [script] name 
 * @param [script] proto
 * @return [script] port number, or -1
 */
static PSInteger _exutil_getservbyname(HPSCRIPTVM v)
{
    const PSChar *name, *proto;
    PSInteger ret=EXFAIL;
    struct servent	*sptr;
    
    ps_getstring(v,2,&name);
    ps_getstring(v,3,&proto);
    
    if (NULL==(sptr = getservbyname(name, proto)))
    {
        NDRX_LOG(log_error, "Failed to lookup port %s/%s: %s", 
                name, proto, strerror(errno));
        EXFAIL_OUT(ret);
    }
    
    /* results are in network order */
    ret = ntohs(sptr->s_port);
    
out:
    
    ps_pushinteger(v, ret);

    return 1;
}

/**
 * Convert hex to long number
 * TODO: move to stdstring
 * @param [script] hex (without 0x suffix)
 * @return [script] converted integer
 */
static PSInteger _exutil_hex2int(HPSCRIPTVM v)
{
    const PSChar *hex;
    long l;
    PSInteger ret=EXFAIL;
    
    ps_getstring(v,2,&hex);
    
    sscanf(hex, "%lx", &l);
    
    ret = l;
    
out:
    
    ps_pushinteger(v, ret);

    return 1;
}

/**
 * Return random string
 * @param v
 * @param [script] len
 * @return [script] random string a-zA-Z0-9
 */
static PSInteger _exutil_rands(HPSCRIPTVM v)
{
    PSChar *stemp;
    PSInteger len;
    static int first = EXTRUE;
    char table[]="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    int i, tablen;
    
    if (first)
    {
        srand(time(NULL));
        first=EXFALSE;
    }
    
    ps_getinteger(v,2,&len);
    
    PSInteger memsize = (len+1)*sizeof(PSChar);
    stemp = ps_getscratchpad(v,memsize);
    
    memset(stemp, 0, (len+1)*sizeof(PSChar));
    
    tablen = strlen(table);
    for (i=0; i<len; i++)
    {
        stemp[i]= table[rand() % tablen];
    }
    
    ps_pushstring(v,stemp,-1);
    
    return 1;
}


/**
 * Parse clopt, with <block1> -- free args
 * @param v PS virtual machine
 * @param [script] clopt command line options
 * @param [script] optstring for getopt
 * @param [script] groups groups of argsX
 * @return {.args1[{.opt, .val}], .args2[{.opt, .val}] .freeargs[]}
 */
static PSInteger _exutil_parseclopt(HPSCRIPTVM v, int nr_groups)
{
    int ret = PS_OK;
    const PSChar *argv, *optstring, *optstring2;
    int c;
    char *token=NULL;
    PSInteger memsize;
    PSChar * stemp;
    char seps[] = NDRX_CMDLINE_SEP; /* to avoid c++ warning */
    char quotes[] = "\"";
    ndrx_growlist_t list;
    int index;
    int bufsz;
    int i;
    ps_getstring(v,2,&argv);
    ps_getstring(v,3,&optstring);
    
    if (nr_groups > 1)
    {
        ps_getstring(v,4,&optstring2);
    }
    
    bufsz = strlen(argv)+1;
    memsize = (bufsz)*sizeof(PSChar);
    stemp = ps_getscratchpad(v,memsize);
    
    ndrx_growlist_init(&list, 10, sizeof(char *));
    
    /* getopt require this: */
    if (EXSUCCEED!=ndrx_growlist_append(&list, (void *)&token))
    {
        NDRX_LOG(log_error, "ndrx_growlist_append() failed - oom?");
        ret=PS_ERROR;
        goto out;
    }
    
    /* load dummy name for parse... */
    NDRX_STRCPY_SAFE_DST(stemp, argv, bufsz);
    
    /* have strtok which respects quoted strings... */
    token = ndrx_strtokblk(stemp, seps, quotes);
    while( token != NULL )
    {
        /* Store the token up there... */
        if (EXSUCCEED!=ndrx_growlist_append(&list, (void *)&token))
        {
            NDRX_LOG(log_error, "ndrx_growlist_append() failed - oom?");
            ret=PS_ERROR;
            goto out;
        }
        
        /* Push tokens to array */
        token = ndrx_strtokblk( NULL, seps, quotes);
    }
    
#ifdef __GNU_LIBRARY__
    optind=0; /* reset lib, so that we can scan again. */
#else
    optind=1; /* reset lib, so that we can scan again. */
#endif
    
    /* return value table 
     * table stays on the stack to have return value.
     */
    ps_newtable(v);
    
    for (i=0; i<nr_groups; i++)
    {
        char key[16];
        const PSChar *p_opts = optstring;
        
        if (i==1)
        {
            p_opts = optstring2;
        }
        
        snprintf(key, sizeof(key), "args%d", i+1);
        
        /* set the hash key... */
        ps_pushstring(v, key,-1);
        /* the value is array */
        ps_newarray(v, 0);

        while ((c = getopt (list.maxindexused+1, (char **)list.mem, (const char *)p_opts)) != -1)
        {
            char opt[2] = {(char)c, EXEOS};
            
            /* Load parsed argument */
            ps_newtable(v);

            ps_pushstring(v,"opt",-1);
            ps_pushstring(v,opt,-1);
            ps_newslot(v,-3,PSFalse);

            if (NULL!=optarg)
            {
                ps_pushstring(v,"val",-1);
                ps_pushstring(v,optarg,-1);
                ps_newslot(v,-3,PSFalse);
            }

            ps_arrayappend(v,-2);
        }
        ps_newslot(v,-3,PSFalse);
    }
    
    /* set the hash key... */
    ps_pushstring(v,"freeargs",-1);
    
    /* value is array */
    ps_newarray(v, 0);
    
    for (index = optind; index < list.maxindexused+1; index++)
    {
        ps_pushstring(v,((char **)list.mem)[index],-1);
        ps_arrayappend(v,-2);
    }

    ps_newslot(v,-3,PSFalse);
    
out:
    
    ndrx_growlist_free(&list);

    if (PS_OK==ret)
    {
        return 1;
    }
    else
    {
        return ps_throwerror(v, "Failed to process");
    }
}

/**
 * Parse clopt with 1 group
 * @param [script] clopt first group -- free args
 * @param [script] optstring 
 * @return see _exutil_parseclopt
 */
static PSInteger _exutil_parseclopt1(HPSCRIPTVM v)
{
    return _exutil_parseclopt(v, 1);
}

/**
 * Parse clopt with 2 groups -a -r -g -s 
 * @param [script] clot first group -- second group -- free args
 * @param optstring1 opt string for first group
 * @param optstring2 opt string for for second group
 * @return see _exutil_parseclopt
 */
static PSInteger _exutil_parseclopt2(HPSCRIPTVM v)
{
    return _exutil_parseclopt(v, 2);
}

/**
 * Check confirm call
 * @param [script] Message
 * @return [script] TRUE (accept) / FALSE (rejected)
 */
static PSInteger _exutil_chk_confirm(HPSCRIPTVM v)
{
    const PSChar *str;
    PSInteger ret;
    
    ps_getstring(v,2,&str);
    
    ret = ndrx_chk_confirm((char *)str, EXFALSE);

    ps_pushinteger(v, ret);
    return 1;
}

/**
 * Print to stdout
 * @param [stdout] message
 * @return 1
 */
static PSInteger _exutil_print_stdout(HPSCRIPTVM v)
{
    const PSChar *str;
    
    ps_getstring(v,2,&str);
    
    fprintf(stdout, "%s", str);    
    return 1;
}

/**
 * Print to stderr
 * @param [stdout] message
 * @return 1
 */
static PSInteger _exutil_print_stderr(HPSCRIPTVM v)
{
    const PSChar *str;
    
    ps_getstring(v,2,&str);
    
    fprintf(stdout, "%s", str);    
    return 1;
}

#define _DECL_FUNC(name,nparams,pmask) {_SC(#name),_exutil_##name,nparams,pmask}
static PSRegFunction exutillib_funcs[]={
	_DECL_FUNC(getline,1,_SC(".s")),
        _DECL_FUNC(getcwd,1,_SC(".s")),
        _DECL_FUNC(getosname,1,_SC(".s")),
        _DECL_FUNC(getpoller,1,_SC(".s")),
        _DECL_FUNC(getwizardbase,1,_SC(".s")),
        _DECL_FUNC(userlog,2,_SC(".s")),
        _DECL_FUNC(mkdir,2,_SC(".s")),
        _DECL_FUNC(rmdir,2,_SC(".s")),
        _DECL_FUNC(unlink,2,_SC(".s")),
        _DECL_FUNC(fileexists,2,_SC(".s")),
        _DECL_FUNC(chmod,3,_SC(".ss")),
        _DECL_FUNC(basename,2,_SC(".s")),
        _DECL_FUNC(dirname,2,_SC(".s")),
        _DECL_FUNC(rands,2,_SC(".n")),
        _DECL_FUNC(parseclopt1,3,_SC(".ss")),
        _DECL_FUNC(parseclopt2,4,_SC(".sss")),
        _DECL_FUNC(getservbyname,3,_SC(".sss")),
        _DECL_FUNC(hex2int,2,_SC(".s")),
        _DECL_FUNC(chk_confirm,2,_SC(".s")),
        _DECL_FUNC(print_stdout,2,_SC(".s")),
        _DECL_FUNC(print_stderr,2,_SC(".s")),
	{0,0}
};
#undef _DECL_FUNC

PSInteger psstd_register_exutillib(HPSCRIPTVM v)
{
    PSInteger i=0;
    while(exutillib_funcs[i].name!=0)
    {
        ps_pushstring(v,exutillib_funcs[i].name,-1);
        ps_newclosure(v,exutillib_funcs[i].f,0);
        ps_setparamscheck(v,exutillib_funcs[i].nparamscheck,exutillib_funcs[i].typemask);
        ps_setnativeclosurename(v,-1,exutillib_funcs[i].name);
        ps_newslot(v,-3,PSFalse);
        i++;
    }
    return 1;
}
/* vim: set ts=4 sw=4 et smartindent: */
