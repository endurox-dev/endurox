/* 
** API to NDRXD admin server
** TODO: Might think to convert all ndrxd API operations to adminQ!
** Due to possible collisions with async replies in Q....
**
** @file ndrxdapi.c
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

#include <ndrstandard.h>
#include <ndebug.h>

#include "srv_int.h"
#include "tperror.h"
#include "userlog.h"
#include <atmi_int.h>
#include <ndrxdcmn.h>
#include <unistd.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/* We should have monitor timer for long not-receiving ndrxd reply
 * but receiving tons of event postages...*/
n_timer_t M_getbrs_timer;

/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Report to ndrxd
 * TODO: Test recovery case, it might not report to ndrxd that process is bridge!
 * @return
 */
public int report_to_ndrxd(void)
{
    int ret=SUCCEED;
    char buf[ATMI_MSG_MAX_SIZE];
    srv_status_t *status = (srv_status_t *)buf;
    int i, offset=0;
    svc_entry_fn_t *entry;
    size_t  send_size;

    memset(buf, 0, sizeof(buf));
    
    /* format out the status report */
    status->srvinfo.pid = getpid();
    status->srvinfo.state = NDRXD_PM_RUNNING_OK;
    status->srvinfo.srvid = G_server_conf.srv_id;
    status->srvinfo.flags = G_server_conf.flags;
    status->srvinfo.nodeid = G_server_conf.nodeid;

    /* fill the service list */
    for (i=0; i<G_server_conf.adv_service_count; i++)
    {
        entry = G_server_conf.service_array[i];
        /* TODO: Still some admin stuff gets over? */
        if (entry->is_admin || EOS==entry->svc_nm[0])
        {
            offset++;
            continue; /* not interested in admin q */
        }
        strcpy(status->svcs[i-offset].svc_nm, entry->svc_nm);
        strcpy(status->svcs[i-offset].fn_nm, entry->fn_nm);
        status->svcs[i-offset].qopen_time = entry->qopen_time;
        status->svc_count++;

    }

    send_size = sizeof(srv_status_t)+sizeof(svc_inf_t)*status->svc_count;
    NDRX_LOG(log_debug, "About to send: %d bytes/%d svcs, srvid: %d",
                        send_size, status->svc_count, status->srvinfo.srvid);

    ret=cmd_generic_call(NDRXD_COM_SVCINFO_RQ, NDRXD_SRC_SERVER,
                        NDRXD_CALL_TYPE_PM_INFO,
                        (command_call_t *)status, send_size,
                        G_atmi_conf.reply_q_str,
                        G_atmi_conf.reply_q,
                        (mqd_t)FAIL,   /* do not keep open ndrxd q open */
                        G_atmi_conf.ndrxd_q_str,
                        0, NULL,
                        NULL,
                        NULL,
                        NULL,
                        FALSE);

out:
    return ret;
}


/**
 * Send unsubscribe message to ndrxd
 * @return
 */
public int unadvertse_to_ndrxd(char *svcname)
{
    int ret=SUCCEED;
    char buf[ATMI_MSG_MAX_SIZE];
    command_dynadvertise_t *unadv = (command_dynadvertise_t *)buf;
    int i, offset=0;
    svc_entry_fn_t *entry;
    size_t  send_size=sizeof(command_dynadvertise_t);

    memset(buf, 0, sizeof(buf));
    
    /* format out the status report */
    unadv->srvid= G_server_conf.srv_id;
    strcpy(unadv->svc_nm, svcname);
    
    ret=cmd_generic_call(NDRXD_COM_SRVUNADV_RQ, NDRXD_SRC_SERVER,
                        NDRXD_CALL_TYPE_PM_INFO,
                        (command_call_t *)unadv, send_size,
                        G_atmi_conf.reply_q_str,
                        G_atmi_conf.reply_q,
                        (mqd_t)FAIL,   /* do not keep open ndrxd q open */
                        G_atmi_conf.ndrxd_q_str,
                        0, NULL,
                        NULL,
                        NULL,
                        NULL,
                        FALSE);
    if (SUCCEED!=ret)
    {
        /*Just ignore the error*/
        if (!G_shm_srv)
        {
            NDRX_LOG(log_error, "Not attached to shm/ndrxd - ingore error");
            ret=SUCCEED;
        }    
        else
        {
          _TPset_error_fmt(TPESYSTEM, "Failed to send command %d to [%s]", 
                        NDRXD_COM_SRVUNADV_RQ, G_atmi_conf.ndrxd_q_str);  
        }
    }

out:
    return ret;
}


/**
 * Send advertise block to ndrxd.
 * @return
 */
public int advertse_to_ndrxd(svc_entry_fn_t *entry)
{
    int ret=SUCCEED;
    char buf[ATMI_MSG_MAX_SIZE];
    command_dynadvertise_t *adv = (command_dynadvertise_t *)buf;
    int i, offset=0;
    size_t  send_size=sizeof(command_dynadvertise_t);

    memset(buf, 0, sizeof(buf));
    
    /* format out the status report */
    adv->srvid= G_server_conf.srv_id;
    strcpy(adv->svc_nm, entry->svc_nm);
    strcpy(adv->fn_nm, entry->fn_nm);
    /*Transfer the time there*/
    adv->qopen_time = entry->qopen_time;
    
    ret=cmd_generic_call(NDRXD_COM_SRVADV_RQ, NDRXD_SRC_SERVER,
                        NDRXD_CALL_TYPE_PM_INFO,
                        (command_call_t *)adv, send_size,
                        G_atmi_conf.reply_q_str,
                        G_atmi_conf.reply_q,
                        (mqd_t)FAIL,   /* do not keep open ndrxd q open */
                        G_atmi_conf.ndrxd_q_str,
                        0, NULL,
                        NULL,
                        NULL,
                        NULL,
                        FALSE);
    if (SUCCEED!=ret)
    {
        /*Just ignore the error*/
        if (!G_shm_srv)
        {
            NDRX_LOG(log_error, "Not attached to shm/ndrxd - ingore error");
            ret=SUCCEED;
        }    
        else
        {
          _TPset_error_fmt(TPESYSTEM, "Failed to send command %d to [%s]", 
                        NDRXD_COM_SRVUNADV_RQ, G_atmi_conf.ndrxd_q_str);  
        }
    }

out:
    return ret;
}

