/**
 * @brief Enduro X administration utility.
 *
 * @file xadmin.c
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
exprivate int M_argc;
exprivate char **M_argv;

/* Final command buffer  */
expublic int G_cmd_argc_logical; /* logical */
expublic int G_cmd_argc_raw; /* raw strings in argv */

expublic ndrx_inicfg_section_keyval_t *G_xadmin_config = NULL;
expublic char G_xadmin_config_file[PATH_MAX+1] = {EXEOS};

/* if we read from stdin: */
expublic char *G_cmd_argv[MAX_ARGS]; /* Assume max 50 arguments */
exprivate char M_buffer[CMD_MAX];
exprivate char M_buffer_prev[CMD_MAX]={EXEOS};  /* Previous command (for interactive terminal) */
exprivate int M_quit_requested = EXFALSE;       /* Mark the system state that we want exit! */

/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

exprivate int cmd_quit(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);
exprivate int cmd_echo(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);
exprivate int cmd_ver(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);
exprivate int cmd_idle(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);
exprivate int cmd_help(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);
exprivate int cmd_stat(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);
exprivate int cmd_poller(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);

/**
 * Initialize command mapping table
 */
cmd_mapping_t M_command_map[] = 
{
    {"quit",    cmd_quit,  EXFAIL,              1,  1,  0, "Quit from command line utility", NULL},
    {"q",       cmd_quit,  EXFAIL,              1,  1,  0, "Alias for `quit'", NULL},
    {"exit",    cmd_quit,  EXFAIL,              1,  1,  0, "Alias for `quit'", NULL},
    {"echo",    cmd_echo,  EXFAIL,              1,  999,0, "Echo text back to terminal", NULL},
    {"idle",    cmd_idle,  EXFAIL,              1,  1,  2, "Enter daemon process in idle state (if not started)", NULL},
    {"help",    cmd_help,  EXFAIL,              1,  2,  0, "Print help (this output)\n"
                                                "\t args: help [command]", NULL},
    {"h",       cmd_help,  EXFAIL,              1,  2,  0, "Alias for `help'", NULL},
    {"info",    cmd_help,  EXFAIL,              1,  2,  0, "Alias for `help'", NULL},
    {"stat",    cmd_stat,  EXFAIL,              1,  1,  2, "Prints general status information", NULL},
    {"ldcf",    cmd_ldcf,  NDRXD_COM_LDCF_RQ,   1,  1,  1, "Load configuration", NULL},
    {"start",   cmd_start, NDRXD_COM_START_RQ,  1,  5,  1, "Start application domain\n"
                                                         "\t Also loads configuration automatically.\n"
                                                         "\t args: start [-y] [-s <server>] [-i <srvid>]", NULL},
    {"psc",     cmd_psc,   NDRXD_COM_PSC_RQ,    1,  2,  1, "Print services\n"
                                                            "\t args: psc [-s]\n"
                                                            "\t -s : print full service name"
                                                         , NULL},
    {"stop",    cmd_stop,  NDRXD_COM_STOP_RQ,   1,  5,  2, "Stop application domain\n"
                                                         "\t args: stop [-y] [-c]|[-s <server>] [-i <srvid>] [-k] [-f]", NULL},
    {"down",    cmd_fdown, EXFAIL,              1,  2,  0, "Force appserver shuttdown & resurce cleanup\n"
                                                         "\t args: fdown [-y]\n"
                                                         "\t RUN ONLY IF YOU KNOW WHAT YOU ARE DOING!", NULL},
    {"cat",     cmd_cat,    NDRXD_COM_AT_RQ,    1,  1,  1, "Attached to ndrxd user session in progress", NULL},
    {"reload",  cmd_reload,NDRXD_COM_RELOAD_RQ, 1,  1,  1, "Load new configuration", NULL},
    {"testcfg", cmd_testcfg,NDRXD_COM_TESTCFG_RQ,1,  1,  1, "Test new configuration", NULL},
    {"unadv",   cmd_unadv,NDRXD_COM_XADUNADV_RQ, 5,  5,  1,"Un-advertise service.\n"
                                                         "\t args: unadv -i server_id -s service_name", NULL},
    {"readv",   cmd_unadv,NDRXD_COM_XADREADV_RQ, 5,  5,  1,"Re-advertise service.\n"
                                                         "\t args: readv -i server_id -s service_name\n"
                                                         "\t might be usable if service Q was unlinked", NULL},
    {"restart", cmd_r,    EXFAIL,                1,  4,  2, "Restart app or service (invokes start & stop with same args!)", NULL},
    {"r",       cmd_r,    EXFAIL,                1,  5,  2, "Alias for `restart'", NULL},
    {"-v",      cmd_ver,  EXFAIL,                1,  1,  0, "Print version info", NULL},
    {"ver",     cmd_ver,  EXFAIL,                1,  1,  0, "Print version info, same as -v", NULL},
    {"ppm",     cmd_ppm,  NDRXD_COM_XAPPM_RQ,    1,  2,  2, "Print process model"
                                    "\tUsage ppm [OPTION]...\n"
                                    "\t\t -2\tPrint Page 2\n"
                                    ,NULL},
    {"psvc",cmd_shm_psvc,NDRXD_COM_XASHM_PSVC_RQ,1,  1,  1, "Shared mem, print services", NULL},
    {"psrv",cmd_shm_psrv,NDRXD_COM_XASHM_PSRV_RQ,1,  1,  1, "Shared mem, print servers", NULL},
    {"cabort",cmd_cabort,NDRXD_COM_XACABORT_RQ,  1,  2,  1, "Abort app shutdown or startup.\n"
                                                           "\t args: abort [-y]", NULL},
    {"sreload", cmd_sreload, NDRXD_COM_SRELOAD_RQ,   1,  3,  1, "Restarts server instance by instance\n"
                                                         "\t Args: sreload [-y] [-s <server>] [-i <srvid>]", NULL},
    {"sr", cmd_sreload, NDRXD_COM_SRELOAD_RQ,    1,  3,  1, "Alias for `sreload'", NULL},
    {"pq",cmd_pq,NDRXD_COM_XAPQ_RQ,   1,  1,  1, "Print service queues", NULL},
    {"pqa",cmd_pqa,  EXFAIL,            1,  2,  2, "Print all queues\n"
                                                "\t args: pqa [-a]\n"
                                                "\t -a - print all queues "
                                                "(incl. other local systems)", NULL},
    /* New XA commands: printtrans (pt), abort, commit, exsting abort move to: sabort (start/stop) 
     * abort + install ctrl+c handler 
     */
    {"pt",        cmd_pt,EXFAIL,   1,  1,  1, "Print transactions", NULL},
    {"printtrans",cmd_pt,EXFAIL,   1,  1,  1, "Alias for `pt'", NULL},
    {"abort",     cmd_abort,EXFAIL,   3,  5,  1, "Abort transaction\n"
                                            "\t args: abort -t <RM Ref> -x <XID> [-g <Group No>] [-y]", NULL},
    {"aborttrans",cmd_abort,EXFAIL,   3,  5,  1, "Alias for `abort'", NULL},
    {"commit",     cmd_commit,EXFAIL,   3,  4,  1, "Commit transaction\n"
                                            "\t args: commit -t <RM Ref> -x <XID> [-y]", NULL},
    {"committrans",cmd_commit,EXFAIL,   3,  4,  1, "Alias for `commit'", NULL},
    {"pe",        cmd_pe,NDRXD_COM_PE_RQ,   1,  1,  1, "Print env (from ndrxd)", NULL},
    {"printenv",  cmd_pe,NDRXD_COM_PE_RQ,   2,  2,  1, "Alias for `pe'", NULL},
    {"set",       cmd_set,NDRXD_COM_SET_RQ,   1,  99,  1, "Set env variable. \n"
                                                "\t Format: VAR=SOME VALUE (will be contact with spaces)", NULL},
    {"unset",     cmd_unset,NDRXD_COM_UNSET_RQ,   1,  1,  1, "Set env variable. Forma: VAR"},
    {"pc",        cmd_pc,EXFAIL,   1,  1,  1, "Print clients", NULL},
    {"sc",        cmd_sc,EXFAIL,   1,  4,  1, "Stop client\n"
                                    "\t args: sc -t <Tag> [-s <Subsection (default -)] [-w <wait in millis>]", NULL},
    {"bc",        cmd_bc,EXFAIL,   1,  4,  1, "Boot(start) client\n"
                                    "\t args: bc -t <Tag> [-s <Subsection (default -)] [-w <wait in millis>]", NULL},
    {"rc",        cmd_rc,EXFAIL,   1,  4,  1, "Reload/Restart clients one-by-one\n"
                                    "\t args: bc -t <Tag> [-s <Subsection (default -)] [-w <wait in millis>]", NULL},
    {"mqlc",      cmd_mqlc,EXFAIL,   1,  1,  1, "List persistent queue configuration", NULL},
    {"mqlq",      cmd_mqlq,EXFAIL,   1,  1,  1, "List persistent queues (active/dynamic)", NULL},
    {"mqrc",      cmd_mqrc,EXFAIL,   1,  1,  1, "Reload TMQ config", NULL},
    {"mqlm",      cmd_mqlm,EXFAIL,   3,  3,  1, "List messages in q\n"
                                    "\t args: mqlm -s <QSpace> -q <QName>", NULL},
    {"mqdm",      cmd_mqdm,EXFAIL,   4,  4,  1, "Dump/peek message to stdout\n"
                                    "\t args: mqdm -n <Cluster node id> -i <Server ID> -m <Message ID>", NULL},
    {"mqch",      cmd_mqch,EXFAIL,   4,  4,  1, "Change queue config (runtime only)\n"
                                    "\t args: mqch -n <Cluster node id> -i <Server ID> -q <Q def (conf format)>", NULL},
    {"mqrm",      cmd_mqrm,EXFAIL,   4,  4,  1, "Remove message from Q space\n"
                                    "\t args: mqrm -n <Cluster node id> -i <Server ID> -m <Message ID>", NULL},
    {"mqmv",      cmd_mqmv,EXFAIL,   6,  6,  1, "Move message to different qspace/qname\n"
                                    "\t args: mqmv -n <Source cluster node id> -i <Source server ID>\n"
                                    "\t -m <Source Message ID> -s <Dest qspace> -q <Dest qname>", NULL},
    {"killall",   cmd_killall,EXFAIL, 1,  999,  0, "Kill all processes (in ps -ef) matching the name\n"
                                    "\t args: killall <name1> <name2> ... <nameN>", NULL},
    {"qrm",       cmd_qrm,EXFAIL, 1,  999,  0, "Remove specific queue\n"
                                    "\t args: qrm <qname1> <qname2> ... <qnameN>", NULL},
    {"qrmall",    cmd_qrmall,EXFAIL, 1,  999,  0, "Remove queue matching the substring \n"
                                    "\t args: qrmall <substr1> <substr2> ... <substrN>", NULL},
    {"provision", cmd_provision,EXFAIL, 0,  999,  0, "Prepare initial Enduro/X instance environment \n"
                                     "\t args: provision [-d] [-v<param1>=<value1>] ... [-v<paramN>=<valueN>]", NULL},
    {"gen",       cmd_gen,EXFAIL, 0,  999,  0, "Generate sources\n"
                                     "\t args: gen <target> <type> [-d] "
                                     "[-v<param1>=<value1>] ... [-v<paramN>=<valueN>]\n"
                                     "\t tagets/types available:", cmd_gen_help},
    {"cs",        cmd_cs,EXFAIL,   1,  3,  1, "Cache show\n"
                                            "\t args: cs <cache_db_name>|-d <cache_db_name>", NULL},
    {"cacheshow", cmd_cs,EXFAIL,   1,  3,  1, "Alias for `cs' ", NULL},
    {"cd",        cmd_cd,EXFAIL,   3,  4,  1, "Dump message in cache\n"
                                    "\t args: cd -d <dbname> -k <key> [-i interpret_result]", NULL},
    {"cachedump", cmd_cd,EXFAIL,   3,  4,  1, "Alias for `cd' ", NULL},
    {"ci",        cmd_ci,EXFAIL,   2,  4,  1, "Invalidate cache\n"
                                    "\t args: ci -d <dbname> [-k <key>][-r use_regexp]", NULL},
    {"cacheinval",cmd_ci,EXFAIL,   2,  4,  1, "Alias for `ci' ", NULL},
#ifdef EX_USE_SYSVQ
    {"svmaps",    cmd_svmaps,EXFAIL,   1,  6,  0, "Print System V Queue mapping tables\n"
                                    "\tUsage svmaps [OPTION]...\n"
                                    "\t\t -p\tPrint Posix to System V table (default)\n"
                                    "\t\t -s\tPrint System V to Posix table\n"
                                    "\t\t -a\tPrint all entries (lots of records...)\n"
                                    "\t\t -i\tPrint in use entries (default)\n"
                                    "\t\t -w\tPrint entries which were used but now free",
                                    NULL},
#endif
    {"pubfdb",    cmd_pubfdb,EXFAIL,   1,  1,  0, "Print UBF custom fields (from DB)", NULL},
    {"poller",    cmd_poller,EXFAIL,   1,  1,  0, "Print active poller sub-system", NULL}
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
    {NDRXD_COM_LDCF_RQ,     NULL,           simple_output,  EXTRUE},/*0*/
    {NDRXD_COM_LDCF_RP,     NULL,           NULL,           EXFALSE},/*1*/
    {NDRXD_COM_START_RQ,    ss_rsp_process, simple_output,  EXTRUE},/*2*/
    {NDRXD_COM_START_RP,    NULL,           NULL,           EXFALSE},/*3*/
    {NDRXD_COM_SVCINFO_RQ,  NULL,           NULL,           EXFALSE},/*4*/
    {NDRXD_COM_SVCINFO_RP,  NULL,           NULL,           EXFALSE},/*5*/
    {NDRXD_COM_PMNTIFY_RQ,  NULL,           NULL,           EXFALSE},/*6*/
    {NDRXD_COM_PMNTIFY_RP,  NULL,           NULL,           EXFALSE},/*7*/
    {NDRXD_COM_PSC_RQ,      psc_rsp_process,simple_output,  EXTRUE},/*8*/
    {NDRXD_COM_PSC_RP,      NULL,           NULL,           EXFALSE},/*9*/
    {NDRXD_COM_STOP_RQ,     ss_rsp_process, simple_output,  EXTRUE},/*10*/
    {NDRXD_COM_STOP_RP,     NULL,           NULL,           EXFALSE},/*11*/
    {NDRXD_COM_SRVSTOP_RQ,  NULL,           NULL,           EXFALSE},/*12*/
    {NDRXD_COM_SRVSTOP_RP,  NULL,           NULL,           EXFALSE},/*13*/
    {NDRXD_COM_AT_RQ,       at_rsp_process, simple_output,  EXTRUE},/*14*/
    {NDRXD_COM_AT_RP,       NULL,           NULL,           EXFALSE},/*15*/
    {NDRXD_COM_RELOAD_RQ,reload_rsp_process,simple_output,  EXTRUE},/*16*/
    {NDRXD_COM_RELOAD_RP,   NULL,           NULL,           EXFALSE},/*17*/
    {NDRXD_COM_TESTCFG_RQ,reload_rsp_process,simple_output,  EXTRUE},/*18*/
    {NDRXD_COM_TESTCFG_RP,   NULL,          NULL,           EXFALSE},/*19*/
    {NDRXD_COM_SRVINFO_RQ,   NULL,          NULL,           EXFALSE},/*20*/
    {NDRXD_COM_SRVINFO_RP,   NULL,          NULL,           EXFALSE},/*21*/
    {NDRXD_COM_SRVUNADV_RQ,   NULL,         NULL,           EXFALSE},/*22*/
    {NDRXD_COM_SRVUNADV_RP,   NULL,         NULL,           EXFALSE},/*23*/
    {NDRXD_COM_XADUNADV_RQ,   NULL,         simple_output,  EXTRUE},/*24*/
    {NDRXD_COM_XADUNADV_RP,   NULL,         NULL,           EXFALSE},/*25*/
    {NDRXD_COM_NXDUNADV_RQ,   NULL,         NULL,           EXFALSE},/*26*/
    {NDRXD_COM_NXDUNADV_RP,   NULL,         NULL,           EXFALSE},/*27*/
    {NDRXD_COM_SRVADV_RQ,     NULL,         NULL,           EXFALSE},/*28*/
    {NDRXD_COM_SRVADV_RP,     NULL,         NULL,           EXFALSE},/*29*/
    {NDRXD_COM_XAPPM_RQ,      ppm_rsp_process,simple_output,EXTRUE}, /*30*/
    {NDRXD_COM_XAPPM_RP,      NULL,         NULL,         EXFALSE},/*31*/
    {NDRXD_COM_XASHM_PSVC_RQ, shm_psvc_rsp_process,simple_output,EXTRUE},/*31*/
    {NDRXD_COM_XASHM_PSVC_RP, NULL,         NULL,         EXFALSE},/*32*/
    {NDRXD_COM_XASHM_PSRV_RQ, shm_psrv_rsp_process,simple_output,EXTRUE},/*33*/
    {NDRXD_COM_XASHM_PSRV_RP, NULL,         NULL,         EXFALSE},/*34*/
    {NDRXD_COM_NXDREADV_RQ,   NULL,         NULL,         EXFALSE},/*36*/
    {NDRXD_COM_NXDREADV_RP,   NULL,         NULL,         EXFALSE},/*37*/
    {NDRXD_COM_XADREADV_RQ,   NULL,         simple_output,EXTRUE },/*38*/
    {NDRXD_COM_XADREADV_RP,   NULL,         NULL,         EXFALSE}, /*39*/
    {NDRXD_COM_XACABORT_RQ,    NULL,         simple_output,EXTRUE },/*40*/
    {NDRXD_COM_XAABORT_RP,    NULL,         NULL,         EXFALSE}, /*41*/
    {NDRXD_COM_BRCON_RQ,      NULL,         NULL,         EXFALSE},/*42*/
    {NDRXD_COM_BRCON_RP,      NULL,         NULL,         EXFALSE},/*43*/
    {NDRXD_COM_BRDISCON_RQ,   NULL,         NULL,         EXFALSE},/*44*/
    {NDRXD_COM_BRDISCON_RP,   NULL,         NULL,         EXFALSE},/*45*/
    {NDRXD_COM_BRREFERSH_RQ,  NULL,         NULL,         EXFALSE},/*46*/
    {NDRXD_COM_BRREFERSH_RP,  NULL,         NULL,         EXFALSE},/*47*/
    {NDRXD_COM_BRCLOCK_RQ,    NULL,         NULL,         EXFALSE},/*48*/
    {NDRXD_COM_BRCLOCK_RP,    NULL,         NULL,         EXFALSE},/*49*/
    {NDRXD_COM_SRVGETBRS_RQ,  NULL,         NULL,         EXFALSE},/*50*/
    {NDRXD_COM_SRVGETBRS_RP,  NULL,         NULL,         EXFALSE},/*51*/
    {NDRXD_COM_SRVPING_RQ,    NULL,         NULL,         EXFALSE},/*52*/
    {NDRXD_COM_SRVPING_RP,    NULL,         NULL,         EXFALSE}, /*53*/
    {NDRXD_COM_SRELOAD_RQ,    ss_rsp_process, simple_output,EXTRUE},/*54*/
    {NDRXD_COM_SRELOAD_RP,    NULL,           NULL,        EXFALSE},/*55*/
    {NDRXD_COM_XAPQ_RQ,       pq_rsp_process,simple_output,EXTRUE},/*56*/
    {NDRXD_COM_XAPQ_RP,       NULL,         NULL,         EXFALSE},/*57*/
    {NDRXD_COM_PE_RQ,         pe_rsp_process,simple_output,EXTRUE},/*58*/
    {NDRXD_COM_PE_RP,         NULL,         NULL,         EXFALSE},/*59*/
    {NDRXD_COM_SET_RQ,NULL,simple_output,      EXTRUE},/*60*/
    {NDRXD_COM_SET_RP,        NULL,           NULL,       EXFALSE},/*61*/
    {NDRXD_COM_UNSET_RQ,NULL,simple_output,    EXTRUE},/*62*/
    {NDRXD_COM_UNSET_RP,       NULL,           NULL,      EXFALSE},/*63*/
};

