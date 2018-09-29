/**
 * @brief Enduro/X server main entry point
 *
 * @file svqdispatch.c
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

/*---------------------------Includes-----------------------------------*/
#include <ndrx_config.h>
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
#include <nstopwatch.h>
#include <atmi_shm.h>
#include <gencall.h>
#include <tperror.h>
#include <userlog.h>
#include <atmi.h>
/*---------------------------Externs------------------------------------*/
/* THIS IS HOOK FOR TESTING!! */
expublic void (*___G_test_delayed_startup)(void) = NULL;
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
expublic int G_shutdown_req = 0;

/* Only for poll() mode: */
expublic int G_shutdown_nr_wait = 0;   /* Number of self shutdown messages to wait */
expublic int G_shutdown_nr_got = 0;    /* Number of self shutdown messages got  */

/* Used no-long-jump systems, this should be thread object based.
 * Here we will not use context as the one thread that start the processing
 * will finish it.
 * Also maybe not so actual here, as we expect that main poller will run only
 * in one thread.
 */
expublic int G_atmisrv_reply_type = 0; 
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Open queues for listening.
 * @return 
 */
expublic int sv_open_queue(void)
{
    int ret=EXSUCCEED;
    int i;
    svc_entry_fn_t *entry;
    struct ndrx_epoll_event ev;
    int use_sem = EXFALSE;
    
    
    /* Register for (e-)polling 
     * moved up here for system v resources have be known when installing to SHM
     */
    G_server_conf.epollfd = ndrx_epoll_create(G_server_conf.max_events);
    if (EXFAIL==G_server_conf.epollfd)
    {
        ndrx_TPset_error_fmt(TPEOS, "ndrx_epoll_create(%d) fail: %s",
                                G_server_conf.adv_service_count,
                                ndrx_poll_strerror(ndrx_epoll_errno()));
        ret=EXFAIL;
        goto out;
    }
    
    for (i=0; i<G_server_conf.adv_service_count; i++)
    {
        entry = G_server_conf.service_array[i];

        NDRX_LOG(log_debug, "About to listen on: %s", entry->listen_q);

        /* TODO: unlink the queue? If specific Q? admin or reply? */

        
        /* ###################### CRITICAL SECTION ############################### */
        
        /* Acquire semaphore here.... */
        if (G_shm_srv && EXEOS!=entry->svc_nm[0]) 
        {
            use_sem = EXTRUE;
        }
        
        if (use_sem && EXSUCCEED!=ndrx_lock_svc_op(__func__))
        {
            NDRX_LOG(log_error, "Failed to lock sempahore");
            ret=EXFAIL;
            goto out;
        }
        
        if (NULL!=___G_test_delayed_startup && use_sem)
        {
            ___G_test_delayed_startup();
        }
        
        /* Open the queue */
#ifdef EX_USE_POLL
        /* for poll mode, we must ensure that queue does not exists before start
         */
        if (EXSUCCEED!=ndrx_mq_unlink(entry->listen_q))
        {
            NDRX_LOG(log_debug, "debug: Failed to unlink [%s]: %s", entry->listen_q, 
                    ndrx_poll_strerror(ndrx_epoll_errno()))
        }
#endif
        /* open service Q, also give some svc name here!  */
        
        if (ndrx_epoll_shallopensvc(i))
        {       
            /* normal operations, each service have it's own queue... */
            entry->q_descr = ndrx_mq_open_at (entry->listen_q, O_RDWR | O_CREAT |
                    O_NONBLOCK, S_IWUSR | S_IRUSR, NULL);
            
            if ((mqd_t)EXFAIL!=entry->q_descr)
            {
                /* re-define service, used for particular systems... like system v */
                entry->q_descr=ndrx_epoll_service_add(entry->svc_nm, i, entry->q_descr);
            }
        }
        else
        {
            /* System V mode, where services does not require separate queue  */
            entry->q_descr = ndrx_epoll_service_add(entry->svc_nm, 
                    i, (mqd_t)EXFAIL);
        }
        
        /*
         * Check are we ok or failed?
         */
        if ((mqd_t)EXFAIL==entry->q_descr)
        {
            /* Release semaphore! */
            if (use_sem) 
                ndrx_unlock_svc_op(__func__);
            
            ndrx_TPset_error_fmt(TPEOS, "Failed to open queue: %s: %s",
                                        entry->listen_q, strerror(errno));
            ret=EXFAIL;
            goto out;
        }
        
        /* Register stuff in shared memory! */
        if (use_sem)
        {
#ifdef EX_USE_SYSVQ
            ndrx_shm_install_svc(entry->svc_nm, 0, ndrx_epoll_resid_get());
#else
            ndrx_shm_install_svc(entry->svc_nm, 0, G_server_conf.srv_id);
#endif
        }

        /* Release semaphore! */
        if (G_shm_srv && EXEOS!=entry->svc_nm[0]) ndrx_unlock_svc_op(__func__);
        /* ###################### CRITICAL SECTION, END ########################## */
        
        /* Save the time when stuff is open! */
        ndrx_stopwatch_reset(&entry->qopen_time);

        NDRX_LOG(log_debug, "Got file descriptor: %d", entry->q_descr);
    }

    /* allocate events */
    G_server_conf.events = (struct ndrx_epoll_event *)NDRX_CALLOC(sizeof(struct ndrx_epoll_event),
                                            G_server_conf.max_events);
    if (NULL==G_server_conf.events)
    {
        ndrx_TPset_error_fmt(TPEOS, "Failed to allocate epoll events: %s", 
                strerror(errno));
        ret=EXFAIL;
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
        /* this is ok, more accurate */
        ev.data.mqd = G_server_conf.service_array[i]->q_descr;
#endif
        
        if (EXFAIL==ndrx_epoll_ctl_mq(G_server_conf.epollfd, EX_EPOLL_CTL_ADD,
                                G_server_conf.service_array[i]->q_descr, &ev))
        {
            ndrx_TPset_error_fmt(TPEOS, "ndrx_epoll_ctl failed: %s", 
                    ndrx_poll_strerror(ndrx_epoll_errno()));
            ret=EXFAIL;
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
expublic int sv_serve_call(int *service, int *status)
{
    int ret=EXSUCCEED;
    char *request_buffer = NULL;
    long req_len = 0;
    int reply_type;
    typed_buffer_descr_t *call_type;
    tp_command_call_t *call = (tp_command_call_t*)G_server_conf.last_call.buf_ptr;
    buffer_obj_t *outbufobj=NULL; /* Have a reference to allocated buffer */
    long call_age;
    int generate_rply = EXFALSE;
    tp_command_call_t * last_call;
    *status=EXSUCCEED;
    G_atmisrv_reply_type = 0;
    atmi_lib_env_t * env = ndrx_get_G_atmi_env();
    
    call_age = ndrx_stopwatch_get_delta_sec(&call->timer);

    NDRX_LOG(log_debug, "got call, cd: %d timestamp: %d (id: %d%d) callseq: %u, "
			"svc: %s, flags: %ld, call age: %ld, data_len: %ld, caller: %s "
                        " reply_to: %s, clttout: %d",
                    	call->cd, call->timestamp, call->cd, call->timestamp, call->callseq, 
			call->name, call->flags, call_age, call->data_len,
                        call->my_id, call->reply_to, call->clttout);
    
    if (call->clttout > 0 && call_age >= call->clttout && 
            !(call->flags & TPNOTIME))
    {
        NDRX_LOG(log_warn, "Received call already expired! "
                "call age = %ld s, client timeout = %d s", call_age, call->clttout);
        *status=EXFAIL;
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
            *status=EXFAIL;
            generate_rply = EXTRUE;
            goto out;
        }
        call_type = &G_buf_descr[call->buffer_type_id];
        
        ret=call_type->pf_prepare_incoming(call_type,
                        call->data,
                        call->data_len,
                        &request_buffer,
                        &req_len,
                        0L);

        if (EXSUCCEED!=ret)
        {

            /* TODO: Reply with failure - TPEOTYPE - type not supported! */
            *status=EXFAIL;
            generate_rply = EXTRUE;
            goto out;
        }
        else
        {
            /* this must succeed */
            outbufobj=ndrx_find_buffer(request_buffer);
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
        NDRX_STRCPY_SAFE(svcinfo.name, call->name);
        svcinfo.flags = call->flags;
        svcinfo.cd = call->cd;
        
        /* set the client id to caller */
        NDRX_STRCPY_SAFE(svcinfo.cltid.clientdata, (char *)call->my_id);
        last_call = ndrx_get_G_last_call();
        memcpy(last_call, call, sizeof(tp_command_call_t));
                             /* save last call info to ATMI library
                              * (this does excludes data by default) */
        
        /* Register global tx */
        if (EXEOS!=call->tmxid[0] && 
                EXSUCCEED!=_tp_srv_join_or_new_from_call(call, EXFALSE))
        {
            NDRX_LOG(log_error, "Failed to start/join global tx!");
            
            /* TODO: We have died here... so the dispatcher must
             * return TPFAIL, and we should notify master, that this RM is
             * failed!!!!
             */
            *status=EXFAIL;
            generate_rply = EXTRUE;
            goto out;
        }
        
        /* If we run in abort only mode and do some forwards & etc.
         * Then we should keep the abort status.
         Moved to tmisabortonly! flag field.
        if (ndrx_get_G_atmi_xa_curtx()->txinfo && (ndrx_get_last_call()->sysflags & SYS_XA_ABORT_ONLY))
        {
            NDRX_LOG(log_warn, "Marking current global tx as ABORT_ONLY");
            ndrx_get_G_atmi_xa_curtx()->txinfo->tmisabortonly = TRUE;
        }
         * */
        
        /* call the function */
        *service=no-ATMI_SRV_Q_ADJUST;
        if (G_shm_srv)
        {
            G_shm_srv->svc_status[*service] = NDRXD_SVC_STATUS_BUSY;
            /* put reply address */
            NDRX_STRCPY_SAFE(G_shm_srv->last_reply_q, call->reply_to);
        }
        
        /* We need to convert buffer here (if function set...) */
        if (NULL!=request_buffer &&
                G_server_conf.service_array[no]->xcvtflags && 
                
                /* convert in case when really needed */
                ( 
                  /* UBF2JSON */
                  (BUF_TYPE_UBF == outbufobj->type_id && 
                    SYS_SRV_CVT_UBF2JSON & G_server_conf.service_array[no]->xcvtflags)
                ||
                  (BUF_TYPE_JSON == outbufobj->type_id && SYS_SRV_CVT_JSON2UBF & 
                    G_server_conf.service_array[no]->xcvtflags)
                
                  /* VIEW2JSON */
                || (BUF_TYPE_VIEW == outbufobj->type_id && 
                    SYS_SRV_CVT_VIEW2JSON & G_server_conf.service_array[no]->xcvtflags)
                ||
                  (BUF_TYPE_JSON == outbufobj->type_id && SYS_SRV_CVT_JSON2VIEW & 
                    G_server_conf.service_array[no]->xcvtflags)
                
                )
            )
        {
            /* 
             * Mark that buffer is converted...
             * So that later we can convert back...
             */
            last_call->sysflags|= G_server_conf.service_array[no]->xcvtflags;
            call->sysflags |= G_server_conf.service_array[no]->xcvtflags;
            
            if (EXSUCCEED!=typed_xcvt(&outbufobj, call->sysflags, EXFALSE))
            {
                NDRX_LOG(log_debug, "Failed to convert buffer service "
                            "format: %llx", last_call->sysflags);
                userlog("Failed to convert buffer service "
                            "format: %llx", last_call->sysflags);
                *status=EXFAIL;
                generate_rply = EXTRUE;
                goto out;
            }
            else
            {
                svcinfo.data = outbufobj->buf;
                svcinfo.len = outbufobj->size;
            }
        }
        
        last_call->autobuf = outbufobj;
        
        /* For golang integration we need to know at service the function name */
        NDRX_STRCPY_SAFE(svcinfo.fname, G_server_conf.service_array[no]->fn_nm);
        
        if (EXFAIL!=*status) /* Dot not invoke if failed! */
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
                *status=EXFAIL;
            }
        }
        else if (G_libatmisrv_flags & ATMI_SRVLIB_NOLONGJUMP &&
                G_atmisrv_reply_type & RETURN_TYPE_THREAD)
        {
            NDRX_LOG(log_warn, "tpcontinue() issued from integra (no longjmp)!");
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
                *status=EXFAIL;
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
            *status=EXFAIL;
        }
    }
    
out:

    if (generate_rply)
    {
        /* Reply back with failure... */
        ndrx_reply_with_failure(call, TPNOBLOCK, TPESVCERR, 
                ndrx_get_G_atmi_conf()->reply_q_str);
    }

    /* free_up_buffers(); - services assumes that memory is alloced for all the time
     * i.e. they do manual management of memory: tpfree.
     */ 
    /* We shall find auto allocated buffer and remove it! */

    /* for integra continue leave the buffers in place.. 
     * We will do this per call return buffer bases & if buffer is auto.
    if (!(G_libatmisrv_flags & ATMI_SRVLIB_NOLONGJUMP &&
                G_atmisrv_reply_type & RETURN_TYPE_THREAD))
    {
        free_auto_buffers();
    }
    */
    
    return ret;
}

/**
 * Serve service call
 * TODO: we need XA handling here too!
 * @return
 */
expublic int sv_serve_connect(int *service, int *status)
{
    int ret=EXSUCCEED;
    char *request_buffer = NULL;
    long req_len = 0;
    int reply_type;
    typed_buffer_descr_t *call_type;
    tp_command_call_t *call = (tp_command_call_t*)G_server_conf.last_call.buf_ptr;
    *status=EXSUCCEED;
    long call_age;
    atmi_lib_env_t *env = ndrx_get_G_atmi_env();
    tp_command_call_t * last_call = ndrx_get_G_last_call();

    *status=EXSUCCEED;
    G_atmisrv_reply_type = 0;
    
    NDRX_LOG(log_debug, "got connect, cd: %d timestamp: %d callseq: %hu, clttout",
                        call->cd, call->timestamp, call->callseq, call->clttout);
    
    call_age = ndrx_stopwatch_get_delta_sec(&call->timer);
    
    if (call->clttout > 0 && call_age >= call->clttout && 
            !(call->flags & TPNOTIME))
    {
        NDRX_LOG(log_warn, "Received call already expired! "
                "call age = %ld s, client timeout = %d s", call_age, call->clttout);
        *status=EXFAIL;
        goto out;
    }
    
    /* Prepare the call buffer */
    /* TODO: Check buffer type - if invalid this should raise segfault! */
    /* We can have data len 0! */
    if (call->data_len > 0)
    {
        call_type = &G_buf_descr[call->buffer_type_id];

        ret=call_type->pf_prepare_incoming(call_type,
                        call->data,
                        call->data_len,
                        &request_buffer,
                        &req_len,
                        0L);
    
        if (EXSUCCEED!=ret)
        {

            /* TODO: Reply with failure - TPEOTYPE - type not supported! */
            goto out;
        }

    }

    /* Now we should call the service by it self, also we should check was reply back or not */

    if (G_libatmisrv_flags & ATMI_SRVLIB_NOLONGJUMP || 
            0==(reply_type=setjmp(G_server_conf.call_ret_env)))
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

        NDRX_STRCPY_SAFE(svcinfo.name, call->name);
        svcinfo.flags = call->flags;
        svcinfo.cd = call->cd;
        /* set the client id to caller */
        NDRX_STRCPY_SAFE(svcinfo.cltid.clientdata, (char *)call->my_id);
        
        
        *last_call = *call; /* save last call info to ATMI library
                              * (this does excludes data by default) */
        
        if (NULL!=request_buffer)
        {
            last_call->autobuf=ndrx_find_buffer(request_buffer);
            last_call->autobuf->autoalloc = 1; 
        }
        else
        {
            last_call->autobuf = NULL;
        }

        /* Because we are in conversation, we should make a special cd
         * for case when we are as server
         * This will be cd + MAX, meaning, that we have called.
         */
        svcinfo.cd+=MAX_CONNECTIONS;
        last_call->cd+=MAX_CONNECTIONS;
        NDRX_LOG(log_debug, "Read cd=%d making as %d (+%d - we are server!)",
                                        call->cd, svcinfo.cd, MAX_CONNECTIONS);


        /* At this point we should build up conversation queues
         * Open for read their queue, and open for write our queue to listen
         * on.
         */
        if (EXFAIL==accept_connection())
        {
            ret=EXFAIL;

            /* Try hardly to send SVCFAIL/ERR response back! */
            reply_with_failure(0, last_call, NULL, NULL, TPESVCERR);
            
            goto out;
        }

        /* Register global tx */
        if (EXEOS!=call->tmxid[0] && 
                EXSUCCEED!=_tp_srv_join_or_new_from_call(call, EXFALSE))
        {
            NDRX_LOG(log_error, "Failed to start/join global tx!");
            
            /* TODO: We have died here... so the dispatcher must
             * return TPFAIL, and we should notify master, that this RM is
             * failed!!!!
             */
            *status=EXFAIL;
        }
        
        /* If we run in abort only mode and do some forwards & etc.
         * Then we should keep the abort status.
         Moved to tmisabortonly! flag field.
        if (ndrx_get_G_atmi_xa_curtx()->txinfo && (ndrx_get_last_call()->sysflags & SYS_XA_ABORT_ONLY))
        {
            NDRX_LOG(log_warn, "Marking current global tx as ABORT_ONLY");
            ndrx_get_G_atmi_xa_curtx()->txinfo->tmisabortonly = TRUE;
        }
         */
 

        /* call the function */
        *service=no-ATMI_SRV_Q_ADJUST;
        if (G_shm_srv)
        {
            G_shm_srv->svc_status[*service] = NDRXD_SVC_STATUS_BUSY;
            /* put reply address */
            NDRX_STRCPY_SAFE(G_shm_srv->last_reply_q, call->reply_to);
        }
        /* For golang integration we need to know at service the function name */
        NDRX_STRCPY_SAFE(svcinfo.fname, G_server_conf.service_array[no]->fn_nm);
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
                *status=EXFAIL;
            }
        }
        else if (G_libatmisrv_flags & ATMI_SRVLIB_NOLONGJUMP &&
                G_atmisrv_reply_type & RETURN_TYPE_THREAD)
        {
            NDRX_LOG(log_warn, "tpcontinue() issued from integra (no longjmp)!");
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
                *status=EXFAIL;
            }
        }
    }
    else
    {
        NDRX_LOG(log_debug, "Got back from reply/forward (%d)",
                                        reply_type);
        
        if (reply_type & RETURN_FAILED || reply_type & RETURN_SVC_FAIL)
        {
            *status=EXFAIL;
        }
        
    }
    
