/* ********************************
 * Author:       Johan Hanssen Seferidis
 * License:	     MIT
 * Description:  Library providing a threading pool where you can add
 *               work. For usage, check the thpool.h file or README.md
 *
 *//** @file thpool.h *//*
 *
 ********************************/

/* #define _POSIX_C_SOURCE 200809L */
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <sys/time.h> 
#include <ndebug.h>
#include <time.h>
#include <ndrstandard.h>
#ifdef EX_OS_LINUX
#include <sys/prctl.h>
#endif

#include <exthpool.h>

#include <nstdutil.h>
#include <ndrxdiag.h>

#ifdef THPOOL_DEBUG
#define THPOOL_DEBUG 1
#else
#define THPOOL_DEBUG 0
#endif

#if !defined(DISABLE_PRINT) || defined(THPOOL_DEBUG)
#define err(str) NDRX_LOG(log_error, str)
#else
#define err(str)
#endif

/* ========================== STRUCTURES ============================ */


/* Binary semaphore */
typedef struct bsem 
{
    pthread_mutex_t mutex;
    pthread_cond_t   cond;
    int v;
} bsem;


/* Job */
typedef struct job
{
    struct job*  prev;                   /* pointer to previous job   */
    void  (*function)(void* arg, int *p_finish_off);/* function pointer*/
    void*  arg;                          /* function's argument       */
} job;


/* Job queue */
typedef struct jobqueue
{
    job  *front;                         /* pointer to front of queue */
    job  *rear;                          /* pointer to rear  of queue */
    bsem *has_jobs;                      /* flag as binary semaphore  */
    int   len;                           /* number of jobs in queue   */
} jobqueue;


/* Thread */
typedef struct poolthread
{
    int       id;                        /* friendly id               */
    pthread_t pthread;                   /* pointer to actual thread  */
    struct thpool_* thpool_p;            /* access to thpool          */
} poolthread;


/* Threadpool */
typedef struct thpool_
{
    poolthread**   threads;                 /**< pointer to threads         */
    volatile int num_threads_alive;         /**< threads currently alive    */
    volatile int num_threads_working;       /**< threads currently working  */
    pthread_mutex_t  thcount_lock;          /**< used for thread count etc  */
    pthread_cond_t  threads_all_idle;       /**< signal to thpool_wait      */
    pthread_cond_t  threads_one_idle;       /**< signal to thpool_wait_one  */
    pthread_cond_t  proc_one;               /**< One job is processed       */
    int threads_keepalive;
    int num_threads;                        /**< total number of threads    */
    int thread_status;                      /**< if EXTRUE, init OK         */
    jobqueue  jobqueue;                     /**< job queue                  */
    ndrx_thpool_tpsvrthrinit_t pf_init;    /**< init function, if any      */
    ndrx_thpool_tpsvrthrdone_t pf_done;    /**< done function, if any      */
    int argc;                               /**< cli argument count         */
    char **argv;                            /**< cli arguments              */
} thpool_;


/* ========================== PROTOTYPES ============================ */

static int  poolthread_init(thpool_* thpool_p, struct poolthread** thread_p, int id);
static void* poolthread_do(struct poolthread* thread_p);
static void  poolthread_hold(int sig_id);
static void  poolthread_destroy(struct poolthread* thread_p);

static int   jobqueue_init(jobqueue* jobqueue_p);
static void  jobqueue_clear(jobqueue* jobqueue_p);
static void  jobqueue_push(jobqueue* jobqueue_p, struct job* newjob_p);
static struct job* jobqueue_pull(jobqueue* jobqueue_p);
static void  jobqueue_destroy(jobqueue* jobqueue_p);

static void  bsem_init(struct bsem *bsem_p, int value);
static void  bsem_reset(struct bsem *bsem_p);
static void  bsem_post(struct bsem *bsem_p);
static void  bsem_post_all(struct bsem *bsem_p);
static void  bsem_wait(struct bsem *bsem_p);


/* ========================== THREADPOOL ============================ */


