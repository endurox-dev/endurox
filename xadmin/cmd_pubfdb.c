/**
 * @brief Print UBF Database
 *
 * @file cmd_pubfdb.c
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
    
    /* Load UBF fields (if no already loaded...) */
    if (EXFAIL==(ret=Bflddbload()))
    {
        fprintf(stderr, XADMIN_ERROR_FORMAT_PFX "Failed to load UBF field database: %s\n", 
                Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    if (EXTRUE!=ret)
    {
        fprintf(stderr, "WARNING ! No configuration defined for UBF DB!\n");
        goto out;
    }
    
    /* reset back to succeed */
    ret = EXSUCCEED;
    
    /* get DB Env */
    if (NULL==(dbenv=Bfldddbgetenv(&dbi_id, &dbi_nm)))
    {
        fprintf(stderr, XADMIN_ERROR_FORMAT_PFX "Failed to load UBF DB env handler: %s\n", 
                Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    /* begin transaction */
    if (EXSUCCEED!=(ret=edb_txn_begin(dbenv, NULL, EDB_RDONLY, &txn)))
    {
        fprintf(stderr, XADMIN_ERROR_FORMAT_PFX "Failed to start LMDB transaction: %s\n", 
                edb_strerror(ret));
        EXFAIL_OUT(ret);
    }
    
    tran_started = EXTRUE;
    
    /* loop over the db... */
    
    if (EXSUCCEED!=(ret=edb_cursor_open(txn, *dbi_nm, &cursor)))
    {
        fprintf(stderr, XADMIN_ERROR_FORMAT_PFX "Failed to open cursor: %s\n",
            edb_strerror(ret));
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
                fprintf(stderr, XADMIN_ERROR_FORMAT_PFX "cursor get failed: %s\n",
                    edb_strerror(ret));
            }
            goto out;
        }
        
        if (EXSUCCEED!=Bflddbget(&data, &fldtype, &bfldno, &bfldid, 
                fldname, sizeof(fldname)))
        {
            fprintf(stderr, XADMIN_ERROR_FORMAT_PFX "failed to decode db data: %s\n", 
                    Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
        
        /* print the data on screen */
        if (first)
        {
            fprintf(stderr, "NO         ID         TYPE       Field Name\n");
            fprintf(stderr, "---------- ---------- ---------- --------------------\n");
            first = EXFALSE;
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

    Bflddbunload();

    return ret;
}
/* vim: set ts=4 sw=4 et smartindent: */
