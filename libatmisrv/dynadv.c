/* 
** Dynamic advertise & unadvertise routines
**
** @file dynadv.c
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
#include <errno.h>
#include <sys/stat.h>
#include <sys/epoll.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <utlist.h>
#include <string.h>
#include "srv_int.h"
#include "tperror.h"
#include <atmi_int.h>
#include <atmi_shm.h>

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
public int dynamic_readvertise(char *svcname)
{
    int ret=SUCCEED;
    svc_entry_fn_t *entry=NULL;
    char *fn="dynamic_readvertise";
    int found = FALSE;
    static int first = TRUE;
    int mod;
    
    NDRX_LOG(log_warn, "%s: enter, svcname = [%s]", fn, svcname);
    
    if (first)
    {
        first = FALSE;
        srand ( time(NULL) );
    }
    
    if ( (entry = (svc_entry_fn_t*)malloc(sizeof(svc_entry_fn_t))) == NULL)
    {
            NDRX_LOG(log_error, "Failed to allocate %d bytes while parsing -s",
                                sizeof(svc_entry_fn_t));
            ret=FAIL;
            goto out;
    }
    
    memset(entry, 0, sizeof(*entry));
    
    /* Un-advertise it firstly */
    if (SUCCEED!=dynamic_unadvertise(svcname, &found, entry) || !found)
    {
        NDRX_LOG(log_error, "Failed to unadvertise: [%s]", svcname);
        ret=FAIL;
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
    if (SUCCEED!=dynamic_advertise(entry, svcname, entry->p_func, entry->fn_nm))
    {
        NDRX_LOG(log_error, "Failed to advertise: [%s]", svcname);
        ret=FAIL;
        goto out;
    }
    
out:

    if (SUCCEED!=ret && NULL!=entry)
        free(entry);

    NDRX_LOG(log_warn, "%s: return, ret = %d", fn, ret);
    return ret;
}

/**
 * Dynamic unadvertise
 * @param svcname
 * @return 
 */
