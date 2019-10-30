/**
 * @brief Client Process Monitor interface
 *
 * @file cpm.h
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

#ifndef _CPM_H
#define	_CPM_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <ndrx_config.h>
#include <nstd_shm.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define CPM_CMD_PC          "pc" /* Print Clients(s) */
#define CPM_CMD_BC          "bc" /* Boot Client(s) */
#define CPM_CMD_SC          "sc" /* Stop Client(s) */
#define CPM_CMD_RC          "rc" /* Reload Client(s) (restart one by one) */
    
#define CPM_DEF_BUFFER_SZ       1024
    
#define CPM_OUTPUT_SIZE         256 /* Output buffer size */
    
#define CPM_TAG_LEN             128
#define CPM_SUBSECT_LEN         128
#define NDRX_CPM_SEP                        0x1c /**< Field seperator */
#define CPM_KEY_LEN             (CPM_TAG_LEN+1+CPM_SUBSECT_LEN) /* including FS in middle */
        
#define NDRX_CPM_CMDMIN         32      /**< Min command len            */

#define NDRX_CPM_MAP_ISUSED       NDRX_LH_FLAG_ISUSED /**< Entry used        */
#define NDRX_CPM_MAP_WASUSED      NDRX_LH_FLAG_WASUSED /**< Entry was used   */
#define NDRX_CPM_MAP_CPMPROC      0x0004  /**< Process added by CPM          */
#define NDRX_CPM_MAP_SELFMANG     0x0008  /**< Self managed process, non cpm */

#define NDRX_CPMSHM_MAX_READERS 10  /**< Max concurrent readers of sh   */
    
    
#define NDRX_CPM_INDEX(MEM, IDX) ((ndrx_clt_shm_t*)(((char*)MEM)+(int)(sizeof(ndrx_clt_shm_t)*IDX)))

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
    
/**
 * Shared memory entry for service
 */
typedef struct ndrx_clt_shm ndrx_clt_shm_t;
struct ndrx_clt_shm
{
    char key[NDRX_MAX_Q_SIZE+1];        /**< tag<fs>subsection          */
    pid_t pid;                          /**< System V Queue id          */
    short flags;                        /**< See NDRX_SVQ_MAP_STAT_*    */
    time_t stattime;                    /**< Status change time         */
    char procname[NDRX_CPM_CMDMIN+1];   /**< process name               */
    long padding1;                      /**< ensure that struct is padded*/
};

/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

extern NDRX_API void ndrx_cltshm_down(int *signals, int *p_was_any);
extern NDRX_API int ndrx_cltshm_setpos(char *key, pid_t pid, short flags, 
        char *procname);
extern NDRX_API pid_t ndrx_cltshm_getpid(char *key, char *procname, 
        size_t procnamesz, time_t *p_stattime);
extern NDRX_API ndrx_sem_t* ndrx_cltshm_sem_get(void);
extern NDRX_API ndrx_shm_t* ndrx_cltshm_mem_get(void);
extern NDRX_API int ndrx_cltshm_remove(int force);
extern NDRX_API void ndrx_cltshm_detach(void);
extern NDRX_API int ndrx_cltshm_get_key(char *key, int oflag, int *pos, int *have_value);
extern NDRX_API int ndrx_cltshm_init(int attach_only);

#ifdef	__cplusplus
}
#endif

#endif	/* _CPM_H */

/* vim: set ts=4 sw=4 et smartindent: */
