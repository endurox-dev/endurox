/**
 * @brief Cache Show
 *
 * @file cmd_cs.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or Mavimax's license for commercial use.
 * -----------------------------------------------------------------------------
 * GPL license:
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
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
#include "nclopt.h"
#include <ndrx.h>
#include <qcommon.h>
#include <atmi_cache.h>
#include <ubfutil.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Print header
 * @return
 */
exprivate void print_hdr(char *dbname)
{
    fprintf(stderr, "ND  UTC DATE ADDED      HITS  TY DLEN  KEY\n");
    fprintf(stderr, "--- ------------------- ----- -- ----- ----------------------------------------\n");
}

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
    
    ndrx_debug_dump_UBF(log_debug, "Got reply buffer", p_ub);
    
    if (EXSUCCEED!=ndrx_cache_mgt_ubf2data(p_ub, &cdata, NULL, &keydata, NULL, NULL))
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
exprivate int call_cache(char *dbname)
{
    UBFH *p_ub = (UBFH *)tpalloc("UBF", "", 1024);
    int ret=EXSUCCEED;
    int cd;
    long revent;
    int recv_continue = 1;
    int tp_errno;
    long rcv_count = 0;
    char *svcnm;
    char cmd = NDRX_CACHE_SVCMD_CLSHOW;
    
    
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
    
    if (EXFAIL == (cd = tpconnect(svcnm,
                                    (char *)p_ub,
                                    0,
                                    TPNOTRAN |
                                    TPRECVONLY)))
    {
        NDRX_LOG(log_error, "Connect error [%s]", tpstrerror(tperrno) );
        fprintf(stderr, "Connect error [%s]\n", tpstrerror(tperrno));
        ret = EXFAIL;
        goto out;
    }
    NDRX_LOG(log_debug, "Connected OK, cd = %d", cd );

    while (recv_continue)
    {
        recv_continue=0;
        if (EXFAIL == tprecv(cd,
                            (char **)&p_ub,
                            0L,
                            0L,
                            &revent))
        {
            ret = EXFAIL;
            tp_errno = tperrno;
            if (TPEEVENT == tp_errno)
            {
                    if (TPEV_SVCSUCC == revent)
                            ret = EXSUCCEED;
                    else
                    {
                        NDRX_LOG(log_error,
                                 "Unexpected conv event %lx", revent );
                        EXFAIL_OUT(ret);
                    }
            }
            else
            {
                NDRX_LOG(log_error, "recv error %d", tp_errno  );
                EXFAIL_OUT(ret);
            }
        }
        else
        {
            if (EXSUCCEED!=print_buffer(p_ub, svcnm))
            {
                EXFAIL_OUT(ret);
            }
            rcv_count++;
            recv_continue=1;
        }
    }
    
    fprintf(stderr, "%ld records cached in \"%s\" database\n", rcv_count, dbname);

out:

    if (NULL!=p_ub)
    {
        tpfree((char *)p_ub);
    }

    return ret;
}

/**
 * Cache Show command
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
expublic int cmd_cs(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret=EXSUCCEED;
    char dbname[NDRX_CCTAG_MAX+1]={EXEOS};
    ncloptmap_t clopt[] =
    {
        {'d', BFLD_STRING, (void *)dbname, sizeof(dbname), 
                                NCLOPT_MAND|NCLOPT_HAVE_VALUE, "Database name"},
        {0}
    };
        
    if (argc>=2 && '-'!=argv[1][0])
    {
        NDRX_STRCPY_SAFE(dbname, argv[1]);
    }
    else
    {
        /* parse command line */
        if (nstd_parse_clopt(clopt, EXTRUE,  argc, argv, EXFALSE))
        {
            fprintf(stderr, XADMIN_INVALID_OPTIONS_MSG);
            EXFAIL_OUT(ret);
        }
    }

    /* we need to init TP subsystem... */
    if (EXSUCCEED!=tpinit(NULL))
    {
        fprintf(stderr, "Failed to tpinit(): %s\n", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    print_hdr(dbname);
    
    if (EXSUCCEED!=call_cache(dbname))
    {
        NDRX_LOG(log_debug, "Failed to call cache server for db [%s]", dbname);
        fprintf(stderr, "Failed to call cache server!\n");
        EXFAIL_OUT(ret);
    }
        
out:
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