/* Initialise thread pool 
 * @param num_threads number of threads
 * @param p_ret return status;
 * @param pf_init thread init func
 * @param pf_done thread done func
 * @param argc command line arguments for thread init func
 * @param argv commanc line arguments for thread init func
 * @return thread pool (contain 0 or more intitialized threads) or NULL on fail
 */
struct thpool_* ndrx_thpool_init(int num_threads, int *p_ret,
        ndrx_thpool_tpsvrthrinit_t pf_init, ndrx_thpool_tpsvrthrdone_t pf_done,
        int argc, char **argv)
{

    thpool_* thpool_p;
    if (num_threads < 0)
    {
        num_threads = 0;
    }

    /* Make new thread pool */
    thpool_p = (struct thpool_*)NDRX_FPMALLOC(sizeof(struct thpool_), 0);
    if (thpool_p == NULL)
    {
        err("thpool_init(): Could not allocate memory for thread pool\n");
        return NULL;
    }
    thpool_p->num_threads   = 0;
    thpool_p->threads_keepalive = 1;
    thpool_p->num_threads_alive   = 0;
    thpool_p->num_threads_working = 0;
    thpool_p->pf_init = pf_init;
    thpool_p->pf_done = pf_done;
    thpool_p->argc = argc;
    thpool_p->argv = argv;

    /* Initialise the job queue */
    if (jobqueue_init(&thpool_p->jobqueue) == -1)
    {
        err("thpool_init(): Could not allocate memory for job queue\n");
        NDRX_FPFREE(thpool_p);
        return NULL;
    }

    /* Make threads in pool */
    thpool_p->threads = (struct poolthread**)NDRX_FPMALLOC(num_threads * sizeof(struct poolthread *), 0);
    
    if (thpool_p->threads == NULL)
    {
        err("thpool_init(): Could not allocate memory for threads\n");
        jobqueue_destroy(&thpool_p->jobqueue);
        NDRX_FPFREE(thpool_p);
        return NULL;
    }

    pthread_mutex_init(&(thpool_p->thcount_lock), NULL);
    pthread_cond_init(&thpool_p->threads_all_idle, NULL);
    pthread_cond_init(&thpool_p->threads_one_idle, NULL);
    pthread_cond_init(&thpool_p->proc_one, NULL);

    /* Thread init, lets do one by one... and check the final counts... */
    int n;
    for (n=0; n<num_threads; n++)
    {
        /* lock here */
        thpool_p->thread_status = EXSUCCEED;
        MUTEX_LOCK_V(thpool_p->thcount_lock);
        
        /* run off the thread */
        if (EXSUCCEED!=poolthread_init(thpool_p, &thpool_p->threads[n], n))
        {
            if (NULL!=p_ret)
            {
                *p_ret=EXFAIL;
            }
        
            /* unlock the mutex, as thread failed to init... */
            MUTEX_UNLOCK_V(thpool_p->thcount_lock);
            goto out;
        }
        
        /* wait for init complete */
        pthread_cond_wait(&thpool_p->threads_one_idle, &thpool_p->thcount_lock);
        
        MUTEX_UNLOCK_V(thpool_p->thcount_lock);
        
        /* Check the status... */
        if (EXFAIL==thpool_p->thread_status)
        {
            /* join this one... and terminate all */
            pthread_join(thpool_p->threads[n]->pthread, NULL);
            /* indicate that we finish off */
            if (NULL!=p_ret)
            {
                *p_ret=EXFAIL;
            }
        }
        
#if THPOOL_DEBUG
            printf("THPOOL_DEBUG: Created thread %d in pool \n", n);
#endif
    }

    /* TODO: Wait for threads to initialize 
    while (thpool_p->num_threads_alive != num_threads) {sched_yield();}
     * */
    
out:
    return thpool_p;
}

