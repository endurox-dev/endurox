/**
 * @brief `aborttrans' aka `abort' command implementation
 *
 * @file cmd_abort.c
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
#include <ndrx.h>
#include <nclopt.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/


/**
 * Do the call to transaction manager
 * @param svcnm - Service name of transaction manager.
 * @return SUCCEED/FAIL
 */
exprivate int call_tm(char *svcnm, char *tmxid, short tmrmid)
{
    UBFH *p_ub = atmi_xa_alloc_tm_call(ATMI_XA_ABORTTRANS);
    int ret=EXSUCCEED;
    
    /* Setup the call buffer... */
    if (NULL==p_ub)
    {
        NDRX_LOG(log_error, "Failed to alloc FB!");        
        EXFAIL_OUT(ret);
    }
    
    /* Do The TM call */
    if (EXSUCCEED!=Bchg(p_ub, TMXID, 0, tmxid, 0L))
    {
        fprintf(stderr, "System error!\n");
        NDRX_LOG(log_error, "Failed to set TMXID: %s!", 
                Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    if (EXFAIL!=tmrmid)
    {
        if (EXSUCCEED!=Bchg(p_ub, TMTXRMID, 0, (char *)&tmrmid, 0L))
        {
            fprintf(stderr, "System error!\n");
            NDRX_LOG(log_error, "Failed to set TMTXRMID: %s!", 
                    Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
    }
    
    /* printf("!!! About to call [%s]", svcnm); */
    /* This will return ATMI error */
    if (NULL==(p_ub = atmi_xa_call_tm_generic_fb(ATMI_XA_ABORTTRANS, svcnm, EXFALSE, EXFAIL, 
        NULL, p_ub)))
    {
        EXFAIL_OUT(ret);
    }
    
out:

    if (NULL!=p_ub)
    {
        tpfree((char *)p_ub);
    }

    return ret;
}

/**
 * Abort transaction
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
expublic int cmd_abort(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret = EXSUCCEED;
    char tmxid_real[NDRX_XID_SERIAL_BUFSIZE+1];
    char srvcnm_real[MAXTIDENT+1];
    short confirm = EXFALSE;
    short tmrmid_real = EXFAIL;
    ncloptmap_t clopt[] =
    {
        {'y', BFLD_SHORT, (void *)&confirm, 0, 
                                NCLOPT_OPT | NCLOPT_TRUEBOOL, "Confirm"},
        {'t', BFLD_STRING, (void *)srvcnm_real, sizeof(tmxid_real), 
                                NCLOPT_MAND|NCLOPT_HAVE_VALUE, "TM reference"},
        {'x', BFLD_STRING, (void *)tmxid_real, sizeof(tmxid_real), 
                                NCLOPT_MAND|NCLOPT_HAVE_VALUE, "XID"},
        {'g', BFLD_SHORT, (void *)&tmrmid_real, 0, 
                                NCLOPT_OPT|NCLOPT_HAVE_VALUE, "Group No"},
        {0}
    };
    
    /* we need to init TP subsystem... */
    if (EXSUCCEED!=tpinit(NULL))
    {
        fprintf(stderr, "Failed to tpinit(): %s\n", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    /* parse command line */
    if (nstd_parse_clopt(clopt, EXTRUE,  argc, argv, EXFALSE))
    {
        fprintf(stderr, XADMIN_INVALID_OPTIONS_MSG);
        EXFAIL_OUT(ret);
    }
    
    /* Check for confirmation */
    if (!chk_confirm("Are you sure you want to abort the transaction?", confirm))
    {
        EXFAIL_OUT(ret);
    }
    
    /* call the transaction manager */
    if (EXSUCCEED!=call_tm(srvcnm_real, tmxid_real, tmrmid_real))
    {
        fprintf(stderr, "ERROR: %s\n", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    printf("OK\n");
    
out:
    return ret;
}

