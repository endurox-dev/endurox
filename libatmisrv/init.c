/* 
 * @brief Enduro/X server API functions
 *
 * @file init.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2018, Mavimax, Ltd. All Rights Reserved.
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
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <utlist.h>
#include <string.h>
#include "srv_int.h"
#include "tperror.h"
#include "atmi_tls.h"
#include <atmi_int.h>
#include <atmi_shm.h>
#include <xa_cmn.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
expublic shm_srvinfo_t *G_shm_srv = NULL;    /* ptr to shared memory block of the server */
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Function for checking service function existance
 * @param a
 * @param b
 * @return
 */
exprivate int svc_entry_fn_cmp(svc_entry_fn_t *a, svc_entry_fn_t *b)
{
    return strcmp(a->svc_nm,b->svc_nm);
}

/**
 * This resolves service entry 
 * @param svc
 * @return
 */
exprivate svc_entry_fn_t* resolve_service_entry(char *svc)
{
    svc_entry_fn_t *ret=NULL, eltmp;

    if (NULL!=svc)
    {
        NDRX_STRCPY_SAFE(eltmp.svc_nm, svc);
        DL_SEARCH(G_server_conf.service_raw_list, ret, &eltmp, svc_entry_fn_cmp);
    }

    return ret;
}

/**
 * Build final list of services to be advertised
 * @param svn_nm_srch
 * @param svn_nm_add
 * @param resolved
 * @return
 */
exprivate int sys_advertise_service(char *svn_nm_srch, char *svn_nm_add, svc_entry_fn_t *resolved)
{
    int ret=EXSUCCEED;
    
    svc_entry_fn_t *svc_fn, *entry;

    /* resolve alias & add svc entry */
    if (NULL==resolved)
    {
        svc_fn=resolve_service_entry(svn_nm_srch);
    }
    else
    {
        svc_fn=resolved; /* already resolved. */
    }

    if (NULL==svc_fn)
    {
        ndrx_TPset_error_fmt(TPENOENT, "There is no entry for [%s] [%s]",
                        svn_nm_srch, svn_nm_add);
        ret=EXFAIL;
    }
    else
    {
        /* OK register that stuff */
        /* do the malloc! */
        if ( (entry = (svc_entry_fn_t*)NDRX_MALLOC(sizeof(svc_entry_fn_t))) == NULL)
        {
            NDRX_LOG(log_error, "Failed to allocate %d bytes for service entry",
                                            sizeof(svc_entry_fn_t));

            ndrx_TPset_error_fmt(TPEOS, "Failed to allocate %d bytes for service entry",
                                sizeof(svc_entry_fn_t));
            ret=EXFAIL;
        }
        else
        {
            /* Fill up the details & let it run! */
            memcpy(entry, svc_fn, sizeof(svc_entry_fn_t));
            /* Set service name */
            NDRX_STRCPY_SAFE(entry->svc_nm, svn_nm_add);
            
            /* Set queue on which to listen */
#ifdef EX_USE_POLL
            snprintf(entry->listen_q, sizeof(entry->listen_q), NDRX_SVC_QFMT_SRVID, 
                    G_server_conf.q_prefix, 
                    entry->svc_nm, (short)G_server_conf.srv_id);
#else
            snprintf(entry->listen_q, sizeof(entry->listen_q), NDRX_SVC_QFMT, 
                    G_server_conf.q_prefix, entry->svc_nm);
#endif

            /* Add to list! */
            DL_APPEND(G_server_conf.service_list, entry);
            G_server_conf.adv_service_count++;
            NDRX_LOG(log_debug, "Advertising: SVC: [%s] FN: [%s] ADDR: [%p] QUEUE: [%s]",
                        entry->svc_nm, entry->fn_nm, entry->p_func, entry->listen_q);
        }
    }

    return ret;
}

/**
 * Builds linear array
 * @return
 */
