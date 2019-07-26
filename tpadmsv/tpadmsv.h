/**
 *
 * @file brdcstsv.h
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * AGPL or Mavimax's license for commercial use.
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

#ifndef BRDCSTSV_H
#define	BRDCSTSV_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
/*---------------------------Externs------------------------------------*/
extern char ndrx_G_svcnm2[];
/*---------------------------Macros-------------------------------------*/

/** minimum error free size to install in buffer                */
#define TPADM_ERROR_MINSZ           MAX_TP_ERROR_LEN+128


/** client data size                                            */
#define TPADM_CLIENT_DATASZ         (sizeof(ndrx_adm_client_t)*2)

/** minimum free buffer size for processing responses / paging  */
#define TPADM_DEFAULT_BUFFER_MINSZ      4096
/** Number of seconds to live */
#define TPADM_DEFAULT_VALIDITY          30      
/** houskeep of the cursors */
#define TPADM_DEFAULT_SCANTIME          15
/** Max number of open cursors same time */
#define TPADM_DEFAULT_CURSORS_MAX       100
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/**
 * Admin server configuration
 */
typedef struct
{
    int buffer_minsz;
    int validity;
    int scantime;
    int cursors_max;
} ndrx_adm_conf_t;

/**
 * C structure fields to UBF fields which are loaded to caller
 */
typedef struct
{
    BFLDID fid;
    int c_offset;
} ndrx_adm_elmap_t;

/**
 * Admin cursors open
 */
struct ndrx_adm_cursors
{
    char cursorid[MAXTIDENT+1]; /**< Cached cursor id                       */
    int curspos;                /**< Current cursor position                */
    ndrx_growlist_t list;       /**< List of the cursor items               */
    
    ndrx_stopwatch_t w;         /**< Stopwatch when the cursor was open     */
    ndrx_adm_elmap_t *map;      /**< Field map for response                 */
    EX_hash_handle hh;          /**< makes this structure hashable          */
};

/**
 * Cursors type
 */
typedef struct ndrx_adm_cursors ndrx_adm_cursors_t;

/**
 * Map classes to operations
 */
typedef struct
{
    char clazz[MAXTIDENT+1];                    /**< Class name               */
    char clazzshort[3];                         /**< Short class name         */
    /** Get cursor   */
    int (*p_get)(char *clazz, ndrx_adm_cursors_t *cursnew, long flags);
} ndrx_adm_class_map_t;

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

extern ndrx_adm_conf_t ndrx_G_adm_config;   /**< admin server config    */

/*---------------------------Prototypes---------------------------------*/
extern long ndrx_adm_error_get(UBFH *p_ub);
extern int ndrx_adm_error_set(UBFH *p_ub, long error_code, 
        long fldid, const char *fmt, ...);

/* Class types: */
extern int ndrx_adm_client_get(char *clazz, ndrx_adm_cursors_t *cursnew, long flags);
extern int ndrx_adm_domain_get(char *clazz, ndrx_adm_cursors_t *cursnew, long flags);
extern int ndrx_adm_machine_get(char *clazz, ndrx_adm_cursors_t *cursnew, long flags);
extern int ndrx_adm_queue_get(char *clazz, ndrx_adm_cursors_t *cursnew, long flags);
extern int ndrx_adm_server_get(char *clazz, ndrx_adm_cursors_t *cursnew, long flags);
extern int ndrx_adm_service_get(char *clazz, ndrx_adm_cursors_t *cursnew, long flags);
extern int ndrx_adm_svcgrp_get(char *clazz, ndrx_adm_cursors_t *cursnew, long flags);

extern int ndrx_adm_ppm_call(int (*p_rsp_process)(command_reply_t *reply, size_t reply_len));
extern int ndrx_adm_psc_call(int (*p_rsp_process)(command_reply_t *reply, size_t reply_len));

extern ndrx_adm_cursors_t* ndrx_adm_curs_get(char *cursid);

extern ndrx_adm_cursors_t* ndrx_adm_curs_new(UBFH *p_ub, ndrx_adm_class_map_t *map,
        ndrx_adm_cursors_t *data);

extern int ndrx_adm_curs_housekeep(void);
extern void ndrx_adm_curs_close(ndrx_adm_cursors_t *curs);
extern int ndrx_adm_curs_fetch(UBFH *p_ub, ndrx_adm_cursors_t *curs,
        long *ret_occurs, long *ret_more);


#ifdef	__cplusplus
}
#endif

#endif	/* BRDCSTSV_H */

/* vim: set ts=4 sw=4 et smartindent: */
