/* 
** AIX Abstraction Layer (AAL)
**
** @file sys_aix.c
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

/*---------------------------Includes-----------------------------------*/
#include <stdio.h>
#include <stdlib.h>


#include <unistd.h>
#include <stdarg.h>
#include <ctype.h>
#include <memory.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>
#include <pthread.h>
#include <string.h>
#include <dirent.h>

#include <procinfo.h>
#include <sys/types.h>


#include <ndrstandard.h>
#include <ndebug.h>
#include <nstdutil.h>
#include <limits.h>
#include <sys_unix.h>
#include <utlist.h>


/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Return list of message queues (actually it is list of named pipes
 * as work around for missing posix queue listing functions.
 */
expublic string_list_t* ndrx_sys_mqueue_list_make_pl(char *qpath, int *return_status)
{
    return ndrx_sys_folder_list(qpath, return_status);
}

/**
 * Get by process name by getprocss() aix call
 * @return 
 */
expublic char * ndrx_sys_get_proc_name_getprocs(void)
{
    static char out[PATH_MAX] = "unknown";
    struct procentry64 pr;
/*    struct fdsinfo64 f;*/
    char *p;
    int l;
    static int first = EXTRUE;
    
    if (first)
    {
        pid_t self = getpid();
        
        if (EXFAIL!=getprocs64(&pr, (int)sizeof(pr), NULL, 0, &self, 1))
        {
            p = pr.pi_comm;
            
            l = strlen(p);
            
            if (l>0 && '\n'==p[l-1])
            {
                p[l-1] = EXEOS;
                l--;
            }
            
            if (l>0 && '\r'==p[l-1])
            {
                p[l-1] = EXEOS;
                l--;
            }

            if (EXEOS!=*p)
            {
                strcpy(out, p);
            }
        }
        else
        {
            NDRX_LOG(log_error, "getprocs64 failed: %s",
                strerror(errno));
        }
        first = EXFALSE;
    }    
    return out;
}