exprivate int build_service_array_list(void)
{
    int ret=EXSUCCEED;
    int i=0;
    svc_entry_fn_t *f_tmp, *f_el;
            
    if (G_server_conf.service_array!=NULL)
        NDRX_FREE(G_server_conf.service_array);

    NDRX_LOG(log_debug, "about to allocate %d of svc ptrs", G_server_conf.adv_service_count);

    G_server_conf.service_array = NDRX_CALLOC(sizeof(svc_entry_fn_t *), G_server_conf.adv_service_count);
    
    if (NULL==G_server_conf.service_array)
    {
        ndrx_TPset_error_fmt(TPEOS, "Failed to allocate: %s", strerror(errno));
        ret=EXFAIL;
    }
    else
    {
        DL_FOREACH_SAFE(G_server_conf.service_list, f_el,f_tmp)
        {
            NDRX_LOG(log_debug, "assigning %d", i);
            G_server_conf.service_array[i] = f_el;
            i++;
        }
    }
    
    return ret;
}

/**
 * Add system (Enduro/X) internals specific queue 
 * @param qname queue name
 * @param is_admin is admin, else it is reply q
 * @return EXSUCCEED/EXFAIL
 */
exprivate int add_specific_queue(char *qname, int is_admin)
{
    int ret=EXSUCCEED;

    svc_entry_fn_t *entry;
    /* phase 0. Generate admin q */
    if ( (entry = (svc_entry_fn_t*)NDRX_CALLOC(1, sizeof(svc_entry_fn_t))) == NULL)
    {
        NDRX_LOG(log_error, "Failed to allocate %d bytes for admin service entry",
                                        sizeof(svc_entry_fn_t));

        ndrx_TPset_error_fmt(TPEOS, "Failed to allocate %d bytes for admin service entry",
                            sizeof(svc_entry_fn_t));
    }
    else
    {
        entry->q_descr=(mqd_t)EXFAIL;
        entry->p_func=NULL;
        entry->is_admin = is_admin;
        NDRX_STRCPY_SAFE(entry->listen_q, qname);
        /*
        sprintf(entry->listen_q, NDRX_ADMIN_FMT, G_server_conf.q_prefix,
                                G_server_conf.binary_name, G_server_conf.srv_id);
        sprintf(entry->svc_nm, NDRX_ADMIN_SVC, G_server_conf.binary_name, G_server_conf.srv_id);
        strcpy(entry->fn_nm, entry->svc_nm);*/
        /* register admin service */
        DL_APPEND(G_server_conf.service_list, entry);
        G_server_conf.adv_service_count++;
        NDRX_LOG(log_debug, "Advertising: SVC: [%s] FN: [%s] ADDR: [%p] QUEUE: [%s]",
                        entry->svc_nm, entry->fn_nm, entry->p_func, entry->listen_q);
    }

    return ret;
}

/**
 * Functions builds final list what needs to be actually advertised to EX system
 * See description of tpadvertise_full() for -A & -s logic
 *
 * We have some few specific queues:
 * 0 - admin queue;
 * 1 - reply queue;
 * 
 * @return
 */