int ndrx_thpool_add_work2(thpool_* thpool_p, void (*function_p)(void*, int *), void* arg_p, long flags, int max_len)
{
    int ret = EXSUCCEED;
    job* newjob;

    newjob=(struct job*)NDRX_FPMALLOC(sizeof(struct job), 0);
    
    if (newjob==NULL)
    {
        err("thpool_add_work(): Could not allocate memory for new job\n");
        return -1;
    }

    /* add function and argument */
    newjob->function=function_p;
    newjob->arg=arg_p;
    
    /* add job to queue */
    MUTEX_LOCK_V(thpool_p->thcount_lock);
    
    if (flags & NDRX_THPOOL_ONEJOB && thpool_p->jobqueue.len > 0)
    {
        NDRX_LOG(log_debug, "NDRX_THPOOL_ONEJOB set and queue len is %d - skip this job",
                thpool_p->jobqueue.len);
        
        /* WARNING !!!! Early return! */
        NDRX_FPFREE(newjob);
        MUTEX_UNLOCK_V(thpool_p->thcount_lock);
        return NDRX_THPOOL_ONEJOB;
    }

    /* wait for pool to process some amount of jobs, before we continue....
     * otherwise this might cause us to buffer lot of outgoing messages
     */
    if (max_len > 0)
    {
        while (thpool_p->jobqueue.len > max_len)
        {
            /* wait for 1 sec... so that we release some control */
            struct timespec wait_time;
            struct timeval now;

            gettimeofday(&now,NULL);

            wait_time.tv_sec = now.tv_sec+1;
            wait_time.tv_nsec = now.tv_usec*1000;

            if (EXSUCCEED!=(ret=pthread_cond_timedwait(&thpool_p->proc_one, 
                    &thpool_p->thcount_lock, &wait_time)))
            {
                NDRX_LOG(log_error, "Waiting for %d jobs (current: %d) but expired... (err: %s)", 
                        max_len, thpool_p->jobqueue.len, strerror(ret));            
                
                /* allow to continue... */
                break;
            }
        }
    }

    jobqueue_push(&thpool_p->jobqueue, newjob);
    
    MUTEX_UNLOCK_V(thpool_p->thcount_lock);

    return 0;
}

/* Add work to the thread pool 
 * default version
 */
int ndrx_thpool_add_work(thpool_* thpool_p, void (*function_p)(void*, int *), void* arg_p)
{
    return ndrx_thpool_add_work2(thpool_p, function_p, arg_p, 0, 0);
}

/**
 *  Wait until all jobs have finished 
 * called by dispatch tread
 */
void ndrx_thpool_wait(thpool_* thpool_p)
{
    MUTEX_LOCK_V(thpool_p->thcount_lock);
    
    while (thpool_p->jobqueue.len || thpool_p->num_threads_working)
    {
        pthread_cond_wait(&thpool_p->threads_all_idle, &thpool_p->thcount_lock);
    }
    MUTEX_UNLOCK_V(thpool_p->thcount_lock);
}

/**
 * Wait until one thread is free.
 * Called by dispatch thread.
 * @param thpool_p 
 */
void ndrx_thpool_wait_one(thpool_* thpool_p)
{
    MUTEX_LOCK_V(thpool_p->thcount_lock);

    /* Wait for at-leat one free thread (i.e.) no job found... */
    while ( (thpool_p->jobqueue.len - 
            (thpool_p->num_threads-thpool_p->num_threads_working) >= 0 ))
    {
        pthread_cond_wait(&thpool_p->threads_one_idle, &thpool_p->thcount_lock);
    }

    MUTEX_UNLOCK_V(thpool_p->thcount_lock);
}


/**
 * Get number of non working and non job scheduled threads
 * @param thpool_p thread pool
 * @return number of threads fully free (no job scheduled)
 */
int ndrx_thpool_nr_not_working(thpool_* thpool_p)
{
    int nr;
    
    MUTEX_LOCK_V(thpool_p->thcount_lock);

    nr = thpool_p->num_threads - thpool_p->num_threads_working - thpool_p->jobqueue.len;
    
    MUTEX_UNLOCK_V(thpool_p->thcount_lock);
    
    return nr;
}

/**
 * Is one thread available?
 * @param thpool_p 
 * @return EXTRUE/EXFALSE
 */
