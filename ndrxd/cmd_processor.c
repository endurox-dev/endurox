/* 
** Main command processor.
** This serves requests/responses either from ndrx (admin utlity)
** or from EnduroX servers.
**
** @file cmd_processor.c
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
#include <errno.h>

/* Mqueue: */
#include <fcntl.h>           /* For O_* constants */
#include <sys_mqueue.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <ndrxd.h>
#include <ndrxdcmn.h>

#include <cmd_processor.h>
#include <signal.h>
#include <userlog.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define QUEUE_WAIT              1           /* Wait one second on queue, move to param? */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
command_state_t G_command_state;
command_call_t * G_last_interactive_call=NULL;  /* The last call make by user (in progress) */
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

private int cmd_dummy (command_call_t * call, char *data, size_t len, int context);
typedef struct
{
    int command;        /* Command id */
    /* Function to call: */
    int (*p_cmd_call) (command_call_t * call, char *data, size_t len, int context);
    char  *descr;
    char  *contexts;
    int interactive; /* Interactive shall be those which run command processor inside. */
    int entercontext;
} command_map_t;

command_map_t M_command_map [] =
{   /* Command */
    {NDRXD_COM_LDCF_RQ,      cmd_config_load,"ldcf",        ",0,", 0, NDRXD_CTX_NOCHG},
    {NDRXD_COM_LDCF_RP,      cmd_dummy,      "ldcf",        "",  0, NDRXD_CTX_NOCHG}, /* not used */
    {NDRXD_COM_START_RQ,     cmd_start,      "start",       ",0,", 1, NDRXD_CTX_START},
    {NDRXD_COM_START_RP,     cmd_dummy,      "start",       "",  1, NDRXD_CTX_ZERO},  /* not used */
    /* We can get svcinfo any time... */
    {NDRXD_COM_SVCINFO_RQ,   cmd_srvinfo,    "srvinfo",     ",-1,",0,NDRXD_CTX_NOCHG},
    {NDRXD_COM_SVCINFO_RP,   cmd_dummy,      "srvinfo",     "",  0,NDRXD_CTX_NOCHG}, /* not used */
    /* Notification */
    {NDRXD_COM_PMNTIFY_RQ,   cmd_notify,     "notify",      ",-1,", 0,NDRXD_CTX_NOCHG},
    {NDRXD_COM_PMNTIFY_RP,   cmd_dummy,      "notify",      "",   0,NDRXD_CTX_NOCHG}, /* not used */
    /* psc command */
    {NDRXD_COM_PSC_RQ,       cmd_psc,        "psc",         ",-1,", 0,NDRXD_CTX_NOCHG},
    {NDRXD_COM_PSC_RP,       cmd_dummy,      "psc",         "",  0,NDRXD_CTX_NOCHG},  /* not used */
    /* stop command */
    {NDRXD_COM_STOP_RQ,      cmd_stop,       "stop",        ",0,",1,NDRXD_CTX_STOP},
    {NDRXD_COM_STOP_RP,      cmd_dummy,      "stop",        "", 1,NDRXD_CTX_ZERO},  /* not used */
    /* These two are requests to servers, not received by ndrxd in requests! */
    {NDRXD_COM_SRVSTOP_RQ,   cmd_dummy,      "srvstop",     "", 0,NDRXD_CTX_NOCHG},  /* not used */
    {NDRXD_COM_SRVSTOP_RP,   cmd_dummy,      "srvstop",     "", 0,NDRXD_CTX_NOCHG},  /* not used */
    /* attach command, this is not interactive (so to not mix up) */
    {NDRXD_COM_AT_RQ,        cmd_at,         "at",          ",-1,",0,NDRXD_CTX_NOCHG},
    {NDRXD_COM_AT_RP,        cmd_at,         "at",          "", 0,NDRXD_CTX_NOCHG},  /* not used */
    {NDRXD_COM_RELOAD_RQ,    cmd_reload,     "reload",      ",0,",0,NDRXD_CTX_NOCHG},
    {NDRXD_COM_RELOAD_RP,    cmd_reload,     "reload",      "", 0,NDRXD_CTX_NOCHG},  /* not used */
    {NDRXD_COM_TESTCFG_RQ,   cmd_testcfg,    "testcfg",     ",0,",0,NDRXD_CTX_NOCHG},
    {NDRXD_COM_TESTCFG_RP,   cmd_testcfg,    "testcfg",     "", 0,NDRXD_CTX_NOCHG},  /* not used */
    {NDRXD_COM_SRVINFO_RQ,   cmd_dummy,      "srvinfo",     ",0,",0,NDRXD_CTX_NOCHG},
    {NDRXD_COM_SRVINFO_RP,   cmd_dummy,      "srvinfo",     "", 0,NDRXD_CTX_NOCHG},  /* not used */
    {NDRXD_COM_SRVUNADV_RQ,  cmd_srvunadv,   "srvunadv",    ",-1,", 0,NDRXD_CTX_NOCHG},
    {NDRXD_COM_SRVUNADV_RP,  cmd_dummy,      "srvunadv",    "", 0,NDRXD_CTX_NOCHG},
    {NDRXD_COM_XADUNADV_RQ,  cmd_xadunadv,   "xadunadv",    ",0,", 0,NDRXD_CTX_NOCHG},
    {NDRXD_COM_XADUNADV_RP,  cmd_dummy,      "xadunadv",    "", 0,NDRXD_CTX_NOCHG},
    {NDRXD_COM_NXDUNADV_RQ,  cmd_dummy,      "nxdunadv",    ",-1,", 0,NDRXD_CTX_NOCHG},
    {NDRXD_COM_NXDUNADV_RP,  cmd_dummy,      "nxdunadv",    "", 0,NDRXD_CTX_NOCHG},
    {NDRXD_COM_SRVADV_RQ,    cmd_srvadv,     "srvadv",      ",-1,", 0,NDRXD_CTX_NOCHG},
    {NDRXD_COM_SRVADV_RP,    cmd_dummy,      "srvadv",      "", 0,NDRXD_CTX_NOCHG},
    {NDRXD_COM_XAPPM_RQ,     cmd_ppm,        "xappm",       ",-1,", 0,NDRXD_CTX_NOCHG},
    {NDRXD_COM_XAPPM_RP,     cmd_dummy,      "xappm",       "", 0,NDRXD_CTX_NOCHG},
    {NDRXD_COM_XASHM_PSVC_RQ,cmd_shm_psvc,   "xashm_psvc",  ",-1,", 0,NDRXD_CTX_NOCHG},
    {NDRXD_COM_XASHM_PSVC_RP,cmd_dummy,      "xashm_psvc",  "", 0,NDRXD_CTX_NOCHG},
    {NDRXD_COM_XASHM_PSRV_RQ,cmd_shm_psrv,   "xashm_psrv",  ",-1,", 0,NDRXD_CTX_NOCHG},
    {NDRXD_COM_XASHM_PSRV_RP,cmd_dummy,      "xashm_psrv",  "", 0,NDRXD_CTX_NOCHG},
    {NDRXD_COM_NXDREADV_RQ,  cmd_dummy,      "nxdreadv",    ",-1,", 0,NDRXD_CTX_NOCHG},
    {NDRXD_COM_NXDREADV_RP,  cmd_dummy,      "nxdreadv",    "", 0,NDRXD_CTX_NOCHG},
    {NDRXD_COM_XADREADV_RQ,  cmd_xadreadv,   "xadreadv",    ",0,", 0,NDRXD_CTX_NOCHG},
    {NDRXD_COM_XADREADV_RP,  cmd_dummy,      "xadreadv",    "", 0,NDRXD_CTX_NOCHG},
    {NDRXD_COM_XACABORT_RQ,  cmd_abort,       "xaabort",    ",1,2,", 0,NDRXD_CTX_NOCHG},
    {NDRXD_COM_XAABORT_RP,  cmd_dummy,       "xaabort",    "", 0,NDRXD_CTX_NOCHG},
    /* Bridge internal commands: */
    {NDRXD_COM_BRCON_RQ,    cmd_brcon,       "brcon",      ",-1,", 0,NDRXD_CTX_NOCHG},
    {NDRXD_COM_BRCON_RP,    cmd_dummy,       "brcon",      "", 0,NDRXD_CTX_NOCHG},
    {NDRXD_COM_BRDISCON_RQ, cmd_brdiscon,    "brdiscon",   ",-1,", 0,NDRXD_CTX_NOCHG},
    {NDRXD_COM_BRDISCON_RP, cmd_dummy,       "brdiscon",   "", 0,NDRXD_CTX_NOCHG},
    {NDRXD_COM_BRREFERSH_RQ,cmd_brrefresh,   "brrefresh",  ",-1,", 0,NDRXD_CTX_NOCHG},
    {NDRXD_COM_BRREFERSH_RP,cmd_dummy,       "brrefresh",  "", 0,NDRXD_CTX_NOCHG},
    {NDRXD_COM_BRCLOCK_RQ, cmd_dummy,        "brclock",   "", 0,NDRXD_CTX_NOCHG},
    {NDRXD_COM_BRCLOCK_RP, cmd_dummy,        "brclock",   "", 0,NDRXD_CTX_NOCHG},
    {NDRXD_COM_SRVGETBRS_RQ,cmd_getbrs,      "getbrs",  ",-1,", 0,NDRXD_CTX_NOCHG},
    {NDRXD_COM_SRVGETBRS_RP,cmd_dummy,       "getbrs",     "", 0,NDRXD_CTX_NOCHG},
    {NDRXD_COM_SRVPING_RQ,cmd_dummy,         "srvping",  ",-1,", 0,NDRXD_CTX_NOCHG},
    {NDRXD_COM_SRVPING_RP,cmd_srvpingrsp,    "srvping",  ",-1,", 0,NDRXD_CTX_NOCHG},
    /* sreload function... */
    {NDRXD_COM_SRELOAD_RQ,cmd_sreload,         "sreload",  ",0,", 0,NDRXD_CTX_START},
    {NDRXD_COM_SRELOAD_RP,cmd_dummy,           "sreload",  ",0,", 0,NDRXD_CTX_START},
    /* Print Queue */
    {NDRXD_COM_XAPQ_RQ,   cmd_pq,              "xapq",     ",-1,", 0,NDRXD_CTX_NOCHG},
    {NDRXD_COM_XAPQ_RP,   cmd_dummy,           "xapq",     "", 0,NDRXD_CTX_NOCHG},
    /* Print Environment */
    {NDRXD_COM_PE_RQ,     cmd_pe,              "xape",     ",-1,", 0,NDRXD_CTX_NOCHG},
    {NDRXD_COM_PE_RP,     cmd_dummy,           "xape",     "", 0,NDRXD_CTX_NOCHG},
    /* Set Environment */
    {NDRXD_COM_SET_RQ,     cmd_set,            "xaset",     ",-1,", 0,NDRXD_CTX_NOCHG},
    {NDRXD_COM_SET_RP,     cmd_dummy,          "xaset",     "", 0,NDRXD_CTX_NOCHG},
    {NDRXD_COM_UNSET_RQ,   cmd_unset,          "xaunset",   ",-1,", 0,NDRXD_CTX_NOCHG},
    {NDRXD_COM_UNSET_RP,   cmd_dummy,          "xaunset",   "", 0,NDRXD_CTX_NOCHG},
    {NDRXD_COM_SRELOADI_RQ,cmd_sreloadi,         "sreloadi",  ",0,", 0,NDRXD_CTX_START},
    {NDRXD_COM_SRELOADI_RP,cmd_dummy,           "sreloadi",  ",0,", 0,NDRXD_CTX_START}
    
};

