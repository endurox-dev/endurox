/*****************************************************************************
 *
 * POSIX Message Queue for Windows NT (emulation)
 *
 *****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>

#include <ndrstandard.h>
#if defined(WIN32)
#   include <io.h>
#endif
#include "sys_emqueue.h"
#include "sys_unix.h"
#include "ndebug.h"

#include <stddef.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/mman.h>
#include <signal.h>
#include <thlock.h>
#include <limits.h>
#include <exhash.h>
#include <nstopwatch.h>
#include <nstd_tls.h>
#if defined(WIN32)
#   define S_IXUSR  0000100
#   define sleep(a) Sleep((a)*1000)

    typedef unsigned short mode_t;
#endif

    
#define  LOCK_Q if ( (n = pthread_mutex_lock(&emqhdr->emqh_lock)) != 0)\
    {\
        if (EBUSY!=n)\
        {\
            errno = n;\
            return(-1);\
        }\
        else\
        {\
            NDRX_LOG(log_error, "lock was busy!");\
        }\
    }
    
    
#define MAX_TRIES   10
exprivate  struct mq_attr defattr = { 0, 128, 1024, 0 };


struct qd_hash
{
    void *qd;
    EX_hash_handle hh; /* makes this structure hashable        */
};
typedef struct qd_hash qd_hash_t;

exprivate MUTEX_LOCKDECL(M_lock);
exprivate qd_hash_t *M_qd_hash = NULL;

exprivate  int M_first = EXTRUE; /**< Had random init? */


/*
 * For darwin https://www.endurox.org/issues/512
 * There is issue in case if PTHREAD_PROCESS_SHARED mutex & conditional variables are used
 * between different processes.
 * 
 * See https://github.com/apple/darwin-libpthread/blob/master/src/pthread_cond.c
 * 
 * Problem is that pthread_cond_wait() of Darwin internally compares some saved
 * mutex ptr in ocond variable.
 *
 * And thing is that mmap may return differ addresses (e.g. address randomization).
 * Thus cond and mutex for each process may have it's own pointer.
 * 
 * Thus it may not match with previously set cond->busy by other process, thus may get EINVAL.
 * According to http://openradar.appspot.com/19600706 only option is to access
 * internal variable 'busy' and reset it to NULL. Probably only working case here
 * is if process performs forks. But in our case processes which are using shared
 * mutex & conds are completely different processes.
 * 
 * Though there could be race conditions too, if several processes performs
 * _pthread_cond_wait(), one before call may set busy to NULL, while other
 * may already set it to mutex. Thus we will attempt some 100 times with
 * random sleeps in case of EINVAL, and only after 100 we give up.
 * 
 */
#ifdef EX_OS_DARWIN

/**
 * Wrapper for OSX limitation work around
 * Reset internals before call. Fight with race conditions
 */
expublic int ndrx_pthread_cond_timedwait(pthread_cond_t *restrict cond, 
        pthread_mutex_t *restrict mutex, 
        const struct timespec *restrict abstime)
{
    int attempt = 0;
    
    int ret;
    ndrx_osx_pthread_cond_t *p = (ndrx_osx_pthread_cond_t *)cond;
    do
    {
        if (attempt > 0)
        {
            if (M_first)
            {
                srand(time(NULL));
                M_first = EXFALSE;
            }
            /* max sleep 0.001 sec... */
            usleep(rand() % 1000);
        }
        
        p->busy = NULL;
        ret = pthread_cond_timedwait(cond, mutex, abstime);
        
        attempt++;
        
    } while (EINVAL==ret && attempt < NDRX_OSX_COND_FIX_ATTEMPTS);
    
    return ret;
    
}

/**
 * Wrapper for OSX limitation work around
 * Reset internals before call. Fight with race conditions
 */
expublic int ndrx_pthread_cond_wait(pthread_cond_t *restrict cond, 
        pthread_mutex_t *restrict mutex)
{
    int attempt = 0;
    int ret;
    ndrx_osx_pthread_cond_t *p = (ndrx_osx_pthread_cond_t *)cond;
    do
    {
        if (attempt > 0)
        {
            if (M_first)
            {
                srand(time(NULL));
                M_first = EXFALSE;
            }
            /* max sleep 0.001 sec... */
            usleep(rand() % 1000);
        }
        
        p->busy = NULL;
        ret = pthread_cond_wait(cond, mutex);
        
        attempt++;
        
    } while (EINVAL==ret && attempt < NDRX_OSX_COND_FIX_ATTEMPTS);
    
    return ret;
}

