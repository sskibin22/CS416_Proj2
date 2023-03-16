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
#define MLFQ_LEVELS 4
#define S 250 //0.25 seconds

typedef uint worker_t;

typedef enum {READY, SCHEDULED, BLOCKED, EXITED, MAIN} state_t;
typedef enum {FREE, HELD} lock_t;

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
	unsigned int t_prio;
	void* t_stack;
    ucontext_t t_ctx;
    void *return_value;
    int elapsed;
	struct timespec arrival;
	struct timespec start;
	struct timespec completion;
	int t_scheduled;

} tcb; 

/* mutex struct definition */
typedef struct worker_mutex_t {
	/* add something here */
	// YOUR CODE HERE
	lock_t _lock;
	tcb* _owner;
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
	int length;
} queue_t;

typedef struct Scheduler{
	queue_t** p_queues;
	queue_t* p_queue;
    void* sched_stack;
    ucontext_t sched_ctx;
} sched_t;



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
