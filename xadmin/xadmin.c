/* 
** Enduro X administration utility.
**
** @file xadmin.c
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
#include <sys/param.h>
#include <unistd.h>

#include <ndrstandard.h>
#include <ndebug.h>

#include "ndrx.h"
#include "cconfig.h"
#include <ndrxdcmn.h>
#include <gencall.h>
#include <errno.h>
#include <sys_unix.h>
#include <inicfg.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

#define PARSE_CMD_STUFF(X)\
        if (0==strncmp(X, "-", 1))\
        {\
            G_cmd_argc_logical++;\
        }
            
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/* Command line arguments */
private int M_argc;
private char **M_argv;

/* Final command buffer  */
public int G_cmd_argc_logical; /* logical */
public int G_cmd_argc_raw; /* raw strings in argv */

public ndrx_inicfg_section_keyval_t *G_xadmin_config = NULL;
public char G_xadmin_config_file[PATH_MAX+1] = {EOS};

/* if we read from stdin: */
public char *G_cmd_argv[MAX_ARGS]; /* Assume max 50 arguments */
private char M_buffer[CMD_MAX];
private char M_buffer_prev[CMD_MAX]={EOS};  /* Previous command (for interactive terminal) */
private int M_quit_requested = FALSE;       /* Mark the system state that we want exit! */

/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

private int cmd_quit(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);
private int cmd_echo(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);
private int cmd_ver(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);
private int cmd_idle(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);
private int cmd_help(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);
private int cmd_stat(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);

/**
 * Initialize command mapping table
 */