#else

#define ndrx_pthread_cond_wait pthread_cond_wait
#define ndrx_pthread_cond_timedwait pthread_cond_timedwait

#endif


/**
 * Add queue descriptor to hash
 * @param q
 * @return 
 */
exprivate int qd_exhash_add(mqd_t q)
{
    int ret = EXSUCCEED;
    qd_hash_t * el = NDRX_CALLOC(1, sizeof(qd_hash_t));
    
    NDRX_LOG(log_dump, "Registering %p as mqd_t", q);
    if (NULL==el)
    {
        NDRX_LOG(log_error, "Failed to alloc: %s", strerror(errno));
        EXFAIL_OUT(ret);
    }
    
    el->qd  = (void *)q;
    
    MUTEX_LOCK_V(M_lock);
    
    EXHASH_ADD_PTR(M_qd_hash, qd, el);
    NDRX_LOG(log_dump, "added...");
    
    MUTEX_UNLOCK_V(M_lock);
    
out:

    return ret;
}

/**
 * Check is 
 * @param q
 * @return 
 */
exprivate int qd_hash_chk(mqd_t qd)
{
    qd_hash_t *ret = NULL;
    
    NDRX_LOG(log_dump, "checking qd %p", qd);
    
    MUTEX_LOCK_V(M_lock);
    
    EXHASH_FIND_PTR( M_qd_hash, ((void **)&qd), ret);
    
    MUTEX_UNLOCK_V(M_lock);
    
    if (NULL!=ret)
    {
        return EXTRUE;
    }
    else
    {
        return EXFALSE;
    }
}

/**
 * Check is 
 * @param q
 * @return 
 */
exprivate void qd_hash_del(mqd_t qd)
{
    qd_hash_t *ret = NULL;
    
    NDRX_LOG(log_dump, "Unregistering %p as mqd_t", qd);
    
    MUTEX_LOCK_V(M_lock);
    EXHASH_FIND_PTR( M_qd_hash, ((void **)&qd), ret);
    
    if (NULL!=ret)
    {
        EXHASH_DEL(M_qd_hash, ret);
        NDRX_FREE(ret);
    }
    
    MUTEX_UNLOCK_V(M_lock);
}


/**
 * Get he queue path
 * @param path
 * @param[out] bufout where to copy the output path string 
 * @param[out] bufoutsz output buffer size
 * @return 
 */
static char *get_path(const char *path, char *bufout, size_t bufoutsz)
{
    static int first = 1;
    static char q_path[PATH_MAX]={EXEOS};
    
/*
    NSTD_TLS_ENTRY;
*/
    
    if (first)
    {
        char *p;
        if (NULL!=(p=getenv(CONF_NDRX_QPATH)))
        {
            strcpy(q_path, p);
        }
        
        first = 0;
    }
    
    
/*
    strcpy(G_nstd_tls->emq_x, q_path);
    strcat(G_nstd_tls->emq_x, path);
    
*/
    NDRX_STRCPY_SAFE_DST(bufout, q_path, bufoutsz);
    NDRX_STRCAT_S(bufout, bufoutsz, path);

    return bufout;
}

/**
 * Close message queue
 * @param emqd queue dsc
 * @return  standard queue errors
 */
expublic int emq_close(mqd_t emqd)
{
    long            msgsize, filesize;
    struct emq_hdr  *emqhdr;
    struct mq_attr *attr;
    struct emq_info *emqinfo;

    emqinfo = emqd;
    if (emqinfo->emqi_magic != EMQI_MAGIC) {
        errno = EBADF;
        return(-1);
    }
    emqhdr = emqinfo->emqi_hdr;
    attr = &emqhdr->emqh_attr;

    if (emq_notify(emqd, NULL) != 0)        /* unregister calling process */
        return(-1);

    msgsize = MSGSIZE(attr->mq_msgsize);
    filesize = sizeof(struct emq_hdr) + (attr->mq_maxmsg *
                      (sizeof(struct msg_hdr) + msgsize));

    NDRX_LOG(log_dump, "Before munmap()");
#if defined(WIN32)
    if (!UnmapViewOfFile(emqinfo->emqi_hdr) || !CloseHandle(emqinfo->emqi_fmap))
#else
    if (munmap(emqinfo->emqi_hdr, filesize) == -1)
#endif
        return(-1);
    NDRX_LOG(log_dump, "After munmap()");

    emqinfo->emqi_magic = 0;          /* just in case */
    NDRX_FREE(emqinfo);
    qd_hash_del(emqd);

    NDRX_LOG(log_dump, "into: emq_close ret 0");
    
    return(0);
}

