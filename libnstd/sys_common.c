/* 
** Commons for system abstractions
**
** @file sys_common.c
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
#include <dirent.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <nstdutil.h>
#include <limits.h>
#include <exhash.h>
#include <sys_unix.h>

#include <utlist.h>
#include <pwd.h>

#include "userlog.h"



/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define SYSCOMMON_ENABLE_DEBUG
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/


/**
 * Add item to string hash
 * @param h
 * @param str
 * @return SUCCEED/FAIL
 */
public int ndrx_string_hash_add(string_hash_t **h, char *str)
{
    int ret = SUCCEED;
    string_hash_t * tmp = calloc(1, sizeof(string_hash_t));
    
    if (NULL==tmp)
    {
#ifdef SYSCOMMON_ENABLE_DEBUG
        NDRX_LOG(log_error, "alloc of string_hash_t (%d) failed", 
                sizeof(string_hash_t));
#endif
        FAIL_OUT(ret);
    }

    if (NULL==(tmp->str = strdup(str)))
    {
#ifdef SYSCOMMON_ENABLE_DEBUG
        NDRX_LOG(log_error, "strdup() failed: %s", strerror(errno));
#endif
        FAIL_OUT(ret);
    }
    
    /* Add stuff to hash finaly */
    EXHASH_ADD_KEYPTR( hh, (*h), tmp->str, strlen(tmp->str), tmp );
    
out:
    return ret;
}

/**
 * Search for string existance in hash
 * @param h hash handler
 * @param str string to search for
 * @return NULL not found/not NULL - found
 */
public string_hash_t * ndrx_string_hash_get(string_hash_t *h, char *str)
{
    string_hash_t * r = NULL;
    
    EXHASH_FIND_STR( h, str, r);
    
    return r;
}

/**
 * Free up the hash list
 * @param h
 * @return 
 */
public void ndrx_string_hash_free(string_hash_t *h)
{
    string_hash_t * r, *rt;
    /* safe iter over the list */
    EXHASH_ITER(hh, h, r, rt)
    {
        EXHASH_DEL(h, r);
        free(r->str);
        free(r);
    }
}

/**
 * Free the list of message queues
 */
public void ndrx_string_list_free(string_list_t* list)
{
    string_list_t *elt, *tmp;
    
    if (NULL!=list)
    {
        LL_FOREACH_SAFE(list,elt,tmp) 
        {
            LL_DELETE(list,elt);
            if (NULL!=elt->qname)
            {
                free(elt->qname);
            }
            
            free((char *)elt);
        }
    }
}

/**
 * Add element to string list
 * @param list
 * @param string
 * @return 
 */
public int ndrx_sys_string_list_add(string_list_t**list, char *string)
{
    int ret = SUCCEED;
    string_list_t* tmp = NULL;
    
    if (NULL==(tmp = calloc(1, sizeof(string_list_t))))
    {
#ifdef SYSCOMMON_ENABLE_DEBUG
        NDRX_LOG(log_error, "alloc of string_list_t (%d) failed", 
                sizeof(string_list_t));
#endif
        FAIL_OUT(ret);
    }
    
    /* Alloc the string down there */
    if (NULL==(tmp->qname = malloc(strlen(string)+1)))
    {
#ifdef SYSCOMMON_ENABLE_DEBUG
        NDRX_LOG(log_error, "alloc of string_list_t qname (%d) failed: %s", 
               strlen(string)+1, strerror(errno));
#endif
        FAIL_OUT(ret);
    }
    
    /*  Add the string to list finally */
    LL_APPEND(*list, tmp);
    
 out:
    return ret;
}


/**
 * List processes by filters
 * @param filter1
 * @param filter2
 * @param filter3
 * @return 
 */
