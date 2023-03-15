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
#define NUM_THREADS 10
worker_t t1, t2, t3, t4;
int x = 0;
int loop = 50000;

void add_counter() {
    int num = 10000;
    clock_t start = clock();
    int i;

    for(i = 0; i < num; i++){
	    x = x + 1;
        int t = 0;
        for(int n = 0; n < num; n++){
            t++;
        }
    }
    
    clock_t end = clock();
    double elapsed = (((double) (end - start)) / CLOCKS_PER_SEC)*1000.0;
    printf("Time measured: %.2f milliseconds.\n", elapsed);
    worker_exit(NULL);
}

int main(int argc, char** argv){
    worker_t* threads = (worker_t*)malloc(NUM_THREADS*sizeof(worker_t));

    for (int i = 0; i < NUM_THREADS; ++i){
        pthread_create(&threads[i], NULL, &add_counter, NULL);
    }
		

    for (int j = 0; j < NUM_THREADS; ++j){
        void *res;
        if(pthread_join(threads[j], &res) == 0){
            printf("joined thread %d with result %p\n", threads[j], res);
        }
    }
		

    printf("The final value of x is %d\n", x);

    return 0;
}
