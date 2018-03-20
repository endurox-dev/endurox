/* 
** xadmin tooling header file
**
** @file ndrx.h
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

#ifndef NDRX_H
#define	NDRX_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <sys/param.h>
#include <atmi.h>
#include <sys_mqueue.h>
#include <signal.h>

#include <ndrxdcmn.h>
#include <gencall.h>
#include <inicfg.h>
    
#ifndef NDRX_DISABLEPSCRIPT
#include <pscript.h>
#endif

/*---------------------------Externs------------------------------------*/
#define         MAX_ARGS        20
    
#define         CMD_MAX         PATH_MAX
#define         MAX_ARG_LEN     500
#define         ARG_DEILIM      " \t"
#define         MAX_CMD_LEN     300

extern int G_cmd_argc_logical;
extern int G_cmd_argc_raw;
extern char *G_cmd_argv[MAX_ARGS];
/*---------------------------Macros-------------------------------------*/
#define     NDRXD_STAT_NOT_STARTED           0
#define     NDRXD_STAT_MALFUNCTION           1
#define     NDRXD_STAT_RUNNING               2

    
#define FIX_SVC_NM(Xsrc, Xbuf, Xlen) \
        if (strlen(Xsrc) > Xlen)\
        {\
            strncpy(Xbuf, Xsrc, Xlen-1);\
            Xbuf[Xlen-1] = '+';\
            Xbuf[Xlen] = EXEOS;\
        }\
        else\
            strcpy(Xbuf, Xsrc);

#define FIX_SVC_NM_DIRECT(Xbuf, Xlen) \
        if (strlen(Xbuf) > Xlen)\
        {\
            Xbuf[Xlen-1] = '+';\
            Xbuf[Xlen] = EXEOS;\
        }

#define XADMIN_INVALID_OPTIONS_MSG      "Invalid options, see `help'.\n"
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/**
 * EnduroX command line utility config
 */
typedef struct ndrx_config
{
    char pid_file[PATH_MAX];               /* PID file               */
    mqd_t ndrxd_q;                         /* Queue to admin process */
    char ndrxd_q_str[NDRX_MAX_Q_SIZE+1];   /* Queue to admin process */
    char *qprefix;                         /* Global queue prefix    */
    char *qpath;                           /* Global qpath prefix    */

    /* Reply queue (opened on admin utility startup) */
    mqd_t reply_queue;                     /* Queue on which we wait reply */
    char reply_queue_str[NDRX_MAX_Q_SIZE+1]; /* Reply queue (string)   */

    /* Runtime status: */
    int  ndrxd_stat;                       /* Back-end status        */
    short is_idle;                         /* Are we idle?           */
    char *ndrxd_logfile;                   /* Log filename for ndrxd */
} ndrx_config_t;


/* Command mapping */
typedef struct cmd_mapping cmd_mapping_t;       /* Type defintion used in callbacks */
struct cmd_mapping
{
    char *cmd;          /* Command name */
    /* command callback */
    int (*p_exec_command) (cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);
    int ndrxd_cmd;                  /* EnduroX Daemon command code          */
    int min_args;                   /* Minimum argument count for command   */
    int max_args;                   /* Maximum argument count for command   */
    int reqidle;                    /* This requires backend started        */
    char *help;                     /* Short help descr of command          */
    int (*p_add_help) (void);       /* additional help function             */
};
/*---------------------------Globals------------------------------------*/
extern ndrx_config_t G_config;
extern gencall_args_t G_call_args[];
extern ndrx_inicfg_section_keyval_t *G_xadmin_config;
extern char G_xadmin_config_file[PATH_MAX+1];
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
extern int start_daemon_idle(void);
extern int load_env_config(void);
extern int is_ndrxd_running(void);
extern void simple_output(char *buf);
/* extern int get_arg(char *param, int argc, char **argv, char **out); */
extern int chk_confirm(char *message, short is_confirmed);
extern int chk_confirm_clopt(char *message, int argc, char **argv);
extern int ndrx_start_idle();
extern void sign_chld_handler(int sig);

#ifndef NDRX_DISABLEPSCRIPT
extern void printfunc(HPSCRIPTVM v,const PSChar *s,...);
extern void errorfunc(HPSCRIPTVM v,const PSChar *s,...);
extern int load_value(HPSCRIPTVM v, char *key_val_string);
extern int add_defaults_from_config(HPSCRIPTVM v, char *section);
extern int register_getExfields(HPSCRIPTVM v);

#endif

extern int cmd_ldcf(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);
extern int cmd_start(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);
extern int cmd_stop(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);
extern int cmd_sreload(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);
/* start stop process: */
extern int ss_rsp_process(command_reply_t *reply, size_t reply_len);
extern int cmd_psc(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);
extern int psc_rsp_process(command_reply_t *reply, size_t reply_len);
extern int cmd_fdown(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);
extern int cmd_cat(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);
extern int at_rsp_process(command_reply_t *reply, size_t reply_len);
extern int cmd_reload(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);
extern int cmd_testcfg(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);
extern int reload_rsp_process(command_reply_t *reply, size_t reply_len);
extern int cmd_r(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);
extern int cmd_cabort(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);
/*Adv & unadv*/
extern int cmd_unadv(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);
/*readv: */
extern int cmd_readv(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);

/*ppm:*/
extern int cmd_ppm(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);
extern int ppm_rsp_process(command_reply_t *reply, size_t reply_len);


/* shm: */
extern int shm_psrv_rsp_process(command_reply_t *reply, size_t reply_len);
extern int cmd_shm_psrv(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);
    
extern int shm_psvc_rsp_process(command_reply_t *reply, size_t reply_len);    
extern int cmd_shm_psvc(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);

/* queues */
extern int pq_rsp_process(command_reply_t *reply, size_t reply_len);
extern int cmd_pq(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);
extern int cmd_pqa(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);

/* Transactions */
extern int cmd_pt(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);
extern int cmd_abort(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);
extern int cmd_commit(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);


/* env: */
extern int cmd_pe(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);
extern int pe_rsp_process(command_reply_t *reply, size_t reply_len);
extern int cmd_set(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);
extern int cmd_unset(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);

/* cpm: */
extern int cmd_pc(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);
extern int cmd_sc(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);
extern int cmd_bc(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);
extern int cmd_rc(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);


/* persistent queues: */
extern int cmd_mqlq(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);
extern int cmd_mqlc(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);
extern int cmd_mqlm(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);
extern int cmd_mqdm(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);
extern int cmd_mqrc(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);
extern int cmd_mqch(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);
extern int cmd_mqrm(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);
extern int cmd_mqmv(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);

extern int mqfilter(char *svcnm);
extern int cmd_killall(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);

/* posix queues: */
extern int cmd_qrmall(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);
extern int cmd_qrm(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);

/* scripting: */
extern int cmd_provision(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);

/* gen: */
extern int cmd_gen_load_scripts(void);
extern int cmd_gen_help(void);
extern int cmd_gen(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);

/* cache:  */
extern int cmd_cs(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);
extern int cmd_cd(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);
extern int cmd_ci(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next);
#ifdef	__cplusplus
}
#endif

#endif	/* NDRX_H */

