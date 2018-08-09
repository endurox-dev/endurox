/**
 * @brief Dynamic advertise & unadvertise routines
 *
 * @file dynadv.c
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
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <utlist.h>
#include <string.h>
#include "srv_int.h"
#include "tperror.h"
#include <atmi_int.h>
#include <atmi_shm.h>
#include <sys_unix.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define READVERTISE_SLEEP_SRV       2   /* Sleep X sec for re-advertise */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * 1. Take a copy of service,
 * 2. un-advertise
 * 3. advertise + save time when q is open
 * 
 * This should be used in case when ndrxd removed the queue, un in same time
 * we did advertise!
 * 
 * @param svcname
 * @return 
 */
expublic int dynamic_readvertise(char *svcname)
{
    int ret=EXSUCCEED;
    svc_entry_fn_t *entry=NULL;
    char *fn="dynamic_readvertise";
    int found = EXFALSE;
    static int first = EXTRUE;
    int mod;
    
    NDRX_LOG(log_warn, "%s: enter, svcname = [%s]", fn, svcname);
    
    if (first)
    {
        first = EXFALSE;
        srand ( time(NULL) );
    }
    
    if ( (entry = (svc_entry_fn_t*)NDRX_MALLOC(sizeof(svc_entry_fn_t))) == NULL)
    {
            NDRX_LOG(log_error, "Failed to allocate %d bytes while parsing -s",
                                sizeof(svc_entry_fn_t));
            ret=EXFAIL;
            goto out;
    }
    
    memset(entry, 0, sizeof(*entry));
    
    /* Un-advertise it firstly */
    if (EXSUCCEED!=dynamic_unadvertise(svcname, &found, entry) || !found)
    {
        NDRX_LOG(log_error, "Failed to unadvertise: [%s]", svcname);
        ret=EXFAIL;
        goto out;
    }
    
    /* So that we do not get infinite loop, in case 
     * if many servers does restarts at the same time! 
     */
    mod = rand() % 4;
    /* Have some sleep, as limit */
    NDRX_LOG(log_warn, "Sleeping %d seconds for re-advertise!", 
            READVERTISE_SLEEP_SRV+mod);
    
    sleep(READVERTISE_SLEEP_SRV+mod);
    
    /* Now advertise back on! */
    if (EXSUCCEED!=dynamic_advertise(entry, svcname, entry->p_func, entry->fn_nm))
    {
        NDRX_LOG(log_error, "Failed to advertise: [%s]", svcname);
        ret=EXFAIL;
        goto out;
    }
    
out:

    if (EXSUCCEED!=ret && NULL!=entry)
        NDRX_FREE(entry);

    NDRX_LOG(log_warn, "%s: return, ret = %d", fn, ret);
    return ret;
}

/**
 * Dynamic unadvertise
 * @param svcname
 * @return 
 */
