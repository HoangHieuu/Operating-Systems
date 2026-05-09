#define _GNU_SOURCE
#include "bktpool.h"
#include <signal.h>
#include <stdio.h>
#include <sched.h>
#include <linux/sched.h>
#include <sys/syscall.h>
#include <unistd.h>

// #define DEBUG
#define INFO
#define WORK_THREAD

int bkwrk_worker(void * arg)
{
    sigset_t set;
    int sig;
    int s;
    int i = *((int *) arg); // Default arg is integer of workid
    struct bkworker_t * wrk = &worker[i];

    /* Taking the mask for waking up */
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGQUIT);

#ifdef DEBUG
    fprintf(stderr, "worker %i start living tid %d \n", i, getpid());
    fflush(stderr);
#endif

    while (1) {
        /* wait for signal */
        s = sigwait(&set, &sig);
        if (s != 0)
            continue;

#ifdef INFO
        fprintf(stderr, "worker wake %d up\n", i);
#endif

        /* Busy running */
        if (wrk -> func != NULL)
            wrk -> func(wrk -> arg);

        /* Advertise I DONE WORKING */
        wrkid_busy[i] = 0;
        worker[i].func = NULL;
        worker[i].arg = NULL;
        worker[i].bktaskid = -1;
    }

    return 0;
}

int bktask_assign_worker(unsigned int bktaskid, unsigned int wrkid)
{
    if (wrkid >= MAX_WORKER)
        return -1;

    struct bktask_t * tsk = bktask_get_byid(bktaskid);
    if (tsk == NULL)
        return -1;

    /* Advertise I AM WORKING */
    wrkid_busy[wrkid] = 1;

    worker[wrkid].func = tsk -> func;
    worker[wrkid].arg = tsk -> arg;
    worker[wrkid].bktaskid = bktaskid;

    printf("Assign tsk %d wrk %d \n", tsk -> bktaskid, wrkid);
    return 0;
}

int bkwrk_create_worker()
{
    unsigned int i;

    for (i = 0; i < MAX_WORKER; i++) {
#ifdef WORK_THREAD
        void * child_stack = malloc(STACK_SIZE);
        if (child_stack == NULL)
            return -1;

        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, SIGQUIT);
        sigaddset(&set, SIGUSR1);
        sigprocmask(SIG_BLOCK, &set, NULL);

        /* Stack grows downward -- start at top of allocation */
        void * stack_top = (char *) child_stack + STACK_SIZE;

        wrkid_tid[i] = clone(&bkwrk_worker, stack_top,
                              CLONE_VM | CLONE_FILES,
                              (void *) &i);
#ifdef INFO
        fprintf(stderr, "bkwrk_create_worker got worker %u\n", wrkid_tid[i]);
#endif

        usleep(100);

#else

        /* TODO: Implement fork version of create worker */

#endif
    }

    return 0;
}

int bkwrk_get_worker()
{
    /*
     * FIFO Scheduler Policy:
     * Start searching from wrkid_cur and wrap around.
     * Return the first worker whose wrkid_busy[i] == 0 (not busy).
     * This gives a FIFO-like assignment: earliest available worker gets the task.
     */
    unsigned int i;

    for (i = 0; i < MAX_WORKER; i++) {
        unsigned int idx = (wrkid_cur + i) % MAX_WORKER;
        if (wrkid_busy[idx] == 0) {
            wrkid_cur = (idx + 1) % MAX_WORKER;
            return idx;
        }
    }

    return -1; /* All workers are busy */
}

int bkwrk_dispatch_worker(unsigned int wrkid) 
{
    if (wrkid >= MAX_WORKER)
        return -1;

#ifdef WORK_THREAD
    unsigned int tid = wrkid_tid[wrkid];

    /* Invalid task */
    if (worker[wrkid].func == NULL)
        return -1;

#ifdef DEBUG
    fprintf(stderr, "brkwrk dispatch wrkid %d - send signal %u \n", wrkid, tid);
#endif

    syscall(SYS_tkill, tid, SIG_DISPATCH);

#else
    /* TODO: Implement fork version to signal worker process here */

#endif
    return 0;
}
