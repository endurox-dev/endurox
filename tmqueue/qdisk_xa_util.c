/**
 * @brief Q XA Backend, utility functions
 *
 * @file qdisk_xa_util.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2023, Mavimax, Ltd. All Rights Reserved.
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
#include <math.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <assert.h>

#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>
#include <ndrstandard.h>
#include <nstopwatch.h>

#include <xa.h>
#include <atmi_int.h>
#include <unistd.h>

#include "userlog.h"
#include "tmqueue.h"
#include "nstdutil.h"
#include "Exfields.h"
#include "atmi_tls.h"
#include "tmqd.h"
#include <qcommon.h>
#include <xa_cmn.h>
#include <ubfutil.h>
#include "qtran.h"
#include <singlegrp.h>
#include <sys_test.h>
#include "qdisk_xa.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/


/**
 * Minimal XA call to tmqueue, used by external processes.
 * @param tmxid serialized xid
 * @param cmd TMQ_CMD* command code
 * @return XA error code
 */
expublic int ndrx_xa_qminicall(char *tmxid, char cmd)
{
    long rsplen;
    UBFH *p_ub = NULL;
    long ret = XA_OK;
    short nodeid = (short)tpgetnodeid();
   
    p_ub = (UBFH *)tpalloc("UBF", "", 1024 );
    
    if (NULL==p_ub)
    {
        NDRX_LOG(log_error, "Failed to allocate notif buffer");
        ret = XAER_RMERR;
        goto out;
    }
    
    if (EXSUCCEED!=Bchg(p_ub, EX_QCMD, 0, &cmd, 0L))
    {
        NDRX_LOG(log_error, "Failed to setup EX_QMSGID!");
        ret = XAER_RMERR;
        goto out;
    }
    
    if (EXSUCCEED!=Bchg(p_ub, TMXID, 0, tmxid, 0L))
    {
        NDRX_LOG(log_error, "Failed to setup TMXID!");
        ret = XAER_RMERR;
        goto out;
    }

    if (EXSUCCEED!=Bchg(p_ub, TMNODEID, 0, (char *)&nodeid, 0L))
    {
        NDRX_LOG(log_error, "Failed to setup TMNODEID!");
        ret = XAER_RMERR;
        goto out;
    }
    
    NDRX_LOG(log_info, "Calling QSPACE [%s] for tmxid [%s], command %c",
                ndrx_G_qspacesvc, tmxid, cmd);
    
    ndrx_debug_dump_UBF(log_info, "calling Q space with", p_ub);

    if (EXFAIL == tpcall(ndrx_G_qspacesvc, (char *)p_ub, 0L, (char **)&p_ub, 
        &rsplen, TPNOTRAN))
    {
        NDRX_LOG(log_error, "%s failed: %s", ndrx_G_qspacesvc, tpstrerror(tperrno));
        
        /* best guess -> not available */
        ret = XAER_RMFAIL;
        
        /* anyway if have detailed response, use bellow. */
    }
    
    ndrx_debug_dump_UBF(log_info, "Reply from RM", p_ub);
    
    /* try to get the result of the OP */
    if (Bpres(p_ub, TMTXRMERRCODE, 0) &&
            EXSUCCEED!=Bget(p_ub, TMTXRMERRCODE, 0, (char *)&ret, 0L))
    {
        NDRX_LOG(log_debug, "Failed to get TMTXRMERRCODE: %s", Bstrerror(Berror));
        ret = XAER_RMERR;
    }
    
out:

    if (NULL!=p_ub)
    {
        tpfree((char *)p_ub);
    }

    NDRX_LOG(log_info, "returns %d", ret);
    
    return ret;
}


/**
 * Minimal XA call to tmqueue, used by external processes.
 * @param tmxid serialized xid
 * @param cmd TMQ_CMD* command code
 * @param pp_ub request buffer (output on success)
 * @param flags ATMI flags used for call
 * @return connection descriptor, or -1 on error
 */
expublic int ndrx_xa_qminiconnect(char cmd, UBFH **pp_ub, long flags)
{
    long rsplen;
    UBFH *p_ub = NULL;
    long ret = XA_OK;
    short nodeid = (short)tpgetnodeid();
   
    *pp_ub = (UBFH *)tpalloc("UBF", "", 1024 );
    
    if (NULL==*pp_ub)
    {
        NDRX_LOG(log_error, "Failed to allocate notif buffer");
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=Bchg(*pp_ub, EX_QCMD, 0, &cmd, 0L))
    {
        NDRX_LOG(log_error, "Failed to setup EX_QMSGID!");
        EXFAIL_OUT(ret);
    }

    if (EXSUCCEED!=Bchg(*pp_ub, TMNODEID, 0, (char *)&nodeid, 0L))
    {
        NDRX_LOG(log_error, "Failed to setup TMNODEID!");
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_info, "Connecting to QSPACE [%s], command %c",
                ndrx_G_qspacesvc, cmd);
    
    ndrx_debug_dump_UBF(log_info, "Connecting to Q space with", p_ub);

    if (EXFAIL == tpconnect(ndrx_G_qspacesvc, (char *)p_ub, 0L, flags|TPNOTRAN))
    {
        NDRX_LOG(log_error, "%s tpconnect() failed: %s", ndrx_G_qspacesvc, tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }

out:

    if ( EXSUCCEED!=ret && NULL!=p_ub)
    {
        tpfree((char *)*pp_ub);
        *pp_ub = NULL;
    }

    NDRX_LOG(log_info, "returns %d ubf ptr %p", ret, *pp_ub);
    
    return ret;
}


/* vim: set ts=4 sw=4 et smartindent: */
