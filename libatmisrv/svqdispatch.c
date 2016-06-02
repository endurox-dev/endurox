/* 
** EnduroX server main entry point
**
** @file svqdispatch.c
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

/*---------------------------Includes-----------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <sys_mqueue.h>
#include <errno.h>
#include <sys/stat.h>
#include <setjmp.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <utlist.h>
#include <string.h>
#include <fcntl.h>

#include "srv_int.h"
#include "xa_cmn.h"
#include <atmi_int.h>
#include <typed_buf.h>
#include <ntimer.h>
#include <atmi_shm.h>
#include <gencall.h>
#include <tperror.h>
#include <userlog.h>
/*---------------------------Externs------------------------------------*/
/* THIS IS HOOK FOR TESTING!! */
public void (*___G_test_delayed_startup)(void) = NULL;
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
public int G_shutdown_req = 0;
public int G_atmisrv_reply_type = 0; /* Used no-long-jump systems  */
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Open queues for listening.
 * @return 
 */
public int sv_open_queue(void)
{
    int ret=SUCCEED;
    int i;
    svc_entry_fn_t *entry;
    struct ex_epoll_event ev;
    int use_sem = FALSE;
    for (i=0; i<G_server_conf.adv_service_count; i++)
    {
        entry = G_server_conf.service_array[i];

        NDRX_LOG(log_debug, "About to listen on: %s", entry->listen_q);

        /* TODO: unlink the queue? If specific Q? admin or reply? */

        
        /* ###################### CRITICAL SECTION ############################### */
        
        /* Acquire semaphore here.... */
        if (G_shm_srv && EOS!=entry->svc_nm[0]) 
        {
            use_sem = TRUE;
        }
        
        if (use_sem && SUCCEED!=ndrx_lock_svc_op())
        {
            NDRX_LOG(log_error, "Failed to lock sempahore");
            ret=FAIL;
            goto out;
        }
        
        if (NULL!=___G_test_delayed_startup && use_sem)
        {
            ___G_test_delayed_startup();
        }
        
        /* Open the queue */
        entry->q_descr = ndrx_ex_mq_open_at (entry->listen_q, O_RDWR | O_CREAT |
                O_NONBLOCK, S_IWUSR | S_IRUSR, NULL);
        
        /*
         * Check are we ok or failed?
         */
        if ((mqd_t)FAIL==entry->q_descr)
        {
            /* Release semaphore! */
            if (use_sem) 
                ndrx_unlock_svc_op();
            
            _TPset_error_fmt(TPEOS, "Failed to open queue: %s: %s",
                                        entry->listen_q, strerror(errno));
            ret=FAIL;
            goto out;
        }
        
        /* Register stuff in shared memory! */
        if (use_sem)
        {
            ndrx_shm_install_svc(entry->svc_nm, 0);
        }

        /* Release semaphore! */
        if (G_shm_srv && EOS!=entry->svc_nm[0]) ndrx_unlock_svc_op();
        /* ###################### CRITICAL SECTION, END ########################## */
        
        /* Save the time when stuff is open! */
        n_timer_reset(&entry->qopen_time);

        NDRX_LOG(log_debug, "Got file descriptor: %d", entry->q_descr);
    }
    
    /* Register for (e-)polling */
    G_server_conf.epollfd = ex_epoll_create(G_server_conf.max_events);
    if (FAIL==G_server_conf.epollfd)
    {
        _TPset_error_fmt(TPEOS, "ex_epoll_create(%d) fail: %s",
                                G_server_conf.adv_service_count,
                                ex_poll_strerror(ex_epoll_errno()));
        ret=FAIL;
        goto out;
    }

    /* allocate events */
    G_server_conf.events = (struct ex_epoll_event *)calloc(sizeof(struct ex_epoll_event),
                                            G_server_conf.max_events);
    if (NULL==G_server_conf.events)
    {
        _TPset_error_fmt(TPEOS, "Failed to allocate epoll events: %s", strerror(errno));
        ret=FAIL;
        goto out;
    }

    /* Bind to epoll queue descriptors */    

    memset(&ev, 0, sizeof(ev));

    for (i=0; i<G_server_conf.adv_service_count; i++)
    {
        ev.events = EX_EPOLL_FLAGS;
#ifdef EX_USE_EPOLL
        ev.data.fd = G_server_conf.service_array[i]->q_descr;
#else
        ev.data.mqd = G_server_conf.service_array[i]->q_descr;
#endif
        
        if (FAIL==ex_epoll_ctl_mq(G_server_conf.epollfd, EX_EPOLL_CTL_ADD,
                                G_server_conf.service_array[i]->q_descr, &ev))
        {
            _TPset_error_fmt(TPEOS, "ex_epoll_ctl failed: %s", ex_poll_strerror(ex_epoll_errno()));
            ret=FAIL;
            goto out;
        }
    }

out:
    return ret;
}

