/**
 * @brief Implementation for start & stop commands.
 *
 * @file cmd_startstop.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2023, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * AGPL (with Java and Go exceptions) or Mavimax's license for commercial use.
 * See LICENSE file for full text.
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
#include <lcfint.h>
#include <singlegrp.h>

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
 * @return static test from console output
 */
exprivate char *proc_state_to_str(long state, short msg_type)
{
    static char *started = "Started";
    static char *not_started = "Not started";
    static char *died = "Died";
    static char *shutdown = "Shutdown succeeded";
    static char *stopping = "Shutdown in progress";
    static char *nosuchfile = "No such file or directory";
    static char *eaccess = "Access denied";
    static char *ebadfile = "Bad executable";
    static char *elimits = "Limits exceeded";
    static char *stillstarting = "Still starting";
    static char *eargslim = "Cli args on env params too long";
    static char *eenv= "Environment setup failure";
    static char *esys= "System failure";
    static char *restart= "Restarting";
    static char *ewait= "Waiting on group lock";
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
        case NDRXD_PM_EACCESS:
            ret = eaccess;
            break;
        case NDRXD_PM_EBADFILE:
            ret = ebadfile;
            break;
        case NDRXD_PM_ELIMIT:
            ret = elimits;
            break;
        case NDRXD_PM_STOPPING:
            ret = stopping;
            break;
        case NDRXD_PM_EARGSLIM:
            ret = eargslim;
            break;
        case NDRXD_PM_EENV:
            ret = eenv;
            break;
        case NDRXD_PM_ESYSTEM:
            ret = esys;
            break;
        case NDRXD_PM_RESTART:
            ret = restart;
            break;
        case NDRXD_PM_WAIT:
            ret = ewait;
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
    char procgrp[MAXTIDENT+1]={EXEOS};
    short confirm = EXFALSE;
    short keep_running_ndrxd;
    int ids=0;
    short include_lp=EXFALSE;

    ncloptmap_t clopt[] =
    {
        {'i', BFLD_SHORT, (void *)&srvid, 0, 
                                NCLOPT_OPT|NCLOPT_HAVE_VALUE, "Server ID"},
        {'s', BFLD_STRING, (void *)srvnm, sizeof(srvnm), 
                                NCLOPT_OPT|NCLOPT_HAVE_VALUE, "Server name"},
        {'g', BFLD_STRING, (void *)procgrp, sizeof(procgrp), 
                                NCLOPT_OPT|NCLOPT_HAVE_VALUE, "Process group"},
        {'L', BFLD_SHORT,  (void *)&include_lp, 0, 
                                NCLOPT_OPT|NCLOPT_TRUEBOOL, 
                                "Include lock provider of the process group"},
        {'y', BFLD_SHORT, (void *)&confirm, 0, 
                                NCLOPT_OPT|NCLOPT_TRUEBOOL, "Confirm"},
        {'k', BFLD_SHORT, (void *)&keep_running_ndrxd, 0, 
                                NCLOPT_OPT|NCLOPT_TRUEBOOL, "Keep ndrxd running"},
        {0}
    };
        
    if (argc>=2 && '-'!=argv[1][0])
    {
	    NDRX_STRCPY_SAFE(srvnm, argv[1]);
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

    if (EXFAIL!=srvid)
    {
        ids++;
    }

    if (EXEOS!=srvnm[0])
    {
        ids++;
    }

    if (EXEOS!=procgrp[0])
    {
        ids++;
    }

    if (ids>1)
    {
        fprintf(stderr, "-i, -s and -g cannot be combined!\n");
        EXFAIL_OUT(ret);
    }
    
    /* ask for -y for -g too: */
    if (EXFAIL==srvid && EXEOS==srvnm[0] && EXEOS==procgrp[0] &&
          !ndrx_chk_confirm("Are you sure you want to start application?", confirm))
    {
        EXFAIL_OUT(ret);
    }
    
    /* prepare for call */
    call.srvid = srvid;
    NDRX_STRCPY_SAFE(call.binary_name, srvnm);
    NDRX_STRCPY_SAFE(call.procgrp, procgrp);

    if (include_lp)
    {
        call.flags|=NDRXD_CALL_FLAGS_LP2GRP;
    }

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
                        EXFALSE,
                        G_config.listcall_flags);
out:
    return ret;
}

