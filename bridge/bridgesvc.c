/**
 * @brief Bridge server
 *   This is special kind of EnduroX Server.
 *   It does configures the library to work in bridged mode.
 *   It parses command line by looking of following config:
 *   In client mode (-tA = type [A]ctive, connect to server)
 *   -n <Node id of server> -i <IP of the server> -p <Port of the server> -tA
 *   In server mode (-tP = type [P]asive, wait for call)
 *   -n <node id of server> -i <Bind address usually 0.0.0.0> -p <Port to bind on> -tP
 *   Notes for multi thread:
 *   - Outgoing message fully received by xatmi main thread,
 *   and is submitted to thread pool. The treads perform async send to network.
 *   - Incoming message from network also are fully received from main thread.
 *   Once read fully, submitted to thread pool for further processing.
 *   In case if thread count is set to 0, then do not use threading model, just
 *   just do direct calls if send & receive.
 *
 * @file bridgesvc.c
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
#include <ndrx_config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <regex.h>
#include <utlist.h>
#include <unistd.h>    /* for getopt */
#include <poll.h>

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
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
expublic bridge_cfg_t G_bridge_cfg;
expublic __thread int G_thread_init = EXFALSE; /* Was thread init done?     */
/*---------------------------Statics------------------------------------*/
exprivate int M_init_ok = EXFALSE;
/*---------------------------Prototypes---------------------------------*/

/**
 * Send status to NDRXD
 * @param is_connected TRUE connected, FALSE not connected
 * @return SUCCEED/FAIL 
 */
exprivate int br_send_status(int is_connected)
{
    int ret = EXSUCCEED;
    
    bridge_info_t gencall;

    /* Reset gencall */
    memset(&gencall, 0, sizeof(gencall));
    gencall.nodeid = G_bridge_cfg.nodeid;
    
    NDRX_LOG(log_debug, "Reporting to ndrxd (is_connected=%d)", is_connected);
    
    /* Send command call to ndrxd */
    ret=cmd_generic_call((is_connected?NDRXD_COM_BRCON_RQ:NDRXD_COM_BRDISCON_RQ),
                    NDRXD_SRC_BRIDGE,
                    NDRXD_CALL_TYPE_BRIDGEINFO,
                    (command_call_t *)&gencall, sizeof(bridge_info_t),
                    ndrx_get_G_atmi_conf()->reply_q_str,
                    ndrx_get_G_atmi_conf()->reply_q,
                    (mqd_t)EXFAIL,   /* do not keep open ndrxd q open */
                    ndrx_get_G_atmi_conf()->ndrxd_q_str,
                    0, NULL,
                    NULL,
                    NULL,
                    NULL,
                    EXFALSE);
    
    return ret;
}
/**
 * Connection have been established
 */
expublic int br_connected(exnetcon_t *net)
{
    int ret=EXSUCCEED;
    
    /* once created this shall not change... */
    G_bridge_cfg.con = net;
    NDRX_LOG(log_debug, "Net=%p", G_bridge_cfg.net);
    
    /* Send our clock to other node. */
    if (EXSUCCEED==br_send_clock())
    {
        ret=br_send_status(EXTRUE);
    }
    return ret;
}

/**
 * Bridge is disconnected.
 */
expublic int br_disconnected(exnetcon_t *net)
{
    int ret=EXSUCCEED;
    
    /*
     * - leave object in place...
    G_bridge_cfg.con = NULL;
    */
    ret=br_send_status(EXFALSE);
    
    return ret;  
}

/**
 * Send zero length message to socket, to keep some activity
 * Processed by sender worker thread
 * @param ptr ptr to network structure
 * @param p_finish_off not used
 * @return EXSUCCEED
 */
exprivate int br_snd_zero_len_th(void *ptr, int *p_finish_off)
{
    exnetcon_t *net = (exnetcon_t *)ptr;
    
    /* Lock to network */
    exnet_rwlock_read(net);

    if (exnet_is_connected(net))
    {
        if (EXSUCCEED!=exnet_send_sync(net, NULL, 0, NULL, 0, 0, 0))
        {
            NDRX_LOG(log_debug, "Failed to send zero length message!");
        }
    }

    /* unlock the network */
    exnet_rwlock_unlock(net); 
    
    return EXSUCCEED;
}