/**
 * Serve service call
 * @return
 */
public int sv_serve_call(int *service, int *status)
{
    int ret=SUCCEED;
    char *request_buffer = NULL;
    long req_len = 0;
    int reply_type;
    typed_buffer_descr_t *call_type;
    tp_command_call_t *call = (tp_command_call_t*)G_server_conf.last_call.buf_ptr;
    buffer_obj_t *outbufobj=NULL; /* Have a reference to allocated buffer */
    long call_age;
    int generate_rply = FALSE;
    
    *status=SUCCEED;
    G_atmisrv_reply_type = 0;
    
    call_age = n_timer_get_delta_sec(&call->timer);

    NDRX_LOG(log_debug, "got call, cd: %d timestamp: %d callseq: %u, "
			"svc: %s, flags: %ld call age: %ld data_len: %ld",
                    	call->cd, call->timestamp, call->callseq, 
			call->name, call->flags, call_age, call->data_len);
    
    if (G_atmi_env.time_out>0 && call_age >= G_atmi_env.time_out && 
            !(call->flags & TPNOTIME))
    {
        NDRX_LOG(log_warn, "Received call already expired! "
                "call age = %ld s, timeout = %d s", call_age, G_atmi_env.time_out);
        *status=FAIL;
        goto out;
    }

    /* Prepare the call buffer */
    if (call->data_len > 0)
    {
        /* Assume that received data is valid. */
	/* 20/08/2014 - have some more debugs - we get core dumps here! */
        NDRX_LOG(6, "Recevied len = %ld buffer_type_id = %hd", 
			call->data_len, call->buffer_type_id);
	/* Validate buffer type id, otherwise we get core */
        if (call->buffer_type_id < BUF_TYPE_MIN || 
            call->buffer_type_id  > BUF_TYPE_MAX )
        {
            NDRX_LOG(log_always, "Invalid buffer type received %hd"
                                        "min = %d max %d",
                            call->buffer_type_id, BUF_TYPE_MIN, BUF_TYPE_MAX);
            *status=FAIL;
            generate_rply = TRUE;
            goto out;
        }
        call_type = &G_buf_descr[call->buffer_type_id];
        
        ret=call_type->pf_prepare_incoming(call_type,
                        call->data,
                        call->data_len,
                        &request_buffer,
                        &req_len,
                        0L);

        if (SUCCEED!=ret)
        {

            /* TODO: Reply with failure - TPEOTYPE - type not supported! */
            *status=FAIL;
            generate_rply = TRUE;
            goto out;
        }
        else
        {
            /* this must succeed */
            outbufobj=find_buffer(request_buffer);
            outbufobj->autoalloc = 1; /* We have stuff autoallocated! */
            NDRX_LOG(log_debug, "Buffer=%p autoalloc=%hd", 
                    outbufobj->buf, outbufobj->autoalloc);
        }
    }

    /* Now we should call the service by it self, also we should check was reply back or not */

    if (G_libatmisrv_flags & ATMI_SRVLIB_NOLONGJUMP ||
            0==(reply_type=setjmp(G_server_conf.call_ret_env)))
    {
        int no = G_server_conf.last_call.no;
        TPSVCINFO svcinfo;
        memset(&svcinfo, 0, sizeof(TPSVCINFO));

        svcinfo.data = request_buffer;
        svcinfo.len = req_len;
        strcpy(svcinfo.name, call->name);
        svcinfo.flags = call->flags;
        svcinfo.cd = call->cd;
        G_last_call = *call; /* save last call info to ATMI library
                              * (this does excludes data by default) */
        
        /* Register global tx */
        if (EOS!=call->tmxid[0] && 
                SUCCEED!=_tp_srv_join_or_new_from_call(call, FALSE))
        {
            NDRX_LOG(log_error, "Failed to start/join global tx!");
            
            /* TODO: We have died here... so the dispatcher must
             * return TPFAIL, and we should notify master, that this RM is
             * failed!!!!
             */
            *status=FAIL;
            generate_rply = TRUE;
            goto out;
        }
        
        /* If we run in abort only mode and do some forwards & etc.
         * Then we should keep the abort status.
         Moved to tmisabortonly! flag field.
        if (G_atmi_xa_curtx.txinfo && (G_last_call.sysflags & SYS_XA_ABORT_ONLY))
        {
            NDRX_LOG(log_warn, "Marking current global tx as ABORT_ONLY");
            G_atmi_xa_curtx.txinfo->tmisabortonly = TRUE;
        }
         * */
        
        /* call the function */
        *service=no-ATMI_SRV_Q_ADJUST;
        if (G_shm_srv)
        {
            G_shm_srv->svc_status[*service] = NDRXD_SVC_STATUS_BUSY;
            /* put reply address */
            strcpy(G_shm_srv->last_reply_q, call->reply_to);
        }
        
        /* We need to convert buffer here (if function set...) */
        if (NULL!=request_buffer &&
                G_server_conf.service_array[no]->xcvtflags)
        {
            /* 
             * Mark that buffer is converted...
             * So that later we can convert back...
             */
            G_last_call.sysflags|= G_server_conf.service_array[no]->xcvtflags;
            call->sysflags |= G_server_conf.service_array[no]->xcvtflags;
            
            if (SUCCEED!=typed_xcvt(&outbufobj, call->sysflags, FALSE))
            {
                NDRX_LOG(log_debug, "Failed to convert buffer service "
                            "format: %llx", G_last_call.sysflags);
                userlog("Failed to convert buffer service "
                            "format: %llx", G_last_call.sysflags);
                *status=FAIL;
                generate_rply = TRUE;
                goto out;
            }
            else
            {
                svcinfo.data = outbufobj->buf;
                svcinfo.len = outbufobj->size;
            }
        }
        
        /* For golang integration we need to know at service the function name */
        strcpy(svcinfo.fname, G_server_conf.service_array[no]->fn_nm);
        
        if (FAIL!=*status) /* Dot not invoke if failed! */
        {
            G_server_conf.service_array[no]->p_func(&svcinfo);
        }
        
        if (G_libatmisrv_flags & ATMI_SRVLIB_NOLONGJUMP &&
                /* Server did return:  */
                (G_atmisrv_reply_type & RETURN_TYPE_TPRETURN || 
                 G_atmisrv_reply_type & RETURN_TYPE_TPFORWARD
                )
            )
        {
            /* System does normal function return... */
            NDRX_LOG(log_debug, "Got back from reply/forward (%d) w/o long jump",
                                        G_atmisrv_reply_type);
            if (G_atmisrv_reply_type & RETURN_FAILED || 
                    G_atmisrv_reply_type & RETURN_SVC_FAIL)
            {
                *status=FAIL;
            }
        }
        else
        {
            NDRX_LOG(log_warn, "No return from service!");

            if (!(svcinfo.flags & TPNOREPLY))
            {
                /* if we are here, then there was no reply! */
                NDRX_LOG(log_error, "PROTO error - no reply from service [%s]",
                                                call->name);
                /* reply with failure back */
                *status=FAIL;
                goto out;
            }
        }
    }
    else
    {
        NDRX_LOG(log_debug, "Got back from reply/forward (%d)",
                                        reply_type);
        if (reply_type & RETURN_FAILED || reply_type & RETURN_SVC_FAIL)
        {
            *status=FAIL;
        }
    }
    
out:

    if (generate_rply)
    {
        /* Reply back with failure... */
        ndrx_reply_with_failure(call, TPNOBLOCK, TPESVCERR, G_atmi_conf.reply_q_str);
    }

    /* free_up_buffers(); - services assumes that memory is alloced for all the time
     * i.e. they do manual management of memory: tpfree.
     */ 
    /* We shall find auto allocated buffer and remove it! */
    free_auto_buffers();
    
    return ret;
}

