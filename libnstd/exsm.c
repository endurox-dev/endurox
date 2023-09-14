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
    int flags;              /**< internal flags of the state */
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
 * return size of the single state
 * @param nr_tran number of transitions per state (must be same for all states)
 * @return state object size in bytes
 */
exprivate size_t ndrx_sm_state_size(int nr_tran)
{
    return (sizeof(ndrx_sm_state_handle_t) + nr_tran*sizeof(ndrx_sm_tran_t));
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
        if (cur->flags & NDRX_SM_FLAGS_SCAN)
        {
            for (i=0; i<nr_tran; i++)
            {
                 if (NDRX_SM_EV_EOF==cur->transitions[i].event)
                 {
                    break;
                 }
                 else if (cur->transitions[i].event == event)
                 {
                    break;
                 }
            }
        }
        else if (cur->flags & NDRX_SM_FLAGS_INDEX)
        {
            if (event>=0 && event<nr_tran)
            {
                i = event;
            }
            else
            {
                /* event ouf of the rnage: */
                NDRX_LOG(log_error, "sm: ERROR ! state %s (%d), event %d: "
                    "no transition found!", cur->state_name, cur->state, event);
                userlog("sm: ERROR ! state %s (%d), event %d: "
                        "no transition found!",cur->state_name, cur->state, event);
                EXFAIL_OUT(ret);
            }
        }
        else
        {
            NDRX_LOG(log_error, "sm: ERROR ! machine not compiled (flags=%x)",
                    cur->flags);
            userlog("sm: ERROR ! machine not compiled (flags=%x)",
                    cur->flags);
            EXFAIL_OUT(ret);
        }

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
            NDRX_LOG(log_debug , "sm: %s, event %s", 
                cur->state_name, cur->transitions[i].event_name);
            next_state = cur->transitions[i].next_state;
        }

        if (NDRX_SM_ST_RETURN==next_state)
        {
            NDRX_LOG(log_debug, "sm: return from %s, with ret=%d", 
                    cur->state_name, event);
            ret=event;
            goto out;
        }
        else if (NDRX_SM_ST_RETURN0==next_state)
        {
            NDRX_LOG(log_debug, "sm: return from %s, with ev %d as ret=0",
                    cur->state_name, event);
            ret=0;
            goto out;
        }
        
        cur = ndrx_sm_state_get(sm, nr_tran, next_state);       
    }
out:
    return ret;
}

/**
 * Compile + Validate state machine.
 * Compilation makes converts structures for mapping events to array indexes for transitions
 * (where possible)
 * !!!! This assumes that SM buffer size is enough to handle largest state number.
 * thus machine must be allocated with last state in mind.
 * Checks:
 *  - states are in sequence with memory arrays
 *  - every state transitions ends with NDRX_SM_EV_EOF (may cause core dump if not found)
 *  - every state transitions count is not greater than nr_tran
 *  - every state transitions point to valid state (i.e. state nr is less than last_state)
 * No gaps in states are allowed.
 * @param sm state machine (basically according to ndrx_sm_state_handle_t,
 * but having dynamic number of transitions)
 * @param nr_state number of total states
 * @param nr_tran number of transitions per state (must be same for all states)
 * @param last_last last state used of the machine, so that remainder can be reset.
 * @return 0 if ok, -1 if error
 */