/**
 * Display EnduroX Version string
 */
exprivate int cmd_ver(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    printf(NDRX_VERSION
		"\n");
    return EXSUCCEED;
}

/**
 * Show help/command summary
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
exprivate int cmd_help(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
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
    return EXSUCCEED;
}

/**
 * Start idle instance (if backend does not exists!)
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
exprivate int cmd_idle(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret=EXSUCCEED;

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
exprivate int cmd_stat(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret=EXSUCCEED;
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
exprivate int cmd_quit(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    NDRX_LOG(log_debug, "cmd_quit called");
    M_quit_requested = EXTRUE;
    *p_have_next = EXFALSE;
    return EXSUCCEED;
}

/**
 * echo command
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
exprivate int cmd_echo(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int i;
    NDRX_LOG(log_debug, "cmd_echo called");

    for (i=1; i< argc; i++)
    {
        printf("%s ", argv[i]);
    }
    
    printf("\n");

    return EXSUCCEED;
}

/**
 * Print current poller sub-system
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
exprivate int cmd_poller(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    printf("%s\n", ndrx_epoll_mode());
    return EXSUCCEED;
}

/**
 * Simple output
 * @param buf
 */
expublic void simple_output(char *buf)
{
    fprintf(stderr, "%s", buf);
    fprintf(stderr, "\n");
}

