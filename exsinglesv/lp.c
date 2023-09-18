/**
 * @brief Lock provder server specifics
 *
 * @file lp.c
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <regex.h>
#include <utlist.h>
#include <unistd.h>
#include <assert.h>

#include <ndebug.h>
#include <atmi.h>
#include <atmi_int.h>
#include <typed_buf.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <ubfutil.h>
#include <cconfig.h>
#include "exsinglesv.h"
#include <Exfields.h>
#include <singlegrp.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
/**
 * Un-init procedure.
 * Close any open lock files.
 * @param normal_unlock shall we perform normal unlock (i.e. we were locked)
 * @param force_unlock shall we force unlock (i.e. we were not locked)
 */
expublic void ndrx_exsinglesv_uninit(int normal_unlock, int force_unlock)
{
    int do_unlock=EXFALSE;

    if (ndrx_G_exsinglesv_conf.locked1)
    {
        ndrx_exsinglesv_file_unlock(NDRX_LOCK_FILE_1);

        if (normal_unlock || force_unlock)
        {
            do_unlock = EXTRUE;
        }
    }
    else if (force_unlock)
    {
        do_unlock = EXTRUE;
    }

    if (ndrx_G_exsinglesv_conf.locked2)
    {
        ndrx_exsinglesv_file_unlock(NDRX_LOCK_FILE_2);
    }

    if (do_unlock)
    {
        ndrx_sg_shm_t * sg = ndrx_sg_get(ndrx_G_exsinglesv_conf.procgrp_lp_no);

        if (NULL!=sg)
        {
            ndrx_sg_unlock(sg, 
                force_unlock?NDRX_SG_RSN_LOCKE:NDRX_SG_RSN_NORMAL);
        }
    }

    ndrx_G_exsinglesv_conf.is_locked=EXFALSE;
}


/* vim: set ts=4 sw=4 et smartindent: */
