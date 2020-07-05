/**
 * @brief TMIB commands
 *
 * @file cmd_tmib.c
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
 * Print mib details
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
expublic int cmd_mibget(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret = EXSUCCEED;
    char clazz[MAXTIDENT+1];
    char lmid[MAXTIDENT+1] = {EXEOS};
    UBFH *p_ub = NULL;
    UBFH *p_ub_rsp = NULL;
    long have_more = EXTRUE;
    long flags = 0;
    int first = EXTRUE;
    long cnt = 0;
    long rsplen;
    short machine_fmt = EXFALSE;
    int i, j;
    char tmpbuf[1024];
    BFLDLEN flen;
    char msg[MAX_TP_ERROR_LEN+1] = {EXEOS};
    BFLDID fldid = 0;
    long error_code = 0;
    ndrx_adm_class_map_t *clazz_descr;
    ndrx_growlist_t table;
    ndrx_growlist_t coltypes;

    ncloptmap_t clopt[] =
    {
        {'c', BFLD_STRING, clazz, sizeof(clazz), 
                                NCLOPT_MAND|NCLOPT_HAVE_VALUE, "Class name"},
        /* - currently not supported
        {'l', BFLD_STRING, lmid, sizeof(lmid), 
                                NCLOPT_OPT|NCLOPT_HAVE_VALUE, "Machine ID (nodeid)"},
         */
        {'m', BFLD_SHORT, (void *)&machine_fmt, 0, 
                                NCLOPT_OPT | NCLOPT_TRUEBOOL, "Machine output"},
        {0}
    };
    
    
    ndrx_tab_init(&table);
    ndrx_growlist_init(&coltypes, 100, sizeof(long));
    
    /* parse command line */
    if (nstd_parse_clopt(clopt, EXTRUE,  argc, argv, EXFALSE))
    {
        fprintf(stderr, XADMIN_INVALID_OPTIONS_MSG);
        EXFAIL_OUT(ret);
    }
    
    /* we need to init TP subsystem... */
    if (EXSUCCEED!=tpinit(NULL))
    {
        fprintf(stderr, "Failed to tpinit(): %s\n", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    /* let error to be handled by TMIB */
    clazz_descr = ndrx_adm_class_map_get(clazz);
    
    /* alloc 1kb.. */
    p_ub = (UBFH *)tpalloc("UBF", NULL, 0);
    
    if (NULL==p_ub)
    {
        fprintf(stderr, "Failed to allocate call buffer: %s\n", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    if (EXEOS!=lmid[0])
    {
        flags|=MIB_LOCAL;
    }
    
    /* call the service */
    
    if (EXFAIL==Bchg(p_ub, TA_OPERATION, 0, NDRX_TA_GET, 0L)
                || EXFAIL==Bchg(p_ub, TA_CLASS, 0, clazz, 0)
                ||  EXFAIL==Bchg(p_ub, TA_FLAGS, 0, (char *)&flags, 0)
                ||  (EXEOS!=lmid[0] && EXFAIL==Bchg(p_ub, TA_LMID, 0, lmid, 0))
                )
    {
        NDRX_LOG(log_error, "Failed to setup UBF buffer: %s", Bstrerror(Berror));
        fprintf(stderr, "Failed to setup UBF buffer: %s\n", Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }

    while (have_more)
    {
        have_more = 0;
        
        ndrx_debug_dump_UBF(log_info, ".TMIB Request buffer:", p_ub);
        
        if (EXFAIL==tpcall(NDRX_SVC_TMIB, (char *)p_ub, 0, 
                (char **)&p_ub_rsp, &rsplen, 0))
        {
            if (TPESVCFAIL==tperrno && NULL!=p_ub_rsp)
            {
                
                ndrx_debug_dump_UBF(log_info, ".TMIB Response (ERR) buffer:", p_ub_rsp);
                
                /* read the error fields and print details... */
                Bget(p_ub_rsp, TA_ERROR, 0, (char *)&error_code, 0L);
                Bget(p_ub_rsp, TA_STATUS, 0, msg, 0L);
                Bget(p_ub_rsp, TA_BADFLD, 0, (char *)&fldid, 0L);
                
                fprintf(stderr, "TMIB failed with %ld on field %u: %s\n", 
                        error_code, (unsigned int)fldid, msg);
                EXFAIL_OUT(ret);
            }
            else
            {   
                NDRX_LOG(log_error, "Failed to call [%s]: %s", NDRX_SVC_TMIB, 
                        tpstrerror(tperrno));
                fprintf(stderr, "Failed to call [%s]: %s\n", NDRX_SVC_TMIB, 
                        tpstrerror(tperrno));
            }
            
            EXFAIL_OUT(ret);
        }
        
        ndrx_debug_dump_UBF(log_info, ".TMIB Response buffer:", p_ub_rsp);
        

        if (EXSUCCEED!=Bget(p_ub_rsp, TA_OCCURS, 0, (char *)&cnt, NULL))
        {
            NDRX_LOG(log_error, "Failed to get TA_OCCURS: %s", Bstrerror(Berror));
            fprintf(stderr, "Failed to get TA_OCCURS: %s\n", Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
        
        if (EXSUCCEED!=Bget(p_ub_rsp, TA_MORE, 0, (char *)&have_more, NULL))
        {
            NDRX_LOG(log_error, "Failed to get TA_MORE: %s", Bstrerror(Berror));
            fprintf(stderr, "Failed to get TA_MORE: %s\n", Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
        
        if (cnt > 0)
        {
            if (first)
            {
                /* print headers.. for selected class ... */
                for (i=0; BBADFLDID!=clazz_descr->fields_map[i].fid; i++)
                {
                    long typ;
                    
                    if (machine_fmt)
                    {
                        fprintf(stderr, "%s|", clazz_descr->fields_map[i].name);
                    }
                    else
                    {
                        /* Load into linked list... */
                        if (EXSUCCEED!=ndrx_tab_add_col(&table, i, 
                                clazz_descr->fields_map[i].name))
                        {
                            fprintf(stderr, "Failed to prepare results\n");
                        }
                        
                        typ = Bfldtype(clazz_descr->fields_map[i].fid);
                        
                        if (TA_DOMAINID==clazz_descr->fields_map[i].fid ||
                                TA_LMID==clazz_descr->fields_map[i].fid)
                        {
                            /* Enduro/X machine id is number */
                            typ=BFLD_SHORT;
                        }
                        
                        /* set column types */
                        if (EXSUCCEED!=ndrx_growlist_add(&coltypes, (char *)&typ, 
                                coltypes.maxindexused+1))
                        {
                            NDRX_LOG(log_error, "Failed to add column type code");
                            fprintf(stderr, "Failed to add column type code\n");
                            EXFAIL_OUT(ret);
                        }
                    }
                }
                
                if (machine_fmt)
                {
                    fprintf(stderr, "\n");
                }
                
                first = EXFALSE;
            }
            
            /* print the results on screen... */
            for (i=0; i<cnt; i++)
            {
                for (j=0; BBADFLDID!=clazz_descr->fields_map[j].fid; j++)
                {
                    flen = sizeof(tmpbuf);
                    
                    if (EXSUCCEED!=CBget(p_ub_rsp, clazz_descr->fields_map[j].fid, 
                            i, tmpbuf, &flen, BFLD_STRING))
                    {
                        
                        NDRX_LOG(log_error, "Failed to get [%s] at %d occ: %s",
                                Bfname(clazz_descr->fields_map[j].fid), i, 
                                Bstrerror(Berror));
                        /* Bfname resets Berror! */
                        fprintf(stderr, "Failed to get [%s] at %d occ\n",
                                Bfname(clazz_descr->fields_map[j].fid), i);
                        EXFAIL_OUT(ret);
                    }
                    
                    if (machine_fmt)
                    {
                        fprintf(stdout, "%s|", tmpbuf);
                    }
                    else
                    {
                        /* Load into linked list... */
                        if (EXSUCCEED!=ndrx_tab_add_col(&table, j, tmpbuf))
                        {
                            fprintf(stderr, "Failed to prepare results\n");
                        }
                    }
                }
                
                if (machine_fmt)
                {
                    fprintf(stdout, "\n");
                }
            }
        }
        
        if (have_more)
        {
            char cursid[MAXTIDENT+1];
            
            if (EXSUCCEED!=Bchg(p_ub, TA_OPERATION, 0, NDRX_TA_GETNEXT, 0L))
            {
                NDRX_LOG(log_error, "Failed to setup UBF buffer: %s", 
                        Bstrerror(Berror));
                fprintf(stderr, "Failed to setup UBF buffer: %s\n", 
                        Bstrerror(Berror));
                EXFAIL_OUT(ret);
            }
            
            flen = sizeof(cursid);
            
            if (EXSUCCEED!=Bget(p_ub_rsp, TA_CURSOR, 0, cursid, &flen))
            {
                NDRX_LOG(log_error, "Failed to get TA_CURSOR: %s", 
                        Bstrerror(Berror));
                fprintf(stderr, "Failed to get TA_CURSOR: %s\n", 
                        Bstrerror(Berror));
                EXFAIL_OUT(ret);
            }
            
            if (EXSUCCEED!=Bchg(p_ub, TA_CURSOR, 0, cursid, 0L))
            {
                NDRX_LOG(log_error, "Failed to setup TA_CURSOR: %s", 
                        Bstrerror(Berror));
                fprintf(stderr, "Failed to setup TA_CURSOR: %s\n", 
                        Bstrerror(Berror));
                EXFAIL_OUT(ret);
            }
        }
    } /* while have more */
    
    if (!machine_fmt)
    {
        ndrx_tab_print(&table, &coltypes);
    }
out:
    
    if (NULL!=p_ub)
    {
        tpfree((char *)p_ub);
    }

    if (NULL!=p_ub_rsp)
    {
        tpfree((char *)p_ub_rsp);
    }

    ndrx_tab_free(&table);
    ndrx_growlist_free(&coltypes);


    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
