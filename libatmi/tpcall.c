/* 
** ATMI tpcall function implementation (Common version)
** This will have to make the call and wait back for answer.
** This should pass to function file descriptor for reply back queue.
**
** @file tpcall.c
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

/*---------------------------Includes-----------------------------------*/
#include <stdio.h>
#include <stdarg.h>
#include <memory.h>
#include <stdlib.h>
#include <errno.h>
#include <sys_mqueue.h>
#include <fcntl.h>

#include <atmi.h>
#include <userlog.h>
#include <ndebug.h>
#include <tperror.h>
#include <typed_buf.h>
#include <atmi_int.h>

#include "../libatmisrv/srv_int.h"
#include "ndrxd.h"
#include "utlist.h"
#include <thlock.h>
#include <xa_cmn.h>
#include <atmi_shm.h>
#include <atmi_tls.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
EX_SPIN_LOCKDECL(M_cd_lock);
EX_SPIN_LOCKDECL(M_callseq_lock);
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
exprivate void unlock_call_descriptor(int cd, short status);

/**
 * Dump to the log Command call buffer
 * @param lev Debug level
 * @param call call struct addr
 */
expublic void ndrx_dump_call_struct(int lev, tp_command_call_t *call)
{
    ndrx_debug_t * dbg = debug_get_ndrx_ptr();
    if (dbg->level>=lev)
    {
        NDRX_LOG(lev, "=== Start of tp_command_call_t call dump, ptr=%p ===", call);
        NDRX_LOG(lev, "command_id=[%hd]", call->command_id);
        NDRX_LOG(lev, "proto_ver=[%c%c%c%c]", call->proto_ver[0], call->proto_ver[1],
            call->proto_ver[2], call->proto_ver[3]);
        NDRX_LOG(lev, "proto_magic=[%d]", call->proto_magic);
        NDRX_LOG(lev, "buffer_type_id=[%hd]", call->buffer_type_id);
        NDRX_LOG(lev, "name=[%s]", call->name);
        NDRX_LOG(lev, "reply_to=[%s]", call->reply_to);
        NDRX_LOG(lev, "callstack=[%s]", call->callstack);
        NDRX_LOG(lev, "my_id=[%s]", call->my_id);
        NDRX_LOG(lev, "sysflags=[%p]", call->sysflags);
        NDRX_LOG(lev, "cd=[%d]", call->cd);
        NDRX_LOG(lev, "rval=[%d]", call->rval);
        NDRX_LOG(lev, "rcode=[%ld]", call->rcode);
        NDRX_LOG(lev, "extradata=[%s]", call->extradata);
        NDRX_LOG(lev, "flags=[%p]", call->flags);
        NDRX_LOG(lev, "timestamp=[%lu]", call->timestamp);
        NDRX_LOG(lev, "callseq=[%hu]", call->callseq);
        NDRX_LOG(lev, "timer.tv_nsec=[%lu]", call->timer.t.tv_nsec);
        NDRX_LOG(lev, "timer.tv_sec=[%lu]", call->timer.t.tv_sec);
        NDRX_LOG(lev, "tmtxflags=[0x%x]", call->tmtxflags);
        NDRX_LOG(lev, "tmxid=[%s]", call->tmxid);
        NDRX_LOG(lev, "tmrmid=[%hd]", call->tmrmid);
        NDRX_LOG(lev, "tmnodeid=[%hd]", call->tmnodeid);
        NDRX_LOG(lev, "tmsrvid=[%hd]", call->tmsrvid);
        NDRX_LOG(lev, "tmknownrms=[%s]", call->tmknownrms);
        NDRX_LOG(lev, "data_len=[%ld]", call->data_len);
        NDRX_LOG(lev, "===== End of tp_command_call_t call dump, ptr=%p ===", call);
    }
}

/**
 * Process individual call in progress.
 * @param cd
 * @return 
 */
