/* 
** Cache Dump - dump cache data to console
**
** @file cmd_cd.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, Mavimax, Ltd. All Rights Reserved.
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
#include <nstdutil.h>

#include <ndrxdcmn.h>
#include <atmi_int.h>
#include <gencall.h>
#include <utlist.h>
#include <Exfields.h>

#include "xa_cmn.h"
#include "nclopt.h"
#include <ndrx.h>
#include <qcommon.h>
#include <atmi_cache.h>
#include <typed_buf.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * List queue definitions
 * We will run in conversation mode.
 * @param svcnm
 * @return SUCCEED/FAIL
 */
exprivate int print_buffer(UBFH *p_ub, char *dbname)
{
    int ret = EXSUCCEED;
    ndrx_tpcache_data_t cdata;
    char *keydata = NULL;
    
    if (EXSUCCEED!=ndrx_cache_mgt_ubf2data(p_ub, &cdata, NULL, &keydata))
    {
        NDRX_LOG(log_error, "Failed to get mandatory UBF data!");
        EXFAIL_OUT(ret);
    }
    
    printf("%-3d %-19.19s %-5.5s %-2hd %-5.5s %s\n"
            ,cdata.nodeid
            ,ndrx_get_strtstamp_from_sec(0, cdata.t)
            ,ndrx_decode_num(cdata.hits, 0, 0, 1)
            ,cdata.atmi_type_id
            ,ndrx_decode_num(cdata.atmi_buf_len, 1, 0, 1)
            ,keydata
            );
    
out:

    if (NULL!=keydata)
    {
        NDRX_FREE(keydata);
    }

    return ret;
}

/**
 * Call cache server
 * @return
 */
exprivate int call_cache(char *dbname, char *key, int interpret)
{
    UBFH *p_ub = (UBFH *)tpalloc("UBF", "", 1024);
    int ret=EXSUCCEED;
    long rcvlen;
    char *svcnm;
    char cmd = NDRX_CACHE_SVCMD_CLCDUMP;
    long tperr;
    ndrx_tpcache_data_t cdata;
    char *keydata = NULL;
    char *data = NULL;
    
    svcnm = ndrx_cache_mgt_getsvc();
            
    /* Setup the call buffer... */
    if (NULL==p_ub)
    {
        NDRX_LOG(log_error, "Failed to alloc FB!");        
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=Bchg(p_ub, EX_CACHE_CMD, 0, &cmd, 0L))
    {
        NDRX_LOG(log_error, "Failed to install command code: %s", 
                Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=Bchg(p_ub, EX_CACHE_DBNAME, 0, dbname, 0L))
    {
        NDRX_LOG(log_error, "Failed to install db name to buffer: %s",
                Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    
    if (EXSUCCEED!=Bchg(p_ub, EX_CACHE_OPEXPR, 0, key, 0L))
    {
        NDRX_LOG(log_error, "Failed to install key to buffer: %s",
                Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    /* Call cache server! */
    if (EXSUCCEED!=tpcall(svcnm, (char *)p_ub, 0L, (char **)&p_ub, &rcvlen, 0L))
    {
        if (Bpres(p_ub, EX_TPSTRERROR, 0))
        {
            fprintf(stderr, "%s\n", Bfind(p_ub, EX_TPSTRERROR, 0, 0L));
        }
        else
        {
            NDRX_LOG(log_error, "Failed to call [%s]: %s", svcnm, tpstrerror(tperrno));
            fprintf(stderr, "Failed to call cache server [%s]: %s\n", 
                svcnm, tpstrerror(tperrno));
        }
        
        EXFAIL_OUT(ret);
    }
    
    /* Dump results to stdout (currently hex dump of buffer) 
     * If it is UBF buffer, then we might want to interpret it as UBF...
     */
    if (EXSUCCEED!=ndrx_cache_mgt_ubf2data(p_ub, &cdata, &data, &keydata))
    {
        NDRX_LOG(log_error, "Failed to get mandatory UBF data!");
        EXFAIL_OUT(ret);
    }
    
    ndrx_debug_dump_UBF(log_debug, "Got reply buffer", p_ub);
    
    /* Print the results */
    printf("nodeid: %hd\n", cdata.nodeid);
    printf("saved_tperrno: %d\n", cdata.saved_tperrno);
    printf("saved_tpurcode: %ld\n", cdata.saved_tpurcode);
    printf("time_added: %ld (%s)\n", cdata.t, ndrx_get_strtstamp_from_sec(0, cdata.t));
    printf("time_added_usec: %ld\n", cdata.tusec);
    printf("time_hit: %ld (%s)\n", cdata.hit_t, ndrx_get_strtstamp_from_sec(0, cdata.hit_t));
    printf("time_hit_usec: %ld\n", cdata.hit_tusec);
    printf("hits: %ld\n", cdata.hits);
    printf("atmi_type_id: %hd\n", cdata.atmi_type_id);
    printf("atmi_buf_len: %ld\n", cdata.atmi_buf_len);
    
    
    printf("================================== HEX DUMP ====================================\n");
    STDOUT_DUMP(log_info, "Massage dump", data, cdata.atmi_buf_len);
    printf("================================================================================\n\n");
    
    if (interpret)
    {
        UBFH *p_ubf_h = (UBFH *)data;
        /* interpret the results, currently we support UBF buffer only */
        
        if (BUF_TYPE_UBF==cdata.atmi_type_id)
        {
            printf("================================== UBF BUFFER ==================================\n");
            Bprint(p_ubf_h);
            printf("================================================================================\n");
        }
    }

out:

    if (NULL!=p_ub)
    {
        tpfree((char *)p_ub);
    }

    if (NULL!=keydata)
    {
        NDRX_FREE(keydata);
    }
    
    if (NULL!=data)
    {
        NDRX_FREE(data);
    }

    return ret;
}

/**
 * Cache Dump command
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
expublic int cmd_cd(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret=EXSUCCEED;
    char dbname[NDRX_CCTAG_MAX+1]={EXEOS};
    char key[PATH_MAX+1]={EXEOS};
    int interpret = EXFALSE;
    ncloptmap_t clopt[] =
    {
        {'d', BFLD_STRING, (void *)dbname, sizeof(dbname), 
                                NCLOPT_MAND|NCLOPT_HAVE_VALUE, "Database name"},
	{'k', BFLD_STRING, (void *)key, sizeof(key), 
                                NCLOPT_MAND|NCLOPT_HAVE_VALUE, "Key string"},
				
	/* should we interpret the result? For example UBF
	 * Boolean flag
	 */
	{'i', BFLD_SHORT, (void *)&interpret, sizeof(interpret), 
				NCLOPT_OPT|NCLOPT_TRUEBOOL, "Interpret?"},
        {0}
    };
        
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
    
    if (EXSUCCEED!=call_cache(dbname, key, interpret))
    {
        NDRX_LOG(log_debug, "Failed to call cache server for db [%s]", dbname);
        fprintf(stderr, "Failed to call cache server!\n");
        EXFAIL_OUT(ret);
    }
        
out:
    return ret;
}

