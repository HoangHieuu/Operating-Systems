#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <limits.h>

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

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <n>\n", argv[0]);
        return 1;
    }

    long long n = 0;
    if (!parse_positive_ll(argv[1], &n)) {
        fprintf(stderr, "Error: <n> must be a positive integer.\n");
        return 1;
    }

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    long long sum = 0;
    for (long long i = 1; i <= n; ++i) {
        if (sum > LLONG_MAX - i) {
            fprintf(stderr, "Error: overflow while computing sum(1..%lld).\n", n);
            return 1;
        }
        sum += i;
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("Sum = %lld\n", sum);
    printf("Number of threads: 1\n");
    printf("Execution time: %.6f seconds\n", time);
    return 0;
}