/**
 * Shutdown application server.
 * TODO: xadmin stop -i -1 makes stop to all domain!
 * If there was full shutdown, then we should disconnect from shared memory
 * resources too...
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
    char procgrp[MAXTIDENT+1]={EXEOS};
    short confirm = EXFALSE;
    short keep_running_ndrxd = EXFALSE;
    short force_off = EXFALSE;  /* force shutdown (for malfunction/no pid instances) */
    short dummy;
    short keep_lcf=EXFALSE;
    pid_t pid = EXFAIL;
    int ids=0;
    short include_lp=EXFALSE;

    ncloptmap_t clopt[] =
    {
        {'i', BFLD_SHORT, (void *)&srvid, 0, 
                                NCLOPT_OPT|NCLOPT_HAVE_VALUE, "Server ID"},
        {'s', BFLD_STRING, (void *)srvnm, sizeof(srvnm), 
                                NCLOPT_OPT|NCLOPT_HAVE_VALUE, "Server name"},
        {'y', BFLD_SHORT, (void *)&confirm, 0, 
                                NCLOPT_OPT|NCLOPT_TRUEBOOL, "Confirm"},
        {'g', BFLD_STRING, (void *)procgrp, sizeof(procgrp), 
                                NCLOPT_OPT|NCLOPT_HAVE_VALUE, "Process group"},
        {'L', BFLD_SHORT,  (void *)&include_lp, 0, 
                                NCLOPT_OPT|NCLOPT_TRUEBOOL, 
                                "Include lock provider of the process group"},
        {'c', BFLD_SHORT, (void *)&dummy, 0, 
                                NCLOPT_OPT|NCLOPT_TRUEBOOL, "Left for compatibility"},
        {'k', BFLD_SHORT, (void *)&keep_running_ndrxd, 0, 
                                NCLOPT_OPT|NCLOPT_TRUEBOOL, "Keep ndrxd running"},
        {'f', BFLD_SHORT, (void *)&force_off, 0, 
                                NCLOPT_OPT|NCLOPT_TRUEBOOL, "Force shutdown"},
        {'l', BFLD_SHORT, (void *)&keep_lcf, 0, 
                                NCLOPT_OPT|NCLOPT_TRUEBOOL, "Keep lcf commands"},

        {0}
    };
        
    if (argc>=2 && '-'!=argv[1][0])
    {
	    NDRX_STRCPY_SAFE(srvnm, argv[1]);
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
    
    if (EXFAIL!=srvid)
    {
        ids++;
    }

    if (EXEOS!=srvnm[0])
    {
        ids++;
    }

    if (EXEOS!=procgrp[0])
    {
        ids++;
    }

    if (ids>1)
    {
        fprintf(stderr, "-i, -s and -g cannot be combined!\n");
        EXFAIL_OUT(ret);
    }
    
    if (EXFAIL!=srvid || EXEOS!=srvnm[0])
    {
        keep_running_ndrxd = EXTRUE;
    }
    
    if (EXFAIL==srvid && EXEOS==srvnm[0] && EXEOS==procgrp[0] &&
          !ndrx_chk_confirm("Are you sure you want to stop application?", confirm))
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
    
    if (call.complete_shutdown && !is_ndrxd_running(&pid, EXFALSE) && !force_off)
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
    NDRX_STRCPY_SAFE(call.procgrp, procgrp);
    if (include_lp)
    {
        call.flags|=NDRXD_CALL_FLAGS_LP2GRP;
    }

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
                    EXFALSE,
                    G_config.listcall_flags);
    
    /* Do not wait in case if sending was not successful */
    if (call.complete_shutdown && (EXSUCCEED==ret || force_off))
    {
        NDRX_LOG(log_debug, "About to un-init after shutdown");
        un_init(EXTRUE);
        NDRX_LOG(log_debug, "Un-init completed (after shutdown)");
        /* TODO: 
         * how about some sleep here to allow the ndrxd to kill shared resources
         * before user might want to move forward with next commands which open
         * shm resources. Thus we can get some race conditions here.
         * Maybe monitor until the all resources are unlinked...
         * or at least pid is dead.
         */
        
        if (EXFAIL!=pid)
        {
            /* monitor the pid */
            while (EXSUCCEED==kill(pid, 0))
            {
                /* have some usleep -> for aix needs to check sigchilds... */
                usleep(100000);
                /* zap any childs..
                sign_chld_handler(0);*/
            }
            /* shutdown is ok */
            ret = EXSUCCEED;
        }
        else
        {
            /* no info of pid, so have some sleep */
            sleep(1);
        }
        
        /* 
         * clean up the commands... 
         */
        if (!keep_lcf)
        {
            ndrx_lcf_reset();
        }

        /* Reset all singlegrp infos */
        ndrx_sg_reset();
    }

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
    short srvid=EXFAIL;
    char srvnm[MAXTIDENT+1]={EXEOS};
    char procgrp[MAXTIDENT+1]={EXEOS};
    short confirm = EXFALSE;
    short keep_running_ndrxd = EXFALSE;
    int ids=0;
    short include_lp=EXFALSE;

    /* just verify that content is ok: */
    ncloptmap_t clopt[] =
    {
        {'i', BFLD_SHORT, (void *)&srvid, 0, 
                                NCLOPT_OPT|NCLOPT_HAVE_VALUE, "Server ID"},
        {'s', BFLD_STRING, (void *)srvnm, sizeof(srvnm), 
                                NCLOPT_OPT|NCLOPT_HAVE_VALUE, "Server name"},
        {'g', BFLD_STRING, (void *)procgrp, sizeof(procgrp), 
                                NCLOPT_OPT|NCLOPT_HAVE_VALUE, "Process group"},
        {'L', BFLD_SHORT,  (void *)&include_lp, 0, 
                                NCLOPT_OPT|NCLOPT_TRUEBOOL, 
                                "Include lock provider of the process group"},
        {'y', BFLD_SHORT, (void *)&confirm, 0, 
                                NCLOPT_OPT|NCLOPT_TRUEBOOL, "Confirm"},
        {'k', BFLD_SHORT, (void *)&keep_running_ndrxd, 0, 
                                NCLOPT_OPT|NCLOPT_TRUEBOOL, "Keep ndrxd running"},
        {0}
    };
        
    if (argc>=2 && '-'!=argv[1][0])
    {
	    NDRX_STRCPY_SAFE(srvnm, argv[1]);
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
    
    NDRX_LOG(log_info, "Shutting down...");
    strcpy(argv[0], "stop");
    if (EXSUCCEED==(ret=process_command_buffer(EXFALSE)))
    {
        if (!keep_running_ndrxd && EXEOS==srvnm[0] && EXFAIL==srvid)
        {
            sleep(2);
        }
        NDRX_LOG(log_debug, "Starting up...");
        strcpy(argv[0], "start"); 
        ret=process_command_buffer(EXFALSE);
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
                        EXFALSE,
                        G_config.listcall_flags);
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
    char procgrp[MAXTIDENT+1]={EXEOS};
    short confirm = EXFALSE;
    int ids=0;
    short include_lp=EXFALSE;
    ncloptmap_t clopt[] =
    {
        {'i', BFLD_SHORT, (void *)&srvid, 0, 
                                NCLOPT_OPT|NCLOPT_HAVE_VALUE, "Server ID"},
        {'s', BFLD_STRING, (void *)srvnm, sizeof(srvnm), 
                                NCLOPT_OPT|NCLOPT_HAVE_VALUE, "Server name"},
        {'g', BFLD_STRING, (void *)procgrp, sizeof(procgrp), 
                                NCLOPT_OPT|NCLOPT_HAVE_VALUE, "Process group"},
        {'L', BFLD_SHORT,  (void *)&include_lp, 0, 
                                NCLOPT_OPT|NCLOPT_TRUEBOOL, 
                                "Include lock provider of the process group"},
        {'y', BFLD_SHORT, (void *)&confirm, 0, 
                                NCLOPT_OPT|NCLOPT_TRUEBOOL, "Confirm"},
        {0}
    };
        
    if (argc>=2 && '-'!=argv[1][0])
    {
	    NDRX_STRCPY_SAFE(srvnm, argv[1]);
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
    
    if (EXFAIL!=srvid)
    {
        ids++;
    }

    if (EXEOS!=srvnm[0])
    {
        ids++;
    }

    if (EXEOS!=procgrp[0])
    {
        ids++;
    }

    if (ids>1)
    {
        fprintf(stderr, "-i, -s and -g cannot be combined!\n");
        EXFAIL_OUT(ret);
    }
    
    if (EXFAIL==srvid && EXEOS==srvnm[0] && EXEOS==procgrp[0] &&
          !ndrx_chk_confirm("Are you sure you want to start application?", confirm))
    {
        EXFAIL_OUT(ret);
    }
    
    /* prepare for call */
    call.srvid = srvid;
    NDRX_STRCPY_SAFE(call.binary_name, srvnm);
    NDRX_STRCPY_SAFE(call.procgrp, procgrp);

    if (include_lp)
    {
        call.flags|=NDRXD_CALL_FLAGS_LP2GRP;
    }

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
                        EXFALSE,
                        G_config.listcall_flags);
out:
    return ret;
}
/* vim: set ts=4 sw=4 et smartindent: */
