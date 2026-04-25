#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <limits.h>
#include "utils.h"

#define THRSL_MIN 5

int processopts(int argc, char **argv, struct _appconf *conf)
{
    if (argc < 3 || argc > 4)
        return -1;

    if (tonum(argv[1], &conf->arrsz) != 0)
        return -1;

    if (tonum(argv[2], &conf->tnum) != 0)
        return -1;

    conf->seednum = (argc == 4) ? atoi(argv[3]) : SEEDNO;

    return 0;
}

int tonum(const char *nptr, int *num)
{
    long val;
    char *endptr;

    if (nptr == NULL || *nptr == '\0')
        return -1;

    errno = 0;
    val = strtol(nptr, &endptr, 10);

    if (errno == ERANGE || val > INT_MAX || val < INT_MIN)
        return -1;

    if (endptr == nptr || *endptr != '\0')
        return -1;

    *num = (int)val;
    return 0;
}

int validate_and_split_argarray(int arraysize, int num_thread, struct _range *thread_idx_range)
{
    int chunk_size;
    int i;

    if (arraysize < THRSL_MIN * num_thread)
        return -1;

    chunk_size = arraysize / num_thread;

    for (i = 0; i < num_thread; i++) {
        thread_idx_range[i].start = i * chunk_size;
        thread_idx_range[i].end   = thread_idx_range[i].start + chunk_size - 1;
    }

    thread_idx_range[num_thread - 1].end = arraysize - 1;

    return 0;
}

int generate_array_data(int *buf, int arraysize, int seednum)
{
    int i;

    srand((unsigned int)seednum);

    for (i = 0; i < arraysize; i++) {
        buf[i] = rand() % (UPBND_DATA_VAL - LWBND_DATA_VAL + 1) + LWBND_DATA_VAL;
    }

    return 0;
}

void help(int xcode)
{
    fprintf(stderr, "%s, %s\n\n", PACKAGE, VERSION);
    fprintf(stderr, "usage:  %s %s %s [%s]\n\n", PACKAGE, ARG1, ARG2, ARG3);
    fprintf(stderr, "Generate randomly integer array size <%s> and calculate sum\n", ARG1);
    fprintf(stderr, "parallelly using <%s> threads.\n", ARG2);
    fprintf(stderr, "The optional <%s> value use to control the randomization of the generated array.\n\n", ARG3);
    fprintf(stderr, "Arguments:\n");
    fprintf(stderr, "    %-12s specifies the size of array.\n", ARG1);
    fprintf(stderr, "    %-12s number of parallel threads.\n", ARG2);
    fprintf(stderr, "    %-12s initialize the state of the randomized generator.\n", ARG3);
    exit(xcode);
}