/**
 * Dummy function for mapping command replies
 * @param call
 * @param data
 * @param len
 * @return 
 */
private int cmd_dummy(command_call_t * call, char *data, size_t len, int context)
{
    NDRX_LOG(log_debug, "Un-expected command call!");
    return SUCCEED;
}

/**
 * Open listening queue
 * @return
 */
private int cmd_open_queue(void)
{
    int ret=SUCCEED;

    /* Unlink previous admin queue (if have such) - ignore any error */
    ndrx_mq_unlink(G_command_state.listenq_str);
    NDRX_LOG(log_debug, "About to open deamon queue: [%s]",
                                        G_command_state.listenq_str);
    /* Open new queue (non blocked, so  that we do not get deadlock on batch deaths! */
    if ((mqd_t)FAIL==(G_command_state.listenq = ndrx_mq_open_at(G_command_state.listenq_str, O_RDWR | O_CREAT,
                                        S_IWUSR | S_IRUSR, NULL)))
    {
        NDRX_LOG(log_error, "Failed to open queue: [%s] err: %s",
                                        G_command_state.listenq_str, strerror(errno));
        userlog("Failed to open queue: [%s] err: %s",
                                        G_command_state.listenq_str, strerror(errno));
        ret=FAIL;
        goto out;
    }

    NDRX_LOG(log_error, "Queue [%s] opened!", G_command_state.listenq_str);
    
out:
    return ret;
}

