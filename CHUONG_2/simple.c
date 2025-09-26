#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <time.h>   // cần cho time()

int shared = 10;

void * fun(void * args){
    time_t start = time(NULL);
    time_t end = start + 5; // run for 5 seconds

    // YOUR-CODE-HERE
    while (time(NULL) < end) {
        shared++;
        // nếu muốn giảm tải CPU thì có thể:
        // usleep(1000); // ngủ 1ms
    }

    return NULL;
}

int main(){
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, fun, NULL);
    pthread_join(thread_id, NULL);

    printf("shared: %d\n", shared);
    return 0;
}

