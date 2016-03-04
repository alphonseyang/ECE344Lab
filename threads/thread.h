#ifndef _THREAD_H_
#define _THREAD_H_

typedef int Tid;
#define THREAD_MAX_THREADS 1024
#define THREAD_MIN_STACK 32768

/*
 * Valid thread identifiers (Tid) range between 0 and THREAD_MAX_THREADS-1. The
 * first thread to run must have a thread id of 0. Note that this thread is the
 * main thread, i.e., it is created before the first call to
 * thread_create. Negative Tid values are used for error codes or control codes.
 */

enum { THREAD_ANY = -1,
	THREAD_SELF = -2,
	THREAD_INVALID = -3,
	THREAD_NONE = -4,
	THREAD_NOMORE = -5,
	THREAD_NOMEMORY = -6,
	THREAD_FAILED = -7
};

static inline int
thread_ret_ok(Tid ret)
{
	return (ret >= 0 ? 1 : 0);
}

/*************************************************
 * Lab 2: Implement the following four functions *
 *************************************************/

/* perform any initialization needed by your threading system */
void thread_init(void);

/* return the thread identifier of the currently running thread */
Tid thread_id();

/* create a thread that starts running the function fn(arg). Upon success,
 * return the thread identifier. On failure, return the following:
 *
 * THREAD_NOMORE: no more threads can be created.
 * THREAD_NOMEMORY: no more memory available to create a thread stack. */
Tid thread_create(void (*fn) (void *), void *arg);

/* suspend calling thread and run the thread with identifier tid. The calling
 * thread is put in the ready queue. tid can be identifier of any available
 * thread or the following constants:
 *
 * THREAD_ANY:	   run any thread in the ready queue.
 * THREAD_SELF:    continue executing calling thread, for debugging purposes.
 *
 * Upon success, return the identifier of the thread that ran. The calling
 * thread does not see this result until it runs later. Upon failure, the
 * calling thread continues running, and returns the following:
 *
 * THREAD_INVALID: identifier tid does not correspond to a valid thread.
 * THREAD_NONE:    no more threads, other than the caller, are available to
 *		   run. this can happen is response to a call with tid set to
 *		   THREAD_ANY. */
Tid thread_yield(Tid tid);

/* destroy the thread whose identifier is tid. The calling thread continues to
 * execute and receives the result of the call. tid can be identifier of any
 * available thread or the following constants:
 *
 * THREAD_ANY:     destroy any thread except the caller.
 * THREAD_SELF:    destroy the calling thread and reclaim its resources. in this
 *		   case, the calling thread obviously doesn't run any
 *		   longer. some other ready thread is run.
 *
 * Upon success, return the identifier of the destroyed thread. A new thread
 * should be able to reuse this identifier. Upon failure, the calling thread
 * continues running, and returns the following:
 *
 * THREAD_INVALID: identifier tid does not correspond to a valid thread.
 * THREAD_NONE:	   no more threads, other than the caller, are available to
 *		   destroy, i.e., this is the last thread in the system. This
 *		   can happen in response to a call with tid set to THREAD_ANY
 *		   or THREAD_SELF. */
Tid thread_exit(Tid tid);

/********************************************
 * Lab 3: Implement the following functions *
 ********************************************/

/* create a queue of waiting threads. initially, the queue is empty. */
struct wait_queue *wait_queue_create();

/* destroy the wait queue. be sure to check that the queue is empty when it is
 * being destroyed. */
void wait_queue_destroy(struct wait_queue *wq);

/* suspend calling thread and run some other thread. The calling thread is put
 * in the wait queue. Upon success, return the identifier of the thread that
 * ran. The calling thread does not see this result until it runs later. Upon
 * failure, the calling thread continues running, and returns the following:
 *
 * THREAD_INVALID: queue is invalid, e.g., it is NULL.
 * THREAD_NONE:    no more threads, other than the caller, are available to
 *		   run. */
Tid thread_sleep(struct wait_queue *queue);

/* wake up one or more threads that are suspended in the wait queue. These
 * threads are put in the ready queue. The calling thread continues to execute
 * and receives the result of the call. When "all" is 0, then one thread is
 * woken up.  When "all" is 1, all suspended threads are woken up. Wake up
 * threads in FIFO order, i.e., first thread to sleep must be woken up
 * first. The function returns the number of threads that were woken up. It can
 * return zero if there were no suspended threads in the wait queue. */
int thread_wakeup(struct wait_queue *queue, int all);

/* create a blocking lock. initially, the lock is available. associate a wait
 * queue with the lock so that threads that need to acquire the lock can wait in
 * this queue. */
struct lock *lock_create();
/* destroy the lock. be sure to check that the lock is available when it is
 * being destroyed. */
void lock_destroy(struct lock *lock);

/* acquire the lock. threads should be suspended until they can acquire the
 * lock. */
void lock_acquire(struct lock *lock);
/* release the lock. be sure to check that the lock had been acquired by the
 * calling thread, before it is released. wakeup all threads that are waiting to
 * acquire the lock. */
void lock_release(struct lock *lock);

/* create a condition variable. associate a wait queue with the condition
 * variable so that threads issuing cv_wait can wait in this queue. */
struct cv *cv_create();
/* destroy the condition variable. */
void cv_destroy(struct cv *cv);

/* suspend the calling thread on the condition variable cv. be sure to check
 * that the calling thread had acquired lock when this call is made. you will
 * need to release the lock before waiting, and reacquire it before returning
 * from wait. */
void cv_wait(struct cv *cv, struct lock *lock);

/* wake up one thread that is waiting on the condition variable cv. be sure to
 * check that the calling thread had acquired lock when this call is made. */
void cv_signal(struct cv *cv, struct lock *lock);

/* wake up all threads that are waiting on the condition variable cv. be sure to
 * check that the calling thread had acquired lock when this call is made. */
void cv_broadcast(struct cv *cv, struct lock *lock);

#endif /* _THREAD_H_ */
