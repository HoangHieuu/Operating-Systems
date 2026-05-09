#define _GNU_SOURCE
#include "bktpool.h"
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sched.h>
#include <linux/sched.h>

/* Choose implementation: fork (process) vs clone (thread). */
/* #define WORK_THREAD */

struct shared_worker_t {
    volatile int busy;
    volatile int active;
    unsigned int bktaskid;
    void (*func)(void * arg);
    int has_arg;
    int arg_int;
} * shared_workers;

int bkwrk_worker(void * arg)
{
    sigset_t set;
    int sig;
    int s;
    int i = *((int *) arg);
    struct bkworker_t * wrk = &worker[i];

    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGQUIT);

    while (1) {
        s = sigwait(&set, &sig);
        if (s != 0)
            continue;

        if (sig == SIGQUIT)
            break;

        fprintf(stderr, "worker wake %d up\n", i);

        if (wrk -> func != NULL)
            wrk -> func(wrk -> arg);

        wrkid_busy[i] = 0;
        worker[i].func = NULL;
        worker[i].arg = NULL;
        worker[i].bktaskid = -1;
    }

    return 0;
}

#ifndef WORK_THREAD

static int fork_worker_main(void * arg)
{
    int i = *((int *) arg);
    sigset_t set;

    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGQUIT);

    fprintf(stderr, "fork worker %d started (pid %d)\n", i, getpid());
    fflush(stderr);

    while (1) {
        int sig;
        int s = sigwait(&set, &sig);
        if (s != 0)
            continue;

        if (sig == SIGQUIT)
            _exit(0);

        fprintf(stderr, "fork worker %d woke up\n", i);

        if (shared_workers[i].active && shared_workers[i].func != NULL) {
            void * argp = NULL;

            /*
             * Parent and child do not share the parent's stack after fork().
             * The lab sample passes int pointers, so copy that integer into
             * the shared command slot before waking the worker.
             */
            if (shared_workers[i].has_arg)
                argp = (void *) &shared_workers[i].arg_int;

            shared_workers[i].func(argp);

            shared_workers[i].active = 0;
            shared_workers[i].busy = 0;
            write(worker_pipe_write[i], &i, sizeof(int));
        }
    }
}