/**
 * Checks is terminal tty.
 * @return 
 */
exprivate int is_tty()
{
    return isatty(0);
}

/**
 * Reads the command either from command line or from stdin.
 * If from stdin, then command gets splitted.
 * @return SUCCEED/FAIL
 */
exprivate int get_cmd(int *p_have_next)
{
    int ret=EXSUCCEED;
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
            NDRX_STRNCPY_SAFE(G_cmd_argv[i-1], M_argv[i], MAX_ARG_LEN);
            G_cmd_argc_raw++;
        }
        
        *p_have_next = EXFALSE;
        
    }
    else /* Operate with stdin. */
    {
        char *p;
        memset(M_buffer, 0, sizeof(M_buffer));

        /* Welcome only if it is terminal */
        if (is_tty())
            printf("NDRX %s> ", ndrx_xadmin_nodeid());

        /* We should get something! */
        while (NULL==fgets(M_buffer, sizeof(M_buffer), stdin))
        {
            /* if we do not have tty, then exit */
            if (!is_tty())
            {
                /* do not have next */
                *p_have_next = EXFALSE;
                goto out;
            }
        }

        /* strip off trailing newline */
        if ('\n'==M_buffer[strlen(M_buffer)-1])
            M_buffer[strlen(M_buffer)-1] = EXEOS;
        
        /* Allow repeated commands */
        if (is_tty())
        {
            if (EXEOS!=M_buffer_prev[0] && '/' == M_buffer[0])
            {
                NDRX_STRCPY_SAFE(M_buffer, M_buffer_prev);
            }
            else if (EXEOS!=M_buffer[0])
            {
                NDRX_STRCPY_SAFE(M_buffer_prev, M_buffer);
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
            
            NDRX_STRNCPY_SAFE(G_cmd_argv[G_cmd_argc_raw], p, MAX_ARG_LEN);
            G_cmd_argc_raw++;
            
            p = strtok (NULL, ARG_DEILIM);
        }
        /* We have next from stdin, if not quit command executed last */
        if (!M_quit_requested)
            *p_have_next = EXTRUE;        
    }
out:
    return ret;
}