int ndrx_thpool_is_one_avail(thpool_* thpool_p)
{
    int ret = EXFALSE;
    
    MUTEX_LOCK_V(thpool_p->thcount_lock);

    /* Wait for at-leat one free thread (i.e.) no job found... */
    if ( (thpool_p->jobqueue.len - 
            (thpool_p->num_threads-thpool_p->num_threads_working) < 0 ))
    {
        ret=EXTRUE;
    }

    MUTEX_UNLOCK_V(thpool_p->thcount_lock);
out:
    return ret;
}


/* Destroy the threadpool */
void ndrx_thpool_destroy(thpool_* thpool_p)
{
    int n;
    volatile int threads_total = thpool_p->num_threads;
    /* Give one second to kill idle threads */
    double TIMEOUT = 1.0;
    time_t start, end;
    double tpassed = 0.0;
    time (&start);
    
    /* No need to destory if it's NULL */
    if (thpool_p == NULL) return ;
    /* End each thread 's infinite loop */
    thpool_p->threads_keepalive = 0;
    
    /* num_threads_alive - reads are atomic... as the same as writes. */
    while (tpassed < TIMEOUT && thpool_p->num_threads_alive)
    {
        bsem_post_all(thpool_p->jobqueue.has_jobs);
        time (&end);
        tpassed = difftime(end,start);
    }
    
    /* Poll remaining threads */
    while (thpool_p->num_threads_alive)
    {
        bsem_post_all(thpool_p->jobqueue.has_jobs);
        sleep(1);
    }
    
    /* join threads... */
    for (n=0; n<thpool_p->num_threads; n++)
    {
        pthread_join(thpool_p->threads[n]->pthread, NULL);
    }
    
    /* Job queue cleanup */
    jobqueue_destroy(&thpool_p->jobqueue);
    
    /* avoid mem leak #250 */
    for (n=0; n < threads_total; n++)
    {
        poolthread_destroy(thpool_p->threads[n]);
    }
    
    NDRX_FPFREE(thpool_p->threads);
    NDRX_FPFREE(thpool_p);
}

/* ============================ THREAD ============================== */


/* Initialize a thread in the thread pool
 *
 * @param thread        address to the pointer of the thread to be created
 * @param id            id to be given to the thread
 * @return 0 on success, -1 otherwise.
 */
static int poolthread_init (thpool_* thpool_p, struct poolthread** thread_p, int id)
{
    int ret = EXSUCCEED;
    pthread_attr_t pthread_custom_attr;
    pthread_attr_init(&pthread_custom_attr);

    *thread_p = (struct poolthread*)NDRX_FPMALLOC(sizeof(struct poolthread), 0);

    if (*thread_p == NULL)
    {
        err("poolthread_init(): Could not allocate memory for thread\n");
        return -1;
    }

    (*thread_p)->thpool_p = thpool_p;
    (*thread_p)->id       = id;

    /* have some stack space... */
    ndrx_platf_stack_set(&pthread_custom_attr);

    if (EXSUCCEED!=pthread_create(&(*thread_p)->pthread, &pthread_custom_attr,
                    (void *)poolthread_do, (*thread_p)))
    {
        NDRX_PLATF_DIAG(NDRX_DIAG_PTHREAD_CREATE, errno, "poolthread_init");
        EXFAIL_OUT(ret);
    }
    /* pthread_detach((*thread_p)->pthread); */
out:
    return ret;
}

