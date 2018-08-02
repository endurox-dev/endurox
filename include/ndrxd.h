/* 
 * @brief Enduro/X Deamon definitions
 *
 * @file ndrxd.h
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or Mavimax's license for commercial use.
 * -----------------------------------------------------------------------------
 * GPL license:
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * -----------------------------------------------------------------------------
 * A commercial use license is available from Mavimax, Ltd
 * contact@mavimax.com
 * -----------------------------------------------------------------------------
 */
#ifndef NDRXD_H
#define	NDRXD_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <sys/param.h>
#include <atmi.h>
#include <atmi_int.h>
#include <ndrxdcmn.h>
#include <nstopwatch.h>
#include <cconfig.h>
#include <exenv.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define PARSE_SECTION_FAIL         EXFAIL
#define PARSE_SECTION_CONFIG      0
#define PARSE_SECTION_SERVERS     1


#define COMMAND_WAIT_DEFAULT        2 /* seconds */

#define RESPAWN_CNTR_MAX           1000
#define DEF_SRV_STARTWAIT          30 /* Default process startup time        */
#define DEF_SRV_STOPWAIT           30 /* Default process shutdown time       */
    
#define SANITY_CNT_IDLE            -1 /* The PING is not issued, do not count*/
#define SANITY_CNT_START           0  /* Reset value of cycle counter        */
    
#define MAX_SERVICE_LIST           1024    /* MAX list of exportsvcs */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/**
 * Type defintion for *SERVER section entry
 */
typedef struct conf_server_node conf_server_node_t;
struct conf_server_node
{
    char binary_name[MAXTIDENT+1]; /* Name of the binary */
    /** Real binary name */
    char binary_name_real[MAXTIDENT+1];
    char fullpath[PATH_MAX+1]; /* full path to executable, optional */
    /** Command line format (optional) */
    char cmdline[PATH_MAX+1];
    int srvid;
    int min;
    int max;
    int autokill;
    char env[PATH_MAX]; /* Environment override file */
    
    int start_max;  /* Max process startup time, if no server info in time
                     * process will be killed. */
    int pingtime;   /* ping servers every X seconds (divided by sanity counter) */
    int ping_max;   /* max time in which server must respond, otherwise it will be killed */
    int end_max;    /* Max time in which process should exit */
    int killtime;   /* periodical time for signal sending */
    /* list of services to export (for bridge, special). comma seperated list */
    char exportsvcs[MAX_SERVICE_LIST]; 
    /* list of services that should not be exported over the bridge */
    char blacklistsvcs[MAX_SERVICE_LIST]; 
    int sleep_after;
    char SYSOPT[PATH_MAX/2 - 8 ]; /* take off -i xxxxx PID */
    char APPOPT[PATH_MAX/2];
    char clopt[PATH_MAX];
    
    int srvstartwait; /* Time to wait for server startup (after report in progress) */
    int srvstopwait; /* Time to wait for server shutdown (after report in progress)*/
    
    /* common-config tag (loaded into NDRX_CCTAG before start) */
    char cctag[NDRX_CCTAG_MAX+1]; 
    int isprotected; /* is binary protected... */
    
    int reloadonchange; /* Reload binary on change - this max the monitoring of the bin */
    int respawn;	/* Should we ndrxd respawn process automatically when dead */
    
    /* have entries for environment */
    
    /** linked environment groups */
    ndrx_env_grouplist_t *envgrouplist;
    
    /** individual environment variables */
    ndrx_env_list_t *envlist;
    
    /* Linked list */
    conf_server_node_t *prev;
    conf_server_node_t *next;
};

/**
 * Service node
 */
typedef struct pm_node_svc pm_node_svc_t;
struct pm_node_svc
{
    svc_inf_t svc;
    pm_node_svc_t *prev;
    pm_node_svc_t *next;
};
/**
 * Process Model node.
 * Will be present in simple linked list.
 */
typedef struct pm_node pm_node_t;
struct pm_node
{
    conf_server_node_t *conf; /* <<< This can be NULL?  */
    /** Name of the binary (logical name) */
    char binary_name[MAXTIDENT+1];
    /** Real binary name (if cmdline is used for startup config) */
    char binary_name_real[MAXTIDENT+1];
    int srvid;
    char clopt[PATH_MAX - 128]; /* take off -i xxxxx PID and some key       */
    long state;             /* process state code (current)                 */
    long reqstate;          /* Requested state                              */
    short autostart;        /* Start automatically after "start" cmd        */
    int exec_seq_try;       /* Sequental start try                          */
    long rspstwatch;        /* Sanity cycle counter for (start/ping/stop)   */
    long pingstwatch;       /* Ping stopwatch  (measures stanity cycles)    */
    long pingtimer;    /* Timer which counts ping intervals                 */
    ndrx_stopwatch_t pingroundtrip; /* Ping  roundtrip tipe                 */
    int pingseq;            /* Last ping sequence sent                      */
            
