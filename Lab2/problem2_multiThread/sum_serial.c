#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <n>\n", argv[0]);
        return 1;
    }
    long long n = atoll(argv[1]);

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    long long sum = 0;
    for (long long i = 1; i <= n; ++i) {
        sum += i;
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("Sum = %lld\n", sum);
    printf("Number of threads: 1\n");
    printf("Execution time: %.6f seconds\n", time);
    return 0;
}
