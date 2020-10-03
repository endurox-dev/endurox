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
    /* no need to send to network if already has to-net jobs... 
     * thus avoid any blocking conditions on outgoing...
     */
    ndrx_thpool_add_work2(G_bridge_cfg.thpool_tonet, (void *)br_snd_zero_len_th, 
            (void *)net, NDRX_THPOOL_ONEJOB, 0);
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
 * Bridge service entry
 * @param p_svc - data & len used only...!
 */
void TPBRIDGE (TPSVCINFO *p_svc)
{
    /* Dummy service, calls will never reach this point. */
}

/**
 * Terminate the thread...
 */
void ndrx_thpool_thread_done(void)
{
    tpterm();
}

/**
 * This is thread init
 * @param argc
 * @param argv
 * @return EXSUCCEED/EXFAIL
 */
int ndrx_thpool_thread_init(int argc, char **argv)
{
    int ret = EXSUCCEED;
    
    if (EXSUCCEED!=tpinit(NULL))
    {
        NDRX_LOG(log_error, "Failed to tpinit for thread...");
        EXFAIL_OUT(ret);
    }
    
out:
    return ret;
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
    char *p;
    int flags = SRV_KEY_FLAGS_BRIDGE; /* This is bridge */
    int thpoolcfg = 0;
    int qaction=EXFAIL;
    NDRX_LOG(log_debug, "tpsvrinit called");
    
    /* Reset network structs */
    exnet_reset_struct(&G_bridge_cfg.net);
    
    G_bridge_cfg.nodeid = EXFAIL;
    G_bridge_cfg.qsize = EXFAIL;
    G_bridge_cfg.qsizesvc = EXFAIL;
    G_bridge_cfg.qttl= EXFAIL;
    G_bridge_cfg.qmaxsleep= EXFAIL;
    G_bridge_cfg.qminsleep= EXFAIL;
    G_bridge_cfg.timediff = 0;
    G_bridge_cfg.threadpoolsize = BR_DEFAULT_THPOOL_SIZE; /* will be reset to default */
    G_bridge_cfg.qretries = BR_QRETRIES_DEFAULT;
    G_bridge_cfg.check_interval=EXFAIL;
    G_bridge_cfg.threadpoolbufsz=EXFAIL;
    G_bridge_cfg.qfullaction = EXFAIL;
    G_bridge_cfg.qfullactionsvc = EXFAIL;
    
    /* Parse command line  */
    while ((c = getopt(argc, argv, "frn:i:p:t:T:z:c:g:s:P:R:a:6h:Q:q:L:M:B:m:A:")) != -1)
    {
        /* NDRX_LOG(log_debug, "%c = [%s]", c, optarg); - on solaris gets cores? */
        switch(c)
        {
            case '6':
                NDRX_LOG(log_debug, "Using IPv6 addresses");
                G_bridge_cfg.net.is_ipv6=EXTRUE;
                break;
            case 'r':
                NDRX_LOG(log_debug, "Will send refersh to node.");
                /* Will send refersh */
                flags|=SRV_KEY_FLAGS_SENDREFERSH;
                break;
            case 'n':
                G_bridge_cfg.nodeid=(short)atoi(optarg);
                NDRX_LOG(log_debug, "Node ID, -n = [%hd]", G_bridge_cfg.nodeid);
                break;
            case 'Q':
                G_bridge_cfg.qsize=(short)atoi(optarg);
                NDRX_LOG(log_debug, "Temporary queue size, -Q = [%d]", G_bridge_cfg.qsize);
                break;
            case 'q':
                G_bridge_cfg.qsizesvc=(short)atoi(optarg);
                NDRX_LOG(log_debug, "Temporary service queue size, -q = [%d]", 
                        G_bridge_cfg.qsizesvc);
                break;
            case 'A':
                qaction=(short)atoi(optarg);
                NDRX_LOG(log_debug, "Temp queue action, -A = [%d]", 
                        qaction);
                break;
            case 'L':
                G_bridge_cfg.qttl=(short)atoi(optarg);
                NDRX_LOG(log_debug, "Temporary queue TTL, -L = [%d] ms", G_bridge_cfg.qttl);
                break;
            case 'M':
                G_bridge_cfg.qmaxsleep=(short)atoi(optarg);
                NDRX_LOG(log_debug, "Temporary queue Max Sleep, -M = [%d] ms", 
                        G_bridge_cfg.qmaxsleep);
                break;
                
            case 'm':
                G_bridge_cfg.qminsleep=(short)atoi(optarg);
                NDRX_LOG(log_debug, "Temporary queue Min Sleep, -m = [%d] ms", 
                        G_bridge_cfg.qmaxsleep);
                break;
            
            case 'B':
                G_bridge_cfg.threadpoolbufsz=(short)atoi(optarg);
                NDRX_LOG(log_debug, "Thread pool buffer size, -B = [%d]", 
                        G_bridge_cfg.threadpoolbufsz);
                break;
            case 'i':
                if (EXEOS!=G_bridge_cfg.net.addr[0])
                {
                    NDRX_LOG(log_error, "ERROR! Connection address already set "
                            "to: [%s] cannot process -i", G_bridge_cfg.net.addr);
                    EXFAIL_OUT(ret);
                }
                
                NDRX_STRCPY_SAFE(G_bridge_cfg.net.addr, optarg);
                NDRX_LOG(log_debug, "IP server/binding address, -i = [%s]", 
                        G_bridge_cfg.net.addr);
                G_bridge_cfg.net.is_numeric = EXTRUE;
                break;
                
            case 'h':
                
                if (EXEOS!=G_bridge_cfg.net.addr[0])
                {
                    NDRX_LOG(log_error, "ERROR! Connection address already set "
                            "to: [%s] connect process -h", G_bridge_cfg.net.addr);
                    EXFAIL_OUT(ret);
                }
                
                NDRX_STRCPY_SAFE(G_bridge_cfg.net.addr, optarg);
                NDRX_LOG(log_debug, "DNS host name, -h = [%s]", 
                        G_bridge_cfg.net.addr);
                G_bridge_cfg.net.is_numeric = EXFALSE;
                break;
            case 'p':
                NDRX_STRCPY_SAFE(G_bridge_cfg.net.port, optarg);
		/* port will be promoted to integer... */
                NDRX_LOG(log_debug, "Port no, -p = [%s]", G_bridge_cfg.net.port);
                break;
            case 'T':
                G_bridge_cfg.net.rcvtimeout = atoi(optarg);
                NDRX_LOG(log_debug, "Receive time-out (-T): %d", 
                        G_bridge_cfg.net.rcvtimeout);
                break;
            case 'b':
                G_bridge_cfg.net.backlog = atoi(optarg);
                NDRX_LOG(log_debug, "Backlog (-b): %d", 
                        G_bridge_cfg.net.backlog);
                break;
            case 'c':
                G_bridge_cfg.check_interval = atoi(optarg);
                NDRX_LOG(log_debug, "check (-c): %d", 
                        G_bridge_cfg.check_interval);
                break;
            case 'z':
                G_bridge_cfg.net.periodic_zero = atoi(optarg);
                NDRX_LOG(log_debug, "periodic_zero (-z): %d", 
                                G_bridge_cfg.net.periodic_zero);
                break;
            case 'a':
                G_bridge_cfg.net.recv_activity_timeout = atoi(optarg);
                NDRX_LOG(log_debug, "recv_activity_timeout (-a): %d", 
                                G_bridge_cfg.net.recv_activity_timeout);
                break;
            case 'f':
                G_bridge_cfg.common_format = EXTRUE;
                NDRX_LOG(log_debug, "Using common network protocol.");
                break;
            case 'g':
                NDRX_LOG(log_warn, "-g not supported any more");
                break;
            case 's':
                NDRX_LOG(log_warn, "-s not supported any more");
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
                    G_bridge_cfg.net.is_server = EXTRUE;
                }
                else
                {
                    NDRX_LOG(log_debug, "Client mode enabled - "
                            "will connect to server");
                    G_bridge_cfg.net.is_server = EXFALSE;
                }
                break;
            default:
                /*return FAIL;*/
                break;
        }
    }
    
    if (G_bridge_cfg.qsize <= 0 && NULL!=(p=getenv(CONF_NDRX_MSGMAX)))
    {
        NDRX_LOG(log_debug, "Reading queue size from [%s]", CONF_NDRX_MSGMAX);
        G_bridge_cfg.qsize = atoi(p);
    }
    
    if (G_bridge_cfg.qsize <= 0)
    {
        NDRX_LOG(log_debug, "Defaulting queue size");
        G_bridge_cfg.qsize = DEFAULT_QUEUE_SIZE;
    }
    
    if (G_bridge_cfg.qsizesvc <= 0)
    {
        NDRX_LOG(log_debug, "Defaulting service queue size");
        G_bridge_cfg.qsizesvc = G_bridge_cfg.qsize / 2;
        
        if ( G_bridge_cfg.qsizesvc < 1)
        {
            G_bridge_cfg.qsizesvc=1;
        }
    }
    
    /* there is no sense to have service queue bigger than global limit */
    if (G_bridge_cfg.qsize < G_bridge_cfg.qsizesvc)
    {
        NDRX_LOG(log_error, "Error: Global temp queue size shorter than "
                "service queue size: -Q (%d) <  -q (%d)", 
                G_bridge_cfg.qsize, G_bridge_cfg.qsizesvc);
        EXFAIL_OUT(ret);
    }
    
    if (G_bridge_cfg.qmaxsleep <= 0)
    {
        NDRX_LOG(log_debug, "Defaulting queue max sleep");
        G_bridge_cfg.qmaxsleep = DEFAULT_QUEUE_MAXSLEEP;
    }
    
    if (G_bridge_cfg.qminsleep <= 0)
    {
        NDRX_LOG(log_debug, "Defaulting queue min sleep");
        G_bridge_cfg.qminsleep = DEFAULT_QUEUE_MINSLEEP;
    }
    
    if (G_bridge_cfg.qttl < 0)
    {
        NDRX_LOG(log_debug, "Setting TTL to NDRX_TOUT value");
        /* convert from seconds to ms */
        G_bridge_cfg.qttl = tptoutget()*1000;
    }
    
     if (G_bridge_cfg.threadpoolsize < 1)
    {
        NDRX_LOG(log_warn, "Thread pool size (-P) have invalid value "
                "(%d) defaulting to %d", 
                thpoolcfg, BR_DEFAULT_THPOOL_SIZE*2);
        G_bridge_cfg.threadpoolsize = BR_DEFAULT_THPOOL_SIZE;
    }
    
    if (G_bridge_cfg.threadpoolbufsz < 0)
    {
        G_bridge_cfg.threadpoolbufsz = G_bridge_cfg.threadpoolsize/2;
    }
    
    if (G_bridge_cfg.check_interval < 1)
    {
        G_bridge_cfg.check_interval=5;
    }
    
    /* configure action */
    switch (qaction)
    {
        case EXFAIL:
        case QUEUE_FLAG_ACTION_BLKIGN:
            G_bridge_cfg.qfullaction = QUEUE_ACTION_BLOCK;
            G_bridge_cfg.qfullactionsvc = QUEUE_ACTION_IGNORE;
            NDRX_LOG(log_warn, "Queue action: temp full - block, svc full - ignore");
            break;
        case QUEUE_FLAG_ACTION_BLKDROP:
            G_bridge_cfg.qfullaction = QUEUE_ACTION_BLOCK;
            G_bridge_cfg.qfullactionsvc = QUEUE_ACTION_DROP;
            NDRX_LOG(log_warn, "Queue action: temp full - block, svc full - drop");
            break;
        case QUEUE_FLAG_ACTION_DROPDROP:
            G_bridge_cfg.qfullaction = QUEUE_ACTION_BLOCK;
            G_bridge_cfg.qfullactionsvc = QUEUE_ACTION_DROP;
            NDRX_LOG(log_warn, "Queue action: temp full - drop, svc full - drop");
            break;
        default:
            
            NDRX_LOG(log_error, "Invalid -A value: %d, supported: %d %d %d default: %d",
                    QUEUE_FLAG_ACTION_BLKIGN, QUEUE_FLAG_ACTION_BLKDROP, 
                    QUEUE_FLAG_ACTION_DROPDROP,
                    QUEUE_FLAG_ACTION_DROPDROP);
            EXFAIL_OUT(ret);
            
            break;
    }
    
    NDRX_LOG(log_warn, "Temporary queue size set to: %d", G_bridge_cfg.qsize);
    NDRX_LOG(log_warn, "Temporary service queue size set to: %d", G_bridge_cfg.qsizesvc);
    NDRX_LOG(log_warn, "Temporary queue ttl set to: %d", G_bridge_cfg.qttl);
    NDRX_LOG(log_warn, "Temporary queue max sleep set to: %d", G_bridge_cfg.qmaxsleep);
    NDRX_LOG(log_warn, "Temporary queue min sleep set to: %d", G_bridge_cfg.qminsleep);
    NDRX_LOG(log_warn, "Threadpool job queue size: %d", G_bridge_cfg.threadpoolbufsz);
    NDRX_LOG(log_warn, "Check interval is: %d seconds", G_bridge_cfg.check_interval);
    
    if (0>G_bridge_cfg.net.recv_activity_timeout)
    {
        G_bridge_cfg.net.recv_activity_timeout = G_bridge_cfg.net.periodic_zero*2;
    }
    
    NDRX_LOG(log_warn, "Threadpool size set to: from-net=%d to-net=%d (cfg=%d)",
            G_bridge_cfg.threadpoolsize, G_bridge_cfg.threadpoolsize, thpoolcfg);
    
    NDRX_LOG(log_warn, "Periodic zero: %d sec, reset on no received: %d sec",
            G_bridge_cfg.net.periodic_zero, G_bridge_cfg.net.recv_activity_timeout);
    
    /* Check configuration */
    if (EXFAIL==G_bridge_cfg.nodeid)
    {
        NDRX_LOG(log_error, "Flag -n not set!");
        EXFAIL_OUT(ret);
    }
    
    if (EXEOS==G_bridge_cfg.net.addr[0])
    {
        NDRX_LOG(log_error, "Flag -i not set!");
        EXFAIL_OUT(ret);
    }
    
    if (EXFAIL==G_bridge_cfg.net.port[0])
    {
        NDRX_LOG(log_error, "Flag -p not set!");
        EXFAIL_OUT(ret);
    }
    
    if (EXFAIL==G_bridge_cfg.net.is_server)
    {
        NDRX_LOG(log_error, "Flag -T not set!");
        EXFAIL_OUT(ret);
    }
    
    br_tempq_init();
    
    /* Install call-backs */
    exnet_install_cb(&G_bridge_cfg.net, br_process_msg, br_connected, 
            br_disconnected, br_snd_zero_len);
    
    ndrx_set_report_to_ndrxd_cb(br_report_to_ndrxd_cb);
    
    /* Then configure the lib - we will have only one client session! */
    G_bridge_cfg.net.len_pfx=NET_LEN_PFX_LEN;
    G_bridge_cfg.net.max_cons=1;
    
    if (EXSUCCEED!=exnet_configure(&G_bridge_cfg.net))
    {
        NDRX_LOG(log_error, "Failed to configure network lib!");
        EXFAIL_OUT(ret);
    }
    
    /* Set server flags  */
    tpext_configbrige(G_bridge_cfg.nodeid, flags, br_got_message_from_q);
    
    snprintf(G_bridge_cfg.svc, sizeof(G_bridge_cfg.svc), NDRX_SVC_BRIDGE, 
            G_bridge_cfg.nodeid);
    
    if (EXSUCCEED!=tpadvertise(G_bridge_cfg.svc, TPBRIDGE))
    {
        NDRX_LOG(log_error, "Failed to advertise %s service!", G_bridge_cfg.svc);
        EXFAIL_OUT(ret);
    }
    
    if (NULL==(G_bridge_cfg.thpool_tonet = ndrx_thpool_init(G_bridge_cfg.threadpoolsize, 
            &ret, ndrx_thpool_thread_init, ndrx_thpool_thread_done, 0, NULL)))
    {
        NDRX_LOG(log_error, "Failed to initialize to-net thread pool (cnt: %d)!", 
                G_bridge_cfg.threadpoolsize);
        EXFAIL_OUT(ret);
    }
    
    if (NULL==(G_bridge_cfg.thpool_queue = ndrx_thpool_init(1, 
            &ret, ndrx_thpool_thread_init, ndrx_thpool_thread_done, 0, NULL)))
    {
        NDRX_LOG(log_error, "Failed to initialize queue-runner thread pool (cnt: %d)!", 
                G_bridge_cfg.threadpoolsize);
        EXFAIL_OUT(ret);
    }
    
    if (NULL==(G_bridge_cfg.thpool_fromnet = ndrx_thpool_init(G_bridge_cfg.threadpoolsize, 
            &ret, ndrx_thpool_thread_init, ndrx_thpool_thread_done, 0, NULL)))
    {
        NDRX_LOG(log_error, "Failed to initialize from-net thread pool (cnt: %d)!",
                G_bridge_cfg.threadpoolsize);
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=br_netin_setup())
    {
        NDRX_LOG(log_error, "Failed to init network management (net-in) thread");
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
    NDRX_LOG(log_debug, "tpsvrdone called");
    
    /* shutdown network runner... */
    br_netin_shutdown();
    
    /* Bug #170
     * Shutdown threads and only then close connection
     * Otherwise we remove connection and threads are generating core at shutdown.
     * as network object is gone..
     */
    if (M_init_ok)
    {   
        /* Wait for threads to finish */
        ndrx_thpool_wait(G_bridge_cfg.thpool_tonet);
        ndrx_thpool_destroy(G_bridge_cfg.thpool_tonet);
        
        ndrx_thpool_wait(G_bridge_cfg.thpool_fromnet);
        ndrx_thpool_destroy(G_bridge_cfg.thpool_fromnet);
        
        ndrx_thpool_wait(G_bridge_cfg.thpool_queue);
        ndrx_thpool_destroy(G_bridge_cfg.thpool_queue);
    }
    
    /* close if not server connection...  */
    if (NULL!=G_bridge_cfg.con && (&G_bridge_cfg.net)!=G_bridge_cfg.con)   
    {
        /* we do not have a locks... */
        exnet_rwlock_read(G_bridge_cfg.con);
        exnet_close_shut(G_bridge_cfg.con);
    }
    
    /* If we were server, then close server socket too */
    if (G_bridge_cfg.net.is_server)
    {
        /* we do not have a locks... */
        exnet_rwlock_read(&G_bridge_cfg.net);
        exnet_close_shut(&G_bridge_cfg.net);
    }
    
    /* erase addresses... */
    exnet_unconfigure(&G_bridge_cfg.net);
    
}

/* vim: set ts=4 sw=4 et smartindent: */
