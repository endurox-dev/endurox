/**
 * @brief Commons for system abstractions
 *
 * @file sys_common.c
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
#include <regex.h>

#include "userlog.h"
#include "exregex.h"
#include "exparson.h"
#include "atmi_int.h"



/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
    
/*#define SYSCOMMON_ENABLE_DEBUG - causes locks in case of invalid config,
 *      due to recursive debug init */
#define MAX_ATFORKS         3
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

/** Function to run before fork */
exprivate void (*M_prepare[MAX_ATFORKS])(void) = {NULL, NULL, NULL}; 
/** Function to run after fork, by parent */
exprivate void (*M_parent[MAX_ATFORKS])(void) = {NULL, NULL, NULL};
/** Function to run after fork, by child */
exprivate void (*M_child[MAX_ATFORKS])(void) = {NULL, NULL, NULL};

/*---------------------------Prototypes---------------------------------*/


/**
 * Add item to string hash
 * @param h
 * @param str
 * @return SUCCEED/FAIL
 */
expublic int ndrx_string_hash_add(string_hash_t **h, char *str)
{
    int ret = EXSUCCEED;
    string_hash_t * tmp = NDRX_CALLOC(1, sizeof(string_hash_t));
    
    if (NULL==tmp)
    {
#ifdef SYSCOMMON_ENABLE_DEBUG
        NDRX_LOG(log_error, "alloc of string_hash_t (%d) failed", 
                sizeof(string_hash_t));
#endif
        EXFAIL_OUT(ret);
    }

    if (NULL==(tmp->str = strdup(str)))
    {
#ifdef SYSCOMMON_ENABLE_DEBUG
        NDRX_LOG(log_error, "strdup() failed: %s", strerror(errno));
#endif
        EXFAIL_OUT(ret);
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
expublic string_hash_t * ndrx_string_hash_get(string_hash_t *h, char *str)
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
expublic void ndrx_string_hash_free(string_hash_t *h)
{
    string_hash_t * r, *rt;
    /* safe iter over the list */
    EXHASH_ITER(hh, h, r, rt)
    {
        EXHASH_DEL(h, r);
        NDRX_FREE(r->str);
        NDRX_FREE(r);
    }
}

/**
 * Free the list of message queues
 */
expublic void ndrx_string_list_free(string_list_t* list)
{
    string_list_t *elt, *tmp;
    
    if (NULL!=list)
    {
        LL_FOREACH_SAFE(list,elt,tmp) 
        {
            LL_DELETE(list,elt);
            if (NULL!=elt->qname)
            {
                NDRX_FREE(elt->qname);
            }
            
            NDRX_FREE((char *)elt);
        }
    }
}

/**
 * Add element to string list
 * @param list List to append, can be ptr to NULL
 * @param string String element to add to list
 * @return SUCCEED/FAIL. If fail, list element will not be allocated
 */
expublic int ndrx_string_list_add(string_list_t**list, char *string)
{
    int ret = EXSUCCEED;
    string_list_t* tmp = NULL;
    
    if (NULL==(tmp = NDRX_CALLOC(1, sizeof(string_list_t))))
    {
        NDRX_LOG(log_error, "alloc of string_list_t (%d) failed", 
                sizeof(string_list_t));
        EXFAIL_OUT(ret);
    }
    
    /* Alloc the string down there */
    if (NULL==(tmp->qname = NDRX_MALLOC(strlen(string)+1)))
    {
        NDRX_LOG(log_error, "alloc of string_list_t qname (%d) failed: %s", 
               strlen(string)+1, strerror(errno));
        NDRX_FREE(tmp);
        EXFAIL_OUT(ret);
    }
    
    strcpy(tmp->qname, string);
    
    /*  Add the string to list finally */
    LL_APPEND(*list, tmp);
    
 out:
    return ret;
}

/**
 * Return PS output for the parent
 * assuming that next we will extract only pid and not any other infos
 * because for BSD the output will be slightly different than normal ps -ef or auxxw
 * @param ppid  Parent PID id
 * @return List of child processes or NULL.
 */
expublic string_list_t * ndrx_sys_ps_getchilds(pid_t ppid)
{
    /*
     * Free BSD: ps -jauxxw
     * Other Unix: ps -ef
     * compare third column with 
     */
    char cmd[128];
    FILE *fp=NULL;
    string_list_t* ret = NULL;
    pid_t pid;
    char path[PATH_MAX];
    int is_error = EXFALSE;
    
#ifdef EX_OS_FREEBSD
    snprintf(cmd, sizeof(cmd), "ps -jauxxw");
#else
    snprintf(cmd, sizeof(cmd), "ps -ef");
#endif
    
    fp = popen(cmd, "r");
    
    if (fp == NULL)
    {
        NDRX_LOG(log_warn, "failed to run command [%s]: %s", cmd, strerror(errno));
        goto out;
    }
    
    while (fgets(path, sizeof(path)-1, fp) != NULL)
    {
        if (EXSUCCEED==ndrx_proc_ppid_get_from_ps(path, &pid) && ppid == pid)
        {
            if (EXSUCCEED!=ndrx_string_list_add(&ret, path))
            {
                NDRX_LOG(log_error, "Failed to add [%s] to list of processes", path);
                is_error = EXTRUE;
                goto out;
            }
        }
    }
    
 out:
    /* close */
    if (fp!=NULL)
    {
        pclose(fp);
    }
 
    if (is_error)
    {
        ndrx_string_list_free(ret);
        ret = NULL;
    }

    return ret;
}
    

/**
 * List processes by filters
 * @param filter1 - match the string1
 * @param filter2 - match the string2
 * @param filter3 - match the string3
 * @param filter4 - match the string4
 * @param regex1  - match by regular expression (if any set)
 * @return String list of matched lines
 */
expublic string_list_t * ndrx_sys_ps_list(char *filter1, char *filter2, 
        char *filter3, char *filter4, char *regex1)
{
    FILE *fp=NULL;
    char cmd[128];
    char path[PATH_MAX];
    int ok;
    int i;
    string_list_t* tmp;
    string_list_t* ret = NULL;
    regex_t r1;
    int r1_alloc = EXFALSE;
    int is_error = EXFALSE;
    
#define MAX_FILTER      5
    char *filter[MAX_FILTER] = {filter1, filter2, filter3, filter4, regex1};
    
#ifdef EX_OS_FREEBSD
    snprintf(cmd, sizeof(cmd), "ps -auwwx");
#elif EX_OS_DARWIN
    /* we need full username instead of uid in output...*/
    snprintf(cmd, sizeof(cmd), "ps -je");
#else
    snprintf(cmd, sizeof(cmd), "ps -ef");
#endif
    
    if (EXEOS!=regex1[0])
    {
        if (EXSUCCEED!=ndrx_regcomp(&r1, regex1))
        {
            NDRX_LOG(log_error, "ndrx_sys_ps_list: Failed to compile regex1: [%s]", regex1);
            userlog("ndrx_sys_ps_list: Failed to compile regex1: [%s]", regex1);
            ret = NULL;
            goto out;
        }
        
        r1_alloc = EXTRUE;
    }
    
#ifdef SYSCOMMON_ENABLE_DEBUG
    NDRX_LOG(log_debug, "Listing processes [%s] f1=[%s] f2=[%s] f3=[%s] f4=[%s] r1=[%s]", 
            cmd, filter1, filter2, filter3, filter4, regex1);
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

        /*NDRX_LOG(log_debug, "Got line [%s]", path); - causes locks... */
        
        ok = 0;
        
        for (i = 0; i<MAX_FILTER; i++)
        {
            /* Do the regexp match.. */
            if (EXEOS!=filter[i][0] && filter[i]==regex1 
                    && EXSUCCEED==ndrx_regexec(&r1, path))
            {
                /* for example [/ ]cpmsrv\s */
                ok++;
            }
            else if (EXEOS!=filter[i][0] && strstr(path, filter[i]))
            {
             /*   NDRX_LOG(log_debug, "filter%d [%s] - ok", i, filter[i]); */
                ok++;
            }
            else if (EXEOS==filter[i][0])
            {
                /* NDRX_LOG(log_debug, "filter%d [%s] - ok", i, filter[i]); */
                ok++;
            }
            else
            {
                /* NDRX_LOG(log_debug, "filter%d [%s] - fail", i, filter[i]); */
            }
        }
        
        /* NDRX_LOG(log_debug, "Filters vote %d/%d", ok, MAX_FILTER); */
        
        if (MAX_FILTER==ok)
        {
            /* Remove trailing newline */
            ndrx_chomp(path);
            if (EXSUCCEED!=ndrx_string_list_add(&ret, path))
            {
                is_error = EXTRUE;
                goto out;
            }
        }
    }

out:
    /* close */
    if (fp!=NULL)
    {
        pclose(fp);
    }

    if (r1_alloc)
    {
        ndrx_regfree(&r1);
    }

    if (is_error)
    {
        ndrx_string_list_free(ret);
        ret=NULL;
    }

    return ret;
    
}

/**
 * Get current system username
 */
expublic char *ndrx_sys_get_cur_username(void)
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
 * Return hostname
 * @param out_hostname
 * @param bufsz
 * @return 
 */
expublic int ndrx_sys_get_hostname(char *out_hostname, long out_bufsz)
{
    int ret = EXSUCCEED;
    
    if (EXSUCCEED!=gethostname(out_hostname, out_bufsz))
    {
        userlog("Failed to get hostname: %s", strerror(errno));
        EXFAIL_OUT(ret);
    }
    
out:
    return ret;
}

/**
 * List the contents of the folder
 */
expublic string_list_t* ndrx_sys_folder_list(char *path, int *return_status)
{
    string_list_t* ret = NULL;
    struct dirent **namelist;
    int n;
    string_list_t* tmp;
    int len;
    
    *return_status = EXSUCCEED;
    
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
                NDRX_FREE(namelist[n]);
                continue;
            }
            
            len = 1 /* / */ + strlen(namelist[n]->d_name) + 1 /* EOS */;
            
            if (NULL==(tmp = NDRX_CALLOC(1, sizeof(string_list_t))))
            {
                /* standard logging might doing init right now */
#ifdef SYSCOMMON_ENABLE_DEBUG
                NDRX_LOG(log_error, "alloc of mq_list_t (%d) failed: %s", 
                        sizeof(string_list_t), strerror(errno));
#endif
                goto exit_fail;
            }
            
            if (NULL==(tmp->qname = NDRX_MALLOC(len)))
            {
                /* standard logging might doing init right now */
#ifdef SYSCOMMON_ENABLE_DEBUG
                NDRX_LOG(log_error,"alloc of %d bytes failed: %s", 
                        len, strerror(errno));
#endif
                NDRX_FREE(tmp);
                goto exit_fail;
            }
            
            
            strcpy(tmp->qname, "/");
            strcat(tmp->qname, namelist[n]->d_name);
            
            /* Add to LL */
            LL_APPEND(ret, tmp);
            
            NDRX_FREE(namelist[n]);
        }
        NDRX_FREE(namelist);
    }
    
    return ret;
    
