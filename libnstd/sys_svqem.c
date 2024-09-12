/**
 * @brief Emulated System-V message queue
 *  Emulates: msgget, msgsnd, msgrcv, msgctl
 *  Will be used as part of the SystemVEM messaging.
 *  + journal...
 * @file sys_svqem.c
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

/*---------------------------Includes-----------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <sys/sem.h>
#include <sys/mman.h>

#include <ndrstandard.h>
#include "sys_unix.h"

#include "ndebug.h"

#include <lcfint.h>
#include <stddef.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys_svq.h>

/*---------------------------Externs------------------------------------*/
extern ndrx_sem_t ndrx_G_svqem_semc;
extern ndrx_sem_t ndrx_G_svqem_seml;
extern ndrx_shm_t ndrx_G_svqem_shm;
extern int volatile __thread M_signalled;
/*---------------------------Macros-------------------------------------*/
#define MSG_OVERHEAD (sizeof(struct ndrx_msgbuf))

/* #define EXTRA_DEBUG 1 */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/**
 * Message queue header
 */
typedef struct
{
    unsigned long              svqem_pos;   /**< offset from IPC key (shifted) */
    unsigned long              svqem_head;  /**< first message index           */
    unsigned long              svqem_free;  /**< first free message index      */
    unsigned long              svqem_nwait; /**< number of threads waiting     */
    unsigned long              svqem_curmsgs;  /**< number of messages in Q    */

    /* Journal data (replicate after msg data installed and all fields set,
     * flag svqem_nwait_j is set to true. Any operation on Q will
     * trigger replication (as we do not know at which point the
     * sender did crash, thus to avoid corruption, data is journaled
     * and journal is replicated, after that svqem_dirty_j is cleared:
     */


} ndrx_svqem_hdr_t;

/**
 * Message header
 */
typedef struct
{
    volatile ssize_t         msg_len;     /**< actual length                     */
    volatile long            msg_next;    /**< next msg index (aligned access)   */

} ndrx_svqem_msg_hdr_t;


struct ndrx_msgbuf
{
    long mtype;       /* message type, must be > 0 */
};
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
exprivate void validate_q(ndrx_svqem_hdr_t *hdr);

/**
 *  ndrx_svqem_msgget, ndrx_svqem_msgsnd, ndrx_svqem_msgrcv, ndrx_svqem_msgctl
 * - Implements fixed blocks in shared memory, internally linked with header
 * - Implements journal of last pointer swap
 * - Uses two seamphores for mutex lock / conditional checks
 * - In case if condition is signalled more than needed, no problem, the next waiter on q will just consume few more empty reads ?
 * - shared mems: - Emulation over the system- shared memory, ipckey+20 + Nr of Queues
 * - msgid => shmid
 * - Sub Sem_id => stored in header of Q. Sub-sem-id => position in s2p array.
 */

/**
 * Perform locking, similar to pthread_mutex_lock
 * @param hdr queue heqder
 * @param int pos
 * @param p_locked is locked result or no
 * @return EXSUCCEED/EXFAIL (errno set)
 */
exprivate int ndrx_svqem_lock(ndrx_svqem_hdr_t *hdr, int *p_locked)
{
    int ret = EXSUCCEED;
    struct sembuf sops;

    sops.sem_num = hdr->svqem_pos;
    sops.sem_op = -1;   /* Decrement semaphore value */
    sops.sem_flg = SEM_UNDO;

    if (M_signalled)
    {
        errno=EINTR;
        EXFAIL_OUT(ret);
    }

    if (EXSUCCEED!=semop(ndrx_G_svqem_seml.semid, &sops, 1))
    {
        int err=errno;
        NDRX_LOG(log_error, "YOPT semop failed sem_num=%d: %s", sops.sem_num, strerror(errno));
        errno=err;
        EXFAIL_OUT(ret);
    }
    *p_locked=EXTRUE;

out:
    /* NDRX_LOG(log_error, "LOCKED: %lu %d", hdr->svqem_pos, ret);*/
    return ret;

}

/**
 * Function to emulate mutex unlock
 * TODO: in case of intr... -> we shall unlock, but still report EINTR (
 * so that we do not consume the timeout by our selves).
 * @param hdr shm header
 * @param eintr_ignr ingnore EINTR (doing exit... of this module)
 * @param p_locked is locked result or no
 * @return EXSUCCEED/EXFAIL (errno set)
 */
