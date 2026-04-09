#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

typedef struct {
    long long start;
    long long end;
    long long *result_location; 
} ThreadArgs;


void *sum_range(void *arg) {
    
    ThreadArgs *args = (ThreadArgs *)arg;
    
    long long partial_sum = 0;

    
    for (long long i = args->start; i <= args->end; i++) {
        partial_sum += i;
    }
   
    *(args->result_location) = partial_sum;
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "How to use: %s <numThreads> <n>\n", argv[0]);
        return 1;
    }

    int numThreads = atoi(argv[1]);
    long long n = atoll(argv[2]);

    if (n <= 0 || numThreads <= 0) {
        fprintf(stderr, "Error, nums of threads must be positive numbers.\n");
        return 1;
    }
    
    struct timespec start_time, end_time;
    double time_taken;

    pthread_t threads[numThreads];
    ThreadArgs thread_args[numThreads];
    
    long long partial_results[numThreads];

    long long total_sum = 0;

    clock_gettime(CLOCK_MONOTONIC, &start_time);

    //START_Cal

    long long chunk_size = n / numThreads;
    long long current_start = 1;
    for (int i = 0; i < numThreads; i++) {
        long long current_end = (i == numThreads - 1) ? n : (current_start + chunk_size - 1);

        thread_args[i].start = current_start;
        thread_args[i].end = current_end;
        thread_args[i].result_location = &partial_results[i];

        if (pthread_create(&threads[i], NULL, sum_range, (void *)&thread_args[i]) != 0) {
            perror("Cannot create thread");
            return 1;
        }
        
        current_start = current_end + 1;
    }

    //Waiting
    for (int i = 0; i < numThreads; i++) {
        pthread_join(threads[i], NULL);
    }
    for (int i = 0; i < numThreads; i++) {
        total_sum += partial_results[i];
    }

    //END_Cal

    clock_gettime(CLOCK_MONOTONIC, &end_time);
    
    time_taken = (end_time.tv_sec - start_time.tv_sec) * 1e9;
    time_taken = (time_taken + (end_time.tv_nsec - start_time.tv_nsec)) * 1e-9;

    printf("Sum = %lld\n", total_sum);
    printf("Numbers of threads : %d\n", numThreads);
    printf("Execution Time : %f seconds\n", time_taken);

    return 0;
}
