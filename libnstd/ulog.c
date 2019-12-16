/**
 * @brief User log
 *
 * @file ulog.c
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
#include <ndrx_config.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdarg.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <nstdutil.h>
#include <sys_unix.h>
#include <userlog.h>
#include <atmi.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * API version of USERLOG
 */
expublic int userlog (char *data, ...)
{
    int ret=EXSUCCEED;
    /* TODO: Might need semaphore for first init... */
    static int first = 1;
    static char *out_f = NULL;
    static char *out_f_dflt = ".";
    FILE *output;
    char  pre[100];
    int fopened=0;
    struct timeval  time_val;
    char full_name[FILENAME_MAX] = {EXEOS};
    long ldate, ltime, lusec;
    int print_label = 0;
    pid_t pid;
    va_list ap;
    /* No need for contexting... */

    gettimeofday( &time_val, NULL );
    
    ndrx_get_dt_local(&ldate, &ltime, &lusec);
    
    if (first)
    {
        if (NULL==(out_f=getenv(CONF_NDRX_ULOG)))
        {
            print_label = 1;
            out_f=out_f_dflt;
        }

        /* get pid */
        pid = getpid();
        first = 0;
    }

    /* Format the full output file */
    if (NULL!=out_f)
    {
        snprintf(full_name, sizeof(full_name), "%s/ULOG.%06ld", out_f, ldate);
        
        if (print_label)
        {
            fprintf(stderr, "Logging to %s\n", full_name);
        }
    }

    /* if no file or failed to open, then use stderr as output */
    /* we cannot have fopen/fclose debug here, it will cause recursion */
    if (NULL==out_f || NULL==(output=fopen(full_name, "a")))
    {
        if (NULL!=out_f)
        {
            fprintf(stderr, "Failed to open [%s]\n", full_name);
        }
        output=stderr;
    }
    else
    {
        fopened=1;
    }
    
    snprintf(pre, sizeof(pre), "%05lu:%08ld:%06ld%02ld:%-12.12s:",
            (long)pid, ldate, ltime,
                    (long)time_val.tv_usec/10000, EX_PROGNAME);

    va_start(ap, data);
    fputs(pre, output);
    (void) vfprintf(output, data, ap);
    fputs("\n", output);
    va_end(ap);

    if (fopened)
    {
        fclose( output );
    }
    
out:
    return ret;
}

/**
 * Write the userlog message by const string
 * @param msg
 */
expublic int userlog_const (const char *msg)
{
    return userlog("%s", msg);
}

/* vim: set ts=4 sw=4 et smartindent: */