/**
 * Serve service call
 * TODO: we need XA handling here too!
 * @return
 */
public int sv_serve_connect(int *service, int *status)
{
    int ret=SUCCEED;
    char *request_buffer = NULL;
    long req_len = 0;
    int reply_type;
    typed_buffer_descr_t *call_type;
    tp_command_call_t *call = (tp_command_call_t*)G_server_conf.last_call.buf_ptr;
    *status=SUCCEED;
    long call_age;

    *status=SUCCEED;
    G_atmisrv_reply_type = 0;
    
    NDRX_LOG(log_debug, "got connect, cd: %d timestamp: %d callseq: %u",
                                        call->cd, call->timestamp, call->callseq);
    
    call_age = n_timer_get_delta_sec(&call->timer);
    
    if (G_atmi_env.time_out>0 && call_age >= G_atmi_env.time_out && 
            !(call->flags & TPNOTIME))
    {
        NDRX_LOG(log_warn, "Received call already expired! "
                "call age = %ld s, timeout = %d s", call_age, G_atmi_env.time_out);
        *status=FAIL;
        goto out;
    }
    
    /* Prepare the call buffer */
    /* TODO: Check buffer type - if invalid this should raise segfault! */
    /* We can have data len 0! */
    if (call->data_len>0)
    {
        call_type = &G_buf_descr[call->buffer_type_id];

        ret=call_type->pf_prepare_incoming(call_type,
                        call->data,
                        call->data_len,
                        &request_buffer,
                        &req_len,
                        0L);
    
        if (SUCCEED!=ret)
        {

            /* TODO: Reply with failure - TPEOTYPE - type not supported! */
            goto out;
        }

    }

    /* Now we should call the service by it self, also we should check was reply back or not */

    if (G_libatmisrv_flags & ATMI_SRVLIB_NOLONGJUMP || 0==(reply_type=setjmp(G_server_conf.call_ret_env)))
    {
        int no = G_server_conf.last_call.no;
        TPSVCINFO svcinfo;
        memset(&svcinfo, 0, sizeof(TPSVCINFO));

        if (call->data_len>0)
        {
            svcinfo.data = request_buffer;
            svcinfo.len = req_len;
        }
        else
        {
            NDRX_LOG(log_warn, "Connection W/O data");
            /* no data available */
            svcinfo.data = NULL;
            svcinfo.len = 0;
        }

        strcpy(svcinfo.name, call->name);
        svcinfo.flags = call->flags;
        svcinfo.cd = call->cd;
        G_last_call = *call; /* save last call info to ATMI library
                              * (this does excludes data by default) */

        /* Because we are in conversation, we should make a special cd
         * for case when we are as server
         * This will be cd + MAX, meaning, that we have called.
         */
        svcinfo.cd+=MAX_CONNECTIONS;
        G_last_call.cd+=MAX_CONNECTIONS;
        NDRX_LOG(log_debug, "Read cd=%d making as %d (+%d - we are server!)",
                                               call->cd, svcinfo.cd, MAX_CONNECTIONS);


        /* At this point we should build up conversation queues
         * Open for read their queue, and open for write our queue to listen
         * on.
         */
        if (FAIL==accept_connection())
        {
            ret=FAIL;

            /* Try hardly to send SVCFAIL/ERR response back! */
            reply_with_failure(0, &G_last_call, NULL, NULL, TPESVCERR);
            
            goto out;
        }

        /* Register global tx */
        if (EOS!=call->tmxid[0] && 
                SUCCEED!=_tp_srv_join_or_new_from_call(call, FALSE))
        {
            NDRX_LOG(log_error, "Failed to start/join global tx!");
            
            /* TODO: We have died here... so the dispatcher must
             * return TPFAIL, and we should notify master, that this RM is
             * failed!!!!
             */
            *status=FAIL;
        }
        
        /* If we run in abort only mode and do some forwards & etc.
         * Then we should keep the abort status.
         Moved to tmisabortonly! flag field.
        if (G_atmi_xa_curtx.txinfo && (G_last_call.sysflags & SYS_XA_ABORT_ONLY))
        {
            NDRX_LOG(log_warn, "Marking current global tx as ABORT_ONLY");
            G_atmi_xa_curtx.txinfo->tmisabortonly = TRUE;
        }
         */
 

        /* call the function */
        *service=no-ATMI_SRV_Q_ADJUST;
        if (G_shm_srv)
        {
            G_shm_srv->svc_status[*service] = NDRXD_SVC_STATUS_BUSY;
            /* put reply address */
            strcpy(G_shm_srv->last_reply_q, call->reply_to);
        }
        /* For golang integration we need to know at service the function name */
        strcpy(svcinfo.fname, G_server_conf.service_array[no]->fn_nm);
        G_server_conf.service_array[no]->p_func(&svcinfo);
        
        /*Needs some patch for go-lang that we do not use long jumps...
         * + we we need to get the status back...
         */
        if (G_libatmisrv_flags & ATMI_SRVLIB_NOLONGJUMP &&
                /* Server did return:  */
                (G_atmisrv_reply_type & RETURN_TYPE_TPRETURN || 
                 G_atmisrv_reply_type & RETURN_TYPE_TPFORWARD
                )
            )
        {
            NDRX_LOG(log_debug, "Got back from reply/forward (%d) (no longjmp)",
                                        G_atmisrv_reply_type);
        
            if (G_atmisrv_reply_type & RETURN_FAILED || 
                    G_atmisrv_reply_type & RETURN_SVC_FAIL)
            {
                *status=FAIL;
            }
        }
        else
        {
            NDRX_LOG(log_warn, "No return from service!");

            if (!(svcinfo.flags & TPNOREPLY))
            {
                /* if we are here, then there was no reply! */
                NDRX_LOG(log_error, "PROTO error - no reply from service [%s]",
                                                call->name);
                /* reply with failure back */
                *status=FAIL;
            }
        }
    }
    else
    {
        NDRX_LOG(log_debug, "Got back from reply/forward (%d)",
                                        reply_type);
        
        if (reply_type & RETURN_FAILED || reply_type & RETURN_SVC_FAIL)
        {
            *status=FAIL;
        }
        
    }
    
out:

    /* free_up_buffers(); - processes manages memory manuall!!! */
    if (NULL!=request_buffer)
    {
        _tpfree(request_buffer, NULL);
    }
    return ret;
}

