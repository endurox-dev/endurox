/**
 * @brief Incoming network poller
 *
 * @file net-in.c
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
#include <fcntl.h>

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
#include "exsha1.h"
#include <ndrxdiag.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

/** The index of the "read" end of the pipe */
#define READ 0
/** The index of the "write" end of the pipe */
#define WRITE 1

#define MAX_POLL_FD     100
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

exprivate int M_shutdownpipe[2];          /**< When the shutdown is needed..   */
exprivate int M_shutdown_req = EXFALSE;   /**< is shutdown requested?          */
exprivate pthread_t M_netin_thread;       /**< thread handler                  */
exprivate int M_init_ok=EXFALSE;          /**< was init ok?                    */
/*---------------------------Prototypes---------------------------------*/
exprivate void * br_netin_run(void *arg);


/**
 * Receive pipe notificatino (currently only for shutdown)
 * @return 0
 */
exprivate int pipenotf(int fd, uint32_t events, void *ptr1)
{
    M_shutdown_req=EXTRUE;
    return EXSUCCEED;
}
/**
 * Basic setup for polling API on the separate thread 
 */
expublic int br_netin_setup(void)
{
    int ret = EXSUCCEED;
    int err;
    
    /* disable polling API... */
    ndrx_ext_pollsync(EXFALSE);
    
    /* O_NONBLOCK */
    if (EXFAIL==pipe(M_shutdownpipe))
    {
        err = errno;
        NDRX_LOG(log_error, "pipe failed: %s", strerror(err));
        EXFAIL_OUT(ret);
    }

    if (EXFAIL==fcntl(M_shutdownpipe[READ], F_SETFL, 
                fcntl(M_shutdownpipe[READ], F_GETFL) | O_NONBLOCK))
    {
        err = errno;
        NDRX_LOG(log_error, "fcntl READ pipe set O_NONBLOCK failed: %s", 
                strerror(err));
        EXFAIL_OUT(ret);
    }
    
    /* create the thread which would do the required polling? */
    if (EXSUCCEED!=tpext_addpollerfd(M_shutdownpipe[READ], POLLIN|POLLERR,
        NULL, pipenotf))
    {
        NDRX_LOG(log_error, "tpext_addpollerfd failed for pipenotf!");
        ret=EXFAIL;
        goto out;
    }
    
    /* create the thread */
    pthread_attr_t pthread_custom_attr;
    pthread_attr_init(&pthread_custom_attr);
    
    /* set some small stacks size, 1M should be fine! */
    ndrx_platf_stack_set(&pthread_custom_attr);
    if (EXSUCCEED!=pthread_create(&M_netin_thread, &pthread_custom_attr, 
            br_netin_run, NULL))
    {
        NDRX_PLATF_DIAG(NDRX_DIAG_PTHREAD_CREATE, errno, "br_netin_setup");
        EXFAIL_OUT(ret);
    }    
    
    M_init_ok=EXTRUE;
out:
    return ret;
}

/**
 * Ask for net-in thread to shutdown
 */
expublic void br_netin_shutdown(void)
{
    char b=EXTRUE;
    M_shutdown_req=EXTRUE;
    
    if (M_init_ok)
    {
        if (write(M_shutdownpipe[WRITE], &b, sizeof(b)) <= 0)
        {
            NDRX_LOG(log_error, "Failed to send shutdown notification: %s", 
                    strerror(errno));
        }
        pthread_join(M_netin_thread, NULL);
    }
    
}

/**
 * Task before going into poll
 */
exprivate int b4poll(void)
{
    /* stop processing if we are waiting on incoming queues... grown */
    br_chk_limit();
    
    return exnet_b4_poll_cb();
}


/**
 * Network thread entry
 * Loop over the incoming traffic and process it accordingly..
 * After this we can simplify the bridgesvc, so that it only performs
 * network-away sending....
 * And networking incoming may check the incoming service overflows
 * and that would not stop the bridge from sending away
 */
