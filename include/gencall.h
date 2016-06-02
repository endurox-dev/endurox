/* 
** Generic Posix queue call
**
** @file gencall.h
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
#ifndef GENCALL_H
#define	GENCALL_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <sys_mqueue.h>

#include <ndrstandard.h>
#include <ndebug.h>

#include <ndrxdcmn.h>
#include <atmi_int.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
typedef struct
{
    int ndrxd_cmd;
    int (*p_rsp_process)(command_reply_t *reply, size_t reply_len);
    void (*p_put_output)(char *text);
    int need_reply;
} gencall_args_t;
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

extern int cmd_generic_call(int ndrxd_cmd, int msg_src, int msg_type,
                            command_call_t *call, size_t call_size,
                            char *reply_q,
                            mqd_t reply_queue, /* listen for answer on this! */
                            mqd_t admin_queue, /* this might be FAIL! */
                            char *admin_q_str, /* should be set! */
                            int argc, char **argv,
                            int *p_have_next,
                            int (*p_rsp_process)(command_reply_t *reply, size_t reply_len),
                            void (*p_put_output)(char *text),
                            int need_reply);

extern int cmd_generic_listcall(int ndrxd_cmd, int msg_src, int msg_type,
                            command_call_t *call, size_t call_size,
                            char *reply_q,
                            mqd_t reply_queue, /* listen for answer on this! */
                            mqd_t admin_queue, /* this might be FAIL! */
                            char *admin_q_str, /* should be set! */
                            int argc, char **argv,
                            int *p_have_next,
                            gencall_args_t *arglist/* this is list of process */,
                            int reply_only);

extern int cmd_generic_callfl(int ndrxd_cmd, int msg_src, int msg_type,
                            command_call_t *call, size_t call_size,
                            char *reply_q,
                            mqd_t reply_queue, /* listen for answer on this! */
                            mqd_t admin_queue, /* this might be FAIL! */
                            char *admin_q_str, /* should be set! */
                            int argc, char **argv,
                            int *p_have_next,
                            int (*p_rsp_process)(command_reply_t *reply, size_t reply_len),
                            void (*p_put_output)(char *text),
                            int need_reply,
                            int flags);

extern int cmd_generic_bufcall(int ndrxd_cmd, int msg_src, int msg_type,
                            command_call_t *call, size_t call_size,
                            char *reply_q,
                            mqd_t reply_queue, /* listen for answer on this! */
                            mqd_t admin_queue, /* this might be FAIL! */
                            char *admin_q_str, /* should be set! */
                            int argc, char **argv,
                            int *p_have_next,
                            int (*p_rsp_process)(command_reply_t *reply, size_t reply_len),
                            void (*p_put_output)(char *text),
                            int need_reply,
                            int reply_only,
                            char *rply_buf_out,             /* might be a null  */
                            int *rply_buf_out_len,          /* if above is set, then must not be null */
                            int flags,
                            int (*p_rply_request)(char *buf, long len));

extern void cmd_generic_init(int ndrxd_cmd, int msg_src, int msg_type,
                            command_call_t *call, char *reply_q);

extern int reply_with_failure(long flags, tp_command_call_t *last_call,
            char *buf, int *len, long rcode);

extern int ndrx_get_q_attr(char *q, struct mq_attr *p_att);

extern atmi_svc_list_t* ndrx_get_svc_list(int (*p_filter)(char *svcnm));

extern void ndrx_reply_with_failure(tp_command_call_t *tp_call, long flags, 
        long rcode, char *reply_to_q);

#ifdef	__cplusplus
}
#endif

#endif	/* GENCALL_H */