exprivate int ndrx_svqem_unlock(ndrx_svqem_hdr_t *hdr, int eintr_ignr, int *p_locked)
{
    int ret = EXSUCCEED;
    struct sembuf sops;
    int was_intr = EXFALSE;

    sops.sem_num = hdr->svqem_pos;
    sops.sem_op = 1;   /* Increment semaphore value */
    sops.sem_flg = SEM_UNDO;

    if (!eintr_ignr && M_signalled)
    {
        errno=EINTR;
        EXFAIL_OUT(ret);
    }

    //It must be 0 here
    int sem_value = semctl(ndrx_G_svqem_seml.semid, hdr->svqem_pos, GETVAL);

    if (sem_value!=0)
    {
        NDRX_LOG(log_error, "YOPT sem_value!=0 => %d", sem_value);
        exit(-1);
    }

    if (!eintr_ignr && M_signalled)
    {
        errno=EINTR;
        EXFAIL_OUT(ret);
    }

    while (EXSUCCEED!=(ret=semop(ndrx_G_svqem_seml.semid, &sops, 1))
        && EINTR==errno && eintr_ignr)
    {
       was_intr=EXTRUE;
    }

    if (EXSUCCEED==ret)
    {
        *p_locked=EXFALSE;
    }

    if (EXSUCCEED==ret && was_intr || M_signalled)
    {
        ret=EXFAIL;
        errno=EINTR;
    }

out:

    /* NDRX_LOG(log_error, "UNLOCKED: %d %d", hdr->svqem_pos, ret);*/

    return ret;
}

/**
 * Wait for condition
 * @param hdr queue header]
 * @param p_locked are we still locked?
 */
exprivate int ndrx_svqem_condwait(ndrx_svqem_hdr_t *hdr, int *p_locked)
{
    int ret = EXSUCCEED;
    struct sembuf sops;

    /* Increment waiter (this is hint, extra loops will cope with interrupted
     * waiters)
     * we are locked here.
     */
    hdr->svqem_nwait++;

    if (EXSUCCEED!=ndrx_svqem_unlock(hdr, EXFALSE, p_locked))
    {
        EXFAIL_OUT(ret);
    }

    sops.sem_num = hdr->svqem_pos;
    sops.sem_op = -1;   /* Decrement semaphore value */
    sops.sem_flg = SEM_UNDO;

    //NDRX_LOG(log_error, "YOPT ndrx_svqem_condwait: semid=%d pos =%d",
    //            ndrx_G_svqem_semc.semid, hdr->svqem_pos);

    if (M_signalled)
    {
        errno=EINTR;
        EXFAIL_OUT(ret);
    }

    if (EXSUCCEED!=semop(ndrx_G_svqem_semc.semid, &sops, 1))
    {
        int err=errno;
        NDRX_LOG(log_error, "YOPT semop failed sem_num=%d: %s", sops.sem_num, strerror(errno));
        errno=err;
        EXFAIL_OUT(ret);
    }

    if (EXSUCCEED!=ndrx_svqem_lock(hdr, p_locked))
    {
        EXFAIL_OUT(ret);
    }
out:
    return ret;
}

/**
 * Send signal to waiter
 * (no problem if there is actually no waiter, the next cond waiter will just
 * get some extra loops)
 * @param hdr queue header
 */
