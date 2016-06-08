/* 
** User log
**
** @file ulog.c
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
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdarg.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <atmi.h>
#include <nstdutil.h>
#include <sys_unix.h>
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
public int userlog (char *data, ...)
{
    int ret=SUCCEED;
    int     interval = 20;
    long    size = 0;
    char* msg_buf;
    long msg_len =0;
    int first = 1;
    char *out_f = NULL;
    FILE *output;
    char  pre[100];
    int fopened=0;
    struct timeval  time_val;
    struct timezone time_zone;
    char full_name[FILENAME_MAX] = {EOS};
    long ldate, ltime;
    pid_t pid;
    va_list ap;

    gettimeofday( &time_val, NULL );
    
    ndrx_get_dt_local(&ldate, &ltime);
    
    if (first)
    {
        if (NULL==(out_f=getenv(CONF_NDRX_ULOG)))
        {
            NDRX_LOG(log_warn, "%s not set!", CONF_NDRX_ULOG);
        }

        /* get pid */
        pid = getpid();
    }

    /* Format the full output file */
    if (NULL!=out_f)
    {
        sprintf(full_name, "%s/ULOG.%06ld", out_f, ldate);
    }

    /* if no file or failed to open, then use stderr as output */
    if (NULL==out_f || NULL==(output=fopen(full_name, "a")))
    {
        if (NULL!=out_f)
        {
            NDRX_LOG(log_warn, "Failed to open [%s]", full_name);
        }
        output=stderr;
    }
    else
    {
        fopened=1;
    }
    
    sprintf(pre, "%5ld:%08ld:%06ld%02ld:%-12.12s:",
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
