/**
 * @brief Common routines of the admin server
 *
 * @file commons.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2019, Mavimax, Ltd. All Rights Reserved.
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <regex.h>
#include <utlist.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

#include <ndebug.h>
#include <atmi.h>
#include <atmi_int.h>
#include <typed_buf.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <Exfields.h>
#include <Excompat.h>
#include <ubfutil.h>
#include <sys_unix.h>
#include <gencall.h>
#include <tpadm.h>
#include "tpadmsv.h"
#include "expr.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/**
 * Mapping to from classes to their operations...
 */
expublic ndrx_adm_class_map_t ndrx_G_class_map[] =
{  
    /* Driving of the Preparing: */
    {NDRX_TA_CLASS_CLIENT,      "CL",       &ndrx_adm_client_get, ndrx_G_client_map}
    ,{NDRX_TA_CLASS_DOMAIN,     "DM",       &ndrx_adm_domain_get, ndrx_G_domain_map}
    ,{NDRX_TA_CLASS_MACHINE,    "MA",       &ndrx_adm_machine_get, ndrx_G_machine_map}
    ,{NDRX_TA_CLASS_QUEUE,      "QU",       &ndrx_adm_queue_get, ndrx_G_queue_map}
    ,{NDRX_TA_CLASS_SERVER,     "SR",       &ndrx_adm_server_get, ndrx_G_server_map}
    ,{NDRX_TA_CLASS_SERVICE,    "SC",       &ndrx_adm_service_get, ndrx_G_service_map}
    ,{NDRX_TA_CLASS_SVCGRP,     "SG",       &ndrx_adm_svcgrp_get, ndrx_G_svcgrp_map}
    ,{NDRX_TA_CLASS_BRCON,      "BC",       &ndrx_adm_brcon_get, ndrx_G_brcon_map}
    ,{NULL}
};

/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
/**
 * Generic ppm call
 * @param p_rsp_process callback of ppm
 * @param req_cmd NDRXD_COM_*_RQ
 * @param rsp_cmd NDRXD_COM_*_RP
 * @param dst_qstr dest queue ndrxd, bridge, etc...
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_adm_list_call(int (*p_rsp_process)(command_reply_t *reply, size_t reply_len), 
        int req_cmd, int resp_cmd, char *dst_qstr)
{
    int ret = EXSUCCEED;
    command_call_t call;
    gencall_args_t call_args[NDRXD_COM_MAX];
    
    struct mq_attr new_attr, org_attr;
    
    memset(&call, 0, sizeof(call));
    
    call_args[req_cmd].ndrxd_cmd = req_cmd;
    call_args[req_cmd].p_rsp_process = p_rsp_process;
    call_args[req_cmd].p_put_output = NULL;
    call_args[req_cmd].need_reply = EXTRUE;
    
    call_args[resp_cmd].ndrxd_cmd = resp_cmd;
    call_args[resp_cmd].p_rsp_process = NULL;
    call_args[resp_cmd].p_put_output = NULL;
    call_args[resp_cmd].need_reply = EXFALSE;

    /* set queue to blocked */
    memset(&org_attr, 0, sizeof(org_attr));
    
    if (EXSUCCEED!=ndrx_mq_getattr(ndrx_get_G_atmi_conf()->reply_q, 
                &org_attr))
    {
        NDRX_LOG(log_error, "Failed to get attr: %s", strerror(errno));
        EXFAIL_OUT(ret);
    }
    
    memcpy(&new_attr, &org_attr, sizeof(new_attr));
    new_attr.mq_flags &= ~O_NONBLOCK; /* remove non block flag */
    
    if (new_attr.mq_flags!=org_attr.mq_flags)
    {
        NDRX_LOG(log_debug, "change attr to blocked");
        if (EXSUCCEED!=ndrx_mq_setattr(ndrx_get_G_atmi_conf()->reply_q, 
                &new_attr, NULL))
        {
            NDRX_LOG(log_error, "Failed to set new attr: %s", strerror(errno));
            EXFAIL_OUT(ret);
        }
    }

    /* This will scan the service list and return the machines connected */
        /* Then get listing... */
    ret = cmd_generic_listcall(req_cmd, NDRXD_SRC_SERVER,
                        NDRXD_CALL_TYPE_GENERIC,
                        &call, sizeof(call),
                        ndrx_get_G_atmi_conf()->reply_q_str,
                        ndrx_get_G_atmi_conf()->reply_q,
                        (mqd_t)EXFAIL,   /* do not keep open ndrxd q open */
                        dst_qstr /*ndrx_get_G_atmi_conf()->ndrxd_q_str*/,
                        0, NULL,
                        NULL,
                        call_args,
                        EXFALSE,
                        0);
    
    /* set queue back to unblocked. */
    if (new_attr.mq_flags!=org_attr.mq_flags)
    {
        NDRX_LOG(log_debug, "change attr to non blocked");
        if (EXSUCCEED!=ndrx_mq_setattr(ndrx_get_G_atmi_conf()->reply_q, 
                &org_attr, NULL))
        {
            NDRX_LOG(log_error, "Failed to set old attr: %s", strerror(errno));
            EXFAIL_OUT(ret);
        }
    }
    
    if (EXSUCCEED!=ret)
    {
        NDRX_LOG(log_error, "Failed to call `%s' to collect infos", dst_qstr);
        EXFAIL_OUT(ret);
    }
    
out:
    return ret;
}

/**
 * Return class map
 * @param clazz class name
 * @return ptr to class descr or NULL
 */
expublic ndrx_adm_class_map_t *ndrx_adm_class_map_get(char *clazz)
{
    int i;
    
    for (i=0; NULL!=ndrx_G_class_map[i].clazz; i++)
    {
        if (0==strcmp(ndrx_G_class_map[i].clazz, clazz))
        {
            return &ndrx_G_class_map[i];
        }
    }
    
    return NULL;
}

/* vim: set ts=4 sw=4 et smartindent: */
