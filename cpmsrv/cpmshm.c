/**
 * @brief Shared memory handler
 *
 * @file cpmshm.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * AGPL or Mavimax's license for commercial use.
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
#include <sys_mqueue.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <libxml/xmlreader.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include <ndrstandard.h>
#include <userlog.h>
#include <atmi.h>
#include <exenvapi.h>
#include <cpm.h>

#include "cpmsrv.h"
#include "../libatmisrv/srv_int.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Sync PIDs with SHM (after the boot, if crash recovery there shall be
 * something.
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_cpm_sync_from_shm(void)
{
    int ret = EXSUCCEED;
    cpm_process_t *c = NULL;
    cpm_process_t *ct = NULL;
    
    EXHASH_ITER(hh, G_clt_config, c, ct)
    {
        /* lookup the process */
        pid_t pid = ndrx_cltshm_getpid(c->key, c->stat.procname, 
                sizeof(c->stat.procname), &c->dyn.stattime);
        
        if (EXFAIL!=pid)
        {
            c->dyn.cur_state=CLT_STATE_STARTED;
            /* set current temp stamp */
            c->dyn.pid = pid;
        }
    }
    
out:
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