cmd_mapping_t M_command_map[] = 
{
    {"quit",    cmd_quit,  FAIL,                 1,  1,  0, "Quit from command line utility", NULL},
    {"q",       cmd_quit,  FAIL,                 1,  1,  0, "Alias for `quit'", NULL},
    {"echo",    cmd_echo,  FAIL,                 1,  999,0, "Echo text back to terminal", NULL},
    {"idle",    cmd_idle,  FAIL,                 1,  1,  0, "Enter daemon process in idle state (if not started)", NULL},
    {"help",    cmd_help,  FAIL,                 1,  2,  0, "Print help (this output)\n"
                                                "\t\targs: help [command]", NULL},
    {"h",       cmd_help,  FAIL,                 1,  2,  0, "Alias for `help'", NULL},
    {"info",    cmd_help,  FAIL,                 1,  2,  0, "Alias for `help'", NULL},
    {"stat",    cmd_stat,  FAIL,                 1,  1,  0, "Prints general status information", NULL},
    {"ldcf",    cmd_ldcf,  NDRXD_COM_LDCF_RQ,    1,  1,  1, "Load configuration", NULL},
    {"start",   cmd_start, NDRXD_COM_START_RQ,   1,  4,  1, "Start application domain\n"
                                                         "\t\tAlso loads configuration automatically.\n"
                                                         "\t\targs: start [-y] [-s <server>] [-i <srvid>]", NULL},
    {"psc",     cmd_psc,   NDRXD_COM_PSC_RQ,     1,  1,  1, "Print services", NULL},
    {"stop",    cmd_stop,  NDRXD_COM_STOP_RQ,    1,  4,  0, "Stop application domain\n"
                                                         "\t\targs: stop [-y] [-c]|[-s <server>] [-i <srvid>]", NULL},
    {"down",    cmd_fdown, FAIL,                 1,  2,  0, "Force appserver shuttdown & resurce cleanup\n"
                                                         "\t\targs: fdown [-y]\n"
                                                         "\t\tRUN ONLY IF YOU KNOW WHAT YOU ARE DOING!", NULL},
    {"cat",     cmd_cat,    NDRXD_COM_AT_RQ,      1,  1,  1, "Attached to ndrxd user session in progress", NULL},
    {"reload",  cmd_reload,NDRXD_COM_RELOAD_RQ,  1,  1,  1, "Load new configuration", NULL},
    {"testcfg", cmd_testcfg,NDRXD_COM_TESTCFG_RQ,1,  1,  1, "Test new configuration", NULL},
    {"unadv",   cmd_unadv,NDRXD_COM_XADUNADV_RQ, 5,  5,  1,"Un-advertise service.\n"
                                                         "\t\targs: unadv -i server_id -s service_name", NULL},
    {"readv",   cmd_unadv,NDRXD_COM_XADREADV_RQ, 5,  5,  1,"Re-advertise service.\n"
                                                         "\t\targs: readv -i server_id -s service_name\n"
                                                         "\t\tmight be usable if service Q was unlinked", NULL},
    {"restart", cmd_r,    FAIL,                 1,  4,  0, "Restart app or service (invokes start & stop with same args!)", NULL},
    {"r", cmd_r,    FAIL,                       1,  4,  0, "Alias for `restart'", NULL},
    {"-v",      cmd_ver,  FAIL,                 1,  1,  0, "Print version info", NULL},
    {"ver",     cmd_ver,  FAIL,                 1,  1,  0, "Print version info, same as -v", NULL},
    {"ppm",     cmd_ppm,  NDRXD_COM_XAPPM_RQ,   1,  1,  1, "Print process model", NULL},
    {"psvc",cmd_shm_psvc,NDRXD_COM_XASHM_PSVC_RQ,   1,  1,  1, "Shared mem, print services", NULL},
    {"psrv",cmd_shm_psrv,NDRXD_COM_XASHM_PSRV_RQ,   1,  1,  1, "Shared mem, print servers", NULL},
    {"cabort",cmd_cabort,NDRXD_COM_XACABORT_RQ,   1,  2,  1, "Abort app shutdown or startup.\n"
                                                           "\t\targs: abort [-y]", NULL},
    {"sreload", cmd_sreload, NDRXD_COM_SRELOAD_RQ,   1,  3,  1, "Restarts server instance by instance\n"
                                                         "\t\tArgs: sreload [-y] [-s <server>] [-i <srvid>]", NULL},
    {"sr", cmd_sreload, NDRXD_COM_SRELOAD_RQ,   1,  3,  1, "Alias for `sreload'", NULL},
    {"pq",cmd_pq,NDRXD_COM_XAPQ_RQ,   1,  1,  1, "Print service queues", NULL},
    {"pqa",cmd_pqa,  FAIL,            1,  2,  0, "Print all queues\n"
                                                "\t\targs: pqa [-a]\n"
                                                "\t\t-a - print all queues "
                                                "(incl. other local systems)", NULL},
    /* New XA commands: printtrans (pt), abort, commit, exsting abort move to: sabort (start/stop) 
     * abort + install ctrl+c handler 
     */
    {"pt",        cmd_pt,FAIL,   1,  1,  1, "Print transactions", NULL},
    {"printtrans",cmd_pt,FAIL,   1,  1,  1, "Alias for `pt'", NULL},
    {"abort",     cmd_abort,FAIL,   3,  5,  1, "Abort transaction\n"
                                            "\t\targs: abort -t <RM Ref> -x <XID> [-g <Group No>] [-y]", NULL},
    {"aborttrans",cmd_abort,FAIL,   3,  5,  1, "Alias for `abort'", NULL},
    {"commit",     cmd_commit,FAIL,   3,  4,  1, "Commit transaction\n"
                                            "\t\targs: commit -t <RM Ref> -x <XID> [-y]", NULL},
    {"committrans",cmd_commit,FAIL,   3,  4,  1, "Alias for `commit'", NULL},
    {"pe",        cmd_pe,NDRXD_COM_PE_RQ,   1,  1,  1, "Print env (from ndrxd)", NULL},
    {"printenv",  cmd_pe,NDRXD_COM_PE_RQ,   2,  2,  1, "Alias for `pe'", NULL},
    {"set",       cmd_set,NDRXD_COM_SET_RQ,   1,  99,  1, "Set env variable. \n"
                                                "\t\tFormat: VAR=SOME VALUE (will be contact with spaces)", NULL},
    {"unset",     cmd_unset,NDRXD_COM_UNSET_RQ,   1,  1,  1, "Set env variable. Forma: VAR"},
    {"pc",        cmd_pc,FAIL,   1,  1,  1, "Print clients", NULL},
    {"sc",        cmd_sc,FAIL,   2,  3,  1, "Stop client\n"
                                    "\t\targs: sc -t <Tag> [-s <Subsection (default -)]", NULL},
    {"bc",        cmd_bc,FAIL,   2,  3,  1, "Boot(start) client\n"
                                    "\t\targs: bc -t <Tag> [-s <Subsection (default -)]", NULL},
    {"mqlc",      cmd_mqlc,FAIL,   1,  1,  1, "List persistent queue configuration", NULL},
    {"mqlq",      cmd_mqlq,FAIL,   1,  1,  1, "List persistent queues (active/dynamic)", NULL},
    {"mqrc",      cmd_mqrc,FAIL,   1,  1,  1, "Reload TMQ config", NULL},
    {"mqlm",      cmd_mqlm,FAIL,   3,  3,  1, "List messages in q\n"
                                    "\t\targs: mqlm -s <QSpace> -q <QName>", NULL},
    {"mqdm",      cmd_mqdm,FAIL,   4,  4,  1, "Dump/peek message to stdout\n"
                                    "\t\targs: mqdm -n <Cluster node id> -i <Server ID> -m <Message ID>", NULL},
    {"mqch",      cmd_mqch,FAIL,   4,  4,  1, "Change queue config (runtime only)\n"
                                    "\t\targs: mqch -n <Cluster node id> -i <Server ID> -q <Q def (conf format)>", NULL},
    {"mqrm",      cmd_mqrm,FAIL,   4,  4,  1, "Remove message from Q space\n"
                                    "\t\targs: mqrm -n <Cluster node id> -i <Server ID> -m <Message ID>\n", NULL},
    {"mqmv",      cmd_mqmv,FAIL,   6,  6,  1, "Move message to different qspace/qname\n"
                                    "\t\targs: mqmv -n <Source cluster node id> -i <Source server ID>\n"
                                    "\t\t-m <Source Message ID> -s <Dest qspace> -q <Dest qname>", NULL},
    {"killall",   cmd_killall,FAIL, 1,  999,  0, "Kill all processes (in ps -ef) matching the name\n"
                                    "\t\targs: killall <name1> <name2> ... <nameN>", NULL},
    {"qrm",       cmd_qrm,FAIL, 1,  999,  0, "Remove specific queue"
                                    "\t\targs: qrm <qname1> <qname2> ... <qnameN>\n", NULL},
    {"qrmall",    cmd_qrmall,FAIL, 1,  999,  0, "Remove queue matching the substring \n"
                                    "\t\targs: qrmall <substr1> <substr2> ... <substrN>", NULL},
    {"provision", cmd_provision,FAIL, 0,  999,  0, "Prepare initial Enduro/X instance environment \n"
                                     "\t\targs: provision [-d] [-v<param1>=<value1>] ... [-v<paramN>=<valueN>]\n", NULL},
    {"gen",       cmd_gen,FAIL, 0,  999,  0, "Generate sources\n"
                                     "\t\targs: gen <language> <type> [-d] "
                                     "[-v<param1>=<value1>] ... [-v<paramN>=<valueN>]\n"
                                     "\t\tsub-targets available:", cmd_gen_help}
};