exit_fail:

    *return_status = EXFAIL;

    if (NULL!=ret)
    {
        ndrx_string_list_free(ret);
        ret = NULL;
    }

    return ret;   
}

/**
 * Kill the list
 * @param list list of ps output processes to kill
 */
expublic void ndrx_proc_kill_list(string_list_t *list)
{
    string_list_t* elt = NULL;
    int signals[] = {SIGTERM, SIGKILL};
    int i;
    int max_signals = 2;
    int was_any = EXFALSE;
    pid_t pid;
    char *fn = "ndrx_proc_kill_list";
    NDRX_LOG(log_info, "%s enter-> %p", fn, list);
    
    for (i=0; i<max_signals; i++)
    {
        LL_FOREACH(list,elt)
        {
            if (EXSUCCEED==ndrx_proc_pid_get_from_ps(elt->qname, &pid))
            {
                 NDRX_LOG(log_error, "! killing  sig=%d "
                         "pid=[%d] (%s)", signals[i], pid, elt->qname);

                 if (EXSUCCEED!=kill(pid, signals[i]))
                 {
                     NDRX_LOG(log_error, "failed to kill with signal %d pid %d: %s",
                             signals[i], pid, strerror(errno));
                 }
                 else
                 {
                    was_any = EXTRUE;
                 }
            }    
        } /* for list entry */
    } /* for signals */
    
}

