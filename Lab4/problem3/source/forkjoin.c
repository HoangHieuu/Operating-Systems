#include "forkjoin.h"
#include "bktpool.h"

#include <stdlib.h>

static void fj_release_worker(unsigned int wrkid)
{
    if (wrkid >= MAX_WORKER)
        return;

    wrkid_busy[wrkid] = 0;
    worker[wrkid].func = NULL;
    worker[wrkid].arg = NULL;
    worker[wrkid].bktaskid = -1;
}

static void fj_unlink_task(struct fj_context_t * ctx, struct fj_task_t * task)
{
    struct fj_task_t ** cur = &ctx->tasks;

    while (*cur != NULL) {
        if (*cur == task) {
            *cur = task->next;
            ctx->total_tasks--;
            return;
        }

        cur = &(*cur)->next;
    }
}

int fj_context_init(struct fj_context_t * ctx)
{
    if (ctx == NULL)
        return -1;

    ctx->tasks = NULL;
    ctx->total_tasks = 0;
    ctx->completed_tasks = 0;

    if (pthread_mutex_init(&ctx->lock, NULL) != 0)
        return -1;

    if (pthread_cond_init(&ctx->cond, NULL) != 0) {
        pthread_mutex_destroy(&ctx->lock);
        return -1;
    }

    return 0;
}

static void fj_task_runner(void * arg)
{
    struct fj_runner_arg_t * runner = arg;
    struct fj_context_t * ctx;
    struct fj_task_t * task;

    if (runner == NULL)
        return;

    ctx = runner->ctx;
    task = runner->task;

    task->result = task->func(task->arg);

    pthread_mutex_lock(&ctx->lock);
    task->status = 1;
    ctx->completed_tasks++;
    pthread_cond_broadcast(&ctx->cond);
    pthread_mutex_unlock(&ctx->lock);

    free(runner);
}

int fj_fork(struct fj_context_t * ctx, fj_func_t func, void * arg)
{
    struct fj_task_t * task;
    struct fj_runner_arg_t * runner;
    unsigned int tid;
    int wid;
    int task_id;

    if (ctx == NULL || func == NULL)
        return -1;

    task = malloc(sizeof(struct fj_task_t));
    runner = malloc(sizeof(struct fj_runner_arg_t));
    if (task == NULL || runner == NULL) {
        free(task);
        free(runner);
        return -1;
    }

    wid = bkwrk_get_worker();
    if (wid < 0) {
        free(task);
        free(runner);
        return -1;
    }

    task->func = func;
    task->arg = arg;
    task->result = NULL;
    task->status = 0;
    task->next = NULL;

    runner->ctx = ctx;
    runner->task = task;

    if (bktask_init(&tid, fj_task_runner, runner) != 0) {
        free(task);
        free(runner);
        return -1;
    }

    if (bktask_assign_worker(tid, (unsigned int) wid) != 0) {
        free(task);
        free(runner);
        return -1;
    }

    pthread_mutex_lock(&ctx->lock);
    task->task_id = ctx->total_tasks++;
    task->next = ctx->tasks;
    ctx->tasks = task;
    task_id = task->task_id;
    pthread_mutex_unlock(&ctx->lock);

    if (bkwrk_dispatch_worker((unsigned int) wid) != 0) {
        pthread_mutex_lock(&ctx->lock);
        fj_unlink_task(ctx, task);
        pthread_mutex_unlock(&ctx->lock);

        fj_release_worker((unsigned int) wid);
        free(task);
        free(runner);
        return -1;
    }

    return task_id;
}

int fj_join(struct fj_context_t * ctx)
{
    if (ctx == NULL)
        return -1;

    pthread_mutex_lock(&ctx->lock);

    while (ctx->completed_tasks < ctx->total_tasks) {
        pthread_cond_wait(&ctx->cond, &ctx->lock);
    }

    pthread_mutex_unlock(&ctx->lock);
    return 0;
}

void fj_context_destroy(struct fj_context_t * ctx)
{
    struct fj_task_t * cur;

    if (ctx == NULL)
        return;

    cur = ctx->tasks;
    while (cur != NULL) {
        struct fj_task_t * next = cur->next;
        free(cur);
        cur = next;
    }

    ctx->tasks = NULL;
    ctx->total_tasks = 0;
    ctx->completed_tasks = 0;

    pthread_mutex_destroy(&ctx->lock);
    pthread_cond_destroy(&ctx->cond);
}