/* What each thread is doing
*
* In principle this is an endless loop. The only time this loop gets interuppted is once
* thpool_destroy() is invoked or the program exits.
*
* @param  thread        thread that will run this function
* @return nothing
*/
static void* poolthread_do(struct poolthread* thread_p)
{

    /* Set thread name for profiling and debugging */
    int finish_off = 0;
    int all_idle;
    int one_idle;
    int ret = EXSUCCEED;
    char thread_name[128] = {0};
    snprintf(thread_name, sizeof(thread_name), "thread-pool-%d", thread_p->id);

#ifdef EX_OS_LINUX
    prctl(PR_SET_NAME, thread_name);
#endif

    /* Assure all threads have been created before starting serving */
    thpool_* thpool_p = thread_p->thpool_p;
    
    /* run off the init, if any... */
    if (NULL!=thread_p->thpool_p->pf_init)
    {
        NDRX_LOG(log_debug, "About to call tpsvrthrinit()");
        ret = thread_p->thpool_p->pf_init(thread_p->thpool_p->argc, 
                thread_p->thpool_p->argv);
        
        if (EXSUCCEED!=ret)
        {
            NDRX_LOG(log_error, "tpsvrthrinit() failed %d", ret);
            userlog("tpsvrthrinit() failed %d", ret);
        }
        else
        {
            NDRX_LOG(log_debug, "tpsvrthrinit() OK");
        }
    }
    
    /* 
     * Signal that we are ready
     */
    MUTEX_LOCK_V(thpool_p->thcount_lock);
    /* Mark thread as alive (initialized) 
     * We could use mutex here, as these are called only at init.
     */
    if (EXSUCCEED==ret)
    {
        thpool_p->num_threads_alive += 1;
        thpool_p->num_threads+=1;
    }
    else
    {
        thpool_p->thread_status = EXFAIL;
        return NULL;
    }
    
    pthread_cond_signal(&thpool_p->threads_one_idle);
    MUTEX_UNLOCK_V(thpool_p->thcount_lock);
    
    while(thread_p->thpool_p->threads_keepalive && !finish_off)
    {
        bsem_wait(thpool_p->jobqueue.has_jobs);

        if (thread_p->thpool_p->threads_keepalive)
        {
            void(*func_buff)(void* arg, int *p_finish_off);
            void*  arg_buff;
            job* job_p;
            
            MUTEX_LOCK_V(thpool_p->thcount_lock);
            thpool_p->num_threads_working++;
            

            /* Read job from queue and execute it */
            job_p = jobqueue_pull(&thpool_p->jobqueue);
            
            /* FOR WAIT ONE.. HAVE FIXED LEN */
            MUTEX_UNLOCK_V(thpool_p->thcount_lock);
            if (job_p)
            {
                func_buff = job_p->function;
                arg_buff  = job_p->arg;
                func_buff(arg_buff, &finish_off);
                NDRX_FPFREE(job_p);
            }

            /* if no threads working, wake up the shutdown
             * waiter...
             * It will check that there are no any jobs
             * To get atomic signaling to waiter, we must lock
             * the decision and signal are. So that we catch any
             * waiter which might stepping in for signal wait, 
             * but we here get faster and trigger the signal.
             * and the waiter 
             */
            all_idle=EXFALSE;
            one_idle=EXFALSE;
            MUTEX_LOCK_V(thpool_p->thcount_lock);

            /* get the final change before trigger */
            thpool_p->num_threads_working--;
            
            if (!thpool_p->num_threads_working)
            {
                all_idle=EXTRUE;
            }
            
            if ((thpool_p->jobqueue.len - 
                (thpool_p->num_threads-thpool_p->num_threads_working) < 0 ))
            {
                one_idle=EXTRUE;
            }
            
            if (all_idle)
            {  
                pthread_cond_signal(&thpool_p->threads_all_idle);
            } 
            
            if (one_idle)
            {  
                pthread_cond_signal(&thpool_p->threads_one_idle);
            }
            
            /* one job is processed... */
            pthread_cond_signal(&thpool_p->proc_one);

            MUTEX_UNLOCK_V(thpool_p->thcount_lock);

        }
    }
    
    /* terminate it off... */
    if (NULL!=thread_p->thpool_p->pf_done)
    {
        thread_p->thpool_p->pf_done();
    }
    
    MUTEX_LOCK_V(thpool_p->thcount_lock);
    thpool_p->num_threads_alive --;
    MUTEX_UNLOCK_V(thpool_p->thcount_lock);

    return NULL;
}


/* Frees a thread  */
static void poolthread_destroy (poolthread* thread_p)
{
    NDRX_FPFREE(thread_p);
}

/* ============================ JOB QUEUE =========================== */


/* Initialize queue */
static int jobqueue_init(jobqueue* jobqueue_p)
{
    jobqueue_p->len = 0;
    jobqueue_p->front = NULL;
    jobqueue_p->rear  = NULL;

    jobqueue_p->has_jobs = (struct bsem*)NDRX_FPMALLOC(sizeof(struct bsem), 0);
    
    if (jobqueue_p->has_jobs == NULL)
    {
        return -1;
    }

    bsem_init(jobqueue_p->has_jobs, 0);

    return 0;
}