exprivate int ndrx_svqem_cond_signal(ndrx_svqem_hdr_t *hdr)
{
    int ret = EXSUCCEED;
    int was_intr = EXFALSE;

   // NDRX_LOG(log_error, "YOPT ndrx_svqem_cond_signal svqem_nwait=%d pos=%d",
   //             hdr->svqem_nwait, hdr->svqem_pos);

    if (hdr->svqem_nwait > 0)
    {
        struct sembuf sops;
        int sem_value;
        sops.sem_num = hdr->svqem_pos;
        sops.sem_op = 1;    /* Increment semaphore value */
        sops.sem_flg = SEM_UNDO;

        //sem_value = semctl(ndrx_G_svqem_semc.semid, sops.sem_num, GETVAL);
       // NDRX_LOG(log_error, "YOPT ndrx_svqem_cond_signal: semid=%d pos=%d val=%d",
       //             ndrx_G_svqem_semc.semid, hdr->svqem_pos, sem_value);

        while (EXSUCCEED!=(ret=semop(ndrx_G_svqem_semc.semid, &sops, 1)) && EINTR==errno)
        {
            NDRX_LOG(log_error, "YOPT semop failed sem_num=%d: %s", sops.sem_num, strerror(errno));
            ret=EXSUCCEED;
            was_intr=EXTRUE;
        }

        //sem_value = semctl(ndrx_G_svqem_semc.semid, sops.sem_num, GETVAL);
        //NDRX_LOG(log_error, "YOPT ndrx_svqem_cond_signal AFTER: pos=%d val=%d", hdr->svqem_pos, sem_value);

        if (EXSUCCEED!=ret)
        {
            goto out;
        }

        /* it shall be locked here, so safe */
        hdr->svqem_nwait--;

    }

out:
    return ret;
}

/**
 * Calculate queue size
 */
expublic unsigned long ndrx_svqem_get_q_size(void)
{
    /* TODO: the msgsize also shall be modulus of 4 or 8! */
    return sizeof(ndrx_svqem_hdr_t) + (ndrx_G_libnstd_cfg.msg_max *
        (sizeof(ndrx_svqem_msg_hdr_t) + ndrx_msgsizemax() ));
}

/** get queue header.. */
#define NDRX_HDR_GET(pos) (ndrx_svqem_hdr_t *)(ndrx_G_svqem_shm.mem + ndrx_svqem_get_q_size()*pos)

/**
 * Get queue id
 * @param key IPC
 * @param pos index in p2s memory (their count matches with max shms...)
 * @param msgflag IPC_CREAT, IPC_EXCL
 * @return qid -> shared memory segment identifier...
 */
expublic int ndrx_svqem_msgget(ndrx_svqem_key_t key, int pos, int msgflg)
{
    int ret = EXSUCCEED;
    unsigned long index;
    int i;

    if (msgflg & IPC_CREAT)
    {
        ret=pos;

        /* set header ... */
        ndrx_svqem_hdr_t *hdr = NDRX_HDR_GET(pos);
        /* lock... */
        char *mem = (char *) hdr;
        ndrx_svqem_msg_hdr_t      *msghdr;
        long msgsize = ndrx_msgsizemax();

        hdr->svqem_pos=pos;
        hdr->svqem_head=0;
        hdr->svqem_nwait=0;
        hdr->svqem_curmsgs=0;

        index = sizeof(ndrx_svqem_hdr_t);
        hdr->svqem_free=index;

        for (i = 0; i < ndrx_G_libnstd_cfg.msg_max - 1; i++)
        {
            msghdr = (ndrx_svqem_msg_hdr_t *) &mem[index];
            index += sizeof(ndrx_svqem_msg_hdr_t) + msgsize;

            /* NDRX_LOG(log_error, "YOPT index %ld msgsize %ld", index, msgsize);*/
            msghdr->msg_next = index;
        }

        /* terminate the last one..: */
        msghdr = (ndrx_svqem_msg_hdr_t *) &mem[index];
        msghdr->msg_next = 0;

        /*validate_q(hdr);*/

        /* configure locks */
        semctl(ndrx_G_svqem_semc.semid, hdr->svqem_pos, SETVAL, 0);
        semctl(ndrx_G_svqem_seml.semid, hdr->svqem_pos, SETVAL, 1);

    }
    else
    {
        NDRX_LOG(log_error, "Invalid mode %d for key %d", msgflg, (int)key);
        errno = ENOSPC;
    }

out:
    return ret;
}

/**
 * Attach to shared memory segments
 * @param mqd queue descriptor
 * @param pos entry position in p2s array
 * @param msgflg flags passed to ndrx_svqem_msgget
 */
expublic int ndrx_svqem_mqd_open2(mqd_t mqd, int pos, int msgflg)
{
    int ret = EXSUCCEED;
    return ret;
}

/**
 * Close shared memory segments
 */
expublic int ndrx_svqem_mqd_close2(mqd_t mqd)
{
    /* return ndrx_shm_close(&mqd->shm);*/
    return EXSUCCEED;
}

