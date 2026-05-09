#include "forkjoin.h"
#include "bktpool.h"

#include <stdio.h>
#include <unistd.h>

static void * compute_chunk(void * arg)
{
    int chunk_id = *((int *) arg);

    printf("Computing chunk %d\n", chunk_id);
    fflush(stdout);
    sleep(1);

    return (void *) (long) (chunk_id * 10);
}

int main(void)
{
    struct fj_context_t ctx;
    int ids[4] = {0, 1, 2, 3};
    int i;

    taskid_seed = 0;
    wrkid_cur = 0;
    bktask_sz = 0;
    bktask = NULL;

    if (bktpool_init() != 0)
        return -1;

    if (fj_context_init(&ctx) != 0)
        return -1;

    for (i = 0; i < 4; i++) {
        if (fj_fork(&ctx, compute_chunk, &ids[i]) < 0) {
            printf("fj_fork failed for task %d\n", i);
            fj_context_destroy(&ctx);
            return -1;
        }
    }

    fj_join(&ctx);

    printf("All chunks done. Merging results.\n");
    for (struct fj_task_t * t = ctx.tasks; t != NULL; t = t->next) {
        printf("task %d result = %ld\n", t->task_id, (long) t->result);
    }

    fj_context_destroy(&ctx);
    return 0;
}
