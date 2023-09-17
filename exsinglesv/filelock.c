/**
 * @brief File locking routines
 *
 * @file filelock.c
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
#include <signal.h>

#include <ndebug.h>
#include <atmi.h>
#include <atmi_int.h>
#include <typed_buf.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <ubfutil.h>
#include <cconfig.h>
#include "exsinglesv.h"
#include <fcntl.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define MAX_LOCKS   2
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/**
 * Locking structure 
*/
typedef struct 
{
    int fd;         /**< file-fd locked         */
    char lockfile[PATH_MAX+1]; /**< lock file              */
} ndrx_exsinglesv_filelock_t;

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/**
 * Locks used.
 */
exprivate ndrx_exsinglesv_filelock_t M_locks[MAX_LOCKS] = {
        {.fd=EXFAIL, .lockfile=""},
        {.fd=EXFAIL, .lockfile=""}
};

/*---------------------------Prototypes---------------------------------*/

/**
 * Check PID of the the lock file (file must be locked)
 * @param lock_no lock number (file numbers)
 * @param lockfile lock file
 * @return EXSUCCEED/EXFAIL
 */ 
expublic int ndrx_exsinglesv_file_chkpid(int lock_no, char *lockfile)
{
    int ret = EXSUCCEED;
    struct flock lck;
    
    memset(&lck, 0, sizeof(lck));

    lck.l_whence = SEEK_SET;
    lck.l_start = 0;
    lck.l_len = 1;
    lck.l_type = F_WRLCK;

    NDRX_LOG(log_debug, "Checking (%d) [%s] lock status...", lock_no, lockfile);

    if ( EXSUCCEED!=fcntl(M_locks[lock_no].fd, F_GETLK, &lck))
    {
        NDRX_LOG(log_info, "Failed to fcntl F_GETLK on [%s]: %s",
                    lockfile, strerror(errno));
        EXFAIL_OUT(ret);
    }

    /* 
     * we as lock owners, can lock it twice, thus must be reported as F_UNLOCK
     */
    if (lck.l_type!=F_UNLCK)
    {
        NDRX_LOG(log_error, "ERROR ! Lock file [%s] locked by other process (pid=%d)",
                    lockfile, (int)lck.l_pid);
        EXFAIL_OUT(ret);
    }

out:
    return ret;
}

/**
 * Perform locking on the file
 * @param lock_no lock number
 * @param lockfile lock file
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_exsinglesv_file_lock(int lock_no, char *lockfile)
{
    int ret = EXSUCCEED;
    struct flock lck;
    
    memset(&lck, 0, sizeof(lck));

    lck.l_whence = SEEK_SET;
    lck.l_start = 0;
    lck.l_len = 1;
    lck.l_type = F_WRLCK;

    NDRX_LOG(log_debug, "Trying to lock file (%d) [%s]", lock_no, lockfile);

    /* open lock file */
    if (EXFAIL==(M_locks[lock_no].fd = open(lockfile, O_RDWR|O_CREAT, 0666)))
    {
        NDRX_LOG(log_error, "Failed to open lock file [%s]: %s", 
                lockfile, strerror(errno));
        ret=NDRX_LOCKE_FILE;
        goto out;
    }

    /* lock the region with fcntl() */
    if (EXFAIL==fcntl(M_locks[lock_no].fd, F_SETLK, &lck))
    {
        if (EACCES==errno || EAGAIN==errno)
        {
            /* memset(&lck, 0, sizeof(lck)); */
            fcntl(M_locks[lock_no].fd, F_GETLK, &lck);

            NDRX_LOG(log_info, "Failed to lock file [%s]: %s (pid=%d)",
                    lockfile, strerror(errno), (int)lck.l_pid);
            ret=NDRX_LOCKE_BUSY;
            goto out;
        }
        else
        {
            NDRX_LOG(log_error, "Failed to lock file [%s]: %s", 
                lockfile, strerror(errno));
            ret=NDRX_LOCKE_LOCK;
            goto out;
        }
    }

    NDRX_STRCPY_SAFE(M_locks[lock_no].lockfile, lockfile);

out:

    /* close the file in case of lock failure */
    if (EXSUCCEED!=ret && M_locks[lock_no].fd!=EXFAIL)
    {
        close(M_locks[lock_no].fd);
        M_locks[lock_no].fd = EXFAIL;
    }
    return ret;
}

/**
 * Unlock the file
 * @param lock_no lock number
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_exsinglesv_file_unlock(int lock_no)
{
    int ret = EXSUCCEED;
    struct flock lock;

    if (EXFAIL==M_locks[lock_no].fd)
    {
        NDRX_LOG(log_error, "Lock file [%s] is not locked", 
                M_locks[lock_no].lockfile);
        ret=NDRX_LOCKE_NOLOCK;
        goto out;
    }

    lock.l_type = F_UNLCK;  /* Unlock */
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 1;  /* Unlock the entire file */

    if (fcntl(M_locks[lock_no].fd, F_SETLK, &lock) == -1) {
        NDRX_LOG(log_error, "Failed to unlock [%s]: %s", 
                M_locks[lock_no].lockfile, strerror(errno));
        ret=NDRX_LOCKE_LOCK;
        goto out;
    }

out:

    /* close anyway... */
    close(M_locks[lock_no].fd);
    M_locks[lock_no].fd = EXFAIL;

    return ret;
}

/**
 * Extended group check.
 * @return EXFAIL/EXTRUE/EXFALSE
 */
expublic int ndrx_exsinglesv_sg_is_locked(void)
{
    /* check local */

    /* check all remote */

    /* if any fail, check local */

    return EXFAIL;
}

/* vim: set ts=4 sw=4 et smartindent: */
