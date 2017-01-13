/* 
** This file contains routines for bridge/cluster support.
** Basically if server tells about it self that it is bridge, we add it to
** Linked list & hash (just like we do for process mode server ids...!).
** After that by linked list we can send advertise to
**
** @file bridge.c
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <libxml/xmlreader.h>
#include <errno.h>

#include <ndrstandard.h>
#include <ndrxd.h>

#include <ndebug.h>
#include <nstdutil.h>
#include <exhash.h>
#include <bridge_int.h>
#include <ntimer.h>
#include <cmd_processor.h>
#include <atmi_shm.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
bridgedef_t *G_bridge_hash = NULL; /* Hash table of bridges */
bridgedef_svcs_t *G_bridge_svc_hash = NULL; /* Our full list of local services! */

bridgedef_svcs_t *G_bridge_svc_diff= NULL; /* Service diff to be sent to nodes, 
                                            * procssed before G_bridge_svc_hash, 
                                            * because we need the point from what 
                                            * to make a diff */
private int M_build_diff = FALSE;           /* Diff mode enabled        */
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/


/**
 * Remove bridge service hash & remove stuff from shared mem.
 * @param 
 */
public void brd_remove_bridge_services(bridgedef_t *cur)
{
    bridgedef_svcs_t *r = NULL;
    bridgedef_svcs_t *rtmp = NULL;
    
    EXHASH_ITER(hh, cur->theyr_services, r, rtmp)
    {
        EXHASH_DEL(cur->theyr_services, r);
        /* Remove stuff from shared mem. */
        brd_lock_and_update_shm(cur->nodeid, r->svc_nm, 0, BRIDGE_REFRESH_MODE_FULL);
        NDRX_FREE(r);
    }
    
}

/**
 * Remove bridge from hash.
 * @param nodeid
 * @return 
 */
public int brd_del_bridge(int nodeid)
{
    bridgedef_t* cur = brd_get_bridge(nodeid);
    
    if (NULL==cur)
    {
        NDRX_LOG(log_error, "Failed to remove bridge %d - "
                "node not found", nodeid);
        return FAIL;
    }
    else
    {
        NDRX_LOG(log_debug, "Removing bridge %d", nodeid);
        brd_remove_bridge_services(cur);
        EXHASH_DEL(G_bridge_hash, cur);
        NDRX_FREE(cur);
    }
    
    return SUCCEED;
}

/**
 * Add bridge server....
 * @param srvinfo
 * @return 
 */
public int brd_addupd_bridge(srv_status_t * srvinfo)
{
    int ret=SUCCEED;
    char *fn = "brd_addupd_bridge";
    bridgedef_t *cur;
    int add=FALSE;
    if (NULL!=(cur=brd_get_bridge(srvinfo->srvinfo.nodeid)))
    {
        NDRX_LOG(log_warn, "Bridge %d already exists - updating", 
                 srvinfo->srvinfo.nodeid);
    }
    else
    {
        NDRX_LOG(log_warn, "Bridge %d does not exists - registering", 
                 srvinfo->srvinfo.nodeid);
        cur = NDRX_CALLOC(1, sizeof(bridgedef_t));
        if (NULL==cur)
        {
            
            NDRX_LOG(log_error, "Failed to allocate %d bytes: %s", 
                    sizeof(bridgedef_t), strerror(errno));
            FAIL_OUT(ret);
        }
        
        cur->nodeid = srvinfo->srvinfo.nodeid;
        add=TRUE;
    }
    
    cur->srvid = srvinfo->srvinfo.srvid;
    cur->flags = srvinfo->srvinfo.flags;
    
    if (add)
    {
        /* here nodeid is hash field: */
        EXHASH_ADD_INT(G_bridge_hash, nodeid, cur);
    }
    
out:

    NDRX_LOG(log_error, "%s return %d", fn, ret);

    return ret;
}

/**
 * Get bridge node (find in hash)
 * @param nodeid - node id.
 * @return 
 */