/*
 * List of commands that does not require init
 */
char *M_noinit[] = {
    "provision"
    ,"help"
    ,"h"
    ,"killall"
    ,"gen"
};

/**
 * Common call arguments - all commands (even not related) must be listed here!
 */
gencall_args_t G_call_args[] =
{
    {NDRXD_COM_LDCF_RQ,     NULL,           simple_output,  TRUE},/*0*/
    {NDRXD_COM_LDCF_RP,     NULL,           NULL,           FALSE},/*1*/
    {NDRXD_COM_START_RQ,    ss_rsp_process, simple_output,  TRUE},/*2*/
    {NDRXD_COM_START_RP,    NULL,           NULL,           FALSE},/*3*/
    {NDRXD_COM_SVCINFO_RQ,  NULL,           NULL,           FALSE},/*4*/
    {NDRXD_COM_SVCINFO_RP,  NULL,           NULL,           FALSE},/*5*/
    {NDRXD_COM_PMNTIFY_RQ,  NULL,           NULL,           FALSE},/*6*/
    {NDRXD_COM_PMNTIFY_RP,  NULL,           NULL,           FALSE},/*7*/
    {NDRXD_COM_PSC_RQ,      psc_rsp_process,simple_output,  TRUE},/*8*/
    {NDRXD_COM_PSC_RP,      NULL,           NULL,           FALSE},/*9*/
    {NDRXD_COM_STOP_RQ,     ss_rsp_process, simple_output,  TRUE},/*10*/
    {NDRXD_COM_STOP_RP,     NULL,           NULL,           FALSE},/*11*/
    {NDRXD_COM_SRVSTOP_RQ,  NULL,           NULL,           FALSE},/*12*/
    {NDRXD_COM_SRVSTOP_RP,  NULL,           NULL,           FALSE},/*13*/
    {NDRXD_COM_AT_RQ,       at_rsp_process, simple_output,  TRUE},/*14*/
    {NDRXD_COM_AT_RP,       NULL,           NULL,           FALSE},/*15*/
    {NDRXD_COM_RELOAD_RQ,reload_rsp_process,simple_output,  TRUE},/*16*/
    {NDRXD_COM_RELOAD_RP,   NULL,           NULL,           FALSE},/*17*/
    {NDRXD_COM_TESTCFG_RQ,reload_rsp_process,simple_output,  TRUE},/*18*/
    {NDRXD_COM_TESTCFG_RP,   NULL,          NULL,           FALSE},/*19*/
    {NDRXD_COM_SRVINFO_RQ,   NULL,          NULL,           FALSE},/*20*/
    {NDRXD_COM_SRVINFO_RP,   NULL,          NULL,           FALSE},/*21*/
    {NDRXD_COM_SRVUNADV_RQ,   NULL,         NULL,           FALSE},/*22*/
    {NDRXD_COM_SRVUNADV_RP,   NULL,         NULL,           FALSE},/*23*/
    {NDRXD_COM_XADUNADV_RQ,   NULL,         simple_output,  TRUE},/*24*/
    {NDRXD_COM_XADUNADV_RP,   NULL,         NULL,           FALSE},/*25*/
    {NDRXD_COM_NXDUNADV_RQ,   NULL,         NULL,           FALSE},/*26*/
    {NDRXD_COM_NXDUNADV_RP,   NULL,         NULL,           FALSE},/*27*/
    {NDRXD_COM_SRVADV_RQ,     NULL,         NULL,           FALSE},/*28*/
    {NDRXD_COM_SRVADV_RP,     NULL,         NULL,           FALSE},/*29*/
    {NDRXD_COM_XAPPM_RQ,      ppm_rsp_process,simple_output,TRUE}, /*30*/
    {NDRXD_COM_XAPPM_RP,      NULL,         NULL,         FALSE},/*31*/
    {NDRXD_COM_XASHM_PSVC_RQ, shm_psvc_rsp_process,simple_output,TRUE},/*31*/
    {NDRXD_COM_XASHM_PSVC_RP, NULL,         NULL,         FALSE},/*32*/
    {NDRXD_COM_XASHM_PSRV_RQ, shm_psrv_rsp_process,simple_output,TRUE},/*33*/
    {NDRXD_COM_XASHM_PSRV_RP, NULL,         NULL,         FALSE},/*34*/
    {NDRXD_COM_NXDREADV_RQ,   NULL,         NULL,         FALSE},/*36*/
    {NDRXD_COM_NXDREADV_RP,   NULL,         NULL,         FALSE},/*37*/
    {NDRXD_COM_XADREADV_RQ,   NULL,         simple_output,TRUE },/*38*/
    {NDRXD_COM_XADREADV_RP,   NULL,         NULL,         FALSE}, /*39*/
    {NDRXD_COM_XACABORT_RQ,    NULL,         simple_output,TRUE },/*40*/
    {NDRXD_COM_XAABORT_RP,    NULL,         NULL,         FALSE}, /*41*/
    {NDRXD_COM_BRCON_RQ,      NULL,         NULL,         FALSE},/*42*/
    {NDRXD_COM_BRCON_RP,      NULL,         NULL,         FALSE},/*43*/
    {NDRXD_COM_BRDISCON_RQ,   NULL,         NULL,         FALSE},/*44*/
    {NDRXD_COM_BRDISCON_RP,   NULL,         NULL,         FALSE},/*45*/
    {NDRXD_COM_BRREFERSH_RQ,  NULL,         NULL,         FALSE},/*46*/
    {NDRXD_COM_BRREFERSH_RP,  NULL,         NULL,         FALSE},/*47*/
    {NDRXD_COM_BRCLOCK_RQ,    NULL,         NULL,         FALSE},/*48*/
    {NDRXD_COM_BRCLOCK_RP,    NULL,         NULL,         FALSE},/*49*/
    {NDRXD_COM_SRVGETBRS_RQ,  NULL,         NULL,         FALSE},/*50*/
    {NDRXD_COM_SRVGETBRS_RP,  NULL,         NULL,         FALSE},/*51*/
    {NDRXD_COM_SRVPING_RQ,    NULL,         NULL,         FALSE},/*52*/
    {NDRXD_COM_SRVPING_RP,    NULL,         NULL,         FALSE}, /*53*/
    {NDRXD_COM_SRELOAD_RQ,    ss_rsp_process, simple_output,TRUE},/*54*/
    {NDRXD_COM_SRELOAD_RP,    NULL,           NULL,        FALSE},/*55*/
    {NDRXD_COM_XAPQ_RQ,       pq_rsp_process,simple_output,TRUE},/*56*/
    {NDRXD_COM_XAPQ_RP,       NULL,         NULL,         FALSE},/*57*/
    {NDRXD_COM_PE_RQ,         pe_rsp_process,simple_output,TRUE},/*58*/
    {NDRXD_COM_PE_RP,         NULL,         NULL,         FALSE},/*59*/
    {NDRXD_COM_SET_RQ,NULL,simple_output,      TRUE},/*60*/
    {NDRXD_COM_SET_RP,        NULL,           NULL,       FALSE},/*61*/
    {NDRXD_COM_UNSET_RQ,NULL,simple_output,    TRUE},/*62*/
    {NDRXD_COM_UNSET_RP,       NULL,           NULL,      FALSE},/*63*/
};