/* 
 * Clear the queue
 * The caller must ensure that all worker threads have exit.
 * Called by dispatch thread
 */
static void jobqueue_clear(jobqueue* jobqueue_p)
{
    while(jobqueue_p->len)
    {
        NDRX_FPFREE(jobqueue_pull(jobqueue_p));
    }

    jobqueue_p->front = NULL;
    jobqueue_p->rear  = NULL;
    bsem_reset(jobqueue_p->has_jobs);
    jobqueue_p->len = 0;

}

/* Add (allocated) job to queue
 *! must be protected by outside thcount_lock
 * This is called by dispatcher thread
 */
static void jobqueue_push(jobqueue* jobqueue_p, struct job* newjob)
{
    newjob->prev = NULL;

    switch(jobqueue_p->len)
    {

        case 0:  /* if no jobs in queue */
            jobqueue_p->front = newjob;
            jobqueue_p->rear  = newjob;
            break;

        default: /* if jobs in queue */
            jobqueue_p->rear->prev = newjob;
            jobqueue_p->rear = newjob;

    }
    jobqueue_p->len++;

    bsem_post(jobqueue_p->has_jobs);
}


/* Get first job from queue(removes it from queue)
 *
 * Notice: Caller MUST hold a mutex
 * ! must be protected by outside thcount_lock
 * This is done by worker
 */
static struct job* jobqueue_pull(jobqueue* jobqueue_p)
{
    job* job_p = jobqueue_p->front;

    switch(jobqueue_p->len)
    {
        case 0:  /* if no jobs in queue */
            break;

        case 1:  /* if one job in queue */
            jobqueue_p->front = NULL;
            jobqueue_p->rear  = NULL;
            jobqueue_p->len = 0;
            break;

        default: /* if >1 jobs in queue */
            jobqueue_p->front = job_p->prev;
            jobqueue_p->len--;
            /* more than one job in queue -> post it */
            bsem_post(jobqueue_p->has_jobs);

    }
    return job_p;
}


/* Free all queue resources back to the system 
 * Called by dispatch thread
 */
static void jobqueue_destroy(jobqueue* jobqueue_p)
{
    jobqueue_clear(jobqueue_p);
    NDRX_FPFREE(jobqueue_p->has_jobs);
}


/* ======================== SYNCHRONISATION ========================= */


/* Init semaphore to 1 or 0 */
static void bsem_init(bsem *bsem_p, int value)
{
    if (value < 0 || value > 1)
    {
        err("bsem_init(): Binary semaphore can take only values 1 or 0");
        exit(1);
    }
    pthread_mutex_init(&(bsem_p->mutex), NULL);
    pthread_cond_init(&(bsem_p->cond), NULL);
    bsem_p->v = value;
}


/* Reset semaphore to 0 */
static void bsem_reset(bsem *bsem_p)
{
    bsem_init(bsem_p, 0);
}


/* Post to at least one thread */
static void bsem_post(bsem *bsem_p)
{
    MUTEX_LOCK_V(bsem_p->mutex);
    bsem_p->v = 1;
    pthread_cond_signal(&bsem_p->cond);
    MUTEX_UNLOCK_V(bsem_p->mutex);
}


/* Post to all threads */
static void bsem_post_all(bsem *bsem_p)
{
    MUTEX_LOCK_V(bsem_p->mutex);
    bsem_p->v = 1;
    pthread_cond_broadcast(&bsem_p->cond);
    MUTEX_UNLOCK_V(bsem_p->mutex);
}


/* Wait on semaphore until semaphore has value 0 */
static void bsem_wait(bsem* bsem_p)
{
    MUTEX_LOCK_V(bsem_p->mutex);
    while (bsem_p->v != 1) {
            pthread_cond_wait(&bsem_p->cond, &bsem_p->mutex);
    }
    bsem_p->v = 0;
    MUTEX_UNLOCK_V(bsem_p->mutex);
}