public bridgedef_t* brd_get_bridge(int nodeid)
{
    bridgedef_t *ret = NULL;
    
    EXHASH_FIND_INT(G_bridge_hash, &nodeid, ret);
    
    return ret;
}

/**
 * Build full refresh message.
 * Buffer size should match the max ATMI msg size.
 * @param buf where to put the message.
 * @return 
 */
public int brd_build_refresh_msg(bridgedef_svcs_t *svcs, 
                bridge_refresh_t *ref, char mode)
{
    bridgedef_svcs_t *r = NULL;
    bridgedef_svcs_t *rtmp = NULL;
    
    /* This is global.
     * Thing is that on full diffs other side might want to search for services
     * what they actually do not have!
     */
    ref->mode = mode;
    
    EXHASH_ITER(hh, svcs, r, rtmp)
    {
        ref->svcs[ref->count].count = r->count;
        strcpy(ref->svcs[ref->count].svc_nm, r->svc_nm);
        ref->svcs[ref->count].mode = mode;
        
        NDRX_LOG(log_debug, "Built refresh line: count: %d svc_nm: [%s] mode: %c",
                ref->svcs[ref->count].count,
                ref->svcs[ref->count].svc_nm,
                ref->svcs[ref->count].mode);
        ref->count++;
    }
    
    return SUCCEED;
}

/**
 * Send refresh to node
 * @param refresh
 * @return 
 */
public int brd_send_update(int nodeid, bridgedef_t *cur, bridge_refresh_t *refresh)
{
    int ret=SUCCEED;
    int send_size;
    bridgedef_t *br = NULL;
    pm_node_t *srv = NULL;
    char *fn = "brd_send_update";
    
    char buf[ATMI_MSG_MAX_SIZE];
    bridge_refresh_t *tmp_refresh= (bridge_refresh_t *)buf;
    /* default use all */
    bridge_refresh_t *p_refresh = refresh;
    
    if ( NULL!=cur )
    {
        br = cur;
    }
    else
    {
        br = brd_get_bridge(nodeid);
    }
    
    if (NULL==br)
    {
        NDRX_LOG(log_error, "Invalid nodeid %d!!!", nodeid);
        ret=FAIL;
        goto out;
    }
    
    srv = get_pm_from_srvid(br->srvid);
    
    if (NULL==srv)
    {
        NDRX_LOG(log_error, "Invalid server id %d!!!", br->srvid);
        ret=FAIL;
        goto out;
    }
    
    /* Check is per server filter enabled? */
    if (EOS!=srv->conf->exportsvcs[0] || EOS!=srv->conf->blacklistsvcs[0])
    {
        int i;
        NDRX_LOG(6, "filtering by exportsvcs or blacklistsvcs");
        
        memset(tmp_refresh, 0, sizeof(*tmp_refresh));
        /* Initialize the list */
        tmp_refresh->mode = refresh->mode;
        
        /* We should export only services in list. */
        
        for (i=0; i<refresh->count; i++)
        {
            char search_svc[MAXTIDENT+3];
            search_svc[0]=',';
            search_svc[1]=EOS;
            
            strcat(search_svc, refresh->svcs[i].svc_nm);
            strcat(search_svc, ",");
            
            /*
             * If blacklist set, then filter out blacklisted services
             */
            if (EOS!=srv->conf->blacklistsvcs[0] && strstr(srv->conf->blacklistsvcs, 
                    search_svc))
            {
                NDRX_LOG(6, "svc %s blacklisted for export", refresh->svcs[i].svc_nm);
                continue;
            }
            /* If export list empty - export all
             * If export list set, then only those in the list
             */
            else if (EOS==srv->conf->exportsvcs[0] || strstr(srv->conf->exportsvcs, 
                    search_svc))
            {
                NDRX_LOG(6, "svc %s ok for export", refresh->svcs[i].svc_nm);
                /* Copy the service data off */
                memcpy(&tmp_refresh->svcs[tmp_refresh->count], &refresh->svcs[i], 
                        sizeof(refresh->svcs[0]));
                tmp_refresh->count++;
            }
        }
        /* Swith pointers over */
        p_refresh = tmp_refresh;
    }
    
    send_size = sizeof(bridge_refresh_t);
    send_size+=p_refresh->count*sizeof(bridge_refresh_svc_t);
    
    /* Call the server */
    ret = cmd_generic_call(NDRXD_COM_BRREFERSH_RQ, NDRXD_SRC_ADMIN,
            NDRXD_CALL_TYPE_BRIDGESVCS,
            (command_call_t *)p_refresh, send_size,
            G_command_state.listenq_str,
            G_command_state.listenq,
            (mqd_t)FAIL,
            get_srv_admin_q(srv),
            0, NULL,
            NULL,
            NULL,
            NULL,
            FALSE);
    
out:

    NDRX_LOG(log_debug, "%s return %d", fn, ret);
    return ret;
}