expublic int dynamic_unadvertise(char *svcname, int *found, svc_entry_fn_t *copy)
{
    int ret=EXSUCCEED;
    int pos;
    svc_entry_fn_t *ent=NULL;
    int service;
    int len;
    
    char *thisfn="_dynamic_unadvertise";
    
    for (pos=0; pos<G_server_conf.adv_service_count; pos++)
    {
        if (0==strcmp(svcname, G_server_conf.service_array[pos]->svc_nm))
        {
            ent = G_server_conf.service_array[pos];
            NDRX_LOG(log_warn, "Service [%s] found in array at %d", svcname, pos);
            break;
        }
    }
    
    if (ent)
    {
        /*Return some stuff back there usable by dynamic_readvertise*/
        if (NULL!=copy)
        {
            memcpy(copy, ent, sizeof(svc_entry_fn_t));
        }
        
        if (NULL!=found)
        {
            *found = EXTRUE;
        }
        
        NDRX_LOG(log_error, "Q File descriptor: %d - removing from polling struct", 
                ent->q_descr);
        
        if (EXFAIL==ndrx_epoll_ctl_mq(G_server_conf.epollfd, EX_EPOLL_CTL_DEL,
                            ent->q_descr, NULL))
        {
            ndrx_TPset_error_fmt(TPEOS, "ndrx_epoll_ctl failed to remove fd %d from epollfd: %s", 
                    ent->q_descr, ndrx_poll_strerror(ndrx_epoll_errno()));
            ret=EXFAIL;
            goto out;
        }
        
        /* Now close the FD */
        if (EXSUCCEED!=ndrx_mq_close(ent->q_descr))
        {
            ndrx_TPset_error_fmt(TPEOS, "ndrx_mq_close failed to close fd %d: %s", 
                    ent->q_descr, strerror(errno));
            ret=EXFAIL;
            goto out;
        }
        
        len = G_server_conf.adv_service_count;
        
        if (EXSUCCEED!=atmisrv_array_remove_element((void *)(G_server_conf.service_array), pos, 
                    len, sizeof(svc_entry_fn_t *)))
        {
            NDRX_LOG(log_error, "Failed to shift memory for "
                                "G_server_conf.service_array!");
            ret=EXFAIL;
            goto out;
        }
        /* Now reduce the memory usage */
        G_server_conf.service_array=NDRX_REALLOC(G_server_conf.service_array, 
                                (sizeof(svc_entry_fn_t *)*len-1));

        if (NULL==G_server_conf.service_array)
        {
            ndrx_TPset_error_fmt(TPEOS, "realloc failed: %s", strerror(errno));
            ret=EXFAIL;
            goto out;
        }
        
        /* Free up the memory used!?!  */
        NDRX_FREE(ent);
        ent=NULL;

        service = pos - ATMI_SRV_Q_ADJUST;
        if (EXSUCCEED!=atmisrv_array_remove_element((void *)G_shm_srv->svc_fail, service, 
                    MAX_SVC_PER_SVR, sizeof(*(G_shm_srv->svc_fail))))
        {
            NDRX_LOG(log_error, "Failed to shift memory for G_shm_srv->svc_succeed!");
            ret=EXFAIL;
            goto out;
        }
        
        if (EXSUCCEED!=unadvertse_to_ndrxd(svcname))
        {
            ret=EXFAIL;
            goto out;
        }
        
        G_server_conf.adv_service_count--;

        if (G_shm_srv)
        {
            /* Shift shared memory, adjust the stuff by: ATMI_SRV_Q_ADJUST*/
            if (EXSUCCEED!=atmisrv_array_remove_element((void *)(G_shm_srv->svc_succeed), service, 
                        MAX_SVC_PER_SVR, sizeof(*(G_shm_srv->svc_succeed))))
            {
                NDRX_LOG(log_error, "Failed to shift memory for G_shm_srv->svc_succeed!");
                ret=EXFAIL;
                goto out;
            }

            if (EXSUCCEED!=atmisrv_array_remove_element((void *)&(G_shm_srv->min_rsp_msec), 
                     service, MAX_SVC_PER_SVR, sizeof(*(G_shm_srv->min_rsp_msec))))
            {
                NDRX_LOG(log_error, "Failed to shift memory for "
                                                "G_shm_srv->min_rsp_msec!");
                ret=EXFAIL;
                goto out;
            }

            if (EXSUCCEED!=atmisrv_array_remove_element((void *)(G_shm_srv->max_rsp_msec), 
                        service, MAX_SVC_PER_SVR, sizeof(*(G_shm_srv->max_rsp_msec))))
            {
                NDRX_LOG(log_error, "Failed to shift memory for "
                                                "G_shm_srv->max_rsp_msec!");
                ret=EXFAIL;
                goto out;
            }

            if (EXSUCCEED!=atmisrv_array_remove_element((void *)(G_shm_srv->last_rsp_msec), 
                        service,  MAX_SVC_PER_SVR, sizeof(*(G_shm_srv->last_rsp_msec))))
            {
                NDRX_LOG(log_error, "Failed to shift memory for 1"
                                                "G_shm_srv->last_rsp_msec!");
                ret=EXFAIL;
                goto out;
            }

            if (EXSUCCEED!=atmisrv_array_remove_element((void *)&(G_shm_srv->svc_status), 
                        service, MAX_SVC_PER_SVR, sizeof(*(G_shm_srv->svc_status))))
            {
                NDRX_LOG(log_error, "Failed to shift memory for "
                                                "G_shm_srv->svc_status!");
                ret=EXFAIL;
                goto out;
            }
        }
    }
    else 
    {
        ndrx_TPset_error_fmt(TPENOENT, "%s: service [%s] not advertised", thisfn, svcname);
        ret=EXFAIL;
        goto out;
    }
    
out:
    return ret;
}

/**
 * We are going to dynamically advertise the service
 * @param svcname
 * @return 
 */