/**
 * Hmm we shall not kill up here. but just generate a list of childs
 * afterwards will kill one by one
 * 
 * @param list list to fill with all processes under then parent
 * @param pid PID of the parent
 * @return SUCCEED/FAIL
 */
expublic int ndrx_proc_children_get_recursive(string_list_t**list, pid_t pid)
{
    int ret = EXSUCCEED;
    string_list_t* elt = NULL;
    string_list_t* children = NULL;
    
    char *fn = "ndrx_get_childproc_recursive";
    NDRX_LOG(log_info, "%s enter-> list=%p, pid=%d", fn, list, pid);
    
    children = ndrx_sys_ps_getchilds(pid);
    
    LL_FOREACH(children,elt)
    {
        if (EXSUCCEED==ndrx_proc_pid_get_from_ps(elt->qname, &pid))
        {
            /* Step into, search for childs */
            if (EXSUCCEED!=ndrx_proc_children_get_recursive(list, pid))
            {
                EXFAIL_OUT(ret);
            }
            
            if (EXSUCCEED!=ndrx_string_list_add(list, elt->qname))
            {
                EXFAIL_OUT(ret);
            }
    
        }
    } /* For children... */
    
out:
    ndrx_string_list_free(children);
    return ret;
}

/**
 * Parse pid from PS output
 * @param psout
 * @param pid
 * @return 
 */
