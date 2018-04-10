/* 
** Master Configuration for ndrx command line utility!
**
** @file config.c
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <sys_mqueue.h>
#include <errno.h>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>
#include <unistd.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <ndrx.h>
#include <atmi.h>
#include <userlog.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
ndrx_config_t G_config;
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Load environment configuration
 * @return SUCCEED/FAIL
 */
expublic int load_env_config(void)
{
    char *p;
    int ret=EXSUCCEED;

    memset(&G_config, 0, sizeof(G_config));
    G_config.ndrxd_q = (mqd_t)EXFAIL;
    G_config.reply_queue = (mqd_t)EXFAIL;
    
    /* Common configuration loading... */   
    if (EXSUCCEED!=ndrx_load_common_env())
    {
        NDRX_LOG(log_error, "Failed to load common env");
        ret=EXFAIL;
        goto out;
    }
    
    /* set custom timeout if available */
    if (NULL!=(p = getenv(CONF_NDRX_XADMINTOUT)))
    {
        int tout = atoi(p);
        if (EXSUCCEED!=tptoutset(tout))
        {
            NDRX_LOG(log_error, "Failed to set `xadmin' Q timeout to: %d: %s",
                    tout, tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
    }
    else
    {
        /* set the list call flags 
         * default to no timeout use
         */
        G_config.listcall_flags|=TPNOTIME;
    }
    
    if (NULL==(p=getenv(CONF_NDRX_DPID)))
    {
        NDRX_LOG(log_error, "Missing %s environment variable!", CONF_NDRX_DPID);
    }
    else
    {
        NDRX_STRCPY_SAFE(G_config.pid_file, p);
        NDRX_LOG(log_debug, "ndrxd pid file: %s", G_config.pid_file);
    }
    
    G_config.qprefix = getenv(CONF_NDRX_QPREFIX);
    if (NULL==G_config.qprefix)
    {
        NDRX_LOG(log_error, "Missing config key %s - FAIL", CONF_NDRX_QPREFIX);
        ret=EXFAIL;
        goto out;
    }
    
    G_config.qpath = getenv(CONF_NDRX_QPATH);
    if (NULL==G_config.qpath)
    {
        NDRX_LOG(log_error, "Missing config key %s - FAIL", CONF_NDRX_QPATH);
        ret=EXFAIL;
        goto out;
    }

    /* format the admin queue */
    snprintf(G_config.ndrxd_q_str, sizeof(G_config.ndrxd_q_str), 
            NDRX_NDRXD, G_config.qprefix);

    /* Open client queue to wait replies on */
    snprintf(G_config.reply_queue_str, sizeof(G_config.reply_queue_str),
            NDRX_NDRXCLT, G_config.qprefix, getpid());

    /* Unlink previous admin queue (if have such) - ignore any error */
    ndrx_mq_unlink(G_config.reply_queue_str);
    NDRX_LOG(log_debug, "About to open reply queue: [%s]",
                                        G_config.reply_queue_str);
    /* Open new queue... */
    if ((mqd_t)EXFAIL==(G_config.reply_queue = ndrx_mq_open_at(G_config.reply_queue_str,
                                        O_RDWR | O_CREAT,
                                        S_IWUSR | S_IRUSR, NULL)))
    {
        NDRX_LOG(log_error, "Failed to open queue: [%s] err: %s",
                                        G_config.reply_queue_str, strerror(errno));
        userlog("Failed to open queue: [%s] err: %s",
                                        G_config.reply_queue_str, strerror(errno));
        ret=EXFAIL;
        goto out;
    }
    
    NDRX_LOG(log_error, "Reply queue [%s] opened!", G_config.reply_queue_str);

    /* Get config key */
    G_config.ndrxd_logfile = getenv(CONF_NDRX_DMNLOG);
    if (NULL==G_config.ndrxd_logfile)
    {
        NDRX_LOG(log_error, "Missing config key %s - FAIL", CONF_NDRX_DMNLOG);
        ret=EXFAIL;
        goto out;
    }    

out:
    return ret;
}