expublic int emq_getattr(mqd_t emqd, struct mq_attr *emqstat)
{
    int             n;
    struct emq_hdr  *emqhdr;
    struct mq_attr *attr;
    struct emq_info *emqinfo;

    NDRX_LOG(log_dump, "into: emq_getattr");
    if (!qd_hash_chk((mqd_t) emqd)) {
        NDRX_LOG(log_error, "Invalid queue descriptor: %p", emqd);
        errno = EBADF;
        return(-1);
    }
    
    emqinfo = emqd;
    if (emqinfo->emqi_magic != EMQI_MAGIC)
    {
        errno = EBADF;
        return(-1);
    }
    emqhdr = emqinfo->emqi_hdr;
    attr = &emqhdr->emqh_attr;
    
    LOCK_Q;

    emqstat->mq_flags = emqinfo->emqi_flags;   /* per-open */
    emqstat->mq_maxmsg = attr->mq_maxmsg;    /* remaining three per-queue */
    emqstat->mq_msgsize = attr->mq_msgsize;
    emqstat->mq_curmsgs = attr->mq_curmsgs;

    MUTEX_UNLOCK_V(emqhdr->emqh_lock);
    NDRX_LOG(log_dump, "into: emq_getattr ret 0");
    return(0);
}

/**
 * Configure notification
 * @param emqd
 * @param notification
 * @return 
 */
expublic int emq_notify(mqd_t emqd, const struct sigevent *notification)
{
#if !defined(WIN32)
    int             n;
    pid_t           pid;
    struct emq_hdr  *emqhdr;
    struct emq_info *emqinfo;
    
    if (!qd_hash_chk((mqd_t) emqd)) {
        NDRX_LOG(log_error, "Invalid queue descriptor: %p", emqd);
        errno = EBADF;
        return(-1);
    }

    emqinfo = emqd;
    if (emqinfo->emqi_magic != EMQI_MAGIC) {
        errno = EBADF;
        return(-1);
    }
    emqhdr = emqinfo->emqi_hdr;
    
    LOCK_Q;

    pid = getpid();
    if (notification == NULL)
    {
        if (emqhdr->emqh_pid == pid)
        {
            emqhdr->emqh_pid = 0;     /* unregister calling process */
        }                           /* no error if c aller not registered */
    } 
    else 
    {
        if (emqhdr->emqh_pid != 0)
        {
            if (kill(emqhdr->emqh_pid, 0) != -1 || errno != ESRCH)
            {
                errno = EBUSY;
                goto err;
            }
        }
        emqhdr->emqh_pid = pid;
        emqhdr->emqh_event = *notification;
    }
    MUTEX_UNLOCK_V(emqhdr->emqh_lock);
    return(0);

err:
    MUTEX_UNLOCK_V(emqhdr->emqh_lock);
    return(-1);
#else
    errno = EINVAL;
    return(-1);
#endif
}

/**
 * Open queue
 * @param pathname
 * @param oflag
 * @param ...
 * @return 
 */