    int num_term_sigs;      /* Number of times to send term sig, before -9  */
    long last_sig;          /* Time when signal was sent                    */
    int autokill;           /* Kill the process if not started in time      */
    /**
     * process kill is requested (long startup, 
     * no ping, long shutdown)                      
     */
    int killreq;            
    /** process PID parent/root */
    pid_t pid;
    /** server process pid */
    pid_t svpid;
    long state_changed;     /* Timer for state changed                      */
    pm_node_svc_t   *svcs;  /* list of services                             */
    int flags;              /* Flags sent by server info                    */
    short   nodeid;         /* other node id, if this is bridge             */
    int reloadonchange_cksum; /* Checksum code of the binary                */
    char binary_path[PATH_MAX+1]; /* Path were binary lives...              */
    
    /* Linked list */
    pm_node_t *prev;
    pm_node_t *next;
};

/**
 * PID hash entry (we will have linked list for each entry.
 */
typedef struct pm_pidhash pm_pidhash_t;
struct pm_pidhash
{
    pid_t pid;            /* process PID          */
    pm_node_t *p_pm;
    pm_pidhash_t *prev;
    pm_pidhash_t *next;
};

/**
 * Full configuration handler
 */
typedef struct
{
    /* Active monitor configuration */
    conf_server_node_t *monitor_config;
    int default_min;
    int default_max;
    int default_autokill; /* Kill process which have not started in time */
    char default_env[PATH_MAX]; /* Default env file (might be empty!)*/
    
    /* Reloadable system configuration */
    int sanity; /* Sanity checking */
   
    int restart_min; /* restart min wait sec */
    int restart_step; /* restart stepping, sec */
    int restart_max; /* restart max wait time, sec */
    
    int default_start_max;  /* Max process startup time, if no server info in time
                             * process will be killed. */
    int default_pingtime;   /* ping servers every X seconds (divided by sanity counter) */
    int default_ping_max;   /* max time in which server must respond, otherwise it will be killed */
    int default_end_max;    /* Max time in which process should exit */
    int default_killtime;   /* periodical time for signal sending */
    /* Special config param for bridge services - which services to export */
    char default_exportsvcs[MAX_SERVICE_LIST];
    /* List of services that should not be exported over the bridge */
    char default_blacklistsvcs[MAX_SERVICE_LIST];
    /* common-config tag (loaded into NDRX_CCTAG before start) */
    char default_cctag[NDRX_CCTAG_MAX+1]; 
    
    int default_reloadonchange; /* Reload binary on change */
    
    int qrmdelay;   /* queue remove delay (i.e. remove only after this time + check shm on removal!) */
    
    int restart_to_check;    /* Seconds after re-attach sanity & spawn checks will be done */  
    
    int checkpm;             /* Time for sending info to self about process exit. */
    int brrefresh;           /* Bridge refresh timer */
    
    int default_srvstartwait; /* Time to wait for server startup (after report in progress) */
    int default_srvstopwait; /* Time to wait for server shutdown (after report in progress)*/
    
    int gather_pq_stats;	/* if set to 1, then queue stats will be gathered */
    int default_isprotected;	/* if set to 1, then xadmin stop will not shutdown the process (only with -c) */
    int default_respawn;	/* Set to 1 if auto respawn is required for process */
    
    /** Environment group hash */
    ndrx_env_group_t *envgrouphash;
    
} config_t;

/**
 * Standard command arguments
 */
typedef struct
{
    char    args[CMD_ARG_MAX+1];
    int     cmd;
} cmd_arg_t;

/**
 * Daemon configuration
 */
typedef struct
{
    char    *qprefix;
    char    *config_file; /* Pointer to configuration file path... */
    long    cmd_wait_time;  /* Command wait time in nano-seconds (1/(10^9)) */
    char    *pidfile;       /* PID file to open... */
    long    stat_flags;     /* Program state flags */
    char    *qpath;         /* Path to the queue directory */
    /* NDRXD restart: */
    short restarting;  /* In restart mode, after restart_to_check expired, 
                        * process becomes in normal mode */
    ndrx_stopwatch_t time_from_restart; /* Time counter, how long we are restarting/learning */
    int     fullstart;  /* Are we in full start mode or not? */
} sys_config_t;

