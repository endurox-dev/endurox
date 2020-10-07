/**
 * @brief Temporary queue runner
 *
 * @file temq.c
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
#include <poll.h>

#include <ndebug.h>
#include <atmi.h>
#include <atmi_int.h>
#include <typed_buf.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <Exfields.h>
#include <gencall.h>

#include <exnet.h>
#include <ndrxdcmn.h>

#include "bridge.h"
#include "../libatmisrv/srv_int.h"
#include "exsha1.h"
#include <ndrxdiag.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

/**
 * Remove message from hash queue
 * and delete queue hash if there arn't any msgs in
 */
#define RM_MSG(QHASH)  do {\
                    /* locking here needed.. */\
                    MUTEX_LOCK_V(M_in_q_lock);\
                    /* remove from Q - ok */\
                    DL_DELETE(QHASH->msgs, el);\
                    NDRX_FPFREE(el->buffer);\
                    NDRX_FPFREE(el);\
                    M_msgs_in_q--;\
                    QHASH->nrmsg--;\
                    if (0==QHASH->nrmsg) {EXHASH_DEL(M_qstr_hash, QHASH); NDRX_FPFREE(QHASH);}\
                    MUTEX_UNLOCK_V(M_in_q_lock);\
                } while (0);
                
#define DISCARD_CALL_LOG    do {NDRX_LOG(log_error, \
                    "Discarding svc call! call age = %ld s, client timeout = %d "\
                    "cd: %d timestamp: %d (id: %d%d) callseq: %u, "\
                    "svc: %s, flags: %ld, call age: %ld, data_len: %ld, caller: %s "\
                    " reply_to: %s, call_stack: %s",\
                    call_age, call->clttout,\
                    call->cd, call->timestamp, call->cd, call->timestamp, call->callseq, \
                    call->name, call->flags, call_age, call->data_len,\
                    call->my_id, call->reply_to, call->callstack);} while (0)

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

exprivate MUTEX_LOCKDECL(M_in_q_lock);  /**< Queue lock the queue cache       */
exprivate int M_msgs_in_q = 0;          /**< Number of messages enqueued      */
exprivate in_msg_hash_t *M_qstr_hash = NULL; /**< Hash of temp queues         */
exprivate int M_stopped = EXFALSE;      /**< Is incoming stopped              */
MUTEX_LOCKDECL(ndrx_G_global_br_lock);  /**< Global bridge lock               */

/**< Some job is enqueued, check the counts & incoming lockups                */
exprivate pthread_cond_t  M_wakup_queue_runner; 


/*---------------------------Prototypes---------------------------------*/

/**
 * init the temp queue runner.
 */
expublic void br_tempq_init(void)
{
    pthread_cond_init(&M_wakup_queue_runner, NULL);
}

/**
 * Generate error to network if required.
 * We detect the packet type here.
 * @param buf
 * @param len
 * @param pack_type
 * @return 
 */
exprivate int br_generate_error_to_net(char *buf, int len, int pack_type, long rcode)
{
    int ret=EXSUCCEED;
    
    switch(pack_type)
    {
        case PACK_TYPE_TONDRXD:
            /* Will not generate any error here*/
            break;
        case PACK_TYPE_TOSVC:
            /* So This was TPCALL, we might want to send error reply back. */
        {
            tp_command_call_t *call = (tp_command_call_t *)buf;
            
            if (!(call->flags & TPNOREPLY))
            {
                NDRX_LOG(log_warn, "Sending back error reply");
                reply_with_failure(TPNOBLOCK, call, NULL, NULL, rcode);
            }
        }
            break;
        case PACK_TYPE_TORPLYQ:
            /* Will not generate any error here*/
            break;
        default:
            NDRX_LOG(log_warn, "Nothing to do for pack_type=%d", 
                    pack_type);
            break;
    }
    
out:
    return ret;
}

/**
 * So we got q send error.
 * @param destqstr dest queue where message sent attempt was made
 * @param qhash for queue runner hash of each destination
 *  in case of NULL, no enqueue required (failed to resolve service q, nothing todo)
 */
