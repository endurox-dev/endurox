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
#include <tpadm.h>
#include <exhash.h>
#include "tpadmsv.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
exprivate long M_cntr = 1; /**< Counter of the cursor ID */
exprivate ndrx_adm_cursors_t *M_cursors = NULL; /**< List of open cursors */
exprivate int M_nr_cursor = 0;  /**< Number of cursor open. */
/*---------------------------Prototypes---------------------------------*/


/**
 * Housekeep routines
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_adm_curs_housekeep(void)
{
    long delta;
    ndrx_adm_cursors_t *el, *elt;
    
    EXHASH_ITER(hh, M_cursors, el, elt)
    {
        if ((delta=ndrx_stopwatch_get_delta_sec(&el->w)) > ndrx_G_adm_config.validity)
        {
            NDRX_LOG(log_error, "Cursor [%s] expired (%d sec)", el->cursorid, delta);
            ndrx_adm_curs_close(el);
        }
    }
    
    return EXSUCCEED;
}


/**
 * Find the cursor stored locally
 * @param cursid cursor id string
 * @return ptr or NULL
 */
expublic ndrx_adm_cursors_t* ndrx_adm_curs_get(char *cursid)
{
    ndrx_adm_cursors_t* ret;
    
    EXHASH_FIND_STR(M_cursors, cursid, ret);
    
    NDRX_LOG(log_debug, "Find cursor [%s] result: %p", cursid, ret);
    
    return ret;
}

/**
 * Open new cursor. Assign the cursor id
 * @param p_ub UBF buffer where to install error, if any
 * @param map class map
 * @param clazz
 * @param data data to load into cursor
 * @return cursor object
 */
expublic ndrx_adm_cursors_t* ndrx_adm_curs_new(UBFH *p_ub, ndrx_adm_class_map_t *class_map,
        ndrx_adm_cursors_t *data)
{
    int ret = EXSUCCEED;
    char cursid[MAXTIDENT+1];
    ndrx_adm_cursors_t *el = NULL;
    
    if (M_nr_cursor+1 > ndrx_G_adm_config.cursors_max)
    {
        NDRX_LOG(log_error, "Currently open cursor limit (%d) reached - wait for houskeep, "
                "check program defects", ndrx_G_adm_config.cursors_max);
        ndrx_adm_error_set(p_ub, TAELIMIT, BBADFLDID, 
                "Currently open cursor limit (%d) reached - wait for houskeep, "
                "check program defects", ndrx_G_adm_config.cursors_max);
        EXFAIL_OUT(ret);
    }
    
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
    snprintf(cursid, sizeof(cursid), "%s_%s%09ld", ndrx_G_svcnm2, 
            class_map->clazzshort, M_cntr);
    
    /* Allocate the hash. */
    
    el = NDRX_MALLOC(sizeof(ndrx_adm_cursors_t));
    
    if (NULL==el)
    {
        NDRX_LOG(log_error, "Failed to malloc %d bytes: %s", sizeof(ndrx_adm_cursors_t),
                strerror(errno));
        EXFAIL_OUT(ret);
    }
    
    memcpy(el, data, sizeof(ndrx_adm_cursors_t));
    NDRX_STRCPY_SAFE(el->cursorid, cursid);
    ndrx_stopwatch_reset(&el->w);
    
    EXHASH_ADD_STR( M_cursors, cursorid, el );
    M_nr_cursor++;
out:
    
    if (EXSUCCEED!=ret && NULL!=el)
    {
        NDRX_FREE(el);
        el = NULL;
    }
    
    return el;
}

/**
 * Delete the cursor
 */
expublic void ndrx_adm_curs_close(ndrx_adm_cursors_t *curs)
{
    NDRX_LOG(log_debug, "Close/remove: [%s]", curs->cursorid);
    
    /* Remove from hash */
    EXHASH_DELETE(hh, M_cursors, curs);
    ndrx_growlist_free(&curs->list);
    NDRX_FREE(curs);
    M_nr_cursor--;
    
}

/**
 * This shall find the cursor, and perform the buffer mappings according to tables
 *  and structures.
 *  The buffer shall be received and filled accordingly. If fetched till the end
 *  the data item shall be deleted.
 * @param[out] p_ub where to fetch the cursor data
 * @param[in] curs which cursor to fetch
 * @return occurrences loaded/EXFAIL
 */
expublic int ndrx_adm_curs_fetch(UBFH *p_ub, ndrx_adm_cursors_t *curs)
{
    int ret = 0;
    int i;
    long freesz;
    void *cur_pos;
    ndrx_adm_elmap_t *map;
    int start_pos = curs->curspos;
    
    /*
     * - estimate free size one block + error block.
     * Load and step forward
     */
    
    NDRX_LOG(log_info, "Cursor [%s] curpos=%d total=%d", curs->cursorid, 
            curs->curspos, curs->list.maxindexused);
    
    for (i=start_pos; i<=curs->list.maxindexused; i++)
    {
        freesz = Bfree(p_ub);
        
        if (freesz < (curs->list.size*2) + TPADM_ERROR_MINSZ)
        {
            break;
        }
        
        
        /* Load the data according to current position & mapping */
        cur_pos = curs->list.mem + (curs->list.size*i);
        
        /* drive the mappings.. */
        map = curs->map;
        
        
        while (BBADFLDID!=map->fid)
        {
            BFLDOCC occ = i - start_pos;
            void *elm = cur_pos + curs->list.size*i;
            
            /* the C types matches UBF types! */
            if (EXSUCCEED!=Bchg(p_ub, map->fid, occ, elm, 0L))
            {
                NDRX_LOG(log_error, "Failed to set [%s] for occ %d: %s",
                        Bfname(map->fid), Bstrerror(Berror));
                EXFAIL_OUT(ret);
            }
            
            map++;
        }
        
        curs->curspos++;
        ret++;
    }
    
    NDRX_LOG(log_info, "Cursor [%s] fetch end at curpos=%d total=%d", curs->cursorid, 
            curs->curspos, curs->list.maxindexused);
    
    if (curs->curspos == curs->list.maxindexused)
    {
        NDRX_LOG(log_debug, "Cursor %d fetched fully -> remove", curs->cursorid);
        ndrx_adm_curs_close(curs);
    }
    
out:
    return ret;
}



/* vim: set ts=4 sw=4 et smartindent: */