exprivate int call_check_tout(int cd)
{
    int ret=EXSUCCEED;
    time_t t;
    int t_diff;
    /* ATMI_TLS_ENTRY; - already called by parent processes - already called by parent processes  */
    
    if (CALL_WAITING_FOR_ANS==G_atmi_tls->G_call_state[cd].status &&
            !(G_atmi_tls->G_call_state[cd].flags & TPNOTIME) &&
              (t_diff = ((t = time(NULL)) - G_atmi_tls->G_call_state[cd].timestamp)) > G_atmi_env.time_out
            )
    {
        /* added some more debug info, because we have strange timeouts... */
        NDRX_LOG(log_warn, "cd %d (callseq %u) timeout condition - generating error "
                "(locked at: %ld current tstamp: %ld, diff: %d, timeout_value: %d)", 
                cd, G_atmi_tls->G_call_state[cd].callseq, 
                G_atmi_tls->G_call_state[cd].timestamp, t, t_diff, G_atmi_env.time_out);
        
        _TPset_error_fmt(TPETIME, "cd %d (callseq %u) timeout condition - generating error "
                "(locked at: %ld current tstamp: %ld, diff: %d, timeout_value: %d)", 
                cd, G_atmi_tls->G_call_state[cd].callseq, 
                G_atmi_tls->G_call_state[cd].timestamp, t, t_diff, G_atmi_env.time_out);
        
        /* mark cd as free (will mark as cancelled) */
        unlock_call_descriptor(cd, CALL_CANCELED);
        
        ret=EXFAIL;
        goto out;
    }
out:
    return ret;
}

/**
 * Function for extra debug.
 */
exprivate void call_dump_descriptors(void)
{
    int i;
    time_t t = time(NULL);
    int t_diff;
    ATMI_TLS_ENTRY;
    
    NDRX_LOG(log_warn, "***List of call descriptors waiting for answer***");
    NDRX_LOG(log_warn, "timeout(system wide): %d curr_tstamp: %ld", 
                            G_atmi_env.time_out, t);
    NDRX_LOG(log_warn, "cd\tcallseq\tlocked_at\tdiff");
        
    for (i=1; i<MAX_ASYNC_CALLS; i++)
    {
        if (CALL_WAITING_FOR_ANS==G_atmi_tls->G_call_state[i].status)
        {
            t_diff = t - G_atmi_tls->G_call_state[i].timestamp;
            NDRX_LOG(log_warn, "%d\t%u\t%ld\t%d", 
                    i, G_atmi_tls->G_call_state[i].callseq, 
                    G_atmi_tls->G_call_state[i].timestamp, t_diff);
        }
    }
    
    NDRX_LOG(log_warn, "*************************************************");
}

#define CALL_TOUT_DEBUG

/**
 * We should check all call descriptors and imitate time-out condition,
 * if any of calls are timed-out
 * 
 * TODO: hash vs iterate...
 * @param cd - if > 0 the
 * @param cd_out - return failed cd...
 * @return 
 */
exprivate int call_scan_tout(int cd, int *cd_out)
{
    /* for performance reasons shouldn't we keep in HASH list 
     * the list of open call descriptors, iterate them and check for
     * time-out condition
     */
    int ret = EXSUCCEED;
    int i;
    long delta = 0;
    /* ATMI_TLS_ENTRY; - already called by parent*/
    /* NOTE: no need for mutexes, as we have call descriptors per TLS */
    
#ifdef CALL_TOUT_DEBUG
    call_dump_descriptors();
#endif
    
    /* Check that it is time for scan... */
    if (G_atmi_tls->tpcall_first || 
            (delta=ndrx_stopwatch_get_delta(&G_atmi_tls->tpcall_start)) >=1000 || 
                /* incase of overflow: */
                delta < 0)
    {
        /* we should scan the stuff. */
        if (0 < cd)
        {
            if (EXSUCCEED!=call_check_tout(cd))
            {
                *cd_out = cd;
                ret=EXFAIL;
                goto out;
            }
        }
        else
        {
            for (i=1; i<MAX_ASYNC_CALLS; i++)
            {
                if (EXSUCCEED!=call_check_tout(i))
                {
                    *cd_out = i;
                    ret=EXFAIL;
                    goto out;
                }
            }
        }
        /* if all ok, schedule after 1 sec. */
        ndrx_stopwatch_reset(&G_atmi_tls->tpcall_start);
        G_atmi_tls->tpcall_first = EXFALSE; /* only when all ok... */
    } /* if check time... */
    
out:

    return ret;
}

/**
 * Return next call sequence number
 * The number is shared between conversational and standard call
 * @param p_callseq ptr to return next number into
 * @return 
 */
expublic unsigned short ndrx_get_next_callseq_shared(void)
{
    static volatile unsigned short shared_callseq=0;
    
    EX_SPIN_LOCK_V(M_callseq_lock);
    shared_callseq++;
    EX_SPIN_UNLOCK_V(M_callseq_lock);
    
    return shared_callseq;
}

/**
 * Returns free call descriptro
 * @return >0 (ok), -1 = FAIL
 */
