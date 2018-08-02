/* 
 * @brief Linux Abstraction Layer (LAL)
 *
 * @file sys_linux.c
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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <nstdutil.h>
#include <limits.h>

#include <sys_unix.h>
#include <exregex.h>

#include <utlist.h>


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
expublic int ndrx_sys_is_process_running_procfs(pid_t pid, char *proc_name)
{
    char   proc_file[PATH_MAX];
    int     ret = EXFALSE;
    char    cmdline[2048] = {EXEOS};
    int len;
    int fd=EXFAIL;
    int i;
    /* Check for correctness - is it ndrxd */
    snprintf(proc_file, sizeof(proc_file), "/proc/%d/cmdline", pid);
    
    fd = open(proc_file, O_RDONLY);
    if (EXFAIL==fd)
    {
        NDRX_LOG(log_error, "Failed to open process file: [%s]: %s",
                proc_file, strerror(errno));
        goto out;
    }
    
    len = read(fd, cmdline, sizeof(cmdline));
    if (EXFAIL==len)
    {
        NDRX_LOG(log_error, "Failed to read from fd %d: [%s]: %s",
                fd, proc_file, strerror(errno));
        goto out;
    }
    
    len--;
    for (i=0; i<len; i++)
        if (0x00==cmdline[i])
            cmdline[i]=' ';
    
    
    /* compare process name */
    NDRX_LOG(6, "pid: %d, cmd line: [%s]", pid, cmdline);
    if (NULL!=strstr(cmdline, proc_name))
    {
        ret=EXTRUE;
    }


out:
    if (EXFAIL!=fd)
        close(fd);	

    return ret;
}

/**
 * Return list of message queues
 */
expublic string_list_t* ndrx_sys_mqueue_list_make_pl(char *qpath, int *return_status)
{
    return ndrx_sys_folder_list(qpath, return_status);
}

/**
 * Test the pid to contain regexp 
 * @param pid process id to test
 * @param p_re compiled regexp to test against
 * @return -1 failed, 0 - not matched, 1 - matched
 */
expublic int ndrx_sys_env_test(pid_t pid, regex_t *p_re)
{
    FILE *f = NULL;
    char path[256];
    int ret = EXSUCCEED;
    char *buf = NULL;
    size_t n = PATH_MAX;

    /* Alloc the buffer buffer */
    if (NULL==(buf=NDRX_MALLOC(n)))
    {
        NDRX_LOG(log_error, "Failed to malloc: %s", strerror(errno));
        EXFAIL_OUT(ret);
    }
    
    snprintf(path, sizeof(path), "/proc/%d/environ", (int)pid);
    
    if (NULL==(f=NDRX_FOPEN(path, "r")))
    {
        NDRX_LOG(log_error, "Failed to open: [%s]: %s", path, strerror(errno));
        EXFAIL_OUT(ret);
    }
    
    /* read environ 
     * TODO: use ssize_t getdelim(char **lineptr, size_t *n, int delim, FILE *stream);
     */
    while(EXFAIL!=getdelim(&buf, &n, 0x0, f)) 
    {
        /* test regexp on env */
        if (EXSUCCEED==ndrx_regexec(p_re, buf))
        {
            NDRX_LOG(log_debug, "Matched env [%s] for pid %d", buf, (int)pid);
            ret=EXTRUE;
            goto out;
        }
    }
    
out:
   
    if (NULL!=f)
    {
        NDRX_FCLOSE(f);
    }

    if (NULL!=buf)
    {
        NDRX_FREE(buf);
    }
    
    return ret;
}

/* vim: set ts=4 sw=4 et cindent: */
