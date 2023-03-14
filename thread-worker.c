// File:	thread-worker.c

// List all group member's name: Scott Skibin(ss3793), Kevin Schwarz(kjs309)
// username of iLab:
// iLab Server: iLab2

#include "thread-worker.h"

//Global counter for total context switches and 
//average turn around and response time
long tot_cntx_switches=0;
double avg_turn_time=0;
double avg_resp_time=0;


// INITAILIZE ALL YOUR OTHER VARIABLES HERE
// YOUR CODE HERE
/*Global Variables*/
int lib_active = 0;
int id_count = 1;
psjf_t* psjf = NULL;
// queue_t* exit_q;
tcb* scheduled = NULL;
int total_quants = 0;
ucontext_t main_ctx;
int thread_count = 0;
int lib_complete = 0;


/* create a new thread */
int worker_create(worker_t * thread, pthread_attr_t * attr, 
                      void *(*function)(void*), void * arg) {

       // - create Thread Control Block (TCB)
       // - create and initialize the context of this worker thread
       // - allocate space of stack for this thread to run
       // after everything is set, push this thread into run queue and 
       // - make it ready for the execution.

       // YOUR CODE HERE
	int ret_val = 0;
	thread_count++;
	if(!lib_active){
		psjf = psjf_init();
		// exit_q = queue_init();
		lib_active = 1;
	}

	tcb* _tcb = (tcb*)malloc(sizeof(tcb));
	if (_tcb == NULL){
		perror("Failed to allocate _tcb");
		exit(1);
	}
	_tcb->t_id = id_count++;
	*thread = _tcb->t_id;
	_tcb->t_state = READY;
	_tcb->t_prio = 0;
	_tcb->elapsed = 0;
	if(getcontext(&_tcb->t_ctx) < 0){
		perror("getcontext");
		return -1;
	}
	_tcb->t_stack = (void*)malloc(STACK_SIZE);
	if (_tcb->t_stack == NULL){
		perror("Failed to allocate _tcb->stack");
		exit(1);
	}
	_tcb->t_ctx.uc_stack.ss_sp = _tcb->t_stack;
	_tcb->t_ctx.uc_stack.ss_size = STACK_SIZE;
	_tcb->t_ctx.uc_link = &psjf->sched_ctx;
	_tcb->t_ctx.uc_flags = 0;
	// memset(_tcb->t_ctx.uc_stack.ss_sp, 0, STACK_SIZE);

	if(arg == NULL){
		makecontext(&_tcb->t_ctx, function, 0);
	}
	else{
		makecontext(&_tcb->t_ctx, function, 1, arg);
	}    
	
	enqueue(_tcb, psjf->p_queue);
	
	swapcontext(&main_ctx, &psjf->sched_ctx);

	return ret_val;
};

/* give CPU possession to other user-level worker threads voluntarily */
int worker_yield() {
	
	// - change worker thread's state from Running to Ready
	// - save context of this thread to its thread control block
	// - switch from thread context to scheduler context

	// YOUR CODE HERE
	if(swapcontext(&scheduled->t_ctx, &psjf->sched_ctx) < 0){
        return -1;
    }
    return 0;
};

/* terminate a thread */
void worker_exit(void *value_ptr) {
	// - de-allocate any dynamic memory created when starting this thread

	// YOUR CODE HERE
	if(value_ptr == NULL){
        free(scheduled->t_stack);
        free(scheduled);
        scheduled = NULL;
        thread_count--;
    }
};


/* Wait for thread termination */
int worker_join(worker_t thread, void **value_ptr) {
	
	// - wait for a specific thread to terminate
	// - de-allocate any dynamic memory created by the joining thread
  
	// YOUR CODE HERE
	if(value_ptr == NULL){
        while(exists(thread, psjf->p_queue) || (scheduled != NULL && thread == scheduled->t_id));
        if(thread_count == 0 && lib_complete == 0){
            psjf_destroy(psjf);
            lib_complete = 1;
        }
    }
	return 0;
};

/* initialize the mutex lock */
int worker_mutex_init(worker_mutex_t *mutex, 
                          const pthread_mutexattr_t *mutexattr) {
	//- initialize data structures for this mutex

	// YOUR CODE HERE
	return 0;
};

