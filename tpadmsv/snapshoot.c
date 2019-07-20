/**
 * @brief Library for keeping snapshoot cursor data
 *
 * @file snapshoot.c
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <regex.h>
#include <utlist.h>
#include <unistd.h>
#include <signal.h>

#include <ndebug.h>
#include <atmi.h>
#include <atmi_int.h>
#include <typed_buf.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <Exfields.h>
#include <Excompat.h>
#include <ubfutil.h>

#include "tpadmsv.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
exprivate long M_cntr = 1; /**< Counter of the cursor ID */
exprivate ndrx_adm_cursors_t *M_cursors = NULL; /**< List of open cursors */
/*---------------------------Prototypes---------------------------------*/

/**
 * Open new cursor. Assign the cursor id
 * @param clazz
 * @param data data to load into cursor
 * @return 
 */
expublic ndrx_adm_cursors_t* ndrx_adm_curs_new(char *clazz, ndrx_adm_cursors_t *data)
{
    int ret = EXSUCCEED;
    char curs[MAXTIDENT+1];
    ndrx_adm_cursors_t *el = NULL;
    
    M_cntr++;
    
    if (M_cntr > 999999999)
    {
        M_cntr=1;
    }
    
    /* Cursor format:
     * .TMIB-N-S_CCNNNNNN
     * Where:
     * - N -> Node id;
     * - S -> Service ID
     */
    snprintf(curs, sizeof(curs), "%s_%s%09ld", ndrx_G_svcnm2, clazz, M_cntr);
    
    /* Allocate the hash. */
    
    el = NDRX_MALLOC(sizeof(ndrx_adm_cursors_t));
    
    if (NULL==el)
    {
        NDRX_LOG(log_error, "Failed to malloc %d bytes: %s", sizeof(ndrx_adm_cursors_t),
                strerror(errno));
        EXFAIL_OUT(ret);
    }
    
    memcpy(el, data, sizeof(ndrx_adm_cursors_t));
    NDRX_STRCPY_SAFE(el->cursorid, curs);
    
    EXHASH_ADD_STR( M_cursors, cursorid, el );
    
out:
    
    if (EXSUCCEED!=ret && NULL!=el)
    {
        NDRX_FREE(el);
        el = NULL;
    }
    
    return el;
}

/**
 * This shall find the cursor, and perform the buffer mappings according to tables
 *  and structures.
 *  The buffer shall be received and filled accordingly. If fetched till the end
 *  the data item shall be deleted.
 * @return EXUSCCEED/EXFAIL
 */
expublic int ndrx_adm_curs_fetch(UBFH *p_ub)
{
    
}



/* vim: set ts=4 sw=4 et smartindent: */