exprivate int get_call_descriptor_and_lock(unsigned short *p_callseq,
        time_t timestamp, long flags)
{
    int start_cd = G_atmi_tls->tpcall_get_cd; /* mark where we began */
    int ret = EXFAIL;
    unsigned short callseq=0;
    
    /* ATMI_TLS_ENTRY; - already got from caller */
    /* Lock the call descriptor giver...! So that we have common CDs 
     * over the hreads
     */
    
    while (CALL_WAITING_FOR_ANS==G_atmi_tls->G_call_state[G_atmi_tls->tpcall_get_cd].status)
    {
        G_atmi_tls->tpcall_get_cd++;

        if (G_atmi_tls->tpcall_get_cd > MAX_ASYNC_CALLS-1)
        {
            G_atmi_tls->tpcall_get_cd=1; /* TODO: Maybe start with 0? */
        }

        if (start_cd==G_atmi_tls->tpcall_get_cd)
        {
            NDRX_LOG(log_debug, "All call descriptors overflow restart!");
            break;
        }
    }

    if (CALL_WAITING_FOR_ANS==G_atmi_tls->G_call_state[G_atmi_tls->tpcall_get_cd].status)
    {
        NDRX_LOG(log_debug, "All call descriptors have been taken - FAIL!");
        /* MUTEX_UNLOCK_V(M_cd_lock); */
        EXFAIL_OUT(ret);
    }
    else
    {
        callseq = ndrx_get_next_callseq_shared();
        
        ret = G_atmi_tls->tpcall_get_cd;
        *p_callseq=callseq;
        NDRX_LOG(log_debug, "Got free call descriptor %d, callseq: %u",
                                            ret, callseq);

        NDRX_LOG(log_debug, "cd %d locked to %d timestamp callseq: %u",
                                        ret, timestamp, callseq);
        G_atmi_tls->G_call_state[ret].status = CALL_WAITING_FOR_ANS;
        G_atmi_tls->G_call_state[ret].timestamp = timestamp;
        G_atmi_tls->G_call_state[ret].callseq = callseq;
        G_atmi_tls->G_call_state[ret].flags = flags;

        /* MUTEX_UNLOCK_V(M_cd_lock); */
            
        /* If we have global tx open, then register cd under it! */
        if (!(flags & TPNOTRAN) && G_atmi_tls->G_atmi_xa_curtx.txinfo)
        {
            NDRX_LOG(log_debug, "Registering cd=%d under global "
                    "transaction!", ret);
            if (EXSUCCEED!=atmi_xa_cd_reg(&(G_atmi_tls->G_atmi_xa_curtx.txinfo->call_cds), ret))
            {
                EXFAIL_OUT(ret);
            }
        }
    }
out:
    
    return ret;

}

/**
 * Unlock call descriptor
 * @param cd
 * @return
 */
exprivate void unlock_call_descriptor(int cd, short status)
{
    /* ATMI_TLS_ENTRY; - already done by caller */
    
    if (!(G_atmi_tls->G_call_state[cd].flags & TPNOTRAN) && 
            G_atmi_tls->G_atmi_xa_curtx.txinfo)
    {
        NDRX_LOG(log_debug, "Un-registering cd=%d from global "
                "transaction!", cd);
        
        atmi_xa_cd_unreg(&(G_atmi_tls->G_atmi_xa_curtx.txinfo->call_cds), cd);
    }
    
    G_atmi_tls->G_call_state[cd].status = status;
}

/**
 * Cancel the call descriptor if was expected
 * @param call
 */
expublic void cancel_if_expected(tp_command_call_t *call)
{
    ATMI_TLS_ENTRY;
    
    call_descriptor_state_t *callst  = &G_atmi_tls->G_call_state[call->cd];
    if (CALL_WAITING_FOR_ANS==callst->status &&
            call->timestamp==callst->timestamp &&
            call->callseq==callst->callseq)
    {
        NDRX_LOG(log_debug, "Reply was expected, but probably "
                                        "timeouted - cancelling!");
        unlock_call_descriptor(call->cd, CALL_CANCELED);
    }
    else
    {
        NDRX_LOG(log_debug, "Reply was NOT expected, ignoring!");
    }
}
/**
 * Do asynchronous call.
 * If it is evpost (is_evpost) htne extradata must be loaded with `my_id' from then
 * server. We will parse it out get the target queue message must be put!
 * 
 * @param svc
 * @param data
 * @param len
 * @param flags - should be managed from parent function (is it real async call
 *                  or tpcall wrapper)
 * @return call descriptor
 */