/**
 * Decode received request & do the operation
 * @param buf
 * @param len
 * @return
 */
public int sv_server_request(char *buf, int len)
{
    int ret=SUCCEED;

    tp_command_generic_t *gen_command = (tp_command_generic_t *)G_server_conf.last_call.buf_ptr;
    n_timer_t timer;
    /* take time */
    n_timer_reset(&timer);
    int service = FAIL;
    int status;
    
    /*if we are bridge, then no more processing required!*/
    if (G_server_conf.flags & SRV_KEY_FLAGS_BRIDGE)
    {
        if (NULL!=G_server_conf.p_qmsg)
        {
            if (SUCCEED!=G_server_conf.p_qmsg(buf, len, BR_NET_CALL_MSG_TYPE_ATMI))
            {
                NDRX_LOG(log_error, "Failed to process ATMI request on bridge!");
                FAIL_OUT(ret);
            }
        }
        else
        {
            NDRX_LOG(log_error, "This is no p_qmsg for bridge!");
            
        }
        /* go out, nothing to do new... */
        goto out;
    }
    

    NDRX_LOG(log_debug, "Got command: %hd", gen_command->command_id);

    if (G_shm_srv)
    {
        G_shm_srv->status = NDRXD_SVC_STATUS_BUSY;
        G_shm_srv->last_command_id = gen_command->command_id;
    }

    switch (gen_command->command_id)
    {
        case ATMI_COMMAND_TPCALL:
        case ATMI_COMMAND_EVPOST:

            ret=sv_serve_call(&service, &status);

            break;
        case ATMI_COMMAND_CONNECT:
            /* We have connection for converstation */
            ret=sv_serve_connect(&service, &status);
            break;
        case ATMI_COMMAND_CONNRPLY:
            {
                tp_command_call_t *call = (tp_command_call_t*)G_server_conf.last_call.buf_ptr;
                NDRX_LOG(log_warn, "Dropping un-soliceded/event reply "
                                                "cd: %d callseq: %u timestamp: %d",
                        call->cd, call->callseq, call->timestamp);
                /* Register as completed (if not cancelled) */
                cancel_if_expected(call);
            }
            break;
        default:
            NDRX_LOG(log_error, "Unknown command ID: %hd", gen_command->command_id);
            
            /* Dump the message to log... */
            NDRX_DUMP(log_error, "Command content", buf,  len);
            
            ret=FAIL;
            goto out;
            break;
    }

    /* Update stats, if ptr available */
    if (FAIL!=service && G_shm_srv)
    {
        unsigned result = n_timer_get_delta(&timer);

        /* reset back to avail. */
        G_shm_srv->svc_status[service] = NDRXD_SVC_STATUS_AVAIL;
        G_shm_srv->status = NDRXD_SVC_STATUS_AVAIL;

        /* update timeing */
        /* min, if this is first time, then update directly */
        if (0==G_shm_srv->svc_succeed[service] && 0==G_shm_srv->svc_fail[service])
        {
            G_shm_srv->min_rsp_msec[service]=result;
        }
        else if (result<G_shm_srv->min_rsp_msec[service])
        {
            G_shm_srv->min_rsp_msec[service]=result;
        }

#if 0
        - not actual.
        /* Fix issues with strange minuses... */
        if (G_shm_srv->min_rsp_msec[service] < 0)
            G_shm_srv->min_rsp_msec[service] = 0;
#endif
        
        /* max */
        if (result>G_shm_srv->max_rsp_msec[service])
            G_shm_srv->max_rsp_msec[service]=result;
#if 0
        - not actual.
        /* Fix issues with strange minuses... */
        if (G_shm_srv->max_rsp_msec[service] < 0)
            G_shm_srv->max_rsp_msec[service] = 0;
#endif
        
        G_shm_srv->last_rsp_msec[service]=result;

        if (status==SUCCEED)
        {
            /* Loop over to zero */
            if (INT_MAX==G_shm_srv->svc_succeed[service])
            {
                G_shm_srv->svc_succeed[service] = 0;
            }
            
            G_shm_srv->svc_succeed[service]++;
            
        }
        else
        {
            /* Loop over to zero */
            if (INT_MAX==G_shm_srv->svc_fail[service])
            {
                G_shm_srv->svc_fail[service] = 0;
            }
            
            G_shm_srv->svc_fail[service]++;
            
            /* If we are in global transaction,
             * then we shall notify the master RM of failure
             * or this will be done by caller. The master buffer will be marked
             * as abort only, because response is not ok.
             * 
             * But if we have called with tpacall() and no reply needed.
             * And we fail, then we need to mark the transaction as bad...
             * RM should know that....
             */
            
            /* TODO: But note... We might still need the transaction flags
             * Maybe we already know that transaction is abort only...!
             *  */
            _tp_srv_tell_tx_fail();
        }
        
        /* If we were in global tx, then we have to disassociate us from tx...*/
        if (G_atmi_xa_curtx.txinfo)
        {
            _tp_srv_disassoc_tx();
        }
    }

out:
    return ret;
}

