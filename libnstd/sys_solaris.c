/* 
 * @brief Solaris Abstraction Layer (SAL)
 *
 * @file sys_solaris.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or ATR Baltic's license for commercial use.
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
#include <dirent.h>
#include <stdarg.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <nstdutil.h>
#include <limits.h>

#include <sys_mqueue.h>
#include <sys_unix.h>

#include <utlist.h>


/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define SOL_RND_SLEEP	10000 /* 0.01 sec */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Return the list of queues (build the list according to /tmp/.MQD files.
 * e.g ".MQPn00b,srv,admin,atmi.sv1,123,2229" translates as
 * "/n00b,srv,admin,atmi.sv1,123,2229")
 * the qpath must point to /tmp
 */
expublic string_list_t* ndrx_sys_mqueue_list_make_pl(char *qpath, int *return_status)
{
    string_list_t* ret = NULL;
    struct dirent **namelist;
    int n;
    string_list_t* tmp;
    int len;
    
    *return_status = EXSUCCEED;
    
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
                        0==strcmp(namelist[n]->d_name, "..") ||
                        0!=strncmp(namelist[n]->d_name, ".MQP", 4))
            {
                NDRX_FREE(namelist[n]);
                continue;
            }
            
            len = strlen(namelist[n]->d_name) -3 /*.MQP*/ + 1 /* EOS */;
            
            if (NULL==(tmp = NDRX_CALLOC(1, sizeof(string_list_t))))
            {
                NDRX_LOG(log_always, "alloc of string_list_t (%d) failed: %s", 
                        sizeof(string_list_t), strerror(errno));
                
                
                goto exit_fail;
            }
            
            if (NULL==(tmp->qname = NDRX_MALLOC(len)))
            {
                NDRX_LOG(log_always, "alloc of %d bytes failed: %s", 
                        len, strerror(errno));
                NDRX_FREE(tmp);
                goto exit_fail;
            }
            
            strcpy(tmp->qname, "/");
            strcat(tmp->qname, namelist[n]->d_name+4); /* strip off .MQP */
            
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
 * Wrapper for solaris bugfix Bug #128
 * On undocumented error EBUSY, retry the call. Seems to help.
 */
expublic inline int sol_mq_close(mqd_t mqdes)
{
    int ret;
    int err;

    while (EXSUCCEED!=(ret = mq_close(mqdes)) && (err=errno)==EBUSY)
    {
/*	NDRX_LOG(log_warn, "%s: got EBUSY - restarting call...", __func__);*/
        usleep(SOL_RND_SLEEP);
    }

    errno = err;
    return ret;	
}

/**
 * Wrapper for solaris bugfix Bug #128
 * On undocumented error EBUSY, retry the call. Seems to help.
 */
expublic inline int  sol_mq_getattr(mqd_t mqdes, struct mq_attr * attr)
{
    int ret;
    int err;

    while (EXSUCCEED!=(ret = mq_getattr(mqdes, attr)) && ((err=errno)==EBUSY))
    {
    /*	NDRX_LOG(log_warn, "%s: got EBUSY - restarting call...", __func__); */
            usleep(SOL_RND_SLEEP);
    }
    errno = err;
    return ret;
}

/**
 * Wrapper for solaris bugfix Bug #128
 * On undocumented error EBUSY, retry the call. Seems to help.
 */
expublic inline int sol_mq_notify(mqd_t mqdes, struct sigevent * sevp)
{
    int ret;
    int err;

    NDRX_LOG(log_warn, "%s: mqdes=%d", __func__, mqdes);
    while (EXSUCCEED!=(ret =mq_notify(mqdes, sevp)) && ((err=errno)==EBUSY))
    {
            NDRX_LOG(log_warn, "%s: got EBUSY - restarting call...", __func__);
            usleep(SOL_RND_SLEEP);
    }
    errno =err;
    return ret;
}

/**
 * Wrapper for solaris bugfix Bug #128
 * On undocumented error EBUSY, retry the call. Seems to help.
 */
expublic inline mqd_t   sol_mq_open(char *name, int oflag, mode_t mode, struct mq_attr *attr)
{
    mqd_t  ret;
    int err;

    while (EXFAIL==(int)(ret = mq_open(name, oflag, mode, attr)) && 
            ((err=errno)==EBUSY))
    {
            usleep(SOL_RND_SLEEP);
    }

    errno = err;
    return ret;	
}