expublic int dynamic_advertise(svc_entry_fn_t *entry_new, 
                    char *svc_nm, void (*p_func)(TPSVCINFO *), char *fn_nm)
{
    int ret=EXSUCCEED;
    int pos, service;
    svc_entry_fn_t *entry_chk=NULL;
    struct ndrx_epoll_event ev;
    int sz;
    
    for (pos=0; pos<G_server_conf.adv_service_count; pos++)
    {
        if (0==strcmp(svc_nm, G_server_conf.service_array[pos]->svc_nm))
        {
            entry_chk = G_server_conf.service_array[pos];
            break;
        }
    }
    
    /* Check for advertise existance */
    if (NULL!=entry_chk)
    {
        NDRX_LOG(log_warn, "Service [%s] found in array at %d", 
                                svc_nm, pos);
        
        if (entry_chk->p_func == p_func)
        {
            NDRX_LOG(log_warn, "Advertised function ptr "
                                "the same - return OK!");
            goto out;
        }
        else
        {
            ndrx_TPset_error_fmt(TPEMATCH, "Service [%s] already advertised by func. "
                    "ptr. 0x%lx, but now requesting advertise by func. ptr. 0x%lx!",
                    svc_nm, entry_chk->p_func, p_func);
            ret=EXFAIL;
            goto out;
        }
    }
    
    /* Check the service count already in system! */
    if (G_server_conf.adv_service_count+1>MAX_SVC_PER_SVR)
    {
        ndrx_TPset_error_fmt(TPELIMIT, "Servce limit %d reached!", MAX_SVC_PER_SVR);
        ret=EXFAIL;
        goto out;
    }
    /* This will be the current, note that count is already lass then 1, 
     * and suites ok as index
     */
    service = G_server_conf.adv_service_count - ATMI_SRV_Q_ADJUST;
    
#ifdef EX_USE_POLL
    snprintf(entry_new->listen_q, sizeof(entry_new->listen_q), NDRX_SVC_QFMT_SRVID, 
            G_server_conf.q_prefix, entry_new->svc_nm, (short)G_server_conf.srv_id);
#else
    snprintf(entry_new->listen_q, sizeof(entry_new->listen_q), NDRX_SVC_QFMT, 
            G_server_conf.q_prefix, entry_new->svc_nm);
#endif
    
    /* We are good to go, open q? */
    
    NDRX_LOG(log_debug, "About to listen on: %s", entry_new->listen_q);
    
    /* ###################### CRITICAL SECTION ############################### */
    /* Acquire semaphore here.... */
    if (G_shm_srv && EXSUCCEED!=ndrx_lock_svc_op(__func__))
    {
        NDRX_LOG(log_error, "Failed to lock sempahore");
        ret=EXFAIL;
        goto out;
    }

    /* Open the queue */
    entry_new->q_descr = ndrx_mq_open_at (entry_new->listen_q, O_RDWR | O_CREAT | O_NONBLOCK, 
                                            S_IWUSR | S_IRUSR, NULL);
    /*
     * Check are we ok or failed?
     */
    if ((mqd_t)EXFAIL==entry_new->q_descr)
    {
        /* Release semaphore! */
         if (G_shm_srv) ndrx_unlock_svc_op(__func__);
         
        ndrx_TPset_error_fmt(TPEOS, "Failed to open queue: %s: %s",
                                    entry_new->listen_q, strerror(errno));
        ret=EXFAIL;
        goto out;
    }
    
    /* Register stuff in shared memory! */
    if (G_shm_srv)
    {
        ndrx_shm_install_svc(entry_new->svc_nm, 0, G_server_conf.srv_id);
    }
    
    /* Release semaphore! */
    if (G_shm_srv) ndrx_unlock_svc_op(__func__);
    /* ###################### CRITICAL SECTION, END ########################## */
    
    /* Save the time when stuff is open! */
    ndrx_stopwatch_reset(&entry_new->qopen_time);

    NDRX_LOG(log_debug, "Got file descriptor: %d, adding to e-poll!",
                            entry_new->q_descr);
    
    /* Put stuff in linear array */
    sz = sizeof(*G_server_conf.service_array)*(G_server_conf.adv_service_count+1);
    
    G_server_conf.service_array = NDRX_REALLOC(G_server_conf.service_array, sz);
    
    if (NULL==G_server_conf.service_array)
    {
        ndrx_TPset_error_fmt(TPEOS, "Failed to reallocate memory to %d bytes!", sz);
        ret=EXFAIL;
        goto out;
    }
    
    /* Fill up service array */
    G_server_conf.service_array[G_server_conf.adv_service_count] = entry_new;
            
    G_server_conf.adv_service_count++;
    
    memset(&ev, 0, sizeof(ev));
    ev.events = EX_EPOLL_FLAGS;
#ifdef EX_USE_EPOLL
    ev.data.fd = entry_new->q_descr;
#else
    ev.data.mqd = entry_new->q_descr;
#endif
    
    if (EXFAIL==ndrx_epoll_ctl_mq(G_server_conf.epollfd, EX_EPOLL_CTL_ADD,
                            entry_new->q_descr, &ev))
    {
        ndrx_TPset_error_fmt(TPEOS, "ndrx_epoll_ctl failed: %s", 
                ndrx_poll_strerror(ndrx_epoll_errno()));
        ret=EXFAIL;
        goto out;
    }
    
    /* Set shared memory to available! */
    G_shm_srv->svc_status[service] = NDRXD_SVC_STATUS_AVAIL;
    
    
    /* Send to NDRXD that we have are OK! */
    if (EXSUCCEED!=advertse_to_ndrxd(entry_new))
    {
        NDRX_LOG(log_error, "Failed to send advertise message to NDRXD!");
        ret=EXFAIL;
        goto out;
    }

out:
    return ret;
}
/* vim: set ts=4 sw=4 et smartindent: */