expublic int ndrx_proc_pid_get_from_ps(char *psout, pid_t *pid)
{
    char tmp[PATH_MAX+1];
    char *token;
    int ret = EXSUCCEED;
    
    NDRX_STRCPY_SAFE(tmp, psout);

    /* get the first token */
    if (NULL==(token = strtok(tmp, "\t ")))
    {
        NDRX_LOG(log_error, "missing username in ps -ef output")
        EXFAIL_OUT(ret);
    }

    /* get second token */
    token = strtok(NULL, "\t ");
    if (NULL==token)
    {
        NDRX_LOG(log_error, "missing pid in ps -ef output")
        EXFAIL_OUT(ret);
    }   
    else
    {
        *pid = atoi(token);
#ifdef SYSCOMMON_ENABLE_DEBUG
        NDRX_LOG(log_debug, "ndrx_get_pid_from_ps: Got %d as pid of [%s]", *pid, psout);
#endif
    }
    
out:
    return ret;
}

/**
 * Extract line output from command
 * @param cmd
 * @param buf
 * @param busz
 * @return 
 */
expublic int ndrx_proc_get_line(int line_no, char *cmd, char *buf, int bufsz)
{
    int ret = EXSUCCEED;
    FILE *fp=NULL;
    int line = 0;
    NDRX_LOG(log_debug, "%s: About to run: [%s]", __func__, cmd);
    
    fp = popen(cmd, "r");
    if (fp == NULL)
    {
#ifdef SYSCOMMON_ENABLE_DEBUG
        NDRX_LOG(log_warn, "failed to run command [%s]: %s", cmd, strerror(errno));
#endif
        EXFAIL_OUT(ret);
    }
    
    while (fgets(buf, bufsz, fp) != NULL)
    {
        line ++;
        
        if (line==line_no)
        {
            break;
        }
    }

out:

    /* close */
    if (fp!=NULL)
    {
        pclose(fp);
    }

    if (line!=line_no)
    {
        NDRX_LOG(log_error, "Extract lines: %d, but requested: %d", 
                line, line_no);
        ret=EXFAIL;
    }

    if (EXSUCCEED==ret)
    {
        ndrx_chomp(buf);
    }

    return ret;   
}