/**
 * Process requested command buffer.
 * The command by itself should be found in first argument
 * @return SUCCEED/FAIL
 */
exprivate int process_command_buffer(int *p_have_next)
{
    int ret=EXSUCCEED;
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
        EXFAIL_OUT(ret);
    }
    else
    {
        /* check min/max args */
        if ( G_cmd_argc_logical < map->min_args)
        {
            fprintf(stderr, "Syntax error, too few args (min %d, max %d, got %d)!\n",
                                map->min_args, map->max_args, G_cmd_argc_logical);
            EXFAIL_OUT(ret);
        }
        else if ( G_cmd_argc_logical > map->max_args)
        {
            fprintf(stderr, "Syntax error, too many args (min %d, max %d, got %d)!\n",
                                map->min_args, map->max_args, G_cmd_argc_logical);
            EXFAIL_OUT(ret);
        }
        else if ( (NDRX_XADMIN_RPLYQREQ==map->reqidle || NDRX_XADMIN_IDLEREQ==map->reqidle)
                && EXFAIL==ndrx_xadmin_open_rply_q())
        {
            EXFAIL_OUT(ret);
        }
        else if (NDRX_XADMIN_IDLEREQ==map->reqidle
                && !is_ndrxd_running() && EXFAIL==ndrx_start_idle())
        {
            EXFAIL_OUT(ret);
        }
        else
        {
            /* execute function 
            fprintf(stderr, "exec: %s\n", M_cmd_argv[0]); */
            ret = M_command_map[i].p_exec_command(&M_command_map[i],
                                    G_cmd_argc_raw, (char **)G_cmd_argv, p_have_next);
        }
    }