exprivate void * br_netin_run(void *arg)
{
    pollextension_rec_t *el;
    struct pollfd fds[MAX_POLL_FD]; /* if we have more, then we can crash, as really currenlty max is 2 conns */
    ndrx_stopwatch_t   periodic_cb;
    int i, j;
    int err, ret = EXSUCCEED;
    pollextension_rec_t *ext;
    
    NDRX_LOG(log_error, "br_netin_run starting...");
    
    
    /* Allocate network buffer */
    if (EXSUCCEED!=exnet_net_init(&G_bridge_cfg.net))
    {
        NDRX_LOG(log_error, "Failed to allocate data buffer!");
        EXFAIL_OUT(ret);
    }
    
    /* add custom pipe for shutdown? */
    
    if (EXSUCCEED!=tpinit(NULL))
    {
        NDRX_LOG(log_error, "Failed to tpinit() net-in thread - terminate");
        userlog("Failed to tpinit() net-in thread - terminate");
        EXFAIL_OUT(ret);
    }
    
    /* run simpler poller (as we have few socket only...) 
     * loop over the list of file descriptors and use for creating a poll
     * API
     */
    
    ndrx_stopwatch_reset(&periodic_cb);
    
    while (!M_shutdown_req)
    {
        /* before */
        if (EXSUCCEED!=b4poll())
        {
            NDRX_LOG(log_always, "Bridge b4poll failed - terminating!");
            userlog("Bridge b4poll failed - terminating!");
            EXFAIL_OUT(ret);
        }

        /* the list  
        loop over the G_pollext - get the count and allocate the buffer ... */
        i=0;
        DL_FOREACH(ndrx_G_pollext, el)
        {
            fds[i].fd = el->fd;
            fds[i].events = POLLIN|POLLERR;
            fds[i].revents=0;
            i++;
        }
        
        ret=poll(fds, i, G_bridge_cfg.check_interval*1000);
        
        if (EXFAIL==ret)
        {
            /* if timed out ... no problem.. */
            err=errno;
            NDRX_LOG(log_error, "in-in poll failed: %s - terminate", strerror(err));
            userlog("in-in poll failed: %s - terminate", strerror(err));
            EXFAIL_OUT(ret);
        }
        
        if (ndrx_stopwatch_get_delta_sec(&periodic_cb) >= G_bridge_cfg.check_interval)
        {
            if (EXSUCCEED!=exnet_periodic())
            {
                NDRX_LOG(log_always, "Bridge periodic check failed - terminating!");
                userlog("Bridge periodic check failed - terminating!");
                EXFAIL_OUT(ret);
            }
            
            ndrx_stopwatch_reset(&periodic_cb);
        }
        
        for (j=0; j<i; j++)
        {
            if (!fds[j].revents)
            {
                continue;
            }
            ext=ext_find_poller(fds[j].fd);
                
            if (NULL!=ext)
            {
                NDRX_LOG(log_info, "FD %d found in extension list, invoking", ext->fd);

                ret = ext->p_pollevent(fds[j].fd, fds[j].revents, ext->ptr1);
                
                if (EXSUCCEED!=ret)
                {
                    NDRX_LOG(log_error, "p_pollevent at 0x%lx failed (fd=%d)! - terminating",
                            ext->p_pollevent, ext->fd);
                    userlog("p_pollevent at 0x%lx failed (fd=%d)! - terminating",
                            ext->p_pollevent, ext->fd);
                    EXFAIL_OUT(ret);
                }
                else
                {
                    continue;
                }
            }
            else
            {
                NDRX_LOG(log_error, "NULL Extension callback - terminating");
                userlog("NULL Extension callback - terminating");
                EXFAIL_OUT(ret);
            }
        }
    }
    
out:
    tpterm();


    /* clean shutdown requested... */
    if (!M_shutdown_req && EXSUCCEED!=ret)
    {
        tpexit();
    }

    return NULL;
}

/* vim: set ts=4 sw=4 et smartindent: */
