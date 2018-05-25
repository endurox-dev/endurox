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
expublic int ndrx_sys_is_process_running_by_ps(pid_t pid, char *proc_name)
{
    /* CAUSES STALL ON AIX. */
    FILE *fp=NULL;
    char cmd[128];
    char path[PATH_MAX];
    int ret = EXFALSE;
    
    snprintf(cmd, sizeof(cmd), "ps -p %d -o comm=", pid);
    
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
            ret=EXTRUE;
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
expublic int ndrx_sys_is_process_running_by_kill(pid_t pid, char *proc_name)
{
    int ret = EXFALSE;
    
    if (kill(pid, 0) == 0)
    {
        /* process is running or a zombie */
        ret = EXTRUE;
    }
    else if (errno == ESRCH)
    {
        /* no such process with the given pid is running */
        ret = EXFALSE;
    }
    else
    {
       NDRX_LOG(log_error, "Failed to test processes: %s", strerror(errno));
       ret = EXFALSE; /* some other error, assume process running... */ 
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
expublic char * ndrx_sys_get_proc_name_by_ps(void)
{
    static char out[PATH_MAX] = "unknown";
    FILE *fp=NULL;
    char path[PATH_MAX];
    char cmd[128] = {EXEOS};
    int ret = EXSUCCEED;
    static int first = EXTRUE;
    char *p = NULL;
    int err;
    int l;
    
    if (first)
    {
        snprintf(cmd, sizeof(cmd), "ps -p %d -o comm=", getpid());
/* avoid recursive lookup by debug config
        NDRX_LOG(log_debug, "About to check pid: [%s]", cmd);
*/

        /* Open the command for reading. */
        fp = popen(cmd, "r");
        if (fp == NULL)
        {
            first = EXFALSE; /* avoid recursive lookup by debug lib*/
 /*           NDRX_LOG(log_warn, "failed to run command [%s]: %s", cmd, strerror(errno));*/
            goto out;
        }

        /* Check the process name in output... */
        if (fgets(path, sizeof(path)-1, fp) == NULL)
        {
            err = errno;
            ret=EXFAIL;
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

        if (EXSUCCEED!=ret)
        {
            first = EXFALSE;
/*            NDRX_LOG(log_error, "Failed to get proc name: %s", strerror(err));*/
        }
        else
        {
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

            while ('/'==p[0])
            {
               p++;
            }


            if (EXEOS!=*p)
            {
                NDRX_STRCPY_SAFE(out, p);
            }

            first = EXFALSE;

/*            NDRX_LOG(log_debug, "current process name [%s]", out);*/
        }
    }
    
    return out;
}

/**
 * Return list of message queues (actually it is list of named pipes
 * as work around for missing posix queue listing functions.
 * For emulated message queue
 */
expublic string_list_t* ndrx_sys_mqueue_list_make_emq(char *qpath, int *return_status)
{
    return ndrx_sys_folder_list(qpath, return_status);
}

/**
 * Test the regexp against the command output strings.
 * This will be used for aix/freebsd/macos/solaris
 * @param fmt format string for command, must contain %d for pid
 * @param pid process id to test
 * @param p_re regular expression to match the output
 * @return EXFAIL (failed) / EXSUCCEED (0) - not matched, EXTRUE (1) - matched
 */
expublic int ndrx_sys_cmdout_test(char *fmt, pid_t pid, regex_t *p_re)
{
    /* popen the process and match th output lines to the format */
    return EXFAIL;
}

/**
 * Enduro/X Cross platform getline version (system version, more close to GNU)
 * @param lineptr must be pre-allocated (for Macos will use fgets on this buffer)
 * @param n buffer size (ptr to)
 * @param stream file to read from
 * @return number bytes read for Macos will return just 1 or -1
 */
expublic ssize_t ndrx_sys_getline(char **lineptr, size_t *n, FILE *stream)
{
    return EXFAIL;
}

