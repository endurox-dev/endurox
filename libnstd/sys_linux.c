/* 
** Linux Abstraction Layer (LAL)
**
** @file sys_linux.c
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

#include <ndrstandard.h>
#include <ndebug.h>
#include <nstdutil.h>
#include <limits.h>

#include <sys_unix.h>
#include <sys/epoll.h>

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
public int ex_sys_is_process_running(pid_t pid, char *proc_name)
{
    char   proc_file[PATH_MAX];
    int     ret = FALSE;
    char    cmdline[2048] = {EOS};
    int len;
    int fd=FAIL;
    int i;
    /* Check for correctness - is it ndrxd */
    sprintf(proc_file, "/proc/%d/cmdline", pid);
    
    fd = open(proc_file, O_RDONLY);
    if (FAIL==fd)
    {
        NDRX_LOG(log_error, "Failed to open process file: [%s]: %s",
                proc_file, strerror(errno));
        goto out;
    }
    
    len = read(fd, cmdline, sizeof(cmdline));
    if (FAIL==len)
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
        ret=TRUE;
    }


out:
    if (FAIL!=fd)
        close(fd);	

    return ret;
}

/**
 * Return list of message queues
 */
public mq_list_t* ex_sys_mqueue_list_make(char *qpath, int *return_status)
{
    mq_list_t* ret = NULL;
    struct dirent **namelist;
    int n;
    mq_list_t* tmp;
    int len;
    
    *return_status = SUCCEED;
    
    n = scandir(qpath, &namelist, 0, alphasort);
    if (n < 0)
    {
        NDRX_LOG(log_error, "Failed to open queue directory: %s", 
                strerror(errno));
        goto exit_fail;
    }
    else 
    {
        while (n--)
        {
            if (0==strcmp(namelist[n]->d_name, ".") || 
                        0==strcmp(namelist[n]->d_name, ".."))
                continue;
            
            len = 1 /* / */ + strlen(namelist[n]->d_name) + 1 /* EOS */;
            
            if (NULL==(tmp = calloc(1, sizeof(mq_list_t))))
            {
                NDRX_LOG(log_always, "alloc of mq_list_t (%d) failed: %s", 
                        sizeof(mq_list_t), strerror(errno));
                
                
                goto exit_fail;
            }
            
            if (NULL==(tmp->qname = malloc(len)))
            {
                NDRX_LOG(log_always, "alloc of %d bytes failed: %s", 
                        len, strerror(errno));
                free(tmp);
                goto exit_fail;
            }
            
            
            strcpy(tmp->qname, "/");
            strcat(tmp->qname, namelist[n]->d_name);
            
            /* Add to LL */
            LL_APPEND(ret, tmp);
            
            free(namelist[n]);
        }
        free(namelist);
    }
    
    return ret;
    
exit_fail:

    *return_status = FAIL;

    if (NULL!=ret)
    {
        ex_sys_mqueue_list_free(ret);
        ret = NULL;
    }

    return ret;   
}
