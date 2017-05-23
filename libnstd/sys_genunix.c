/* 
** Generic unix abstractions 
**
** @file sys_genunix.c
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
#include <time.h>

#include <unistd.h>
#include <stdarg.h>
#include <ctype.h>
#include <memory.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>
#include <pthread.h>
#include <string.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <nstdutil.h>
#include <limits.h>

#include <sys_unix.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/


/**
 * Checks weither process is running or not...
 * @param pid
 * @param proc_name
 * @return
 */
public int ndrx_sys_is_process_running_by_ps(pid_t pid, char *proc_name)
{
    /* CAUSES STALL ON AIX. */
    FILE *fp=NULL;
    char cmd[128];
    char path[PATH_MAX];
    int ret = FALSE;
    
    sprintf(cmd, "ps -p %d -o comm=", pid);
    
    NDRX_LOG(log_debug, "About to check pid: [%s]", cmd);
    
    /* Open the command for reading. */
    fp = popen(cmd, "r");
    if (fp == NULL)
    {
        NDRX_LOG(log_warn, "failed to run command [%s]: %s", cmd, strerror(errno));
        goto out;
    }
    
    /* Check the process name in output... */
    while (fgets(path, sizeof(path)-1, fp) != NULL)
    {
        if (strstr(path, proc_name))
        {
            ret=TRUE;
            goto out;
        }
    }

out:
    /* close */
    if (fp!=NULL)
    {
        pclose(fp);
    }

    NDRX_LOG(log_debug, "process %s status: %s", proc_name, 
            ret?"running":"not running");
    return ret;
}


/**
 * Checks weither process is running or not...
 * @param pid
 * @param proc_name
 * @return
 */
public int ndrx_sys_is_process_running_by_kill(pid_t pid, char *proc_name)
{
    int ret = FALSE;
    
    if (kill(pid, 0) == 0)
    {
        /* process is running or a zombie */
        ret = TRUE;
    }
    else if (errno == ESRCH)
    {
        /* no such process with the given pid is running */
        ret = FALSE;
    }
    else
    {
       NDRX_LOG(log_error, "Failed to test processes: %s", strerror(errno));
       ret = FALSE; /* some other error, assume process running... */ 
    }

    NDRX_LOG(log_debug, "process %s status: %s", proc_name, 
            ret?"running":"not running");
    return ret;
}

/**
 * Get the process name. No debug please, at it gets locked
 * because mostly it is firstly called from debug lib, if we call debug
 * again it will lock against semaphore.
 *
 * @param pid
 * @param proc_name
 * @return
 */
public char * ndrx_sys_get_proc_name_by_ps(void)
{
    static char out[PATH_MAX] = "unknown";
    FILE *fp=NULL;
    char path[PATH_MAX];
    char cmd[128] = {EOS};
    int ret = SUCCEED;
    static int first = TRUE;
    char *p = NULL;
    int err;
    int l;
    
    if (first)
    {
        sprintf(cmd, "ps -p %d -o comm=", getpid());
/* avoid recursive lookup by debug config
        NDRX_LOG(log_debug, "About to check pid: [%s]", cmd);
*/

        /* Open the command for reading. */
        fp = popen(cmd, "r");
        if (fp == NULL)
        {
            first = FALSE; /* avoid recursive lookup by debug lib*/
 /*           NDRX_LOG(log_warn, "failed to run command [%s]: %s", cmd, strerror(errno));*/
            goto out;
        }

        /* Check the process name in output... */
        if (fgets(path, sizeof(path)-1, fp) == NULL)
        {
            err = errno;
            ret=FAIL;
            goto out;
        }

        if (NULL==(p = strrchr(path, '/')))
        {
            p = path;
        }
out:
        /* close */
        if (fp != NULL)
        {
            pclose(fp);
        }

        if (SUCCEED!=ret)
        {
            first = FALSE;
/*            NDRX_LOG(log_error, "Failed to get proc name: %s", strerror(err));*/
        }
        else
        {
            l = strlen(p);
            
            if (l>0 && '\n'==p[l-1])
            {
                p[l-1] = EOS;
                l--;
            }
            
            if (l>0 && '\r'==p[l-1])
            {
                p[l-1] = EOS;
                l--;
            }

            if (EOS!=*p)
            {
                strcpy(out, p);
            }

            first = FALSE;

/*            NDRX_LOG(log_debug, "current process name [%s]", out);*/
        }
    }
    
    return out;
}

