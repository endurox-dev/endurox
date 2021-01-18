/**
 * @brief Clock related routines.
 *
 * @file clock.c
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

#include <ndebug.h>
#include <atmi.h>
#include <atmi_int.h>
#include <typed_buf.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <Exfields.h>

#include <exnet.h>
#include <ndrxdcmn.h>

#include "bridge.h"
#include "../libatmisrv/srv_int.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define CLOCK_DEBUG     1
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

MUTEX_LOCKDECL(M_timediff_lock);
exprivate ndrx_stopwatch_t M_timediff_sent; /**< for roundtrip calc                     */
exprivate time_t M_timediff_tstamp;      /**< UTC tstamp when msg was sent  for matching*/
exprivate unsigned long M_seq;         /**< Last sequence sent                          */

/**
 * Connection infos
 * bridge internals
 * @param call message from queue
 * @return EXSUCCEED/EXFAIL
 */
expublic int br_coninfo(command_call_t *call)
{
    int ret=EXSUCCEED;
    long long diff;
    command_reply_brconinfo_t infos; /* currently one ... */
    ndrx_stopwatch_t our_time;
    
    memset(&infos, 0, sizeof(infos)); /* not mission critical, so can set mem */
    
    
    ndrx_stopwatch_reset(&our_time);
    /* form up the reply */
    infos.rply.magic = NDRX_MAGIC;
    infos.rply.command = call->command+1; /* Make reponse */
    NDRX_LOG(log_debug, "Reply command: %d", infos.rply.command);
    infos.rply.msg_type = NDRXD_CALL_TYPE_BRCONINFO;
    infos.rply.msg_src = NDRXD_SRC_BRIDGE; /* from NDRXD */
    
    /* Response flags, echo back request flags too... */
    infos.rply.flags = call->flags;
    infos.rply.error_code = 0;
    
    infos.time = our_time.t.tv_sec;
    infos.timems = (long)(our_time.t.tv_nsec / 1000000);
    
    /* have some consistency */
    MUTEX_LOCK_V(M_timediff_lock);
    
    infos.lastsync = ndrx_stopwatch_get_delta_sec(&G_bridge_cfg.timediff_ourt);
    infos.locnodeid = tpgetnodeid();
    infos.remnodeid = G_bridge_cfg.nodeid;
    infos.srvid = tpgetsrvid();
    
    if (G_bridge_cfg.is_server)
    {
        infos.mode = NDRX_CONMODE_PASSIVE;
    }
    else
    {
        infos.mode = NDRX_CONMODE_ACTIVE;
    }
    
    if (NULL!=G_bridge_cfg.con)
    {
        infos.fd = G_bridge_cfg.con->sock;
    }
    else
    {
        infos.fd=EXFAIL;
    }

    /* read in fast way */
    NDRX_SPIN_LOCK_V(G_bridge_cfg.timediff_lock);
    diff = G_bridge_cfg.timediff;
    NDRX_SPIN_UNLOCK_V(G_bridge_cfg.timediff_lock);
        
    /* convert to seconds*/
    infos.timediffs = (long)(diff/1000);
    infos.timediffms = (long)(diff%1000);
    infos.roundtrip = G_bridge_cfg.timediff_roundtrip;
    
    MUTEX_UNLOCK_V(M_timediff_lock);
    
    ret = ndrx_generic_q_send_2(call->reply_queue, 
            (char *)&infos, sizeof(infos), 0, BR_ADMININFO_TOUT, 0);
    
out:
    return ret;
}
/**
 * Initialize clock diff.
 * @param call
 * @return 
 */
expublic int br_calc_clock_diff(command_call_t *call)
{
    int ret=EXSUCCEED;
    ndrx_stopwatch_t our_time;
    cmd_br_time_sync_t *their_time = (cmd_br_time_sync_t *)call;
    long long diff=EXFAIL;
    long rountrip=0;
    int load_time=EXFALSE;
    
    /* if got request, just send reply */
    if (NDRX_BRCLOCK_MODE_REQ==their_time->mode)
    {
        /* just send data back */
        NDRX_LOG(log_debug, "Reply with timestamp");
        return br_send_clock(NDRX_BRCLOCK_MODE_RSP, their_time);
    }
    
    /* update the timestamps... stored locally -> the last sync 
     * once we move to multi-connections, all clock data needs to be associated
     * with connections.s
     */
    MUTEX_LOCK_V(M_timediff_lock);
        
    /* So we have their time let timer lib, to get diff */
    ndrx_stopwatch_reset(&our_time);
    
    if (NDRX_BRCLOCK_MODE_ASYNC==their_time->mode)
    {
        load_time=EXTRUE;
    }
    else if (NDRX_BRCLOCK_MODE_RSP==their_time->mode)
    {

        /* check the vars... */
       
        rountrip = (long)ndrx_stopwatch_diff(&our_time, &M_timediff_sent);
        
        if (their_time->orig_seq==M_seq
                && rountrip <= G_bridge_cfg.max_roundtrip
                && their_time->orig_nodeid == tpgetnodeid()
                && their_time->orig_timestamp == M_timediff_tstamp
                )
        {
            load_time=EXTRUE;
        }
        else
        {
            NDRX_LOG(log_error, "DROP time sync echo data: "
                    "seq_rcv: %lu vs seq_our: %lu, "
                    "rountrip: %ld vs max allow: %ld, "
                    "nodeid_rcv: %d vs nodeid_our: %d, "
                    "tstamp_rcv: %lld vs tstamp_our: %lld",
                    their_time->orig_seq, M_seq,
                    rountrip, G_bridge_cfg.max_roundtrip,
                    their_time->orig_nodeid, tpgetnodeid(),
                    their_time->orig_timestamp, M_timediff_tstamp
                    );
        }    
    }
    
    if (load_time)
    {
        diff=ndrx_stopwatch_diff(&our_time, &their_time->time);
    
        NDRX_LOG(log_warn, "Monotonic clock time correction between us "
                "and node %d is: %lld msec (%d), roundtrip: %ld ms, seq: %ld, data mode: %d", 
                call->caller_nodeid, diff, sizeof(diff), rountrip, M_seq, their_time->mode);
        
        
        /* so if admin tool reads the stuff needs to have spin + to get all readings... */
        NDRX_SPIN_LOCK_V(G_bridge_cfg.timediff_lock);
        G_bridge_cfg.timediff=diff;
        NDRX_SPIN_UNLOCK_V(G_bridge_cfg.timediff_lock);
        
        /* normally there shall be now time updates in the row
         * and the bellow infos are use only for admin tool
         * thus do not keep the spinlock for al long...
         */
        G_bridge_cfg.timediff_ourt = our_time;
        G_bridge_cfg.timediff_roundtrip = rountrip;
        
    }
    MUTEX_UNLOCK_V(M_timediff_lock);
    
out:
    return ret;
}