expublic int atmisrv_build_advertise_list(void)
{
    int ret=EXSUCCEED;
    svc_entry_t *s_tmp, *s_el;
    svc_entry_fn_t *f_tmp, *f_el;
    pid_t mypid = getpid();

    char *svn_nm_srch=NULL;
    char *svn_nm_add=NULL;
    
    char adminq[NDRX_MAX_Q_SIZE+1];
    char replyq[NDRX_MAX_Q_SIZE+1];

    /* Server admin queue */
    snprintf(adminq, sizeof(adminq), NDRX_ADMIN_FMT, G_server_conf.q_prefix,
                                G_server_conf.binary_name, 
                                G_server_conf.srv_id, mypid);
    ret=add_specific_queue(adminq, 1);
    
    if (EXFAIL==ret)
        goto out;

    /* Server reply queue */
    snprintf(replyq, sizeof(replyq), NDRX_SVR_QREPLY, G_server_conf.q_prefix,
                                    G_server_conf.binary_name, 
                                    G_server_conf.srv_id, mypid);
    
    ret=add_specific_queue(replyq, 0);
    if (EXFAIL==ret)
        goto out;

    /* phase 1. check all command line specified */
    DL_FOREACH_SAFE(G_server_conf.svc_list, s_el, s_tmp)
    {
        /* In this case we are only interested in aliases */
        if (EXEOS!=s_el->svc_alias[0])
        {
            svn_nm_srch=s_el->svc_alias;
            svn_nm_add=s_el->svc_nm;
        }
        else if (!G_server_conf.advertise_all)
        {
            svn_nm_srch=s_el->svc_nm;
            svn_nm_add=s_el->svc_nm;
        }

        if (EXSUCCEED!=(ret=sys_advertise_service(svn_nm_srch, svn_nm_add, NULL)))
        {
            NDRX_LOG(log_error, "Phase 1 advertise FAIL!");
            goto out;
        }

    }
    
    /* phase 2. advertise all, that we have from tpadvertise 
     * (only in case if we have -A) 
     */
    if (G_server_conf.advertise_all)
    {
        DL_FOREACH_SAFE(G_server_conf.service_raw_list,f_el,f_tmp)
        {
            
            /* check that this service isn't masked out for advertise */
            
            if (ndrx_skipsvc_chk(f_el->svc_nm))
            {
                NDRX_LOG(log_info, "%s masked by -n - not advertising", 
                        f_el->svc_nm);
                continue;
            }
            
            svn_nm_srch=f_el->svc_nm;
            svn_nm_add=f_el->svc_nm;

            if (EXSUCCEED!=(ret=sys_advertise_service(svn_nm_srch, 
                    svn_nm_add, NULL)))
            {
                NDRX_LOG(log_error, "Phase 2 advertise FAIL!");
                goto out;
            }
        }
    }
    
    ret=build_service_array_list();

out:

    return ret;
}

/**
 * Initialize common ATMI library
 * @return SUCCED/FAIL
 */
expublic int atmisrv_initialise_atmi_library(void)
{
    int ret=EXSUCCEED;
    atmi_lib_conf_t conf;
    pid_t pid = getpid();
    memset(&conf, 0, sizeof(conf));

    /* Generate my_id */
    snprintf(conf.my_id, sizeof(conf.my_id), NDRX_MY_ID_SRV, 
            G_server_conf.binary_name, 
            G_server_conf.srv_id, pid, 
            G_atmi_tls->G_atmi_conf.contextid, 
            G_atmi_env.our_nodeid);
    
    conf.is_client = 0;
    
    /*
    conf.reply_q = G_server_conf.service_array[1]->q_descr;
    strcpy(conf.reply_q_str, G_server_conf.service_array[1]->listen_q);
    */
    
    NDRX_STRCPY_SAFE(conf.q_prefix, G_server_conf.q_prefix);
    if (EXSUCCEED!=(ret=tp_internal_init(&conf)))
    {
        goto out;
    }
    
    /* Try to open shm... */
    G_shm_srv = ndrxd_shm_getsrv(G_srv_id);
    /* Mark stuff as used! */
    if (NULL!=G_shm_srv)
    {
        G_shm_srv->srvid = G_srv_id;  
    }
out:
    return ret;
}

/**
 * Un-initialize all stuff
 * @return void
 */