expublic int _tpacall (char *svc, char *data,
                long len, long flags, char *extradata, 
                int dest_node, int ex_flags, TPTRANID *p_tranid)
{
    int ret=EXSUCCEED;
    char buf[ATMI_MSG_MAX_SIZE];
    tp_command_call_t *call=(tp_command_call_t *)buf;
    typed_buffer_descr_t *descr;
    buffer_obj_t *buffer_info;
    char fn[] = "_tpacall";
    long data_len = MAX_CALL_DATA_SIZE;
    char send_q[NDRX_MAX_Q_SIZE+1];
    time_t timestamp;
    int is_bridge;
    int tpcall_cd;
    ATMI_TLS_ENTRY;
    NDRX_LOG(log_debug, "%s enter", fn);

    
    if (G_atmi_tls->G_atmi_xa_curtx.txinfo)
    {
        atmi_xa_print_knownrms(log_info, "Known RMs before call: ",
                    G_atmi_tls->G_atmi_xa_curtx.txinfo->tmknownrms);
    }
    
     /* Might want to remove in future... but it might be dangerouse!*/
     memset(call, 0, sizeof(tp_command_call_t));
    
    /* Check service availability by SHM? 
     * TODO: We might want to check the flags, the service might be marked for shutdown!
     */
    if (ex_flags & TPCALL_BRCALL)
    {
        /* If this is a bridge call, then format the bridge Q */
#ifdef EX_USE_POLL
        {
            int tmp_is_bridge;
            char tmpsvc[MAXTIDENT+1];
            
            snprintf(tmpsvc, sizeof(tmpsvc), NDRX_SVC_BRIDGE, dest_node);

            if (EXSUCCEED!=ndrx_shm_get_svc(tmpsvc, send_q, &tmp_is_bridge))
            {
                NDRX_LOG(log_error, "Failed to get bridge svc: [%s]", 
                        tmpsvc);
                EXFAIL_OUT(ret);
            }
        }
#else
        snprintf(send_q, sizeof(send_q), NDRX_SVC_QBRDIGE, 
                G_atmi_tls->G_atmi_conf.q_prefix, dest_node);
#endif
        is_bridge=EXTRUE;
    }
    else if (ex_flags & TPCALL_EVPOST)
    {
        if (EXSUCCEED!=_get_evpost_sendq(send_q, sizeof(send_q), extradata))
        {
            NDRX_LOG(log_error, "%s: Cannot get send Q for server: [%s]", 
                    fn, extradata);
            _TPset_error_fmt(TPENOENT, "%s: Cannot get send Q for server: [%s]", 
                    fn, extradata);
            EXFAIL_OUT(ret);
        }
    }
    else if (EXSUCCEED!=ndrx_shm_get_svc(svc, send_q, &is_bridge))
    {
        NDRX_LOG(log_error, "Service is not available %s by shm", 
                svc);
        ret=EXFAIL;
        _TPset_error_fmt(TPENOENT, "%s: Service is not available %s by shm", 
                fn, svc);
        goto out;
    }

    if (NULL!=data)
    {
        if (NULL==(buffer_info = ndrx_find_buffer(data)))
        {
            _TPset_error_fmt(TPEINVAL, "Buffer %p not known to system!", fn);
            EXFAIL_OUT(ret);
        }
    }

    if (NULL!=data)
    {
        descr = &G_buf_descr[buffer_info->type_id];
        /* prepare buffer for call */
        if (EXSUCCEED!=descr->pf_prepare_outgoing(descr, data, len, call->data, 
                &data_len, flags))
        {
            /* not good - error should be already set */
            EXFAIL_OUT(ret);
        }
    }
    else
    {
        data_len=0;
    }

    /* OK, now fill up the details */
    call->data_len = data_len;
    
    data_len+=sizeof(tp_command_call_t);

    if (NULL==data)
        call->buffer_type_id = BUF_TYPE_NULL;
    else
        call->buffer_type_id = buffer_info->type_id;

    NDRX_STRCPY_SAFE(call->reply_to, G_atmi_tls->G_atmi_conf.reply_q_str);
    if (!(ex_flags & TPCALL_EVPOST))
    {
        call->command_id = ATMI_COMMAND_TPCALL;
    }
    else
    {
        call->command_id = ATMI_COMMAND_EVPOST;
    }
    
    strncpy(call->name, svc, XATMI_SERVICE_NAME_LENGTH);
    call->name[XATMI_SERVICE_NAME_LENGTH] = EXEOS;
    call->flags = flags;
    
    if (NULL!=extradata)
    {
        NDRX_STRCPY_SAFE(call->extradata, extradata);
    }

    timestamp = time(NULL);
    
    /* Add global transaction info to call (if needed & tx available) */
    if (!(call->flags & TPNOTRAN) && G_atmi_tls->G_atmi_xa_curtx.txinfo)
    {
        NDRX_LOG(log_info, "Current process in global transaction (%s) - "
                "prepare call", G_atmi_tls->G_atmi_xa_curtx.txinfo->tmxid);
        
        atmi_xa_cpy_xai_to_call(call, G_atmi_tls->G_atmi_xa_curtx.txinfo);
        
        if (call->flags & TPTRANSUSPEND && NULL!=p_tranid &&
                EXSUCCEED!=_tpsuspend(p_tranid, 0, EXFALSE))
        {
            EXFAIL_OUT(ret);
        }
    }
    
    /* lock call descriptor */
    if (!(flags & TPNOREPLY))
    {
        /* get the call descriptor */
        if (EXFAIL==(tpcall_cd = get_call_descriptor_and_lock(&call->callseq, 
                timestamp, flags)))
        {
            NDRX_LOG(log_error, "Do not have resources for "
                                "track this call!");
            _TPset_error_fmt(TPELIMIT, "%s:All call descriptor entries have been used "
                                "(check why they do not free up? Maybe need to "
                                "use tpcancel()?)", fn);
            EXFAIL_OUT(ret);
        }
    }
    else
    {
        NDRX_LOG(log_warn, "TPNOREPLY => cd=0");
        tpcall_cd = 0;
    }
    
    call->cd = tpcall_cd;
    call->timestamp = timestamp;
    
    /* Reset call timer */
    ndrx_stopwatch_reset(&call->timer);
    
    NDRX_STRCPY_SAFE(call->my_id, G_atmi_tls->G_atmi_conf.my_id); /* Setup my_id */
    NDRX_LOG(log_debug, "Sending request to: [%s] my_id=[%s] reply_to=[%s] cd=%d callseq=%u", 
            send_q, call->my_id, call->reply_to, tpcall_cd, call->callseq);
    
    NDRX_DUMP(log_dump, "Sending away...", (char *)call, data_len);

    if (EXSUCCEED!=(ret=ndrx_generic_q_send(send_q, (char *)call, data_len, flags, 0)))
    {
        int err;

        if (ENOENT==ret)
        {
            err=TPENOENT;
        }
        else
        {
            CONV_ERROR_CODE(ret, err);
        }
        _TPset_error_fmt(err, "%s: Failed to send, os err: %s", fn, strerror(ret));
        ret=EXFAIL;

        /* unlock call descriptor */
        unlock_call_descriptor(tpcall_cd, CALL_NOT_ISSUED);
        
        goto out;

    }
    /* return call descriptor */
    ret=tpcall_cd;

out:
    NDRX_LOG(log_debug, "%s return %d", fn, ret);
    return ret;
}

