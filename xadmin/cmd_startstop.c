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
#include <unistd.h>

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
exprivate char *proc_state_to_str(long state, short msg_type)
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
            snprintf(unknown, sizeof(unknown), "Unknown state (%ld)", state);
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
expublic int ss_rsp_process(command_reply_t *reply, size_t reply_len)
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
    return EXSUCCEED;
}

/**
 * Start application.
 * This uses feedback function to report progress..
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
expublic int cmd_start(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret=EXSUCCEED;
    command_startstop_t call;
    short srvid=EXFAIL;
    char srvnm[MAXTIDENT+1]={EXEOS};
    short confirm = EXFALSE;
    short keep_running_ndrxd;
    
    ncloptmap_t clopt[] =
    {
        {'i', BFLD_SHORT, (void *)&srvid, 0, 
                                NCLOPT_OPT|NCLOPT_HAVE_VALUE, "Server ID"},
        {'s', BFLD_STRING, (void *)srvnm, sizeof(srvnm), 
                                NCLOPT_OPT|NCLOPT_HAVE_VALUE, "Server name"},
        {'y', BFLD_SHORT, (void *)&confirm, 0, 
                                NCLOPT_OPT|NCLOPT_TRUEBOOL, "Confirm"},
        {'k', BFLD_SHORT, (void *)&keep_running_ndrxd, 0, 
                                NCLOPT_OPT|NCLOPT_TRUEBOOL, "Keep ndrxd running"},
        {0}
    };
        
    if (argc>=2 && '-'!=argv[1][0])
    {
	NDRX_STRNCPY(srvnm, argv[1], MAXTIDENT);
	srvnm[MAXTIDENT] = 0;
    }
    else
    {
        /* parse command line */
        if (nstd_parse_clopt(clopt, EXTRUE,  argc, argv, EXFALSE))
        {
            fprintf(stderr, XADMIN_INVALID_OPTIONS_MSG);
            EXFAIL_OUT(ret);
        }
    }
    
    memset(&call, 0, sizeof(call));
    
    if (EXFAIL!=srvid && EXEOS!=srvnm[0])
    {
        fprintf(stderr, "-i and -s cannot be combined!\n");
        EXFAIL_OUT(ret);
    }
    
    if (EXFAIL==srvid && EXEOS==srvnm[0] &&
          !chk_confirm("Are you sure you want to start application?", confirm))
    {
        EXFAIL_OUT(ret);
    }
    
    /* prepare for call */
    call.srvid = srvid;
    NDRX_STRCPY_SAFE(call.binary_name, srvnm);
    
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
                        EXFALSE);
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
expublic int cmd_stop(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret=EXSUCCEED;
    command_startstop_t call;
    short srvid=EXFAIL;
    char srvnm[MAXTIDENT+1]={EXEOS};
    short confirm = EXFALSE;
    short keep_running_ndrxd = EXFALSE;
    short force_off = EXFALSE;  /* force shutdown (for malfunction/no pid instances) */
    short dummy;
    
    ncloptmap_t clopt[] =
    {
        {'i', BFLD_SHORT, (void *)&srvid, 0, 
                                NCLOPT_OPT|NCLOPT_HAVE_VALUE, "Server ID"},
        {'s', BFLD_STRING, (void *)srvnm, sizeof(srvnm), 
                                NCLOPT_OPT|NCLOPT_HAVE_VALUE, "Server name"},
                                
        {'y', BFLD_SHORT, (void *)&confirm, 0, 
                                NCLOPT_OPT|NCLOPT_TRUEBOOL, "Confirm"},
        {'c', BFLD_SHORT, (void *)&dummy, 0, 
                                NCLOPT_OPT|NCLOPT_TRUEBOOL, "Left for compatibility"},
        {'k', BFLD_SHORT, (void *)&keep_running_ndrxd, 0, 
                                NCLOPT_OPT|NCLOPT_TRUEBOOL, "Keep ndrxd running"},
        {'f', BFLD_SHORT, (void *)&force_off, 0, 
                                NCLOPT_OPT|NCLOPT_TRUEBOOL, "Force shutdown"},
        {0}
    };
        
    if (argc>=2 && '-'!=argv[1][0])
    {
	NDRX_STRNCPY(srvnm, argv[1], MAXTIDENT);
	srvnm[MAXTIDENT] = 0;
    }
    else
    {
        /* parse command line */
        if (nstd_parse_clopt(clopt, EXTRUE,  argc, argv, EXFALSE))
        {
            fprintf(stderr, XADMIN_INVALID_OPTIONS_MSG);
            EXFAIL_OUT(ret);
        }
    }
    
    memset(&call, 0, sizeof(call));
    
    if (EXFAIL!=srvid && EXEOS!=srvnm[0])
    {
        fprintf(stderr, "-i and -s cannot be combined!\n");
        EXFAIL_OUT(ret);
    }
    
    if (EXFAIL!=srvid || EXEOS!=srvnm[0])
    {
        keep_running_ndrxd = EXTRUE;
    }
    
    if (EXFAIL==srvid && EXEOS==srvnm[0] &&
          !chk_confirm("Are you sure you want to stop application?", confirm))
    {
        EXFAIL_OUT(ret);
    }
    
    /* prepare for call */
    if (keep_running_ndrxd)
    {
        call.complete_shutdown = EXFALSE;
    }
    else
    {
        /* do full shutdown by default */
        call.complete_shutdown = EXTRUE;
    }
    
    if (call.complete_shutdown && !is_ndrxd_running() && !force_off)
    {
        fprintf(stderr, "WARNING ! `ndrxd' daemon is in `%s', use -f "
                "to force shutdown!\n",
                NDRXD_STAT_NOT_STARTED==G_config.ndrxd_stat?"not started":"malfunction");

        NDRX_LOG(log_warn, "WARNING ! `ndrxd' daemon is in `%s' state, use -f "
                "to force shutdown!\n",
                NDRXD_STAT_NOT_STARTED==G_config.ndrxd_stat?"not started":"malfunction");
        EXFAIL_OUT(ret);
    }
    
    call.srvid = srvid;
    NDRX_STRCPY_SAFE(call.binary_name, srvnm);

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
                    EXFALSE);
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
expublic int cmd_r(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret=EXSUCCEED;
    cmd_mapping_t cmd;
    short srvid=EXFAIL;
    char srvnm[MAXTIDENT+1]={EXEOS};
    short confirm = EXFALSE;
    short keep_running_ndrxd = EXFALSE;
    
    /* just verify that content is ok: */
    ncloptmap_t clopt[] =
    {
        {'i', BFLD_SHORT, (void *)&srvid, 0, 
                                NCLOPT_OPT|NCLOPT_HAVE_VALUE, "Server ID"},
        {'s', BFLD_STRING, (void *)srvnm, sizeof(srvnm), 
                                NCLOPT_OPT|NCLOPT_HAVE_VALUE, "Server name"},
                                
        {'y', BFLD_SHORT, (void *)&confirm, 0, 
                                NCLOPT_OPT|NCLOPT_TRUEBOOL, "Confirm"},
                                
        {'k', BFLD_SHORT, (void *)&keep_running_ndrxd, 0, 
                                NCLOPT_OPT|NCLOPT_TRUEBOOL, "Keep ndrxd running"},
        {0}
    };
        
    if (argc>=2 && '-'!=argv[1][0])
    {
	NDRX_STRNCPY(srvnm, argv[1], MAXTIDENT);
	srvnm[MAXTIDENT] = 0;
    }
    else
    {
        /* parse command line */
        if (nstd_parse_clopt(clopt, EXTRUE,  argc, argv, EXFALSE))
        {
            fprintf(stderr, XADMIN_INVALID_OPTIONS_MSG);
            EXFAIL_OUT(ret);
        }
    }
    
    
    if (EXFAIL!=srvid || EXEOS!=srvnm[0])
    {
        keep_running_ndrxd = EXTRUE;
    }
    
    
    memset(&cmd, 0, sizeof(cmd));
    
    cmd.ndrxd_cmd = NDRXD_COM_STOP_RQ;
    if (EXSUCCEED==(ret=cmd_stop(&cmd, argc, argv, p_have_next)))
    {
        if (!keep_running_ndrxd && EXEOS==srvnm[0] && EXFAIL==srvid)
        {
            /* let daemon to finish the exit process (unlink pid file/queues) */
            sleep(2); /* this will be interrupted when we got sig child */
            if (!is_ndrxd_running() && EXFAIL==ndrx_start_idle())
            {
                fprintf(stderr, "Failed to start idle instance of ndrxd!");
                EXFAIL_OUT(ret);
            }
        }
        
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
expublic int cmd_cabort(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    command_call_t call;
    memset(&call, 0, sizeof(call));
    int ret=EXSUCCEED;

    if (!chk_confirm_clopt("Are you sure you want to abort app domain start/stop?", argc, argv))
    {
        ret=EXFAIL;
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
                        EXFALSE);
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
expublic int cmd_sreload(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret=EXSUCCEED;
    command_startstop_t call;
    short srvid=EXFAIL;
    char srvnm[MAXTIDENT+1]={EXEOS};
    short confirm = EXFALSE;
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
	NDRX_STRNCPY(srvnm, argv[1], MAXTIDENT);
	srvnm[MAXTIDENT] = 0;
    }
    else
    {
        /* parse command line */
        if (nstd_parse_clopt(clopt, EXTRUE,  argc, argv, EXFALSE))
        {
            fprintf(stderr, XADMIN_INVALID_OPTIONS_MSG);
            EXFAIL_OUT(ret);
        }
    }
    
    memset(&call, 0, sizeof(call));
    
    if (EXFAIL!=srvid && EXEOS!=srvnm[0])
    {
        fprintf(stderr, "-i and -s cannot be combined!\n");
        EXFAIL_OUT(ret);
    }
    
    if (EXFAIL==srvid && EXEOS==srvnm[0] &&
          !chk_confirm("Are you sure you want to start application?", confirm))
    {
        EXFAIL_OUT(ret);
    }
    
    /* prepare for call */
    call.srvid = srvid;
    NDRX_STRCPY_SAFE(call.binary_name, srvnm);
    
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
                        EXFALSE);
out:
    return ret;
}
