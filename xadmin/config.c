/* 
** Master Configuration for ndrx command line utilty!
**
** @file config.c
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
public int load_env_config(void)
{
    char *p;
    int ret=SUCCEED;

    memset(&G_config, 0, sizeof(G_config));
    G_config.ndrxd_q = (mqd_t)FAIL;
    G_config.reply_queue = (mqd_t)FAIL;
    
    /* Common configuration loading... */   
    if (SUCCEED!=ndrx_load_common_env())
    {
        NDRX_LOG(log_error, "Failed to load common env");
        ret=FAIL;
        goto out;
    }

    if (NULL==(p=getenv(CONF_NDRX_DPID)))
    {
        NDRX_LOG(log_error, "Missing %s environment variable!", CONF_NDRX_DPID);
    }
    else
    {
        strcpy(G_config.pid_file, p);
        NDRX_LOG(log_debug, "ndrxd pid file: %s", G_config.pid_file);
    }
    
    G_config.qprefix = getenv(CONF_NDRX_QPREFIX);
    if (NULL==G_config.qprefix)
    {
        NDRX_LOG(log_error, "Missing config key %s - FAIL", CONF_NDRX_QPREFIX);
        ret=FAIL;
        goto out;
    }
    
    G_config.qpath = getenv(CONF_NDRX_QPATH);
    if (NULL==G_config.qpath)
    {
        NDRX_LOG(log_error, "Missing config key %s - FAIL", CONF_NDRX_QPATH);
        ret=FAIL;
        goto out;
    }

    /* format the admin queue */
    sprintf(G_config.ndrxd_q_str, NDRX_NDRXD, G_config.qprefix);

    /* Open client queue to wait replies on */
    sprintf(G_config.reply_queue_str, NDRX_NDRXCLT, G_config.qprefix, getpid());

    /* Unlink previous admin queue (if have such) - ignore any error */
    ndrx_mq_unlink(G_config.reply_queue_str);
    NDRX_LOG(log_debug, "About to open reply queue: [%s]",
                                        G_config.reply_queue_str);
    /* Open new queue... */
    if ((mqd_t)FAIL==(G_config.reply_queue = ndrx_ex_mq_open_at(G_config.reply_queue_str,
                                        O_RDWR | O_CREAT,
                                        S_IWUSR | S_IRUSR, NULL)))
    {
        NDRX_LOG(log_error, "Failed to open queue: [%s] err: %s",
                                        G_config.reply_queue_str, strerror(errno));
        userlog("Failed to open queue: [%s] err: %s",
                                        G_config.reply_queue_str, strerror(errno));
        ret=FAIL;
        goto out;
    }
    
    NDRX_LOG(log_error, "Reply queue [%s] opened!", G_config.reply_queue_str);

    /* Get config key */
    G_config.ndrxd_logfile = getenv(CONF_NDRX_DMNLOG);
    if (NULL==G_config.ndrxd_logfile)
    {
        NDRX_LOG(log_error, "Missing config key %s - FAIL", CONF_NDRX_DMNLOG);
        ret=FAIL;
        goto out;
    }

out:
    return ret;
}

