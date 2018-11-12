/**
 * @brief Remove service queue.
 *   We should open the service queue, receive all messages and reply back with failure.
 *   Thus function shall be called after the service shared memory block is uninstalled!
 *
 * @file svc_q_remove.c
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <memory.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <utlist.h>
#include <fcntl.h>

#include <ndrstandard.h>
#include <ndrxd.h>
#include <atmi_int.h>
#include <nstopwatch.h>

#include <ndebug.h>
#include <cmd_processor.h>
#include <signal.h>

#include "userlog.h"

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define REMOVE_SVC_READVERTISE_LISTS            10*60 /* Keep that stuff for 10 minutes! 
                                                         Might consider to move to config param! 
                                                        */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
typedef struct removed_svcs removed_svcs_t;
struct removed_svcs
{
    char svc[XATMI_SERVICE_NAME_LENGTH+1];
    ndrx_stopwatch_t remove_time;
    removed_svcs_t *prev, *next;
};

/*---------------------------Globals------------------------------------*/
removed_svcs_t * M_removed = NULL;
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/


/**
 * Find entry for service
 * @param svc
 * @return 
 */
exprivate removed_svcs_t * find_removed_entry(char *svc)
{
    removed_svcs_t * ret = NULL;
    removed_svcs_t * tmp = NULL;
    
    DL_FOREACH(M_removed, tmp)
    {
        if (0==strcmp(tmp->svc, svc))
        {
            ret=tmp;
            break;
        }
    }
    
    return ret;
}

/**
 * Firstly we should open the Q.
 * Then reply with bad response to all msgs, then unlink it.
 * So firstly we try to get semaphore...
 * After we got it, we check do really Q needs to be removed!
 * @param[in] svc service name to unlink, conditional
 * @param[in] srvid Server id providing the the service, conditional
 * @param[in] in_qd Already open queue descriptor, used by System V zapping
 * @param[in] in_qstr for debug purposes queue strnig, either svc+srv 
 *  or in_qd +in_qstr
 * @return EXSUCCEED/EXFAIL
 */
expublic int remove_service_q(char *svc, short srvid, mqd_t in_qd, char *in_qstr)
{
    int ret=EXSUCCEED;
    char q_str[NDRX_MAX_Q_SIZE+1];
    mqd_t qd=(mqd_t)EXFAIL;
    char msg_buf[NDRX_MSGSIZEMAX];
    int len;
    unsigned prio;
    tp_command_generic_t *gen_command;
    char *fn = "remove_service_q";
    
    NDRX_LOG(log_debug, "Enter, svc = [%s], srvid = %hd", svc?svc:"(null)", srvid);
    if (NULL!=in_qstr)
    {
        NDRX_STRCPY_SAFE(q_str, in_qstr);
    }
    else
    {
#ifdef EX_USE_POLL
        snprintf(q_str, sizeof(q_str), NDRX_SVC_QFMT_SRVID, 
                G_sys_config.qprefix, svc, srvid);
#else
        snprintf(q_str, sizeof(q_str), NDRX_SVC_QFMT, G_sys_config.qprefix, svc);
#endif
    }
    
    NDRX_LOG(log_debug, "Flushing + unlink the queue [%s]", q_str);
    
    /* Run in non-blocked mode */
    if ((mqd_t)EXFAIL!=in_qd)
    {
        NDRX_LOG(log_debug, "Re-use existing mqd=%p", in_qd);
        qd = in_qd;
    }
    else if ((mqd_t)EXFAIL==(qd = ndrx_mq_open_at(q_str, 
            O_RDWR|O_NONBLOCK,S_IWUSR | S_IRUSR, NULL)))
    {
        NDRX_LOG(log_error, "Failed to open queue: [%s] err: %s",
                                        q_str, strerror(errno));
        ret=EXFAIL;
        goto out;

    }
    
    /* for System V queue is unlinked as part of the sanity checks 
     * also System V queues cannot be unlinked while other processes are connected
     * in such scenario they will 
     */
#ifndef EX_USE_SYSVQ
    /* Unlink the queue, the actual queue will live out through next session! 
     * i.e. all users should close it to dispose it! As by manpage! */
    if (EXSUCCEED!=ndrx_mq_unlink(q_str))
    {
        NDRX_LOG(log_error, "Failed to unlink q [%s]: %s", 
                q_str, strerror(errno));
    }
#endif
    
    /* Read all messages from Q & reply with dummy/FAIL stuff back! */
    while ((len=ndrx_mq_receive (qd,
        (char *)msg_buf, sizeof(msg_buf), &prio)) > 0)
    {
        NDRX_LOG(log_warn, "Got message, size: %d", len);
        gen_command = (tp_command_generic_t *)msg_buf;
        
        /* Not sure how this can affect ATMI_COMMAND_CONNECT or ATMI_COMMAND_CONNRPLY
         * Time will show this for us!
         */
        if (ATMI_COMMAND_TPCALL==gen_command->command_id)
        {
            ndrx_reply_with_failure((tp_command_call_t *)gen_command, 
                    TPNOBLOCK, TPENOENT, G_command_state.listenq_str);
        }
        else
        {
            NDRX_LOG(log_warn, "Skipping command: %d", gen_command->command_id);
        }
    }
    
    NDRX_LOG(log_debug, "Done receive...");
    
out:
    /* close only if we did open the queue */
    if ((mqd_t)EXFAIL==in_qd && (mqd_t)EXFAIL!=qd)
    {
        if (EXSUCCEED!=ndrx_mq_close(qd))
        {
            NDRX_LOG(log_warn, "Failed to close q: %d - %s", qd, 
                    strerror(errno));
        }
    }
  
    NDRX_LOG(log_debug, "%s - return, ret = %d", fn, ret);

    return ret;
    
}

/* vim: set ts=4 sw=4 et smartindent: */