/**
 * Processed by main thread / dispatch to worker pool
 * @return EXUSCCEED
 */
exprivate int br_snd_zero_len(exnetcon_t *net)
{
    ndrx_thpool_add_work(G_bridge_cfg.thpool_tonet, (void *)br_snd_zero_len_th, (void *)net);
    return EXSUCCEED;
}

/**
 * Report status to ndrxd by callback, so that when ndrxd is being restarted
 * we get back correct bridge state.
 * @return SUCCEED/FAIL
 */
expublic int br_report_to_ndrxd_cb(void)
{
    int ret = EXSUCCEED;
    
    NDRX_LOG(log_warn, "br_report_to_ndrxd_cb: Reporting to ndrxd bridge status: %s", 
            (G_bridge_cfg.con?"Connected":"Disconnected"));
    if (G_bridge_cfg.con)
    {
        ret=br_send_status(EXTRUE);
    }
    else
    {
        ret=br_send_status(EXFALSE);
    }
    
    return ret;
}

/**
 * Periodic poll callback.
 * @return 
 */
expublic int poll_timer(void)
{
    /* run any queue left overs... */
    br_run_q();
    
    NDRX_LOG(log_debug, "FD=%d", G_bridge_cfg.net.sock);
    
    return exnet_periodic();
}

/**
 * Invode before poll.
 * @return 
 */
int b4poll(void)
{
    ndrx_stopwatch_t w;
    int spent, ret;
    
    /* avoid over extensive queues. Have some free
     * free thread (i.e. there is less jobs queued
     * then free threads)
     */
    ndrx_thpool_wait_one(G_bridge_cfg.thpool_fromnet);
    
    /*
     * For sending to net, the socket may be full/blocked
     * thus try to receive messages from network.
     * Every 1 sec still process the poll as we might want to receive pings, etc.
     */
    spent = 0;
    ndrx_stopwatch_reset(&w);
    while (!ndrx_thpool_is_one_avail(G_bridge_cfg.thpool_tonet) && 
            NULL!=G_bridge_cfg.con && 
            G_bridge_cfg.con->is_connected && 
            !G_bridge_cfg.con->schedule_close && 
            spent < 1000)
    {
        struct pollfd ufd;
        memset(&ufd, 0, sizeof ufd);
        ufd.fd = G_bridge_cfg.con->sock;
        ufd.events = POLLOUT | POLLIN;

        ret=poll(&ufd, 1, 1000 - spent);

        if (ret > 0)
        {
            if (ufd.revents & POLLOUT)
            {
                /* we can start to send */
                break;
            }
            else if (ufd.revents & POLLIN)
            {
                int buflen = 0;
                char *buf = NULL;

                NDRX_LOG(log_warn, "Early msg receive (due to blocked sending)");
                
                /* we can receive something */
                if(EXSUCCEED == exnet_recv_sync(G_bridge_cfg.con, &buf, &buflen, 0, 0))
                {
                    /* We got the message - do the callback op */
                    G_bridge_cfg.con->p_process_msg(G_bridge_cfg.con, &buf, buflen);
                }
                
                if (NULL!=buf)
                {
                    NDRX_SYSBUF_FREE(buf);
                }
            }
            else
            {
                /* we do not expect anything more... */
                break;
            }
        }
        else
        {
            /* if timedout or error finish off... */
            break;
        }

        spent = ndrx_stopwatch_get_delta_sec(&w);
    }

    return exnet_b4_poll_cb();
}

/**
 * Bridge service entry
 * @param p_svc - data & len used only...!
 */
void TPBRIDGE (TPSVCINFO *p_svc)
{
    /* Dummy service, calls will never reach this point. */
}

/**
 * Do initialization
 * For bridge we could make a special rq address, for example "@TPBRIDGENNN"
 * we will an API for ndrx_reqaddrset("...") which would configure the libnstd
 * properly.
 */