expublic mqd_t emq_open(const char *pathname, int oflag, ...)
{
    int                  i, fd, nonblock, created, save_errno;
    long                 msgsize, filesize, index;
    va_list              ap;
    mode_t               mode;
    char                *mptr;
    struct stat          statbuff;
    struct emq_hdr       *emqhdr;
    struct msg_hdr      *msghdr;
    struct mq_attr      *attr;
    struct emq_info      *emqinfo;
    char emq_x[PATH_MAX+1];
    pthread_mutexattr_t  mattr;
    pthread_condattr_t   cattr;
#if defined(WIN32)
    HANDLE fmap;

    mptr = NULL;
#else
    mptr = (char *) MAP_FAILED;
#endif
    created = 0;
    nonblock = oflag & O_NONBLOCK;
    oflag &= ~O_NONBLOCK;
    emqinfo = NULL;
    NDRX_LOG(log_dump, "into: emq_open");


again:
    if (oflag & O_CREAT)
    {
        va_start(ap, oflag); /* init ap to final named argument */

        /*mode = va_arg(ap, mode_t) & ~S_IXUSR; - gives promition to int warning on osx */
        mode = va_arg(ap, int) & ~S_IXUSR;
        attr = va_arg(ap, struct mq_attr *);
        va_end(ap);

        /* open and specify O_EXCL and user-execute */
        fd = open(get_path(pathname, emq_x, sizeof(emq_x)), oflag | O_EXCL | O_RDWR, mode | S_IXUSR);
        if (fd < 0)
        {
            if (errno == EEXIST && (oflag & O_EXCL) == 0)
                goto exists;            /* already exists, OK */
            else
                return((mqd_t) -1);
        }
        created = 1;
                    /* first one to create the file initializes it */
        if (attr == NULL)
            attr = &defattr;
        else 
        {
            if (attr->mq_maxmsg <= 0 || attr->mq_msgsize <= 0) 
            {
                errno = EINVAL;
                goto err;
            }
        }
        /* calculate and set the file size */
        msgsize = MSGSIZE(attr->mq_msgsize);
        filesize = sizeof(struct emq_hdr) + (attr->mq_maxmsg *
                           (sizeof(struct msg_hdr) + msgsize));
        if (lseek(fd, filesize - 1, SEEK_SET) == -1)
            goto err;
        if (write(fd, "", 1) == -1)
            goto err;

        /* memory map the file */
#if defined(WIN32)
        fmap = CreateFileMapping((HANDLE)_get_osfhandle(fd), NULL, 
                                 PAGE_READWRITE, 0, 0, NULL);
        if (fmap == NULL)
            goto err;
        mptr = MapViewOfFile(fmap, FILE_MAP_WRITE, 0, 0, filesize);
        if (mptr == NULL)
#else
        mptr = mmap(NULL, filesize, PROT_READ | PROT_WRITE,
                                    MAP_SHARED, fd, 0);
        if (mptr == MAP_FAILED)
#endif
            goto err;

        /* allocate one emq_info{} for the queue */
        if ( (emqinfo = NDRX_MALLOC(sizeof(struct emq_info))) == NULL)
            goto err;
#if defined(WIN32)
        emqinfo->emqi_fmap = fmap;
#endif
        emqinfo->emqi_hdr = emqhdr = (struct emq_hdr *) mptr;
        emqinfo->emqi_magic = EMQI_MAGIC;
        emqinfo->emqi_flags = nonblock;

        /* initialize header at beginning of file */
        /* create free list with all messages on it */
        emqhdr->emqh_attr.mq_flags = 0;
        emqhdr->emqh_attr.mq_maxmsg = attr->mq_maxmsg;
        emqhdr->emqh_attr.mq_msgsize = attr->mq_msgsize;
        emqhdr->emqh_attr.mq_curmsgs = 0;
        emqhdr->emqh_nwait = 0;
        emqhdr->emqh_pid = 0;
        emqhdr->emqh_head = 0;
        index = sizeof(struct emq_hdr);
        emqhdr->emqh_free = index;
        for (i = 0; i < attr->mq_maxmsg - 1; i++)
        {
            msghdr = (struct msg_hdr *) &mptr[index];
            index += sizeof(struct msg_hdr) + msgsize;
            msghdr->msg_next = index;
        }
        msghdr = (struct msg_hdr *) &mptr[index];
        msghdr->msg_next = 0;           /* end of free list */

        /* initialize mutex & condition variable */
        if ( (i = pthread_mutexattr_init(&mattr)) != 0)
            goto pthreaderr;
        pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
        i = pthread_mutex_init(&emqhdr->emqh_lock, &mattr);
        pthread_mutexattr_destroy(&mattr);      /* be sure to destroy */
        if (i != 0)
            goto pthreaderr;

        if ( (i = pthread_condattr_init(&cattr)) != 0)
            goto pthreaderr;
        pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);
        i = pthread_cond_init(&emqhdr->emqh_wait, &cattr);
        pthread_condattr_destroy(&cattr);       /* be sure to destroy */
        if (i != 0)
            goto pthreaderr;

        /* initialization complete, turn off user-execute bit */
#if defined(WIN32)
        if (chmod(get_path(pathname, emq_x, sizeof(emq_x)), mode) == -1)
#else
        if (fchmod(fd, mode) == -1)
#endif
            goto err;
        
        close(fd);
        if (EXSUCCEED!=qd_exhash_add((mqd_t) emqinfo))
        {
            NDRX_LOG(log_error, "Failed to add mqd_t to hash!");
            errno = ENOMEM;
        }
        NDRX_LOG(log_dump, "into: emq_open ret ok");
        return((mqd_t) emqinfo);
    }