expublic int br_process_error(char *buf, int len, int err, 
        in_msg_t* el, int pack_type, char *destqstr, in_msg_hash_t * qhash)
{
    long rcode = TPESVCERR;
    
    if (err==ENOENT)
    {
        rcode = TPENOENT;
    }
    
    /* So this is initial call */
    if (NULL==el && NULL!=destqstr)
    {
        /* This will be processed by queue runner */
        if (EAGAIN==err || EINTR==err)
        {
            /* Put stuff in queue */
            br_add_to_q(buf, len, pack_type, destqstr);
        }
        else
        {
            /* TODO: Ignore the error or send error back - TPNOENT probably (if this is request) */
            br_generate_error_to_net(buf, len, pack_type, rcode);
        }
    }
    else
    {
        NDRX_LOG(log_debug, "Got error processing from Q");
        
        /* In this case we should handle the stuff!?!?
         * Generate reply (only if needed), Not sure TPNOTENT or svcerr?
         * SVCERR could be better because initially we thought that service exists.
         */
        br_generate_error_to_net(buf, len, pack_type, rcode);
        
        if (PACK_TYPE_TOSVC==pack_type)
        {
            tp_command_call_t *call = (tp_command_call_t *)buf;
            long call_age = ndrx_stopwatch_get_delta_sec(&call->timer);
            /* print some add info */
            
            DISCARD_CALL_LOG;
        }
        
        NDRX_DUMP(log_warn, "Discarding message", buf, len);
        
        userlog("Discarding message: %p/%d dest q: target q: [%s]", buf, len, 
                destqstr?destqstr:"(null)");
        
        /* just in case if it is first attempt and service does not exists in shm... 
         * discard immediately and thus el does not exist
         */
        if (NULL!=el && NULL!=qhash)
        {
            RM_MSG(qhash);
        }
    }
    
    return EXSUCCEED;
}

/**
 * Check the global processing limit
 * @return EXSUCCEED (ok), EXFAIL (lock time limit)
 */
expublic int br_chk_limit(void)
{
#ifdef EX_OS_DARWIN
    
    MUTEX_LOCK_V(ndrx_G_global_br_lock);
    MUTEX_UNLOCK_V(ndrx_G_global_br_lock);
    return EXSUCCEED;
    
#else
    int ret;
    
    struct timespec wait_time;
    struct timeval now;

    gettimeofday(&now,NULL);

    wait_time.tv_sec = now.tv_sec+1;
    wait_time.tv_nsec = now.tv_usec;

    ret=pthread_mutex_timedlock(&ndrx_G_global_br_lock, &wait_time);
    
    if (EXSUCCEED==ret)
    {
        MUTEX_UNLOCK_V(ndrx_G_global_br_lock);
    }
    else
    {
        NDRX_LOG(log_error, "Global lock timed out: %s", strerror(ret));
    }
    return ret;
#endif
}

/**
 * Run queue from thread.
 * This is started from main thread periodic runner.
 * The special flag is used to indicate if run job was in queue
 * If incoming limit of queue reached, we shall lock the mutex and do not
 * allow any incoming traffic.
 * 
 * @param ptr not used
 * @param p_finish_off not used
 * @return EXSUCCEED;
 */
