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
#define MLFQ_LEVELS 4 //or 8
#define S 3000 //3 seconds (floor: 0.25 seconds(250), ceiling: 5 seconds)

typedef uint worker_t;

/*enums for thread and mutex states*/
typedef enum {READY, SCHEDULED, BLOCKED, EXITED} state_t;
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
	worker_t t_id; //unique id for worker thread
	state_t t_state; //state: READY, SHCEDULED, BLOCKED, EXITED
	unsigned int t_prio; //priority level relevent to MLFQ scheduler
	void* t_stack; //address of stack pointer for worker thread
    ucontext_t t_ctx; //context of thread
    void *return_value; //return value when exiting from routine
    int elapsed; //total time quantums elapsed relevant to PSJF scheduler
	struct timespec arrival; //arrival time
	struct timespec start; //first scheduled time
	struct timespec completion; //thread completion time (when exited)
	int t_scheduled; //if t_scheduled == 0, thread has not been scheduled yet
} tcb; 

/* define your data structures here: */
// Feel free to add your own auxiliary data structures (linked list or queue etc...)

/*Node structure to store thread tcb's in queue*/
typedef struct Node{
    tcb* data;
    struct Node* next;
} node_t;
/*simple queue structure*/
typedef struct Queue{
    node_t* head;
    node_t* tail;
	int length;
} queue_t;
/*scheduler structure */
typedef struct Scheduler{
	queue_t** p_queues; //Multi-level feedback priority queues for MLFQ scheduler
	queue_t* p_queue; //single queue for PSJF scheduler
    void* sched_stack; //address of scheduler stack pointer
    ucontext_t sched_ctx; //scheduler context
} sched_t;

/* mutex struct definition */
typedef struct worker_mutex_t {
	/* add something here */
	// YOUR CODE HERE
	lock_t _lock; //lock states: FREE, HELD
	tcb* _owner; //pointer to tcb that currently holds the lock
	queue_t* blocked_q; //blocked queue to track all threads waiting to hold lock once it is freed
} worker_mutex_t;



// YOUR CODE HERE

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
