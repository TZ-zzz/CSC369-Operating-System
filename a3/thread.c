#include <assert.h>
#include <stdlib.h>
#include <ucontext.h>
#include "thread.h"
#include "interrupt.h"

/* This is the ready queue structure and corresponding funcitons */
typedef struct queue_node{
    struct queue_node * next;
    Tid tid;

} queue_node_t;

/* This is the wait queue structure */
typedef struct wait_queue {
    /* ... Fill this in Assignment 3 ... */
    queue_node_t * root;
    queue_node_t * tail;
}queue_t;

queue_t* queue_initialize(){
    int enabled = interrupts_set(0);
    queue_t * queue = malloc(sizeof(queue_t));
    queue->root = NULL;
    queue->tail = NULL;
    interrupts_set(enabled);
    return queue;
}

void queue_enq(queue_t * q, Tid tid){
    int enabled = interrupts_set(0);
    queue_node_t * node = malloc(sizeof(queue_node_t));
    node->tid = tid;
    node->next = NULL;

    if (q->root == NULL){
        q->root = q->tail = node;
    }
    else{
        q->tail->next = node;
        q->tail = node;
    }
    interrupts_set(enabled);
}

Tid queue_deq(queue_t * q){
    int enabled = interrupts_set(0);
    if (q->root == NULL){
        interrupts_set(enabled);
        return THREAD_NONE;
    }
    else{
        queue_node_t * node = q->root;
        Tid tid = node->tid;
        q->root = q->root->next;
        free(node);

        if (q->root == NULL){
            q->tail = NULL;
        }
        interrupts_set(enabled);
        return tid;
    }
}

Tid queue_exact(queue_t * q, Tid tid){
    int enabled = interrupts_set(0);
    if (q->root == NULL){
        interrupts_set(enabled);
        return THREAD_INVALID;
    }
    else{
        queue_node_t * node = q->root;

        if (node->tid == tid){
            q->root = node->next;
            free(node);

            if (q->root == NULL){
                q->tail = NULL;
            }
            interrupts_set(enabled);
            return tid;
        }
        queue_node_t * pre_node = q->root;
        node = q->root->next;

        while (node != NULL && node->tid != tid){
            pre_node = node;
            node = node->next;
        }

        if (node == NULL){
            interrupts_set(enabled);
            return THREAD_INVALID;
        }
        else{
            pre_node->next = node->next;
            free(node);

            if (pre_node->next == NULL){
                q->tail = pre_node;
            }
        }
        interrupts_set(enabled);
        return tid;
    }
}
/* End of ready queue implementation*/


enum state_t{READY, RUNNING, NOTVALID, BLOCK};

/* This is the thread control block */
struct thread {
    /* ... Fill this in ... */
    enum state_t state;
    ucontext_t context;
    int setcontext_called;
    int be_killed;
    int exit_code;
    queue_t * wait_queue;
};

struct thread threads[THREAD_MAX_THREADS];
volatile Tid current_tid;
volatile unsigned num_thr;
queue_t * ready_queue;
queue_t * exit_queue;
Tid recent_tid;

void
thread_init(void)
{
    /* Add necessary initialization for your threads library here. */
	/* Initialize the thread control block for the first thread */\
    recent_tid = 0;
    current_tid = 0;
    threads[0].state = RUNNING;
    threads[0].setcontext_called = 0;
    threads[0].be_killed = 0;
    threads[0].wait_queue = wait_queue_create();
    threads[0].exit_code = -1;
    num_thr = 1;
    ready_queue = queue_initialize();
    exit_queue = queue_initialize();

    /*initial other threads blocks*/
    for (int i = 1; i < THREAD_MAX_THREADS; i++){
        threads[i].state = NOTVALID;
        threads[i].setcontext_called = 0;
    }
    int err = getcontext(&threads[0].context);
	assert(!err);
}

Tid
thread_id()
{
    if (current_tid < 0 || current_tid > THREAD_MAX_THREADS - 1 || threads[current_tid].state == NOTVALID){
        return THREAD_INVALID;
    }
    return current_tid;
}

