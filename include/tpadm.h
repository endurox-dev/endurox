/**
 * @brief Enduro/X application server administration interface
 *
 * @file tpadm.h
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

#ifndef TPADM_H
#define	TPADM_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <Excompat.h>
#include <ubf.h>
#include <nstdutil.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define MIB_LOCAL   0x00001
    
/** Error codes for Admin API 
 * @defgroup MIB_ERRORS MIB API error codes
 * @{
 */
#define TAEAPP          -1   /**< Other componets failure                */
#define TAECONFIG       -2   /**< Configuration file failure             */    
#define TAEINVAL        -3   /**< Invalid argument, see TA_BADFLD        */    
#define TAEOS           -4   /**< Operating system error, see TA_STATUS  */    
#define TAEPERM         -6   /**< No permissions for operation           */
#define TAEPREIMAGE     -7   /**< Set failed due to invalid image        */
#define TAEPROTO        -8   /**< Protocol error                         */
#define TAEREQUIRED     -9   /**< Required field missing see TA_BADFLD   */
#define TAESUPPORT      -10  /**< Admin call not support in current ver  */
#define TAESYSTEM       -11  /**< System (Enduro/X) error occurred       */
#define TAEUNIQ         -12  /**< Object for update not identified       */
    
#define TAOK            0    /**< Request succeed, no up-updates         */
#define TAUPDATED       1    /**< Succeed, updates made                  */
#define TAPARTIAL       2    /**< Partial succeed, have updates          */
/** @}*/

#define NDRX_TA_CLASS_CLIENT        "T_CLIENT"      /**<  Client process class */
    
#define NDRX_TA_GET                 "GET"           /**< Get infos             */
#define NDRX_TA_GETNEXT             "GETNEXT"       /**< Read next curs        */

    
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
extern NDRX_API int tpadmcall(UBFH *inbuf, UBFH **outbuf, long flags);
extern NDRX_API int ndrx_buffer_list(ndrx_growlist_t *list);

#ifdef	__cplusplus
}
#endif

#endif	/* TPADM_H */

/* vim: set ts=4 sw=4 et smartindent: */
