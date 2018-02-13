/* 
** Cache Invalidate - delete cache records
**
** @file cmd_ci.c
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
 * Call cache server
 * @return
 */
exprivate int call_cache(char *dbname, char *key, int regexp)
{
    UBFH *p_ub = (UBFH *)tpalloc("UBF", "", 1024);
    int ret=EXSUCCEED;
    long rcvlen;
    char *svcnm;
    char cmd = NDRX_CACHE_SVCMD_CLDEL;
    long flags = 0;
    long deleted;
    
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
    
    
    if (EXEOS!=key[0])
    {
        if (EXSUCCEED!=Bchg(p_ub, EX_CACHE_OPEXPR, 0, key, 0L))
        {
            NDRX_LOG(log_error, "Failed to install key to buffer: %s",
                    Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
    }
    
    if (regexp)
    {
        flags|=NDRX_CACHE_SVCMDF_DELREG;
    }
    
    if (EXSUCCEED!=Bchg(p_ub, EX_CACHE_FLAGS, 0, (char *)&flags, 0L))
    {
        NDRX_LOG(log_error, "Failed to set EX_CACHE_FLAGS: %s",
                    Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    /* Call cache server! */
    if (EXSUCCEED!=tpcall(svcnm, (char *)p_ub, 0L, (char **)&p_ub, &rcvlen, 0L))
    {
        NDRX_LOG(log_error, "Failed to call [%s]: %s", svcnm, tpstrerror(tperrno));
        fprintf(stderr, "Failed to call cache server [%s]: %s\n", 
                svcnm, tpstrerror(tperrno));
        
        if (Bpres(p_ub, EX_TPSTRERROR, 0))
        {
            fprintf(stderr, "%s\n", Bfind(p_ub, EX_TPSTRERROR, 0, 0L));
        }
        
        EXFAIL_OUT(ret);
    }
    
    if (Bpres(p_ub, EX_CACHE_NRDEL, 0))
    {
        if (EXSUCCEED!=Bget(p_ub, EX_CACHE_NRDEL, 0, (char *)&deleted, 0L))
        {
            NDRX_LOG(log_error, "Failed to get EX_CACHE_NRDEL: %s", 
                    Bstrerror(Berror));
            printf("WARNING: Failed to get delete results from EX_CACHE_NRDEL: %s\n", 
                    Bstrerror(Berror));
        }
        else
        {
            printf("%ld records deleted\n", deleted);
        }
    }
    
out:

    if (NULL!=p_ub)
    {
        tpfree((char *)p_ub);
    }

    return ret;
}

/**
 * Invalidate/Delete cache records
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
expublic int cmd_ci(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret=EXSUCCEED;
    char dbname[NDRX_CCTAG_MAX+1]={EXEOS};
    char key[PATH_MAX+1]={EXEOS};
    int regex = EXFALSE;
    ncloptmap_t clopt[] =
    {
        {'d', BFLD_STRING, (void *)dbname, sizeof(dbname), 
                                NCLOPT_MAND|NCLOPT_HAVE_VALUE, "Database name"},
	{'k', BFLD_STRING, (void *)key, sizeof(key), 
                                NCLOPT_OPT|NCLOPT_HAVE_VALUE, "Key string"},
				
	/* should we interpret the result? For example UBF
	 * Boolean flag
	 */
	{'r', BFLD_SHORT, (void *)&regex, sizeof(regex), 
				NCLOPT_OPT|NCLOPT_TRUEBOOL, "Use regexp?"},
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
    
    if (EXSUCCEED!=call_cache(dbname, key, regex))
    {
        NDRX_LOG(log_debug, "Failed to call cache server for db [%s]", dbname);
        fprintf(stderr, "Failed to call cache server!\n");
        EXFAIL_OUT(ret);
    }

out:
    return ret;
}