/**
 * Get child process from ps -ef or ps -jauxxw for bsd
 * @param psout ps string
 * @param pid   If succeed then PID is loaded
 * @return SUCCEED/FAIL
 */
expublic int ndrx_proc_ppid_get_from_ps(char *psout, pid_t *ppid)
{
    char tmp[PATH_MAX+1];
    char *token;
    int ret = EXSUCCEED;
    
    NDRX_STRCPY_SAFE(tmp, psout);

    /* get the first token */
    if (NULL==(token = strtok(tmp, "\t ")))
    {
        NDRX_LOG(log_error, "missing username in ps -ef output (1)")
        EXFAIL_OUT(ret);
    }

    /* get second token */
    token = strtok(NULL, "\t ");
    if (NULL==token)
    {
        NDRX_LOG(log_error, "missing pid in ps -ef output (2)")
        EXFAIL_OUT(ret);
    }
    
    /* get third token */
    token = strtok(NULL, "\t ");
    if (NULL==token)
    {
        NDRX_LOG(log_error, "missing pid in ps -ef output (3)")
        EXFAIL_OUT(ret);
    }
    else
    {
        *ppid = atoi(token);
#ifdef SYSCOMMON_ENABLE_DEBUG
        NDRX_LOG(log_debug, "Got %d as parent pid of [%s]", *ppid, psout);
#endif
    }
    
out:
    return ret;
}

/**
 * Get process information
 * WARNING! This generates SIGCHLD - try to avoid to use in user libraries...
 * @param pid
 * @param p_infos return structure
 * @return 
 */
expublic int ndrx_proc_get_infos(pid_t pid, ndrx_proc_info_t *p_infos)
{
    int ret = EXSUCCEED;
    char cmd[128];
    char line[PATH_MAX+1];
    long  meminfo[16];
    int toks;
/*
All unix:

$ ps -o rss,vsz -p 1
RSS  VSZ
132 5388

 * aix:
$ ps v 1
      PID    TTY STAT  TIME PGIN  SIZE   RSS   LIM  TSIZ   TRS %CPU %MEM COMMAND
        1      - A     0:38  298   708   208 32768    30    32  0.0  0.0 /etc/i

+
$ ps -o vsz -p 1
  VSZ
  708
*/
    
#ifdef EX_OS_AIX
    snprintf(cmd, sizeof(cmd), "ps v %d", pid);
    
    if (EXSUCCEED!=ndrx_proc_get_line(2, cmd, line, sizeof(line)))
    {
        NDRX_LOG(log_error, "Failed to get rss infos from  [%s]", cmd);
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_debug, "Parsing output: [%s]", line);
    
    toks = ndrx_tokens_extract(line, "%ld", (void *)meminfo, 
            sizeof(long), N_DIM(meminfo), 0, 15);
    
    if (toks<7)
    {
        NDRX_LOG(log_error, "Invalid tokens, expected at least 7, got %d", toks);
       EXFAIL_OUT(ret);
    }
    
    p_infos->rss = meminfo[6];
    
    snprintf(cmd, sizeof(cmd), "ps -o vsz -p %d", pid);
    
    if (EXSUCCEED!=ndrx_proc_get_line(2, cmd, line, sizeof(line)))
    {
        NDRX_LOG(log_error, "Failed to get rss infos from  [%s]", cmd);
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_debug, "Parsing output: [%s]", line);
    
    toks = ndrx_tokens_extract(line, "%ld", (void *)meminfo, 
            sizeof(long), N_DIM(meminfo), 0, 15);
    
    if (toks!=1)
    {
       NDRX_LOG(log_error, "Invalid tokens, expected at least 1, got %d", toks);
       EXFAIL_OUT(ret);
    }
    
    p_infos->vsz = meminfo[0];  
    
#else
    
    snprintf(cmd, sizeof(cmd), "ps -o rss,vsz -p%d", pid);
    
    if (EXSUCCEED!=ndrx_proc_get_line(2, cmd, line, sizeof(line)))
    {
        NDRX_LOG(log_error, "Failed to get rss/vsz infos from  [%s]", cmd);
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_debug, "Parsing output: [%s]", line);
    
    toks = ndrx_tokens_extract(line, "%ld", (void *)meminfo, 
            sizeof(long), N_DIM(meminfo), 0, 15);
    
    if (2!=toks)
    {
       NDRX_LOG(log_error, "Invalid tokens, expected 2, got %d", toks);
       EXFAIL_OUT(ret);
    }
    
    p_infos->rss = meminfo[0];
    p_infos->vsz = meminfo[1];
    
#endif
    
 
    NDRX_LOG(log_info, "extracted rss=%ld vsz=%ld", p_infos->rss, p_infos->vsz);
    
out:
    
    NDRX_LOG(log_debug, "%s: returns %d", __func__, ret);

    return ret;
}

