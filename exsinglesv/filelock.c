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
#define MAX_LOCKS       2
#define DATA_SIZE       24 /* full block for node */
#define DATA_FOR_CRC    16 /* full block for node */
#define PING_READ_CRC_ERR  -2
#define PING_NO_FILE  -3
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
 * @param lock_ctx lock context
 * @param chk_files check files (do not call remote at all)
 * @return EXFAIL/EXTRUE/EXFALSE
 */
expublic int ndrx_exsinglesv_sg_is_locked(ndrx_locksm_ctx_t *lock_ctx, int force_chk)
{
    int ret=EXFALSE;
    UBFH *p_ub = NULL;
    long tmp, rsplen;
    char svcnm[MAXTIDENT+1]={EXEOS};
    int locked_by_stat;
    char lckstatus;
    long last_refresh;
    long their_sequence;
    int is_net_rply;
    int i;
    ndrx_exsinglesv_lockent_t ent;

    /* check local */
    lock_ctx->pshm=ndrx_sg_get(ndrx_G_exsinglesv_conf.procgrp_lp_no);
    if (NULL==lock_ctx->pshm)
    {
        TP_LOG(log_error, "Failed to get singleton process group: %d", 
                ndrx_G_exsinglesv_conf.procgrp_lp_no);
        EXFAIL_OUT(ret);
    }

    /* Load group locally... */
    ndrx_sg_load(&lock_ctx->local, lock_ctx->pshm);

    ret = ndrx_sg_is_locked(ndrx_G_exsinglesv_conf.procgrp_lp_no, NULL, 0);

    /* check all remote */
    if (EXTRUE!=ret)
    {
        TP_LOG(log_info, "Group %d is not locked by shm", 
                ndrx_G_exsinglesv_conf.procgrp_lp_no);
        goto out;
    }

    if (!(lock_ctx->local.flags & NDRX_SG_VERIFY) && !force_chk)
    {
        TP_LOG(log_info, "Group %d verification is not required", 
                ndrx_G_exsinglesv_conf.procgrp_lp_no);
        goto out;
    }

    /* do the checks of the nodes */
    for (i=0; i<CONF_NDRX_NODEID_COUNT; i++)
    {
        int locked_by_stat=EXFALSE;
        last_refresh = 0;

        if (lock_ctx->local.sg_nodes[i])
        {
            if (lock_ctx->local.sg_nodes[i]==G_atmi_env.our_nodeid)
            {
                continue;
            }
            TP_LOG(log_debug, "Checking node [%d]...", 
                    lock_ctx->local.sg_nodes[i]);

            if (NULL!=p_ub)
            {
                tpfree((char *)p_ub);
                p_ub = NULL;
            }

            /* Try remote (if not disabled) */
            ret=EXSUCCEED;
            if (!ndrx_G_exsinglesv_conf.noremote)
            {                
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
                snprintf(svcnm, sizeof(svcnm), NDRX_SVC_SGREM, (long)lock_ctx->local.sg_nodes[i], 
                    ndrx_G_exsinglesv_conf.procgrp_lp_no);
                TP_LOG(log_debug, "Checking with [%s] group %d lock status", svcnm, 
                    ndrx_G_exsinglesv_conf.procgrp_lp_no);

                ret = tpcall(svcnm, (char*)p_ub, 0L, (char **)&p_ub, &rsplen, TPNOTRAN);
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
            if (EXSUCCEED!=ret || ndrx_G_exsinglesv_conf.noremote)
            {
                int log_lev = log_error;

                /* previous was service call... */
                if (EXSUCCEED!=ret)
                {
                    if (TPESVCFAIL==tperrno)
                    {
                        /* dump the reply buffer... */
                        ndrx_tplogprintubf(log_error, "svc error response", p_ub);
                    }
                    else  if (TPENOENT==tperrno)
                    {
                        log_lev = log_debug;
                    }

                    TP_LOG(log_lev, "Failed to call [%s]: %s - falling back to disk check", 
                        svcnm, tpstrerror(tperrno));

                    /*  succeed anyway */
                    ret=EXSUCCEED;
                }

                /* read the node entry from the disk */
                if (EXSUCCEED!=ndrx_exsinglesv_ping_read(lock_ctx->local.sg_nodes[i], &ent))
                {
                    TP_LOG(log_error, "Failed to ping lock file [%s] for node %d"
                        , ndrx_G_exsinglesv_conf.lockfile_2, (int)lock_ctx->local.sg_nodes[i]);
                    userlog("Failed to ping lock file [%s] for node %d"
                        , ndrx_G_exsinglesv_conf.lockfile_2, (int)lock_ctx->local.sg_nodes[i]);
                    EXFAIL_OUT(ret);
                }

                /* load the values down.. */
                last_refresh = ent.lock_time;
                their_sequence = ent.sequence;
                is_net_rply=EXFALSE;
            }
            else
            {
                ndrx_tplogprintubf(log_debug, "svc response", p_ub);
                /* read lock status... */
                if (EXSUCCEED!=Bget(p_ub, EX_LCKSTATUS, 0, &lckstatus, 0L) 
                    || EXSUCCEED!=Bget(p_ub, EX_TSTAMP, 0, (char *)&last_refresh, 0L)
                    || EXSUCCEED!=Bget(p_ub, EX_SEQUENCE, 0, (char *)&their_sequence, 0L))
                {
                    /* mandatory field is missing in reply */
                    TP_LOG(log_error, "Failed to get EX_LCKSTATUS/EX_TSTAMP/EX_SEQUENCE from node %d: %s", 
                        (int)lock_ctx->local.sg_nodes[i], Bstrerror(Berror));
                    EXFAIL_OUT(ret);
                }

                if (EXTRUE==lckstatus)
                {
                    locked_by_stat=EXTRUE;
                }

                is_net_rply=EXTRUE;
            }

            /* take the decsion */
            if (locked_by_stat)
            {
                TP_LOG(log_error, "Node %d is locked according to network status. "
                    "Their refresh time: %ld, our time: %ld - UNLOCKING local group %d", 
                    (int)lock_ctx->local.sg_nodes[i], last_refresh, lock_ctx->local.last_refresh,
                    ndrx_G_exsinglesv_conf.procgrp_lp_no);
                userlog("Node %d is locked according to network status. "
                    "Their refresh time: %ld, our time: %ld - UNLOCKING local group %d", 
                    (int)lock_ctx->local.sg_nodes[i], last_refresh, lock_ctx->local.last_refresh,
                    ndrx_G_exsinglesv_conf.procgrp_lp_no);

                ndrx_sg_unlock(lock_ctx->pshm, NDRX_SG_RSN_NETLOCK);
                EXFAIL_OUT(ret);
            }

            if (their_sequence>=lock_ctx->local.sequence)
            {
                TP_LOG(log_error, "Node %d has higher sequence than us. "
                    "their seq: %ld, our seq: %ld, their refresh time: %ld, our time: %ld - UNLOCKING local group %d. Source: %s", 
                    (int)lock_ctx->local.sg_nodes[i], their_sequence, lock_ctx->local.sequence, last_refresh, lock_ctx->local.last_refresh,
                    ndrx_G_exsinglesv_conf.procgrp_lp_no, is_net_rply?"network":"disk");
                userlog("Node %d has higher sequence than us. "
                    "their seq: %ld, our seq: %ld, their refresh time: %ld, our time: %ld - UNLOCKING local group %d. Source: %s", 
                    (int)lock_ctx->local.sg_nodes[i], their_sequence, lock_ctx->local.sequence, last_refresh, lock_ctx->local.last_refresh,
                    ndrx_G_exsinglesv_conf.procgrp_lp_no, is_net_rply?"network":"disk");

                if (is_net_rply)
                {
                    ndrx_sg_unlock(lock_ctx->pshm, NDRX_SG_RSN_NETSEQ);
                }
                else
                {
                    ndrx_sg_unlock(lock_ctx->pshm, NDRX_SG_RSN_FSEQ);
                }

                EXFAIL_OUT(ret);
            }
        }
        else
        {
            /* array is sorted out of linked nodes */
            break;
        }
    }

    /* if we are here, then we are locked fine */
    ret=EXTRUE;

    /* if any fail, check local */
out:

    if (NULL!=p_ub)
    {
        tpfree((char *)p_ub);
    }

    return ret;
}

/**
 * read 16 bytes for each of the nodes in the file
 * 8 bytes=> lock time, 8 bytes => crc32 (to match the E/X lib).
 * Note that no parallel locks are allowed. If any lock
 * is found on the file, it shall be reported as error.
 */
expublic int ndrx_exsinglesv_ping_do(ndrx_locksm_ctx_t *lock_ctx)
{
    int fd=EXFAIL;
    struct stat st;
    int ret=EXSUCCEED;
    ssize_t bytes_written;
    struct flock lock;
    size_t fsize = DATA_SIZE*CONF_NDRX_NODEID_COUNT*2;
    ndrx_exsinglesv_lockent_t ent;
    int i, copy;
    char data_block[DATA_SIZE];

    /* write our statistics to two places ...
     * so that if we crash during the first write, the
     * last record sayes OK... and other node can recover from it
     * to keep the counter growing past the crash.
     */
    for (copy=0; copy<2; copy++)
    {
        size_t offset= DATA_SIZE * CONF_NDRX_NODEID_COUNT * copy 
            + DATA_SIZE*(G_atmi_env.our_nodeid-1);

        fd = open(ndrx_G_exsinglesv_conf.lockfile_2, O_RDWR | O_CREAT, 0660);

        if (EXFAIL==fd)
        {
            TP_LOG(log_error, "copy %d: Failed to open [%s]: %s", 
                copy, ndrx_G_exsinglesv_conf.lockfile_2, strerror(errno));
            EXFAIL_OUT(ret);
        }

        /* Acquire an fcntl() write lock */
        lock.l_type = F_WRLCK;
        lock.l_whence = SEEK_SET;
        lock.l_start = 0;

        /* Lock the entire file */
        lock.l_len = 0;

        if (fcntl(fd, F_SETLKW, &lock) == -1)
        {
            TP_LOG(log_error, "copy %d: Failed to write-lock [%s] (fd=%d) file: %s", 
                copy, ndrx_G_exsinglesv_conf.lockfile_2, fd, strerror(errno));
            EXFAIL_OUT(ret);
        }

        if (fstat(fd, &st) == -1)
        {
            TP_LOG(log_error, "copy %d: Failed to stat [%s] (fd=%d) file: %s", 
                copy, ndrx_G_exsinglesv_conf.lockfile_2, fd, strerror(errno));
            EXFAIL_OUT(ret);
        }

        /* initialize the lock file.. */
        if (st.st_size == 0)
        {
            if (ftruncate(fd, fsize) == -1)
            {
                TP_LOG(log_error, "copy %d: Truncate [%s] (fd=%d) to %d bytes failed: %s", 
                    copy, ndrx_G_exsinglesv_conf.lockfile_2, fd, fsize, strerror(errno));
                EXFAIL_OUT(ret);
            }

            /* reset structs to CRC32 valid */            
            memset(data_block, 0, DATA_FOR_CRC);
            ent.crc32=ndrx_Crc32_ComputeBuf(0, (unsigned char *)data_block, DATA_FOR_CRC);
            ent.crc32 = htonll(ent.crc32);
            memcpy(data_block+DATA_FOR_CRC, &ent.crc32, sizeof(ent.crc32));

            /* create location for secondary location too... */
            for (i=0; i<CONF_NDRX_NODEID_COUNT*2; i++)
            {
                size_t init_offset=DATA_SIZE * i;

                if (lseek(fd, init_offset, SEEK_SET) == -1)
                {
                    TP_LOG(log_error, "copy %d: lseek [%s] (fd=%d) to offset %d bytes failed: %s", 
                            copy, ndrx_G_exsinglesv_conf.lockfile_2, fd, init_offset, strerror(errno));
                    EXFAIL_OUT(ret);
                }

                bytes_written = write(fd, data_block, DATA_SIZE);

                /* shall succeed for such small amount... */
                if (DATA_SIZE!=bytes_written)
                {
                    TP_LOG(log_error, "copy %d, write [%s] (fd=%d) of %d bytes (wrote %zd) failed: %s", 
                            copy, ndrx_G_exsinglesv_conf.lockfile_2, fd, DATA_SIZE, 
                            bytes_written, strerror(errno));
                    EXFAIL_OUT(ret);
                }
                else
                {
                    TP_LOG(log_debug, "copy %d: Wrote %zd bytes at offset %d", copy, bytes_written, offset);
                }
            }
        }

        if (lseek(fd, offset, SEEK_SET) == -1)
        {
            TP_LOG(log_error, "copy %d: lseek [%s] (fd=%d) to offset %d bytes failed: %s", 
                    copy, ndrx_G_exsinglesv_conf.lockfile_2, fd, offset, strerror(errno));
            EXFAIL_OUT(ret);
        }

        /* write new sequence down there... */
        /* and write new refresh time down there too... */
        ent.lock_time = htonll(lock_ctx->new_refresh);
        ent.sequence = htonll(lock_ctx->new_sequence);

        /* copy to memory buffer */
        memcpy(data_block, &ent.lock_time, sizeof(ent.lock_time));
        memcpy(data_block+sizeof(ent.lock_time), &ent.sequence, sizeof(ent.sequence));
        ent.crc32=ndrx_Crc32_ComputeBuf(0, (unsigned char *)data_block, DATA_FOR_CRC);

        /* Dump data... */
        TP_LOG(log_info, "copy %d: writting nodeid=%d group=%d "
            "refresh=%ld sequence=%ld crc32=%lx", 
            copy, G_atmi_env.our_nodeid, ndrx_G_exsinglesv_conf.procgrp_lp_no, 
            (long)lock_ctx->new_refresh, (long)lock_ctx->new_sequence, (long)ent.crc32);

        ent.crc32 = htonll(ent.crc32);
        memcpy(data_block+DATA_FOR_CRC, &ent.crc32, sizeof(ent.crc32));

        bytes_written = write(fd, data_block, DATA_SIZE);

        /* shall succeed for such small amount... */
        if (DATA_SIZE!=bytes_written)
        {
            TP_LOG(log_error, "copy %d: write [%s] (fd=%d) of %d bytes (wrote %zd) failed: %s", 
                    copy, ndrx_G_exsinglesv_conf.lockfile_2, fd, DATA_SIZE, 
                    bytes_written, strerror(errno));
            EXFAIL_OUT(ret);
        }
        else
        {
            TP_LOG(log_debug, "copy %d: Wrote %zd bytes at offset %d", 
                copy, bytes_written, offset);
        }

        /* flush the changes to the disk fully */
        if (EXSUCCEED!=fsync(fd))
        {
            TP_LOG(log_error, "copy %d: fsync [%s] (fd=%d) failed: %s", 
                copy, ndrx_G_exsinglesv_conf.lockfile_2, fd, strerror(errno)); 
            EXFAIL_OUT(ret);
        }

        /* Release the lock and close the file */
        lock.l_type = F_UNLCK;
        if (fcntl(fd, F_SETLK, &lock) == -1)
        {
            TP_LOG(log_error, "copy %d: fcntl unlock [%s] (fd=%d) failed: %s", 
                copy, ndrx_G_exsinglesv_conf.lockfile_2, fd, strerror(errno)); 
            EXFAIL_OUT(ret);
        }

        if (EXFAIL!=fd)
        {
            close(fd);
            fd=EXFAIL;
        }
    }

out:

    if (EXFAIL!=fd)
    {
        close(fd);
    }

    return ret;
}

/**
 * Read the data from the ping file.
 *  This totally does 3x attempts:
 *   1) if first one failed, try to read from the second file
 *   2) if second failed, try first, just in case if first was in progress of write / crashed
 *   3) if second try again first, just in case if first was in progress of write
 * @param copy which copy to read
 * @param nodeid 
 * @param p_ent entry to read
 * @return EXFAIL or UTC epoch lock time, or -2 if CRC error
 */
exprivate int ndrx_exsinglesv_ping_read_int(int copy, int nodeid, ndrx_exsinglesv_lockent_t *p_ent)
{
    int ret = EXSUCCEED;
    int fd;
    struct flock lock;
    ssize_t bytes_read;
    size_t offset=DATA_SIZE * CONF_NDRX_NODEID_COUNT * copy + DATA_SIZE*(nodeid-1);
    char data_block[DATA_SIZE];
    uint64_t crc32_calc;

    /* Open the file for reading */
    fd = open(ndrx_G_exsinglesv_conf.lockfile_2, O_RDONLY);
    if (EXFAIL==fd)
    {
        TP_LOG(log_error, "Failed to open [%s]: %s", 
            ndrx_G_exsinglesv_conf.lockfile_2, strerror(errno));

        /* if file is not found, return PING_NO_FILE */
        if (ENOENT==errno)
        {
            ret=PING_NO_FILE;
            goto out;
        }
        
        EXFAIL_OUT(ret);
    }

    lock.l_type = F_RDLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;

    TP_LOG(log_debug, "Acquring read lock on fd %d", fd);

    if (fcntl(fd, F_SETLKW, &lock) == EXFAIL)
    {
        TP_LOG(log_error, "copy %d: Failed to read lock [%s] (fd=%d) file: %s", 
            copy, ndrx_G_exsinglesv_conf.lockfile_2, fd, strerror(errno));
        EXFAIL_OUT(ret);
    }

    TP_LOG(log_debug, "Read locked on fd %d", fd);

    if (lseek(fd, offset, SEEK_SET) == -1)
    {
        TP_LOG(log_error, "copy %d: lseek [%s] (fd=%d) to offset %d bytes failed: %s", 
                copy, ndrx_G_exsinglesv_conf.lockfile_2, fd, offset, strerror(errno));
        EXFAIL_OUT(ret);
    }

    bytes_read = read(fd, data_block, DATA_SIZE);
    if (bytes_read != DATA_SIZE)
    {
        TP_LOG(log_error, "copy %d: read [%s] (fd=%d) of %d bytes (wrote %zd) failed: %s", 
                copy, ndrx_G_exsinglesv_conf.lockfile_2, fd, DATA_SIZE, 
                bytes_read, strerror(errno));
        EXFAIL_OUT(ret);
    } 
    else
    {
        TP_LOG(log_debug, "copy %d: Read %zd bytes at offset %d: for node: %d (fd=%d)", 
            copy, bytes_read, offset, nodeid, fd);
    }

    /* Release the lock and close the file */
    lock.l_type = F_UNLCK;
    if (fcntl(fd, F_SETLK, &lock) == -1)
    {
        TP_LOG(log_error, "copy %d: fcntl unlock [%s] (fd=%d) failed: %s", 
            copy, ndrx_G_exsinglesv_conf.lockfile_2, fd, strerror(errno)); 
        EXFAIL_OUT(ret);
    }

    /* extract from buffer */
    memcpy(&p_ent->lock_time, data_block, sizeof(p_ent->lock_time));
    memcpy(&p_ent->sequence, data_block+sizeof(p_ent->lock_time), sizeof(p_ent->sequence));
    memcpy(&p_ent->crc32, data_block+DATA_FOR_CRC, sizeof(p_ent->crc32));

    /* convert to host format */
    p_ent->lock_time = ntohll(p_ent->lock_time);
    p_ent->crc32 = ntohll(p_ent->crc32);
    p_ent->sequence = ntohll(p_ent->sequence);

    TP_LOG(log_info, "copy %d: got nodeid=%d group=%d "
            "refresh=%lu sequence=%lu crc32=%lx", 
            copy, nodeid, ndrx_G_exsinglesv_conf.procgrp_lp_no, 
            (long)p_ent->lock_time, (long)p_ent->sequence, (long)p_ent->crc32);

    crc32_calc = ndrx_Crc32_ComputeBuf(0, (unsigned char *)data_block, DATA_FOR_CRC);

    if (p_ent->crc32!=crc32_calc)
    {
        TP_LOG(log_warn, "copy %d: CRC32 mismatch (disk: %lx calc: %lx) file [%s] offset: %d", copy,
            (long)p_ent->crc32, (long)crc32_calc, ndrx_G_exsinglesv_conf.lockfile_2, (int)offset);
        ret=PING_READ_CRC_ERR;
    }

out:
    if (EXFAIL!=fd)
    {
        close(fd);
    }

    return ret;
}

/**
 * Try to extract lock entry from file.
 * @param nodeid node id
 * @param p_ent entry to read
 * @return EXFAIL or EXSUCCEED (p_ent loaded with CRC32 validate values)
 */
expublic int ndrx_exsinglesv_ping_read(int nodeid, ndrx_exsinglesv_lockent_t *p_ent)
{
    int i, ret;

    for (i=0; i<3; i++)
    {
        ret=ndrx_exsinglesv_ping_read_int(i<2?i:0, nodeid, p_ent);

        if (ret==PING_READ_CRC_ERR)
        {
            continue;
        }
        else
        {
            /* we have a verified result */
            break;
        }
    }

    /* give some advice if lock file is totally corrupt... */
    if (PING_READ_CRC_ERR==ret)
    {
        TP_LOG(log_error, "Failed to read ping lock file for node %d - corrupted file. "
            "Shutdown the cluster, remove lock file [%s] and boot again.", 
            nodeid, ndrx_G_exsinglesv_conf.lockfile_2);

        userlog("Failed to read ping lock file for node %d - corrupted file. "
            "Shutdown the cluster, remove lock file [%s] and boot again.", 
            nodeid, ndrx_G_exsinglesv_conf.lockfile_2);
        EXFAIL_OUT(ret);
    }

out:

    TP_LOG(log_info, "%s returns %d", __FUNCTION__, ret);
    return ret;
}

/**
 * Read max sequence number from files for all the nodes...
 * Used during intial lock and pinging...
 * If file does not exist, j
 * @return max sequence number as seen in lock files
 */
expublic long ndrx_exsinglesv_sg_max_seq(ndrx_locksm_ctx_t *lock_ctx)
{
    int ret=EXSUCCEED;
    long seq_max=0;
    int i;
    ndrx_exsinglesv_lockent_t ent;

    for (i=0; i<CONF_NDRX_NODEID_COUNT; i++)
    {
        if (lock_ctx->local.sg_nodes[i])
        {
            TP_LOG(log_debug, "Checking node [%d]...", 
                    lock_ctx->local.sg_nodes[i]);

            ret = ndrx_exsinglesv_ping_read(lock_ctx->local.sg_nodes[i], &ent);

            /* no file, max sequence is 0 */
            if (ret==PING_NO_FILE)
            {
                /* we are done... */
                ret=0;
                break;
            }
            else if (EXSUCCEED!=ret)
            {
                TP_LOG(log_error, "Failed to distinguish current max sequence number for node %d", 
                    (int)lock_ctx->local.sg_nodes[i]);
                EXFAIL_OUT(ret);
            }

            if (ent.sequence > seq_max)
            {
                seq_max = ent.sequence;
            }
        }
    }

out:
    TP_LOG(log_info, "returns %d, max sequence %ld", ret, seq_max);

    if (EXSUCCEED==ret)
    {
        return seq_max;
    }
    else
    {
        return ret;
    }   
}

/* vim: set ts=4 sw=4 et smartindent: */