/**
 * Internal version of tpgetrply.
 * TODO: How about TPGETANY|TPNOCHANGE?
 * @param cd
 * @param data
 * @param len
 * @param flags
 * @return
 */
expublic int _tpgetrply (int *cd,
                       int cd_exp,
                       char * *data ,
                       long *len, long flags,
                       TPTRANID *p_tranid)
{
    int ret=EXSUCCEED;
    char fn[] = "_tpgetrply";
    long rply_len;
    unsigned prio;
    /*char rply_buf[ATMI_MSG_MAX_SIZE];*/
    char *pbuf = NULL;
    int pbuf_len;
    tp_command_call_t *rply;
    typed_buffer_descr_t *call_type;
    int answ_ok = EXFALSE;
    int is_abort_only = EXFALSE; /* Should we abort global tx (if open) */
    ATMI_TLS_ENTRY;
    
    /* Allocate the buffer, dynamically... */
    NDRX_SYSBUF_MALLOC_WERR_OUT(pbuf, &pbuf_len, ret);

    NDRX_LOG(log_debug, "%s enter, flags %ld pbuf %p", fn, flags, pbuf);
        
    /* TODO: If we keep linked list with call descriptors and if there is
     * none, then we should return something back - FAIL/proto, not? */
    /**
     * We will drop any answers not registered for this call
     */
    rply  = (tp_command_call_t *)pbuf;
    while (!answ_ok)
    {
        /* We shall check that we do not have something in memq...
         * if so then switch the buffers and make current free
         */
        if (NULL!=G_atmi_tls->memq)
        {
            tpmemq_t * tmp;
            NDRX_LOG(log_info, "Got message from memq...");
            /* grab the buffer of mem linked list */
            NDRX_FREE(pbuf);
            
            /* the buffer is allocated already by sysalloc, thus
             * continue to use this buffer and free up our working buf.
             */
            pbuf = G_atmi_tls->memq->buf;
            pbuf_len = G_atmi_tls->memq->len;
            rply_len = G_atmi_tls->memq->data_len;
            
            /* delete first elem in the list */
            tmp = G_atmi_tls->memq;
            DL_DELETE(G_atmi_tls->memq, tmp);
            NDRX_FREE(tmp);
            
            /* Switch to received buffer... */
            rply  = (tp_command_call_t *)pbuf;
            NDRX_LOG(log_debug, "from memq: pbuf=%p", pbuf);
        }
        else
        {
            NDRX_LOG(log_info, "Waiting on OS Q...");
            
            /* receive the reply back */
            rply_len = ndrx_generic_q_receive(G_atmi_tls->G_atmi_conf.reply_q, 
                    G_atmi_tls->G_atmi_conf.reply_q_str,
                    &(G_atmi_tls->G_atmi_conf.reply_q_attr),
                    pbuf, pbuf_len, &prio, flags);
        }
        
        /* In case  if we did receive any response (in non blocked mode
         * or we did get fail in blocked mode with TPETIME, then we should
         * look up the table for which really we did get the time-out.
         */
        if ((flags & TPNOBLOCK && GEN_QUEUE_ERR_NO_DATA==rply_len) || 
                (EXFAIL==rply_len && TPETIME==tperrno))
        {
            if (flags & TPGETANY)
            {
                if (EXSUCCEED!=(ret = call_scan_tout(EXFAIL, cd)))
                {
                    goto out;
                }
            }
            else
            {
                if (EXSUCCEED!=(ret = call_scan_tout(cd_exp, cd)))
                {
                    goto out;
                }
            }
        }

        if (GEN_QUEUE_ERR_NO_DATA==rply_len)
        {
            /* there is no data in reply, nothing to do & nothing to return */
            *cd = 0;
            goto out;
        }
        else if (EXFAIL==rply_len)
        {
            /* we have failed */
            NDRX_LOG(log_debug, "%s failed to receive answer", fn);
            ret=EXFAIL;
            goto out;
        }
        else
        {
            if (ATMI_COMMAND_TPNOTIFY==rply->command_id ||
                    ATMI_COMMAND_BROADCAST==rply->command_id)
            {
                NDRX_LOG(log_debug, "%s message received -> _tpnotify", 
                        (ATMI_COMMAND_TPNOTIFY==rply->command_id?"Notification":"Broadcast"));
                /* process the notif... */
                ndrx_process_notif(pbuf, rply_len);
                
                /* And continue... */
                continue;
            }
            NDRX_LOG(log_debug, "accept any: %s, cd=%d atmi_tls=%p cmd=%hd rplybuf=%p (pbuf=%p)",
			(flags & TPGETANY)?"yes":"no", rply->cd, G_atmi_tls, rply->command_id, rply, pbuf);

/*
            NDRX_DUMP(log_dump, "Received...", (char *)rply, rply_len);
*/

            /* if answer is not expected, then we receive again! */
            if (CALL_WAITING_FOR_ANS==G_atmi_tls->G_call_state[rply->cd].status &&
                    G_atmi_tls->G_call_state[rply->cd].timestamp == rply->timestamp &&
                    G_atmi_tls->G_call_state[rply->cd].callseq == rply->callseq)
            {
                /* Drop if we do not expect it!!! */
		/* 01/11/2012 - if we have TPGETANY we ignore the cd */
                if (/*cd_exp!=FAIL*/ !(flags & TPGETANY) && rply->cd!=cd_exp)
                {
                    NDRX_LOG(log_warn, "Dropping incoming message (not expected): "
                        "cd: %d, expected cd: %d timestamp: %d callseq: %u, "
                            "reply from %s, cd status %hd",
                        rply->cd, cd_exp, rply->timestamp, rply->callseq, rply->reply_to,
                            G_atmi_tls->G_call_state[rply->cd].status);
                    _tpcancel(rply->cd);
                    continue;
                }

                NDRX_LOG(log_warn, "Reply cd: %d, timestamp :%d callseq: %u from %s - expected OK!",
                        rply->cd, rply->timestamp, rply->callseq, rply->reply_to);
                answ_ok=EXTRUE;
                /* Free up call descriptor!! */
                unlock_call_descriptor(rply->cd, CALL_NOT_ISSUED);
            }
            else
            {
                NDRX_LOG(log_warn, "Dropping incoming message (not expected): "
                        "cd: %d, timestamp :%d, callseq: %u reply from %s cd status %hd",
                        rply->cd, rply->timestamp, rply->callseq, rply->reply_to,
                        G_atmi_tls->G_call_state[rply->cd].status);
                
                continue; /* Wait for next message! */
            }

            /* If we are in global tx & we have abort_only, then report */
            if (TMTXFLAGS_IS_ABORT_ONLY & rply->tmtxflags)
            {
                NDRX_LOG(log_warn, "Reply contains SYS_XA_ABORT_ONLY!");
                is_abort_only = EXTRUE;
            }
            /* TODO: check incoming type! */
            /* we have an answer - prepare buffer */
            *cd = rply->cd;
            if (rply->sysflags & SYS_FLAG_REPLY_ERROR)
            {
                _TPset_error_msg(rply->rcode, "Server failed to generate reply");
                ret=EXFAIL;
                goto out;
            }
            else
            {
                /* Convert only if we have data */
                if (rply->data_len > 0)
                {
                    call_type = &G_buf_descr[rply->buffer_type_id];

                    ret=call_type->pf_prepare_incoming(call_type,
                                    rply->data,
                                    rply->data_len,
                                    data,
                                    len,
                                    flags);
                }
                /* put rcode in global */
                G_atmi_tls->M_svc_return_code = rply->rcode;

                /* TODO: Check buffer acceptance or do it inside of prepare_incoming? */
                if (ret==EXFAIL)
                {
                    goto out;
                }
                /* if all OK, then finally check the reply answer
                 * Maybe want to use code returned in rval?
                 */
                if (TPSUCCESS!=rply->rval)
                {
                    _TPset_error_fmt(TPESVCFAIL, "Service returned %d", rply->rval);
                    ret=EXFAIL;
                    goto out;
                }
            }
        } /* reply recieved ok */
    }
out:

    /* Restore transaction if was suspended. */
    if (flags & TPTRANSUSPEND && p_tranid && p_tranid->tmxid[0])
    {
        /* resume the transaction */
        if (EXSUCCEED!=_tpresume(p_tranid, 0) && EXSUCCEED==ret)
        {
            ret=EXFAIL;
        }
    }

    if (G_atmi_tls->G_atmi_xa_curtx.txinfo && 
            0==strcmp(G_atmi_tls->G_atmi_xa_curtx.txinfo->tmxid, rply->tmxid) &&
            EXSUCCEED!=atmi_xa_update_known_rms(G_atmi_tls->G_atmi_xa_curtx.txinfo->tmknownrms, 
            rply->tmknownrms))
    {
        EXFAIL_OUT(ret);
    }

    if ( !(flags & TPNOTRAN) &&  /* Do not abort, if TPNOTRAN specified. */
	G_atmi_tls->G_atmi_xa_curtx.txinfo &&
	(EXSUCCEED!=ret || is_abort_only))
    {
        NDRX_LOG(log_warn, "Marking current transaction as abort only!");
        
        /* later should be handled by transaction initiator! */
        G_atmi_tls->G_atmi_xa_curtx.txinfo->tmtxflags |= TMTXFLAGS_IS_ABORT_ONLY;
    }

    /* free up the system buffer */
    if (NULL!=pbuf)
    {
        NDRX_FREE(pbuf);
    }
                
    NDRX_LOG(log_debug, "%s return %d", fn, ret);
    /* mvitolin 12/12/2015 - according to spec we must return 
     * service returned return code
     */
    if (EXSUCCEED==ret)
    {
        return G_atmi_tls->M_svc_return_code;
    }
    else 
    {
        return ret;
    }
}

