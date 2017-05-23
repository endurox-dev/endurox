/* 
** Implementation for start & stop commands.
**
** @file cmd_startstop.c
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
#include <sys/param.h>

#include <ndrstandard.h>
#include <ndebug.h>

#include <ndrx.h>
#include <ndrxdcmn.h>
#include <atmi_int.h>
#include <gencall.h>
#include <nclopt.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/


/**
 * Return process state in human understandable string.
 * @param state
 * @return
 */
private char *proc_state_to_str(long state, short msg_type)
{

    static char *started = "Started";
    static char *not_started = "Not started";
    static char *died = "Died";
    static char *shutdown = "Shutdown succeeded";
    static char *stopping = "Shutdown in progress";
    static char *nosuchfile = "No such file or directory";
    static char *stillstarting = "Still starting";
    static char unknown[256];
    char *ret;

    switch (state)
    {
        case NDRXD_PM_NOT_STARTED:
            ret = not_started;
            break;
        case NDRXD_PM_RUNNING_OK:
            ret = started;
            break;
        case NDRXD_PM_DIED:
            ret = died;
            break;
        case NDRXD_PM_STARTING:
            ret = stillstarting;
            break;
        case NDRXD_PM_EXIT:
            /* Binaries might exit with fail at startup... */
            if (NDRXD_CALL_TYPE_PM_STOPPED==msg_type)
            {
                ret = shutdown;
            }
            else
            {
                ret = died;
            }   
            break;
        case NDRXD_PM_ENOENT:
            ret = nosuchfile;
            break;
        case NDRXD_PM_STOPPING:
            ret = stopping;
            break;
        default:
            sprintf(unknown, "Unknown state (%ld)", state);
            ret = unknown;
            break;
    }
    
    return ret;
}
/**
 * Process response back.
 * @param reply
 * @param reply_len
 * @return
 */
public int ss_rsp_process(command_reply_t *reply, size_t reply_len)
{
    command_reply_pm_t * pm_info = (command_reply_pm_t*)reply;

    switch (reply->msg_type)
    {
        case NDRXD_CALL_TYPE_PM_STARTING:
            fprintf(stderr, "exec %s %s :\n\t",
                pm_info->binary_name, pm_info->clopt);
            break;
        case NDRXD_CALL_TYPE_PM_STARTED:
            fprintf(stderr, "process id=%d ... %s.\n",pm_info->pid,
                proc_state_to_str(pm_info->state, reply->msg_type)
                );
            break;
        case NDRXD_CALL_TYPE_PM_STOPPING:
            fprintf(stderr, "Server executable = %s\tId = %d :\t",
                pm_info->binary_name, pm_info->srvid);
            break;
        case  NDRXD_CALL_TYPE_PM_STOPPED:
            fprintf(stderr, "%s.\n",
                proc_state_to_str(pm_info->state, reply->msg_type)
                );
            break;
        case NDRXD_CALL_TYPE_GENERIC:
                if (NDRXD_COM_STOP_RP==reply->command)
                    fprintf(stderr, "Shutdown finished. %ld processes stopped.\n",
                                    reply->userfld1);
                else if (NDRXD_COM_SRELOAD_RP==reply->command)
                    fprintf(stderr, "Reload finished. %ld processes reloaded.\n",
                                    reply->userfld1);
                else
                    fprintf(stderr, "Startup finished. %ld processes started.\n",
                                    reply->userfld1);
            break;
        default:
            NDRX_LOG(log_error, "Ignoring unknown message!");
            break;
    }
    return SUCCEED;
}

