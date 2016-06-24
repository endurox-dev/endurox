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
#include "ndebug.h"

#include <stddef.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/mman.h>
#include <thlock.h>
#include <limits.h>
#include <atmi.h>
#include <uthash.h>
#if defined(WIN32)
#   define S_IXUSR  0000100
#   define sleep(a) Sleep((a)*1000)

    typedef unsigned short mode_t;
#endif

#define MAX_TRIES   10
struct mq_attr defattr = { 0, 128, 1024, 0 };


/* TODO add func for check/add/delete hashsed mqd_t */

struct qd_hash
{
    void *qd;
    UT_hash_handle hh; /* makes this structure hashable        */
};
typedef struct qd_hash qd_hash_t;


MUTEX_LOCKDECL(M_lock);
qd_hash_t *M_qd_hash = NULL;
/**
 * Add queue descriptor to hash
 * @param q
 * @return 
 */
private int qd_hash_add(mqd_t q)
{
    int ret = SUCCEED;
    qd_hash_t * el = calloc(1, sizeof(qd_hash_t));
    
    NDRX_LOG(log_debug, "Registering %p as mqd_t", q);
    if (NULL==el)
    {
        NDRX_LOG(log_error, "Failed to alloc: %s", strerror(errno));
        FAIL_OUT(ret);
    }
    
    el->qd  = (void *)q;
    
    MUTEX_LOCK_V(M_lock);
    
    HASH_ADD_PTR(M_qd_hash, qd, el);
    NDRX_LOG(log_error, "added...");
    
    MUTEX_UNLOCK_V(M_lock);
    
out:

    return ret;
}

/**
 * Check is 
 * @param q
 * @return 
 */
private int qd_hash_chk(mqd_t qd)
{
    qd_hash_t *ret = NULL;
    
    NDRX_LOG(log_debug, "checking qd %p", qd);
    
    MUTEX_LOCK_V(M_lock);
    
    HASH_FIND_PTR( M_qd_hash, ((void **)&qd), ret);
    
    MUTEX_UNLOCK_V(M_lock);
    
    if (NULL!=ret)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/**
 * Check is 
 * @param q
 * @return 
 */
private void qd_hash_del(mqd_t qd)
{
    qd_hash_t *ret = NULL;
    
    NDRX_LOG(log_debug, "Unregistering %p as mqd_t", qd);
    
    MUTEX_LOCK_V(M_lock);
    HASH_FIND_PTR( M_qd_hash, ((void **)&qd), ret);
    
    if (NULL!=ret)
    {
        HASH_DEL(M_qd_hash, ret);
        free(ret);
    }
    
    MUTEX_UNLOCK_V(M_lock);
}


/**
 * Get he queue path
 * @param path
 * @return 
 */
static char *get_path(const char *path)
{
    static __thread char x[512];
    static int first = 1;
    static char q_path[PATH_MAX]={EOS};
    
    if (first)
    {
        char *p;
        if (NULL!=(p=getenv(CONF_NDRX_QPATH)))
        {
            strcpy(q_path, p);
        }
        
        first = 0;
    }
    
    
    strcpy(x, q_path);
    strcat(x, path);
    
    return x;
}

int emq_close(mqd_t emqd)
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
#if defined(WIN32)
    if (!UnmapViewOfFile(emqinfo->emqi_hdr) || !CloseHandle(emqinfo->emqi_fmap))
#else
    if (munmap(emqinfo->emqi_hdr, filesize) == -1)
#endif
        return(-1);

    emqinfo->emqi_magic = 0;          /* just in case */
    free(emqinfo);
    qd_hash_del(emqd);

    NDRX_LOG(log_debug, "into: emq_close ret 0");
    
    return(0);
}

int emq_getattr(mqd_t emqd, struct mq_attr *emqstat)
{
    int             n;
    struct emq_hdr  *emqhdr;
    struct mq_attr *attr;
    struct emq_info *emqinfo;

    NDRX_LOG(log_debug, "into: emq_getattr");
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
    attr = &emqhdr->emqh_attr;
    if ( (n = pthread_mutex_lock(&emqhdr->emqh_lock)) != 0) {
        errno = n;
        return(-1);
    }

    emqstat->mq_flags = emqinfo->emqi_flags;   /* per-open */
    emqstat->mq_maxmsg = attr->mq_maxmsg;    /* remaining three per-queue */
    emqstat->mq_msgsize = attr->mq_msgsize;
    emqstat->mq_curmsgs = attr->mq_curmsgs;

    pthread_mutex_unlock(&emqhdr->emqh_lock);
    NDRX_LOG(log_debug, "into: emq_getattr ret 0");
    return(0);
}

