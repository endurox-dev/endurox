/* 
** Remove service queue.
** We should open the service queue, receive all messages and reply back with failure.
** Thus function shall be called after the service shared memory block is uninstalled!
**
** @file svc_q_remove.c
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
 * @param svc
 * @return 
 */
expublic int remove_service_q(char *svc, short srvid)
{
    int ret=EXSUCCEED;
    char q_str[NDRX_MAX_Q_SIZE+1];
    mqd_t qd=(mqd_t)EXFAIL;
    char msg_buf[ATMI_MSG_MAX_SIZE];
    int len;
    unsigned prio;
    tp_command_generic_t *gen_command;
    char *fn = "remove_service_q";
    NDRX_LOG(log_debug, "%s - Enter, svc = [%s], srvid = %hd", fn, svc, srvid);
     
#ifdef EX_USE_POLL
    sprintf(q_str, NDRX_SVC_QFMT_SRVID, G_sys_config.qprefix, svc, srvid);
#else
    sprintf(q_str, NDRX_SVC_QFMT, G_sys_config.qprefix, svc);
#endif
    
    /* Run in non-blocked mode */
    if ((mqd_t)EXFAIL==(qd = ndrx_mq_open_at(q_str, O_RDWR|O_NONBLOCK,S_IWUSR | S_IRUSR, NULL)))
    {
        NDRX_LOG(log_error, "Failed to open queue: [%s] err: %s",
                                        q_str, strerror(errno));
        ret=EXFAIL;
        goto out;
    }
    
    /* Unlink the queue, the actual queue will live out throught next session! 
     * i.e. all users should close it to dispose it! As by manpage! */
    if (EXSUCCEED!=ndrx_mq_unlink(q_str))
    {
        NDRX_LOG(log_error, "Failed to unlink q [%s]: %s", 
                q_str, strerror(errno));
    }
    
    /* Read all messages from Q & reply with dummy/FAIL stuff back! */
    while ((len=ndrx_mq_receive (qd,
        (char *)msg_buf, ATMI_MSG_MAX_SIZE, &prio)) > 0)
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
    
out:
    if ((mqd_t)EXFAIL!=qd)
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