expublic int ndrx_sm_comp(void *sm, int nr_state, int nr_tran, int last_state)
{
    int i;
    int tran;
    int ret = EXSUCCEED;
    int got_eof;
    int in_range;
    size_t state_size = ndrx_sm_state_size(nr_tran);
    char tmp[ndrx_sm_state_size(nr_tran)*nr_state];

    /* fix the sm.., reset remainder states, as array space is larger than
     * number of states
     */
    got_eof=EXFALSE;
    for (i=0; i<nr_state; i++)
    {
        ndrx_sm_state_handle_t *cur;
        cur = ndrx_sm_state_get(sm, nr_tran, i);

        if (got_eof)
        {
            cur->state=EXFAIL;
        }
        else if (cur->state==last_state)
        {
            got_eof=EXTRUE;
        }
        else if (cur->state >=nr_state)
        {
            NDRX_LOG(log_error, "sm: ERROR ! got state %d, but max allowed is %d!",
                    cur->state, nr_state-1);
            EXFAIL_OUT(ret);
        }
    }

    memcpy(tmp, sm, sizeof(tmp));

    /* reset SM */
    got_eof=EXFALSE;
    for (i=0; i<nr_state; i++)
    {
        ndrx_sm_state_handle_t *cur;
        cur = ndrx_sm_state_get(sm, nr_tran, i);
        cur->state=i;
        cur->func=NULL;
        if (tran>0)
        {
            cur->transitions[0].event=NDRX_SM_EV_EOF;
        }
    }

    /* generate new mapping */
    for (i=0; i<nr_state; i++)
    {
        ndrx_sm_state_handle_t *cur;
        ndrx_sm_state_handle_t *tmp_cur;

        tmp_cur = ndrx_sm_state_get(tmp, nr_tran, i);

        if (tmp_cur->state==EXFAIL)
        {
            /* we are done... */
            break;
        }
        
        /* map out the states to indexes */
        cur = ndrx_sm_state_get(sm, nr_tran, tmp_cur->state);

        memcpy(cur, tmp_cur, state_size);
    }


    /* validate indexes */
    for (i=0; i<nr_state; i++)
    {
        ndrx_sm_state_handle_t *cur = ndrx_sm_state_get(sm, nr_tran, i);

        /* check that state is intialized */

        if (cur->state != i)
        {
            NDRX_LOG(log_error, "sm: ERROR ! state %s (%d) is not in sequence!", 
                    cur->state_name, cur->state);
            EXFAIL_OUT(ret);
        }

        got_eof=EXFALSE;
        in_range=EXTRUE;

        for (tran=0; tran<nr_tran; tran++)
        {
            if (NDRX_SM_EV_EOF==cur->transitions[tran].event)
            {
                /* ok */
                got_eof=EXTRUE;
                break;
            }
            else if ( (cur->transitions[tran].next_state <0 ||
                cur->transitions[tran].next_state >nr_state-1) &&
                NDRX_SM_ST_RETURN!=cur->transitions[tran].next_state &&
                NDRX_SM_ST_RETURN0!=cur->transitions[tran].next_state
                )
            {
                NDRX_LOG(log_error, "sm: ERROR ! state %s (%d), event %s (%d): "
                        "next state %d is out of range [%d..%d]!",
                        cur->state_name, cur->state, cur->transitions[tran].event_name,
                        cur->transitions[tran].event, cur->transitions[tran].next_state,
                        0, nr_state-1);
                EXFAIL_OUT(ret);
            }
            else if (
                NDRX_SM_ST_RETURN!=cur->transitions[tran].next_state &&
                NDRX_SM_ST_RETURN0!=cur->transitions[tran].next_state &&
                NULL==ndrx_sm_state_get(sm, nr_tran, cur->transitions[tran].next_state)->func
                )
            {
                NDRX_LOG(log_error, "sm: ERROR ! state %s (%d), event %s (%d): "
                        "next state %d is not used!",
                        cur->state_name, cur->state, cur->transitions[tran].event_name,
                        cur->transitions[tran].event, cur->transitions[tran].next_state);
                EXFAIL_OUT(ret);
            }
            else if (cur->transitions[tran].event<0 || cur->transitions[tran].event>=nr_tran)
            {
                in_range=EXFALSE;
            }
        }

        /* build index  */
        if (in_range)
        {
            ndrx_sm_tran_t tmp_trn[nr_tran];
            got_eof=EXFALSE;
            /* copy current table */
            memcpy(tmp_trn, cur->transitions, sizeof(tmp_trn));

            cur->flags |= NDRX_SM_FLAGS_INDEX;
            /* generate index table... */
            for (tran=0; tran<nr_tran; tran++)
            {
                /* reset all to EOF*/
                cur->transitions[tran].event = NDRX_SM_EV_EOF;
            }
            for (tran=0; tran<nr_tran; tran++)
            {
                if (NDRX_SM_EV_EOF==tmp_trn[tran].event)
                {
                    /* mapping done. */
                    break;
                }

                cur->transitions[tmp_trn[tran].event] = tmp_trn[tran];
            }
        }
        else
        {
            cur->flags |= NDRX_SM_FLAGS_SCAN;
        }
    }

out:
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