int NDRX_INTEGRA(tpsvrinit)(int argc, char **argv)
{
    int ret=EXSUCCEED;
    int c;
    int i;
    int is_server = EXFAIL;
    char addr[EXNET_ADDR_LEN] = {EXEOS};
    int port=EXFAIL;
    int rcvtimeout = ndrx_get_G_atmi_env()->time_out;
    int backlog = 100;
    int flags = SRV_KEY_FLAGS_BRIDGE; /* This is bridge */
    int check=5;  /* Connection check interval, seconds */
    int periodic_zero = 0; /* send zero length messages periodically */
    int recv_activity_timeout = EXFAIL;
    int thpoolcfg = 0;
    NDRX_LOG(log_debug, "tpsvrinit called");
    
    G_bridge_cfg.nodeid = EXFAIL;
    G_bridge_cfg.timediff = 0;
    G_bridge_cfg.threadpoolsize = BR_DEFAULT_THPOOL_SIZE; /* will be reset to default */
    G_bridge_cfg.qretries = BR_QRETRIES_DEFAULT;
    /* Parse command line  */
    while ((c = getopt(argc, argv, "frn:i:p:t:T:z:c:g:s:P:R:a:")) != -1)
    {
        /* NDRX_LOG(log_debug, "%c = [%s]", c, optarg); - on solaris gets cores? */
        switch(c)
        {
            case 'r':
                NDRX_LOG(log_debug, "Will send refersh to node.");
                /* Will send refersh */
                flags|=SRV_KEY_FLAGS_SENDREFERSH;
                break;
            case 'n':
                G_bridge_cfg.nodeid=(short)atoi(optarg);
                NDRX_LOG(log_debug, "Node ID, -n = [%hd]", G_bridge_cfg.nodeid);
                break;
            case 'i':
                NDRX_STRCPY_SAFE(addr, optarg);
                NDRX_LOG(log_debug, "IP server/binding addresss, -i = [%s]", addr);
                break;
            case 'p':
                port = atoi(optarg);
		/* port will be promoted to integer... */
                NDRX_LOG(log_debug, "Port no, -p = [%d]", port);
                break;
            case 'T':
                rcvtimeout = atoi(optarg);
                NDRX_LOG(log_debug, "Receive time-out (-T): %d", 
                        rcvtimeout);
                break;
            case 'b':
                backlog = atoi(optarg);
                NDRX_LOG(log_debug, "Backlog (-b): %d", 
                        backlog);
                break;
            case 'c':
                check = atoi(optarg);
                NDRX_LOG(log_debug, "check (-c): %d", 
                        check);
                break;
            case 'z':
                periodic_zero = atoi(optarg);
                NDRX_LOG(log_debug, "periodic_zero (-z): %d", 
                                periodic_zero);
                break;
            case 'a':
                recv_activity_timeout = atoi(optarg);
                NDRX_LOG(log_debug, "recv_activity_timeout (-a): %d", 
                                recv_activity_timeout);
                break;
            case 'f':
                G_bridge_cfg.common_format = EXTRUE;
                NDRX_LOG(log_debug, "Using common network protocol.");
                break;
            case 'g':
                NDRX_STRCPY_SAFE(G_bridge_cfg.gpg_recipient, optarg);
                NDRX_LOG(log_debug, "Using GPG Encryption, recipient: [%s]", 
					G_bridge_cfg.gpg_recipient);
                break;
            case 's':
                NDRX_STRCPY_SAFE(G_bridge_cfg.gpg_signer, optarg);
                NDRX_LOG(log_debug, "Using GPG Encryption, signer: [%s]", 
					G_bridge_cfg.gpg_signer);
                break;
            case 'P': 
                /* half is used for download, and other half for upload */
                thpoolcfg = atol(optarg);
                G_bridge_cfg.threadpoolsize = thpoolcfg / 2;
                break;
            case 'R': 
                G_bridge_cfg.qretries = atoi(optarg);
                break;
            case 't': 
                
                if ('P'==*optarg)
                {
                    NDRX_LOG(log_debug, "Server mode enabled - "
                            "will wait for call");
                    is_server = EXTRUE;
                }
                else
                {
                    NDRX_LOG(log_debug, "Client mode enabled - "
                            "will connect to server");
                    is_server = EXFALSE;
                }
                break;
            default:
                /*return FAIL;*/
                break;
        }
    }
    
    if (0>recv_activity_timeout)
    {
        recv_activity_timeout = periodic_zero*2;
    }
    
    if (G_bridge_cfg.threadpoolsize < 1)
    {
        NDRX_LOG(log_warn, "Thread pool size (-P) have invalid value "
                "(%d) defaulting to %d", 
                thpoolcfg, BR_DEFAULT_THPOOL_SIZE*2);
        G_bridge_cfg.threadpoolsize = BR_DEFAULT_THPOOL_SIZE;
    }
    
    NDRX_LOG(log_warn, "Threadpool size set to: from-net=%d to-net=%d (cfg=%d)",
            G_bridge_cfg.threadpoolsize, G_bridge_cfg.threadpoolsize, thpoolcfg);
    
    NDRX_LOG(log_warn, "Periodic zero: %d sec, reset on no received: %d sec",
            periodic_zero, recv_activity_timeout);
    
    /* Check configuration */
    if (EXFAIL==G_bridge_cfg.nodeid)
    {
        NDRX_LOG(log_error, "Flag -n not set!");
        EXFAIL_OUT(ret);
    }
    
    if (EXEOS==addr[0])
    {
        NDRX_LOG(log_error, "Flag -i not set!");
        EXFAIL_OUT(ret);
    }
    
    if (EXFAIL==port)
    {
        NDRX_LOG(log_error, "Flag -p not set!");
        EXFAIL_OUT(ret);
    }
    
    if (EXFAIL==is_server)
    {
        NDRX_LOG(log_error, "Flag -T not set!");
        EXFAIL_OUT(ret);
    }
    
#ifndef DISABLEGPGME

    if (EXEOS!=G_bridge_cfg.gpg_recipient[0])
    {
        if (EXSUCCEED!=br_init_gpg())
        {
            NDRX_LOG(log_error, "GPG init fail");
                EXFAIL_OUT(ret);
        }
        else
        {
            NDRX_LOG(log_error, "GPG init OK");
        }
    }
#endif
    
    if (EXSUCCEED!=ndrx_br_init_queue())
    {
        NDRX_LOG(log_error, "Failed to init queue runner");
        EXFAIL_OUT(ret);
    }
    
    /* Reset network structs */
    exnet_reset_struct(&G_bridge_cfg.net);
    
    /* Allocate network buffer */
    if (EXSUCCEED!=exnet_net_init(&G_bridge_cfg.net))
    {
        NDRX_LOG(log_error, "Failed to allocate data buffer!");
        EXFAIL_OUT(ret);
    }
        
    /* Install call-backs */
    exnet_install_cb(&G_bridge_cfg.net, br_process_msg, br_connected, 
            br_disconnected, br_snd_zero_len);
    
    ndrx_set_report_to_ndrxd_cb(br_report_to_ndrxd_cb);
    
    /* Then configure the lib - we will have only one client session! */
    if (EXSUCCEED!=exnet_configure(&G_bridge_cfg.net, rcvtimeout, addr, port, 
        NET_LEN_PFX_LEN, is_server, backlog, 1, periodic_zero,
            recv_activity_timeout))
    {
        NDRX_LOG(log_error, "Failed to configure network lib!");
        EXFAIL_OUT(ret);
    }
    
    /* Register timer check.... */
    if (EXSUCCEED==ret &&
            EXSUCCEED!=tpext_addperiodcb(check, poll_timer))
    {
        ret=EXFAIL;
        NDRX_LOG(log_error, "tpext_addperiodcb failed: %s",
                        tpstrerror(tperrno));
    }
    
    /* Register poller callback.... */
    if (EXSUCCEED==ret &&
            EXSUCCEED!=tpext_addb4pollcb(b4poll))
    {
        ret=EXFAIL;
        NDRX_LOG(log_error, "tpext_addb4pollcb failed: %s",
                        tpstrerror(tperrno));
    }
    
    /* Set server flags  */
    tpext_configbrige(G_bridge_cfg.nodeid, flags, br_got_message_from_q);
    
    snprintf(G_bridge_cfg.svc, sizeof(G_bridge_cfg.svc), NDRX_SVC_BRIDGE, 
            G_bridge_cfg.nodeid);
    
    if (EXSUCCEED!=tpadvertise(G_bridge_cfg.svc, TPBRIDGE))
    {
        NDRX_LOG(log_error, "Failed to advertise %s service!", G_bridge_cfg.svc);
        ret=EXFAIL;
        goto out;
    }
    
    if (NULL==(G_bridge_cfg.thpool_tonet = ndrx_thpool_init(G_bridge_cfg.threadpoolsize, 
            NULL, NULL, NULL, 0, NULL)))
    {
        NDRX_LOG(log_error, "Failed to initialize to-net thread pool (cnt: %d)!", 
                G_bridge_cfg.threadpoolsize);
        EXFAIL_OUT(ret);
    }
    
    if (NULL==(G_bridge_cfg.thpool_fromnet = ndrx_thpool_init(G_bridge_cfg.threadpoolsize, 
            NULL, NULL, NULL, 0, NULL)))
    {
        NDRX_LOG(log_error, "Failed to initialize from-net thread pool (cnt: %d)!",
                G_bridge_cfg.threadpoolsize);
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_info, "Queue re-submit retries set to: %d", G_bridge_cfg.qretries);
    
    M_init_ok = EXTRUE;
    
out:
    return ret;
}

