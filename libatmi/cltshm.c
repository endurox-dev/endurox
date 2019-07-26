/**
 * @brief Client admin shared memory handler
 *  Added to libatm so that tpadmsv can use it too.
 *
 * @file sem.c
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
#include <ndrx_config.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <sys/sem.h>

#include <atmi.h>
#include <atmi_shm.h>
#include <ndrstandard.h>
#include <ndebug.h>
#include <ndrxd.h>
#include <ndrxdcmn.h>
#include <userlog.h>

/* shm_* stuff, and mmap() */
#include <sys/mman.h>
#include <sys/types.h>
/* exit() etc */
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ipc.h>

#include <nstd_shm.h>
#include <cpm.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/



/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/*
 * 
 * TODO: Func for creating the segment, attaching to segment, get the results
 * created or attached.
 * 
 * get position for add
 * get position  for reading.
 * 
 * TWO mapping tables: TAG/SUBSECT -> PID, PID -> TAG/SUBSECT.
 * 
 * Needs new global setting for size of maps: 
 * Needs to store process name and have cpmsrv setting to validate proc name
 *  or not.
 * 
 * 
 */

/**
 * Get shm pos by pid
 * @param qid
 * @param oflag
 * @param pos
 * @param have_value
 * @return 
 */
expublic int ndrx_cltshm_get_pid(int qid, int oflag, int *pos, int *have_value)
{
    return EXFAIL;
}

/**
 * Get shm pos by key
 * @param key
 * @param oflag
 * @param pos
 * @param have_value
 * @return 
 */
expublic int ndrx_cltshm_get_key(char *key, int oflag, int *pos, int *have_value)
{
    return EXFAIL;
}

expublic int ndrx_cltshm_get_status_key(char *key)
{
    return EXFAIL;
}

expublic int ndrx_cltshm_get_status_pid(char *key)
{
    return EXFAIL;
}

expublic int ndrx_cltshm_set_status_key(char *key, int stat)
{
    return EXFAIL;
}

expublic int ndrx_cltshm_attach(int attach_on_exists)
{
    return EXFAIL;
}

expublic int ndrx_cltshm_close(void)
{
    return EXFAIL;
}

expublic int ndrx_cltshm_remove(void)
{
    return EXFAIL;
}

expublic int ndrx_cltshm_rlock(void)
{
    return EXFAIL;
}

expublic int ndrx_cltshm_runlock(void)
{
    return EXFAIL;
}

expublic void ndrx_cltshm_getptr(void)
{
    return EXFAIL;
}

/* vim: set ts=4 sw=4 et smartindent: */
