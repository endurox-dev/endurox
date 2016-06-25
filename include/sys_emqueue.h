/*****************************************************************************
 *
 * POSIX Message Queue library implemented using memory mapped files
 *
 *****************************************************************************/
#ifndef __sys_emqueue_h
#define __sys_emqueue_h
#include <pthread.h>
#include <unistd.h>
#include <sys/signal.h>

#if defined(WIN32)
#   include <fcntl.h>
#   define EMSGSIZE    4200
#   define O_NONBLOCK  0200000

    union sigval {
        int           sival_int;     /* integer value */
        void          *sival_ptr;    /* pointer value */
    };
    struct sigevent {
        int           sigev_notify;  /* notification type */
        int           sigev_signo;   /* signal number */
        union sigval  sigev_value;   /* signal value */
    };
    typedef int pid_t;
    typedef int ssize_t;
#endif

/*****************************************************************************/

typedef struct emq_info *mqd_t;       /* opaque datatype */

struct mq_attr {
    long mq_flags;     /* message queue flag: O_NONBLOCK */
    long mq_maxmsg;    /* max number of messages allowed on queue */
    long mq_msgsize;   /* max size of a message (in bytes) */
    long mq_curmsgs;   /* number of messages currently on queue */
};

/* one emq_hdr{} per queue, at beginning of mapped file */
struct emq_hdr {
    struct mq_attr    emqh_attr;  /* the queue's attributes */
    long              emqh_head;  /* index of first message */
    long              emqh_free;  /* index of first free message */
    long              emqh_nwait; /* #threads blocked in emq_receive() */
    pid_t             emqh_pid;   /* nonzero PID if emqh_event set */
    struct sigevent   emqh_event; /* for emq_notify() */
    pthread_mutex_t   emqh_lock;  /* mutex lock */
    pthread_cond_t    emqh_wait;  /* and condition variable */
};

/* one msg_hdr{} at the front of each message in the mapped file */
struct msg_hdr {
    long            msg_next;    /* index of next on linked list */
                                 /* msg_next must be first member in struct */
    ssize_t         msg_len;     /* actual length */
    unsigned int    msg_prio;    /* priority */
};

/* one emq_info{} malloc'ed per process per emq_open() */
struct emq_info {
#if defined(WIN32)
    HANDLE         emqi_fmap;     /* file mapping object */
#endif
    struct emq_hdr *emqi_hdr;      /* start of mmap'ed region */
    long           emqi_magic;    /* magic number if open */
    int            emqi_flags;    /* flags for this process */
};
#define EMQI_MAGIC  0x98765432

/* size of message in file is rounded up for alignment */
#define MSGSIZE(i) ((((i) + sizeof(long)-1) / sizeof(long)) * sizeof(long))

/* message queue functions */
extern int     emq_close(mqd_t);
extern int     emq_getattr(mqd_t, struct mq_attr *);
extern int     emq_notify(mqd_t, const struct sigevent *);
extern mqd_t   emq_open(const char *, int, ...);
extern ssize_t emq_receive(mqd_t, char *, size_t, unsigned int *);
extern int     emq_send(mqd_t, const char *, size_t, unsigned int);
extern int     emq_setattr(mqd_t, const struct mq_attr *, struct mq_attr *);
extern int     emq_unlink(const char *name);

extern int emq_timedsend(mqd_t emqd, const char *ptr, size_t len, unsigned int prio,
        const struct timespec *__abs_timeout);    
extern  ssize_t emq_timedreceive(mqd_t emqd, char *ptr, size_t maxlen, unsigned int *priop,
        const struct timespec *__restrict __abs_timeout);

extern void emq_set_lock_timeout(int secs);
        
#endif
