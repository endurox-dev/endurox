/**
 * @brief Table output driver (column with calculator)
 *
 * @file taboutput.c
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <sys/param.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <nstdutil.h>

#include <ndrxdcmn.h>
#include <atmi_int.h>
#include <gencall.h>
#include <utlist.h>
#include <Exfields.h>

#include "xa_cmn.h"
#include "tpadmsv.h"
#include <ndrx.h>
#include <qcommon.h>
#include <nclopt.h>
#include <tpadm.h>
#include <ubfutil.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Init the grow list
 * @param table table of output
 */
expublic void ndrx_tab_init(ndrx_growlist_t *table)
{
    ndrx_growlist_init(table, 100, sizeof(ndrx_growlist_t*));
}

/**
 * Add data to column row
 * @param table table holder
 * @param col_nr column number
 * @param str string to add
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_tab_add_col(ndrx_growlist_t *table, int col_nr, char *str)
{
    ndrx_growlist_t *col_new;
    ndrx_growlist_t **col_cur;
    int ret = EXSUCCEED;
    char *dups;
    long *width;
    int len;
    
    if (col_nr > table->maxindexused)
    {
        col_new = NDRX_MALLOC(sizeof(ndrx_growlist_t));
        
        if (NULL==col_new)
        {
            NDRX_MALLOC_FAIL_NDRX(sizeof(ndrx_growlist_t));
            EXFAIL_OUT(ret);
        }
        
        ndrx_growlist_init(col_new, 100, sizeof(char *));
        
        if (EXSUCCEED!=ndrx_growlist_add(table, (char *)&col_new, col_nr))
        {
            NDRX_LOG(log_error, "Failed to add to growlist");
            EXFAIL_OUT(ret);
        }
    }
    
    col_cur = (ndrx_growlist_t **)(table->mem + sizeof(ndrx_growlist_t *)*col_nr);
    
    /* calculate width on the fly */
    
    if (EXFAIL==(*col_cur)->maxindexused)
    {
        /* this is start we store in ptr the col with -> do not interpret first val */
        width = (long *)0;
        if (EXSUCCEED!=ndrx_growlist_add(*col_cur, (char *)&width, (*col_cur)->maxindexused+1))
        {
            NDRX_LOG(log_error, "Failed to add column width entry at 0");
            EXFAIL_OUT(ret);
        }
    }
    
    width = (long *)((*col_cur)->mem);
    
    dups = NDRX_STRDUP(str);
    
    if (NULL==dups)
    {
        NDRX_MALLOC_FAIL_NDRX(sizeof(char *));
        EXFAIL_OUT(ret);
    }
    
    /* assume ptr is long ... */
    len = strlen(dups);
    if ( (long)*width < len)
    {
        *width = len;
    }

    if (EXSUCCEED!=ndrx_growlist_add(*col_cur, (char *)&dups, (*col_cur)->maxindexused+1))
    {
        NDRX_LOG(log_error, "Failed to add column at row %d to table", col_nr);
        EXFAIL_OUT(ret);
    }
        
out:
    return ret;
}

/**
 * Print the table...
 * First row is column with
 * Second row is title
 * and following are data
 * @param table ptr to table
 */
expublic void ndrx_tab_print(ndrx_growlist_t *table)
{
    int col, row = 0;
    ndrx_growlist_t **col_cur;
    char **val_cur;
    long *width;
    int first = 0;
            
    while (1)
    {
        for (col=0; col<=table->maxindexused; col++)
        {
            
            col_cur = (ndrx_growlist_t **)(table->mem + sizeof(ndrx_growlist_t *)*col);
            
            if (row > (*col_cur)->maxindexused)
            {
                goto out;
            }
                
            val_cur = (*col_cur)->mem + row*sizeof(char *);
            width = (*col_cur)->mem;
            
            if (1==row)
            {
                if (0==first)
                {
                    fprintf(stderr, "%-*s ", (int)*width, *val_cur);
                }
                else
                {
                    int i;
                    for (i=0; i<(int)*width; i++)
                    {
                        fprintf(stderr, "-");
                    }
                    fprintf(stderr, " ");

                }
                /*print footers ... */
            }
            else if (row > 1)
            {
                fprintf(stdout, "%-*s ", (int)*width, *val_cur);
            }
        }
        
        if (1==row)
        {
            fprintf(stderr, "\n");
        }
        else if (row > 1)
        {
            fprintf(stdout, "\n");
        }

        if (row==1)
        {
            first++;
            if (first>1)
            {
                row++;
            }
        }
        else
        {
            row++;
        }
    }
    
    
out:
    
    return;
}

/**
 * Free up the table
 * @param table
 */
expublic void ndrx_tab_free(ndrx_growlist_t *table)
{
    int col, row = 0;
    ndrx_growlist_t **col_cur;
    char **val_cur;
            
    for (col=0; col<=table->maxindexused; col++)
    {
        col_cur = (ndrx_growlist_t **)(table->mem + sizeof(ndrx_growlist_t *)*col);
        
        for (row=1; row<=(*col_cur)->maxindexused; row++)
        {
            val_cur = (*col_cur)->mem + row*sizeof(char *);
            NDRX_FREE(*val_cur);
        }
        
        ndrx_growlist_free(*col_cur);
        NDRX_FREE(*col_cur);
    }

    ndrx_growlist_free(table);
}

/* vim: set ts=4 sw=4 et smartindent: */