exists:
    /* open the file then memory map */
    if ( (fd = open(get_path(pathname, emq_x, sizeof(emq_x)), O_RDWR)) < 0)
    {
        if (errno == ENOENT && (oflag & O_CREAT))
            goto again;
        goto err;
    }

    /* make certain initialization is complete */
    for (i = 0; i < MAX_TRIES; i++)
    {
        if (stat(get_path(pathname, emq_x, sizeof(emq_x)), &statbuff) == -1)
        {
            if (errno == ENOENT && (oflag & O_CREAT))
            {
                close(fd);
                goto again;
            }
            goto err;
        }
        if ((statbuff.st_mode & S_IXUSR) == 0)
            break;
        sleep(1);
	/*usleep(1000000);*/
    }
    if (i == MAX_TRIES)
    {
        errno = ETIMEDOUT;
        goto err;
    }

    filesize = statbuff.st_size;
#if defined(WIN32)
    fmap = CreateFileMapping((HANDLE)_get_osfhandle(fd), NULL, PAGE_READWRITE, 
                             0, 0, NULL);                             
    if (fmap == NULL)
        goto err;
    mptr = MapViewOfFile(fmap, FILE_MAP_WRITE, 0, 0, filesize);
    if (mptr == NULL)
#else
    mptr = mmap(NULL, filesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mptr == MAP_FAILED)
#endif
        goto err;
    close(fd);

    /* allocate one emq_info{} for each open */
    if ( (emqinfo = NDRX_MALLOC(sizeof(struct emq_info))) == NULL)
        goto err;
    emqinfo->emqi_hdr = (struct emq_hdr *) mptr;
    emqinfo->emqi_magic = EMQI_MAGIC;
    emqinfo->emqi_flags = nonblock;
    
    if (EXSUCCEED!=qd_exhash_add((mqd_t) emqinfo))
    {
        NDRX_LOG(log_error, "Failed to add mqd_t to hash!");
        errno = ENOMEM;
    }
    
    NDRX_LOG(log_dump, "into: emq_open ret ok");
    return((mqd_t) emqinfo);
pthreaderr:
    errno = i;
err:
    /* don't let following function calls change errno */
    save_errno = errno;
    if (created)
        unlink(get_path(pathname, emq_x, sizeof(emq_x)));
#if defined(WIN32)
    if (fmap != NULL) {
        if (mptr != NULL) {
            UnmapViewOfFile(mptr);
        }
        CloseHandle(fmap);
    }
#else
    if (mptr != MAP_FAILED)
        munmap(mptr, filesize);
#endif
    if (emqinfo != NULL)
        NDRX_FREE(emqinfo);
    close(fd);
    
    NDRX_LOG(log_dump, "into: emq_open ret -1");
    
    errno = save_errno;
    return((mqd_t) -1);
}

/**
 * Send timed message.
 * Basically we operate on locked queue and unlock at the end (or error)
 * if not lock is received, direct error is returned.
 * @param emqd q descr
 * @param ptr msg ptr
 * @param maxlen max len
 * @param priop priority (not used)
 * @param __abs_timeout timeout
 * @return bytes received or error
 */