/**
 * Open listening queue
 * @return
 */
public int cmd_close_queue(void)
{
    int ret=SUCCEED;

    if (SUCCEED!=ndrx_mq_close(G_command_state.listenq))
    {
        NDRX_LOG(log_error, "Failed to close: [%s] err: %s",
                                     G_command_state.listenq_str, strerror(errno));
    }

    /* Unlink previous admin queue (if have such) - ignore any error */
    if (SUCCEED!=ndrx_mq_unlink(G_command_state.listenq_str))
    {
            NDRX_LOG(log_error, "Failed to unlink: [%s] err: %s",
                                     G_command_state.listenq_str, strerror(errno));
    }
    
out:
    return ret;
}
/**
 * Initialize command processor
 * @return
 */
public int cmd_processor_init(void)
{
    int ret=SUCCEED;

    memset(&G_command_state, 0, sizeof(G_command_state));
    sprintf(G_command_state.listenq_str, NDRX_NDRXD, G_sys_config.qprefix);
    
    if (FAIL==cmd_open_queue())
    {
        ret=FAIL;
        goto out;
    }

out:
    return ret;
}

/**
 * Get Q attribs.
 * @param attr
 * @return 
 */
public int get_cmdq_attr(struct mq_attr *attr)
{
    int ret=SUCCEED;
    
    if (FAIL==ndrx_mq_getattr(G_command_state.listenq, attr))
    {
        NDRX_LOG(log_error, "Failed to ex_mq_getattr on cmd q: %s", 
                strerror(errno));
        ret=FAIL;
    }
    
    return ret;
}
/**
 * Wait for commands and process the one by one...
 * @param finished
 * @return
 */