expublic void atmisrv_un_initialize(int fork_uninit)
{
    int i;
    atmi_tls_t *tls;
    /* We should close the queues and detach shared memory!
     * Also we will not remove service queues, because we do not
     * what other instances do. This is up to ndrxd!
     */
    if (NULL!=G_server_conf.service_array)
    {
        for (i=0; i<G_server_conf.adv_service_count; i++)
        {

            if (NULL==G_server_conf.service_array[i])
            {
                /* nothing to do if NULL */
                continue;
            }

            /* just close it, no error check */
            if(((mqd_t)EXFAIL)!=G_server_conf.service_array[i]->q_descr &&
			EXSUCCEED!=ndrx_mq_close(G_server_conf.service_array[i]->q_descr))
            {

                NDRX_LOG(log_error, "Failed to close q descr %d: %d/%s",
                                            G_server_conf.service_array[i]->q_descr,
                                            errno, strerror(errno));
            }

            if (!fork_uninit && (ATMI_SRV_ADMIN_Q==i || ATMI_SRV_REPLY_Q==i))
            {
                NDRX_LOG(log_debug, "Removing queue: %s",
                                    G_server_conf.service_array[i]->listen_q);

                if (EXSUCCEED!=ndrx_mq_unlink(G_server_conf.service_array[i]->listen_q))
                {
                    NDRX_LOG(log_error, "Failed to remove queue %s: %d/%s",
                                            G_server_conf.service_array[i]->listen_q,
                                            errno, strerror(errno));
                }
            }
        }/* for */
    }

    /* Now detach shared memory block */
    ndrxd_shm_close_all();

    /* close poller */
    NDRX_LOG(log_debug, "epollfd = %d", G_server_conf.epollfd);
    if (G_server_conf.epollfd > 0)
    {
        ndrx_epoll_close(G_server_conf.epollfd);
        G_server_conf.epollfd = 0;
    }

    if (NULL != G_server_conf.events)
    {
        NDRX_FREE((char *)G_server_conf.events);
    }
    
    /* close XA if was open.. */
    atmi_xa_uninit();
    ndrx_skipsvc_delhash();
    /* Mark our environment as un-initialized 
     * In external main() function cases, they might want to reuse the ATMI
     * context, and it is not server any more.
     */
    
    tls = ndrx_atmi_tls_get(0);
    ndrx_atmi_tls_new(tls, tls->is_auto, EXTRUE);
}

/* ===========================================================================*/
/* =========================API FUNCTIONS=====================================*/
/* ===========================================================================*/
/**
 * Advertize service
 * OK, logic will be following:
 * -A advertise all services + additional (aliases) by -s
 * if -A missing, then advertise all
 * if -A missing, -s specified, then advertise those by -s only
 * @return SUCCEED/FAIL
 */
expublic int tpadvertise_full(char *svc_nm, void (*p_func)(TPSVCINFO *), char *fn_nm)
{
    int ret=EXSUCCEED;
    svc_entry_fn_t *entry=NULL, eltmp;
    
    ndrx_TPunset_error();

    /* allocate memory for entry */
    if ( (entry = (svc_entry_fn_t*)NDRX_CALLOC(1, sizeof(svc_entry_fn_t))) == NULL)
    {
            ndrx_TPset_error_fmt(TPEOS, "Failed to allocate %d bytes while parsing -s",
                                sizeof(svc_entry_fn_t));
            ret=EXFAIL;
            goto out;
    }
    else
    {
        svc_entry_fn_t *existing=NULL;
        /* fill entry details */
        NDRX_STRNCPY(entry->svc_nm, svc_nm, XATMI_SERVICE_NAME_LENGTH);
        entry->svc_nm[XATMI_SERVICE_NAME_LENGTH]=EXEOS;
        NDRX_STRNCPY(entry->fn_nm, fn_nm, XATMI_SERVICE_NAME_LENGTH);
        /* At this point we need to check the convert flags... */
        entry->xcvtflags = xcvt_lookup(entry->fn_nm);
        
        entry->fn_nm[XATMI_SERVICE_NAME_LENGTH]=EXEOS;
        entry->p_func = p_func;
        entry->q_descr = (mqd_t)EXFAIL;
        
        /* search for existing entry */
        NDRX_STRCPY_SAFE(eltmp.svc_nm, entry->svc_nm);

        if (NULL==G_server_conf.service_array)
        {
            DL_SEARCH(G_server_conf.service_raw_list, existing, &eltmp, svc_entry_fn_cmp);

            if (existing)
            {
                /* OK, we have in list. So print warning and remove old one! */
                if (existing->p_func==p_func)
                {
                    NDRX_LOG(log_warn, "Service with name [%s] already "
                                    "advertised, same function - IGNORE!", svc_nm);
                }
                else
                {
                    /* We have error! */
                    NDRX_LOG(log_error, "ERROR: Service with name [%s] "
                                        "already advertised, "
                                        "but pointing to different "
                                        "function - FAIL", svc_nm);
                    ndrx_TPset_error_fmt(TPEMATCH, "ERROR: Service with name [%s] "
                                        "already advertised, "
                                        "but pointing to different function - "
                                        "FAIL", svc_nm);
                    ret=EXFAIL;
                }
                NDRX_FREE(entry);
            }
            else
            {
                    /* register the function */
                    NDRX_LOG(log_debug, "Service [%s] "
                                            "(function: [%s]:%p) successfully "
                                            "acknowledged",
                                            entry->svc_nm, entry->fn_nm, entry->p_func);
                    DL_APPEND(G_server_conf.service_raw_list, entry);
            }
        }
        else
        {
            NDRX_LOG(log_warn, "Processing dynamic advertise");
            if (EXFAIL==dynamic_advertise(entry, svc_nm, p_func, fn_nm))
            {
                ret=EXFAIL;
                NDRX_FREE(entry);
                goto out;
            }
            
        }
    }
    
out:
    return ret;
}