/**
 * Display EnduroX Version string
 */
private int cmd_ver(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    printf(NDRX_VERSION
		"\n");
    return SUCCEED;
}

/**
 * Show help/command summary
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
private int cmd_help(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int i;
    cmd_mapping_t *p;
    /**/
    printf("COMMAND\tDESCR\n");
    printf("-------\t------------------------------------------------------------------------\n");
    for (i=0; i<N_DIM(M_command_map); i++)
    {
        p = &M_command_map[i];
        if ((argc>1 && 0==strcmp(p->cmd, argv[1])) || 1==argc)
        {
            if (NULL==p->p_add_help)
            {
                printf("%s\t%s\n\n", p->cmd, p->help);
            }
            else
            {
                printf("%s\t%s\n", p->cmd, p->help);
            }
            
            if (p->p_add_help)
            {
                p->p_add_help();
            }
        }
    }
    return SUCCEED;
}

/**
 * Start idle instance (if backend does not exists!)
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
private int cmd_idle(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret=SUCCEED;

    if (!is_ndrxd_running() && NDRXD_STAT_NOT_STARTED==G_config.ndrxd_stat)
    {
        ret = start_daemon_idle();
    }
    else
    {
        fprintf(stderr, "Cannot start idle in current state!\n");
    }
            
    return ret;
}

/**
 * prints current status info
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
private int cmd_stat(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret=SUCCEED;
#define _STATUS_FMT     "STATUS: %s\n"

    /* Test current status... */
    is_ndrxd_running();
    
    if (NDRXD_STAT_NOT_STARTED==G_config.ndrxd_stat)
    {
        printf(_STATUS_FMT, "Not started");
    }
    else if (NDRXD_STAT_MALFUNCTION==G_config.ndrxd_stat)
    {
        printf(_STATUS_FMT, "Malfunction");
    }
    else
    {
        printf(_STATUS_FMT, "Running OK");
    }
    
    /* This is not used!
    printf("IDLE  : %s\n", G_config.is_idle?"YES":"NO");
    */
    return ret;
}


