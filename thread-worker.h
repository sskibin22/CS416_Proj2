// File:	worker_t.h

// List all group member's name: Scott Skibin(ss3793), Kevin Schwarz(kjs309)
// username of iLab:
// iLab Server: iLab2

#ifndef WORKER_T_H
#define WORKER_T_H

#define _GNU_SOURCE

/* To use Linux pthread Library in Benchmark, you have to comment the USE_WORKERS macro */
#define USE_WORKERS 1

/* include lib header files that you need here: */
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>

#define STACK_SIZE SIGSTKSZ
#define QUANTUM 10000 //10 milliseconds
#define MAX_QUANTS 5

typedef uint worker_t;

typedef enum {READY, SCHEDULED, BLOCKED} state_t;

typedef struct TCB {
	/* add important states in a thread control block */
	// thread Id
	// thread status
	// thread context
	// thread stack
	// thread priority
	// And more ...

	// YOUR CODE HERE
	worker_t t_id;
	state_t t_state;
    ucontext_t t_ctx;
	void* t_stack;
	unsigned int t_prio;
    suseconds_t elapsed; //long
} tcb; 

/* mutex struct definition */
typedef struct worker_mutex_t {
	/* add something here */
	int _lock;
	unsigned int _count;
	worker_t _owner;
	
	// YOUR CODE HERE
} worker_mutex_t;

/* define your data structures here: */
// Feel free to add your own auxiliary data structures (linked list or queue etc...)

// YOUR CODE HERE
typedef struct Node{
    tcb* data;
    struct Node* next;
} node_t;

typedef struct Queue{
    node_t* head;
    node_t* tail;
} queue_t;

typedef struct PSJF{
    queue_t* p_queue;
    struct sigaction sa;
    struct itimerval timer;
    void* sched_stack;
    ucontext_t sched_ctx;
} psjf_t;


/* Function Declarations: */

/* create a new thread */
int worker_create(worker_t * thread, pthread_attr_t * attr, void
    *(*function)(void*), void * arg);

/* give CPU pocession to other user level worker threads voluntarily */
int worker_yield();

/* terminate a thread */
void worker_exit(void *value_ptr);

/* wait for thread termination */
int worker_join(worker_t thread, void **value_ptr);

/* initial the mutex lock */
int worker_mutex_init(worker_mutex_t *mutex, const pthread_mutexattr_t
    *mutexattr);

/* aquire the mutex lock */
int worker_mutex_lock(worker_mutex_t *mutex);

/* release the mutex lock */
int worker_mutex_unlock(worker_mutex_t *mutex);

/* destroy the mutex */
int worker_mutex_destroy(worker_mutex_t *mutex);

static void sched_psjf();
static void sched_mlfq();

queue_t* queue_init();
int is_empty(queue_t* q);
node_t* node_init();
void enqueue(tcb* t, queue_t* q);
tcb* dequeue(queue_t* q);
tcb* remove_at(int id, queue_t* q);
int exists(int id, queue_t* q);
void queue_destroy(queue_t* q);
void queue_display(queue_t* q);
psjf_t* psjf_init();
void psjf_destroy(psjf_t* s);
void timer_handler(int signum, siginfo_t* siginfo, void* sig);


/* Function to print global statistics. Do not modify this function.*/
void print_app_stats(void);

#ifdef USE_WORKERS
#define pthread_t worker_t
#define pthread_mutex_t worker_mutex_t
#define pthread_create worker_create
#define pthread_exit worker_exit
#define pthread_join worker_join
#define pthread_mutex_init worker_mutex_init
#define pthread_mutex_lock worker_mutex_lock
#define pthread_mutex_unlock worker_mutex_unlock
#define pthread_mutex_destroy worker_mutex_destroy
#endif

#endif
