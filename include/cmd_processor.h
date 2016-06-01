/* 
** Main header for NDRXD command processor
**
** @file cmd_processor.h
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

#ifndef CMD_PROCESSOR_H
#define	CMD_PROCESSOR_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <ndrxdcmn.h>
#include <ndrxd.h>
/*---------------------------Externs------------------------------------*/
extern command_state_t G_command_state;
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
typedef struct
{
    void *mod_param1; 	/* Generic pointer */
    int  param2;	/* Generic param 2 */
    void *mod_param3;   /* Generic pointer 3 */
    long param4;         /* long param 4 */
} mod_param_t;

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
extern int cmd_processor_init(void);
/* Handler to command processr */
extern command_call_t * G_last_interactive_call;

/* Command listing */
extern int simple_command_reply(command_call_t * call,
                        int status, long flags,
                        mod_param_t *mod_params,
                        void (*p_mod)(command_reply_t *reply, size_t *send_size,
                        mod_param_t *mod_params),
                        long userfld1, int error_code, char *error_msg);

extern int app_shutdown(command_startstop_t *call,
        /* have some progress feedback */
        void (*p_shutdown_progress)(command_call_t *call, pm_node_t *pm, int calltype),
        long *p_processes_shutdown);

extern int app_sreload(command_startstop_t *call,
        void (*p_startup_progress)(command_startstop_t *call, pm_node_t *pm, int calltype),
        void (*p_shutdown_progress)(command_call_t *call, pm_node_t *pm, int calltype),
        long *p_processes_started);
extern int do_respawn_check(void);
extern int srv_send_ping (pm_node_t *p_pm);
extern void ndrx_reply_with_failure(tp_command_call_t *tp_call, long flags, 
        long rcode, char *reply_to_q);

extern int start_process(command_startstop_t *cmd_call, pm_node_t *p_pm,
            void (*p_startup_progress)(command_startstop_t *call, pm_node_t *p_pm, int calltype),
            long *p_processes_started,
            int no_wait,
            int *abort);

/* cmd_config.c: */
extern int cmd_config_load(command_call_t * call, char *data, size_t len, int context);

/* cmd_reload.c: */
extern int cmd_reload(command_call_t * call, char *data, size_t len, int context);
extern int cmd_testcfg (command_call_t * call, char *data, size_t len, int context);

/* cmd_startstop.c: */
extern int cmd_start (command_call_t * call, char *data, size_t len, int context);
extern int cmd_stop (command_call_t * call, char *data, size_t len, int context);
extern int cmd_sreload (command_call_t * call, char *data, size_t len, int context);
extern int cmd_notify (command_call_t * call, char *data, size_t len, int context);
extern int cmd_abort (command_call_t * call, char *data, size_t len, int context);

/* cmd_srvinfo: */
extern int cmd_srvinfo (command_call_t * call, char *data, size_t len, int context);

/* cmd_psc.c: */
extern int cmd_psc (command_call_t * call, char *data, size_t len, int context);

/* cmd_at.c: */
extern int cmd_at (command_call_t * call, char *data, size_t len, int context);

/* cmd_ppm.c: */
extern int cmd_ppm (command_call_t * call, char *data, size_t len, int context);

/* cmd_shm_psrv.c */
extern int cmd_shm_psrv (command_call_t * call, char *data, size_t len, int context);

/* cmd_shm_psvc.c */
extern int cmd_shm_psvc (command_call_t * call, char *data, size_t len, int context);

/* cmd_bridge.c: */
extern int cmd_brcon (command_call_t * call, char *data, size_t len, int context);
extern int cmd_brdiscon (command_call_t * call, char *data, size_t len, int context);
extern int cmd_brrefresh (command_call_t * call, char *data, size_t len, int context);
extern int cmd_getbrs (command_call_t * call, char *data, size_t len, int context);

/* cmd_ping.c: */
extern int cmd_srvpingrsp (command_call_t * call, char *data, size_t len, int context);

/* cmd_pq.c: */
extern int cmd_pq (command_call_t * call, char *data, size_t len, int context);

/* cmd_env.c: */
extern int cmd_pe (command_call_t * call, char *data, size_t len, int context);
extern int cmd_set (command_call_t * call, char *data, size_t len, int context);
extern int cmd_unset (command_call_t * call, char *data, size_t len, int context);
#ifdef	__cplusplus
}
#endif

#endif	/* CMD_PROCESSOR_H */