/**
 * Quit command 
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
private int cmd_quit(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    NDRX_LOG(log_debug, "cmd_quit called");
    M_quit_requested = TRUE;
    *p_have_next = FALSE;
    return SUCCEED;
}

/**
 * echo command
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
private int cmd_echo(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int i;
    NDRX_LOG(log_debug, "cmd_echo called");

    for (i=1; i< argc; i++)
    {
        printf("%s ", argv[i]);
    }
    
    printf("\n");

    return SUCCEED;
}

/**
 * Simple output
 * @param buf
 */
public void simple_output(char *buf)
{
    fprintf(stderr, "%s", buf);
    fprintf(stderr, "\n");
}

/**
 * Checks is terminal tty.
 * @return 
 */
private int is_tty()
{
    return isatty(0);
}

/**
 * Reads the command either from command line or from stdin.
 * If from stdin, then command gets splitted.
 * @return SUCCEED/FAIL
 */
private int get_cmd(int *p_have_next)
{
    int ret=SUCCEED;
    int i;

    G_cmd_argc_logical = 0;
    G_cmd_argc_raw = 0;

    if (M_argc > 1)
    {
        /* Operate with command line args */   
        for (i=1; i< M_argc && G_cmd_argc_raw<MAX_ARGS; i++)
        {
            PARSE_CMD_STUFF(M_argv[i]);
            
            /* have something (the command) */
            if (!G_cmd_argc_logical)
                G_cmd_argc_logical = 1;
            
            /* Migrate to raw format: */
            strcpy(G_cmd_argv[i-1], M_argv[i]);
            G_cmd_argc_raw++;
        }
        
        *p_have_next = FALSE;
        
    }
    else /* Operate with stdin. */
    {
        char *p;

        memset(M_buffer, 0, sizeof(M_buffer));

        /* Welcome only if it is terminal */
        if (is_tty())
            printf("NDRX> ");

        /* We should get something! */
        while (NULL==fgets(M_buffer, sizeof(M_buffer), stdin))
        {
            /* if we do not have tty, then exit */
            if (!is_tty())
            {
                /* do not have next */
                *p_have_next = FALSE;
                goto out;
            }
        }

        /* strip off trailing newline */
        if ('\n'==M_buffer[strlen(M_buffer)-1])
            M_buffer[strlen(M_buffer)-1] = EOS;
        
        /* Allow repeated commands */
        if (is_tty())
        {
            if (EOS!=M_buffer_prev[0] && '/' == M_buffer[0])
            {
                strcpy(M_buffer, M_buffer_prev);
            }
            else if (EOS!=M_buffer[0])
            {
                strcpy(M_buffer_prev, M_buffer);
            }
        }
        
        /* Now split the stuff */
        p = strtok (M_buffer, ARG_DEILIM);
        while (p != NULL && G_cmd_argc_raw < MAX_ARGS)
        {
            PARSE_CMD_STUFF(p);
            
            /* have something (the command) */
            if (!G_cmd_argc_logical)
                G_cmd_argc_logical = 1;
            
            strcpy(G_cmd_argv[G_cmd_argc_raw], p);
            G_cmd_argc_raw++;
            
            p = strtok (NULL, ARG_DEILIM);
        }

        /* We have next from stdin, if not quit command executed last */
        if (!M_quit_requested)
            *p_have_next = TRUE;        
    }

out:
    return ret;
}