expublic ssize_t emq_timedreceive(mqd_t emqd, char *ptr, size_t maxlen, unsigned int *priop,
        const struct timespec *__abs_timeout)
{
    int             n;
    long            index;
    char           *mptr;
    ssize_t         len;
    struct emq_hdr  *emqhdr;
    struct mq_attr *attr;
    struct msg_hdr *msghdr;
    struct emq_info *emqinfo;

    NDRX_LOG(log_dump, "into: emq_timedreceive");
    
    if (!qd_hash_chk((mqd_t) emqd))
    {
        NDRX_LOG(log_error, "Invalid queue descriptor: %p", emqd);
        errno = EBADF;
        return(-1);
    }

    emqinfo = emqd;
    if (emqinfo->emqi_magic != EMQI_MAGIC)
    {
        errno = EBADF;
        return(-1);
    }
    emqhdr = emqinfo->emqi_hdr;        /* struct pointer */
    mptr = (char *) emqhdr;          /* byte pointer */
    attr = &emqhdr->emqh_attr;
    
    LOCK_Q;

    if (maxlen < (size_t)attr->mq_msgsize)
    {
        errno = EMSGSIZE;
        goto err;
    }
    
    if (attr->mq_curmsgs == 0)
    {            /* queue is empty */
        if (emqinfo->emqi_flags & O_NONBLOCK)
        {
            errno = EAGAIN;
            goto err;
        }
        /* wait for a message to be placed onto queue */
        emqhdr->emqh_nwait++;
        while (attr->mq_curmsgs == 0)
        {
            NDRX_LOG(log_dump, "queue empty on %p", emqd);
            
            if (NULL==__abs_timeout)
            {
                if (EXSUCCEED!=(n=ndrx_pthread_cond_wait(&emqhdr->emqh_wait, 
                        &emqhdr->emqh_lock)))
                {
                    /* have lock as stated by pthread_cond_wait !*/
                    NDRX_LOG(log_error, "%s: pthread_cond_wait failed %d: %s", 
                        __func__, n, strerror(n));
                    userlog("%s: pthread_cond_wait failed %d: %s", 
                        __func__, n, strerror(n));
                    errno = n;

                    emqhdr->emqh_nwait--;
                    goto err;
                }
            }
            else
            {
                /* wait some time...  */
                NDRX_LOG(log_dump, "timed wait...");
                
                /* On osx this can do early returns...
                 * Also... we might get time-outs due to broadcasts...
                 * then we need to reduce the waiting time...
                 */
                if (EXSUCCEED!=(n=ndrx_pthread_cond_timedwait(&emqhdr->emqh_wait, 
                        &emqhdr->emqh_lock, __abs_timeout)))
                { 
                    
                    if (n!=ETIMEDOUT)
                    {
                        /* have lock as per pthread_cond_timedwait spec */
                        NDRX_LOG(log_error, "%s: ndrx_pthread_cond_timedwait failed %d: %s", 
                            __func__, n, strerror(n));
                        userlog("%s: ndrx_pthread_cond_timedwait failed %d: %s", 
                            __func__, n, strerror(n));
                    }
                    else
                    {
                        NDRX_LOG(log_dump, "ETIMEDOUT: attr->mq_curmsgs = %ld", attr->mq_curmsgs);
                    }

                    errno = n;
                    emqhdr->emqh_nwait--;
                    goto err;
                }
            }
        }
        emqhdr->emqh_nwait--;
    }

    if ( (index = emqhdr->emqh_head) == 0)
    {
        NDRX_LOG(log_error, "emq_timedreceive: curmsgs = %ld; head = 0",attr->mq_curmsgs);
        abort();
    }

    msghdr = (struct msg_hdr *) &mptr[index];
    emqhdr->emqh_head = msghdr->msg_next;     /* new head of list */
    len = msghdr->msg_len;
    memcpy(ptr, msghdr + 1, len);           /* copy the message itself */
    if (priop != NULL)
        *priop = msghdr->msg_prio;

    /* just-read message goes to front of free list */
    msghdr->msg_next = emqhdr->emqh_free;
    emqhdr->emqh_free = index;

    /* if configuration of queues are changed, then wake up any one who 
     * is waiting, if not none waiting - no problem
     */
    pthread_cond_signal(&emqhdr->emqh_wait);
    
    attr->mq_curmsgs--;

    MUTEX_UNLOCK_V(emqhdr->emqh_lock);
    
    NDRX_LOG(log_dump, "emq_timedreceive - got something len=%d stats: %ld wait: %ld",
            len, attr->mq_curmsgs, emqhdr->emqh_nwait);
    return(len);

err:
    MUTEX_UNLOCK_V(emqhdr->emqh_lock);
    n = errno;
    NDRX_LOG(log_dump, "emq_timedreceive - failed: %s stats: %ld wait: %ld",
            strerror(errno), attr->mq_curmsgs, emqhdr->emqh_nwait);
    errno = n;
    
    return(-1);
}

/**
 * Send message on queue.
 * @param emqd q descr
 * @param ptr msg ptr
 * @param len msg len
 * @param prio not used
 * @param __abs_timeout timeout
 * @return EXSUCCEED or EXFAIL + errno set.
 */
