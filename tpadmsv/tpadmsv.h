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
/** minimum free buffer size for processing responses / paging  */
#define TPADM_BUFFER_MINSZ          4096

/** client data size                                            */
#define TPADM_CLIENT_DATASZ         (sizeof(ndrx_adm_client_t)*2)
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/**
 * C structure fields to UBF fields which are loaded to caller
 */
typedef struct
{
    int c_offset;
    BFLDID fid;
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
 * Image of the client information
 */
typedef struct 
{
    char clientid[78+1];      /**< myid                                 */
    char name[MAXTIDENT+1];   /**< process name                         */
    char lmid[MAXTIDENT+1];   /**< cluster node id                      */
    /** may be: ACT, SUS (not used), DEA - dead */
    char state[15+1];         /**< state of the client live/dead by pid */
    long pid;                 /**< process PID                          */
    long curconv;             /**< number of conversations process into */
    long contextid;           /**< Multi-threading context id           */
} ndrx_adm_client_t;

/**
 * Map classes to operations
 */
typedef struct
{
    char clazz[MAXTIDENT+1];                    /**< Class name               */
    void (*p_get)(ndrx_adm_cursors_t *cursnew, long flags); /**< Get cursor   */
} ndrx_adm_class_map_t;

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
extern long ndrx_adm_error_get(UBFH *p_ub);
extern int ndrx_adm_error_set(UBFH *p_ub, long error_code, 
        long fldid, const char *fmt, ...);

/* Class types: */
extern int ndrx_adm_client_get(ndrx_adm_cursors_t *cursnew, long flags);

#ifdef	__cplusplus
}
#endif

#endif	/* BRDCSTSV_H */

/* vim: set ts=4 sw=4 et smartindent: */
