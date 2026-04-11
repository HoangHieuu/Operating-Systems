#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <limits.h>

typedef struct {
    long long start;
    long long end;
    long long *result_location; 
} ThreadArgs;

static int parse_positive_ll(const char *s, long long *out) {
    char *endptr = NULL;
    errno = 0;
    long long value = strtoll(s, &endptr, 10);

    if (errno != 0 || endptr == s || *endptr != '\0') {
        return 0;
    }
    if (value <= 0) {
        return 0;
    }

    *out = value;
    return 1;
}

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
        fprintf(stderr, "Usage: %s <numThreads> <n>\n", argv[0]);
        return 1;
    }

    long long numThreadsLL = 0;
    long long n = 0;
    if (!parse_positive_ll(argv[1], &numThreadsLL) || !parse_positive_ll(argv[2], &n)) {
        fprintf(stderr, "Error: <numThreads> and <n> must be positive integers.\n");
        return 1;
    }
    if (numThreadsLL > INT_MAX) {
        fprintf(stderr, "Error: <numThreads> is too large.\n");
        return 1;
    }
    int numThreads = (int) numThreadsLL;

    if (numThreads > n) {
        numThreads = (int) n;
    }
    
    struct timespec start_time, end_time;
    double time_taken;

    pthread_t *threads = malloc((size_t) numThreads * sizeof(*threads));
    ThreadArgs *thread_args = malloc((size_t) numThreads * sizeof(*thread_args));
    long long *partial_results = calloc((size_t) numThreads, sizeof(*partial_results));
    if (threads == NULL || thread_args == NULL || partial_results == NULL) {
        fprintf(stderr, "Error: failed to allocate memory for thread data.\n");
        free(threads);
        free(thread_args);
        free(partial_results);
        return 1;
    }
    
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
            for (int j = 0; j < i; j++) {
                pthread_join(threads[j], NULL);
            }
            free(threads);
            free(thread_args);
            free(partial_results);
            return 1;
        }
        
        current_start = current_end + 1;
    }

    //Waiting
    for (int i = 0; i < numThreads; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Cannot join thread");
            free(threads);
            free(thread_args);
            free(partial_results);
            return 1;
        }
    }
    for (int i = 0; i < numThreads; i++) {
        if (total_sum > LLONG_MAX - partial_results[i]) {
            fprintf(stderr, "Error: overflow while merging partial sums.\n");
            free(threads);
            free(thread_args);
            free(partial_results);
            return 1;
        }
        total_sum += partial_results[i];
    }

    //END_Cal

    clock_gettime(CLOCK_MONOTONIC, &end_time);
    
    time_taken = (end_time.tv_sec - start_time.tv_sec) * 1e9;
    time_taken = (time_taken + (end_time.tv_nsec - start_time.tv_nsec)) * 1e-9;

    printf("Sum = %lld\n", total_sum);
    printf("Number of threads: %d\n", numThreads);
    printf("Execution time: %.6f seconds\n", time_taken);

    free(threads);
    free(thread_args);
    free(partial_results);

    return 0;
}
