#include <assert.h>
#include <stdlib.h>
#include <ucontext.h>
#include "thread.h"
#include "interrupt.h"


/* This is the thread control block */

typedef int State;
typedef int Flag;
#define NEW_STATE 1
#define RECOVER_STATE 2


struct thread {
    ucontext_t thread_context;
    Tid tid;
    State state;
    struct thread *next;
	void *sp;
	Flag flag;
};

struct thread *READY_HEAD = NULL;
struct thread *RUNNING_THREAD = NULL;
struct thread *TO_DELETE_THREAD = NULL;
int thread_id_array[THREAD_MAX_THREADS];

Tid assign_thread_id(){
	int i = 0;
	for(; i < THREAD_MAX_THREADS; i++){
		if(!thread_id_array[i])   //return when encouter 0
			return i;
	}
	if(!thread_id_array[THREAD_MAX_THREADS-1])
		return THREAD_MAX_THREADS-1;
	return THREAD_NOMORE;
}

void thread_stub(void (*thread_main)(void *), void *arg){
	Tid ret;
	thread_main(arg); // call thread_main() function with arg
	ret = thread_exit(THREAD_SELF);
	assert(ret == THREAD_NONE);
	exit(0);
}

void thread_init(void){
    struct thread *new_thread = (struct thread *)malloc(sizeof(struct thread));
    getcontext(&(new_thread->thread_context));    //get the context of the initial thread
    new_thread->tid = 0;
    new_thread->state = NEW_STATE;
	new_thread->next = NULL;
	new_thread->sp = NULL;
	new_thread->flag = 0;
    int i = 0;
    for(; i < THREAD_MAX_THREADS; i++){
        thread_id_array[i] = 0;
    }
    thread_id_array[0] = 1;
    RUNNING_THREAD = new_thread;
}

Tid thread_id(){
    if(RUNNING_THREAD != NULL)
        return RUNNING_THREAD->tid;
    return THREAD_INVALID;
}

void insert_thread_to_ready(struct thread* new_thread){
	if(READY_HEAD ==  NULL)
		READY_HEAD = new_thread;
	else{
		struct thread *temp = READY_HEAD;
		while(temp->next != NULL)
			temp = temp->next;
		temp->next = new_thread;
	}
}

void insert_thread_to_delete(struct thread* new_thread){
	if(TO_DELETE_THREAD ==  NULL)
		TO_DELETE_THREAD = new_thread;
	else{
		struct thread *temp = TO_DELETE_THREAD;
		while(temp->next != NULL)
			temp = temp->next;
		temp->next = new_thread;
	}
}

void delete_threads_list(){
	struct thread *temp = TO_DELETE_THREAD;
	while(temp){
		struct thread *to_delete = temp;
		temp = temp->next;
		if(to_delete->tid != 0){
			free(to_delete->sp);
			to_delete->sp = NULL;
		}
		free(to_delete);
		to_delete = NULL;
	}
	TO_DELETE_THREAD = NULL;
}


Tid thread_create(void (*fn) (void *), void *parg){
	Tid new_id = assign_thread_id();
	if(new_id < 0)
		return THREAD_NOMORE;
	struct thread *new_thread = (struct thread *)malloc(sizeof(struct thread));
	if(new_thread == NULL)
		return THREAD_NOMEMORY;
	getcontext(&(new_thread->thread_context));
	new_thread->tid = new_id;
	thread_id_array[new_id] = 1;
	new_thread->state = NEW_STATE;
	new_thread->next = NULL;
	new_thread->flag = 0;
	void *sp = malloc(THREAD_MIN_STACK);
	if(sp == NULL)
		return THREAD_NOMEMORY;
	new_thread->sp = sp;
	new_thread->thread_context.uc_mcontext.gregs[15] = (unsigned long)(sp + THREAD_MIN_STACK);
	new_thread->thread_context.uc_mcontext.gregs[REG_RBP] = (unsigned long)sp;
	new_thread->thread_context.uc_mcontext.gregs[REG_RDI] = (unsigned long)fn;
	new_thread->thread_context.uc_mcontext.gregs[REG_RSI] = (unsigned long)parg;
	new_thread->thread_context.uc_mcontext.gregs[REG_RIP] = (unsigned long)thread_stub;
	insert_thread_to_ready(new_thread);
	return new_thread->tid;
}