out:
    
    return ret;
}

/**
 * Un-initialize the process
 * @return 
 */
expublic int un_init(void)
{
    NDRX_LOG(log_debug, "into un-init");
    if (G_config.ndrxd_q != (mqd_t)EXFAIL)
    {
        NDRX_LOG(log_debug, "Closing ndrxd_q: %p",
            (void *)G_config.ndrxd_q);
        ndrx_mq_close(G_config.ndrxd_q);
        G_config.ndrxd_q = (mqd_t)EXFAIL;
    }

    if (G_config.reply_queue != (mqd_t)EXFAIL)
    {
        NDRX_LOG(log_debug, "Closing reply_queue: %p",
            (void *)G_config.reply_queue);

        ndrx_mq_close(G_config.reply_queue);

        NDRX_LOG(log_debug, "Unlinking [%s]",
            G_config.reply_queue_str);
        ndrx_mq_unlink(G_config.reply_queue_str);
        G_config.reply_queue = (mqd_t)EXFAIL;
    }
    
    /* In any case if session was open... */
    tpterm();
    
    /* close any additional shared resources */
    ndrx_xadmin_shm_close();
    
    return EXSUCCEED;
}

/**
 * Start idle instance.
 * Caller must ensure that backend is not running.
 * @return
 */
expublic int ndrx_start_idle(void)
{
    int ret=EXSUCCEED;

    /* Start idle instance, if background process is not running. */
    if (NDRXD_STAT_NOT_STARTED==G_config.ndrxd_stat)
    {
        ret = start_daemon_idle();
    }
    else if (NDRXD_STAT_MALFUNCTION==G_config.ndrxd_stat)
    {
        fprintf(stderr, "Enduro/X back-end (ndrxd) malfunction - see logs!\n");
    }
out:
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
exprivate int get_file(char *buf, size_t size, char *path1, char *path2)
{
    if (NULL!=path1 && NULL!=path2)
    {
        snprintf(buf, size, "%s/%s", path1, path2);
    }
    else if (NULL!=path1)
    {
        /* snprintf(buf, size, "%s", path1); */
        NDRX_STRNCPY(buf, path1, size);
        buf[size-1] = EXEOS;
    }
    else
    {
        return EXFALSE;
    }
    
    return ndrx_file_exists(buf);
}

/**
 * Initialize Client (and possible start idle back-end)
 * @return
 */
expublic int ndrx_init(int need_init)
{
    int ret=EXSUCCEED;
    int i;
    ndrx_inicfg_t *cfg = NULL;
    
#ifdef EX_USE_EMQ
    /* We need to get lock in */
    emq_set_lock_timeout(10);
#endif

    if (need_init)
    {
        if (EXSUCCEED!=(ret = load_env_config()))
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
            ret=EXFAIL;
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
            NDRX_STRCPY_SAFE(cfg_section, "@xadmin");
        }
        else
        {
            snprintf(cfg_section, sizeof(cfg_section), "@xadmin/%s", cctag);
        }
        
        /* load the configuration file */
        if (NULL==(cfg=ndrx_inicfg_new()))
        {
            NDRX_LOG(log_error, "Failed to create inicfg: %s", Nstrerror(Nerror));
            EXFAIL_OUT(ret);
        }
        
        /* Add config file 
         * Looks like global section does not needs to be parsed here.
         */
        if (EXSUCCEED!=ndrx_inicfg_add(cfg, G_xadmin_config_file, NULL))
        {
             NDRX_LOG(log_error, "Failed to add resource [%s]: %s", 
                    G_xadmin_config_file, Nstrerror(Nerror));
            EXFAIL_OUT(ret);       
        }
        
        /* Get the section... */
        if (EXSUCCEED!=ndrx_inicfg_get_subsect(cfg, NULL, cfg_section, &G_xadmin_config))
        {
            NDRX_LOG(log_error, "Failed to resolve config: %s", Nstrerror(Nerror));
            EXFAIL_OUT(ret);    
        }
    }
    
    if (EXSUCCEED!=cmd_gen_load_scripts())
    {
        NDRX_LOG(log_error, "Failed to load gen scripts");
        EXFAIL_OUT(ret);    
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
    int ret=EXSUCCEED;
    int need_init = EXTRUE;
    /*test the stack*/
    char buf[NDRX_MSGSIZEMAX];
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
                need_init=EXFALSE;
                break;
            }
            
            i++;
        }
    }
    
    if (EXFAIL==ndrx_init(need_init))
    {
        NDRX_LOG(log_error, "Failed to initialize!");
        fprintf(stderr, "Failed to initialize!\n");
        EXFAIL_OUT(ret);
    }

    signal(SIGCHLD, sign_chld_handler);
    
    /* Print the copyright notice: */
    if (is_tty() && NULL==getenv(CONF_NDRX_SILENT))
    {
        NDRX_BANNER;
    }

    /* Main command loop */
    while(EXSUCCEED==ret && have_next)
    {
        /* Get the command */
        if (EXFAIL==(ret = get_cmd(&have_next)))
        {
            NDRX_LOG(log_error, "Failed to get command!");
            ret=EXFAIL;
            goto out;
        }
        /* Now process command buffer */
        if (G_cmd_argc_logical > 0 &&
            EXSUCCEED!=process_command_buffer(&have_next) &&
                (!have_next || !isatty(0))) /* stop processing if not tty or do not have next */
        {
            ret=EXFAIL;
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

/* vim: set ts=4 sw=4 et smartindent: */