/**
 * Test the regexp against the command output strings.
 * This will be used for aix/freebsd/macos/solaris
 * NOTE: This generate sign child due to fork.
 * @param fmt format string for command, must contain %d for pid
 * @param pid process id to test
 * @param p_re regular expression to match the output
 * @return EXFAIL (failed) / EXSUCCEED (0) - not matched, EXTRUE (1) - matched
 */
expublic int ndrx_sys_cmdout_test(char *fmt, pid_t pid, regex_t *p_re)
{
    char cmd[PATH_MAX];
    FILE *fp=NULL;
    char *buf = NULL;
    size_t n = PATH_MAX;
    int ret = EXSUCCEED;
    
    /* allocate buffer first */
    
    NDRX_MALLOC_OUT(buf, n, char);
    
    snprintf(cmd, sizeof(cmd), fmt, pid);
    
    fp = popen(cmd, "r");
    
    if (fp == NULL)
    {
        NDRX_LOG(log_warn, "failed to run command [%s]: %s", cmd, strerror(errno));
        goto out;
    }
    
    while (EXFAIL!=ndrx_getline(&buf, &n, fp))
    {
        /* test the output... */
        if (EXSUCCEED==ndrx_regexec(p_re, buf))
        {
            NDRX_LOG(log_debug, "Matched env [%s] for pid %d", buf, (int)pid);
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
 
    if (NULL!=buf)
    {
        NDRX_FREE(buf);
    }
 
    return ret;
}

/**
 * Print Enduro/X Banner
 */
expublic void ndrx_sys_banner(void)
{
    NDRX_BANNER;
}

/**
 * Prepare for forking
 */
expublic void ndrx_atfork_prepare(void)
{
    int i;
    
    for (i=MAX_ATFORKS-1; i>=0; i--)
    {
        if (NULL!=M_prepare[i])
        {
            M_prepare[i]();
        }
    }
}

/**
 * After fork, run parent runs
 */
expublic void ndrx_atfork_parent(void)
{
    int i;
    for (i=0; i<MAX_ATFORKS; i++)
    {
        if (NULL!=M_parent[i])
        {
            M_parent[i]();
        }
    }
}

/**
 * After fork, child runs
 */
expublic void ndrx_atfork_child(void)
{
    int i;
    for (i=0; i<MAX_ATFORKS; i++)
    {
        if (NULL!=M_child[i])
        {
            M_child[i]();
        }
    }
}

/**
 * If expecting to continue to use initialized Enduro/X after forking,
 * then fork shall be done with this function.
 * @param chldresume if 0 - no child resume (i.e. no resume of SystemV aux threads,
 *  which is not recommended for XATMI servers as admin thread of child 
 *  might consume parent's messages.) if set != 0 then resume aux threads,
 *  this is can be suitable for initialized clients which are doing forks
 *  for some job/connection handling.
 * @return for parent process child process pid is returned, for child 0 is
 *  returned.
 */
expublic pid_t ndrx_fork(void)
{
    pid_t ret;
    int err;
    
    ndrx_atfork_prepare();
    
    ret = fork();
    err = errno;
    
    if (0==ret)
    {
        ndrx_atfork_child();
    }
    else
    {
        ndrx_atfork_parent();
    }
    
    errno = err;
    
    return ret;
}

/**
 * If expecting to continue to use initialized Enduro/X after forking,
 * then fork shall be done with this function.
 * @param prepare callback to parent
 * @param parent parent after fork callback
 * @param child child after fork callack
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_atfork(void (*prepare)(void), void (*parent)(void),
       void (*child)(void))
{
    int i=0;
    int ret = EXSUCCEED;
    
    for (i=0;i<MAX_ATFORKS;i++)
    {
        if (NULL==M_prepare[i] && 
                NULL==M_parent[i] &&
                NULL==M_child[i]
            )
        {
            break;
        }
    }
    
    if (i==MAX_ATFORKS)
    {
        errno=ENOMEM;
        EXFAIL_OUT(ret);
    }
    
    M_prepare[i] = prepare;
    M_parent[i] = parent;
    M_child[i] = child;
    
out:
    return ret;
}

/**
 * Return user queues or semaphores
 * @param[out] list to init & grow
 * @param[in] Return user queues, otherwise return semaphores
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_sys_sysv_user_res(ndrx_growlist_t *list, int queues)
{
    char cmd[128];
    FILE *fp=NULL;
    char path[PATH_MAX];
    char linematchstr[PATH_MAX];
    int ret = EXSUCCEED;
    regex_t linematch;
    int linematch_comp = EXFALSE;
    
    /* init growlist */
    ndrx_growlist_init(list, 256, sizeof(int));
    
    if (queues)
    {
        NDRX_STRCPY_SAFE(cmd, "ipcs -q");
        /* output example (LINUX):

        $ ipcs -q
        ------ Message Queues --------
        key        msqid      owner      perms      used-bytes   messages    
        0x00000000 1190428672 user1      700        0            0           
        0x00000000 1159593985 user1      700        0            0           
        0x00000000 15368194   user1      760        0            0  
        ...
        
        (AIX):
        $ ipcs -s
        IPC status from /dev/mem as of Thu Oct 18 12:56:29 WET 2018
        T        ID     KEY        MODE       OWNER    GROUP
        Semaphores:
        s   2097152 0x58002281 --ra-ra-ra-     root   system
        s         1 0x44002281 --ra-ra-ra-     root   system
        s   1048578 0x7a0c584a --ra-------   zabbix   zabbix
        s         3 0x620000a3 --ra-r--r--     root   system
        
        (FREEBSD):
        Semaphores:
        T           ID          KEY MODE        OWNER    GROUP   
        s     24969217   1297266887 --rw-rw-r-- user1    user1   
        s     21430274   1297266911 --rw-rw-r-- user1    user1   
        s     21168131   1297266936 --rw-rw-r-- user1    user1   
        s     21168132   1297266983 --rw-rw-r-- user1    user1   
        s     21168133   1297266993 --rw-rw-r-- user1    user1   


        (HPUX):
        $ ipcs
        IPC status from /dev/kmem as of Thu Oct 18 12:59:33 2018
        T         ID     KEY        MODE        OWNER     GROUP
        Message Queues:
        q          0 0x3c1c080d -Rrw--w--w-      root      root
        q          1 0x3e1c080d --rw-r--r--      root      root
        q 1070596098 0x00000000 -Rrw-rw----      toor      toor
        
        (MACOS)
        Shared Memory: 
        T ID     KEY        MODE        OWNER GROUP 
        m 131071 1095910432 --rw-rw-rw- root kpl 
        m 131071 1627522010 --rw-rw---- kpl kpl 
        m 131071 1644299226 --rw-rw---- kpl kpl 
        m 262143 1661076442 --rw-rw---- kpl kpl 
        
        So from above we can say that MACOS/AIX/FREEBSD/HPUX have the same layout
        */
    }
    else
    {
        NDRX_STRCPY_SAFE(cmd, "ipcs -s");
        /* output example:
        $ ipcs -s

        ------ Semaphore Arrays --------
        key        semid      owner      perms      nsems     
        0x0052e2c1 0          user1      600        17        
        0x0052e2c2 32769      user1      600        17        
        0x0052e2c3 65538      user1      600        17        
        0x0052e2c4 98307      user1      600        17          
        ...
         */
    }
#ifdef EX_OS_LINUX
    snprintf(linematchstr, sizeof(linematchstr), "^0x[0-9a-fA-F]+\\s*[0-9]+\\s*%s\\s",
            ndrx_sys_get_cur_username());
#else
    snprintf(linematchstr, sizeof(linematchstr), "^.[ \\t\\r\\n\\v\\f]+[0-9]+[ \\t\\r\\n\\v\\f]+0[x0-9a-fA-F]*[ \\t\\r\\n\\v\\f]+.{11}[ \\t\\r\\n\\v\\f]+%s[ \\t\\r\\n\\v\\f]",
            ndrx_sys_get_cur_username());
#endif
    
    if (EXSUCCEED!=ndrx_regcomp(&linematch, linematchstr))
    {
        userlog("Failed to compile regexp: %s", linematch);
        NDRX_LOG(log_error, "Failed to compile regexp: %s", linematch);
        EXFAIL_OUT(ret);
    }
    else
    {
        linematch_comp = EXTRUE;
    }
    
    NDRX_LOG(log_debug, "Listing resources by: [%s]", cmd);
    
    fp = popen(cmd, "r");
    
    if (fp == NULL)
    {
        NDRX_LOG(log_warn, "failed to run command [%s]: %s", cmd, strerror(errno));
        goto out;
    }
    
    while (fgets(path, sizeof(path)-1, fp) != NULL)
    {        
        if (EXSUCCEED==ndrx_regexec(&linematch, path))
        {
            int id;
            int len = strlen(path);
            
            if (len > 0 && '\n' == path[len-1])
            {
                path[len-1]=EXEOS;
            }
            
            NDRX_LOG(log_debug, "Line matched: [%s]", path);
            
            /* extract second column... valid for Linux and Unix */
            if (1!=ndrx_tokens_extract(path, "%d", &id, sizeof(id), 1, 1, 1))
            {
                NDRX_LOG(log_error, "Failed to extract resource id from [%s]!",
                        path);
                userlog("Failed to extract resource id from [%s]!",
                        path);
                EXFAIL_OUT(ret);
            }
         
            NDRX_LOG(log_debug, "Extract id %d", id);
            
            /* Add resource to growlist */
            if (EXSUCCEED!=ndrx_growlist_append(list, (void *)&id))
            {
                NDRX_LOG(log_error, "Failed to add %d to growlist!", id);
                userlog("Failed to add %d to growlist!", id);
                EXFAIL_OUT(ret);
            }
        }
    }
    
 out:
    /* close */
    if (fp!=NULL)
    {
        pclose(fp);
    }
 
    if (EXSUCCEED!=ret)
    {
        ndrx_growlist_free(list);
    }
 
    if (linematch_comp)
    {
        ndrx_regfree(&linematch);
    }

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