/**
 * Unadvertises service.
 * This will process only last request. All others will expire as queue
 * Possibly will be removed!
 * @param svcname
 * @return 
 */
expublic int tpunadvertise(char *svcname)
{
    int ret=EXSUCCEED;
    char svc_nm[XATMI_SERVICE_NAME_LENGTH+1] = {EXEOS};
    svc_entry_fn_t eltmp;
    svc_entry_fn_t *existing=NULL;
    char *thisfn="tpunadvertise";
    
    ndrx_TPunset_error();
    
    /* Validate argument */
    if (NULL==svcname || EXEOS==svcname[0])
    {
        ndrx_TPset_error_fmt(TPEINVAL, "%s: invalid svcname empty or null!", thisfn);
        ret=EXFAIL;
        goto out;
    }
    
    /* Crosscheck buffer. */
    NDRX_STRNCPY(svc_nm, svcname, XATMI_SERVICE_NAME_LENGTH);
    svc_nm[XATMI_SERVICE_NAME_LENGTH] = EXEOS;
    
    
    /* Search for service entry */
    NDRX_STRCPY_SAFE(eltmp.svc_nm, svc_nm);

    if (NULL==G_server_conf.service_array)
    {
        DL_SEARCH(G_server_conf.service_raw_list, existing, &eltmp, svc_entry_fn_cmp);

        if (existing)
        {
            NDRX_LOG(log_debug, "in server init stage - simply remove from array service");
            DL_DELETE(G_server_conf.service_raw_list, existing);
        }
        else
        {
            /* Need to inform server about changes + reconfigure polling 
            * Firstly we re-configure polling, then delete allocated structs
            * then send info to server.
            */
            ndrx_TPset_error_fmt(TPENOENT, "%s: service [%s] not advertised", 
                    thisfn, svc_nm);
            ret=EXFAIL;
            goto out;
        }
    }
    else
    {
        if (EXSUCCEED!=dynamic_unadvertise(svcname, NULL, NULL))
        {
            ret=EXFAIL;
            goto out;
        }
    }
    
out:
    return ret;
}


/**
 * Removes array element & rsizes the block
 * @param arr
 * @param elem - zero based elem number to be removed.
 * @param len
 * @param sz
 * @return 
 */
expublic int atmisrv_array_remove_element(void *arr, int elem, int len, int sz)
{
    int ret=EXSUCCEED;
    
    if (elem<len-1)
    {
	/*NDRX_DUMP(log_error, "Before mem move", arr, len*sz);*/
        memmove(arr+elem*sz, arr+(elem+1)*sz, (len-elem-1)*sz);
	/*NDRX_DUMP(log_error, "After mem move", arr, len*sz);*/
        /* Clear up last element we removed. */
        memset(arr+(len-1)*sz, 0, sz);
	/*NDRX_DUMP(log_error, "After mem set", arr, len*sz);*/
    }
    
out:
    return ret;
}