/**
 * Process requested command buffer.
 * The command by itself should be found in first argument
 * @return SUCCEED/FAIL
 */
private int process_command_buffer(int *p_have_next)
{
    int ret=SUCCEED;
    int i;
    cmd_mapping_t *map=NULL;

    for (i=0; i< N_DIM(M_command_map); i++)
    {
        if (0==strcmp(G_cmd_argv[0], M_command_map[i].cmd))
        {
            map = &M_command_map[i];
            break;
        }
    }

    if (NULL==map)
    {
        fprintf(stderr, "Command `%s' not found!\n", G_cmd_argv[0]);
    }
    else
    {
        /* check min/max args */
        if ( G_cmd_argc_logical < map->min_args)
        {
            fprintf(stderr, "Syntax error, too few args (min %d, max %d, got %d)!\n",
                                map->min_args, map->max_args, G_cmd_argc_logical);
        }
        else if ( G_cmd_argc_logical > map->max_args)
        {
            fprintf(stderr, "Syntax error, too many args (min %d, max %d, got %d)!\n",
                                map->min_args, map->max_args, G_cmd_argc_logical);
        }
        else if (map->reqidle && !is_ndrxd_running() && FAIL==ndrx_start_idle())
        {
            ret=FAIL;
        }
        else
        {
            /* execute function 
            fprintf(stderr, "exec: %s\n", M_cmd_argv[0]); */
            ret = M_command_map[i].p_exec_command(&M_command_map[i],
                                    G_cmd_argc_raw, (char **)G_cmd_argv, p_have_next);
        }
    }
    
    return ret;
}

