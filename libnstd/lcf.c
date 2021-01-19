/**
 * @brief Latent Command Framework - real time settings frameworks for all Enduro/X
 *  processes (clients & servers)
 *
 * @file lcf.c
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

/*---------------------------Includes-----------------------------------*/
#include <ndrstandard.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

exprivate ndrx_shm_t M_map_lcf = {.fd=0, .path=""};   /**< Shared memory for settings       */
exprivate ndrx_sem_t M_map_lcf = {.semid=0};          /**< RW semaphore for SHM protection  */

/*---------------------------Prototypes---------------------------------*/

/**
 * Load the LCF settings. Attach semaphores and shared memory.
 * - So we will re-use the same system-v sempahore array used for System-V queues
 *  with slot 2 used for LCF
 * - Add new shared memory block for LCF commands and general real time settings
 *  like current page of DDR routing data
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_lcf_init(void)
{
    
}

/* vim: set ts=4 sw=4 et smartindent: */