exprivate int br_run_q_th(void *ptr, int *p_finish_off)
{
    int ret = EXSUCCEED;
    in_msg_t *el, *elt;
    long call_age;
    long time_in_q, spent_from_last_upd;
    long sleep_time;
    in_msg_hash_t *qhash, *qhashtmp;
    int msg_deleted;
    int cur_was_ok;

    /* wait for conditional... so that we get quick wakeups
    * in case if sleeping for long and some msgs is being added...
    */
    int cret;
    struct timespec wait_time;
    struct timeval now;
    
#define NEVER_SLEEP (G_bridge_cfg.qmaxsleep+1)
    /**
     * Possible dead lock if service puts back in queue/ 
     * do the unlock in the middle to allow adding msg?
     * 
     * If delete is allowed only from this thread, then we should sync only
     * on adding..
     * 
     */
    
    /* Master loop of queues... */
    
    /* loop runs in locked mode.. */
    sleep_time=NEVER_SLEEP;
        
    NDRX_LOG(log_debug, "br_run_q_th enter");
    
    MUTEX_LOCK_V(M_in_q_lock);
    EXHASH_ITER(hh, M_qstr_hash, qhash, qhashtmp)
    {
        NDRX_LOG(log_debug, "Checking queue: [%s]", qhash->qstr);
        cur_was_ok=EXFALSE;
        /* process until we get the error... */
        DL_FOREACH_SAFE(qhash->msgs, el, elt)
        {
            MUTEX_UNLOCK_V(M_in_q_lock);
            
            /* check is the queue turn right now? */
            spent_from_last_upd = ndrx_stopwatch_get_delta(&(el->updatetime));
            
            /* if time spent is lesser then scheduled next run, then try next 
             * anyway calc new sleep time..
             * Run only if this is first msg... if was attempt OK 
             */
            if (spent_from_last_upd<el->next_try_ms && !cur_was_ok)
            {
                long time_left = el->next_try_ms-spent_from_last_upd;
                
                NDRX_LOG(log_debug, "Time left for %s is: %ld", 
                        el->destqstr, time_left);
                        
                if (time_left < sleep_time)
                {
                    sleep_time = time_left;
                }
                
                /* WARNING ! break to next queue... */
                MUTEX_LOCK_V(M_in_q_lock);
                break;
            }

            msg_deleted=EXFALSE;
            time_in_q = ndrx_stopwatch_get_delta(&(el->addedtime));
            el->tries++;
            NDRX_LOG(log_warn, "Processing late delivery of %p/%d [%s] "
                    "try %d/%d nrmsg: %d time_in_q %ld ms (ttl: %d) next_try_ms %ld ms", 
                    el->buffer, el->len, el->destqstr, el->tries, 
                    G_bridge_cfg.qretries, qhash->nrmsg, time_in_q, G_bridge_cfg.qttl,
                    el->next_try_ms);

            /* if ttl is used, then check time-out too */
            if (el->tries <= G_bridge_cfg.qretries && time_in_q<=G_bridge_cfg.qttl)
            {
                if (EXSUCCEED!=(ret=ndrx_generic_q_send(el->destqstr, (char *)el->buffer, 
                        el->len, TPNOBLOCK, 0)))
                {

                    NDRX_LOG(log_error, "Failed to send message to [%s]: %s",
                            el->destqstr, tpstrerror(ret));

                    /* analyze the error - if queue is missing
                     * then drop the message -> set the max tries...
                     * or do the retry if queue is full, otherwise we drop the msg.
                     */

                    if (EAGAIN!=ret && EINTR!=ret)
                    {
                       NDRX_LOG(log_error, "Dest queue is broken");

                       br_process_error((char *)el->buffer, el->len, EXFAIL, 
                                el, el->pack_type, el->destqstr, qhash);
                       msg_deleted=EXTRUE;
                    }
                    /* if it is svc reply, check the timeout flag too.. */
                    else if (PACK_TYPE_TOSVC==el->pack_type)
                    {
                        tp_command_call_t *call = (tp_command_call_t *)el->buffer;
                        call_age = ndrx_stopwatch_get_delta_sec(&call->timer);
                        /**
                         * If possible check the expiry of the call,
                         * if so, drop the msg
                         */
                        if ((ATMI_COMMAND_TPCALL==call->command_id || 
                            ATMI_COMMAND_CONNECT==call->command_id) &&
                                call->clttout > 0 && call_age >= call->clttout && 
                                !(call->flags & TPNOTIME))
                        {
                            /* no need to reply? */
                            NDRX_LOG(log_error, "Message expired - remove / no reply");
                            DISCARD_CALL_LOG;
                            RM_MSG(qhash);
                            msg_deleted=EXTRUE;
                        }
                    }

                }
                else
                {
                    /* sent ok */
                    RM_MSG(qhash);
                    msg_deleted=EXTRUE;
                }
            }
            else
            {
                br_process_error((char *)el->buffer, 
                        el->len, EXFAIL, el, el->pack_type, el->destqstr, qhash);
                msg_deleted=EXTRUE;
            }

            /* if last msg, was not removed, then schedule new run 
             * and try next queue...
             */
            
            if (!msg_deleted)
            {
                /* multiple sleep time by 2 */
                el->next_try_ms*=2;
                
                if (el->next_try_ms>G_bridge_cfg.qmaxsleep)
                {
                    el->next_try_ms=G_bridge_cfg.qmaxsleep;
                }
                else if (el->next_try_ms<G_bridge_cfg.qminsleep)
                {
                    el->next_try_ms=G_bridge_cfg.qminsleep;
                }
                
                if (el->next_try_ms < sleep_time)
                {
                    sleep_time = el->next_try_ms;
                }
                
                /* set the life cycle */
                ndrx_stopwatch_reset(&el->updatetime);
                MUTEX_LOCK_V(M_in_q_lock);
                
                /* process next queue... */
                break;
            }
            else
            {
                cur_was_ok=EXTRUE;
            }
            
            MUTEX_LOCK_V(M_in_q_lock);
        }
       
        /* here we are locked... */
    }
    
    
    /* Stop the incoming traffic if total queue is full: */
    if (M_msgs_in_q > G_bridge_cfg.qsize && !M_stopped)
    {
        NDRX_LOG(log_error, "Max number of msgs queued in bridge: %d "
                "currently: %d - stop online traffic", 
                G_bridge_cfg.qsize, M_msgs_in_q);

        /* set global lock */
        MUTEX_LOCK_V(ndrx_G_global_br_lock);
        M_stopped=EXTRUE;

    }
    else if (M_msgs_in_q < G_bridge_cfg.qsize && M_stopped)
    {
        NDRX_LOG(log_error, "Max number of msgs queued in bridge: %d "
                "currently: %d - resume online traffic", 
                G_bridge_cfg.qsize, M_msgs_in_q);

        /* unset global lock */
        MUTEX_UNLOCK_V(ndrx_G_global_br_lock);
        M_stopped=EXFALSE;
    }
    
    MUTEX_UNLOCK_V(M_in_q_lock);
    
    /* Do some sleep if required */
    if (M_msgs_in_q > 0)
    {
        if (sleep_time>0)
        {

            /* while the M_in_q_lock was unlocked (loop) finished, somene has added msg
             * thus use min sleep
             */
            if (sleep_time > G_bridge_cfg.qmaxsleep)
            {
                sleep_time=G_bridge_cfg.qminsleep;
            }

            NDRX_LOG(log_info, "Sleep time: %ld ms M_msgs_in_q: %d", 
                    sleep_time, M_msgs_in_q);

            /* wouldn't it be better to wait for conditional?
             * so that if new msg is enqueued, checks can be performed?
             */
            MUTEX_LOCK_V(M_in_q_lock);

            gettimeofday(&now, NULL);

            wait_time.tv_sec = now.tv_sec;
            /* convert to ms: */
            wait_time.tv_nsec = now.tv_usec*1000;

            ndrx_timespec_plus(&wait_time, sleep_time);

            /* sleep or wait event.. */
            cret=pthread_cond_timedwait(&M_wakup_queue_runner, &M_in_q_lock, &wait_time);

            NDRX_LOG(log_debug, "pthread_cond_timedwait returns %d: %s", 
                    cret, strerror(cret));
            
            MUTEX_UNLOCK_V(M_in_q_lock);
        }

        /* run loop again */
        if (EXSUCCEED!=ndrx_thpool_add_work2(G_bridge_cfg.thpool_queue, (void *)br_run_q_th, 
                NULL, NDRX_THPOOL_ONEJOB, 0))
        {
            NDRX_LOG(log_debug, "Already run queued...");
        }
    }
    
out:
    NDRX_LOG(log_info, "Current queue stats: M_msgs_in_q=%d", M_msgs_in_q);
    return ret;
}