exprivate void validate_q(ndrx_svqem_hdr_t *hdr)
{
    int i;
    char *mem = (char *)hdr;
    long msgsize = ndrx_msgsizemax();
    long q_size = ndrx_svqem_get_q_size();

    NDRX_LOG(log_error, "DUMP Q START");

    NDRX_LOG(log_error, "svqem_pos=%lu", hdr->svqem_pos);
    NDRX_LOG(log_error, "svqem_head=%lu", hdr->svqem_head);
    NDRX_LOG(log_error, "svqem_free=%lu", hdr->svqem_free);
    NDRX_LOG(log_error, "svqem_nwait=%lu", hdr->svqem_nwait);
    NDRX_LOG(log_error, "svqem_curmsgs=%lu", hdr->svqem_curmsgs);

    for (i=0; i<ndrx_G_libnstd_cfg.msg_max; i++)
    {
        ndrx_svqem_msg_hdr_t  *msghdr = (ndrx_svqem_msg_hdr_t  *)(mem + sizeof(ndrx_svqem_hdr_t) +
                (msgsize+sizeof(ndrx_svqem_msg_hdr_t))*i);

        NDRX_LOG(log_error, "   ----------------------------------");
        NDRX_LOG(log_error, "   pos=%d", i);
        NDRX_LOG(log_error, "   msg_len=%lu", (unsigned long)msghdr->msg_len);
        NDRX_LOG(log_error, "   msg_next=%lu", msghdr->msg_next);
        NDRX_LOG(log_error, "   status=%s",
                 msghdr->msg_next > (q_size) ? "msg_next larger than Q":"OK" );

    }

    NDRX_LOG(log_error, "DUMP Q END");
}

/**
 * Send message
 */
