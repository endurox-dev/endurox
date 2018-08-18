/**
 * @brief Regex common routines
 *
 * @file exregex.c
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

/*---------------------------Includes-----------------------------------*/
#include <ndrstandard.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <utlist.h>
#include <nstdutil.h>
#include <ini.h>
#include <inicfg.h>
#include <nerror.h>
#include <sys_unix.h>
#include <errno.h>
#include <nerror.h>
#include <userlog.h>
#include <regex.h>
#include <ndebug.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
        
/**
 * Compile the regular expression rules
 * @param p_re struct of regex
 * @param expr  Expression to compile
 * @return SUCCEED/FAIL
 */
expublic int ndrx_regcomp(regex_t *p_re, char *expr)
{
    int ret=EXSUCCEED;
    
    if (EXSUCCEED!=(ret=regcomp(p_re, expr, REG_EXTENDED | REG_NOSUB)))
    {
        char *errmsg;
        int errlen;
        char errbuf[2048];

        errlen = (int) regerror(ret, p_re, NULL, 0);
        errmsg = (char *) NDRX_MALLOC(errlen*sizeof(char));
        regerror(ret, p_re, errmsg, errlen);

        NDRX_LOG(log_error, "Failed to eventexpr [%s]: %s", expr, errbuf);

        NDRX_FREE(errmsg);
        ret=EXFAIL;
    }
    else
    {
        NDRX_LOG(log_debug, "eventexpr [%s] compiled OK", expr);
    }
    
    return ret;
}

/**
 * Match the regular expressions on data
 * @param p_re compiled regexp
 * @param data data to evaluate
 * @return SUCCEED is matched, any other not matched
 */
expublic int ndrx_regexec(regex_t *p_re, char *data)
{
    return regexec(p_re, data, (size_t) 0, NULL, 0);
}

/**
 * Free up regexp
 * @param p_re
 */
expublic void ndrx_regfree(regex_t *p_re)
{
    regfree(p_re);
}

/**
 * Copy string to dest by doing escape of posix regex symbols
 * @param dest dest buffer assume enought space it has
 * @param src source buffer to copy from
 * @param opt_start optional start symbol to copy to str, opt if 0x0 not used
 * @param opt_end optional end symbol to copy to str, opt if 0x0 not used
 * @param subst_from substitute from char (opt), if 0x0 not used
 * @param subst_to substitute to string (opt)
 */
expublic void ndrx_regasc_cpyesc(char *dest, char *src, 
        char opt_start, char opt_end, char subst_from, char *subst_to)
{
    char *p = dest;
    int i;
    int len = strlen(src);
    
    if (opt_start)
    {
        *p=opt_start;
        p++;
    }
    
    for (i=0; i<len; i++)
    {
        if (strchr(".^$*+?()[{\\|", src[i]))
        {
            *p='\'';
            p++;
            *p = src[i];
        }
        else if (subst_from==src[i])
        {
            while (EXEOS!=*subst_to)
            {
                *p = *subst_to;
                
                p++;
                subst_to++;
            }
        }
        else
        {
            *p = src[i];
            p++;
        }
    }
    
    if (opt_end)
    {
        *p=opt_end;
        p++;
    }
    
    *p=EXEOS;
}

/*
 * If we want to do some extract in future, take a look on: regmatch_t
 * See:
 *  - https://stackoverflow.com/questions/15238468/c-regular-expressions-extracting-the-actual-matches
 *  - https://stackoverflow.com/questions/17764422/regmatch-t-how-can-i-get-match-only
 * 
 */

/* vim: set ts=4 sw=4 et smartindent: */
