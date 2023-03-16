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

#ifndef MLFQ
	// Choose PSJF
	#define SCHEDULER 1
#else 
	// Choose MLFQ
	#define SCHEDULER 0
#endif

/*Static Function Declarations*/
static void schedule();
static void sched_psjf();
static void sched_mlfq();
static int init_main_thread(void);
static void main_thread_func();
static queue_t* queue_init();
static int is_empty(queue_t* q);
static int mlfq_is_empty(queue_t** mq);
static node_t* node_init();
static void enqueue(tcb* t, queue_t* q);
static tcb* dequeue(queue_t* q);
static tcb* remove_at(worker_t id, queue_t* q);
static tcb *find_tcb(worker_t id, queue_t* q);
static worker_t get_min_elapsed(queue_t* q);
static int length(queue_t* q);
static void queue_destroy(queue_t* q);
static void queue_display(queue_t* q);
static void mlfq_display(queue_t** mq);
static sched_t* sched_init();
static void boost_mlfq(queue_t** mq);
static void sched_destroy(sched_t* s);
static void block_signal();
static void unblock_signal();
static void timer_handler(int signum, siginfo_t* siginfo, void* sig);
static int init_sig_timer();
static int power(int base, int exp);

/*Global Variables*/
int lib_active = 0;
int first_ctx = 0;
int id_count = 0;
sched_t* sched = NULL;
queue_t* exit_q;
tcb* scheduled = NULL;
tcb* main_tcb = NULL;
double total_turn_time = 0;
double total_resp_time = 0;
int total_exited = 0;
long s_sum = 0;
struct itimerval timer;
int prev_Q_mult = 1;
int curr_Q_mult = 1;


/* create a new thread */
int worker_create(worker_t * thread, pthread_attr_t * attr, 
                      void *(*function)(void*), void * arg) {

       // - create Thread Control Block (TCB)
       // - create and initialize the context of this worker thread
       // - allocate space of stack for this thread to run
       // after everything is set, push this thread into run queue and 
       // - make it ready for the execution.

       // YOUR CODE HERE

	/*Initialize scheduler/queues, main thread, and initialize timer if first call to library*/
	if(lib_active == 0){
        /*Initialize scheduler struct*/
        sched = sched_init();
        /*Initialize exit queue*/
		exit_q = queue_init();
        /*Initialize main thread (id = 0)*/
        init_main_thread();
        /*Initialize quantum timer*/
        init_sig_timer();
        lib_active = 1;
	}
    /*allocate space for tcb*/
	tcb* _tcb = (tcb*)calloc(1, sizeof(tcb));
	if (_tcb == NULL){
		perror("Failed to allocate tcb");
		return -1;
	}
    /*initialize tcb fields*/
	_tcb->t_id = id_count++;
	*thread = _tcb->t_id;
	_tcb->t_state = READY;
	_tcb->t_prio = 0;
	_tcb->elapsed = 0;
    _tcb->t_scheduled = 0;
    /*initialize tcb context*/
	if(getcontext(&_tcb->t_ctx) < 0){
		perror("getcontext");
		return -1;
	}
	_tcb->t_stack = (void*)malloc(STACK_SIZE);
	if (_tcb->t_stack == NULL){
		perror("Failed to allocate tcb->stack");
		exit(1);
	}
	_tcb->t_ctx.uc_stack.ss_sp = _tcb->t_stack;
	_tcb->t_ctx.uc_stack.ss_size = STACK_SIZE;
	_tcb->t_ctx.uc_link = &sched->sched_ctx;
	_tcb->t_ctx.uc_flags = 0;
	memset(_tcb->t_ctx.uc_stack.ss_sp, 0, STACK_SIZE);

	if(arg == NULL){
		makecontext(&_tcb->t_ctx, function, 0);
	}
	else{
		makecontext(&_tcb->t_ctx, function, 1, arg);
	}    
	
    /*enqueue tcb to ready queue*/
    if(SCHEDULER){
        enqueue(_tcb, sched->p_queue);
    }
    else{
        enqueue(_tcb, sched->p_queues[0]);
    }
	
    clock_gettime(CLOCK_REALTIME, &_tcb->arrival);

	return 0;
};