/**
 * Internal version to tpcall.
 * Support for flags:
 * TPNOTRAN    TPNOBLOCK
 * TPNOTIME    TPSIGRSTRT
 *
 * @param svc
 * @param idata
 * @param ilen
 * @param odata
 * @param olen
 * @param flags
 * @return
 */
expublic int _tpcall (char *svc, char *idata, long ilen,
                char * *odata, long *olen, long flags,
                char *extradata, int dest_node, int ex_flags)
{
    int ret=EXSUCCEED;
    char fn[] = "_tpcall";
    int cd_req = 0;
    int cd_rply = 0;
    TPTRANID tranid, *p_tranid;
    
    NDRX_LOG(log_debug, "%s: enter", fn);

    flags&=~TPNOBLOCK; /* we are working in sync (blocked) mode
                        * because we do want anser back! */
    
    if (flags & TPTRANSUSPEND)
    {
        memset(&tranid, 0, sizeof(tranid));
        p_tranid = &tranid;
    }
    else
    {
        p_tranid = NULL;
    }
    
    if (EXFAIL==(cd_req=_tpacall (svc, idata, ilen, flags, extradata, 
            dest_node, ex_flags, p_tranid)))
    {
        NDRX_LOG(log_error, "_tpacall to %s failed", svc);
        ret=EXFAIL;
        goto out;
    }
    else if (EXSUCCEED!=(ret=_tpgetrply(&cd_rply, cd_req, odata, olen, flags, 
            p_tranid)))
    {
        NDRX_LOG(log_error, "_tpgetrply to %s failed", svc);
        goto out;
    }

    /*
     * Did we get back what we asked for?
     */
    if (cd_req!=cd_rply)
    {
        ret=EXFAIL;
        _TPset_error_fmt(TPEPROTO, "%s: Got invalid reply! cd_req: %d, cd_rply: %d",
                                        fn, cd_req, cd_rply);
        goto out;
    }


out:
    NDRX_LOG(log_debug, "%s: return %d cd %d", fn, ret, cd_rply);

    return ret;
}

