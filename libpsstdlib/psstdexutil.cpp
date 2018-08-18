/**
 * @brief Enduro/X standard utilities
 *
 * @file psstdexutil.cpp
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


extern const char ndrx_G_resource_WizardBase[];

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
            sprintf(err, "chmod failed: %d:%s", 
                    errno, strerror(errno));
            return ps_throwerror(v,err);
        }
        
        
        return 1;
    }
    return 0;
}


//Write user log message
//@param msg    Message
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
                sprintf(err, "mkdir failed: %d:%s", 
                        errno, strerror(errno));
                return ps_throwerror(v,err);
            }
        }
        
        return 1;
    }
    return 0;
}

#define _DECL_FUNC(name,nparams,pmask) {_SC(#name),_exutil_##name,nparams,pmask}
static PSRegFunction exutillib_funcs[]={
	_DECL_FUNC(getline,1,_SC(".s")),
        _DECL_FUNC(getcwd,1,_SC(".s")),
        _DECL_FUNC(getosname,1,_SC(".s")),
        _DECL_FUNC(getwizardbase,1,_SC(".s")),
        _DECL_FUNC(userlog,2,_SC(".s")),
        _DECL_FUNC(mkdir,2,_SC(".s")),
        _DECL_FUNC(fileexists,2,_SC(".s")),
        _DECL_FUNC(chmod,3,_SC(".ss")),
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