/* give CPU possession to other user-level worker threads voluntarily */
int worker_yield() {
	
	// - change worker thread's state from Running to Ready
	// - save context of this thread to its thread control block
	// - switch from thread context to scheduler context

	// YOUR CODE HERE
    tot_cntx_switches++;
    if(scheduled == NULL){
        if(setcontext(&sched->sched_ctx) < 0){
            return -1;
        }
    }

	if(swapcontext(&scheduled->t_ctx, &sched->sched_ctx) < 0){
        return -1;
    }

    return 0;
};

/* terminate a thread */
void worker_exit(void *value_ptr) {
	// - de-allocate any dynamic memory created when starting this thread
    
	// YOUR CODE HERE
    block_signal();
    total_exited++;
    scheduled->return_value = value_ptr;
    scheduled->t_state = EXITED;
    clock_gettime(CLOCK_REALTIME, &scheduled->completion);
    enqueue(scheduled, exit_q);
    if(scheduled->t_id != 0){
        total_resp_time += (scheduled->start.tv_sec - scheduled->arrival.tv_sec) * 1000 + (scheduled->start.tv_nsec - scheduled->arrival.tv_nsec) / 1000000;
        total_turn_time += (scheduled->completion.tv_sec - scheduled->arrival.tv_sec) * 1000 + (scheduled->completion.tv_nsec - scheduled->arrival.tv_nsec) / 1000000;
        avg_resp_time = total_resp_time / (double)total_exited;
        avg_turn_time = total_turn_time / (double)total_exited;
    }
    scheduled = NULL;
    unblock_signal();

};


/* Wait for thread termination */
int worker_join(worker_t thread, void **value_ptr) {
	
	// - wait for a specific thread to terminate
	// - de-allocate any dynamic memory created by the joining thread
  
	// YOUR CODE HERE
    if (thread < 0) {
        perror("No thread exists");
        return -1;
    }


    block_signal();
    tcb* temp = NULL;
    if(SCHEDULER){
        temp = find_tcb(thread, sched->p_queue);
    }
    else{
        for(int i = 0; i < MLFQ_LEVELS; i++){
            temp = find_tcb(thread, sched->p_queues[i]);
            if(temp != NULL){
                break;
            }
        }
    }
    unblock_signal();

    if(temp != NULL){
        while(temp->t_state != EXITED);
    }


    block_signal();
    tcb* exit_t = remove_at(thread, exit_q);
    
    if (exit_t == NULL) {
        return 0;
    } 
    else {
        if(value_ptr != NULL){
            *value_ptr = exit_t->return_value;
        }
        free(exit_t->t_stack);
        free(exit_t);
    }
    if(SCHEDULER){
        if(scheduled == NULL && (is_empty(sched->p_queue) && is_empty(exit_q))){
            sched_destroy(sched);
            queue_destroy(exit_q);
            lib_active = 0;
            first_ctx = 0;
        }
    }
    else{
        if(scheduled == NULL && (mlfq_is_empty(sched->p_queues) && is_empty(exit_q))){
            sched_destroy(sched);
            queue_destroy(exit_q);
            lib_active = 0;
            first_ctx = 0;
        }
    }
    unblock_signal();
	return 0;
};