/**
 * We might get request during bufcall processing...
 * Also we need some timer, so that we can giveup finally...!
 * @param buf
 * @param len
 * @return 
 */
private int get_bridges_rply_request(char *buf, long len)
{
    int ret=SUCCEED;
    svc_entry_fn_t *entry = G_server_conf.service_array[ATMI_SRV_ADMIN_Q];
    command_call_t *p_adm_cmd = (command_call_t *)buf;
    /* we should re-queue back the stuff... */
    sleep(0); /* requeue stuff... */
    
    if (ATMI_COMMAND_EVPOST==p_adm_cmd->command_id)
    {
        NDRX_LOG(log_debug, "Resubmitting event postage to admin Q");
        ret = generic_qfd_send(entry->q_descr, buf, len, 0);
    }
    else
    {
        ret = process_admin_req(buf, len, &G_shutdown_req);
    }
    
    if (n_timer_get_delta_sec(&M_getbrs_timer) > G_atmi_env.time_out)
    {
        NDRX_LOG(log_error, "Did not get reply from ndrxd int time for "
                "bridge listing - FAIL!");
        ret=FAIL;
    }
    
    return ret;
}

/**
 * Get list of bridges connected to the domain.
 * err: We might get ping request during call.
 * this causes corruption of response.
 * !!!NOTE: Might want to store connected nodes in shared mem!!!!!!
 * ndrxd could update shared mem for bridges, for full refresh and for delete updates...
 * @return
 */
public int ndrxd_get_bridges(char *nodes_out)
{
    int ret=SUCCEED;
    command_call_t req;
    size_t  send_size=sizeof(command_call_t);
    command_reply_getbrs_t rply;
    int rply_len= sizeof(rply);
    svc_entry_fn_t *entry = G_server_conf.service_array[ATMI_SRV_ADMIN_Q];

    n_timer_reset(&M_getbrs_timer);
    
    memset(&req, 0, sizeof(req));
    memset(&rply, 0, sizeof(rply));
    
    /* We should enter our reply Q in blocked mode (so that we get response from NDRXD)! */
    ndrx_q_setblock(entry->q_descr, TRUE);

    /*
    NDRX_LOG(log_debug, "ndrxd_get_bridges: call flags=0x%x", req.flags);
    */
    ret=cmd_generic_bufcall(NDRXD_COM_SRVGETBRS_RQ, NDRXD_SRC_SERVER,
                        NDRXD_CALL_TYPE_GENERIC,
                        &req, send_size,
                        entry->listen_q,
                        entry->q_descr,
                        (mqd_t)FAIL,   /* do not keep open ndrxd q open */
                        G_atmi_conf.ndrxd_q_str,
                        0, NULL,
                        NULL,
                        NULL,
                        NULL,
                        TRUE,
                        FALSE,
                        (char *)&rply,
                        &rply_len,
                        0,
                        get_bridges_rply_request);
    if (SUCCEED!=ret)
    {
        /*Just ignore the error*/
        if (!G_shm_srv)
        {
            NDRX_LOG(log_error, "Not attached to shm/ndrxd - ingore error");
            ret=SUCCEED;
        }    
        else
        {
          _TPset_error_fmt(TPESYSTEM, "Failed to send command %d to [%s]", 
                        NDRXD_COM_SRVUNADV_RQ, G_atmi_conf.ndrxd_q_str);  
        }
    }
    else
    {
        if (rply_len != sizeof(command_reply_getbrs_t))
        {
            NDRX_LOG(log_error, "Invalid reply, got len: %d expected: %d",
                    rply_len, sizeof(command_reply_getbrs_t));
            FAIL_OUT(ret);
        }
        else
        {
            strcpy(nodes_out, rply.nodes);
        }
    }

out:
    /* Unblock the Q */
    ndrx_q_setblock(entry->q_descr, FALSE);

    return ret;
}


/**
 * Reply with ping response to ndrxd
 * @return
 */
public int pingrsp_to_ndrxd(command_srvping_t *ping)
{
    int ret=SUCCEED;
    
    ret=cmd_generic_call(NDRXD_COM_SRVPING_RP, NDRXD_SRC_SERVER,
                        NDRXD_CALL_TYPE_PM_INFO,
                        (command_call_t *)ping, sizeof(*ping),
                        G_atmi_conf.reply_q_str,
                        G_atmi_conf.reply_q,
                        (mqd_t)FAIL,   /* do not keep open ndrxd q open */
                        G_atmi_conf.ndrxd_q_str,
                        0, NULL,
                        NULL,
                        NULL,
                        NULL,
                        FALSE);
    if (SUCCEED!=ret)
    {
        /*Just ignore the error*/
        if (!G_shm_srv)
        {
            NDRX_LOG(log_error, "Not attached to shm/ndrxd - ingore error");
            ret=SUCCEED;
        }    
        else
        {
            NDRX_LOG(log_error, "Failed to reply on ping! seq=%d", 
                    ping->seq);
            userlog("Failed to reply with ping to ndrxd. srvid=%d seq=%d", 
                    ping->srvid, ping->seq);
        }
    }
    
out:
    return ret;
}