/* New thread starts by calling thread_stub. The arguments to thread_stub are
 * the thread_main() function, and one argument to the thread_main() function. 
 */
void
thread_stub(void (*thread_main)(void *), void *arg)
{
    interrupts_on();
	thread_main(arg); // call thread_main() function with arg
	thread_exit(0);
}

Tid
thread_create(void (*fn) (void *), void *parg)
{
    int enabled = interrupts_set(0);
    if (num_thr == THREAD_MAX_THREADS){
        interrupts_set(enabled);
        return THREAD_NOMORE;
    }
    Tid tid = -1;
    for (int i = recent_tid + 1; i < THREAD_MAX_THREADS; i++){
        if (threads[i].state == NOTVALID){
            tid = i;
            break;
        }
    }
    if (tid == -1){
        for (int i = 0; i < THREAD_MAX_THREADS; i++){
            if (threads[i].state == NOTVALID){
                tid = i;
                break;
            }
        }
    }
    recent_tid = tid;

    /*initialize the new thread block*/
    void *sp = malloc(THREAD_MIN_STACK);
    if (sp == NULL) {
        interrupts_set(enabled);
        return THREAD_NOMEMORY;
    }
    threads[tid].setcontext_called = 0;
    threads[tid].be_killed = 0;
    threads[tid].state = READY;
    threads[tid].wait_queue = wait_queue_create();
    threads[tid].exit_code = -1;
	assert(!getcontext(&threads[tid].context));

    threads[tid].context.uc_mcontext.gregs[REG_RIP] = (greg_t) &thread_stub;
    threads[tid].context.uc_mcontext.gregs[REG_RDI] = (greg_t) fn;
    threads[tid].context.uc_mcontext.gregs[REG_RSI] = (greg_t) parg;

    threads[tid].context.uc_stack.ss_sp = sp;
    /* rsp point to the start of stack frame*/
    threads[tid].context.uc_mcontext.gregs[REG_RSP] = (greg_t)sp + THREAD_MIN_STACK;
    threads[tid].context.uc_mcontext.gregs[REG_RSP] -= threads[tid].context.uc_mcontext.gregs[REG_RSP] % 16; // aligned to 16 bytes
    // leave space for old rip
    threads[tid].context.uc_mcontext.gregs[REG_RSP] -= 8;

    queue_enq(ready_queue, tid);
    num_thr += 1;
    interrupts_set(enabled);
    return tid;
}

void free_exit_threads(queue_t * q){
    int enabled = interrupts_set(0);
    while (q->root != NULL){
        Tid tid = queue_deq(q);
        free(threads[tid].context.uc_stack.ss_sp);
        wait_queue_destroy(threads[tid].wait_queue);
        threads[tid].wait_queue = NULL;
    }
    interrupts_set(enabled);
}

Tid
thread_yield(Tid want_tid){
    int enabled = interrupts_set(0);
    Tid tid;

    if (want_tid == THREAD_SELF || want_tid == current_tid){
        interrupts_set(enabled);
        return current_tid;
    }
    else if (want_tid == THREAD_ANY){
        if (ready_queue->root == NULL){
            interrupts_set(enabled);
            return THREAD_NONE;
        }
        tid = queue_deq(ready_queue);


    }
    else if (0 <= want_tid && want_tid < THREAD_MAX_THREADS){
        if (threads[want_tid].state == NOTVALID){
            interrupts_set(enabled);
            return THREAD_INVALID;
        }
        tid = queue_exact(ready_queue, want_tid);
        if (tid < 0){
            interrupts_set(enabled);
            return tid;
        }
    }
    else if (want_tid < 0 || want_tid > THREAD_MAX_THREADS - 1){
        interrupts_set(enabled);
        return THREAD_INVALID;
    }

    int err = getcontext(&threads[current_tid].context);
    assert(!err);
    free_exit_threads(exit_queue);
    if (threads[current_tid].setcontext_called == 1){
        if (threads[current_tid].be_killed == 1){
            thread_exit(THREAD_KILLED);
        }
        threads[current_tid].setcontext_called = 0;
        threads[current_tid].state = RUNNING;
        interrupts_set(enabled);
        return tid;
    }
    threads[current_tid].setcontext_called = 1;
    threads[current_tid].state = READY;
    queue_enq(ready_queue, current_tid);
    current_tid = tid;
    setcontext(&threads[tid].context);
    return THREAD_FAILED;
}