expublic int ndrx_svqem_msgsnd(mqd_t mqd, const void *msgp, size_t msgsz, int msgflg)
{
    int ret = EXSUCCEED;
    int              n;
    unsigned long    index, freeindex;
    ndrx_svqem_hdr_t   *hdr = NDRX_HDR_GET(mqd->qid);
    char *mem = (char *) hdr;
    ndrx_svqem_msg_hdr_t  *msghdr, *nmsghdr, *pmsghdr=NULL;
    int locked=EXFALSE;

    errno = 0;

    //NDRX_LOG(log_error, "YOPT ENTER: ndrx_svqem_msgsnd");
    if (EXSUCCEED!=ndrx_svqem_lock(hdr, &locked))
    {
        EXFAIL_OUT(ret);
    }

    if (msgsz > ndrx_msgsizemax()-MSG_OVERHEAD)
    {
        NDRX_LOG(log_error, "YOPT msgsz=%d > limit: %d", msgsz, ndrx_msgsizemax()-MSG_OVERHEAD);
        errno = E2BIG;
        EXFAIL_OUT(ret);
    }

    if (hdr->svqem_curmsgs >= ndrx_G_libnstd_cfg.msg_max)
    {
        /* queue is full */
        if (msgflg & IPC_NOWAIT)
        {
            errno = EAGAIN;
            EXFAIL_OUT(ret);
        }

        /* wait for room for one message on the queue */
        while (hdr->svqem_curmsgs >= ndrx_G_libnstd_cfg.msg_max)
        {
#if 0
            //NDRX_LOG(log_error, "YOPT Q is full: %d", hdr->svqem_curmsgs);
            if (EXSUCCEED!=ndrx_svqem_condwait(hdr, &locked))
            {
                //NDRX_LOG(log_error, "YOPT Q is full AFTER failed: %d", hdr->svqem_curmsgs);
                EXFAIL_OUT(ret);
            }
#endif
            /* above can cause some sort of the deadlock???? */
            if (EXSUCCEED!=ndrx_svqem_unlock(hdr, EXTRUE, &locked))
            {
                EXFAIL_OUT(ret);
            }

            if (M_signalled)
            {
                errno=EINTR;
                EXFAIL_OUT(ret);
            }

            if (EXSUCCEED!=usleep(1000))
            {
                EXFAIL_OUT(ret);
            }

            if (EXSUCCEED!=ndrx_svqem_lock(hdr, &locked))
            {
                EXFAIL_OUT(ret);
            }

        }
    }

    freeindex = hdr->svqem_free;

    /*cannot be 0, as not full here */
    if (0==freeindex)
    {
        NDRX_LOG(log_error, "unexpected freeindex==0, svqem_curmsgs: %lu svqem_head: %lu",
                hdr->svqem_curmsgs, hdr->svqem_head);

        /* iterate through, validate each message offsets
         * 2x queues might end with 0
         * all offset shall be in boundry of the Q size
         */
        validate_q(hdr);

        exit(-1);
    }
    else if (freeindex > ndrx_svqem_get_q_size())
    {
        NDRX_LOG(log_error, "unexpected freeindex==%lu, svqem_curmsgs: %lu svqem_head: %lu",
                freeindex, hdr->svqem_curmsgs, hdr->svqem_head);

        /* iterate through, validate each message offsets
         * 2x queues might end with 0
         * all offset shall be in boundry of the Q size
         */
        validate_q(hdr);

        exit(-1);
    }

    nmsghdr = (ndrx_svqem_msg_hdr_t *) &mem[freeindex];

    nmsghdr->msg_len = msgsz;
    memcpy(nmsghdr + 1, msgp, msgsz + MSG_OVERHEAD);
    hdr->svqem_free = nmsghdr->msg_next;

    /* Search the places for message */
    index = hdr->svqem_head;
    //pmsghdr = (ndrx_svqem_msg_hdr_t *) &mem[hdr->svqem_head];

    /* find end of q: */
    while (index != 0)
    {
        msghdr = (ndrx_svqem_msg_hdr_t *) &mem[index];
        index = msghdr->msg_next;
        pmsghdr = msghdr;
    }

    if (0==hdr->svqem_head)
    {
        /* queue was empty (head set to 0...) */
        hdr->svqem_head = freeindex;
    }
    else
    {
        pmsghdr->msg_next = freeindex;
    }

    /* end of Q */
    nmsghdr->msg_next=0;

    if (0==hdr->svqem_head)
    {
        NDRX_LOG(log_error, "unexpected (after send) svqem_head==0, svqem_curmsgs: %ld svqem_free: %ld",
                hdr->svqem_curmsgs, hdr->svqem_free);
        validate_q(hdr);

        exit(-1);
    }

    hdr->svqem_curmsgs++;
    /* validate_q(hdr); */

    //NDRX_LOG(log_error, "YOPT ENTER: ndrx_svqem_msgsnd about to signal");
    ndrx_svqem_cond_signal(hdr);

out:

    if (locked)
    {
        int err = errno;
        ndrx_svqem_unlock(hdr, EXTRUE, &locked);
        errno = err;
    }

    //NDRX_LOG(log_error, "YOPT ENTER: ndrx_svqem_msgsnd ret %d", ret);

    return ret;
}

/**
 * Receive message
 */