/**
 * Internal version to tpcancel
 * @param
 * @return
 */
expublic int _tpcancel (int cd)
{
    int ret=EXSUCCEED;
    char fn[]="_tpcancel";
    ATMI_TLS_ENTRY;
    
    NDRX_LOG(log_debug, "tpcancel issued for %d", cd);

    if (cd<1||cd>=MAX_ASYNC_CALLS)
    {
        _TPset_error_fmt(TPEBADDESC, "%s: Invalid call descriptor %d, should be 0<cd<%d",
                                        fn, cd, MAX_ASYNC_CALLS);
        ret=EXFAIL;
        goto out;
    }
    /* Mark call as cancelled, so that we could re-use it later. */
    G_atmi_tls->G_call_state[cd].status = CALL_CANCELED;

out:
    return ret;
}


/**
 * ATMI standard
 * @return - pointer to int holding error code?
 */
expublic long * _exget_tpurcode_addr (void)
{
    ATMI_TLS_ENTRY;
    return &G_atmi_tls->M_svc_return_code;
}

/**
 * Get the queue name where we shall post the event!
 * @param send_q
 * @param extradata
 * @return 
 */
expublic int _get_evpost_sendq(char *send_q, size_t send_q_bufsz, char *extradata)
{
    int ret=EXSUCCEED;
    char fn[] = "get_evpost_sendq";
    TPMYID myid;
    ATMI_TLS_ENTRY;
    if (NULL==extradata || EXEOS==extradata[0] || NULL==send_q)
    {
        NDRX_LOG(log_error, "Invalid arguments");
        ret=EXFAIL;
        goto out;
    }
    
    NDRX_LOG(log_debug, "%s: server's id=[%s]", fn, extradata);
    
    if (EXSUCCEED!=ndrx_myid_parse(extradata, &myid, EXFALSE))
    {
        NDRX_LOG(log_error, "Failed to parse my_id string [%s]", extradata);
        /* Do fail? */
        goto out;
    }
    
    NDRX_LOG(log_debug, "Parsed: binary=[%s] srvid=%d pid=%d contextid=%ld nodeid=%d",
            myid.binary_name, myid.srv_id, myid.pid, myid.contextid, myid.nodeid);
   
    
    if (G_atmi_env.our_nodeid!=myid.nodeid)
    {
        NDRX_LOG(log_debug, "Server is located on different server, "
                "our nodeid=%d their=%d",
                G_atmi_env.our_nodeid, myid.nodeid);
#ifdef EX_USE_POLL
        /* poll() mode: */
        {
            int is_bridge;
            char tmpsvc[MAXTIDENT+1];

            snprintf(tmpsvc, sizeof(tmpsvc), NDRX_SVC_BRIDGE, myid.nodeid);

            if (EXSUCCEED!=ndrx_shm_get_svc(tmpsvc, send_q, &is_bridge))
            {
                NDRX_LOG(log_error, "Failed to get bridge svc: [%s]", 
                        tmpsvc);
                EXFAIL_OUT(ret);
            }
        }
#else
        snprintf(send_q, send_q_bufsz, NDRX_SVC_QBRDIGE, 
                G_atmi_tls->G_atmi_conf.q_prefix, myid.nodeid);
#endif
    }
    else
    {
        NDRX_LOG(log_debug, "This is local server");
        snprintf(send_q, send_q_bufsz, NDRX_ADMIN_FMT, 
                G_atmi_tls->G_atmi_conf.q_prefix, myid.binary_name, 
                myid.srv_id, myid.pid);
    }
    
out:
                
    NDRX_LOG(log_debug, "%s returns send_q=[%s] ret=%d", 
                    fn, send_q, ret);

    return ret;
}