/**
 * Bridge have been connected.
 * @param nodeid
 * @return 
 */
public int brd_connected(int nodeid)
{
    int ret=SUCCEED;
    bridgedef_t *cur = brd_get_bridge(nodeid);
    char buf[ATMI_MSG_MAX_SIZE];
    bridge_refresh_t *refresh= (bridge_refresh_t *)buf;
    

    memset(buf, 0, sizeof(buf));
    
    if (NULL!=cur)
    {
        pm_node_t * p_pm = get_pm_from_srvid(cur->srvid);
        cur->connected = TRUE;
        p_pm->flags|=SRV_KEY_FLAGS_CONNECTED;
        
        if (cur->flags & SRV_KEY_FLAGS_SENDREFERSH)
        {
            NDRX_LOG(log_debug, "About to send to node %d full refresh", nodeid);
            if (SUCCEED==brd_build_refresh_msg(G_bridge_svc_hash, refresh, 
                    BRIDGE_REFRESH_MODE_FULL))
            {
                if (SUCCEED==(ret = brd_send_update(nodeid, cur, refresh)))
                {
                    /* Reset full refresh timer */
                    cur->lastrefresh_sent = SANITY_CNT_START;
                }
            }
        }
        else
        {
            NDRX_LOG(log_debug, "node %d does not require refresh", nodeid);
        }
    }
    else
    {
        NDRX_LOG(log_warn, "Unknown bridge nodeid %d!!!", nodeid);
    }
    
    return SUCCEED;
}

/**
 * Bridge have been disconnected - remove any stuff from shared mem.
 * @param nodeid
 * @return 
 */
public int brd_discconnected(int nodeid)
{
    bridgedef_t *cur = brd_get_bridge(nodeid);
    bridgedef_svcs_t *r = NULL;
    bridgedef_svcs_t *rtmp = NULL;
        
    if (NULL!=cur)
    {   pm_node_t * p_pm = get_pm_from_srvid(cur->srvid);
        cur->connected = FALSE;
        /* Clear connected flag. */
        p_pm->flags&=(~SRV_KEY_FLAGS_CONNECTED);
        
        /* ###################### CRITICAL SECTION ###################### */
        /* So we make this part critical... */
        if (SUCCEED!=ndrx_lock_svc_op())
        {
            goto out;
        }
        
        EXHASH_ITER(hh, cur->theyr_services, r, rtmp)
        {
            /* Remove from shm */
            ndrx_shm_install_svc_br(r->svc_nm, 0, 
                        TRUE, nodeid, 0, BRIDGE_REFRESH_MODE_FULL, 0);
            
            /* Delete from hash */
            EXHASH_DEL(cur->theyr_services, r);
            /* Free up memory */
            NDRX_FREE(r);
        }
        
        /* Remove the lock! */
        ndrx_unlock_svc_op();
        /* ###################### CRITICAL SECTION, END ################# */
        
    }
    else
    {
        NDRX_LOG(log_warn, "Unknown bridge nodeid %d!!!", nodeid);
    }
    
out:
    return SUCCEED;
}

/**
 * Our service to be removed from bridge hash..
 * TODO: In diff mode if item does not exists, it should add and run the stuff 
 * into minuses. So that we get the diff finally!!
 * 
 * @param svc
 * @return 
 */