public string_list_t * ndrx_sys_ps_list(char *filter1, char *filter2, char *filter3, char *filter4)
{
    FILE *fp=NULL;
    char cmd[128];
    char path[PATH_MAX];
    int ok;
    int i;
    string_list_t* tmp;
    string_list_t* ret = NULL;
    char *filter[4] = {filter1, filter2, filter3, filter4};
    
#ifdef EX_OS_FREEBSD
    sprintf(cmd, "ps");
#else
    sprintf(cmd, "ps -ef");
#endif
    
#ifdef SYSCOMMON_ENABLE_DEBUG
    NDRX_LOG(log_debug, "Listing processes [%s] f1=[%s] f2=[%s] f3=[%s] f4=[%s]", 
            cmd, filter1, filter2, filter3, filter4);
#endif
    
    /* Open the command for reading. */
    fp = popen(cmd, "r");
    if (fp == NULL)
    {
#ifdef SYSCOMMON_ENABLE_DEBUG
        NDRX_LOG(log_warn, "failed to run command [%s]: %s", cmd, strerror(errno));
#endif
        goto out;
    }
    
    /* Check the process name in output... */
    while (fgets(path, sizeof(path)-1, fp) != NULL)
    {
     /*   NDRX_LOG(log_debug, "Got line [%s]", path); - causes locks... */
        ok = 0;
        
        for (i = 0; i<4; i++)
        {
            if (EOS!=filter[i][0] && strstr(path, filter[i]))
            {
                /* NDRX_LOG(log_debug, "filter%d [%s] - ok", i, filter[i]); */
                ok++;
            }
            else if (EOS==filter[i][0])
            {
                /* NDRX_LOG(log_debug, "filter%d [%s] - ok", i, filter[i]); */
                ok++;
            }
            else
            {
                /* NDRX_LOG(log_debug, "filter%d [%s] - fail", i, filter[i]); */
            }
        }
        
        if (4==ok)
        {
            if (NULL==(tmp = calloc(1, sizeof(string_list_t))))
            {
                
                /* close */
                if (fp!=NULL)
                {
                    fp = NULL;
                    pclose(fp);
                }
#ifdef SYSCOMMON_ENABLE_DEBUG
                NDRX_LOG(log_always, "alloc of string_list_t (%d) failed: %s", 
                        sizeof(string_list_t), strerror(errno));
#endif
                ndrx_string_list_free(ret);
                ret = NULL;
                goto out;
            }
            
            if (NULL==(tmp->qname = malloc(strlen(path)+1)))
            {
                /* close */
                if (fp!=NULL)
                {
                    fp = NULL;
                    pclose(fp);
                }
#ifdef SYSCOMMON_ENABLE_DEBUG
                NDRX_LOG(log_always, "alloc of %d bytes failed: %s", 
                        strlen(path)+1, strerror(errno));
#endif
                free(tmp);
                ndrx_string_list_free(ret);
                ret =  NULL;
                goto out;
            }
            
            strcpy(tmp->qname, path);
            
            LL_APPEND(ret, tmp);
        }
    }

out:
    /* close */
    if (fp!=NULL)
    {
        pclose(fp);
    }

    return ret;
    
}

/**
 * Get current system username
 */
public char *ndrx_sys_get_cur_username(void)
{
    uid_t uid = geteuid();
    struct passwd *pw = getpwuid(uid);
    if (pw)
    {
        return pw->pw_name;
    }

    return "";
}

/**
 * List the contents of the folder
 */
public string_list_t* ndrx_sys_folder_list(char *path, int *return_status)
{
    string_list_t* ret = NULL;
    struct dirent **namelist;
    int n;
    string_list_t* tmp;
    int len;
    
    *return_status = SUCCEED;
    
    n = scandir(path, &namelist, 0, alphasort);
    if (n < 0)
    {
        /* standard logging might doing init right now */
#ifdef SYSCOMMON_ENABLE_DEBUG
        NDRX_LOG(log_error, "Failed to open queue directory [%s]: %s", 
                path, strerror(errno));
#endif
        goto exit_fail;
    }
    else 
    {
        while (n--)
        {
            if (0==strcmp(namelist[n]->d_name, ".") || 
                        0==strcmp(namelist[n]->d_name, ".."))
            {
                free(namelist[n]);
                continue;
            }
            
            len = 1 /* / */ + strlen(namelist[n]->d_name) + 1 /* EOS */;
            
            if (NULL==(tmp = calloc(1, sizeof(string_list_t))))
            {
                /* standard logging might doing init right now */
#ifdef SYSCOMMON_ENABLE_DEBUG
                NDRX_LOG(log_error, "alloc of mq_list_t (%d) failed: %s", 
                        sizeof(string_list_t), strerror(errno));
#endif
                goto exit_fail;
            }
            
            if (NULL==(tmp->qname = malloc(len)))
            {
                /* standard logging might doing init right now */
#ifdef SYSCOMMON_ENABLE_DEBUG
                NDRX_LOG(log_error,"alloc of %d bytes failed: %s", 
                        len, strerror(errno));
#endif
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
        ndrx_string_list_free(ret);
        ret = NULL;
    }

    return ret;   
}