Tid
thread_yield(Tid want_tid)
{
    if(want_tid == THREAD_ANY){
		if(READY_HEAD == NULL)
			return THREAD_NONE;
        struct thread *temp = READY_HEAD;
		READY_HEAD = READY_HEAD->next;
		temp->next = NULL;
		Tid returned_tid = temp->tid;
		insert_thread_to_ready(RUNNING_THREAD);
		RUNNING_THREAD->state = NEW_STATE;
        getcontext(&(RUNNING_THREAD->thread_context));
		if(RUNNING_THREAD->flag)
			thread_exit(THREAD_SELF);
		if(TO_DELETE_THREAD)
			delete_threads_list();
		if(!thread_id_array[returned_tid])
				return returned_tid;
        if(temp->state == NEW_STATE && RUNNING_THREAD->state != RECOVER_STATE){
            temp->state = RECOVER_STATE;
			RUNNING_THREAD = temp;
            setcontext(&(temp->thread_context));
        }
        return returned_tid;
    } else if(want_tid == THREAD_SELF || want_tid == RUNNING_THREAD->tid){
        RUNNING_THREAD->state = NEW_STATE;
        getcontext(&(RUNNING_THREAD->thread_context));
        if(RUNNING_THREAD->state == NEW_STATE){
            RUNNING_THREAD->state = RECOVER_STATE;
            setcontext(&(RUNNING_THREAD->thread_context));
        }
        return RUNNING_THREAD->tid;
    } else if(want_tid > THREAD_MAX_THREADS-1 || want_tid < THREAD_FAILED || !thread_id_array[want_tid])
		return THREAD_INVALID;
	else{
		struct thread *temp = READY_HEAD;
		struct thread *prev = READY_HEAD;
		while(temp->tid != want_tid){
			prev = temp;
			temp = temp->next;
		}
		if(READY_HEAD->tid == want_tid)
			READY_HEAD = READY_HEAD->next;
		else
			prev->next = temp->next;
		temp->next = NULL;
		Tid returned_tid = temp->tid;
		insert_thread_to_ready(RUNNING_THREAD);
		RUNNING_THREAD->state = NEW_STATE;
        getcontext(&(RUNNING_THREAD->thread_context));
		if(RUNNING_THREAD->flag)
			thread_exit(THREAD_SELF);
		if(TO_DELETE_THREAD)
			delete_threads_list();
		if(!thread_id_array[returned_tid])
			return returned_tid;
        if(temp->state == NEW_STATE && RUNNING_THREAD->state != RECOVER_STATE){
            temp->state = RECOVER_STATE;
			RUNNING_THREAD = temp;
            setcontext(&(temp->thread_context));
        }
        return returned_tid;
	}
	return THREAD_FAILED;
}

Tid thread_exit(Tid tid){
	if(tid == THREAD_SELF){
		if(READY_HEAD == NULL)
			return THREAD_NONE;
		struct thread *temp = RUNNING_THREAD;
		struct thread *new_run = READY_HEAD;
		READY_HEAD = READY_HEAD->next;
		temp->next = NULL;
		thread_id_array[temp->tid] = 0;
		insert_thread_to_delete(temp);
		new_run->next = NULL;
		new_run->state = RECOVER_STATE;
		RUNNING_THREAD = new_run;
		setcontext(&(new_run->thread_context));
	} else if(tid == THREAD_ANY){
		if(READY_HEAD == NULL)
			return THREAD_NONE;
		struct thread *temp = READY_HEAD;
		//READY_HEAD = READY_HEAD->next;
		Tid returned_tid = temp->tid;
		//temp->next = NULL;
		//thread_id_array[temp->tid] = 0;
		temp->flag = 1;
		//insert_thread_to_delete(temp);
		return returned_tid;
	} else{
		if(tid > THREAD_MAX_THREADS-1 || tid < THREAD_FAILED ||!thread_id_array[tid])
			return THREAD_INVALID;
		if(READY_HEAD == NULL)
			return THREAD_NONE;
		struct thread *temp = READY_HEAD;
		//struct thread *prev = READY_HEAD;
		while(temp->tid != tid){
			//prev = temp;
			temp = temp->next;
		}
		//if(READY_HEAD->tid == tid)
			//READY_HEAD = READY_HEAD->next;
		//else
			//prev->next = temp->next;
		Tid returned_tid = temp->tid;
		//temp->next = NULL;
		//thread_id_array[temp->tid] = 0;
		//insert_thread_to_delete(temp);
		temp->flag = 1;
		return returned_tid;
	}
	return THREAD_FAILED;
}


/*******************************************************************
 * Important: The rest of the code should be implemented in Lab 3. *
 *******************************************************************/

/* This is the wait queue structure */
struct wait_queue {
	/* ... Fill this in ... */
};

struct wait_queue *
wait_queue_create()
{
	struct wait_queue *wq;

	wq = malloc(sizeof(struct wait_queue));
	assert(wq);

	TBD();

	return wq;
}

void
wait_queue_destroy(struct wait_queue *wq)
{
	TBD();
	free(wq);
}

Tid
thread_sleep(struct wait_queue *queue)
{
	TBD();
	return THREAD_FAILED;
}

/* when the 'all' parameter is 1, wakeup all threads waiting in the queue.
 * returns whether a thread was woken up on not. */
int
thread_wakeup(struct wait_queue *queue, int all)
{
	TBD();
	return 0;
}

struct lock {
	/* ... Fill this in ... */
};

struct lock *
lock_create()
{
	struct lock *lock;

	lock = malloc(sizeof(struct lock));
	assert(lock);

	TBD();

	return lock;
}

void
lock_destroy(struct lock *lock)
{
	assert(lock != NULL);

	TBD();

	free(lock);
}

void
lock_acquire(struct lock *lock)
{
	assert(lock != NULL);

	TBD();
}

void
lock_release(struct lock *lock)
{
	assert(lock != NULL);

	TBD();
}

struct cv {
	/* ... Fill this in ... */
};

struct cv *
cv_create()
{
	struct cv *cv;

	cv = malloc(sizeof(struct cv));
	assert(cv);

	TBD();

	return cv;
}

void
cv_destroy(struct cv *cv)
{
	assert(cv != NULL);

	TBD();

	free(cv);
}

void
cv_wait(struct cv *cv, struct lock *lock)
{
	assert(cv != NULL);
	assert(lock != NULL);

	TBD();
}

void
cv_signal(struct cv *cv, struct lock *lock)
{
	assert(cv != NULL);
	assert(lock != NULL);

	TBD();
}

void
cv_broadcast(struct cv *cv, struct lock *lock)
{
	assert(cv != NULL);
	assert(lock != NULL);

	TBD();
}