public int brd_del_svc_from_hash_g(bridgedef_svcs_t ** svcs, char *svc, int diff)
{
    int ret=SUCCEED;
    bridgedef_svcs_t *r=NULL;
    
    EXHASH_FIND_STR( *svcs, svc, r);
    if (NULL!=r)
    {
        if (diff || r->count>1)
        {
            r->count--;
            NDRX_LOG(log_debug, "bridge view: svc [%s] count: %d", 
                    svc, r->count);
        }
        else
        {
            NDRX_LOG(log_debug, "bridge view: svc [%s] removed", 
                    svc);
            EXHASH_DEL(*svcs, r);
            NDRX_FREE(r);
        }
    }
    else
    {
        if (diff)
        {
            NDRX_LOG(log_debug, "Service [%s] does not exists in "
                    "diff hash - adding", svc);
            r = NDRX_CALLOC(1, sizeof(*r));
            if (NULL==r)
            {
                NDRX_LOG(log_error, "Failed to allocate %d bytes: %s", 
                        sizeof(*r), strerror(errno));
                ret=FAIL;
                goto out;
            }
            r->count=-1; /* should be 1 */
            strcpy(r->svc_nm, svc);
            EXHASH_ADD_STR( *svcs, svc_nm, r );
        }
        else
        {
            NDRX_LOG(log_debug, "WARN: called del svc %s, but no "
                    "entry in hash!", svc);
        }
    }
out:
    return ret;
}

/**
 * Delete service from global view
 * This also builds the diff
 * @param svc
 */
public void brd_del_svc_from_hash(char *svc)
{
    char *fn = "brd_del_svc_from_hash";
    /* Nothing to do if not clustered... 
    if (!G_atmi_env.is_clustered)
        return;
     * we need the list for pq stats..
     */
    
    /* We ignore special services like bridge.. */
    if (0==strncmp(svc, NDRX_SVC_BRIDGE, NDRX_SVC_BRIDGE_STATLEN) ||
            0==strcmp(svc, NDRX_SYS_SVC_PFX EV_TPEVSUBS) ||
            0==strcmp(svc, NDRX_SYS_SVC_PFX EV_TPEVUNSUBS) ||
            0==strcmp(svc, NDRX_SYS_SVC_PFX EV_TPEVPOST) ||
            0==strcmp(svc, NDRX_SYS_SVC_PFX EV_TPEVDOPOST)
        )
    {
        NDRX_LOG(log_debug, "del: IGNORING %s", svc);
        return;
    }
    
    NDRX_LOG(log_debug, "%s - enter", fn);
    
    if (M_build_diff)
    {
        brd_del_svc_from_hash_g(&G_bridge_svc_diff, svc, TRUE);
    }
    
    brd_del_svc_from_hash_g(&G_bridge_svc_hash, svc, FALSE);
}

/**
 * Modify global view of our services...
 * This also builds the diffs
 * @param svc
 * @return 
 */
public int brd_add_svc_to_hash(char *svc)
{
    int ret=SUCCEED;
    
    /* We ignore special services like bridge.. */
    if (0==strncmp(svc, NDRX_SVC_BRIDGE, NDRX_SVC_BRIDGE_STATLEN) ||
            0==strcmp(svc, NDRX_SYS_SVC_PFX EV_TPEVSUBS) ||
            0==strcmp(svc, NDRX_SYS_SVC_PFX EV_TPEVUNSUBS) ||
            0==strcmp(svc, NDRX_SYS_SVC_PFX EV_TPEVPOST) ||
            0==strcmp(svc, NDRX_SYS_SVC_PFX EV_TPEVDOPOST) ||
            /* Also have to skip recovery service for bridges. */
            0==strcmp(svc, NDRX_SYS_SVC_PFX TPRECOVERSVC)
    )
    {
        NDRX_LOG(log_debug, "IGNORING %s", svc);
        return SUCCEED;
    }
    
    NDRX_LOG(log_debug, "ADDING %s", svc);
    
    if (SUCCEED!=brd_add_svc_to_hash_g(&G_bridge_svc_diff, svc))
    {
        FAIL_OUT(ret);
    }
    
    ret=brd_add_svc_to_hash_g(&G_bridge_svc_hash, svc);
    
out:
    return SUCCEED;
}