int bkwrk_create_worker()
{
    unsigned int i;

    shared_workers = mmap(NULL, sizeof(struct shared_worker_t) * MAX_WORKER,
                          PROT_READ | PROT_WRITE,
                          MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (shared_workers == MAP_FAILED) {
        perror("mmap");
        return -1;
    }

    memset(shared_workers, 0, sizeof(struct shared_worker_t) * MAX_WORKER);

    for (i = 0; i < MAX_WORKER; i++) {
        int pipefd[2];
        sigset_t set;
        pid_t pid;

        if (pipe(pipefd) == -1) {
            perror("pipe");
            return -1;
        }

        worker_pipe_read[i] = pipefd[0];
        worker_pipe_write[i] = pipefd[1];

        sigemptyset(&set);
        sigaddset(&set, SIGQUIT);
        sigaddset(&set, SIGUSR1);
        sigprocmask(SIG_BLOCK, &set, NULL);

        pid = fork();
        if (pid < 0) {
            perror("fork");
            return -1;
        }

        if (pid == 0) {
            int wid = i;

            close(worker_pipe_read[i]);
            fork_worker_main(&wid);
            _exit(0);
        }

        close(worker_pipe_write[i]);
        wrkid_tid[i] = pid;
        wrkid_busy[i] = 0;
        fprintf(stderr, "bkwrk_create_worker (fork) got worker %u\n", pid);
    }

    return 0;
}

int bkwrk_get_worker()
{
    unsigned int i;

    for (i = 0; i < MAX_WORKER; i++) {
        unsigned int idx = (wrkid_cur + i) % MAX_WORKER;

        if (shared_workers[idx].busy == 0) {
            wrkid_busy[idx] = 0;
            wrkid_cur = (idx + 1) % MAX_WORKER;
            return idx;
        }
    }

    return -1;
}

int bktask_assign_worker(unsigned int bktaskid, unsigned int wrkid)
{
    struct bktask_t * tsk;

    if (wrkid >= MAX_WORKER || shared_workers == NULL)
        return -1;

    tsk = bktask_get_byid(bktaskid);
    if (tsk == NULL)
        return -1;

    worker[wrkid].func = tsk -> func;
    worker[wrkid].arg = tsk -> arg;
    worker[wrkid].bktaskid = bktaskid;

    shared_workers[wrkid].bktaskid = bktaskid;
    shared_workers[wrkid].func = tsk -> func;
    shared_workers[wrkid].has_arg = (tsk -> arg != NULL);
    if (tsk -> arg != NULL)
        shared_workers[wrkid].arg_int = *((int *) tsk -> arg);
    shared_workers[wrkid].active = 1;
    shared_workers[wrkid].busy = 1;
    wrkid_busy[wrkid] = 1;

    printf("Assign tsk %d wrk %d \n", tsk -> bktaskid, wrkid);
    return 0;
}

int bkwrk_dispatch_worker(unsigned int wrkid)
{
    pid_t pid;

    if (wrkid >= MAX_WORKER || shared_workers == NULL)
        return -1;

    pid = wrkid_tid[wrkid];
    if (shared_workers[wrkid].func == NULL)
        return -1;

    fprintf(stderr, "bkwrk_dispatch (fork) wrkid %d - send SIGUSR1 to pid %d\n",
            wrkid, pid);

    return kill(pid, SIGUSR1);
}

int bkwrk_wait_worker(unsigned int wrkid)
{
    int wid_check;

    if (wrkid >= MAX_WORKER)
        return -1;

    if (read(worker_pipe_read[wrkid], &wid_check, sizeof(int)) > 0) {
        fprintf(stderr, "worker %d completed\n", wid_check);
        return 0;
    }

    return -1;
}

#else /* WORK_THREAD version */

int bkwrk_create_worker()
{
    unsigned int i;

    for (i = 0; i < MAX_WORKER; i++) {
        void * child_stack = malloc(STACK_SIZE);
        sigset_t set;
        void * stack_top;

        if (child_stack == NULL)
            return -1;

        sigemptyset(&set);
        sigaddset(&set, SIGQUIT);
        sigaddset(&set, SIGUSR1);
        sigprocmask(SIG_BLOCK, &set, NULL);

        stack_top = (char *) child_stack + STACK_SIZE;

        wrkid_tid[i] = clone(&bkwrk_worker, stack_top,
                              CLONE_VM | CLONE_FILES,
                              (void *) &i);
        fprintf(stderr, "bkwrk_create_worker got worker %u\n", wrkid_tid[i]);
        usleep(100);
    }

    return 0;
}

int bkwrk_get_worker()
{
    unsigned int i;

    for (i = 0; i < MAX_WORKER; i++) {
        unsigned int idx = (wrkid_cur + i) % MAX_WORKER;
        if (wrkid_busy[idx] == 0) {
            wrkid_cur = (idx + 1) % MAX_WORKER;
            return idx;
        }
    }

    return -1;
}

int bktask_assign_worker(unsigned int bktaskid, unsigned int wrkid)
{
    struct bktask_t * tsk;

    if (wrkid >= MAX_WORKER)
        return -1;

    tsk = bktask_get_byid(bktaskid);
    if (tsk == NULL)
        return -1;

    wrkid_busy[wrkid] = 1;

    worker[wrkid].func = tsk -> func;
    worker[wrkid].arg = tsk -> arg;
    worker[wrkid].bktaskid = bktaskid;

    printf("Assign tsk %d wrk %d \n", tsk -> bktaskid, wrkid);
    return 0;
}

int bkwrk_dispatch_worker(unsigned int wrkid)
{
    unsigned int tid;

    if (wrkid >= MAX_WORKER)
        return -1;

    tid = wrkid_tid[wrkid];
    if (worker[wrkid].func == NULL)
        return -1;

    syscall(SYS_tkill, tid, SIG_DISPATCH);
    return 0;
}

#endif /* WORK_THREAD */