/**
 * Wrapper for solaris bugfix Bug #128
 * On undocumented error EBUSY, retry the call. Seems to help.
 */
expublic inline ssize_t sol_mq_receive(mqd_t mqdes, char *msg_ptr, size_t msg_len, 
				unsigned int *msg_prio)
{
    ssize_t ret;
    int err;

    while (EXFAIL==(ret =mq_receive(mqdes, msg_ptr, msg_len, msg_prio)) && 
            ((err=errno)==EBUSY))
    {
/*	NDRX_LOG(log_warn, "%s: got EBUSY - restarting call...", __func__); */
        usleep(SOL_RND_SLEEP);
    }
    errno = err;
    return ret;
}

/**
 * Wrapper for solaris bugfix Bug #128
 * On undocumented error EBUSY, retry the call. Seems to help.
 */
expublic inline int sol_mq_send(mqd_t mqdes, char *msg_ptr, size_t msg_len,
                    unsigned int msg_prio)
{
    int ret;
    int err;
	
    while (EXSUCCEED!=(ret =mq_send(mqdes, msg_ptr, msg_len, msg_prio)) &&
            ((err=errno)==EBUSY))
    {
/*	NDRX_LOG(log_warn, "%s: got EBUSY - restarting call...", __func__); */
        usleep(SOL_RND_SLEEP);
    }
    errno = err;
    return ret;
}

/**
 * Wrapper for solaris bugfix Bug #128
 * On undocumented error EBUSY, retry the call. Seems to help.
 */
expublic inline int sol_mq_setattr(mqd_t mqdes,
                       struct mq_attr * newattr,
                       struct mq_attr * oldattr)
{
    int ret;
    int err;

    while (EXSUCCEED!=(ret =mq_setattr(mqdes, newattr, oldattr)) &&
            ((err=errno)==EBUSY))
    {
/*	NDRX_LOG(log_warn, "%s: got EBUSY - restarting call...", __func__); */
	usleep(SOL_RND_SLEEP);
    }
    errno = err;
    return ret;
}

/**
 * Wrapper for solaris bugfix Bug #128
 * On undocumented error EBUSY, retry the call. Seems to help.
 */
expublic inline int sol_mq_unlink(char *name)
{
    int ret;
    int err;

    while (EXSUCCEED!=(ret =mq_unlink(name)) &&
            ((err=errno)==EBUSY))
    {
	/* NDRX_LOG(log_warn, "%s: got EBUSY - restarting call...", __func__); */
	usleep(SOL_RND_SLEEP);
    }
    errno = err;
    return ret;
}

/**
 * Wrapper for solaris bugfix Bug #128
 * On undocumented error EBUSY, retry the call. Seems to help.
 */
expublic inline int sol_mq_timedsend(mqd_t mqdes, char *msg_ptr, size_t len, 
			      unsigned int msg_prio, struct timespec *abs_timeout)
{
    int ret;
    int err;

    while (EXSUCCEED!=(ret =mq_timedsend(mqdes, msg_ptr, len, msg_prio, abs_timeout)) &&
            ((err=errno)==EBUSY))
    {
/*	NDRX_LOG(log_warn, "%s: got EBUSY - restarting call...", __func__); */
            usleep(SOL_RND_SLEEP);
    }
    errno = err;
    return ret;
}

/**
 * Wrapper for solaris bugfix Bug #128
 * On undocumented error EBUSY, retry the call. Seems to help.
 */
expublic inline  ssize_t sol_mq_timedreceive(mqd_t mqdes, char *msg_ptr,
     size_t  msg_len,  unsigned  *msg_prio, struct
     timespec *abs_timeout)
{
    ssize_t ret;
    int err;

    while (EXFAIL==(ret =mq_timedreceive(mqdes, msg_ptr, msg_len, msg_prio, abs_timeout)) &&
            ((err=errno)==EBUSY))
    {
/*		NDRX_LOG(log_warn, "%s: got EBUSY - restarting call...", __func__);*/
        usleep(SOL_RND_SLEEP);
    }
    errno = err;
    return ret;
}

/**
 * Test the pid to contain regexp 
 * @param pid process id to test
 * @param p_re compiled regexp to test against
 * @return -1 failed, 0 - not matched, 1 - matched
 */
expublic int ndrx_sys_env_test(pid_t pid, regex_t *p_re)
{
    return ndrx_sys_cmdout_test("pargs -ae %d", pid, p_re);
}

/* vim: set ts=4 sw=4 et cindent: */

