/* 
** Print UBF Database
**
** @file cmd_pubfdb.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2018, Mavimax, Ltd. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or Mavimax's license for commercial use.
** -----------------------------------------------------------------------------
** GPL license:
** 
** This program is free software; you can redistribute it and/or modify it under
** the terms of the GNU General Public License as published by the Free Software
** Foundation; either version 2 of the License, or (at your option) any later
** version.
**
** This program is distributed in the hope that it will be useful, but WITHOUT ANY
** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
** PARTICULAR PURPOSE. See the GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License along with
** this program; if not, write to the Free Software Foundation, Inc., 59 Temple
** Place, Suite 330, Boston, MA 02111-1307 USA
**
** -----------------------------------------------------------------------------
** A commercial use license is available from Mavimax, Ltd
** contact@mavimax.com
** -----------------------------------------------------------------------------
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <sys/param.h>

#include <ndrstandard.h>
#include <ndebug.h>

#include <ndrx.h>
#include <ndrxdcmn.h>
#include <atmi_int.h>
#include <gencall.h>
#include <nclopt.h>
#include <ubf.h>
#include <exdb.h>
#include <fdatatype.h>
#include <cconfig.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Print UBF database
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
expublic int cmd_pubfdb(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret=EXSUCCEED;
    EDB_env * dbenv;
    EDB_dbi *dbi_id, *dbi_nm;
    EDB_txn *txn = NULL;
    EDB_cursor * cursor;
    int tran_started = EXFALSE;
    EDB_cursor_op op;
    EDB_val keydb, data;
    int cursor_open = EXFALSE;
    BFLDID bfldid;
    BFLDID bfldno;
    short fldtype;
    char fldname[UBFFLDMAX+1];
    int first = EXTRUE;
    
    if (EXSUCCEED!=ndrx_cconfig_load())
    {
        fprintf(stderr, "ERROR ! Failed to load common-config\n");
        EXFAIL_OUT(ret);  
    }
    
    if (NULL==ndrx_get_G_cconfig())
    {
        fprintf(stderr, "NOTE: No common config defined!\n");
        goto out;
    }
    
    /* Load UBF fields (if no already loaded...) */
    if (EXSUCCEED!=Bflddbload())
    {
        fprintf(stderr, "ERROR ! Failed to load UBF field database: %s\n", 
                Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    /* get DB Env */
    if (NULL==(dbenv=Bfldddbgetenv(&dbi_id, &dbi_nm)))
    {
        fprintf(stderr, "ERROR ! Failed to load UBF DB env handler: %s\n", 
                Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    /* begin transaction */
    if (EXSUCCEED!=(ret=edb_txn_begin(dbenv, NULL, EDB_RDONLY, &txn)))
    {
        fprintf(stderr, "ERROR ! Failed to start LMDB transaction: %s\n", 
                edb_strerror(ret));
        EXFAIL_OUT(ret);
    }
    
    tran_started = EXTRUE;
    
    /* loop over the db... */
    
    if (EXSUCCEED!=(ret=edb_cursor_open(txn, *dbi_nm, &cursor)))
    {
        fprintf(stderr, "ERROR ! Failed to open cursor: %s\n", edb_strerror(ret));
        EXFAIL_OUT(ret);
    }
    cursor_open = EXTRUE;

    op = EDB_FIRST;
    do
    {
        if (EXSUCCEED!=(ret=edb_cursor_get(cursor, &keydb, &data, op)))
        {
            if (ret!=EDB_NOTFOUND)
            {
                fprintf(stderr, "ERROR ! cursor get failed: %s\n", edb_strerror(ret));
            }
            else
            {
                fprintf(stderr, "EOF\n");
            }
        }
        
        if (EXSUCCEED!=Bflddbget(&keydb, &data,
            &bfldno, &bfldid, 
            &fldtype, fldname, sizeof(fldname)))
        {
            fprintf(stderr, "ERROR ! failed to decode db data: %s\n", 
                    Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
        
        /* print the data on screen */
        if (first)
        {
            fprintf(stderr, "NO         ID         TYPE          Field Name\n");
            fprintf(stderr, "---------- ---------- ----------    --------------------\n");
        }
        printf("%-10d %-10d %-10.10s %s\n",
           bfldno, bfldid, G_dtype_str_map[fldtype].fldname, fldname);
        
        if (EDB_FIRST == op)
        {
            op = EDB_NEXT;
        }

    } while (EXSUCCEED==ret);
    
out:
    
    if (cursor_open)
    {
        edb_cursor_close(cursor);
    }

    if (tran_started)
    {
        edb_txn_abort(txn);
    }

    return ret;
}