/**
 * Shutdown the thread
 * @param arg
 * @param p_finish_off
 */
expublic void tp_thread_shutdown(void *ptr, int *p_finish_off)
{
    NDRX_LOG(log_info, "tp_thread_shutdown - enter");
    tpterm();
    *p_finish_off = EXTRUE;
    NDRX_LOG(log_info, "tp_thread_shutdown - ok");
}


/**
 * Do de-initialization
 */
void NDRX_INTEGRA(tpsvrdone)(void)
{
    int i;
    NDRX_LOG(log_debug, "tpsvrdone called");
    
    /* Bug #170
     * Shutdown threads and only then close connection
     * Otherwise we remove connection and threads are generating core at shutdown.
     * as network object is gone..
     */
    if (M_init_ok)
    {
        /* Terminate the threads */
        for (i=0; i<G_bridge_cfg.threadpoolsize; i++)
        {
            NDRX_LOG(log_info, "Terminating to-net threadpool, thread #%d", i);
            ndrx_thpool_add_work(G_bridge_cfg.thpool_tonet, (void *)tp_thread_shutdown, NULL);
        }
        
        /* Terminate the threads */
        for (i=0; i<G_bridge_cfg.threadpoolsize; i++)
        {
            NDRX_LOG(log_info, "Terminating from-net threadpool, thread #%d", i);
            ndrx_thpool_add_work(G_bridge_cfg.thpool_fromnet, (void *)tp_thread_shutdown, NULL);
        }
        
        /* Wait for threads to finish */
        ndrx_thpool_wait(G_bridge_cfg.thpool_tonet);
        ndrx_thpool_destroy(G_bridge_cfg.thpool_tonet);
        
        ndrx_thpool_wait(G_bridge_cfg.thpool_fromnet);
        ndrx_thpool_destroy(G_bridge_cfg.thpool_fromnet);
    }
    
    /* close if not server connection...  */
    if (NULL!=G_bridge_cfg.con && (&G_bridge_cfg.net)!=G_bridge_cfg.con)
        
    {
        exnet_close_shut(G_bridge_cfg.con);
    }
    
    /* If we were server, then close server socket too */
    if (G_bridge_cfg.net.is_server)
    {
        exnet_close_shut(&G_bridge_cfg.net);
    }
    
    ndrx_br_uninit_queue();
}

/* vim: set ts=4 sw=4 et smartindent: */