/* initialize the mutex lock */
int worker_mutex_init(worker_mutex_t *mutex, 
                          const pthread_mutexattr_t *mutexattr) {
	//- initialize data structures for this mutex

	// YOUR CODE HERE
    // mutex->_lock = FREE;
    // mutex->_owner = NULL;

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
    if(getcontext(&sched->sched_ctx) < 0){
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

    /*No thread is currently scheduled (either because no thread has been scheduled yet or a worker thread has exited)*/
    // queue_display(sched->p_queue);
    // if(scheduled == NULL){
    //     printf("sched: NULL\n");
    // }
    // else{
    //     printf("sched: %i\n", scheduled->t_id);
    // }
    if(scheduled == NULL){
        /*only one thread in queue*/
        if(length(sched->p_queue) == 1){
            scheduled = dequeue(sched->p_queue);
            scheduled->t_state = SCHEDULED;
            if(scheduled->t_scheduled == 0){
                scheduled->t_scheduled = 1;
                clock_gettime(CLOCK_REALTIME, &scheduled->start);
            }
            tot_cntx_switches++;
            setcontext(&scheduled->t_ctx);
        }
        /*More than one thread in queue*/
        else if(length(sched->p_queue) > 1){
            //linear search for thread with minimum elapsed time
            worker_t min_elapsed = get_min_elapsed(sched->p_queue);
            //dequeue thread found in min search from queue
            scheduled = remove_at(min_elapsed, sched->p_queue);
            scheduled->t_state = SCHEDULED;
            if(scheduled->t_scheduled == 0){
                scheduled->t_scheduled = 1;
                clock_gettime(CLOCK_REALTIME, &scheduled->start);
            }
            tot_cntx_switches++;
            setcontext(&scheduled->t_ctx);
        }
    }
    /*One quantum has elapsed prior to the currently scheduled thread exiting*/
    else{
        scheduled->elapsed++;
        /*If queue is empty, continue to run currently scheduled thread*/
        if(is_empty(sched->p_queue)){
            // printf("id: %i, elapsed: %d quantums\n", scheduled->t_id, scheduled->elapsed);
            tot_cntx_switches++;
            setcontext(&scheduled->t_ctx);
        }
        /*If there is only one other thread in the queue simply enqueue() currently scheduled and dequeue() next thread to be scheduled*/
        else if(length(sched->p_queue) == 1){
            // printf("id: %i, elapsed: %d quantums\n", scheduled->t_id, scheduled->elapsed);
            scheduled->t_state = READY;
            enqueue(scheduled, sched->p_queue);
            scheduled = dequeue(sched->p_queue);
            scheduled->t_state = SCHEDULED;
            if(scheduled->t_scheduled == 0){
                scheduled->t_scheduled = 1;
                clock_gettime(CLOCK_REALTIME, &scheduled->start);
            }
            tot_cntx_switches++;
            setcontext(&scheduled->t_ctx);
        }
        /*If length of queue > 1:
         *1) enqueue() currently scheduled thread
         *2) find and schedule thread with the minimum elapsed quantums
        */
        else{
            // printf("id: %i, elapsed: %d quantums\n", scheduled->t_id, scheduled->elapsed);
            scheduled->t_state = READY;
            enqueue(scheduled, sched->p_queue);
            worker_t min_elapsed = get_min_elapsed(sched->p_queue);
            scheduled = remove_at(min_elapsed, sched->p_queue);
            scheduled->t_state = SCHEDULED;
            if(scheduled->t_scheduled == 0){
                scheduled->t_scheduled = 1;
                clock_gettime(CLOCK_REALTIME, &scheduled->start);
            }
            tot_cntx_switches++;
            setcontext(&scheduled->t_ctx);
        }
        
    }
    
}


/* Preemptive MLFQ scheduling algorithm */
static void sched_mlfq() {
	// - your own implementation of MLFQ
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
    mlfq_display(sched->p_queues);

    if(scheduled != NULL){
        if(scheduled->t_prio < (MLFQ_LEVELS - 1)){
            scheduled->t_prio++;
        }
        scheduled->t_state = READY;
        enqueue(scheduled, sched->p_queues[scheduled->t_prio]);
    }
    tcb* temp = NULL;
    for(int i = 0; i < MLFQ_LEVELS; i++){
        if(!is_empty(sched->p_queues[i])){
            temp = dequeue(sched->p_queues[i]);
            curr_Q_mult = (i+1);
            if(curr_Q_mult != prev_Q_mult){
                prev_Q_mult = curr_Q_mult;
                timer.it_interval.tv_sec = 0;
                timer.it_interval.tv_usec = QUANTUM*(power(curr_Q_mult, 2));
                timer.it_value.tv_sec = 0;
                timer.it_value.tv_usec = 1;
                if (setitimer(ITIMER_PROF, &timer, NULL) == - 1) {
                    perror("setitimer");
	            }
            }
            break;
        }
    }
    if(temp != NULL){
        scheduled = temp;
        scheduled->t_state = SCHEDULED;
        if(scheduled->t_scheduled == 0){
            scheduled->t_scheduled = 1;
            clock_gettime(CLOCK_REALTIME, &scheduled->start);
        }
        tot_cntx_switches++;
        setcontext(&scheduled->t_ctx);
    }
    
    
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
/*Main Benchmark Thread initialize function*/
static int init_main_thread()
{
    tcb* m_tcb = (tcb*)calloc(1, sizeof(tcb));
    if (m_tcb == NULL){
        perror("Failed to allocate tcb");
        return -1;
    }
    /*initialize tcb fields*/
    m_tcb->t_id = id_count++;
    m_tcb->t_state = MAIN;
    m_tcb->t_prio = 0;
    m_tcb->elapsed = 0;
    m_tcb->t_scheduled = 1;
    /*initialize tcb context*/
    if(getcontext(&m_tcb->t_ctx) < 0){
        perror("getcontext");
        return -1;
    }
    m_tcb->t_stack = (void*)malloc(STACK_SIZE);
    if (m_tcb->t_stack == NULL){
        perror("Failed to allocate tcb->stack");
        exit(1);
    }
    m_tcb->t_ctx.uc_stack.ss_sp = m_tcb->t_stack;
    m_tcb->t_ctx.uc_stack.ss_size = STACK_SIZE;
    m_tcb->t_ctx.uc_link = &sched->sched_ctx; //NULL
    m_tcb->t_ctx.uc_flags = 0;
    memset(m_tcb->t_ctx.uc_stack.ss_sp, 0, STACK_SIZE);
    makecontext(&m_tcb->t_ctx, main_thread_func, 0);
    main_tcb = m_tcb;
    scheduled = m_tcb;
    return 0;
}

static void main_thread_func(){
    worker_exit(NULL);
    return;
}

/*queue functions*/
queue_t* queue_init(){
    queue_t* q = calloc(1, sizeof(queue_t));
    q->head = NULL;
    q->tail = NULL;
    q->length = 0;
    return q;
}

static int is_empty(queue_t* q) {
    return q->head == NULL;
}

static int mlfq_is_empty(queue_t** mq){
    for(int i = 0; i < MLFQ_LEVELS; i++){
        if(!is_empty(mq[i])){
            return 0;
        }
    }
    return 1;
}

static node_t* node_init(tcb* t){
    node_t* n = calloc(1, sizeof(node_t));
    n->data = t;
    n->next = NULL;
    return n;
}

static void enqueue(tcb* t, queue_t* q){
    node_t* n = node_init(t);
    if(is_empty(q)){
        q->head = q->tail = n;
    }
    else{
        q->tail->next = n;
        q->tail = n;
    }
    q->length++;
}

static tcb* dequeue(queue_t* q){
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
        q->length--;
        return t_temp;
    }
    return NULL;
}

static tcb* remove_at(worker_t id, queue_t* q){
    tcb* r = NULL;
    node_t* prev = NULL;
    node_t* curr = q->head;
    //first node is min elapsed
    if(curr->data->t_id == id){
        r = dequeue(q);
        return r;
    }
    else{
        prev = curr;
        curr = curr->next;
        while(curr != NULL){
            if(curr->data->t_id == id){
                r = curr->data;
                if(curr == q->tail){
                    q->tail = prev;
                    prev->next = NULL;
                }
                else{
                    prev->next = curr->next;
                }
                free(curr);
                q->length--;
                return r;
            }
            prev = curr;
            curr = curr->next;
        }
    }
    return r;
}

static tcb *find_tcb(worker_t id, queue_t* q){
    node_t *temp = q->head;
    while (temp != NULL) {
        if (temp->data->t_id == id) {
            return temp->data;
        }
	    temp = temp->next;
    }
    return NULL;
}

static worker_t get_min_elapsed(queue_t* q){
    node_t* temp = q->head;
    int min = 0;
    worker_t min_worker = 0;
    if(temp != NULL){
        min = temp->data->elapsed;
        min_worker = temp->data->t_id;
        temp = temp->next;
    }
    while(temp != NULL){
        if(temp->data->elapsed < min){
            min = temp->data->elapsed;
            min_worker = temp->data->t_id;
        }
        temp = temp->next;
    }
    return min_worker;
}


static int length(queue_t* q){
    return q->length;
}

static void queue_destroy(queue_t* q){
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
static void queue_display(queue_t* q){
    printf("Queue: ");
    node_t* temp = q->head;
    while(temp != NULL){
        printf("id: %d | ", temp->data->t_id);
        temp = temp->next;
    }
    printf("\n");
}

static void mlfq_display(queue_t** mq){
    for(int i = 0; i < MLFQ_LEVELS; i++){
        printf("Queue: %d |", i+1);
        node_t* temp = mq[i]->head;
        while(temp != NULL){
            printf("id: %d | ", temp->data->t_id);
            temp = temp->next;
        }
        printf("\n");
    }
    printf("-------------------------------------------\n");
}

/*Scheduler functions*/
static sched_t* sched_init(){
    //Initialize psjf queue
    sched_t* s = (sched_t*)calloc(1, sizeof(sched_t));
    if (s == NULL){
		perror("Failed to allocate sched");
		exit(1);
	}
    if(SCHEDULER){
        s->p_queue = queue_init();
        s->p_queues = NULL;
    }
    else{
        s->p_queues = (queue_t**)calloc(MLFQ_LEVELS, sizeof(queue_t*));
        for(int i = 0; i < MLFQ_LEVELS; i++){
            queue_t* queue = queue_init();
            s->p_queues[i] = queue;
        }
        s->p_queue = NULL;
    }
    

    // Initialize ucontext of scheduler
    if(getcontext(&s->sched_ctx) < 0){
        perror("getcontext");
        return NULL;
    }
    // Allocate memory for scheduler stack
    s->sched_stack = (void*)malloc(STACK_SIZE);
    if (s->sched_stack == NULL){
		perror("Failed to allocate sched_stack");
		exit(1);
	}
    s->sched_ctx.uc_stack.ss_sp = s->sched_stack;
    s->sched_ctx.uc_stack.ss_size = STACK_SIZE;
    s->sched_ctx.uc_link = NULL;
    s->sched_ctx.uc_flags = 0;
    memset(s->sched_ctx.uc_stack.ss_sp, 0, STACK_SIZE);
    makecontext(&s->sched_ctx, schedule, 0);

    return s;
}
static void boost_mlfq(queue_t** mq){
    for(int i = 1; i < MLFQ_LEVELS; i++){
        while(!is_empty(mq[i])){
            tcb* t = dequeue(mq[i]);
            t->t_prio = 0;
            t->elapsed = 0;
            enqueue(t, mq[0]);
        }
    }
}

static void sched_destroy(sched_t* s){
    if(SCHEDULER){
        queue_destroy(s->p_queue);
    }
    else{
        for(int i = MLFQ_LEVELS-1; i >= 0; i--){
            queue_destroy(s->p_queues[i]);
        }
        free(s->p_queues);
    }
    free(s->sched_stack);
    free(s); 
}

/*Signal/Timer functions*/
static void block_signal()
{
    sigset_t sm;
    sigemptyset(&sm);
    sigaddset(&sm, SIGPROF);
    if (sigprocmask(SIG_BLOCK, &sm, NULL) == -1) {
	    perror("sigprocmask");
	    abort();
    }
}

static void unblock_signal()
{
    sigset_t sm;
    sigemptyset(&sm);
    sigaddset(&sm, SIGPROF);
    if (sigprocmask(SIG_UNBLOCK, &sm, NULL) == -1) {
	    perror("sigprocmask");
	    abort();
    }
}

static void timer_handler(int signum, siginfo_t* siginfo, void* ctx) {
    if(!SCHEDULER){
        getitimer(ITIMER_PROF, &timer);
        s_sum += ((timer.it_value.tv_sec * 1000) + (timer.it_value.tv_usec / 1000));
        if(s_sum >= S){
            s_sum = 0;
            boost_mlfq(sched->p_queues);
        }
    }
    worker_yield();
}

static int init_sig_timer(){
    
    struct sigaction sa; 
    memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = timer_handler;
    sa.sa_flags = SA_SIGINFO | SA_RESTART;
	sigemptyset(&sa.sa_mask);

    if (sigaction(SIGPROF, &sa, NULL) == -1) {
	    perror("sigaction");
	    abort();
    }

    timer.it_interval.tv_usec = QUANTUM; // 10 milliseconds
	timer.it_interval.tv_sec = 0; // 0 seconds

    timer.it_value.tv_usec = 1; // 1 microsecond
	timer.it_value.tv_sec = 0; // 0 seconds


    if (setitimer(ITIMER_PROF, &timer, NULL) == - 1) {
	    return -1;
    }

    return 0;
}

static int power(int base, int exp) {
    if (exp == 0)
        return 1;
    else if (exp % 2)
        return base * power(base, exp - 1);
    else {
        int temp = power(base, exp / 2);
        return temp * temp;
    }
}

