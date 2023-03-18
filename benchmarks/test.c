#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "../thread-worker.h"

/* A scratch program template on which to call and
 * test thread-worker library functions as you implement
 * them.
 *
 * You can modify and use this program as much as possible.
 * This will not be graded.
 */
#define NUM_THREADS 6
pthread_mutex_t mutex;
int x = 0;

        
void add_counter() {
    struct timespec start, end;
    clock_gettime(CLOCK_REALTIME, &start);
    int loop = 30000;

    for(int i = 0; i < loop; i++){
        pthread_mutex_lock(&mutex);
	    x = x + 1;
        pthread_mutex_unlock(&mutex);
        int t = 0;
        for(int n = 0; n < loop; n++){
            t++;
        }
    }
    
    clock_gettime(CLOCK_REALTIME, &end);
    double elapsed = ((end.tv_sec - start.tv_sec) * 1000) + ((end.tv_nsec - start.tv_nsec) / 1000000);
    printf("Time measured: %.2f micro-seconds.\n", elapsed);
    pthread_exit(NULL);
    return NULL;
}

int main(int argc, char** argv){
    struct timespec start_m, end_m;

    pthread_t* threads = (pthread_t*)malloc(NUM_THREADS*sizeof(pthread_t));

    pthread_mutex_init(&mutex, NULL);
    clock_gettime(CLOCK_REALTIME, &start_m);
    for (int i = 0; i < NUM_THREADS; ++i){
        pthread_create(&threads[i], NULL, &add_counter, NULL);
    }
		

    for (int j = 0; j < NUM_THREADS; ++j){
        void *res;
        if(pthread_join(threads[j], &res) == 0){
            printf("joined thread %d with result %p\n", threads[j], res);
        }
    }	
    
    clock_gettime(CLOCK_REALTIME, &end_m);
    pthread_mutex_destroy(&mutex);

    printf("The final value of x is %d\n", x);
    print_app_stats();
    double elapsed = ((end_m.tv_sec - start_m.tv_sec)* 1000) + ((end_m.tv_nsec - start_m.tv_nsec) / 1000000);
    printf("Total Runtime: %.2f micro-seconds.\n", elapsed);

    return 0;
}
