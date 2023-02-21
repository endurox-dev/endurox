/**
 * @brief Enduro/X Standard Configuration string handler
 *
 * @file stdcfgstr.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2023, Mavimax, Ltd. All Rights Reserved.
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
#include <ndrstandard.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "ndebug.h"
#include "userlog.h"
#include <errno.h>
#include <nstdutil.h>
#include <utlist.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Tokenize settings string. Syntax is following:
 * setting1, setting2 setting3\tsetting=4\t,,,setting="HELLO", settingA="=HELLO",settingB"=CCC"
 * The parser does not recognize is the equal sign in the quotes or outside.
 * We assume that our settings does not contain equal sign.
 * We assume that after equal sign of the block, value follows.
 * @param input input string to parse
 * @param parsed 
 */
expublic int ndrx_stdcfgstr_parse(char *input, ndrx_stdcfgstr_t** parsed)
{
    int ret = EXSUCCEED;
    int i;
    char *tok;
    char *str = NDRX_STRDUP(input);
    char *p;
    ndrx_stdcfgstr_t *list =NULL, *el=NULL;
    
#define SEPS     " \t\n,"
#define BLOCKS     "'\""
    
    if (NULL==str)
    {
        userlog("Failed to strdup!");
        EXFAIL_OUT(ret);
    }
    
    *parsed = NULL;
    
    for (tok = ndrx_strtokblk ( str, SEPS, BLOCKS), i=0; 
                NULL!=tok; 
                tok = ndrx_strtokblk (NULL, SEPS, BLOCKS), i++)
    {
        p = strchr(tok, '=');
        
        el = NDRX_FPMALLOC(sizeof(ndrx_stdcfgstr_t), 0);
        
        if (NULL==el)
        {
            userlog("Failed to fpmalloc %d bytes!", sizeof(ndrx_stdcfgstr_t));
            EXFAIL_OUT(ret);
        }
        
        if (NULL!=p)
        {
            *p=EXEOS;
            p++;
        }
        
        NDRX_STRCPY_SAFE(el->key, tok);
        el->value=NULL;
        
        if (NULL!=p)
        {
            size_t bufsz = strlen(p)+1;
            el->value = NDRX_FPMALLOC(bufsz, 0);
            
            if (NULL==el->value)
            {
                userlog("Failed to fpmalloc %d bytes!", bufsz);
                EXFAIL_OUT(ret);
            }
            
            strcpy(el->value, p);
        }
        
        DL_APPEND(list, el);
        el=NULL;
    }
    
out:
    
    if (EXSUCCEED!=ret)
    {
        if (el!=NULL)
        {
            if (el->value!=NULL)
            {
                NDRX_FPFREE(el->value);
            }
            NDRX_FPFREE(el);
        }
    }

    if (NULL!=str)
    {
        NDRX_FREE(str);
    }

    if (EXSUCCEED==ret)
    {
        *parsed=list;
    }
    else if (NULL!=list)
    {
        ndrx_stdcfgstr_free(list);
    }
    
    return ret;
}

/**
 * Delete memory used by settings parser
 * @param stdcfg parsed settings list
 */
expublic void ndrx_stdcfgstr_free(ndrx_stdcfgstr_t* stdcfg)
{
    ndrx_stdcfgstr_t *el, *elt;
    
    DL_FOREACH_SAFE(stdcfg, el, elt)
    {
        DL_DELETE(stdcfg, el);
        
        if (el->value!=NULL)
        {
            NDRX_FPFREE(el->value);
        }
        
        NDRX_FPFREE(el);
    }
    
}
        
/* vim: set ts=4 sw=4 et smartindent: */
