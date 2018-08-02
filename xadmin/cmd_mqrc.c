/* 
 * @brief Reload Q config
 *
 * @file cmd_mqrc.c
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
#include <qcommon.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/


/**
 * Reload Q config
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
expublic int cmd_mqrc(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret = EXSUCCEED;
    atmi_svc_list_t *el, *tmp, *list;
    UBFH *p_ub = NULL;
    char cmd = TMQ_CMD_MQRC;
    long rsplen;
    
    /* we need to init TP subsystem... */
    if (EXSUCCEED!=tpinit(NULL))
    {
        fprintf(stderr, "Failed to tpinit(): %s\n", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    list = ndrx_get_svc_list(mqfilter);
    
    LL_FOREACH_SAFE(list,el,tmp)
    {
        UBFH *p_ub = (UBFH *)tpalloc("UBF", "", 1024);
        
        if (NULL==p_ub)
        {
            fprintf(stderr, "Failed to alloc call buffer%s\n", tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
        
        if (EXSUCCEED!=Bchg(p_ub, EX_QCMD, 0, &cmd, 0L))
        {
            fprintf(stderr, "Failed to set command: %s\n", Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
        
        printf("Updating %s ... ", list->svcnm);
        
        if (EXSUCCEED!=tpcall(list->svcnm, (char *)p_ub, 0L, 
                (char **)&p_ub, &rsplen, TPNOTRAN))
        {
            printf("Failed: %s\n", tpstrerror(tperrno));
        }
        else
        {
            printf("Succeed\n");
        }
        
        /* Have some housekeep. */
        LL_DELETE(list,el);
        NDRX_FREE(el);
        
        NDRX_FREE((char *)p_ub);
        p_ub = NULL;
    }
    
    printf("Done\n");
    
out:

    if (NULL!=p_ub)
    {
        NDRX_FREE((char *)p_ub);
        p_ub = NULL;
    }

    return ret;
}