expublic int emq_timedsend(mqd_t emqd, const char *ptr, size_t len, unsigned int prio, 
        const struct timespec *__abs_timeout)
{
    int              n;
    long             index, freeindex;
    char            *mptr;
    struct sigevent *sigev;
    struct emq_hdr   *emqhdr;
    struct mq_attr  *attr;
    struct msg_hdr  *msghdr, *nmsghdr, *pmsghdr;
    struct emq_info  *emqinfo;

    NDRX_LOG(log_dump, "into: emq_timedsend");

    if (!qd_hash_chk((mqd_t) emqd))
    {
        NDRX_LOG(log_error, "Invalid queue descriptor: %p", emqd);
        errno = EBADF;
        return(-1);
    }
    
    emqinfo = emqd;
    if (emqinfo->emqi_magic != EMQI_MAGIC)
    {
        errno = EBADF;
        return(-1);
    }
    emqhdr = emqinfo->emqi_hdr;        /* struct pointer */
    mptr = (char *) emqhdr;          /* byte pointer */
    attr = &emqhdr->emqh_attr;
    
    LOCK_Q;

    if (len > (size_t)attr->mq_msgsize)
    {
        errno = EMSGSIZE;
        goto err;
    }

    if (attr->mq_curmsgs >= attr->mq_maxmsg)
    {
        /* queue is full */
        if (emqinfo->emqi_flags & O_NONBLOCK)
        {
            errno = EAGAIN;
            goto err;
        }
        /* wait for room for one message on the queue */
        while (attr->mq_curmsgs >= attr->mq_maxmsg)
        {
            NDRX_LOG(log_dump, "waiting on q %p", emqd);
            
            if (NULL==__abs_timeout)
            {
                if (EXSUCCEED!=(n=ndrx_pthread_cond_wait(&emqhdr->emqh_wait, 
                        &emqhdr->emqh_lock)))
                {
                    NDRX_LOG(log_error, "%s: pthread_cond_wait failed %d: %s", 
                            __func__, n, strerror(n));
                    userlog("%s: pthread_cond_wait failed %d: %s", 
                            __func__, n, strerror(n));
                    
                    /* no lock */
                    errno = n;
                    return -1;
                }
            }
            else
            {
                /* wait some time...  */
                NDRX_LOG(log_dump, "timed wait...");
                /* On osx this can do early returns...*/
                if (EXSUCCEED!=(n=ndrx_pthread_cond_timedwait(&emqhdr->emqh_wait, 
                        &emqhdr->emqh_lock, __abs_timeout)))
                {
                    if (n!=ETIMEDOUT)
                    {
                        NDRX_LOG(log_error, "%s: ndrx_pthread_cond_timedwait failed %d: %s", 
                            __func__, n, strerror(n));
                        userlog("%s: ndrx_pthread_cond_timedwait failed %d: %s", 
                                __func__, n, strerror(n));
                        errno = n;
                        return -1;
                    }
                    NDRX_LOG(log_error, "ETIMEDOUT: attr->mq_curmsgs = %ld", attr->mq_curmsgs);
                    
                    /* we have lock... */
                    errno = n;
                    goto err;
                }
            }
            NDRX_LOG(log_info, "%p - accessed ok", emqd);
        }
    }
    /* nmsghdr will point to new message */
    if ( (freeindex = emqhdr->emqh_free) == 0)
    {
        userlog("emq_send: curmsgs = %ld; free = 0", attr->mq_curmsgs);
    }

    nmsghdr = (struct msg_hdr *) &mptr[freeindex];
    nmsghdr->msg_prio = prio;
    nmsghdr->msg_len = len;
    memcpy(nmsghdr + 1, ptr, len);          /* copy message from caller */
    emqhdr->emqh_free = nmsghdr->msg_next;    /* new freelist head */

    /* find right place for message in linked list */
    index = emqhdr->emqh_head;
    pmsghdr = (struct msg_hdr *) &(emqhdr->emqh_head);
    while (index != 0)
    {
        msghdr = (struct msg_hdr *) &mptr[index];
        if (prio > msghdr->msg_prio)
        {
            nmsghdr->msg_next = index;
            pmsghdr->msg_next = freeindex;
            break;
        }
        index = msghdr->msg_next;
        pmsghdr = msghdr;
    }
    if (index == 0)
    {
        /* queue was empty or new goes at end of list */
        pmsghdr->msg_next = freeindex;
        nmsghdr->msg_next = 0;
    }
    
    if (attr->mq_curmsgs == 0)
    {
        if (emqhdr->emqh_pid != 0 && emqhdr->emqh_nwait == 0)
        {
            sigev = &emqhdr->emqh_event;
#if !defined(WIN32)
            if (sigev->sigev_notify == SIGEV_SIGNAL)
            {
                /*sigqueue(emqhdr->emqh_pid, sigev->sigev_signo,
                                         sigev->sigev_value);*/
                kill(emqhdr->emqh_pid, sigev->sigev_signo);
            }
#endif
            emqhdr->emqh_pid = 0;             /* unregister */
        }
    }

    /* if configuration of queues are changed, then wake up any one who 
     * is waiting, if not none waiting - no problem
     */
    pthread_cond_signal(&emqhdr->emqh_wait);
    attr->mq_curmsgs++;
    
    MUTEX_UNLOCK_V(emqhdr->emqh_lock);
    NDRX_LOG(log_dump, "into: emq_timedsend - return 0 stats: %ld wait: %ld",
            attr->mq_curmsgs, emqhdr->emqh_nwait);
    return(0);

err:
    MUTEX_UNLOCK_V(emqhdr->emqh_lock);

    n = errno;
    NDRX_LOG(log_dump, "into: emq_timedsend - return -1: %s stats: %ld wait: %ld",
            strerror(n), attr->mq_curmsgs, emqhdr->emqh_nwait);
    errno = n;
    return(-1);
}