/* aquire the mutex lock */
int worker_mutex_lock(worker_mutex_t *mutex) {

        // - use the built-in test-and-set atomic function to test the mutex
        // - if the mutex is acquired successfully, enter the critical section
        // - if acquiring mutex fails, push current thread into block list and
        // context switch to the scheduler thread

        // YOUR CODE HERE
        return 0;
};

/* release the mutex lock */
int worker_mutex_unlock(worker_mutex_t *mutex) {
	// - release mutex and make it available again. 
	// - put threads in block list to run queue 
	// so that they could compete for mutex later.

	// YOUR CODE HERE
	return 0;
};


/* destroy the mutex */
int worker_mutex_destroy(worker_mutex_t *mutex) {
	// - de-allocate dynamic memory created in worker_mutex_init

	return 0;
};

/* scheduler */
static void schedule() {
	// - every time a timer interrupt occurs, your worker thread library 
	// should be contexted switched from a thread context to this 
	// schedule() function

	// - invoke scheduling algorithms according to the policy (PSJF or MLFQ)

	// if (sched == PSJF)
	//		sched_psjf();
	// else if (sched == MLFQ)
	// 		sched_mlfq();

	// YOUR CODE HERE
	if (sigaction(SIGPROF, &psjf->sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    if (setitimer(ITIMER_PROF, &psjf->timer, NULL) == -1) {
        perror("setitimer");
        exit(EXIT_FAILURE);
    }

    if(getcontext(&psjf->sched_ctx) < 0){
        perror("getcontext");
        exit(EXIT_FAILURE);
    }

// - schedule policy
#ifndef MLFQ
	// Choose PSJF
	sched_psjf();
#else 
	// Choose MLFQ
	sched_mlfq();
#endif

}

/* Pre-emptive Shortest Job First (POLICY_PSJF) scheduling algorithm */
static void sched_psjf() {
	// - your own implementation of PSJF
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
	if(!is_empty(psjf->p_queue)){
        tcb* s_thread = NULL;
        //if there is only one node in the queue...
        if(psjf->p_queue->head == psjf->p_queue->tail){
            s_thread = dequeue(psjf->p_queue);
        }
        else{
            //linear search for thread with minimum elapsed time
            node_t* temp = psjf->p_queue->head;
            tcb* min_elapsed = temp->data;
            temp = temp->next;
            while(temp != NULL){
                if(temp->data->elapsed < min_elapsed->elapsed){
                    min_elapsed = temp->data;
                }
                temp = temp->next;
            }
            //dequeue thread found in min search from queue
            s_thread = remove_at(min_elapsed->t_id, psjf->p_queue);
        }
        //min elapsed thread is found and ready
        if(s_thread != NULL){
            if(scheduled != NULL){
                scheduled->t_state = READY;
                enqueue(scheduled, psjf->p_queue);
            }
            s_thread->t_state = SCHEDULED;
            scheduled = s_thread;

            setcontext(&scheduled->t_ctx);    
        }

    }
    /*TODO: if psjf queue is empty...*/
    if(scheduled != NULL){
        setcontext(&scheduled->t_ctx);
    }

    setcontext(&main_ctx);
}


/* Preemptive MLFQ scheduling algorithm */
static void sched_mlfq() {
	// - your own implementation of MLFQ
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
}

//DO NOT MODIFY THIS FUNCTION
/* Function to print global statistics. Do not modify this function.*/
void print_app_stats(void) {

       fprintf(stderr, "Total context switches %ld \n", tot_cntx_switches);
       fprintf(stderr, "Average turnaround time %lf \n", avg_turn_time);
       fprintf(stderr, "Average response time  %lf \n", avg_resp_time);
}


// Feel free to add any other functions you need

// YOUR CODE HERE

/*queue functions*/
queue_t* queue_init(){
    queue_t* q = malloc(sizeof(queue_t));
    q->head = NULL;
    q->tail = NULL;
    return q;
}

int is_empty(queue_t* q) {
    return q->head == NULL;
}

node_t* node_init(tcb* t){
    node_t* n = malloc(sizeof(node_t));
    n->data = t;
    n->next = NULL;
    return n;
}

void enqueue(tcb* t, queue_t* q){
    node_t* n = node_init(t);
    if(is_empty(q)){
        q->head = q->tail = n;
    }
    else{
        q->tail->next = n;
        q->tail = n;
    }
}

tcb* dequeue(queue_t* q){
    if(!is_empty(q)){
        node_t* temp = q->head;
        if(q->head == q->tail){
            q->head = q->tail = NULL;
        }
        else{
            q->head = q->head->next;
        }
        tcb* t_temp = temp->data;
        free(temp);
        return t_temp;
    }
    return NULL;
}

tcb* remove_at(int id, queue_t* q){
    tcb* r = NULL;
    node_t* prev = NULL;
    node_t* curr = q->head;
    //first node is min elapsed
    if(curr->data->t_id == id){
        r = dequeue(psjf->p_queue);
        return r;
    }
    else{
        prev = curr;
        curr = curr->next;
        while(curr != NULL){
            if(curr->data->t_id == id){
                r = curr->data;
                prev->next = curr->next;
                free(curr);
                return r;
            }
            prev = curr;
            curr = curr->next;
        }
    }
    return r;
}

int exists(int id, queue_t* q){
    node_t* n = q->head;
    while(n != NULL){
        if(n->data->t_id == id){
            return 1;
        }
        n = n->next;
    }
    return 0;
}

void queue_destroy(queue_t* q){
    node_t* temp;
    while(!is_empty(q)){
        temp = q->head;
        free(temp->data->t_stack);
        free(temp->data);
        if(q->head == q->tail){
            q->head = NULL;
        }
        else{
            q->head = q->head->next;
        }
        free(temp);
    }
    free(q);
}
void queue_display(queue_t* q){
    printf("Queue: ");
    node_t* temp = q->head;
    while(temp != NULL){
        printf("id: %d | ", temp->data->t_id);
        temp = temp->next;
    }
    printf("\n");
}

/*BEGIN: MLFQ data structure/library*/
psjf_t* psjf_init(){
    //Initialize psjf queue
    psjf_t* s = (psjf_t*)malloc(sizeof(psjf_t));
    if (s == NULL){
		perror("Failed to allocate psjf");
		exit(1);
	}
    s->p_queue = queue_init();

    //Initialize ucontext of scheduler
    if(getcontext(&s->sched_ctx) < 0){
        perror("getcontext");
        return NULL;
    }
    //Allocate memory for scheduler stack
    s->sched_stack = (void*)malloc(STACK_SIZE);
    if (s->sched_stack == NULL){
		perror("Failed to allocate sched_stack");
		exit(1);
	}
    s->sched_ctx.uc_stack.ss_sp = s->sched_stack;
    s->sched_ctx.uc_stack.ss_size = STACK_SIZE;
    s->sched_ctx.uc_link = NULL;
    s->sched_ctx.uc_flags = 0;
    makecontext(&s->sched_ctx, (void *)&sched_psjf, 0);
    //Initialize sigaction handler
    memset(&s->sa, 0, sizeof(s->sa));
    s->sa.sa_sigaction = timer_handler;
    s->sa.sa_flags = SA_RESTART | SA_SIGINFO;
    sigemptyset(&s->sa.sa_mask);
    /*Set timer constants*/
    //Reset timer
    s->timer.it_interval.tv_usec = QUANTUM; // < seconds
	s->timer.it_interval.tv_sec = 0; // seconds
    //Timer goes off at
    s->timer.it_value.tv_usec = QUANTUM; // 10 milliseconds
	s->timer.it_value.tv_sec = 0;

    return s;
}

void psjf_destroy(psjf_t* s){
    queue_destroy(s->p_queue);
    free(s->sched_stack);
    free(s);
}

void timer_handler(int signum, siginfo_t* siginfo, void* sig) {
    total_quants++;
    if(scheduled != NULL){
        scheduled->elapsed += psjf->timer.it_value.tv_usec;
        if(scheduled->elapsed >= MAX_QUANTS*QUANTUM){
            total_quants = 0;
            worker_yield();
        } 
    }
}