/**
 * Un-initialize the process
 * @return 
 */
public int un_init(void)
{
    if (G_config.ndrxd_q != (mqd_t)FAIL)
        ndrx_mq_close(G_config.ndrxd_q);

    if (G_config.reply_queue != (mqd_t)FAIL)
    {
        ndrx_mq_close(G_config.reply_queue);
        ndrx_mq_unlink(G_config.reply_queue_str);
    }
    
    /* In any case if session was open... */
    tpterm();

    return SUCCEED;
}

/**
 * Start idle instance.
 * Caller must ensure that backend is not running.
 * @return
 */
public int ndrx_start_idle()
{
    int ret=SUCCEED;

    /* Start idle instance, if background process is not running. */
    if (NDRXD_STAT_NOT_STARTED==G_config.ndrxd_stat)
    {
        ret = start_daemon_idle();
    }
    else if (NDRXD_STAT_MALFUNCTION==G_config.ndrxd_stat)
    {
        fprintf(stderr, "EnduroX back-end (ndrxd) malfunction - see logs!\n");
    }

    return ret;
}


/**
 * Get the file & tests does it exists or not
 * @param buf
 * @param size
 * @param path1
 * @param path2
 * @return TRUE/FALSE
 */
private int get_file(char *buf, size_t size, char *path1, char *path2)
{
    if (NULL!=path1 && NULL!=path2)
    {
        snprintf(buf, size, "%s/%s", path1, path2);
    }
    else if (NULL!=path1)
    {
        /* snprintf(buf, size, "%s", path1); */
        strncpy(buf, path1, size);
        buf[size-1] = EOS;
    }
    else
    {
        return FALSE;
    }
    
    return ndrx_file_exists(buf);
}

/**
 * Initialize Client (and possible start idle back-end)
 * @return
 */
