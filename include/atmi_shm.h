/* 
** Shared memory API for Enduro/X ATMI
**
** @file atmi_shm.h
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

#ifndef ATMI_SHM_H
#define	ATMI_SHM_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <ndrxdcmn.h>
#include <sys/sem.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define NDRX_MAX_SHM_SIZE     128           /* Max shared memory file name len */

/* ATMI SHM Level */
#define NDRX_SHM_LEV_SVC               0x01    /* Service array */
#define NDRX_SHM_LEV_SRV               0x02    /* Server array */
#define NDRX_SHM_LEV_BR                0x04    /* Bridge array */

    
#define NDRX_SHM_BR_CONNECTED          0x01    /* Bridge is connected */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
typedef struct
{
    char path[NDRX_MAX_SHM_SIZE];       /* name of shm                  */
    int fd;                             /* opened fd                    */
    char *mem;                          /* mapped memory                */
    int size;                           /* size used by this shm block  */
} ndrx_shm_t;

typedef struct
{
    key_t key;            /* Key for semaphore... */
    int semid;
    /*struct sembuf sem[2];  sembuf defined in sys/sem.h */
    short attached;                     /* Is attached?                 */
} ndrx_sem_t;
/*---------------------------Globals------------------------------------*/
extern NDRX_API int G_max_svcs;
extern NDRX_API ndrx_shm_t G_svcinfo;
extern NDRX_API int G_max_servers;
extern NDRX_API ndrx_shm_t G_srvinfo;
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
extern NDRX_API int shm_init(char *q_prefix, int max_servers, int max_svcs);
extern NDRX_API int ndrxd_shm_open_all(void);
extern NDRX_API int ndrxd_shm_close_all(void);
extern NDRX_API int ndrxd_shm_delete(void);
extern NDRX_API int ndrx_shm_attach(ndrx_shm_t *shm);
extern NDRX_API int ndrx_shm_attach_all(int lev);
extern NDRX_API int ndrx_shm_get_svc(char *svc, char *send_q, int *is_bridge);
extern NDRX_API int ndrx_shm_get_srvs(char *svc, short **srvlist, int *len); /* poll() only */
extern NDRX_API int _ndrx_shm_get_svc(char *svc, int *pos);
extern NDRX_API int ndrx_shm_install_svc(char *svc, int flags, short srvid);
extern NDRX_API int ndrx_shm_install_svc_br(char *svc, int flags, 
                int is_bridge, int nodeid, int count, char mode, short srvid);
extern NDRX_API void ndrxd_shm_uninstall_svc(char *svc, int *last, short srvid);
extern NDRX_API shm_srvinfo_t* ndrxd_shm_getsrv(int srvid);
extern NDRX_API void ndrxd_shm_resetsrv(int srvid);

extern NDRX_API int ndrx_shm_birdge_set_flags(int nodeid, int flags, int op_end);
extern NDRX_API int ndrx_shm_bridge_disco(int nodeid);
extern NDRX_API int ndrx_shm_bridge_connected(int nodeid);
extern NDRX_API int ndrx_shm_bridge_is_connected(int nodeid);
extern NDRX_API int ndrx_shm_birdge_getnodesconnected(char *outputbuf);

/* Semaphore driving: */
extern NDRX_API int ndrxd_sem_init(char *q_prefix);
extern NDRX_API int ndrx_sem_attach(ndrx_sem_t *sem);
extern NDRX_API int ndrxd_sem_open_all(void);
extern NDRX_API int ndrxd_sem_close_all(void);
extern NDRX_API int ndrxd_sem_delete(void);
extern NDRX_API void ndrxd_sem_delete_with_init(char *q_prefix);
extern NDRX_API int ndrxd_sem_is_attached(ndrx_sem_t *sem);
extern NDRX_API int ndrx_sem_attach_all(void);
extern NDRX_API int ndrx_lock_svc_op(void);
extern NDRX_API int ndrx_unlock_svc_op(void);
extern NDRX_API unsigned int ndrx_hash_fn( void *k );

extern NDRX_API int ndrx_lock_svc_nm(char *svcnm);
extern NDRX_API int ndrx_unlock_svc_nm(char *svcnm);

#ifdef	__cplusplus
}
#endif

#endif	/* ATMI_SHM_H */