/**
 * Send Posix q msg, no tout
 * @param emqd
 * @param ptr
 * @param len
 * @param prio
 * @return 
 */
expublic int emq_send(mqd_t emqd, const char *ptr, size_t len, unsigned int prio)
{
    return emq_timedsend(emqd, ptr, len, prio, NULL);
}

/**
 * Receive Posix q msg, no tout
 * @param emqd
 * @param ptr
 * @param maxlen
 * @param priop
 * @return 
 */
expublic ssize_t emq_receive(mqd_t emqd, char *ptr, size_t maxlen, unsigned int *priop)
{
    return emq_timedreceive(emqd, ptr, maxlen, priop, NULL);
}

/**
 * Set queue attr
 * @param emqd
 * @param emqstat
 * @param oemqstat
 * @return 
 */
expublic int emq_setattr(mqd_t emqd, const struct mq_attr *emqstat, struct mq_attr *oemqstat)
{
    int             n;
    struct emq_hdr  *emqhdr;
    struct mq_attr *attr;
    struct emq_info *emqinfo;

    NDRX_LOG(log_dump, "into: emq_setattr");

    if (!qd_hash_chk((mqd_t) emqd))
    {
        NDRX_LOG(log_error, "Invalid queue descriptor: %p", emqd);
        errno = EBADF;
        return(-1);
    }
    
    emqinfo = emqd;
    if (emqinfo->emqi_magic != EMQI_MAGIC)
    {
        errno = EBADF;
        return(-1);
    }
    emqhdr = emqinfo->emqi_hdr;
    attr = &emqhdr->emqh_attr;
    
    LOCK_Q;

    if (oemqstat != NULL)
    {
        oemqstat->mq_flags = emqinfo->emqi_flags;  /* previous attributes */
        oemqstat->mq_maxmsg = attr->mq_maxmsg;
        oemqstat->mq_msgsize = attr->mq_msgsize;
        oemqstat->mq_curmsgs = attr->mq_curmsgs; /* and current status */
    }

    if (emqstat->mq_flags & O_NONBLOCK)
        emqinfo->emqi_flags |= O_NONBLOCK;
    else
        emqinfo->emqi_flags &= ~O_NONBLOCK;

    MUTEX_UNLOCK_V(emqhdr->emqh_lock);
    NDRX_LOG(log_dump, "into: emq_setattr - return 0");
    return(0);
}

/**
 * Delete Q
 * @param pathname
 * @return 
 */
expublic int emq_unlink(const char *pathname)
{
    char emq_x[PATH_MAX+1];
    NDRX_LOG(log_dump, "into: emq_unlink");
    
    if (unlink(get_path(pathname, emq_x, sizeof(emq_x))) == -1)
    {
        NDRX_LOG(log_dump, "into: emq_unlink ret -1");
        return(-1);
    }
    NDRX_LOG(log_dump, "into: emq_unlink ret 0");
    return(0);
}