/**
 * Add our service to hash list
 * @param svc
 * @param sh
 * @return 
 */
public int brd_add_svc_to_hash_g(bridgedef_svcs_t ** svcs, char *svc)
{
    int ret=SUCCEED;
    bridgedef_svcs_t *r=NULL;
    
    /* Try to get, if stuff in hash, increase counter */
    EXHASH_FIND_STR( *svcs, svc, r);
    if (NULL!=r)
    {
        r->count++;
    }
    /* If not in hash, then add new struct with count 1 */
    else
    {
        r = NDRX_CALLOC(1, sizeof(*r));
        if (NULL==r)
        {
            NDRX_LOG(log_error, "Failed to allocate %d bytes: %s", 
                    sizeof(*r), strerror(errno));
            ret=FAIL;
            goto out;
        }
        r->count++; /* should be 1 */
        strcpy(r->svc_nm, svc);
        EXHASH_ADD_STR( *svcs, svc_nm, r );
    }
    
    NDRX_LOG(log_error, "Number of services of [%s] = %d", svc, r->count);
    
out:
    return ret;
}

/**
 * Erase service hash list...
 */
public void brd_erase_svc_hash_g(bridgedef_svcs_t *svcs)
{
    bridgedef_svcs_t *cur, *tmp;
    
    EXHASH_ITER(hh, svcs, cur, tmp)
    {
        EXHASH_DEL(svcs,cur);
        NDRX_FREE(cur);
    }
}

/**
 * Add svc to bridge svc hash 
 */
public int brd_add_svc_brhash(bridgedef_t *cur, char *svc, int count)
{
    int ret=SUCCEED;
    bridgedef_svcs_t *r=NULL;
    bridgedef_svcs_t *tmp=NULL;
    
    /* Try to get, if stuff in hash, increase counter */
    EXHASH_FIND_STR( cur->theyr_services, svc, r);
    if (NULL!=r)
    {
        NDRX_LOG(log_error, "Service [%s] already exists in bridge's %d hash!",
                                svc, cur->nodeid);
        r->count = count;
    }
    else
    {
        r = NDRX_CALLOC(1, sizeof(*r));
        if (NULL==r)
        {
            NDRX_LOG(log_error, "Failed to allocate %d bytes: %s", 
                    sizeof(*r), strerror(errno));
            ret=FAIL;
            goto out;
        }
        strcpy(r->svc_nm, svc);
        r->count = count;
        EXHASH_ADD_STR( cur->theyr_services, svc_nm, r );
    }
    
out:
    return ret;
}

/**
 * Remove svc from bridge svc hash
 * @param cur - might be a null, then lookup will be made by svc.
 * @param s
 * @param svc
 * @param count
 * @return 
 */
public void brd_del_svc_brhash(bridgedef_t *cur, bridgedef_svcs_t *s, char *svc)
{
    bridgedef_svcs_t *r=NULL;
    
    if (NULL!=cur)
        r = s;
    else
        EXHASH_FIND_STR( cur->theyr_services, svc, r);
    
    if (NULL!=r)
    {
        EXHASH_DEL(cur->theyr_services, r);
        NDRX_FREE(r);
    }
    else
    {
        NDRX_LOG(log_debug, "WARN: called del svc %s, but no "
                "entry in bridge %d hash!", svc, cur->nodeid);
    }
}

/**
 * Get service entry of the bridge hash
 * @param cur
 * @param svc
 * @return 
 */
public bridgedef_svcs_t * brd_get_svc_brhash(bridgedef_t *cur, char *svc)
{
    bridgedef_svcs_t *r=NULL;
    EXHASH_FIND_STR( cur->theyr_services, svc, r);
    
    return r;
}


/**
 * Get service from service hash list.
 * @param cur
 * @param svc
 * @return 
 */