void
thread_exit(int exit_code)
{
    int enabled = interrupts_set(0);
    threads[current_tid].state = NOTVALID;

    threads[current_tid].setcontext_called = 0;
    threads[current_tid].be_killed = 0;
    queue_enq(exit_queue, current_tid);
    queue_exact(ready_queue, current_tid);
    queue_node_t * waiting_node = threads[current_tid].wait_queue->root;
    while (waiting_node != NULL){
        if (threads[waiting_node->tid].be_killed == 0){
            threads[waiting_node->tid].exit_code = exit_code;
            break;
        }
        waiting_node = waiting_node->next;
    }
    thread_wakeup(threads[current_tid].wait_queue, 1);
    num_thr -= 1;
    Tid tid = queue_deq(ready_queue);
    if (tid == THREAD_NONE){
        interrupts_set(enabled);
        exit(exit_code);
    }
    current_tid = tid;
    setcontext(&threads[tid].context);
}

Tid
thread_kill(Tid tid)
{   
    int enabled = interrupts_set(0);
    if (tid == current_tid || tid < 0 || tid > THREAD_MAX_THREADS || (threads[tid].state != READY && threads[tid].state != BLOCK)){
        interrupts_set(enabled);
        return THREAD_INVALID;
    }
    threads[tid].be_killed = 1;
    interrupts_set(enabled);
    return tid;
}

/**************************************************************************
 * Important: The rest of the code should be implemented in Assignment 3. *
 **************************************************************************/

/* make sure to fill the wait_queue structure defined above */
struct wait_queue *
wait_queue_create()
{
    int enabled = interrupts_set(0);
    struct wait_queue *wq;

    wq = malloc(sizeof(struct wait_queue));
    assert(wq);

    wq->root = wq->tail = NULL;
    interrupts_set(enabled);
    return wq;
}

void
wait_queue_destroy(struct wait_queue *wq)
{
    while(wq->root != NULL){
        queue_deq(wq);
    }
    free(wq);
}

Tid
thread_sleep(struct wait_queue *queue)
{
    int enabled = interrupts_set(0);
    if (queue == NULL){
        interrupts_set(enabled);
        return THREAD_INVALID;
    }

    Tid tid = queue_deq(ready_queue);
    if (tid < 0){
        interrupts_set(enabled);
        return THREAD_NONE;
    }

    getcontext(&threads[current_tid].context);
    free_exit_threads(exit_queue);
    if (threads[current_tid].setcontext_called == 1){
        if (threads[current_tid].be_killed == 1){
            thread_exit(THREAD_KILLED);
        }
        threads[current_tid].setcontext_called = 0;
        threads[current_tid].state = RUNNING;
        interrupts_set(enabled);
        return tid;
    }
    threads[current_tid].state = BLOCK;
    queue_enq(queue, current_tid);
    threads[current_tid].setcontext_called = 1;
    current_tid = tid;
    num_thr -= 1;
    setcontext(&threads[tid].context);
    interrupts_set(enabled);
    return tid;
}

/* when the 'all' parameter is 1, wakeup all threads waiting in the queue.
 * returns whether a thread was woken up on not. */
int
thread_wakeup(struct wait_queue *queue, int all)
{
    int enabled = interrupts_set(0);
    if (queue == NULL){
        interrupts_set(enabled);
        return 0;
    }
    if (all == 0){
        Tid tid = queue_deq(queue);
        if (tid == THREAD_NONE){
            interrupts_set(enabled);
            return 0;
        }
        else{
            while (threads[tid].be_killed == 1){
                tid = queue_deq(queue);
            }
            threads[tid].state = READY;
            num_thr += 1;
            queue_enq(ready_queue, tid);
            interrupts_set(enabled);
            return 1;
        }
    }
    else if (all == 1){
        int count = 0;
        while (thread_wakeup(queue, 0) != 0){
            count += 1;
        }
        interrupts_set(enabled);
        return count;
    }
    interrupts_set(enabled);
    return 0;
}