public int ndrx_init(int need_init)
{
    int ret=SUCCEED;
    int i;
    ndrx_inicfg_t *cfg = NULL;
    
#ifdef EX_USE_EMQ
    /* We need to get lock in */
    emq_set_lock_timeout(10);
#endif

    if (need_init)
    {
        if (SUCCEED!=(ret = load_env_config()))
        {
            goto out;
        }
    }
    
    /* Initialize memory for arg array */
    for (i=0; i<MAX_ARGS; i++)
    {
        G_cmd_argv[i] = NDRX_MALLOC(MAX_ARG_LEN);
        if (NULL==G_cmd_argv[i])
        {
            NDRX_LOG(log_error, "Failed to initialise clp array:% s", 
                    strerror(errno));
            ret=FAIL;
            goto out;
        }
    }
    
    /* TODO: Load any config files used by xadmin... 
     * - Try some global var NDRX_XADMIN_CONFIG
     * - Try ~/.xadmin.config
     * - Try /etc/xadmin.config
     * 
     * If found, use the NDRX_CCTAG, and load the config in global variables
     * so that other commands can use it later.
     * The section would be  [@xadmin].
     * 
     * The "provision" and "gen" command will use this.
     */
    
    if (get_file(G_xadmin_config_file, sizeof(G_xadmin_config_file), 
            getenv(CONF_NDRX_XADMIN_CONFIG), "xadmin.config") ||
        get_file(G_xadmin_config_file, sizeof(G_xadmin_config_file), 
            getenv("HOME"), ".xadmin.config") ||
        get_file(G_xadmin_config_file, sizeof(G_xadmin_config_file),
            "/etc/xadmin.config", NULL))
    {
        char cfg_section[128];
        char *cctag = getenv(NDRX_CCTAG);
        
        if (NULL==cctag)
        {
            strcpy(cfg_section, "@xadmin");
        }
        else
        {
            snprintf(cfg_section, sizeof(cfg_section), "@xadmin/%s", cctag);
        }
        
        /* load the configuration file */
        if (NULL==(cfg=ndrx_inicfg_new()))
        {
            NDRX_LOG(log_error, "Failed to create inicfg: %s", Nstrerror(Nerror));
            FAIL_OUT(ret);
        }
        
        /* Add config file */
        if (SUCCEED!=ndrx_inicfg_add(cfg, G_xadmin_config_file, NULL))
        {
             NDRX_LOG(log_error, "Failed to add resource [%s]: %s", 
                    G_xadmin_config_file, Nstrerror(Nerror));
            FAIL_OUT(ret);       
        }
        
        /* Get the section... */
        if (SUCCEED!=ndrx_inicfg_get_subsect(cfg, NULL, cfg_section, &G_xadmin_config))
        {
            NDRX_LOG(log_error, "Failed to resolve config: %s", Nstrerror(Nerror));
            FAIL_OUT(ret);    
        }
    }
    
    if (SUCCEED!=cmd_gen_load_scripts())
    {
        NDRX_LOG(log_error, "Failed to load gen scripts");
        FAIL_OUT(ret);    
    }
    
out:

    if (NULL!=cfg)
    {
        ndrx_inicfg_free(cfg);
    }

    return ret;
}
/*
 * Main Entry point..
 */
int main(int argc, char** argv) {

    int have_next = 1;
    int ret=SUCCEED;
    int need_init = TRUE;
    /* Command line arguments */
    M_argc = argc;
    M_argv = argv;

    if (argc>1)
    {
        int i = 0;
        while (i< N_DIM(M_noinit))
        {
            if (0==strcmp(M_noinit[i], argv[1]))
            {
                need_init=FALSE;
                break;
            }
            
            i++;
        }
    }
    
    if (FAIL==ndrx_init(need_init))
    {
        NDRX_LOG(log_error, "Failed to initialize!");
        fprintf(stderr, "Failed to initialize!\n");
        FAIL_OUT(ret);
    }

    signal(SIGCHLD, sign_chld_handler);
    
    /* Print the copyright notice: */
    if (is_tty())
    {
        fprintf(stderr, "%s, build %s %s, using %s for %s (%ld bits)\n\n", NDRX_VERSION, 
                    __DATE__, __TIME__, ndrx_epoll_mode(), NDRX_BUILD_OS_NAME, sizeof(void *)*8);
        fprintf(stderr, "Enduro/X Middleware Platform for Distributed Transaction Processing\n");
        fprintf(stderr, "Copyright (C) 2015, 2016 ATR Baltic, SIA. All Rights Reserved.\n\n");
        fprintf(stderr, "This software is released under one of the following licenses:\n");
        fprintf(stderr, "GPLv2 (or later) or ATR Baltic's license for commercial use.\n\n");
    }

    /* Main command loop */
    while(SUCCEED==ret && have_next)
    {
        /* Get the command */
        if (FAIL==(ret = get_cmd(&have_next)))
        {
            NDRX_LOG(log_error, "Failed to get command!");
            ret=FAIL;
            goto out;
        }

        /* Now process command buffer */
        if (G_cmd_argc_logical > 0 &&
            SUCCEED!=process_command_buffer(&have_next) &&
                (!have_next || !isatty(0))) /* stop processing if not tty or do not have next */
        {
            ret=FAIL;
            goto out;
        }
        
    }

out:

    if (need_init)
    {
        un_init();
    }
/*
    fprintf(stderr, "xadmin normal shutdown (%d)\n", ret);
*/
    return ret;
}