/**
 * Alloc or find the queue
 * @param qstr queue name
 * @return NULL in case of failure
 */
exprivate in_msg_hash_t* get_qstr_hash(char *qstr)
{
    in_msg_hash_t *ret;
    int err;
    
    EXHASH_FIND_STR(M_qstr_hash, qstr, ret);
    
    if (NULL==ret)
    {
        /* alloc new... */        
        if (NULL==(ret=NDRX_FPMALLOC(sizeof(in_msg_hash_t), 0)))
        {
            err = errno;
            NDRX_LOG(log_error, "Failed to malloc %d bytes: %s", 
                    sizeof(in_msg_hash_t), strerror(err));
            userlog("Failed to malloc %d bytes: %s", sizeof(in_msg_hash_t),
                    strerror(err));
            goto out;
        }
        
        ret->nrmsg=0;
        ret->msgs=NULL;
        NDRX_STRCPY_SAFE(ret->qstr, qstr);
        EXHASH_ADD_STR( M_qstr_hash, qstr, ret );
        
        NDRX_LOG(log_error, "New temporary queue [%s]", qstr);
    }
out:
    return ret;    
}

/**
 * Enqueue the message for delayed send.
 * @return 
 */
expublic int br_add_to_q(char *buf, int len, int pack_type, char *destq)
{
    int ret=EXSUCCEED;
    in_msg_t *msg;
    in_msg_hash_t *qhash;
    int dropmsg = EXFALSE;
    if (NULL==(msg=NDRX_FPMALLOC(sizeof(in_msg_t), 0)))
    {
        NDRX_ERR_MALLOC(sizeof(in_msg_t));
        EXFAIL_OUT(ret);
    }
    
    if (NULL==(msg->buffer=NDRX_FPMALLOC(len, 0)))
    {
        NDRX_ERR_MALLOC(len);
        EXFAIL_OUT(ret);
    }
    
    /*fill in the details*/
    msg->pack_type = pack_type;
    msg->len = len;
    msg->tries=1;
    /* have minimum sleep... */
    msg->next_try_ms=G_bridge_cfg.qminsleep;
    NDRX_STRCPY_SAFE(msg->destqstr, destq);
    memcpy(msg->buffer, buf, len);
    
    ndrx_stopwatch_reset(&msg->addedtime);
    ndrx_stopwatch_reset(&msg->updatetime);
    
    /* Bug #465 moved  after the adding to Q */
    NDRX_LOG(log_warn, "About to add %p/%d [%s] to in-mem queue "
            "for late delivery...", msg->buffer, msg->len, msg->destqstr);
    
    /* search for Q def */
    MUTEX_LOCK_V(M_in_q_lock);
    
    /* we added msgs... */
    if (NULL==(qhash = get_qstr_hash(destq)))
    {
        MUTEX_UNLOCK_V(M_in_q_lock);
        EXFAIL_OUT(ret);
    }
    
    if (G_bridge_cfg.qfullaction == QUEUE_ACTION_DROP && M_msgs_in_q+1 > G_bridge_cfg.qsize)
    {
        NDRX_LOG(log_error, "Temporary queue full (max: %d, new size: %d) "
                "and action is to drop",
                G_bridge_cfg.qsize, M_msgs_in_q+1);
        dropmsg=EXTRUE;
    }
    else if (G_bridge_cfg.qfullactionsvc == QUEUE_ACTION_DROP && qhash->nrmsg+1 > G_bridge_cfg.qsizesvc)
    {
        NDRX_LOG(log_error, "Temporary service queue is full (max: %d, new size: %d) "
                "and action is to drop",
                G_bridge_cfg.qsizesvc, qhash->nrmsg+1);
        dropmsg=EXTRUE;
    }
    else
    {
        M_msgs_in_q++;
        qhash->nrmsg++;

        DL_APPEND(qhash->msgs, msg);
        
        /* wake up if current queue has just added...
         * otherwise the time for hash run is already scheduled
         */
        if (qhash->nrmsg==1)
        {
            /* notify that new message has arrived... */
            pthread_cond_signal(&M_wakup_queue_runner);
        }
    }
    
    MUTEX_UNLOCK_V(M_in_q_lock);
    
    if (dropmsg)
    {
        br_process_error(buf, len, EXFAIL, 
                                msg, pack_type, destq, NULL);
        NDRX_FPFREE(msg->buffer);
        NDRX_FPFREE(msg);
    }
    else
    {
        /* issue the queue runner job */
        ndrx_thpool_add_work2(G_bridge_cfg.thpool_queue, (void*)br_run_q_th, 
                NULL, NDRX_THPOOL_ONEJOB, 0);
    }
    
out:

    /* if not checking for ret, the queue sender may be already processed the msg... */
    if (EXSUCCEED!=ret)
    {
        if (NULL!=msg->buffer)
        {
            NDRX_FPFREE(msg->buffer);
        }
        
        if (NULL!=msg)
        {
            NDRX_FPFREE(msg);
        }
    }

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