/**
 * State of the command processor
 */
typedef struct
{
    mqd_t listenq;                       /* The queue on which  */
    char  listenq_str[NDRX_MAX_Q_SIZE+1];
    int   context;                      /* Current context */
} command_state_t;
/*---------------------------Globals------------------------------------*/

extern config_t     *G_app_config;          /* Running application config   */
extern sys_config_t  G_sys_config;          /* Self daemon configuration    */

/* Active configuration:
 * externs from appconfig.c
 */
extern char *G_config_file;
extern pm_node_t *G_process_model;
extern pm_node_t **G_process_model_hash;
extern pm_pidhash_t **G_process_model_pid_hash;
extern unsigned G_sanity_cycle; /* Sanity cycle */
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
/* cmd processor: */
extern int command_wait_and_run(int *finished, int *abort);
extern int get_cmdq_attr(struct mq_attr *attr);

/* pmodel */
extern int remove_startfail_process(pm_node_t *p_pm, char *svcnm, pm_pidhash_t *pm_pid);
extern pm_node_t * get_pm_from_srvid(int srvid);
extern int remove_service_q(char *svc, short srvid);
extern char * get_srv_admin_q(pm_node_t * p_pm);

/* Configuration */
extern int load_config(config_t *config, char *config_file);
extern int load_active_config(config_t **app_config, pm_node_t **process_model,
            pm_node_t ***process_model_hash, pm_pidhash_t ***process_model_pid_hash);
/* test config or reload */
extern int test_config(int reload, command_call_t * call, void (*p_reload_error)(command_call_t * call,
                    int srvid, char *old_bin, char *new_bin, int error));
extern int build_process_model(conf_server_node_t *p_server_conf,
                                pm_node_t **p_pm_model, /* proces model linked list */
                                pm_node_t **p_pm_hash/* Hash table models */);
extern int add_to_pid_hash(pm_pidhash_t **pid_hash, pm_node_t *p_pm);
extern void ndrxd_sigchld_init(void);
extern void ndrxd_sigchld_uninit(void);
extern pm_pidhash_t * pid_hash_get(pm_pidhash_t **pid_hash, pid_t pid);
extern int delete_from_pid_hash(pm_pidhash_t **pid_hash, pm_pidhash_t *pm_pid);
extern void sign_chld_handler(int sig);
extern int cmd_close_queue(void);

/* Sanity & restart */
extern int do_sanity_check(void);
extern int self_notify(srv_status_t *status, int block);
extern int remove_server_queues(char *process, pid_t pid, int srv_id, char *rplyq);

/* Restart */
extern int do_restart_actions(void);
extern int do_restart_chk(void);

/* Startup */
extern int app_startup(command_startstop_t *call,
        void (*p_startup_progress)(command_startstop_t *call, pm_node_t *pm, int calltype),
        long *p_processes_started); /* have some progress feedback */
extern int is_srvs_down(void);

extern int ndrxd_unlink_pid_file(int second_call);

/* Error handling API */
extern void NDRXD_error (char *str);
extern void NDRXD_set_error(int error_code);
extern void NDRXD_set_error_msg(int error_code, char *msg);
extern void NDRXD_set_error_fmt(int error_code, const char *fmt, ...);
extern void NDRXD_unset_error(void);
extern int NDRXD_is_error(void);
extern void NDRXD_append_error_msg(char *msg);
extern int * _ndrxd_getNDRXD_errno_addr (void);
extern char * ndrxd_strerror (int err);
#define ndrxd_errno	(*_ndrxd_getNDRXD_errno_addr())

/* Advertise & unadvertise */
extern int cmd_xadunadv (command_call_t * call, char *data, size_t len, int context);
extern int cmd_srvunadv (command_call_t * call, char *data, size_t len, int context);
extern int cmd_srvadv (command_call_t * call, char *data, size_t len, int context);
extern int cmd_xadreadv (command_call_t * call, char *data, size_t len, int context);


extern int readv_request(int srvid, char *svc);
extern char * get_srv_admin_q(pm_node_t * p_pm);
extern int pq_run_santiy(int run_hist);
    
/* reload on change: */
extern int roc_is_reload_in_progress(unsigned sanity_cycle);
extern int roc_check_binary(char *binary_path, unsigned sanity_cycle);
extern void roc_mark_as_reloaded(char *binary_path, unsigned sanity_cycle);
extern int self_sreload(pm_node_t *p_pm);

#ifdef	__cplusplus
}
#endif

#endif	/* NDRXD_H */