public int command_wait_and_run(int *finished, int *abort)
{
    int ret=SUCCEED;
    char    msg_buffer_max[ATMI_MSG_MAX_SIZE];
    size_t buf_max = sizeof(msg_buffer_max);
    unsigned int prio = 0;
    struct timespec abs_timeout;
    /* Set recieve time-out */
    struct timeval  timeval;
    command_call_t * call = (command_call_t *)msg_buffer_max;
    size_t  data_len;
    int     error;
    command_map_t *cmd;
    int marked_interactive = FALSE;
    char context_check[16];
    int prev_context = NDRXD_CTX_NOCHG;
    
    memset(msg_buffer_max, 0, sizeof(msg_buffer_max));
    sprintf(context_check, ",%d,", G_command_state.context);

    /* Initialize wait timeout */
    gettimeofday (&timeval, NULL);
    memset(&abs_timeout, 0, sizeof(abs_timeout));
    abs_timeout.tv_sec = timeval.tv_sec + G_sys_config.cmd_wait_time ;
    /* Wait time in nano-seconds, maybe this causes invalid args?*/
    /* non setting this might cause invalid args to queue <0!
    abs_timeout.tv_nsec = 0; */
    
    /*abs_timeout.tv_nsec = timeval.tv_usec*1000;*/

    /* check for child exit... 
    if (check_child_exit())
    {
        goto out;
    }
     * */
    
    if (G_sys_config.restarting)
    {
        /* Check for learning time after restart */
        do_restart_chk();
    }
    else
    {
        /* We will ignore return code, because sanity is not deadly requirement! */
        do_sanity_check();
    }
    
    
    /* Wait for buffer 
    NDRX_LOG(log_warn, "Waiting for message..."); */
    /*siginterrupt(SIGCHLD, TRUE);  This region must be interruptible...
     This causes some messages to be lost! */
    
    /* Change to blocked, if not already! */
    ndrx_q_setblock(G_command_state.listenq, TRUE);
    /* For OSX we need a special case here, we try to access to 
     * the queue for serveral minutes. If we get trylock rejected for
     * configured number of time, we shall close the queue and open it again
     * That would mean that somebody have got the queue lock and died for 
     * some reason (killed). In that secnario, robust mutexes might help
     * but OSX do no have such....
     */ 
    if (FAIL==(data_len = ndrx_mq_timedreceive (G_command_state.listenq,
                    msg_buffer_max, buf_max, &prio, &abs_timeout)))
    {
        error = errno;
    }
    else
    {
        NDRX_LOG(log_warn, ">>>>>>>>>>>>>>>>>>got message, len : %ld",
                        data_len);

        /* reset error code */
        NDRXD_unset_error();
    }
    /*siginterrupt(SIGCHLD, FALSE);*/

    if (FAIL==data_len && EINTR==error)
    {
        NDRX_LOG(log_warn, "ex_mq_timedreceive got interrupted!");
        ret=SUCCEED;
        goto out;
    }
    else if (FAIL==data_len && ETIMEDOUT==error)
    {
        /* nothing recieved - OK */
        ret=SUCCEED;
        goto out;
    }
    else if (FAIL==data_len)
    {
        NDRX_LOG(log_error, "Error occurred when listening on :%s - %d/%s,"
                                "issuing re-init",
                                G_command_state.listenq_str, error, strerror(error));

/*
        ndrx_mq_close(G_command_state.listenq);
*/
        cmd_close_queue();

        if (FAIL==cmd_open_queue())
        {
            ret=FAIL;
        }
        else
        {
            NDRX_LOG(log_error, "Re-init OK");
            ret=SUCCEED;
        }

        goto out; /* finish with this loop */
    }

    /* Check the buffer type... */
    if (NDRX_MAGIC!=call->magic)
    {
        NDRX_LOG(log_error, "Invalid request received, bad magic: got %ld expected %ld",
                call->magic, NDRX_MAGIC);
        /* Dump content... */
        NDRX_DUMP(log_error, "Invalid request recieved", msg_buffer_max, data_len);
        goto out;
    }
#if 0

    /* We can get responses from servers! */

    /* Check is it request or response, we want requests only - even numbers! */
    if (0 != call->command % 2)
    {
        /* Dump content... */
        NDRX_DUMP(log_error, "Response recieved!", msg_buffer_max, data_len);
        goto out;
    }
#endif
    NDRX_LOG(log_debug, "Command ID: %d", call->command);
    if (call->command < NDRXD_COM_MIN || call->command > NDRXD_COM_MAX)
    {
        NDRX_LOG(log_error, "Invalid command recieved: %d from [%s]!",
                                call->command, call->reply_queue);

        /* Reply with error - non supported! */
        NDRXD_set_error_fmt(NDRXD_ECMDNOTFOUND, "Unknown command %d", call->command);
        if (SUCCEED!=simple_command_reply(call, FAIL, 0L, NULL, NULL, 0L, 0, NULL))
        {
            userlog("Failed to send reply back to [%s]", call->reply_queue);
        }
        goto out;
    }
    
    /* Execute command handler */
    cmd = &M_command_map[call->command];

    /* process command received */
    NDRX_LOG(log_debug, "Source: [%s] Command: %d Executing command: %s",
                call->reply_queue, call->command, cmd->descr);
    
    /* Check the running context if not in context & not any. */
    if (NULL==strstr(cmd->contexts, context_check) && NULL==strstr(cmd->contexts, ",-1,"))
    {
        NDRXD_set_error_fmt(NDRXD_ECONTEXT, "Invalid context for command. Current:"
                " [%s] supported: [%s]", context_check, cmd->contexts);
        
        if (SUCCEED!=simple_command_reply(call, FAIL, 0L, NULL, NULL, 0L, 0, NULL))
        {
            userlog("Failed to send reply back to [%s]", call->reply_queue);
        }
        goto out;
    }


    /* save the interactive call */
    if (cmd->interactive && NULL==G_last_interactive_call)
    {
        G_last_interactive_call = call;
        marked_interactive=TRUE;
    }
    
    /* Enter context */
    if (NDRXD_CTX_NOCHG!=cmd->entercontext)
    {
        prev_context = G_command_state.context;
        NDRX_LOG(log_debug, "Entering context: %d", cmd->entercontext);
        G_command_state.context = cmd->entercontext;
    }
        
    /* Handle abort here.  */
    if (NDRXD_COM_XACABORT_RQ==cmd->command)
    {
        NDRX_LOG(log_warn, "Abort requested from xadmin!");
        *abort = TRUE;
    }
  
    ret = cmd->p_cmd_call(call, msg_buffer_max, data_len, G_command_state.context);

    /* Return from call, remove interactive ptr */
    if (marked_interactive)
    {
        G_last_interactive_call = NULL;
        marked_interactive=FALSE;
    }
    

out:
    /* Restore original context */
    if (NDRXD_CTX_NOCHG!=prev_context)
    {
        G_command_state.context = prev_context;
        NDRX_LOG(log_debug, "Restoring context: %d", G_command_state.context);
    }
        
    return ret;
}

/* ndrx will start ndrxd_guard, which will start ndrxd.
 * Meanwhile ndrx will try to open the queue for passing commands to.
 */