/**
 * Start application.
 * This uses feedback function to report progress..
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
public int cmd_start(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret=SUCCEED;
    command_startstop_t call;
    short srvid=FAIL;
    char srvnm[MAXTIDENT+1]={EOS};
    short confirm = FALSE;
    ncloptmap_t clopt[] =
    {
        {'i', BFLD_SHORT, (void *)&srvid, 0, 
                                NCLOPT_OPT|NCLOPT_HAVE_VALUE, "Server ID"},
        {'s', BFLD_STRING, (void *)srvnm, sizeof(srvnm), 
                                NCLOPT_OPT|NCLOPT_HAVE_VALUE, "Server name"},
        {'y', BFLD_SHORT, (void *)&confirm, 0, 
                                NCLOPT_OPT|NCLOPT_TRUEBOOL, "Confirm"},
        {0}
    };
        
    if (argc>=2 && '-'!=argv[1][0])
    {
	strncpy(srvnm, argv[1], MAXTIDENT);
	srvnm[MAXTIDENT] = 0;
    }
    else
    {
        /* parse command line */
        if (nstd_parse_clopt(clopt, TRUE,  argc, argv, FALSE))
        {
            fprintf(stderr, XADMIN_INVALID_OPTIONS_MSG);
            FAIL_OUT(ret);
        }
    }
    
    memset(&call, 0, sizeof(call));
    
    if (FAIL!=srvid && EOS!=srvnm[0])
    {
        fprintf(stderr, "-i and -s cannot be combined!\n");
        FAIL_OUT(ret);
    }
    
    if (FAIL==srvid && EOS==srvnm[0] &&
          !chk_confirm("Are you sure you want to start application?", confirm))
    {
        FAIL_OUT(ret);
    }
    
    /* prepare for call */
    call.srvid = srvid;
    strcpy(call.binary_name, srvnm);
    
    ret=cmd_generic_listcall(p_cmd_map->ndrxd_cmd, NDRXD_SRC_ADMIN,
                        NDRXD_CALL_TYPE_GENERIC,
                        (command_call_t *)&call, sizeof(call),
                        G_config.reply_queue_str,
                        G_config.reply_queue,
                        G_config.ndrxd_q,
                        G_config.ndrxd_q_str,
                        argc, argv,
                        p_have_next,
                        G_call_args,
                        FALSE);
out:
    return ret;
}

/**
 * Shutdown application server.
 * TODO: xadmin stop -i -1 makes stop to all domain!
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @param p_have_next
 * @return 
 */
public int cmd_stop(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret=SUCCEED;
    command_startstop_t call;
    short srvid=FAIL;
    char srvnm[MAXTIDENT+1]={EOS};
    short confirm = FALSE;
    short complete = FALSE;
    ncloptmap_t clopt[] =
    {
        {'i', BFLD_SHORT, (void *)&srvid, 0, 
                                NCLOPT_OPT|NCLOPT_HAVE_VALUE, "Server ID"},
        {'s', BFLD_STRING, (void *)srvnm, sizeof(srvnm), 
                                NCLOPT_OPT|NCLOPT_HAVE_VALUE, "Server name"},
                                
        {'y', BFLD_SHORT, (void *)&confirm, 0, 
                                NCLOPT_OPT|NCLOPT_TRUEBOOL, "Confirm"},
        {'c', BFLD_SHORT, (void *)&complete, 0, 
                                NCLOPT_OPT|NCLOPT_TRUEBOOL, "Complete shutdown (ndrxd off)"},
        {0}
    };
        
    if (argc>=2 && '-'!=argv[1][0])
    {
	strncpy(srvnm, argv[1], MAXTIDENT);
	srvnm[MAXTIDENT] = 0;
    }
    else
    {
        /* parse command line */
        if (nstd_parse_clopt(clopt, TRUE,  argc, argv, FALSE))
        {
            fprintf(stderr, XADMIN_INVALID_OPTIONS_MSG);
            FAIL_OUT(ret);
        }
    }
    
    memset(&call, 0, sizeof(call));
    
    if (FAIL!=srvid && EOS!=srvnm[0])
    {
        fprintf(stderr, "-i and -s cannot be combined!\n");
        FAIL_OUT(ret);
    }
    
    if ((FAIL!=srvid || EOS!=srvnm[0]) && call.complete_shutdown)
    {
        fprintf(stderr, "-i or -s cannot be combined with -c!\n");
        FAIL_OUT(ret);
    }
    
    if (FAIL==srvid && EOS==srvnm[0] &&
          !chk_confirm("Are you sure you want to stop application?", confirm))
    {
        FAIL_OUT(ret);
    }
    
    /* prepare for call */
    call.complete_shutdown = complete;
    call.srvid = srvid;
    strcpy(call.binary_name, srvnm);

    ret=cmd_generic_listcall(p_cmd_map->ndrxd_cmd, NDRXD_SRC_ADMIN,
                    NDRXD_CALL_TYPE_GENERIC,
                    (command_call_t *)&call, sizeof(call),
                    G_config.reply_queue_str,
                    G_config.reply_queue,
                    G_config.ndrxd_q,
                    G_config.ndrxd_q_str,
                    argc, argv,
                    p_have_next,
                    G_call_args,
                    FALSE);
out:
    return ret;
}


/**
 * restart app or server.
 * Invokes stop & start in sequence.
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @param p_have_next
 * @return 
 */