/**
 * Send or clock to server.
 * @param mode see NDRX_BRCLOCK_MODE_* consts
 * @param in case of NDRX_BRCLOCK_MODE_RSP this holds the original request
 * @return EXSUCCEED/EXFAIL 
 */
expublic int br_send_clock(int mode, cmd_br_time_sync_t *rcv)
{
    char *fn = "br_send_clock";
    cmd_br_time_sync_t ourtime;
    int ret=EXSUCCEED;
    
    NDRX_LOG(log_debug, "%s - enter", fn);
    
    memset(&ourtime, 0, sizeof(ourtime));
    
    cmd_generic_init(NDRXD_COM_BRCLOCK_RQ, NDRXD_SRC_BRIDGE, NDRXD_CALL_TYPE_BRBCLOCK,
                            (command_call_t *)&ourtime, ndrx_get_G_atmi_conf()->reply_q_str);
    ndrx_stopwatch_reset(&ourtime.time);
    
    if (NDRX_BRCLOCK_MODE_RSP==mode)
    {
        /* reply with original data  */
        ourtime.orig_nodeid = rcv->orig_nodeid;
        ourtime.orig_timestamp = rcv->orig_timestamp;
        ourtime.orig_seq = rcv->orig_seq;
    }
    else
    {
        /* make new request async at startup or request */
        ourtime.orig_nodeid = tpgetnodeid();
        
        MUTEX_LOCK_V(M_timediff_lock);
        /* have tstamp for correlation.. */
        ourtime.orig_timestamp = time(NULL);
        M_timediff_tstamp = ourtime.orig_timestamp;
        ndrx_stopwatch_reset(&M_timediff_sent);
        /* let if overflow, no problem... */
        M_seq++;
        ourtime.orig_seq = M_seq;
        MUTEX_UNLOCK_V(M_timediff_lock);
    }
    
    ourtime.mode=mode;
    
    ret=br_send_to_net((char*)&ourtime, sizeof(ourtime), BR_NET_CALL_MSG_TYPE_NDRXD, 
            ourtime.call.command);
    
out:

    NDRX_LOG(log_debug, "%s return %d", fn, ret);
    return ret;
    
}

/**
 * Adjust clock in packet.
 * @return 
 */
expublic void br_clock_adj(tp_command_call_t *call, int is_out)
{
    long long diff;
    
    NDRX_SPIN_LOCK_V(G_bridge_cfg.timediff_lock);
    diff = G_bridge_cfg.timediff;
    NDRX_SPIN_UNLOCK_V(G_bridge_cfg.timediff_lock);
    
    N_TIMER_DUMP(log_info, "Call timer: ", call->timer);    
#if CLOCK_DEBUG
    ndrx_stopwatch_t our_time;
    ndrx_stopwatch_reset(&our_time);
    NDRX_LOG(log_debug, "Original call age: %lld ms", 
            ndrx_stopwatch_diff(&call->timer, &our_time));
#endif
    if (is_out)
    {
        ndrx_stopwatch_minus(&call->timer, diff);
    }
    else
    {
        ndrx_stopwatch_plus(&call->timer, diff);
    }
        
    NDRX_LOG(log_debug, "Clock diff: %lld ms", diff);
    N_TIMER_DUMP(log_info, "Adjusted call timer: ", call->timer);
    
#if CLOCK_DEBUG
    NDRX_LOG(log_debug, "New call age: %lld ms", 
            ndrx_stopwatch_diff(&call->timer, &our_time));
    NDRX_LOG(log_debug, "Clock based call age (according to tstamp): %d", 
            time(NULL) - call->timestamp);
#endif
        
}
/* vim: set ts=4 sw=4 et smartindent: */
