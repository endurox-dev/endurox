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
#include <singlegrp.h>
#include <Exfields.h>
#include <nstdutil.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define MAX_LOCKS   2
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

typedef struct
{
    int64_t lock_time;  /**< lock time in UTC epoch */
    uint64_t crc32;     /**< crc32 of the lock time */
} ndrx_exsinglesv_lockent_t;

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

    TP_LOG(log_debug, "Checking (%d) [%s] lock status...", lock_no, lockfile);

    if ( EXSUCCEED!=fcntl(M_locks[lock_no].fd, F_GETLK, &lck))
    {
        TP_LOG(log_info, "Failed to fcntl F_GETLK on [%s]: %s",
                    lockfile, strerror(errno));
        EXFAIL_OUT(ret);
    }

    /* 
     * we as lock owners, can lock it twice, thus must be reported as F_UNLOCK
     */
    if (lck.l_type!=F_UNLCK)
    {
        TP_LOG(log_error, "ERROR ! Lock file [%s] locked by other process (pid=%d)",
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

    TP_LOG(log_debug, "Trying to lock file (%d) [%s]", lock_no, lockfile);

    /* open lock file */
    if (EXFAIL==(M_locks[lock_no].fd = open(lockfile, O_RDWR|O_CREAT, 0666)))
    {
        TP_LOG(log_error, "Failed to open lock file [%s]: %s", 
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

            TP_LOG(log_info, "Failed to lock file [%s]: %s (pid=%d)",
                    lockfile, strerror(errno), (int)lck.l_pid);
            ret=NDRX_LOCKE_BUSY;
            goto out;
        }
        else
        {
            TP_LOG(log_error, "Failed to lock file [%s]: %s", 
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
        TP_LOG(log_error, "Lock file [%s] is not locked", 
                M_locks[lock_no].lockfile);
        ret=NDRX_LOCKE_NOLOCK;
        goto out;
    }

    lock.l_type = F_UNLCK;  /* Unlock */
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 1;  /* Unlock the entire file */

    if (fcntl(M_locks[lock_no].fd, F_SETLK, &lock) == -1) {
        TP_LOG(log_error, "Failed to unlock [%s]: %s", 
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
expublic int ndrx_exsinglesv_sg_is_locked(ndrx_locksm_ctx_t *lock_ctx)
{
    int ret=EXFALSE;
    UBFH *p_ub = NULL;
    long tmp, rsplen;
    char svcnm[MAXTIDENT+1]={EXEOS};

    /* check local */
    lock_ctx->pshm=ndrx_sg_get(ndrx_G_exsinglesv_conf.procgrp_lp_no);
    if (NULL==lock_ctx->pshm)
    {
        TP_LOG(log_error, "Failed to get singleton process group: %s", 
                ndrx_G_exsinglesv_conf.procgrp_lp_no);
        EXFAIL_OUT(ret);
    }

    /* Load group locally... */
    ndrx_sg_load(&lock_ctx->local, lock_ctx->pshm);

    ret = ndrx_sg_is_locked(ndrx_G_exsinglesv_conf.procgrp_lp_no, NULL, 0);
    /* check all remote */

    if (EXTRUE==ret && (lock_ctx->local.flags & NDRX_SG_VERIFY))
    {
        int i;

        for (i=0; i<CONF_NDRX_NODEID_COUNT; i++)
        {
            if (lock_ctx->local.sg_nodes[i])
            {
                TP_LOG(log_debug, "Checking node [%d]...", 
                        lock_ctx->local.sg_nodes[i]);

                if (NULL!=p_ub)
                {
                    tpfree((char *)p_ub);
                    p_ub = NULL;
                }

                p_ub = (UBFH *)tpalloc("UBF", NULL, 1024);

                if (NULL==p_ub)
                {
                    TP_LOG(log_error, "Failed to allocate UBF");
                    EXFAIL_OUT(ret);
                }

                /* load query buffer */

                tmp = ndrx_G_exsinglesv_conf.procgrp_lp_no;
                if (EXSUCCEED!=Bchg(p_ub, EX_COMMAND, 0, NDRX_SGCMD_QUERY, 0L)
                    || EXSUCCEED!=Bchg(p_ub, EX_PROCGRP_NO, 0, (char *)&tmp, 0L))
                {
                    TP_LOG(log_error, "Failed to setup request buffer: %s", 
                        Bstrerror(Berror));
                    EXFAIL_OUT(ret);
                }

                tpsblktime(ndrx_G_exsinglesv_conf.svc_timeout, TPBLK_NEXT);

                /* call server for results */
                snprintf(svcnm, sizeof(svcnm), NDRX_SVC_SINGL, (long)lock_ctx->local.sg_nodes[i], 
                    ndrx_G_exsinglesv_conf.procgrp_lp_no);
                TP_LOG(log_debug, "Checking with [%s] group %d lock status", svcnm, 
                    ndrx_G_exsinglesv_conf.procgrp_lp_no);

                ret = tpcall(svcnm, (char*)p_ub, 0L, (char **)&p_ub, &rsplen, TPNOTRAN);

                if (EXSUCCEED!=ret)
                {
                    TP_LOG(log_error, "Failed to call [%s]: %s", 
                        svcnm, tpstrerror(tperrno));

                    if (TPESVCFAIL==tperrno)
                    {
                        /* dump the reply buffer... */
                        ndrx_tplogprintubf(log_error, "svc error response", p_ub);
                    }

                    /* 
                     * if have their lock status & their are locked.
                     * we have lost the lock, return error.
                     * (unlock the shm). We will reply, however
                     * ndrxd may kill the group earlier.
                     */

                    /*
                     * if they are not locked from the service reply,
                     * our lock time must be newer than theirs.
                     * if not, we have lost the lock and return the error.
                     * (unlock the shm)
                     */

                }

            }
            else
            {
                /* array is sorted out of linked nodes */
                break;
            }
        }
        /* Do additional checks, if group flags require that... */

        /* loop over the nodes try to call them... */
    }

    /* if any fail, check local */
out:
    return ret;
}



/**
 * read 16 bytes for each of the nodes in the file
 * 8 bytes=> lock time, 8 bytes => crc32 (to match the E/X lib)
 */
expublic int ndrx_exsinglesv_ping_do(ndrx_locksm_ctx_t *lock_ctx)
{
    int fd=EXFAIL;
    struct stat st;
    int ret=EXSUCCEED;
    ssize_t bytes_written;
    struct flock lock;
    size_t fsize = sizeof(ndrx_exsinglesv_lockent_t)*CONF_NDRX_NODEID_COUNT;
    size_t offset=sizeof(ndrx_exsinglesv_lockent_t)*(ndrx_G_exsinglesv_conf.procgrp_lp_no-1);
    ndrx_exsinglesv_lockent_t ent;
    int i;

    fd = open(ndrx_G_exsinglesv_conf.lockfile_2, O_RDWR | O_CREAT, 0660);

    if (EXFAIL==fd)
    {
        NDRX_LOG(log_error, "Failed to open [%s]: %s", 
            ndrx_G_exsinglesv_conf.lockfile_2, strerror(errno));
        EXFAIL_OUT(ret);
    }

    /* Acquire an fcntl() write lock */
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;

    /* Lock the entire file */
    lock.l_len = 0;

    if (fcntl(fd, F_SETLK, &lock) == -1)
    {
        TP_LOG(log_error, "Failed to write lock [%s] (fd=%d) file: %s", 
            ndrx_G_exsinglesv_conf.lockfile_2, fd, strerror(errno));
        EXFAIL_OUT(ret);
    }

    if (fstat(fd, &st) == -1)
    {
        TP_LOG(log_error, "Failed to stat [%s] (fd=%d) file: %s", 
            ndrx_G_exsinglesv_conf.lockfile_2, fd, strerror(errno));
        EXFAIL_OUT(ret);
    }

    /* initialize the lock file.. */
    if (st.st_size == 0)
    {
        if (ftruncate(fd, fsize) == -1)
        {
            TP_LOG(log_error, "Truncate [%s] (fd=%d) to %d bytes failed: %s", 
                ndrx_G_exsinglesv_conf.lockfile_2, fd, fsize, strerror(errno));
            EXFAIL_OUT(ret);
        }

        /* reset structs to CRC32 valid */
        memset(&ent, 0, sizeof(ent));
        ndrx_Crc32_ComputeBuf(0, (unsigned char *)&ent.lock_time, sizeof(ent.lock_time));
        ent.lock_time = htonll(ent.lock_time);

        for (i=0; i<CONF_NDRX_NODEID_COUNT; i++)
        {
            bytes_written = write(fd, &ent, sizeof(ndrx_exsinglesv_lockent_t));

            if (lseek(fd, sizeof(ent)*i, SEEK_SET) == -1)
            {
                TP_LOG(log_error, "lseek [%s] (fd=%d) to offset %d bytes failed: %s", 
                        ndrx_G_exsinglesv_conf.lockfile_2, fd, sizeof(ent)*i, strerror(errno));
                EXFAIL_OUT(ret);
            }

            /* shall succeed for such small amount... */
            if (sizeof(ndrx_exsinglesv_lockent_t)!=bytes_written)
            {
                TP_LOG(log_error, "write [%s] (fd=%d) of %d bytes (wrote %zd) failed: %s", 
                        ndrx_G_exsinglesv_conf.lockfile_2, fd, sizeof(ndrx_exsinglesv_lockent_t), 
                        bytes_written, strerror(errno));
                EXFAIL_OUT(ret);
            }
            else
            {
                TP_LOG(log_debug, "Wrote %zd bytes at offset %d", bytes_written, offset);
            }
        }
    }

    if (lseek(fd, offset, SEEK_SET) == -1)
    {
        TP_LOG(log_error, "lseek [%s] (fd=%d) to offset %d bytes failed: %s", 
                ndrx_G_exsinglesv_conf.lockfile_2, fd, offset, strerror(errno));
        EXFAIL_OUT(ret);
    }

    /* prep data + chksum them */
    ent.lock_time = lock_ctx->local.last_refresh;
    ent.crc32 = ndrx_Crc32_ComputeBuf(0, (unsigned char *)&ent.lock_time, sizeof(ent.lock_time));

    /* convert ent fields to network format */
    ent.lock_time = htonll(ent.lock_time);
    ent.crc32 = htonll(ent.crc32);

    bytes_written = write(fd, &ent, sizeof(ndrx_exsinglesv_lockent_t));

    /* shall succeed for such small amount... */
    if (sizeof(ndrx_exsinglesv_lockent_t)!=bytes_written)
    {
        TP_LOG(log_error, "write [%s] (fd=%d) of %d bytes (wrote %zd) failed: %s", 
                ndrx_G_exsinglesv_conf.lockfile_2, fd, sizeof(ndrx_exsinglesv_lockent_t), 
                bytes_written, strerror(errno));
        EXFAIL_OUT(ret);
    }
    else
    {
        TP_LOG(log_debug, "Wrote %zd bytes at offset %d", bytes_written, offset);
    }

    /* flush the changes to the disk fully */
    if (EXSUCCEED!=fsync(fd))
    {
        TP_LOG(log_error, "fsync [%s] (fd=%d) failed: %s", 
            ndrx_G_exsinglesv_conf.lockfile_2, fd, strerror(errno)); 
        EXFAIL_OUT(ret);
    }

    /* Release the lock and close the file */
    lock.l_type = F_UNLCK;
    if (fcntl(fd, F_SETLK, &lock) == -1)
    {
        TP_LOG(log_error, "fcntl unlock [%s] (fd=%d) failed: %s", 
            ndrx_G_exsinglesv_conf.lockfile_2, fd, strerror(errno)); 
        EXFAIL_OUT(ret);
    }

out:

    if (EXFAIL!=fd)
    {
        close(fd);
    }

    return ret;
}

/**
 * Read the data from the ping file
 * @param lock_ctx locking context
 * @param procgrp_no process group number from which to get time-stamp
 * @return EXFAIL or UTC epoch lock time
 */
expublic long ndrx_exsinglesv_ping_read(ndrx_locksm_ctx_t *lock_ctx, int procgrp_no)
{
    long ret = EXFAIL; /**< tstamp read */
    size_t offset=sizeof(ndrx_exsinglesv_lockent_t)*(procgrp_no-1);
    ndrx_exsinglesv_lockent_t ent;
    int fd;
    struct flock lock;
    ssize_t bytes_read;

    /* Open the file for reading */
    fd = open(ndrx_G_exsinglesv_conf.lockfile_2, O_RDONLY);
    if (EXFAIL==fd)
    {
        NDRX_LOG(log_error, "Failed to open [%s]: %s", 
            ndrx_G_exsinglesv_conf.lockfile_2, strerror(errno));
        EXFAIL_OUT(ret);
    }

    lock.l_type = F_RDLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;

    if (fcntl(fd, F_SETLK, &lock) == EXFAIL)
    {
        TP_LOG(log_error, "Failed to read lock [%s] (fd=%d) file: %s", 
            ndrx_G_exsinglesv_conf.lockfile_2, fd, strerror(errno));
        EXFAIL_OUT(ret);
    }

    if (lseek(fd, offset, SEEK_SET) == -1)
    {
        TP_LOG(log_error, "lseek [%s] (fd=%d) to offset %d bytes failed: %s", 
                ndrx_G_exsinglesv_conf.lockfile_2, fd, offset, strerror(errno));
        EXFAIL_OUT(ret);
    }

    bytes_read = read(fd, &ent, sizeof(ent));
    if (bytes_read == -1)
    {
        TP_LOG(log_error, "read [%s] (fd=%d) of %d bytes (wrote %zd) failed: %s", 
                ndrx_G_exsinglesv_conf.lockfile_2, fd, sizeof(ndrx_exsinglesv_lockent_t), 
                bytes_read, strerror(errno));
        EXFAIL_OUT(ret);
    } 
    else
    {
        TP_LOG(log_debug, "Read %zd bytes at offset %d: for grp: %d", 
            bytes_read, offset, procgrp_no);
    }

    /* Release the lock and close the file */
    lock.l_type = F_UNLCK;
    if (fcntl(fd, F_SETLK, &lock) == -1)
    {
        TP_LOG(log_error, "fcntl unlock [%s] (fd=%d) failed: %s", 
            ndrx_G_exsinglesv_conf.lockfile_2, fd, strerror(errno)); 
        EXFAIL_OUT(ret);
    }

    /* verify the record...,
     * firstly convert to host format */

    ent.lock_time = ntohll(ent.lock_time);
    ent.crc32 = ntohll(ent.crc32);

    if (ent.crc32!=ndrx_Crc32_ComputeBuf(0, (unsigned char *)&ent.lock_time, sizeof(ent.lock_time)))
    {
        TP_LOG(log_warn, "CRC32 mismatch for group %d", procgrp_no);
        /* return zero time... (as really no time to return),
         * as storge did not fail, but not to read yet.
         */
        ret=0;
    }

out:
    if (EXFAIL!=fd)
    {
        close(fd);
    }

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