/**
 * Process admin request
 * @param buf
 * @param len
 * @param shutdown_req
 * @return SUCCEED/FAIL
 */
public int process_admin_req(char *buf, long len, int *shutdown_req)
{
    int ret=SUCCEED;

    command_call_t * call = (command_call_t *)buf;

    /* So what, do shutdown, right? */
    if (NDRXD_COM_SRVSTOP_RQ==call->command)
    {
        NDRX_LOG(log_warn, "Shutdown requested by [%s]", 
                                        call->reply_queue);
        *shutdown_req=TRUE;
    }
    else if (NDRXD_COM_SRVINFO_RQ==call->command)
    {
        NDRX_LOG(log_warn, "Server info requested by [%s]",
                                        call->reply_queue);
        /* Send details to ndrxd */
        report_to_ndrxd();
    }
    else if (NDRXD_COM_NXDUNADV_RQ==call->command)
    {
        command_dynadvertise_t *call_srv = (command_dynadvertise_t *)call;
        
        NDRX_LOG(log_warn, "Server requested unadvertise service [%s] by [%s]",
                                        call_srv->svc_nm, call->reply_queue);
        /* Send details to ndrxd */
        dynamic_unadvertise(call_srv->svc_nm, NULL, NULL);
    }
    else if (NDRXD_COM_NXDREADV_RQ==call->command)
    {
        command_dynadvertise_t *call_srv = (command_dynadvertise_t *)call;
        
        NDRX_LOG(log_warn, "Server requested readvertise service [%s] by [%s]",
                                        call_srv->svc_nm, call->reply_queue);
        /* Send details to ndrxd */
        dynamic_readvertise(call_srv->svc_nm);
    }
    else if (NDRXD_COM_SRVPING_RQ==call->command)
    {
        command_srvping_t *call_srv = (command_srvping_t *)call;
        if (call_srv->srvid == G_server_conf.srv_id)
        {
            NDRX_LOG(log_debug, "Got ping request: %d seq", 
                                                call_srv->seq);
            pingrsp_to_ndrxd(call_srv);
        }
    }
    else
    {
        /*if we are bridge, then no more processing required!*/
        if (G_server_conf.flags & SRV_KEY_FLAGS_BRIDGE)
        {
            if (NULL!=G_server_conf.p_qmsg)
            {
                if (SUCCEED!=G_server_conf.p_qmsg(buf, len, BR_NET_CALL_MSG_TYPE_NDRXD))
                {
                    NDRX_LOG(log_error, "Failed to process ATMI request on bridge!");
                    FAIL_OUT(ret);
                }
            }
            else
            {
                NDRX_LOG(log_error, "This is no p_qmsg for brdige!");
                goto out;
            }
        }
    }
    
out:
    return ret;
}

