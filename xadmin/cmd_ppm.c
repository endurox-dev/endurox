/**
 * @brief `ppm' command implementation - Print process model
 *
 * @file cmd_ppm.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <sys/param.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <nstdutil.h>

#include <ndrx.h>
#include <ndrxdcmn.h>
#include <atmi_int.h>
#include <gencall.h>

#include "nstopwatch.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define FIX_NM(Xsrc, Xbuf, Xlen) \
            if (strlen(Xsrc) > Xlen)\
            {\
                NDRX_STRNCPY(Xbuf, Xsrc, Xlen-1);\
                Xbuf[Xlen-1] = '+';\
                Xbuf[Xlen] = EXEOS;\
            }\
            else\
                strcpy(Xbuf, Xsrc);
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
struct 
{
    long status;
    char *descr;
} 
M_descr [] =
{
    {NDRXD_PM_NOT_STARTED, "nstrd"},
    {NDRXD_PM_DIED,        "died"},
    {NDRXD_PM_EXIT,        "exit"},
    {NDRXD_PM_ENOENT,      "nobin"},
    {NDRXD_PM_STARTING,    "stop"},
    {NDRXD_PM_RUNNING_OK,  "runok"},
    {NDRXD_PM_STOPPING,    "start"}
};
/*---------------------------Prototypes---------------------------------*/


/**
 * Print header
 * @return
 */
exprivate void print_hdr(void)
{
    /* TODO: we shall print the `real' process name too */
    fprintf(stderr, "BINARY   SVBIN SRVID PID   SVPID STATE REQST AS EXSQ RSP  NTRM LSIG K STCH FLAGS\n");
    fprintf(stderr, "-------- ----- ----- ----- ----- ----- ----- -- ---- ---- ---- ---- - ---- -----\n");
}

/**
 * Get 
 * @param reply
 * @param reply_len
 * @return 
 */
exprivate char *get_status_descr(long status)
{
    char *ret;
    static char buf[16];
    int i;
    
    for (i=0; i<N_DIM(M_descr); i++)
    {
        if (status==M_descr[i].status)
        {
            ret = M_descr[i].descr;
            goto out;
        }
    }
    snprintf(buf, sizeof(buf), "%ld", status);
    ret = buf;
    
out:
    return ret;
}
/**
 * Decode binary flags
 */
exprivate char *decode_flags(int flags)
{
    static char buf[10];
    
    buf[0] = EXEOS;
    
    if (flags & SRV_KEY_FLAGS_BRIDGE)
    {
        strcat(buf, "B");
    }
    
    if (flags & SRV_KEY_FLAGS_BRIDGE)
    {
        strcat(buf, "r");
    }
    
    if (flags & SRV_KEY_FLAGS_CONNECTED)
    {
        strcat(buf, "C");
    }
    
    return buf;
}

/**
 * Process response back.
 * @param reply
 * @param reply_len
 * @return
 */
expublic int ppm_rsp_process(command_reply_t *reply, size_t reply_len)
{
    char binary[9+1];

    if (NDRXD_CALL_TYPE_PM_PPM==reply->msg_type)
    {
        command_reply_ppm_t * ppm_info = (command_reply_ppm_t*)reply;
        FIX_NM(ppm_info->binary_name, binary, (sizeof(binary)-1));
        fprintf(stdout, "%-8.8s %-5.5s %-5d %-5d %-5d %-5.5s %-5.5s %-2hd %-4.4s "
                "%-4.4s %-4.4s %-4.4s %-1d %-4.4s %-5.5s\n", 
                ppm_info->binary_name,
                ppm_info->binary_name_real,
                ppm_info->srvid,
                ppm_info->pid, 
                ppm_info->svpid,
                get_status_descr(ppm_info->state),
                get_status_descr(ppm_info->reqstate),
                ppm_info->autostart,
                ndrx_decode_num(ppm_info->exec_seq_try, 0, 0, 1), 
                ndrx_decode_num(ppm_info->last_startup, 1, 0, 1), 
                ndrx_decode_num(ppm_info->num_term_sigs, 2, 0, 1), 
                ndrx_decode_num(ppm_info->last_sig, 3, 0, 1), 
                ppm_info->autokill, 
                ndrx_decode_num(ppm_info->state_changed, 4, 0, 1), 
                decode_flags(ppm_info->flags)
                );
    }
    
    return EXSUCCEED;
}

/**
 * Get service listings
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
expublic int cmd_ppm(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    command_call_t call;
    memset(&call, 0, sizeof(call));

    /* Print header at first step! */
    print_hdr();
    /* Then get listing... */
    return cmd_generic_listcall(p_cmd_map->ndrxd_cmd, NDRXD_SRC_ADMIN,
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
}

/* vim: set ts=4 sw=4 et cindent: */