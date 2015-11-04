/* 
** NDRXD Bridge internal structures.
**
** @file bridge_int.h
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

#ifndef BRIDGE_INT_H
#define	BRIDGE_INT_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <uthash.h>
#include <atmi_int.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/*
 * Hash list for services advertised by other node
 */
struct bridgedef_svcs
{
    /* Service name */
    char svc_nm[MAXTIDENT+1];
    /* Number of copies */
    int count;
    int pq_info[PQ_LEN];                  /* Print queues,  statistics */
    /* makes this structure hashable */
    UT_hash_handle hh;
};
typedef struct bridgedef_svcs bridgedef_svcs_t;

/*
 * hash handler for bridges
 */
struct bridgedef 
{
    /* ID of the other node */
    int nodeid;
    /* Have a bridge data... */
    int srvid;
    /* Are we connected? */
    int connected;
    /* Flags configured for bridge */
    int flags;
    /* If above enabled, then when was last time we sent refersh? */
    long lastrefresh_sent;
    
    /* List of other nodes (nodeid) services */
    bridgedef_svcs_t * theyr_services;
    
    /* makes this structure hashable */
    UT_hash_handle hh;
};
typedef struct bridgedef bridgedef_t;
/*---------------------------Globals------------------------------------*/
/* Bridge related stuff from bridge.c */
extern bridgedef_t *G_bridge_hash; /* Hash table of bridges */
extern bridgedef_svcs_t *G_bridge_svc_hash; /* Our full list of local services! */
extern bridgedef_svcs_t *G_bridge_svc_diff; /* Service diff to be sent to nodes */
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
extern bridgedef_t* brd_get_bridge(int nodeid);
extern int brd_addupd_bridge(srv_status_t * srvinfo);

/* Our list of local services */
extern void brd_del_svc_from_hash(char *svc);
extern int brd_add_svc_to_hash(char *svc);


extern int brd_connected(int nodeid);
extern int brd_discconnected(int nodeid);

extern int brd_add_svc_brhash(bridgedef_t *cur, char *svc, int count);
extern void brd_del_svc_brhash(bridgedef_t *cur, bridgedef_svcs_t *s, char *svc);
extern bridgedef_svcs_t * brd_get_svc_brhash(bridgedef_t *cur, char *svc);
extern bridgedef_svcs_t * brd_get_svc(bridgedef_svcs_t * svcs, char *svc);
extern void brd_erase_svc_hash_g(bridgedef_svcs_t *svcs);

extern void brd_end_diff(void);
extern void brd_begin_diff(void);
extern void brd_send_periodrefresh(void);
extern int brd_lock_and_update_shm(int nodeid, char *svc_nm, int count, char mode);

extern int brd_del_bridge(int nodeid);

#ifdef	__cplusplus
}
#endif

#endif	/* BRIDGE_INT_H */