public int cmd_r(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret=SUCCEED;
    cmd_mapping_t cmd;
    short srvid=FAIL;
    char srvnm[MAXTIDENT+1]={EOS};
    short confirm = FALSE;
    
    /* just verify that content is ok: */
    ncloptmap_t clopt[] =
    {
        {'i', BFLD_SHORT, (void *)&srvid, 0, 
                                NCLOPT_OPT|NCLOPT_HAVE_VALUE, "Server ID"},
        {'s', BFLD_STRING, (void *)srvnm, sizeof(srvnm), 
                                NCLOPT_OPT|NCLOPT_HAVE_VALUE, "Server name"},
                                
        {'y', BFLD_SHORT, (void *)&confirm, 0, 
                                NCLOPT_OPT|NCLOPT_TRUEBOOL, "Confirm"},
        {0}
    };
        
    if (argc>=2 && '-'==argv[1][0])
    {
        /* parse command line */
        if (nstd_parse_clopt(clopt, TRUE,  argc, argv, FALSE))
        {
            fprintf(stderr, XADMIN_INVALID_OPTIONS_MSG);
            FAIL_OUT(ret);
        }
    }
    
    
    memset(&cmd, 0, sizeof(cmd));
    
    cmd.ndrxd_cmd = NDRXD_COM_STOP_RQ;
    if (SUCCEED==(ret=cmd_stop(&cmd, argc, argv, p_have_next)))
    {
        cmd.ndrxd_cmd = NDRXD_COM_START_RQ;
        ret = cmd_start(&cmd, argc, argv, p_have_next);
    }
    
out:
    return ret;
}



/**
 * Abort app domain startup or shutdown.
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
public int cmd_cabort(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    command_call_t call;
    memset(&call, 0, sizeof(call));
    int ret=SUCCEED;

    if (!chk_confirm_clopt("Are you sure you want to abort app domain start/stop?", argc, argv))
    {
        ret=FAIL;
        goto out;
    }

    ret=cmd_generic_listcall(p_cmd_map->ndrxd_cmd, NDRXD_SRC_ADMIN,
                        NDRXD_CALL_TYPE_GENERIC,
                        &call, sizeof(call),
                        G_config.reply_queue_str,
                        G_config.reply_queue,
                        G_config.ndrxd_q,
                        G_config.ndrxd_q_str,
                        argc, argv,
                        p_have_next,
                        G_call_args,
                        FALSE);
out:
    return ret;
}


/**
 * Reload services (sequental service restart)
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
public int cmd_sreload(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret=SUCCEED;
    command_startstop_t call;
    short srvid=FAIL;
    char srvnm[MAXTIDENT+1]={EOS};
    short confirm = FALSE;
    ncloptmap_t clopt[] =
    {
        {'i', BFLD_SHORT, (void *)&srvid, 0, 
                                NCLOPT_OPT|NCLOPT_HAVE_VALUE, "Server ID"},
        {'s', BFLD_STRING, (void *)srvnm, sizeof(srvnm), 
                                NCLOPT_OPT|NCLOPT_HAVE_VALUE, "Server name"},
                                
        {'y', BFLD_SHORT, (void *)&confirm, 0, 
                                NCLOPT_OPT|NCLOPT_TRUEBOOL, "Confirm"},
        {0}
    };
        
    if (argc>=2 && '-'!=argv[1][0])
    {
	strncpy(srvnm, argv[1], MAXTIDENT);
	srvnm[MAXTIDENT] = 0;
    }
    else
    {
        /* parse command line */
        if (nstd_parse_clopt(clopt, TRUE,  argc, argv, FALSE))
        {
            fprintf(stderr, XADMIN_INVALID_OPTIONS_MSG);
            FAIL_OUT(ret);
        }
    }
    
    memset(&call, 0, sizeof(call));
    
    if (FAIL!=srvid && EOS!=srvnm[0])
    {
        fprintf(stderr, "-i and -s cannot be combined!\n");
        FAIL_OUT(ret);
    }
    
    if (FAIL==srvid && EOS==srvnm[0] &&
          !chk_confirm("Are you sure you want to start application?", confirm))
    {
        FAIL_OUT(ret);
    }
    
    /* prepare for call */
    call.srvid = srvid;
    strcpy(call.binary_name, srvnm);
    
    ret=cmd_generic_listcall(p_cmd_map->ndrxd_cmd, NDRXD_SRC_ADMIN,
                        NDRXD_CALL_TYPE_GENERIC,
                        (command_call_t *)&call, sizeof(call),
                        G_config.reply_queue_str,
                        G_config.reply_queue,
                        G_config.ndrxd_q,
                        G_config.ndrxd_q_str,
                        argc, argv,
                        p_have_next,
                        G_call_args,
                        FALSE);
out:
    return ret;
}
