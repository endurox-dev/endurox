/**
 * @brief Enduro/X State Machine (moore machine)
 *
 * @file exsm.c
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
/*---------------------------Includes-----------------------------------*/
#include <ndrx_config.h>
#include <ndrstandard.h>
#include <ndebug.h>
#include <exsm.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/**
 * handle for accessing state info
 * must match fields of NDRX_SM_T
 */
typedef struct
{
    int state; /**< state number */
    char state_name[32]; /**< state name */
    int (*func)(void *data); /**< state function (callback) */
    ndrx_sm_tran_t transitions[0]; /**< transitions */
} ndrx_sm_state_handle_t;
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Get state handle
 * @param sm state machine (basically according to ndrx_sm_state_handle_t,
 * but having dynamic number of transitions)
 * @param nr_tran number of transitions per state (must be same for all states)
 * @param state state number
 * @return state handle
 */
exprivate ndrx_sm_state_handle_t * ndrx_sm_state_get(void *sm, int nr_tran, int state)
{
    return (ndrx_sm_state_handle_t *)( 
            (char *)sm + (sizeof(ndrx_sm_state_handle_t)+nr_tran*sizeof(ndrx_sm_tran_t))*state);
}

/**
 * Run state machine.
 * @param sm state machine (basically according to ndrx_sm_state_handle_t,
 *  but having dynamic number of transitions)
 * @param nr_tran number of transitions per state (must be same for all states)
 * @param data data to pass to state functions
 * @return last event passed to NDRX_SM_ST_RETURN (or -1 if error)
 */
expublic int ndrx_sm_run(void *sm, int nr_tran, int entry_state, void *data)
{
    int ret = EXSUCCEED;
    ndrx_sm_state_handle_t *cur = ndrx_sm_state_get(sm, nr_tran, entry_state);

    while (NDRX_SM_ST_RETURN!=cur->state)
    {
        int i;
        int next_state = -1;
        int event = -1;
        
        /* run state function */
        event = cur->func(data);
        
        /* find next state */
        for (i=0; i<nr_tran; i++)
        {
            if (NDRX_SM_EV_EOF==cur->transitions[i].event)
            {
                NDRX_LOG(log_error, "sm: ERROR ! state %s (%d), event %d: "
                        "no transition found!", cur->state_name, cur->state, event);
                userlog("sm: ERROR ! state %s (%d), event %d: "
                        "no transition found!",cur->state_name, cur->state, event);
                EXFAIL_OUT(ret);
            }
            else if (cur->transitions[i].event == event)
            {
                NDRX_LOG(log_debug , "sm: %s (%d), event %s (%d)", 
                cur->state_name, cur->state, cur->transitions[i].event_name, event);
                next_state = cur->transitions[i].next_state;
                break;
            }
        }

        if (NDRX_SM_ST_RETURN==next_state)
        {
            NDRX_LOG(log_debug, "sm: return from %s (%d), with ret=%d", 
                    cur->state_name, cur->state, event);
            ret=event;
            goto out;
        }
        
        cur = ndrx_sm_state_get(sm, nr_tran, next_state);       
    }
out:
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
