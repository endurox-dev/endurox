/**
 * @brief Create new environment for process.
 *
 * @file newenv.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * AGPL or Mavimax's license for commercial use.
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <memory.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <utlist.h>
#include <unistd.h>

#include <ndrstandard.h>
#include <ndrxd.h>
#include <atmi_int.h>
#include <nstopwatch.h>

#include <ndebug.h>
#include <cmd_processor.h>
#include <signal.h>

#include "userlog.h"


/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Strip whitespace from string.
 * @param pp
 */
exprivate char * strip_whitespace(char *pp)
{
    while (' '==pp[0] || '\t'==pp[0])
    {
        pp++;
    }
    
    return pp;
}

/**
 * Build environment
 * @param p_pm
 * @return 
 */
expublic int ndrx_load_new_env(char *file)
{
    int ret=EXSUCCEED;
    FILE *f;
    char line [8192];
    char *p;
    char *e;
    long line_no=0;
    int len;
    
    /* Just open the file, and load new env.... */
    
    if (NULL==(f=NDRX_FOPEN(file, "r")))
    {
        NDRX_LOG(log_error, "Failed to open environment override file [%s]:%s",
                            file, strerror(errno));
        EXFAIL_OUT(ret);
    }
   
    while ( fgets ( line, sizeof(line), f ) != NULL ) /* read a line */
    {
        p = line;
        line_no++;
        
        /* strip trailing newline */
        len = strlen(p);
        if (len > 0 && '\n'==p[len-1])
        {
            p[len-1]=EXEOS;
        }
        
        NDRX_LOG(log_debug, "%d:env over: [%s]", line_no, line);
        
        
        /* strip leading white space */
        p = strip_whitespace(p);
        
        /* Check isn't it comment */
        if ('#' == line[0] || EXEOS == line[0])
        {
            continue;
        }
        
        if (0==strncmp(line, "export ", 7) || 0==strncmp(line, "export\t", 7))
        {
            p+=7;
        }
        
        /* strip leading white space (one more time) */
        p = strip_whitespace(p);
        
        /* now find equal sign... */
        e = strchr(p, '=');
        if (NULL==e)
        {
            NDRX_LOG(log_error, "Error on line %d: No equal "
                    "sign found in [%s]", line_no, p);
            EXFAIL_OUT(ret);
        }
        
        /* we are OK, we have key & value */
        *e=EXEOS;
        e++; /* << This is value and p is key */
        NDRX_LOG(log_debug, "Key: [%s], value: [%s]", p, e);
        
        if (EXSUCCEED!=setenv(p, e, EXTRUE))
        {
            NDRX_LOG(log_error, "Failed to set env: %s", strerror(errno));
            EXFAIL_OUT(ret);
        }
    }
        
out:

    if (NULL!=f)
        NDRX_FCLOSE(f);

    return ret;
}
/* vim: set ts=4 sw=4 et smartindent: */
