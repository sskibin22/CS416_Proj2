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

worker_t t1, t2, t3, t4;
int x = 0;
int loop = 10000;

void add_counter() {
    clock_t start = clock();
    int i;

    for(i = 0; i < 10000; i++){
	    x = x + 1;
        // int t = 0;
        // for(int n = 0; n < 10000; n++){
        //     t++;
        // }
    }
    worker_exit(NULL);
    clock_t end = clock();
    double elapsed = (((double) (end - start)) / CLOCKS_PER_SEC)*1000.0;
    printf("Time measured: %.2f milliseconds.\n", elapsed);
}

int main(int argc, char** argv){


    if(worker_create(&t1, NULL, &add_counter, NULL)){
        perror("ERROR: creating directory thread");
    }
    if(worker_create(&t2, NULL, &add_counter, NULL)){
        perror("ERROR: creating directory thread");
    }
    if(worker_create(&t3, NULL, &add_counter, NULL)){
        perror("ERROR: creating directory thread");
    }
    if(worker_create(&t4, NULL, &add_counter, NULL)){
        perror("ERROR: creating directory thread");
    }

    printf("t1 id: %i, t2 id: %i, t3 id: %i, t4 id: %i\n", t1, t2, t3, t4);

    if (worker_join(t1, NULL)) {
        perror("ERROR: joining directory thread");        
    }
    if (worker_join(t2, NULL)) {
        perror("ERROR: joining directory thread");        
    }
    if (worker_join(t3, NULL)) {
        perror("ERROR: joining directory thread");        
    }
    if (worker_join(t4, NULL)) {
        perror("ERROR: joining directory thread");        
    }

    printf("The final value of x is %d\n", x);

    return 0;
}