int emq_notify(mqd_t emqd, const struct sigevent *notification)
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
    if ( (n = pthread_mutex_lock(&emqhdr->emqh_lock)) != 0) {
        errno = n;
        return(-1);
    }

    pid = getpid();
    if (notification == NULL) {
        if (emqhdr->emqh_pid == pid) {
            emqhdr->emqh_pid = 0;     /* unregister calling process */
        }                           /* no error if c aller not registered */
    } else {
        if (emqhdr->emqh_pid != 0) {
            if (kill(emqhdr->emqh_pid, 0) != -1 || errno != ESRCH) {
                errno = EBUSY;
                goto err;
            }
        }
        emqhdr->emqh_pid = pid;
        emqhdr->emqh_event = *notification;
    }
    pthread_mutex_unlock(&emqhdr->emqh_lock);
    return(0);

err:
    pthread_mutex_unlock(&emqhdr->emqh_lock);
    return(-1);
#else
    errno = EINVAL;
    return(-1);
#endif
}

mqd_t emq_open(const char *pathname, int oflag, ...)
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
    NDRX_LOG(log_debug, "into: emq_open");


again:
    if (oflag & O_CREAT) {
        va_start(ap, oflag); /* init ap to final named argument */
        mode = va_arg(ap, mode_t) & ~S_IXUSR;
        attr = va_arg(ap, struct mq_attr *);
        va_end(ap);

        /* open and specify O_EXCL and user-execute */
        fd = open(get_path(pathname), oflag | O_EXCL | O_RDWR, mode | S_IXUSR);
        if (fd < 0) {
            if (errno == EEXIST && (oflag & O_EXCL) == 0)
                goto exists;            /* already exists, OK */
            else
                return((mqd_t) -1);
            }
            created = 1;
                        /* first one to create the file initializes it */
            if (attr == NULL)
                attr = &defattr;
            else {
                if (attr->mq_maxmsg <= 0 || attr->mq_msgsize <= 0) {
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
            if ( (emqinfo = malloc(sizeof(struct emq_info))) == NULL)
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
            for (i = 0; i < attr->mq_maxmsg - 1; i++) {
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
            if (chmod(get_path(pathname), mode) == -1)
#else
            if (fchmod(fd, mode) == -1)
#endif
                goto err;
            close(fd);
            if (SUCCEED!=qd_hash_add((mqd_t) emqinfo)) {
                NDRX_LOG(log_error, "Failed to add mqd_t to hash!");
                errno = ENOMEM;
            }
            NDRX_LOG(log_debug, "into: emq_open ret ok");
            return((mqd_t) emqinfo);
    }
exists:
    /* open the file then memory map */
    if ( (fd = open(get_path(pathname), O_RDWR)) < 0) {
        if (errno == ENOENT && (oflag & O_CREAT))
            goto again;
        goto err;
    }

    /* make certain initialization is complete */
    for (i = 0; i < MAX_TRIES; i++) {
        if (stat(get_path(pathname), &statbuff) == -1) {
            if (errno == ENOENT && (oflag & O_CREAT)) {
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
    if (i == MAX_TRIES) {
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
    if ( (emqinfo = malloc(sizeof(struct emq_info))) == NULL)
        goto err;
    emqinfo->emqi_hdr = (struct emq_hdr *) mptr;
    emqinfo->emqi_magic = EMQI_MAGIC;
    emqinfo->emqi_flags = nonblock;
    
    if (SUCCEED!=qd_hash_add((mqd_t) emqinfo)) {
        NDRX_LOG(log_error, "Failed to add mqd_t to hash!");
        errno = ENOMEM;
    }
    
    return((mqd_t) emqinfo);
    NDRX_LOG(log_debug, "into: emq_open ret ok");
pthreaderr:
    errno = i;
err:
    /* don't let following function calls change errno */
    save_errno = errno;
    if (created)
        unlink(get_path(pathname));
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
        free(emqinfo);
    close(fd);
    errno = save_errno;
    NDRX_LOG(log_debug, "into: emq_open ret -1");
    return((mqd_t) -1);
}


ssize_t emq_timedreceive(mqd_t emqd, char *ptr, size_t maxlen, unsigned int *priop,
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
    int try =0;

retry:
    NDRX_LOG(log_debug, "into: emq_timedreceive");
    
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
    NDRX_LOG(log_debug, "emq_timedreceive YOPT %p", emqd);
    emqhdr = emqinfo->emqi_hdr;        /* struct pointer */
    mptr = (char *) emqhdr;          /* byte pointer */
    attr = &emqhdr->emqh_attr;
    if ( (n = pthread_mutex_lock(&emqhdr->emqh_lock)) != 0) {
        errno = n;
        return(-1);
    }
    NDRX_LOG(log_debug, "emq_timedreceive YOPT %p", emqd);

    if (maxlen < (size_t)attr->mq_msgsize) {
        errno = EMSGSIZE;
        goto err;
    }
    NDRX_LOG(log_debug, "emq_timedreceive YOPT %p", emqd);
    if (attr->mq_curmsgs == 0) {            /* queue is empty */
        if (emqinfo->emqi_flags & O_NONBLOCK) {
            errno = EAGAIN;
            goto err;
        }
    NDRX_LOG(log_debug, "emq_timedreceive YOPT %p", emqd);
        /* wait for a message to be placed onto queue */
        emqhdr->emqh_nwait++;
        while (attr->mq_curmsgs == 0)
        {
            if (try>10)
            {
                try = 0;
                NDRX_LOG(log_warn, "Doing retry...");
                emqhdr->emqh_nwait--;
                pthread_mutex_unlock(&emqhdr->emqh_lock);
                goto retry;
            }
            NDRX_LOG(log_debug, "queue empty on %p", emqd);
            if (NULL==__abs_timeout) {
                pthread_cond_wait(&emqhdr->emqh_wait, &emqhdr->emqh_lock);
            } else {
                /* wait some time...  */
                if (0!=pthread_cond_timedwait(&emqhdr->emqh_wait, &emqhdr->emqh_lock, 
                        __abs_timeout))
                {
                    NDRX_LOG(log_debug, "%p timed out", emqd);
                    errno = ETIMEDOUT;
                    goto err;
                }
            }
        }
    NDRX_LOG(log_debug, "emq_timedreceive YOPT %p", emqd);
        emqhdr->emqh_nwait--;
    }

    NDRX_LOG(log_debug, "emq_timedreceive YOPT %p", emqd);
    if ( (index = emqhdr->emqh_head) == 0) {
        NDRX_LOG(log_error, "emq_timedreceive: curmsgs = %ld; head = 0",attr->mq_curmsgs);
        abort();
    }

    NDRX_LOG(log_debug, "emq_timedreceive YOPT %p", emqd);
    msghdr = (struct msg_hdr *) &mptr[index];
    emqhdr->emqh_head = msghdr->msg_next;     /* new head of list */
    len = msghdr->msg_len;
    memcpy(ptr, msghdr + 1, len);           /* copy the message itself */
    if (priop != NULL)
        *priop = msghdr->msg_prio;

    NDRX_LOG(log_debug, "emq_timedreceive YOPT %p", emqd);
    /* just-read message goes to front of free list */
    msghdr->msg_next = emqhdr->emqh_free;
    emqhdr->emqh_free = index;

    NDRX_LOG(log_debug, "emq_timedreceive YOPT %p", emqd);
    /* wake up anyone blocked in emq_send waiting for room */
    if (attr->mq_curmsgs == attr->mq_maxmsg)
    {
    /*    pthread_cond_signal(&emqhdr->emqh_wait); */
        pthread_cond_broadcast(&emqhdr->emqh_wait);
    }
    attr->mq_curmsgs--;

    NDRX_LOG(log_debug, "emq_timedreceive YOPT %p", emqd);
    pthread_mutex_unlock(&emqhdr->emqh_lock);
    
    NDRX_LOG(log_debug, "emq_timedreceive - got something len=%d", len);
    return(len);

err:
    pthread_mutex_unlock(&emqhdr->emqh_lock);

    NDRX_LOG(log_debug, "emq_timedreceive - failed");
    return(-1);
}

int emq_timedsend(mqd_t emqd, const char *ptr, size_t len, unsigned int prio, 
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
    int try = 0;

retry:
    NDRX_LOG(log_debug, "into: emq_timedsend");

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
    emqhdr = emqinfo->emqi_hdr;        /* struct pointer */
    mptr = (char *) emqhdr;          /* byte pointer */
    attr = &emqhdr->emqh_attr;
    if ( (n = pthread_mutex_lock(&emqhdr->emqh_lock)) != 0) {
        errno = n;
        return(-1);
    }

    if (len > (size_t)attr->mq_msgsize) {
        errno = EMSGSIZE;
        goto err;
    }
    if (attr->mq_curmsgs == 0) {
        if (emqhdr->emqh_pid != 0 && emqhdr->emqh_nwait == 0) {
            sigev = &emqhdr->emqh_event;
#if !defined(WIN32)
            if (sigev->sigev_notify == SIGEV_SIGNAL) {
                /*sigqueue(emqhdr->emqh_pid, sigev->sigev_signo,
                                         sigev->sigev_value);*/
                kill(emqhdr->emqh_pid, sigev->sigev_signo);
            }
#endif
            emqhdr->emqh_pid = 0;             /* unregister */
        }
    } else if (attr->mq_curmsgs >= attr->mq_maxmsg) {
        /* queue is full */
        if (emqinfo->emqi_flags & O_NONBLOCK) {
            errno = EAGAIN;
            goto err;
        }
        /* wait for room for one message on the queue */
        while (attr->mq_curmsgs >= attr->mq_maxmsg) {
            
            try++;
            if (try>10)
            {
                try = 0;
                /* This stuff we need for Darwin. For some reason
                 * it hangs on mutex it is always retruning
                 * and sender cannot get access to Q anymore.
                 * Thus forcibly we will close the mutex and re-enter
                 * again... (with out this test case 012 fails)
                 */
                NDRX_LOG(log_warn, "Doing retry...");
                pthread_mutex_unlock(&emqhdr->emqh_lock);
                goto retry;
            }
            NDRX_LOG(log_warn, "waiting on q %p", emqd);
            if (NULL==__abs_timeout) {
                /* Seems  we might get stalled, if
                 * the queue is full and there is tons of senders..
                 * Thuse better use wait with timeout. */
                pthread_cond_wait(&emqhdr->emqh_wait, &emqhdr->emqh_lock);
#if 0
                NDRX_LOG(log_debug, "w/o time-out");
                usleep(2000);
                struct timespec abs_timeout;
                struct timeval  timeval;
                gettimeofday (&timeval, NULL);
                abs_timeout.tv_sec = timeval.tv_sec+2; /* wait two secc */
                abs_timeout.tv_nsec = timeval.tv_usec*1000;
                        
                pthread_cond_timedwait(&emqhdr->emqh_wait, &emqhdr->emqh_lock, 
                        &abs_timeout);
#endif
            }
            else {
                /* wait some time...  */
                struct timespec abs_timeout;
                struct timeval  timeval;
                NDRX_LOG(log_debug, "timed wait...");
                gettimeofday (&timeval, NULL);
                abs_timeout.tv_sec = timeval.tv_sec+2; /* wait two secc */
                abs_timeout.tv_nsec = timeval.tv_usec*1000;

                if (timeval.tv_sec > __abs_timeout->tv_sec ||
                      (timeval.tv_sec == __abs_timeout->tv_sec && 
				timeval.tv_usec*1000 > __abs_timeout->tv_nsec))
                {
                    NDRX_LOG(log_error, "Call already expired...");
                    errno = ETIMEDOUT;
                    goto err;
                }

                if (0!=pthread_cond_timedwait(&emqhdr->emqh_wait, &emqhdr->emqh_lock, 
                        __abs_timeout))
                {
                    errno = ETIMEDOUT;
                    goto err;
                }
                abs_timeout.tv_sec = timeval.tv_sec+2; /* wait two secc */
                abs_timeout.tv_nsec = timeval.tv_usec*1000;
            }
            NDRX_LOG(log_warn, "%p - accessed ok", emqd);
        }
    }
    /* nmsghdr will point to new message */
    if ( (freeindex = emqhdr->emqh_free) == 0) {
        fprintf(stderr, "emq_send: curmsgs = %ld; free = 0", attr->mq_curmsgs);
    }

    nmsghdr = (struct msg_hdr *) &mptr[freeindex];
    nmsghdr->msg_prio = prio;
    nmsghdr->msg_len = len;
    memcpy(nmsghdr + 1, ptr, len);          /* copy message from caller */
    emqhdr->emqh_free = nmsghdr->msg_next;    /* new freelist head */

    /* find right place for message in linked list */
    index = emqhdr->emqh_head;
    pmsghdr = (struct msg_hdr *) &(emqhdr->emqh_head);
    while (index != 0) {
        msghdr = (struct msg_hdr *) &mptr[index];
        if (prio > msghdr->msg_prio) {
            nmsghdr->msg_next = index;
            pmsghdr->msg_next = freeindex;
            break;
        }
        index = msghdr->msg_next;
        pmsghdr = msghdr;
    }
    if (index == 0) {
        /* queue was empty or new goes at end of list */
        pmsghdr->msg_next = freeindex;
        nmsghdr->msg_next = 0;
    }
    /* wake up anyone blocked in emq_receive waiting for a message */ 
    if (attr->mq_curmsgs == 0)
    {
    /*    pthread_cond_signal(&emqhdr->emqh_wait); */
        pthread_cond_broadcast(&emqhdr->emqh_wait);
    }
    attr->mq_curmsgs++;

    pthread_mutex_unlock(&emqhdr->emqh_lock);
    NDRX_LOG(log_debug, "into: emq_timedsend - return 0");
    return(0);

err:
    pthread_mutex_unlock(&emqhdr->emqh_lock);
    NDRX_LOG(log_debug, "into: emq_timedsend - return -1");
    return(-1);
}

/**
 * TODO implement.
 */
int emq_send(mqd_t emqd, const char *ptr, size_t len, unsigned int prio)
{
    return emq_timedsend(emqd, ptr, len, prio, NULL);
}
    
/**
 * TODO: implement.
 */
ssize_t emq_receive(mqd_t emqd, char *ptr, size_t maxlen, unsigned int *priop)
{
    return emq_timedreceive(emqd, ptr, maxlen, priop, NULL);
}


int emq_setattr(mqd_t emqd, const struct mq_attr *emqstat, struct mq_attr *oemqstat)
{
    int             n;
    struct emq_hdr  *emqhdr;
    struct mq_attr *attr;
    struct emq_info *emqinfo;

    NDRX_LOG(log_debug, "into: emq_setattr");

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
    attr = &emqhdr->emqh_attr;
    if ( (n = pthread_mutex_lock(&emqhdr->emqh_lock)) != 0) {
        errno = n;
        return(-1);
    }

    if (oemqstat != NULL) {
        oemqstat->mq_flags = emqinfo->emqi_flags;  /* previous attributes */
        oemqstat->mq_maxmsg = attr->mq_maxmsg;
        oemqstat->mq_msgsize = attr->mq_msgsize;
        oemqstat->mq_curmsgs = attr->mq_curmsgs; /* and current status */
    }

    if (emqstat->mq_flags & O_NONBLOCK)
        emqinfo->emqi_flags |= O_NONBLOCK;
    else
        emqinfo->emqi_flags &= ~O_NONBLOCK;

    pthread_mutex_unlock(&emqhdr->emqh_lock);
    NDRX_LOG(log_debug, "into: emq_setattr - return 0");
    return(0);
}

int emq_unlink(const char *pathname)
{
    NDRX_LOG(log_debug, "into: emq_unlink");
    if (unlink(get_path(pathname)) == -1)
    {
        NDRX_LOG(log_debug, "into: emq_unlink ret -1");
        return(-1);
    }
    NDRX_LOG(log_debug, "into: emq_unlink ret 0");
    return(0);
}