public bridgedef_svcs_t * brd_get_svc(bridgedef_svcs_t * svcs, char *svc)
{
    bridgedef_svcs_t *r=NULL;
    EXHASH_FIND_STR( svcs, svc, r);
    return r;
}



/**
 * remove service diff
 */
private void brd_clear_diff(void)
{
    bridgedef_svcs_t *cur, *tmp;
    
    EXHASH_ITER(hh, G_bridge_svc_diff, cur, tmp)
    {
        EXHASH_DEL(G_bridge_svc_diff,cur);
        NDRX_FREE(cur);
    }
    
}

/**
 * Begin diff build
 */
public void brd_begin_diff(void)
{
    if (!ndrx_get_G_atmi_env()->is_clustered)
        return;
    
    M_build_diff = TRUE;
    /* clear hashlist */
    brd_clear_diff();
}
    
/**
 * dispatch the diff to all nodes.
 */
public void brd_end_diff(void)
{
    bridgedef_t *r = NULL;
    bridgedef_t *rtmp = NULL;
    char buf[ATMI_MSG_MAX_SIZE];
    bridge_refresh_t *refresh= (bridge_refresh_t *)buf;
    int first = TRUE;
    
    /* Nothing to do. */
    if (!ndrx_get_G_atmi_env()->is_clustered)
        return;
    
    M_build_diff = FALSE;
    
    /* TODO: Send stuff to everybody */
    EXHASH_ITER(hh, G_bridge_hash, r, rtmp)
    {
        NDRX_LOG(log_debug, "Processing: nodeid %d, is connected=%s, flags=%d",
                                r->nodeid, r->connected?"Yes":"No", r->flags);
        
        if (r->connected && SRV_KEY_FLAGS_SENDREFERSH&r->flags)
        {
            NDRX_LOG(log_debug, "Sending refresh...");
            /* First time, build the diff message */
            if (first)
            {
                memset(buf, 0, sizeof(buf));
                brd_build_refresh_msg(G_bridge_svc_diff, refresh, 
                        BRIDGE_REFRESH_MODE_DIFF);
                first=FALSE;
                
                if (0==refresh->count)
                {
                    NDRX_LOG(log_debug, "Zero count of service "
                            "diff - nothing to do;");
                    break;
                }
            }
            
            if (SUCCEED!=brd_send_update(r->nodeid, r, refresh))
            {
                NDRX_LOG(log_warn, "Failed to send update to node %d", 
                        r->nodeid);
            }
        }
        else
        {
            NDRX_LOG(log_debug, "Not Sending refresh - not connected "
                    "or refresh not required");
        }
        
    }
    
    /* Clear hashlist */
    brd_clear_diff();
    
}

/**
 * Periodic houseekping function
 */
public void brd_send_periodrefresh(void)
{
    bridgedef_t *cur = NULL;
    bridgedef_t *rtmp = NULL;
    char buf[ATMI_MSG_MAX_SIZE];
    bridge_refresh_t *refresh= (bridge_refresh_t *)buf;
    
    /* Nothing to do. */
    if (!ndrx_get_G_atmi_env()->is_clustered || 0==G_app_config->brrefresh)
        return;
    
    /* Reset the buffer otherwise it keeps growing!!! */
    memset(buf, 0, sizeof(buf));
    
    EXHASH_ITER(hh, G_bridge_hash, cur, rtmp)
    {
        /* Using sanity as step interval */
        cur->lastrefresh_sent++;
        
        if (cur->connected && cur->flags & SRV_KEY_FLAGS_SENDREFERSH
                && cur->lastrefresh_sent > G_app_config->brrefresh)
        {
            NDRX_LOG(log_debug, "About to send to node %d full refresh", 
                    cur->nodeid);
            
            if (SUCCEED==brd_build_refresh_msg(G_bridge_svc_hash, refresh, 
                    BRIDGE_REFRESH_MODE_FULL))
            {
                if (SUCCEED == brd_send_update(cur->nodeid, cur, refresh))
                {
                    /* Reset full refresh timer */
                    cur->lastrefresh_sent = SANITY_CNT_START;
                }
            }
        } /* If connected */
    } /* EXHASH_ITER */
}
