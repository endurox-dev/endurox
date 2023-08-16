/**
 * @brief State machine definition
 * 
 * @file exsm.h
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2023, Mavimax, Ltd. All Rights Reserved.
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
#ifndef EXSM_H__
#define EXSM_H__

#if defined(__cplusplus)
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <ndrx_config.h>
#include <ndrstandard.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

/** Return from state machine (special for exit). 
 * This returns last event code as machine exit code
 */
#define NDRX_SM_ST_RETURN   -1

/** Return from state machine (special for exit). 
 * This returns 0 as machine exit code
 */
#define NDRX_SM_ST_RETURN0  -2

#define NDRX_SM_EV_EOF      EXFAIL   /**< Event list end marker       */

#define NDRX_SM_TRAN(EVENT, NEXT_STATE) {EVENT, #EVENT, NEXT_STATE} /**< Transition macro */
#define NDRX_SM_TRAN_END {EXFAIL, "", 0} /**< Transition end macro */

/**
 * State machine state definition
 */
#define NDRX_SM_STATE(STATE, FUNC, ...)                              \
    {                                                                \
        STATE, #STATE, FUNC, {__VA_ARGS__}                           \
    }                                                                \
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/**
 * State machine transition 
 */
typedef struct
{
    int event;              /**< event id                       */
    char event_name[32];    /**<                                */
    int next_state;         /**< next state                     */
} ndrx_sm_tran_t;

/**
 * State machine type, with custom number of transitions
 * @param NAME state machine name
 * @param NR_TRAN number of transitions
 */
#define NDRX_SM_T(NAME, NR_TRAN)                                    \
    typedef struct                                                  \
    {                                                               \
        int state; /**< state number */                             \
        char state_name[32]; /**< state name */                     \
        int (*func)(void *data); /**< state function (callback) */  \
        ndrx_sm_tran_t transitions[NR_TRAN]; /**< transitions */    \
    } NAME
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

extern NDRX_API int ndrx_sm_run(void *sm, int nr_tran, int entry_state, void *data);

#if defined(__cplusplus)
}
#endif

#endif
/* vim: set ts=4 sw=4 et smartindent: */