/**
 * Enter server in waiting state
 * @return
 */
public int sv_wait_for_request(void)
{
    int ret=SUCCEED;
    int nfds, n, len, j;
    unsigned prio;
    char msg_buf[ATMI_MSG_MAX_SIZE];
    int again;
    int tout;
    pollextension_rec_t *ext;
    int evfd;
    mqd_t evmqd;
    n_timer_t   dbg_time;   /* Generally this is used for debug. */
    n_timer_t   periodic_cb;
    command_call_t *p_adm_cmd = (command_call_t *)msg_buf;
    tp_command_call_t *call =  (tp_command_call_t*)msg_buf;
    
    if (G_server_conf.periodcb_sec)
    {
        tout = G_server_conf.periodcb_sec*1000;
    }
    else
    {
        tout=FAIL; /* Timeout disabled */
    }
    
    n_timer_reset(&dbg_time);
    n_timer_reset(&periodic_cb);
    
    /* THIS IS MAIN SERVER LOOP! */
    while(SUCCEED==ret && !G_shutdown_req)
	{
        /* Support for periodical invocation of custom function! */
        
        /* Invoke before poll function */
        if (G_server_conf.p_b4pollcb
                && SUCCEED!=G_server_conf.p_b4pollcb())
        {
            ret=FAIL;
            goto out;
        }
        
        /* We want this debug but if using callbacks, then it might get too many in trace files
         * so we just put timer there if tout used, and no timer if no tout use.
         * If tout used, then 60 sec will be ok for dbug
         */
        if (FAIL==tout || n_timer_get_delta_sec(&dbg_time) >= 60)
        {
            NDRX_LOG(log_debug, "About to poll - timeout=%d millisec", 
                                                tout);
            if (FAIL!=tout)
            {
                n_timer_reset(&dbg_time);
            }
        }
        
        nfds = ex_epoll_wait(G_server_conf.epollfd, G_server_conf.events, 
                G_server_conf.max_events, tout);
        
        /* Print stuff if there is no timeout set or there is some value out there */
        
        if (nfds || FAIL==tout)
        {
            NDRX_LOG(log_debug, "Poll says: %d", nfds);
        }
        
        /* If there are zero FDs &  */
        if (FAIL==nfds)
        {
            int err = errno;
            _TPset_error_fmt(TPEOS, "epoll_pwait failed: %s", 
                    ex_poll_strerror(ex_epoll_errno()));
            
            if (EINTR==err)
            {
                continue;
            }
            
            ret=FAIL;
            goto out;
		}
        /* We should use timer here because, if there are service requests at
         * constant time (less than poll time), then callback will be never called! */
        else if (FAIL!=tout && 
                n_timer_get_delta_sec(&periodic_cb) >= G_server_conf.periodcb_sec)
        {
            if (NULL!=G_server_conf.p_periodcb &&
                    SUCCEED!=(ret=G_server_conf.p_periodcb()))
            {
                NDRX_LOG(log_error, "Periodical callback function "
                        "failed!!! With ret=%d", ret);
                goto out;
            }
            
            n_timer_reset(&periodic_cb);
        }
        
        /*
         * TODO: We should have algorythm which checks request in round robin way.
         * So that if there is big load for first service, and there is outstanding calls to second,
         * then we should process also the second.
         * Maybe not? Maybe kernel already does that?
         */
        for (n = 0; n < nfds; n++)
        {
            int is_mq_only = FAIL;
            evfd = G_server_conf.events[n].data.fd;
            evmqd = G_server_conf.events[n].data.mqd;

#ifndef EX_USE_EPOLL
            NDRX_LOG(log_debug, "not epoll()");
            /* for non linux, we need to distinguish between fd & mq */
            is_mq_only = G_server_conf.events[n].is_mqd;
#endif

            NDRX_LOG(log_debug, "Receiving %d, user data: %d, fd: %d, is_mq_only: %d, G_pollext=%p",
                        n, G_server_conf.events[n].data.u32, evfd, is_mq_only, G_pollext);
            
            /* Check poller extension */
            if (NULL!=G_pollext && (FAIL==is_mq_only || FALSE==is_mq_only) )
            {
                ext=ext_find_poller(evfd);
                
                if (NULL!=ext)
                {
                    NDRX_LOG(log_info, "FD found in extension list, invoking");
                    
                    ret = ext->p_pollevent(evfd, G_server_conf.events[n].events, ext->ptr1);
                    if (SUCCEED!=ret)
                    {
                        NDRX_LOG(log_error, "p_pollevent at 0x%lx failed!!!",
                                ext->p_pollevent);
                        goto out;
                    }
                    else
                    {
                        continue;
                    }
                }
            }
            
            /* ignore fds  */
            if (FALSE==is_mq_only)
            {
                continue;
            }
            
            if (FAIL==(len=ex_mq_receive (evmqd,
                (char *)msg_buf, ATMI_MSG_MAX_SIZE, &prio)))
            {
                if (EAGAIN==errno)
                {
                    /* In this case we ignore the error and try on next q
                     * This may happen in cases when in load balance mode
                     * someone else took the message off.
                     */
                    NDRX_LOG(log_debug, "EAGAIN");
                    continue; /* try to get next message */
                }
                else
                {
                    ret=FAIL;
                    _TPset_error_fmt(TPEOS, "ex_mq_receive failed: %s", strerror(errno));
                }
            }
            else
            {
                /* NDRX_LOG(log_debug, "Got request"); */
                /* OK, we got the message and now we can call the service */
                /*G_server_conf.service_array[n]->p_func((TPSVCINFO *)msg_buf);*/

                /* We generally here will ingore returned error code! */

                /* TODO: We could use hashtable to lookup for lookup*/
                /* figure out target structure */
                G_server_conf.last_call.no=FAIL;
                for (j=0; j<G_server_conf.adv_service_count; j++)
                {
                    if (evmqd==G_server_conf.service_array[j]->q_descr)
                    {
                        G_server_conf.last_call.no = j;
                        break;
                    }
                }
                NDRX_LOG(log_debug, "Got request on logical channel %d, fd: %d",
                            G_server_conf.last_call.no, evmqd);
                
                if (ATMI_SRV_ADMIN_Q==G_server_conf.last_call.no && 
                        ATMI_COMMAND_EVPOST!=p_adm_cmd->command_id)
                {
                    NDRX_LOG(log_debug, "Got admin request");
                    ret=process_admin_req(msg_buf, len, &G_shutdown_req);
                }
                else
                {   
                    /* If this was even post, then we should adjust last call no */
                    if (ATMI_COMMAND_EVPOST==p_adm_cmd->command_id)
                    {
                        G_server_conf.last_call.no = FAIL;
                        for (j=0; j<G_server_conf.adv_service_count; j++)
                        {
                            if (0==strcmp(G_server_conf.service_array[j]->svc_nm,
                                    call->name))
                            {
                               G_server_conf.last_call.no = j;
                               break;
                            }
                        }
                        if (FAIL==G_server_conf.last_call.no)
                        {
                            NDRX_LOG(log_error, "Failed to find service: [%s] "
                                    "- ignore event call!", call->name);
                            continue;
                        }
                    }
                    
                    /* This normally should not happen! */
                    if (FAIL==G_server_conf.last_call.no)
                    {
                        _TPset_error_fmt(TPESYSTEM, "No service entry for call descriptor %d",
                                    evmqd);
                        ret=FAIL;
                        goto out;
                    }

                    G_server_conf.last_call.buf_ptr = msg_buf;
                    G_server_conf.last_call.len = len;

                    sv_server_request(msg_buf, len);
                }
            }
        } /* for */
    }
out:
    return ret;
}
