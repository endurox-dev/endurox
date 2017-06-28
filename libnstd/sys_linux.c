/* 
** Linux Abstraction Layer (LAL)
**
** @file sys_linux.c
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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

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
expublic string_list_t* ndrx_sys_mqueue_list_make(char *qpath, int *return_status)
{
    return ndrx_sys_folder_list(qpath, return_status);
}
