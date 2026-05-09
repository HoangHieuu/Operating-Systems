#ifndef FORKJOIN_H
#define FORKJOIN_H

#include <pthread.h>

typedef void * (*fj_func_t)(void *);

struct fj_task_t {
    fj_func_t func;
    void * arg;
    void * result;
    int task_id;
    int status;
    struct fj_task_t * next;
};

struct fj_context_t {
    struct fj_task_t * tasks;
    int total_tasks;
    int completed_tasks;
    pthread_mutex_t lock;
    pthread_cond_t cond;
};

struct fj_runner_arg_t {
    struct fj_context_t * ctx;
    struct fj_task_t * task;
};

int fj_context_init(struct fj_context_t * ctx);
int fj_fork(struct fj_context_t * ctx, fj_func_t func, void * arg);
int fj_join(struct fj_context_t * ctx);
void fj_context_destroy(struct fj_context_t * ctx);

#endif