public int dynamic_unadvertise(char *svcname, int *found, svc_entry_fn_t *copy)
{
    int ret=SUCCEED;
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
            *found = TRUE;
        }
        
        NDRX_LOG(log_error, "Q File descriptor: %d - removing from polling struct", 
                ent->q_descr);
        
        if (FAIL==epoll_ctl(G_server_conf.epollfd, EPOLL_CTL_DEL,
                            ent->q_descr, NULL))
        {
            _TPset_error_fmt(TPEOS, "epoll_ctl failed to remove fd %d from epollfd: %s", 
                    ent->q_descr, strerror(errno));
            ret=FAIL;
            goto out;
        }
        
        /* Now close the FD */
        if (SUCCEED!=mq_close(ent->q_descr))
        {
            _TPset_error_fmt(TPEOS, "mq_close failed to close fd %d: %s", 
                    ent->q_descr, strerror(errno));
            ret=FAIL;
            goto out;
        }
        
        len = G_server_conf.adv_service_count;
        
        if (SUCCEED!=array_remove_element((void *)(G_server_conf.service_array), pos, 
                    len, sizeof(svc_entry_fn_t *)))
        {
            NDRX_LOG(log_error, "Failed to shift memory for "
                                "G_server_conf.service_array!");
            ret=FAIL;
            goto out;
        }
        /* Now reduce the memory usage */
        G_server_conf.service_array=realloc(G_server_conf.service_array, 
                                (sizeof(svc_entry_fn_t *)*len-1));

        if (NULL==G_server_conf.service_array)
        {
            _TPset_error_fmt(TPEOS, "realloc failed: %s", strerror(errno));
            ret=FAIL;
            goto out;
        }
        
        /* Free up the memory used!?!  */
        free(ent);
        ent=NULL;

        service = pos - ATMI_SRV_Q_ADJUST;
        if (SUCCEED!=array_remove_element((void *)G_shm_srv->svc_fail, service, 
                    MAX_SVC_PER_SVR, sizeof(*(G_shm_srv->svc_fail))))
        {
            NDRX_LOG(log_error, "Failed to shift memory for G_shm_srv->svc_succeed!");
            ret=FAIL;
            goto out;
        }
        
        if (SUCCEED!=unadvertse_to_ndrxd(svcname))
        {
            ret=FAIL;
            goto out;
        }
        
        G_server_conf.adv_service_count--;

        if (G_shm_srv)
        {
            /* Shift shared memory, adjust the stuff by: ATMI_SRV_Q_ADJUST*/
            if (SUCCEED!=array_remove_element((void *)(G_shm_srv->svc_succeed), service, 
                        MAX_SVC_PER_SVR, sizeof(*(G_shm_srv->svc_succeed))))
            {
                NDRX_LOG(log_error, "Failed to shift memory for G_shm_srv->svc_succeed!");
                ret=FAIL;
                goto out;
            }

            if (SUCCEED!=array_remove_element((void *)&(G_shm_srv->min_rsp_msec), 
                     service, MAX_SVC_PER_SVR, sizeof(*(G_shm_srv->min_rsp_msec))))
            {
                NDRX_LOG(log_error, "Failed to shift memory for "
                                                "G_shm_srv->min_rsp_msec!");
                ret=FAIL;
                goto out;
            }

            if (SUCCEED!=array_remove_element((void *)(G_shm_srv->max_rsp_msec), 
                        service, MAX_SVC_PER_SVR, sizeof(*(G_shm_srv->max_rsp_msec))))
            {
                NDRX_LOG(log_error, "Failed to shift memory for "
                                                "G_shm_srv->max_rsp_msec!");
                ret=FAIL;
                goto out;
            }

            if (SUCCEED!=array_remove_element((void *)(G_shm_srv->last_rsp_msec), 
                        service,  MAX_SVC_PER_SVR, sizeof(*(G_shm_srv->last_rsp_msec))))
            {
                NDRX_LOG(log_error, "Failed to shift memory for 1"
                                                "G_shm_srv->last_rsp_msec!");
                ret=FAIL;
                goto out;
            }

            if (SUCCEED!=array_remove_element((void *)&(G_shm_srv->svc_status), 
                        service, MAX_SVC_PER_SVR, sizeof(*(G_shm_srv->svc_status))))
            {
                NDRX_LOG(log_error, "Failed to shift memory for "
                                                "G_shm_srv->svc_status!");
                ret=FAIL;
                goto out;
            }
        }
    }
    else 
    {
        _TPset_error_fmt(TPENOENT, "%s: service [%s] not advertised", thisfn, svcname);
        ret=FAIL;
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
public int	dynamic_advertise(svc_entry_fn_t *entry_new, 
                    char *svc_nm, void (*p_func)(TPSVCINFO *), char *fn_nm)
{
    int ret=SUCCEED;
    int pos, service;
    svc_entry_fn_t *entry_chk=NULL;
    struct epoll_event ev;
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
            _TPset_error_fmt(TPEMATCH, "Service [%s] already advertised by func. "
                    "ptr. 0x%lx, but now requesting advertise by func. ptr. 0x%lx!",
                    svc_nm, entry_chk->p_func, p_func);
            ret=FAIL;
            goto out;
        }
    }
    
    /* Check the service count already in system! */
    if (G_server_conf.adv_service_count+1>MAX_SVC_PER_SVR)
    {
        _TPset_error_fmt(TPELIMIT, "Servce limit %d reached!", MAX_SVC_PER_SVR);
        ret=FAIL;
        goto out;
    }
    /* This will be the current, note that count is already lass then 1, 
     * and suites ok as index
     */
    service = G_server_conf.adv_service_count - ATMI_SRV_Q_ADJUST;
    
    sprintf(entry_new->listen_q, NDRX_SVC_QFMT, G_server_conf.q_prefix, entry_new->svc_nm);
    
    /* We are good to go, open q? */
    
    NDRX_LOG(log_debug, "About to listen on: %s", entry_new->listen_q);
    
    /* ###################### CRITICAL SECTION ############################### */
    /* Acquire semaphore here.... */
    if (G_shm_srv && SUCCEED!=ndrx_lock_svc_op())
    {
        NDRX_LOG(log_error, "Failed to lock sempahore");
        ret=FAIL;
        goto out;
    }

    /* Open the queue */
    entry_new->q_descr = ndrx_mq_open_at (entry_new->listen_q, O_RDWR | O_CREAT | O_NONBLOCK, 
                                            S_IWUSR | S_IRUSR, NULL);
    /*
     * Check are we ok or failed?
     */
    if (FAIL==entry_new->q_descr)
    {
        /* Release semaphore! */
         if (G_shm_srv) ndrx_unlock_svc_op();
         
        _TPset_error_fmt(TPEOS, "Failed to open queue: %s: %s",
                                    entry_new->listen_q, strerror(errno));
        ret=FAIL;
        goto out;
    }
    
    /* Register stuff in shared memory! */
    if (G_shm_srv)
    {
        ndrx_shm_install_svc(entry_new->svc_nm, 0);
    }
    
    /* Release semaphore! */
    if (G_shm_srv) ndrx_unlock_svc_op();
    /* ###################### CRITICAL SECTION, END ########################## */
    
    /* Save the time when stuff is open! */
    n_timer_reset(&entry_new->qopen_time);

    NDRX_LOG(log_debug, "Got file descriptor: %d, adding to e-poll!",
                            entry_new->q_descr);
    
    /* Put stuff in linear array */
    sz = sizeof(*G_server_conf.service_array)*(G_server_conf.adv_service_count+1);
    
    G_server_conf.service_array = realloc(G_server_conf.service_array, sz);
    
    if (NULL==G_server_conf.service_array)
    {
        _TPset_error_fmt(TPEOS, "Failed to reallocate memory to %d bytes!", sz);
        ret=FAIL;
        goto out;
    }
    
    /* Fill up service array */
    G_server_conf.service_array[G_server_conf.adv_service_count] = entry_new;
            
    G_server_conf.adv_service_count++;
    
    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN;
    ev.data.fd = entry_new->q_descr;
    
    if (FAIL==epoll_ctl(G_server_conf.epollfd, EPOLL_CTL_ADD,
                            entry_new->q_descr, &ev))
    {
        _TPset_error_fmt(TPEOS, "epoll_ctl failed: %s", strerror(errno));
        ret=FAIL;
        goto out;
    }
    
    /* Set shared memory to available! */
    G_shm_srv->svc_status[service] = NDRXD_SVC_STATUS_AVAIL;
    
    
    /* Send to NDRXD that we have are OK! */
    if (SUCCEED!=advertse_to_ndrxd(entry_new))
    {
        NDRX_LOG(log_error, "Failed to send advertise message to NDRXD!");
        ret=FAIL;
        goto out;
    }

out:
    return ret;
}
