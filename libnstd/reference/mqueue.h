/*****************************************************************************
 *
 * POSIX Message Queue library implemented using memory mapped files
 *
 *****************************************************************************/
#ifndef __mqueue_h
#define __mqueue_h
#include <pthread.h>

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

typedef struct mq_info *mqd_t;       /* opaque datatype */

struct mq_attr {
    long mq_flags;     /* message queue flag: O_NONBLOCK */
    long mq_maxmsg;    /* max number of messages allowed on queue */
    long mq_msgsize;   /* max size of a message (in bytes) */
    long mq_curmsgs;   /* number of messages currently on queue */
};

/* one mq_hdr{} per queue, at beginning of mapped file */
struct mq_hdr {
    struct mq_attr    mqh_attr;  /* the queue's attributes */
    long              mqh_head;  /* index of first message */
    long              mqh_free;  /* index of first free message */
    long              mqh_nwait; /* #threads blocked in mq_receive() */
    pid_t             mqh_pid;   /* nonzero PID if mqh_event set */
    struct sigevent   mqh_event; /* for mq_notify() */
    pthread_mutex_t   mqh_lock;  /* mutex lock */
    pthread_cond_t    mqh_wait;  /* and condition variable */
};

/* one msg_hdr{} at the front of each message in the mapped file */
struct msg_hdr {
    long            msg_next;    /* index of next on linked list */
                                 /* msg_next must be first member in struct */
    ssize_t         msg_len;     /* actual length */
    unsigned int    msg_prio;    /* priority */
};

/* one mq_info{} malloc'ed per process per mq_open() */
struct mq_info {
#if defined(WIN32)
    HANDLE         mqi_fmap;     /* file mapping object */
#endif
    struct mq_hdr *mqi_hdr;      /* start of mmap'ed region */
    long           mqi_magic;    /* magic number if open */
    int            mqi_flags;    /* flags for this process */
};
#define MQI_MAGIC  0x98765432

/* size of message in file is rounded up for alignment */
#define MSGSIZE(i) ((((i) + sizeof(long)-1) / sizeof(long)) * sizeof(long))

/* message queue functions */
extern int     mq_close(mqd_t);
extern int     mq_getattr(mqd_t, struct mq_attr *);
extern int     mq_notify(mqd_t, const struct sigevent *);
extern mqd_t   mq_open(const char *, int, ...);
extern ssize_t mq_receive(mqd_t, char *, size_t, unsigned int *);
extern int     mq_send(mqd_t, const char *, size_t, unsigned int);
extern int     mq_setattr(mqd_t, const struct mq_attr *, struct mq_attr *);
extern int     mq_unlink(const char *name);

#endif
