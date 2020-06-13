/**********************************
 * @author      Johan Hanssen Seferidis
 * License:     MIT
 *
 **********************************/

#ifndef _THPOOL_
#define _THPOOL_

#ifdef __cplusplus
extern "C" {
#endif

/* =================================== API ======================================= */


typedef struct thpool_* threadpool;

/** thread pool init call */
typedef  int (*ndrx_thpool_tpsvrthrinit_t)(int argc, char **argv);

/** thread done call if any */
typedef  void (*ndrx_thpool_tpsvrthrdone_t)(void);

/**
 * @brief  Initialize threadpool
 *
 * Initializes a threadpool. This function will not return untill all
 * threads have initialized successfully.
 *
 * @example
 *
 *    ..
 *    threadpool thpool;                     //First we declare a threadpool
 *    thpool = thpool_init(4);               //then we initialize it to 4 threads
 *    ..
 *
 * @param num_threads   number of threads to be created in the threadpool
 * @param p_ret return of threads
 * @param pf_init thread init function
 * @param pf_done thread done function
 * @param argc command line argument count
 * @param argv cli arguments for init func
 * @return threadpool    created threadpool on success,
 *                       NULL on error
 */
threadpool ndrx_thpool_init(int num_threads, int *p_ret, 
        ndrx_thpool_tpsvrthrinit_t pf_init, ndrx_thpool_tpsvrthrdone_t pf_done,
        int argc, char **argv);


/**
 * @brief Add work to the job queue
 *
 * Takes an action and its argument and adds it to the threadpool's job queue.
 * If you want to add to work a function with more than one arguments then
 * a way to implement this is by passing a pointer to a structure.
 *
 * NOTICE: You have to cast both the function and argument to not get warnings.
 *
 * @example
 *
 *    void print_num(int num){
 *       printf("%d\n", num);
 *    }
 *
 *    int main() {
 *       ..
 *       int a = 10;
 *       thpool_add_work(thpool, (void*)print_num, (void*)a);
 *       ..
 *    }
 *
 * @param  threadpool    threadpool to which the work will be added
 * @param  function_p    pointer to function to add as work
 * @param  arg_p         pointer to an argument
 * @return 0 on successs, -1 otherwise.
 */
int ndrx_thpool_add_work(threadpool, void (*function_p)(void*, int *), void* arg_p);


/**
 * @brief Wait for all queued jobs to finish
 *
 * Will wait for all jobs - both queued and currently running to finish.
 * Once the queue is empty and all work has completed, the calling thread
 * (probably the main program) will continue.
 *
 * Smart polling is used in wait. The polling is initially 0 - meaning that
 * there is virtually no polling at all. If after 1 seconds the threads
 * haven't finished, the polling interval starts growing exponentially
 * untill it reaches max_secs seconds. Then it jumps down to a maximum polling
 * interval assuming that heavy processing is being used in the threadpool.
 *
 * @example
 *
 *    ..
 *    threadpool thpool = thpool_init(4);
 *    ..
 *    // Add a bunch of work
 *    ..
 *    thpool_wait(thpool);
 *    puts("All added work has finished");
 *    ..
 *
 * @param threadpool     the threadpool to wait for
 * @return nothing
 */
void ndrx_thpool_wait(threadpool);

/**
 * Wait for one thread to become free
 * @param thpool_p
 */
void ndrx_thpool_wait_one(threadpool);

/**
 * Wait for at least one thread to become free
 * @param thpool_p
 * @return EXTRUE(has free), EXFALSE(no free)
 */
int ndrx_thpool_is_one_avail(threadpool);

/**
 * @brief Destroy the threadpool
 *
 * This will wait for the currently active threads to finish and then 'kill'
 * the whole threadpool to free up memory.
 *
 * @example
 * int main() {
 *    threadpool thpool1 = thpool_init(2);
 *    threadpool thpool2 = thpool_init(2);
 *    ..
 *    thpool_destroy(thpool1);
 *    ..
 *    return 0;
 * }
 *
 * @param threadpool     the threadpool to destroy
 * @return nothing
 */
void ndrx_thpool_destroy(threadpool);


#ifdef __cplusplus
}
#endif

#endif