/* suspend current thread until Thread tid exits */
Tid
thread_wait(Tid tid, int *exit_code)
{
    int enabled = interrupts_set(0);
    if (tid < 0 || tid > THREAD_MAX_THREADS || tid == current_tid || threads[tid].state == NOTVALID || threads[tid].be_killed == 1){
        interrupts_set(enabled);
		return THREAD_INVALID;
	}
    thread_sleep(threads[tid].wait_queue);
	if (threads[current_tid].exit_code != -1){
        if (exit_code != NULL){
            * exit_code = threads[current_tid].exit_code;
        }
        threads[current_tid].exit_code = -1;
        interrupts_set(enabled);
        return tid;
    }
    interrupts_set(enabled);
	return THREAD_INVALID;
}

struct lock {
    /* ... Fill this in ... */
    Tid hold_pid;
    queue_t * wq;
};

struct lock *
lock_create()
{
    int enabled = interrupts_set(0);
    struct lock *lock;

    lock = malloc(sizeof(struct lock));
    assert(lock);

    lock->hold_pid = THREAD_INVALID;
    lock->wq = wait_queue_create();
    assert(lock->wq);
    interrupts_set(enabled);
    return lock;
}

void
lock_destroy(struct lock *lock)
{
    int enabled = interrupts_set(0);
    assert(lock != NULL);
    if (lock->hold_pid == THREAD_INVALID && lock->wq->root == NULL){
        wait_queue_destroy(lock->wq);
        free(lock);
    }
    interrupts_set(enabled);
}

void
lock_acquire(struct lock *lock)
{
    int enabled = interrupts_set(0);
    assert(lock != NULL);
    while (lock->hold_pid != THREAD_INVALID){
        thread_sleep(lock->wq);
    }
    lock->hold_pid = current_tid;
    interrupts_set(enabled);
}

void
lock_release(struct lock *lock)
{
    int enabled = interrupts_set(0);
    assert(lock != NULL);
    if (current_tid == lock->hold_pid){
        lock->hold_pid = THREAD_INVALID;
	    thread_wakeup(lock->wq, 1);
    }
    interrupts_set(enabled);
}

struct cv {
    /* ... Fill this in ... */
    queue_t * wq;
};

struct cv *
cv_create()
{
    int enabled = interrupts_set(0);
    struct cv *cv;

    cv = malloc(sizeof(struct cv));
    assert(cv);

    cv->wq = wait_queue_create();
    interrupts_set(enabled);
    return cv;
}

void
cv_destroy(struct cv *cv)
{
    int enabled = interrupts_set(0);
    assert(cv != NULL);

    if (cv->wq->root == NULL){
        wait_queue_destroy(cv->wq);
    }
    free(cv);
    interrupts_set(enabled);
}

void
cv_wait(struct cv *cv, struct lock *lock)
{
    int enabled = interrupts_set(0);
    assert(cv != NULL);
    assert(lock != NULL);
    if (lock->hold_pid == current_tid){
        lock_release(lock);
        thread_sleep(cv->wq);
        interrupts_set(enabled);
		lock_acquire(lock);
    }
    interrupts_set(enabled);
}

void
cv_signal(struct cv *cv, struct lock *lock)
{
    int enabled = interrupts_set(0);
    assert(cv != NULL);
    assert(lock != NULL);

    if (lock->hold_pid == current_tid){
        thread_wakeup(cv->wq, 0);
    }
    interrupts_set(enabled);
}

void
cv_broadcast(struct cv *cv, struct lock *lock)
{
    int enabled = interrupts_set(0);
    assert(cv != NULL);
    assert(lock != NULL);

    if (lock->hold_pid == current_tid){
        thread_wakeup(cv->wq, 1);
    }
    interrupts_set(enabled);
}
