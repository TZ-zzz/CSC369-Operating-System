#include <assert.h>
#include <stdlib.h>
#include <ucontext.h>
#include "thread.h"
#include "interrupt.h"

/* This is the wait queue structure */

/* This is the ready queue structure and corresponding funcitons */
typedef struct queue_node{
    struct queue_node * next;
    Tid tid;

} queue_node_t;

typedef struct wait_queue{
    queue_node_t * root;
    queue_node_t * tail;
}queue_t;

queue_t* queue_initialize(){
    queue_t * queue = malloc(sizeof(queue_t));
    queue->root = NULL;
    queue->tail = NULL;
    return queue;
}

void queue_enq(queue_t * q, Tid tid){
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
}

Tid queue_deq(queue_t * q){
    if (q->root == NULL){
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
        return tid;
    }
}

Tid queue_exact(queue_t * q, Tid tid){
    if (q->root == NULL){
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
            return tid;
        }
        queue_node_t * pre_node = q->root;
        node = q->root->next;

        while (node != NULL && node->tid != tid){
            pre_node = node;
            node = node->next;
        }

        if (node == NULL){
            return THREAD_INVALID;
        }
        else{
            pre_node->next = node->next;
            free(node);

            if (pre_node->next == NULL){
                q->tail = pre_node;
            }
        }
        return tid;
    }
}
/* End of ready queue implementation*/


enum state_t{READY, RUNNING, NOTVALID};

/* This is the thread control block */
struct thread {
    /* ... Fill this in ... */
    enum state_t state;
    ucontext_t context;
    int setcontext_called;
    int be_killed;
};

struct thread threads[THREAD_MAX_THREADS];
volatile Tid current_tid;
volatile unsigned num_thr;
queue_t * ready_queue;
queue_t * exit_queue;

void
thread_init(void)
{
    /* Add necessary initialization for your threads library here. */
	/* Initialize the thread control block for the first thread */
    current_tid = 0;
    threads[0].state = RUNNING;
    threads[0].setcontext_called = 0;
    threads[0].be_killed = 0;
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
	thread_main(arg); // call thread_main() function with arg
	thread_exit(0);
}

Tid
thread_create(void (*fn) (void *), void *parg)
{
    if (num_thr == THREAD_MAX_THREADS){
        return THREAD_NOMORE;
    }
    Tid tid;
    for (int i = 0; i < THREAD_MAX_THREADS; i++){
        if (threads[i].state == NOTVALID){
            tid = i;
            break;
        }
    }

    /*initialize the new thread block*/
    void *sp = malloc(THREAD_MIN_STACK);
    if (sp == NULL) {
        return THREAD_NOMEMORY;
    }
    threads[tid].setcontext_called = 0;
    threads[tid].be_killed = 0;
    threads[tid].state = READY;
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
    return tid;
}

void free_exit_threads(queue_t * q){
    while (q->root != NULL){
        Tid tid = queue_deq(q);
        free(threads[tid].context.uc_stack.ss_sp);
    }
}

Tid
thread_yield(Tid want_tid){

    Tid tid;

    if (want_tid == THREAD_SELF || want_tid == current_tid){
        return current_tid;
    }
    else if (want_tid == THREAD_ANY){
        if (ready_queue->root == NULL){
            return THREAD_NONE;
        }
        tid = queue_deq(ready_queue);


    }
    else if (0 <= want_tid && want_tid < THREAD_MAX_THREADS){
        if (threads[want_tid].state == NOTVALID){
            return THREAD_INVALID;
        }
        tid = queue_exact(ready_queue, want_tid);
        if (tid < 0){
            return tid;
        }
    }
    else if (want_tid < 0 || want_tid > THREAD_MAX_THREADS - 1){
        return THREAD_INVALID;
    }

    int err = getcontext(&threads[current_tid].context);
    assert(!err);
    free_exit_threads(exit_queue);
    if (threads[current_tid].setcontext_called == 1){
        if (threads[current_tid].be_killed == 1){
            thread_exit(0);
        }
        threads[current_tid].setcontext_called = 0;
        threads[current_tid].state = RUNNING;
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
    threads[current_tid].state = NOTVALID;

    threads[current_tid].setcontext_called = 0;
    threads[current_tid].be_killed = 0;
    queue_enq(exit_queue, current_tid);
    queue_exact(ready_queue, current_tid);
    num_thr -= 1;
    Tid tid = queue_deq(ready_queue);
    if (tid == THREAD_NONE){
        exit(exit_code);
    }
    current_tid = tid;
    setcontext(&threads[tid].context);
}

Tid
thread_kill(Tid tid)
{
    if (tid == current_tid || tid < 0 || tid > THREAD_MAX_THREADS || threads[tid].state != READY){
        return THREAD_INVALID;
    }
    threads[tid].be_killed = 1;
    return tid;
}

/**************************************************************************
 * Important: The rest of the code should be implemented in Assignment 3. *
 **************************************************************************/

/* make sure to fill the wait_queue structure defined above */
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

/* suspend current thread until Thread tid exits */
Tid
thread_wait(Tid tid, int *exit_code)
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