out:

    /* free_up_buffers(); - processes manages memory manually!!! */
/*
	mvitolin 26/01/2017 - let the tpreturn free the auto buffer
    if (NULL!=request_buffer)
    {
        _tpfree(request_buffer, NULL);
    }
*/
    return ret;
}

/**
 * Decode received request & do the operation
 * @param buf
 * @param len
 * @return
 */
expublic int sv_server_request(char *buf, int len)
{
    int ret=EXSUCCEED;
    tp_command_generic_t *gen_command = (tp_command_generic_t *)G_server_conf.last_call.buf_ptr;
    ndrx_stopwatch_t timer;
    /* take time */
    ndrx_stopwatch_reset(&timer);
    int service = EXFAIL;
    int status;
    
    /*if we are bridge, then no more processing required!*/
    if (G_server_conf.flags & SRV_KEY_FLAGS_BRIDGE)
    {
        if (NULL!=G_server_conf.p_qmsg)
        {
            if (EXSUCCEED!=G_server_conf.p_qmsg(buf, len, BR_NET_CALL_MSG_TYPE_ATMI))
            {
                NDRX_LOG(log_error, "Failed to process ATMI request on bridge!");
                EXFAIL_OUT(ret);
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

            ret=sv_serve_call(&service, &status);

            break;
        case ATMI_COMMAND_CONNECT:
            /* We have connection for conversation */
            ret=sv_serve_connect(&service, &status);
            break;
        case ATMI_COMMAND_SELF_SD:
            
            G_shutdown_nr_got++;
            
            NDRX_LOG(log_warn, "Got shutdown req %d of %d", 
                    G_shutdown_nr_got, G_shutdown_nr_wait);
            goto out;
            
            break;
        case ATMI_COMMAND_CONNRPLY:
            {
                tp_command_call_t *call = (tp_command_call_t*)G_server_conf.last_call.buf_ptr;
                NDRX_LOG(log_warn, "Dropping unsolicited/event reply "
                                        "cd: %d callseq: %u timestamp: %d",
                        call->cd, call->callseq, call->timestamp);
                /* Register as completed (if not cancelled) */
                cancel_if_expected(call);
            }
            break;
        case ATMI_COMMAND_TPREPLY:
            {
                tp_command_call_t *call = (tp_command_call_t*)G_server_conf.last_call.buf_ptr;
                NDRX_LOG(log_warn, "Dropping unsolicited reply "
                                        "cd: %d callseq: %u timestamp: %d",
                        call->cd, call->callseq, call->timestamp);
                
                NDRX_DUMP(log_error, "Command content", G_server_conf.last_call.buf_ptr,  
                        G_server_conf.last_call.len);
                
                ndrx_dump_call_struct(log_error, call);
            }
            break;
        case ATMI_COMMAND_TPNOTIFY:
        case ATMI_COMMAND_BROADCAST:
            {
                /* Got broadcast message, just use ATMI lib internal
                 * dispatcher...
                 */
                tp_notif_call_t *notif = (tp_notif_call_t*)G_server_conf.last_call.buf_ptr;
                char *request_buffer = NULL;
                long req_len = 0;
                typed_buffer_descr_t *call_type;
                
                call_type = &G_buf_descr[notif->buffer_type_id];
                
                NDRX_LOG(log_info, "Doing local %s... (buffer type %hd)",
                     (ATMI_COMMAND_TPNOTIFY==gen_command->command_id?"tpnotify":"tpbroadcast"),
                        notif->buffer_type_id);
                
                /* How about prepare incoming buffer? */
                
                if (0==notif->data_len ||
                        EXSUCCEED==call_type->pf_prepare_incoming(call_type,
                        notif->data,
                        notif->data_len,
                        &request_buffer,
                        &req_len,
                        0L))
                {
                    NDRX_LOG(log_debug, "ATMI Command id: %d", 
                            gen_command->command_id);
                    if (ATMI_COMMAND_TPNOTIFY==gen_command->command_id)
                    {
                        TPMYID myid;
                        CLIENTID *clt;
                    
                        /* NDRX_LOG(log_debug, "Doing notify..."); */
                        clt = (CLIENTID *)notif->destclient; /* same char arr.. */
                        
                        if (EXSUCCEED!=ndrx_myid_parse(notif->destclient, &myid, EXFALSE))
                        {
                            NDRX_LOG(log_error, "Failed to parse client: [%s]",
                                    notif->destclient);
                            EXFAIL_OUT(ret);
                        }
                        
                        ret=ndrx_tpnotify((CLIENTID *)notif->destclient, /* basically clientid */
                                &myid, 
                                NULL, 
                                request_buffer, 
                                req_len, 
                                notif->flags,
                                myid.nodeid,
                                (notif->nodeid_isnull?NULL:notif->nodeid),
                                (notif->usrname_isnull?NULL:notif->usrname),
                                (notif->cltname_isnull?NULL:notif->cltname),
                                 0L);
                    }
                    else
                    {
                        NDRX_LOG(log_debug, "Doing tpbroadcast... flags = %ld, is regexp=%d", 
                                notif->flags, notif->flags&TPREGEXMATCH);
                        
                        NDRX_LOG(log_debug, "notif->nodeid_isnull: %d (%s)", 
                                notif->nodeid_isnull, notif->nodeid);
                        NDRX_LOG(log_debug, "notif->usrname_isnull: %d (%s)", 
                                notif->usrname_isnull, notif->usrname);
                        NDRX_LOG(log_debug, "notif->cltname_isnull: %d (%s)", 
                                notif->cltname_isnull, notif->cltname);
                        
                        ret=ndrx_tpbroadcast_local((notif->nodeid_isnull?NULL:notif->nodeid),
                                (notif->usrname_isnull?NULL:notif->usrname),
                                (notif->cltname_isnull?NULL:notif->cltname),
                                request_buffer, req_len, notif->flags, EXTRUE);
                    }
                    
                    if (NULL!=request_buffer)
                    {
                        tpfree(request_buffer);
                    }
                    
                    if (EXSUCCEED!=ret)
                    {
                        NDRX_LOG(log_error, "Local notification/broadcast failed");
                    }
                }
                
            }
            break;
        default:
            NDRX_LOG(log_error, "Unknown command ID: %hd", gen_command->command_id);
            
            /* Dump the message to log... */
            NDRX_DUMP(log_error, "Command content", buf,  len);
            
            EXFAIL_OUT(ret);
            break;
    }

    /* Update stats, if ptr available */
    if (EXFAIL!=service && G_shm_srv)
    {
        unsigned result = ndrx_stopwatch_get_delta(&timer);

        /* reset back to avail. */
        G_shm_srv->svc_status[service] = NDRXD_SVC_STATUS_AVAIL;
        G_shm_srv->status = NDRXD_SVC_STATUS_AVAIL;

        /* update timing */
        /* min, if this is first time, then update directly */
        if (0==G_shm_srv->svc_succeed[service] && 0==G_shm_srv->svc_fail[service])
        {
            G_shm_srv->min_rsp_msec[service]=result;
        }
        else if (result<G_shm_srv->min_rsp_msec[service])
        {
            G_shm_srv->min_rsp_msec[service]=result;
        }
        
        /* max */
        if (result>G_shm_srv->max_rsp_msec[service])
            G_shm_srv->max_rsp_msec[service]=result;
        
        G_shm_srv->last_rsp_msec[service]=result;

        if (status==EXSUCCEED)
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
        if (ndrx_get_G_atmi_xa_curtx()->txinfo)
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
expublic int process_admin_req(char *buf, long len, int *shutdown_req)
{
    int ret=EXSUCCEED;
    tp_command_generic_t shut_msg; /* shutdown msg */
    int i;

    command_call_t * call = (command_call_t *)buf;

    /* So what, do shutdown, right? */
    if (NDRXD_COM_SRVSTOP_RQ==call->command)
    {
        
        NDRX_LOG(log_warn, "Shutdown requested by [%s]", 
                                        call->reply_queue);
        *shutdown_req=EXTRUE;
#ifdef EX_USE_POLL
        /* TODO: We shall send request to all open service queues
         * to do the shutdown. This is only for poll() mode.
         */
        memset(&shut_msg, 0, sizeof(shut_msg));
        
        shut_msg.command_id = ATMI_COMMAND_SELF_SD;
        
        /* Send over all open service queues: */
        
        for (i=ATMI_SRV_Q_ADJUST; i<G_server_conf.adv_service_count; i++)
        {
            if (EXSUCCEED!=ndrx_generic_qfd_send(G_server_conf.service_array[i]->q_descr, 
                    (char *)&shut_msg, sizeof(shut_msg), 0))
            {
                NDRX_LOG(log_debug, "Failed to send self notification to %s q",
                        G_server_conf.service_array[i]->listen_q);
            }
            else
            {
                G_shutdown_nr_wait++;
            }
        }/* for */
        
        NDRX_LOG(log_warn, "Send %d self notifications to "
                "service queues for shutdown...", G_shutdown_nr_wait);
#endif
        
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
                if (EXSUCCEED!=G_server_conf.p_qmsg(buf, len, BR_NET_CALL_MSG_TYPE_NDRXD))
                {
                    NDRX_LOG(log_error, "Failed to process ATMI request on bridge!");
                    EXFAIL_OUT(ret);
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
expublic int sv_wait_for_request(void)
{
    int ret=EXSUCCEED;
    int nfds, n, len, j;
    unsigned prio;
    char msg_buf[NDRX_MSGSIZEMAX];
    int again;
    int tout;
    pollextension_rec_t *ext;
    int evfd;
    mqd_t evmqd;
    ndrx_stopwatch_t   dbg_time;   /* Generally this is used for debug. */
    ndrx_stopwatch_t   periodic_cb;
    
    if (G_server_conf.periodcb_sec)
    {
        tout = G_server_conf.periodcb_sec*1000;
    }
    else
    {
        tout=EXFAIL; /* Timeout disabled */
    }
    
    ndrx_stopwatch_reset(&dbg_time);
    ndrx_stopwatch_reset(&periodic_cb);
    
    /* THIS IS MAIN SERVER LOOP! */
    while(EXSUCCEED==ret && (!G_shutdown_req || 
            /* if shutdown request then wait for all queued jobs to finish. */
            G_shutdown_nr_got <  G_shutdown_nr_wait))
    {
        /* Support for periodical invocation of custom function! */
        
        /* Invoke before poll function */
        if (G_server_conf.p_b4pollcb
                && EXSUCCEED!=G_server_conf.p_b4pollcb())
        {
            ret=EXFAIL;
            goto out;
        }
        
        /* We want this debug but if using callbacks, then it might get too many in trace files
         * so we just put timer there if tout used, and no timer if no tout use.
         * If tout used, then 60 sec will be ok for dbug
         */
        if (EXFAIL==tout || ndrx_stopwatch_get_delta_sec(&dbg_time) >= 60)
        {
            NDRX_LOG(log_debug, "About to poll - timeout=%d millisec", 
                                                tout);
            if (EXFAIL!=tout)
            {
                ndrx_stopwatch_reset(&dbg_time);
            }
        }
        
        /* some epoll backends can return already buffer received
         * for example System V Queues
         * for others it is just -1
         * 
         * So the plan is following for SystemV:
         * - first service Q is open as real queue and main dispatcher
         * - the other queues are just open as some dummy pointers with
         *  service name recorded inside and we return it as mqd_t
         *  so that pointers can be compared
         * - when epoll_wait gets the message, we trace down the pointer
         *  by the service name in the message and then we return then
         *  we will events correspondingly.
         */
        len = sizeof(msg_buf);
        nfds = ndrx_epoll_wait(G_server_conf.epollfd, G_server_conf.events, 
                G_server_conf.max_events, tout, msg_buf, &len);
        
        /* Print stuff if there is no timeout set or there is some value out there */
        
        if (nfds || EXFAIL==tout)
        {
            NDRX_LOG(log_debug, "Poll says: %d len: %d", nfds, len);
        }
        
        /* If there are zero FDs &  */
        if (EXFAIL==nfds)
        {
            int err = errno;
            ndrx_TPset_error_fmt(TPEOS, "epoll_pwait failed: %s", 
                    ndrx_poll_strerror(ndrx_epoll_errno()));
            
            if (EINTR==err)
            {
                continue;
            }
            
            ret=EXFAIL;
            goto out;
		}
        /* We should use timer here because, if there are service requests at
         * constant time (less than poll time), then callback will be never called! */
        else if (EXFAIL!=tout && 
                ndrx_stopwatch_get_delta_sec(&periodic_cb) >= G_server_conf.periodcb_sec)
        {
            if (NULL!=G_server_conf.p_periodcb &&
                    EXSUCCEED!=(ret=G_server_conf.p_periodcb()))
            {
                NDRX_LOG(log_error, "Periodical callback function "
                        "failed!!! With ret=%d", ret);
                goto out;
            }
            
            ndrx_stopwatch_reset(&periodic_cb);
        }
        
        /*
         * TODO: We should have algorithm which checks request in round robin way.
         * So that if there is big load for first service, and there is outstanding calls to second,
         * then we should process also the second.
         * Maybe not? Maybe kernel already does that?
         */
        for (n = 0; n < nfds; n++)
        {
            int is_mq_only = EXFAIL;
            evfd = G_server_conf.events[n].data.fd;
            evmqd = G_server_conf.events[n].data.mqd;

/* so this way will work for POLL & Kqueue */
#if !defined(EX_USE_FDPOLL) && !defined(EX_USE_EPOLL)
            NDRX_LOG(log_debug, "not epoll()");
            /* for non linux, we need to distinguish between fd & mq */
            is_mq_only = G_server_conf.events[n].is_mqd;
#endif

            NDRX_LOG(log_debug, "Receiving %d, user data: %d, fd: %d, evmqd: %d, "
                    "is_mq_only: %d, G_pollext=%p",
                    n, G_server_conf.events[n].data.u32, evfd, evmqd, 
                    is_mq_only, G_pollext);
            
            /* Check poller extension */
            if (NULL!=G_pollext && (EXFAIL==is_mq_only || EXFALSE==is_mq_only) )
            {
                ext=ext_find_poller(evfd);
                
                if (NULL!=ext)
                {
                    NDRX_LOG(log_info, "FD found in extension list, invoking");
                    
                    ret = ext->p_pollevent(evfd, G_server_conf.events[n].events, ext->ptr1);
                    if (EXSUCCEED!=ret)
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
            if (EXFALSE==is_mq_only)
            {
                continue;
            }
            
            if (EXFAIL==len && EXFAIL==(len=ndrx_mq_receive (evmqd,
                (char *)msg_buf, sizeof(msg_buf), &prio)))
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
                    ret=EXFAIL;
                    ndrx_TPset_error_fmt(TPEOS, "ndrx_mq_receive failed: %s", 
                            strerror(errno));
                }
            }
            else
            {   
                /* OK, we got the message and now we can call the service */
                /*G_server_conf.service_array[n]->p_func((TPSVCINFO *)msg_buf);*/

                /* We generally here will ingore returned error code! */

                /* TODO: We could use hashtable to lookup for lookup*/
                /* figure out target structure */
                G_server_conf.last_call.no=EXFAIL;
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
                
                if (ATMI_SRV_ADMIN_Q==G_server_conf.last_call.no)
                {
                    NDRX_LOG(log_debug, "Got admin request");
                    ret=process_admin_req(msg_buf, len, &G_shutdown_req);
                }
                else
                {   
                    /* This normally should not happen! */
                    if (EXFAIL==G_server_conf.last_call.no)
                    {
                        ndrx_TPset_error_fmt(TPESYSTEM, "No service entry for "
                                "call descriptor %d", evmqd);
                        ret=EXFAIL;
                        goto out;
                    }
                    
                    /* Save on the big message copy... */
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
/* vim: set ts=4 sw=4 et smartindent: */