expublic ssize_t ndrx_svqem_msgrcv(mqd_t mqd, void *msgp, size_t msgsz,
                long msgtyp, int msgflg)
{
    ssize_t ret = EXSUCCEED;
    unsigned long      index;
    ndrx_svqem_hdr_t   *hdr = NDRX_HDR_GET(mqd->qid);
    char *mem = (char *) hdr;
    ndrx_svqem_msg_hdr_t  *msghdr, *nmsghdr, *pmsghdr;
    int locked = EXFALSE;


    //NDRX_LOG(log_error, "YOPT ENTER: ndrx_svqem_msgrcv");
    errno = 0;

    if (EXSUCCEED!=ndrx_svqem_lock(hdr, &locked))
    {
        EXFAIL_OUT(ret);
    }

    if (hdr->svqem_curmsgs == 0)
    {
        /* queue is full */
        if (msgflg & IPC_NOWAIT)
        {
            errno = EAGAIN;
            EXFAIL_OUT(ret);
        }

        while (hdr->svqem_curmsgs == 0)
        {
            if (EXSUCCEED!=ndrx_svqem_condwait(hdr, &locked))
            {
                EXFAIL_OUT(ret);
            }
        }
    }

    if ( (index = hdr->svqem_head) == 0)
    {
        /* abort();*/
        NDRX_LOG(log_error, "unexpected svqem_head==0, svqem_curmsgs: %ld svqem_free: %ld",
                hdr->svqem_curmsgs, hdr->svqem_free);
        validate_q(hdr);

        exit(-1);
    }
    else if (hdr->svqem_head > ndrx_svqem_get_q_size())
    {
        NDRX_LOG(log_error, "unexpected svqem_head==%ld, svqem_curmsgs: %ld svqem_head: %ld",
                hdr->svqem_head, hdr->svqem_curmsgs, hdr->svqem_head);

        /* iterate through, validate each message offsets
         * 2x queues might end with 0
         * all offset shall be in boundry of the Q size
         */
        validate_q(hdr);

        exit(-1);
    }

    msghdr = (ndrx_svqem_msg_hdr_t *) &mem[index];

    ret = msghdr->msg_len;

    if (msgsz < ret - MSG_OVERHEAD)
    {
        //NDRX_LOG(log_error, "YOPT msgsz=%d < limit: %d", msgsz, ndrx_msgsizemax()-MSG_OVERHEAD);
        errno = E2BIG;
        EXFAIL_OUT(ret);
    }

    memcpy(msgp, msghdr + 1, ret + MSG_OVERHEAD);

    hdr->svqem_head = msghdr->msg_next;

    /* just-read message goes to front of free list */
    msghdr->msg_next = hdr->svqem_free;
    msghdr->msg_len = 0;
    hdr->svqem_free = index;

    hdr->svqem_curmsgs--;

    /* validate_q(hdr); */

    /* ndrx_svqem_cond_signal(hdr);*/

out:

    if (locked)
    {
        int err = errno;
        ndrx_svqem_unlock(hdr, EXTRUE, &locked);
        errno = err;
    }

    //NDRX_LOG(log_error, "ret=%d errno=%d %d %d: %s", ret, errno, msgsz, (int)ndrx_msgsizemax(), strerror(errno));
    return ret;
}

/**
 * Queue control. Get infos or unlink.
 * @param mqd queue descriptor. In case of remove op -> only qid is set with the descr
 * @param cmd
 * @param buf
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_svqem_msgctl(mqd_t mqd, int cmd, struct ndrx_svqem_msqid_ds *buf)
{
    int ret = EXSUCCEED;
    ndrx_svqem_hdr_t *hdr;

    if (NULL==mqd)
    {
        NDRX_LOG(log_error, "mqd is NULL");
        errno = EINVAL;
        EXFAIL_OUT(ret);
    }

    if (IPC_STAT==cmd)
    {
        int we_open = EXFALSE;
        int locked = EXFALSE;

        if (NULL==buf)
        {
            NDRX_LOG(log_error, "qid=%d, buf == NULL", mqd->qid);
            errno = EINVAL;
            EXFAIL_OUT(ret);
        }
#if 0
        if (NULL==mqd->shm.mem)
        {
           /* try by qid*/
            mqd->shm.mem = (char *)shmat(mqd->qid, 0, 0);

            if (MAP_FAILED==mqd->shm.mem)
            {
                int err=errno;

                NDRX_LOG(log_error, "Failed to shmat memory for fd (qid) %d: %s",
                                    mqd->qid, strerror(errno));
                errno = err;
                EXFAIL_OUT(ret);
            }
            we_open=EXTRUE;
        }
#endif

        /* ok, lets lock up and read current counters... (add journal later) */
        hdr = (ndrx_svqem_hdr_t *) NDRX_HDR_GET(mqd->qid);

        if (EXSUCCEED==ndrx_svqem_lock(hdr, &locked))
        {
            buf[0].msg_qnum = hdr->svqem_curmsgs;
            ndrx_svqem_unlock(hdr, EXTRUE, &locked);
        }

#if 0
        if (we_open)
        {
            shmdt(mqd->shm.mem);
        }
#endif
    }
    else if (IPC_RMID==cmd)
    {
#if 0
        //Nothing to unnlink...
        if (EXSUCCEED!=shmctl(mqd->qid, IPC_RMID, NULL))
        {
            int err = errno;
            NDRX_LOG(log_error, "Failed to IPC_RMID %d: %s",
                            mqd->qid, strerror(err));
            /* restore the error */
            errno = err;
            EXFAIL_OUT(ret);
        }
#endif
    }
    else
    {
        NDRX_LOG(log_error, "Invalid command: %d", cmd);
        errno = EINVAL;
        EXFAIL_OUT(ret);
    }

out:
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
